/*
tab_device
CREATE TABLE tab_device(
	id 				INTEGER PRIMARY KEY 	AUTOINCREMENT,
	create_time		DATETIME 				NOT NULL,
	update_time		DATETIME 				NOT NULL,
	name           	VARCHAR(255), 			DEFAULT 'unknown',
	uid				CHAR(255)				NOT NULL UNIQUE,
	type 			INTEGER					DEFAULT 0,
   	ip 				CHAR(64)				DEFAULT '0.0.0.0',
   	port 			INTEGER					DEFAULT 0,
	category 		CHAR(64)				DEFAULT 'elektra',
	state 			INTEGER					DEFAULT 0,
   	mode           	INTEGER					DEFAULT 0,
   	token 			VARCHAR(255)
);

tab_event
CREATE TABLE tab_event(
	id 				INTEGER PRIMARY KEY 	AUTOINCREMENT,
	create_time		DATETIME 				NOT NULL,
	type 			INTEGER					DEFAULT 0,
	name           	INTEGER,
	description		TEXT,
	device_id       INTEGER,
	FOREIGN KEY(device_id) references tab_device(id) on delete cascade on update cascade
);

tab_media
CREATE TABLE tab_media(
	id 				INTEGER PRIMARY KEY 	AUTOINCREMENT,
	create_time		DATETIME 				NOT NULL,
	type 			INTEGER					DEFAULT 0,
	num 			INTEGER,
	rel_url 		TEXT					NOT NULL,
	event_id        INTEGER,
	FOREIGN KEY(event_id) references tab_event(id) on delete cascade on update cascade
);
*/

var fs = require('fs'),
	util = require('util'),
	path = require('path'),
	async = require('async'),
	exec = require('child_process').exec,
	sqlite3 = require('sqlite3'),
	defs = require('./defs'),
	config = require('./config');

const TAG = 'db';
var db = undefined;
var deviceCache = {};

const MSG = defs.MSG;
const DEVICE_TYPE = defs.DEVICE_TYPE;
const DEVICE_STATE = defs.DEVICE_STATE;
const DEVICE_MODE = defs.DEVICE_MODE;
const EVENT_TYPE = defs.EVENT_TYPE;
const MEDIA_TYPE = defs.MEDIA_TYPE;

const DEFAULT_DEVICE_NAME = 'camera';
const DEFAULT_DEVICE_TYPE = DEVICE_TYPE.CAMERA;
const DEFAULT_EVENT_TYPE = EVENT_TYPE.COMMON;

const DEFAULT_TAB_DEVICE_NAME = 'tab_device';
const DEFAULT_TAB_EVENT_NAME = 'tab_event';
const DEFAULT_TAB_MEDIA_NAME = 'tab_media';
const devicesPath = path.join(config.publicPath, config.device.rootDir);

var q = async.queue(function(task, callback) {
    task.run(task.context, callback);
}, 1);

function forAll(tab_name, callback) {
	var statement = util.format('SELECT * FROM %s', tab_name);
	db.all(statement, callback);
}

function forEach(tab_name, doEach, done) {
	var statement = util.format('SELECT * FROM %s', tab_name);
    db.each(statement, function(err, row) {
        if (err){
            util.log('FAIL to retrieve row ' + err);
            done(err, null);
        } else {
            doEach(null, row);
        }
    }, done);
}

function forAllByKV(tab_name, keys, values, callback) {
	if (keys.length == 0 || keys.length != values.length) {
		util.log(util.format('[%s] forAllByKV invalid params - keys:%s values:%s', TAG, keys, values));
		return ;
	}

	var statement = util.format('SELECT * FROM %s WHERE', tab_name);
	for (var i = 0; i < keys.length; i++) {
		if (i != 0) { statement += ','; }
		statement += util.format(' %s = ?', keys[i]);
	}
	db.all(statement, values, callback);
}

function forEachByKV(tab_name, keys, values, doEach, done) {
	if (keys.length == 0 || keys.length != values.length) {
		util.log(util.format('[%s] forEachByKV invalid params - keys:%s values:%s', TAG, keys, values));
		return ;
	}

	var statement = util.format('SELECT * FROM %s WHERE', tab_name);
	for (var i = 0; i < keys.length; i++) {
		if (i != 0) { statement += ','; }
		statement += util.format(' %s = ?', keys[i]);
	}

    db.each(statement, values, function(err, row) {
        if (err){
            util.log('FAIL to retrieve row ' + err);
            done(err, null);
        } else {
            doEach(null, row);
        }
    }, done);
}

function itemInsert(tab_name, keys, values, callback) {
	if (keys.length == 0 || keys.length != values.length) {
		util.log(util.format('[%s] itemInsert invalid params - keys:%s values:%s', TAG, keys, values));
		return ;
	}

	var statement = util.format('INSERT INTO %s (', tab_name);
	for (var i = 0; i < keys.length; i++) {
		if (i != 0) { statement += ', '; }
		statement += util.format('%s', keys[i]);
	}
	statement += ') VALUES (';
	for (var i = 0; i < keys.length; i++) {
		if (i != 0) { statement += ', '; }
		statement += '?';
	}
	statement += ');';

	db.run(statement, values, callback);
}

function itemDeleteAll(tab_name, callback) {
	var statement = util.format('DELETE FROM %s', tab_name);
	db.run(statement, callback);
}

function itemDelete(tab_name, keys, values, callback) {
	if (keys.length == 0 || keys.length != values.length) {
		util.log(util.format('[%s] itemDelete invalid params - keys:%s values:%s', TAG, keys, values));
		return ;
	}

	var statement = util.format('DELETE FROM %s WHERE', tab_name);
	for (var i = 0; i < keys.length; i++) {
		if (i != 0) { statement += ','; }
		statement += util.format(' %s = ?', keys[i]);
	}
	db.run(statement, values, callback);
}

function itemUpdate(tab_name, setKeys, setValues, whereKeys, whereValues, callback) {
	if (setKeys.length == 0 || setKeys.length != setValues.length) {
		util.log(util.format('[%s] itemUpdate invalid params - setKeys:%s setValues:%s', TAG, setKeys, setValues));
		return ;
	}

	if (whereKeys.length == 0 || whereKeys.length != whereValues.length) {
		util.log(util.format('[%s] itemUpdate invalid params - whereKeys:%s whereValues:%s', TAG, whereKeys, whereValues));
		return ;
	}


	var statement = util.format('UPDATE %s SET', tab_name);
	for (var i = 0; i < setKeys.length; i++) {
		if (i != 0) { statement += ','; }
		statement += util.format(' %s = ?', setKeys[i]);
	}

	statement += ' WHERE';
	for (var i = 0; i < whereKeys.length; i++) {
		if (i != 0) { statement += ','; }
		statement += util.format(' %s = ?', whereKeys[i]);
	}

	var values = setValues.concat(whereValues);
	db.run(statement, values, callback);
}

function deviceAdd(name, uid, type, category, callback) {
	util.log(util.format('deviceAdd: name:%s uid:%s category:%s', name, uid, category));

	var date = new Date();
	itemInsert(DEFAULT_TAB_DEVICE_NAME,
		['create_time', 'update_time', 'name', 'uid', 'type', 'category'],
		[date, date, name, uid, type, category],
		callback);
};

function deviceItemByUID(uid, callback) {
	forAllByKV(DEFAULT_TAB_DEVICE_NAME, ['uid'], [uid], callback);
};

function eventAdd(create_time, type, name, device_id, callback) {
	itemInsert(DEFAULT_TAB_EVENT_NAME,
		['create_time', 'type', 'name', 'device_id'],
		[create_time, type, name, device_id],
		callback);
};

function mediaAdd(create_time, type, num, rel_url, event_id, callback) {
    var date = new Date();
	itemInsert(DEFAULT_TAB_MEDIA_NAME,
		['create_time', 'type', 'num', 'rel_url', 'event_id'],
		[date, type, num, rel_url, event_id],
		callback);
};

function eventFindByDevID(device_id, event_name, callback) {
	var statement = "SELECT e.id, e.type, e.name FROM tab_event AS e" +
		    		" INNER JOIN tab_device as d" +
		    		" ON e.device_id = d.id" +
		    		" WHERE d.id = ? AND e.name = ?";

	db.all(statement, [device_id, event_name], callback);
};

function deviceUpdate(what, uid, ip, port, state, mode, token, stream_id, category, streaming_type, video_codec, audio_codec, cb) {
	var date = new Date();
	var statement = undefined;
	var args = undefined;
	var wake_mode = mode;

	var device = deviceCache[uid];
	if (state == DEVICE_STATE.STANDBY && mode == DEVICE_MODE.STANDBY_TCP) {
		if (token) {
		} else {
			token = device.token;
		}
	}

	if (device.wake_mode) {
		if (state == DEVICE_STATE.STANDBY && device.wake_mode != mode) {
			// nothing: mode
		} else {
			wake_mode = device.wake_mode;
		}
	}

	if (device.uid === uid
	 && device.ip === ip
	 && device.port === port
     && device.category === category
	 && device.state === state
	 && device.mode === mode
	 && device.token === token) {
		statement = "UPDATE tab_device" +
					" SET update_time = ?"
		args = [date];
	} else {
		if (mode != DEVICE_MODE.INVALID) {
			statement = "UPDATE tab_device" +
					" SET update_time = ?, ip = ?, port = ?, category = ?, state = ?, mode = ?, wake_mode = ?, token = ?, stream_id = ?"
			args = [date, ip, port, category, state, mode, wake_mode, token, stream_id];
		} else {
			statement = "UPDATE tab_device" +
					" SET update_time = ?, ip = ?, port = ?, category = ?, state = ?, mode = ?, token = ?, stream_id = ?"
			args = [date, ip, port, category, state, mode, token, stream_id];
		}
	}

    if(what == 131073 && mode === 3)
    {
        if(audio_codec != null && audio_codec.length > 0)
        {
            statement += ", audio_codec = ?";
            args.push(audio_codec);
        }

        if(video_codec != null && video_codec.length > 0)
        {
            statement += ", video_codec = ?";
            args.push(video_codec);
        }

        if(streaming_type != null && streaming_type.length > 0)
        {
            statement += ", streaming_type = ?";
            args.push(streaming_type);
        }
    }
    statement += " WHERE uid = ?";
    args.push(uid);

	// util.log(util.format("statement:%s args:%s", statement, args));
	db.run(statement, args, function(err) {
	    if (err) {
	        util.log('deviceUpdate err ' + err);
	    } else {
	    	// update cache
			device.update_time = date.getTime();
			device.ip = ip;
			device.port = port;
            device.category = category;
			device.state = state;
			device.mode = mode;
			device.token = token;
			device.stream_id = stream_id;
			if (state == DEVICE_STATE.STANDBY) {
				device.wake_mode = wake_mode;
			}
		}
		cb(err);
	});
}

function deviceStatusUpdate(jsonDevice, cb) {
    const what = jsonDevice.what;
    const streaming_type = jsonDevice.streaming_type;
    const audio_codec = jsonDevice.audio_codec;
    const video_codec = jsonDevice.video_codec;
	const uid = jsonDevice.uid;
	const ip = jsonDevice.ip;
	const port = jsonDevice.port;
	const state = jsonDevice.state;
	const mode = jsonDevice.mode;
          category = jsonDevice.category;
    if( category == null)
        category = 'elektra';
	var token = '';
	if (jsonDevice.token) { token = jsonDevice.token }

	var stream_id = 0;
	if (jsonDevice.stream_id) { stream_id = jsonDevice.stream_id; }

	// util.log('0 ' + util.inspect({jsonDevice: jsonDevice}));

	{
		// util.log('0-2 ' + util.inspect({deviceCache: deviceCache}));

		async.waterfall(
			[
				// check whether device exits in cache
				function(next) {
					deviceItemByUID(uid, function(err, rows) {
						if (err) {
							next(err, null);
						} else {
							if (rows.length > 0) {
								deviceCache[uid] = rows[0];
							}
							// util.log(util.format('[%s] deviceStatusUpdate 1 %s', TAG, util.inspect({deviceCache: deviceCache})));
							next(null, deviceCache[uid]);
						}
					});
			    },

			    function(arg1, next) {
			    	var device = arg1;

			    	if (device) {
			    		next(null, device);
					} else {
						deviceAdd(DEFAULT_DEVICE_NAME, uid, DEFAULT_DEVICE_TYPE, category, function(err) {
							if (err) {
						        next(err, null);
						    } else {
						    	deviceItemByUID(uid, function(error, rows) {
									if (err) {
										next(err, null);
									} else {
										if (rows.length > 0) {
											device = rows[0];
											deviceCache[uid] = device;
										}
										// util.log(util.format('[%s] deviceStatusUpdate 2 %s', TAG, util.inspect({deviceCache: deviceCache})));
										next(null, device);
									}
								});
						    }
						});
					}
			    },

			    function(arg1, next) {
			    	var device = arg1;

			    	if (device) {
						deviceUpdate(what, uid, ip, port, state, mode, token, stream_id, category, streaming_type, video_codec, audio_codec, cb);
			    	} else {
			    		next('invalid device', null);
			    	}
			    },
			],

			function(err, results) {
				if (err) {
					util.log(util.format('[%s] deviceStatusUpdate err - %s', TAG, err));
				}
		    	cb(null);
			}
		);
	}
}

function mediaInsert(jsonMedia, cb) {
	const uid = jsonMedia.uid;
	const event_name = jsonMedia.eventName;
	const event_type = DEFAULT_EVENT_TYPE;		// TODO
	const event_num = jsonMedia.eventNum;
	const event_rel_url = jsonMedia.eventRelUrl;
	const media_type = jsonMedia.eventType;
    category = jsonMedia.category;
    if(category == null)
        category = 'elektra';
	async.waterfall(
		[
			function(next) {
				if (deviceCache[uid]) {
					next(null, deviceCache[uid]);
				} else {
					deviceItemByUID(uid, function(err, rows) {
						if (err) {
							next(err, null);
						} else {
							if (rows.length > 0) {
								deviceCache[uid] = rows[0];
							}

							// util.log(util.format('[%s] mediaInsert 1 %s', TAG, util.inspect({deviceCache: deviceCache})));
							next(null, deviceCache[uid]);
						}
					});
				}
		    },

		    function(arg1, next) {
		    	var device = arg1;

		    	if (device) {
		    		next(null, device);
		    	} else {
					deviceAdd(DEFAULT_DEVICE_NAME, uid, DEFAULT_DEVICE_TYPE, category, function(err) {
						if (err) {
							next(err, null);
					    } else {
					    	deviceItemByUID(uid, function(error, rows) {
								if (error) {
									next(error, null);
								} else {
									if (rows.length > 0) {
										device = rows[0];
										deviceCache[uid] = device;
									}
									// util.log(util.format('[%s] mediaInsert 2 %s', TAG, util.inspect({deviceCache: deviceCache})));
									next(null, device);
								}
							});
					    }
					});
		    	}
		    },

		    function(arg1, next) {
		    	var device = arg1;
		    	var event_id = null;

		    	eventFindByDevID(device.id, event_name, function(err, rows) {
		    		if (err) {
						next(err, null);
					} else {
						if (rows.length > 0) {
							event_id = rows[0]['id'];
						}
						next(null, device.id, event_id);
					}
				});
		    },

		    function(arg1, arg2, next) {
		    	var device_id = arg1;
		    	var event_id = arg2;

		    	if (event_id) {
		    		next(null, event_id);
		    	} else {
		    		eventAdd(new Date(), event_type, event_name, device_id, function(err) {
						if (err) {
							next(err, null);
					    } else {
					    	eventFindByDevID(device_id, event_name, function(error, rows) {
					    		if (error) {
									next(error, null);
								} else {
									if (rows.length > 0) {
										event_id = rows[0]['id'];
									}
									next(null, event_id);
								}
							});
					    }
					});
		    	}
		    },

		    function(arg1, next) {
		    	var event_id = arg1;

		    	if (event_id) {
		    		mediaAdd(new Date(), media_type, event_num, event_rel_url, event_id, cb);
		    	} else {
		    		next('invalid event_id', null);
		    	}
		    }
		],

		function(err, results) {
			if (err) {
				util.log(util.format('[%s] mediaInsert err - %s', TAG, err));
			}
		    cb(null);
		}
	);
};

exports.setup = function(dbPath, callback) {
	if (fs.existsSync(dbPath)) {
		callback(null);
		return ;
	}

	var local_db = new sqlite3.Database(dbPath, sqlite3.OPEN_READWRITE | sqlite3.OPEN_CREATE, function(err) {
		if (err) {
            util.log('FAIL on creating database ' + err);
            callback(err);
        } else {
       		var sqls = {
			    'create_tab_device': 	"CREATE TABLE IF NOT EXISTS tab_device" +
		    						 	" (id INTEGER PRIMARY KEY AUTOINCREMENT," +
		      							" create_time DATETIME NOT NULL," +
								      	" update_time DATETIME NOT NULL," +
								      	" name TEXT DEFAULT 'unknown'," +
								      	" uid CHAR(255) NOT NULL UNIQUE," +
								      	" type INTEGER DEFAULT 0," +
										" ip CHAR(64) DEFAULT '0.0.0.0'," +
										" port INTEGER DEFAULT 0," +
										" category CHAR(64) DEFAULT 'elektra'," +
										" state INTEGER DEFAULT 0," +
										" mode INTEGER DEFAULT 0," +
										" wake_mode INTEGER DEFAULT 0," +
										" token VARCHAR(255)," +
                                        " streaming_type CHAR(16) DEFAULT 'null'," +
                                        " audio_codec CHAR(16) DEFAULT 'null'," +
                                        " video_codec CHAR(16) DEFAULT 'null'," +
										" stream_id INTEGER DEFAULT 0)",

			    'create_tab_event': 	"CREATE TABLE IF NOT EXISTS tab_event" +
								    	" (id INTEGER PRIMARY KEY AUTOINCREMENT," +
								      	" create_time DATETIME NOT NULL," +
								      	" name INTEGER," +
									  	" type INTEGER DEFAULT 0," +
										" description TEXT," +
										" device_id INTEGER," +
										"FOREIGN KEY(device_id) references tab_device(id) on delete cascade on update cascade)",

			    'create_tab_media': 	"CREATE TABLE IF NOT EXISTS tab_media" +
								    	" (id INTEGER PRIMARY KEY AUTOINCREMENT," +
								      	" create_time DATETIME NOT NULL," +
									  	" type INTEGER DEFAULT 0," +
										" num INTEGER," +
										" rel_url TEXT NOT NULL," +
										" event_id INTEGER," +
										" FOREIGN KEY(event_id) references tab_event(id) on delete cascade on update cascade)"
			};

			var tasks = ['create_tab_device', 'create_tab_event', 'create_tab_media'];
			async.eachSeries(tasks, function(item, cb) {
			    local_db.run(sqls[item], function(err) {
		        	if (err) {
		                util.log('FAIL on creating table' + err);
		                cb(err);
		            } else {
		                cb(null);
		    		}
				});
			}, function (err) {
			    local_db.close();
			    callback(err);
			});
        }
	});
};

exports.connect = function(dbPath, callback) {
	if (db) {
		util.log('db has been connected');
	} else {
		db = new sqlite3.Database(dbPath,  sqlite3.OPEN_READWRITE | sqlite3.OPEN_CREATE, function(err) {
			if (err) {
	            util.log('FAIL on creating database ' + err);
	            callback(err);
	        } else {
	            var statement = "PRAGMA foreign_keys=ON";
			    db.run(statement, function(err) {
			    	if (err) {
			            util.log('FAIL on enable foreign_keys ' + err);
			            callback(err);
			        } else {
			            callback(null);
					}
			    });
	        }
	    });
	}
};

exports.disconnect = function(callback) {
	if (db) {
		db.close(callback);
		db = undefined;
	}
};

exports.deviceList = function(callback) {
	forAll(DEFAULT_TAB_DEVICE_NAME, callback);
};

exports.devicesDelete = function(callback) {
	itemDeleteAll(DEFAULT_TAB_DEVICE_NAME, function(err) {
		if (config.platform === 'win32' || config.platform === 'win64') {
			var cmd = util.format('rd /s /q %s', devicesPath);
			exec(cmd);

			cmd = util.format('mkdir %s', devicesPath);
			exec(cmd);

			cmd = util.format('mkdir %s%stmp', devicesPath, path.sep);
			exec(cmd);
		} else {
			var cmd = util.format('rm -rf %s/* && mkdir %s/tmp', devicesPath, devicesPath);
			exec(cmd);
		}

        delete deviceCache;
        deviceCache = [];

		callback(null);
	});
};

exports.deviceItem = function(id, callback) {
	forAllByKV(DEFAULT_TAB_DEVICE_NAME, ['id'], [id], callback);
};

exports.deviceItemByUID = deviceItemByUID;

exports.deviceDelete = function(id, callback) {
	forAllByKV(DEFAULT_TAB_DEVICE_NAME, ['id'], [id], function(err, rows) {
		if (err) {
	            util.log('FAIL on find row ' + err);
	            callback(err);
	        } else {
	        	if (rows.length > 0) {
	        		delete deviceCache[rows[0].uid];

					if (config.platform === 'win32' || config.platform === 'win64') {
						var cmd = 'rd /s /q ' + devicesPath + '/' + rows[0].uid;
						exec(cmd);
					} else {
						var cmd = 'rm -rf ' + devicesPath + '/' + rows[0].uid;
						exec(cmd);
					}
	        	}
	        	itemDelete(DEFAULT_TAB_DEVICE_NAME, ['id'], [id], callback);
	        }
	});
};

exports.eventList = function(callback) {
	forAll(DEFAULT_TAB_EVENT_NAME, callback);
};

exports.eventItem = function(id, callback) {
	forAllByKV(DEFAULT_TAB_EVENT_NAME, ['id'], [id], callback);
};

exports.eventDelete = function(id, callback) {
	forAllByKV(DEFAULT_TAB_MEDIA_NAME, ['event_id'], [id], function(err, rows) {
		if (err) {
	            util.log('FAIL on find row ' + err);
	            callback(err);
	        } else {
				if (config.platform === 'win32' || config.platform === 'win64') {
					for (var i = 0; i < rows.length; i++) {
						var cmd = 'del /s /q ' + devicesPath + rows[i].rel_url.replace(/\//g, path.sep);
						exec(cmd);
					}
				} else {
					for (var i = 0; i < rows.length; i++) {
						var cmd = 'rm -f ' + devicesPath + rows[i].rel_url;
						exec(cmd);
					}
				}
            	itemDelete(DEFAULT_TAB_EVENT_NAME, ['id'], [id], callback);
	        }
	});
};

exports.mediaList = function(callback) {
	forAll(DEFAULT_TAB_MEDIA_NAME, callback);
};

exports.mediaItem = function(id, callback) {
	forAllByKV(DEFAULT_TAB_MEDIA_NAME, ['id'], [id], callback);
};

exports.mediaDelete = function(id, callback) {
	forAllByKV(DEFAULT_TAB_MEDIA_NAME, ['id'], [id], function(err, rows) {
		if (err) {
            util.log('FAIL on find row ' + err);
            callback(err);
        } else {
        	if (rows.length > 0) {
				if (config.platform === 'win32' || config.platform === 'win64') {
					var cmd = 'del /s /q ' + devicesPath + rows[0].rel_url.replace(/\//g, path.sep);
					console.log(cmd);
					exec(cmd);
				} else {
					var cmd = 'rm -f ' + devicesPath + rows[0].rel_url;
					exec(cmd);
				}
        		itemDelete(DEFAULT_TAB_MEDIA_NAME, ['id'], [id], callback);
        	} else {
        		callback(null);
        	}
        }
	});
};

exports.eventListByDevID = function(id, callback) {
	var statement = util.format('SELECT * FROM %s WHERE device_id = ? ORDER BY name DESC', DEFAULT_TAB_EVENT_NAME);
	db.all(statement, [id], callback);
};

exports.eventDeleteByDevID = function(id, callback) {
	var statement = "SELECT m.rel_url FROM tab_event AS e" +
		    		" INNER JOIN tab_media as m" +
		    		" ON e.id = m.event_id" +
		    		" WHERE e.device_id = ?";

	db.all(statement, [id], function(err, rows) {
		if (err) {
            util.log('FAIL on find row ' + err);
            callback(err);
        } else {
        	if (rows.length > 0) {
				if (config.platform === 'win32' || config.platform === 'win64') {
					var filelist = '';
					for (var i = 0; i < rows.length; i++) {
						if (i != 0) filelist += ' ';
						filelist += util.format('%s%s', devicesPath, rows[i].rel_url.replace(/\//g, path.sep));
					}

					var cmd = 'del /s /q ' + filelist;
					exec(cmd);
				} else {
					var filelist = '';
					for (var i = 0; i < rows.length; i++) {
						if (i != 0) filelist += ' ';
						filelist += util.format('%s%s', devicesPath, rows[i].rel_url);
					}

					var cmd = 'rm -f ' + filelist;
					exec(cmd);
				}

        		itemDelete(DEFAULT_TAB_EVENT_NAME, ['device_id'], [id], callback);
        	} else {
        		callback(null);
        	}
        }
	});
};

exports.mediaListByEventID = function(id, callback) {
	var statement = util.format('SELECT * FROM %s WHERE event_id = ? ORDER BY num ASC', DEFAULT_TAB_MEDIA_NAME);
	db.all(statement, [id], callback);
};

exports.mediaThumbnailByEventID = function(id, callback) {
	var statement = util.format('SELECT * FROM %s WHERE event_id = ? AND type = ?', DEFAULT_TAB_MEDIA_NAME);
	db.all(statement, [id, MEDIA_TYPE.THUMBNAIL], callback);
};

exports.mediaInsert = function(jsonMedia, cb) {
	q.push(
		{
			name: 		'mediaInsert',
			context: 	jsonMedia,
			run: 		mediaInsert
		},

		cb
	);
};

const DEVICE_ONLINE_TIMEOUT = config.device.onlineTimeout;
const DEVICE_STANDBY_TIMEOUT = config.device.standbyTimeout;

exports.deviceStatusCheck = function(cb) {
	var date = new Date();
	var curTime = date.getTime();

	const onlineTimeThreshold = curTime - DEVICE_ONLINE_TIMEOUT*1000;
	const standbyTimeThreshold = curTime - DEVICE_STANDBY_TIMEOUT*1000;

	// check whether online device is timeout
	var statement = "SELECT * FROM tab_device" +
					" WHERE (category != 'daredevil') AND ((state = ? AND update_time < ?) OR (state = ? AND update_time < ?))";
	db.all(statement, [DEVICE_STATE.ONLINE, onlineTimeThreshold, DEVICE_STATE.STANDBY, standbyTimeThreshold], function(err, rows) {
		if (err) {
			util.log('FAIL on find row ' + err);
			cb(err, null);
		} else if (rows.length > 0) {
			var stat = "UPDATE tab_device" +
						" SET update_time = ?, state = ?, mode = ?, token = ?" +
						" WHERE (state = ? AND update_time < ?) OR (state = ? AND update_time < ?)";
			db.run(stat,
				[
					curTime, DEVICE_STATE.OFFLINE, DEVICE_MODE.INVALID, '',
				 	DEVICE_STATE.ONLINE, onlineTimeThreshold,
				 	DEVICE_STATE.STANDBY, standbyTimeThreshold
				],

				function(err) {
					for (var i = 0; i < rows.length; i++) {
						if(err){
							util.log('FAIL when try to offline device ' + err);
						}else{
							rows[i].update_time = curTime;
							rows[i].state = DEVICE_STATE.OFFLINE;
							rows[i].mode = DEVICE_MODE.INVALID;
							delete rows[i].token;
							var uid = rows[i].uid;
							if (deviceCache[uid]) {
								deviceCache[uid].state = rows[i].state;
								deviceCache[uid].mode = rows[i].mode;
							}
							cb(null, rows);
						}
					}
				}
			);
		} else {
			cb('no device update', null);
		}

	});
};

exports.deviceStatusUpdate = function(jsonDevice, cb) {
	const uid = jsonDevice.uid;
	if (deviceCache[uid]) {
        const what = jsonDevice.what;
		const ip = jsonDevice.ip;
		const port = jsonDevice.port;
		const state = jsonDevice.state;
		const mode = jsonDevice.mode;
		category = jsonDevice.category;
        const streaming_type = jsonDevice.streaming_type;
        const audio_codec = jsonDevice.audio_codec;
        const video_codec = jsonDevice.video_codec;
		if(category == null)
			category = 'elektra';

		var token = 'amba_wakeup';
		if (jsonDevice.token) { token = jsonDevice.token }

		var stream_id = 0;
		if (jsonDevice.stream_id) { stream_id = jsonDevice.stream_id; }

		// util.log(util.inspect({jsonDevice: jsonDevice}));

		deviceUpdate(what,uid, ip, port, state, mode, token, stream_id, category, streaming_type, video_codec, audio_codec, cb);
	} else {
		q.push(
			{
				name: 		'deviceStatusUpdate',
				context: 	jsonDevice,
				run: 		deviceStatusUpdate
			},
			cb
		);
	}
};

var context = {
	config			: config,
	defs			: defs,
	srvDevTCP 		: null,
	srvDevUDP 		: null,
	srvAppTCP 		: null,
	srvIPC 			: null,
	streamID 		: 0
};
exports.context = context;

function appSendMsgSolo(uid, msg) {
	if (context.srvAppTCP) {
		var items = [];
		items.push({ uid:uid, data:JSON.stringify(msg) });
		context.srvDevTCP.send({ what: MSG.SELF_MSG_SEND_MULTICAST, items:items });
	}
}

function appSendMsgMulticast(rows, msgRows) {
	if (rows.length == 0 || rows.length != msgRows.length) {
		util.log(util.format('[%s] appSendMsgMulticast invalid rows:%j msgRows:%j', TAG, rows, msgRows));
		return ;
	}

	if (context.srvAppTCP) {
		var items = [];
		for (var i = 0; i < rows.length; i++) {
			var row = rows[i];

			// TODO
			// check whether app is online

			items.push({ uid:row.uid, data:JSON.stringify(msgRows[i]) });
		}

		if (items.length > 0) {
			context.srvAppTCP.send({ what: MSG.SELF_MSG_SEND_MULTICAST, items:items });
		}
	}
}

function appSendMsgBroadcast(msg) {
	if (context.srvAppTCP) {
		context.srvAppTCP.send({ what: MSG.SELF_MSG_SEND_BROADCAST, item: { data:JSON.stringify(msg) } });
	}
}

function deviceSendMsgSolo(uid, msg) {
	if (context.srvDevTCP) {
		var items = [];
		items.push({ uid:uid, data:JSON.stringify(msg) });
		context.srvDevTCP.send({ what: MSG.SELF_MSG_SEND_MULTICAST, items:items });
	}
}

function deviceSendMsgMulticast_1(rows, what) {
	const msg = { what: what };
	var msgRows = [];
	for (var i = 0; i < rows.length; i++) {
		msgRows[i] = msg;
	}
	deviceSendMsgMulticast(rows, msgRows);
}

function deviceSendMsgMulticast(rows, msgRows) {
	if (rows.length == 0 || rows.length != msgRows.length) {
		util.log(util.format('[%s] deviceSendMsgMulticast invalid rows:%j msgRows:%j', TAG, rows, msgRows));
		return ;
	}

	if (context.srvDevTCP) {
		var items = [];
		for (var i = 0; i < rows.length; i++) {
			var row = rows[i];

			if (row.state != DEVICE_STATE.ONLINE) {
				util.log(util.format('[%s] deviceSendMsgMulticast invalid state:%s mode:%d msg:%j', TAG, row.state, row.mode, msgRows[i]));
				continue;
			}

			items.push({ uid:row.uid, data:JSON.stringify(msgRows[i]) });
		}

		if (items.length > 0) {
			context.srvDevTCP.send({ what: MSG.SELF_MSG_SEND_MULTICAST, items:items });
		}
	}
}

function deviceSendMsgBroadcast(msg) {
	if (context.srvDevTCP) {
		context.srvDevTCP.send({ what: MSG.SELF_MSG_SEND_BROADCAST, item: { data:JSON.stringify(msg) } });
	}
}

exports.deviceSendMsgSolo = deviceSendMsgSolo;
exports.deviceSendMsgMulticast =deviceSendMsgMulticast;
exports.deviceSendMsgBroadcast = deviceSendMsgBroadcast;

exports.appSendMsgSolo = appSendMsgSolo;
exports.appSendMsgMulticast = appSendMsgMulticast;
exports.appSendMsgBroadcast = appSendMsgBroadcast;

exports.deviceWake = function(rows) {
	var udpItems = [];
	var tcpItems = [];

	for (var i = 0; i < rows.length; i++) {
		var row = rows[i];

		if (row.token) {
			if (row.state == DEVICE_STATE.STANDBY && row.mode == DEVICE_MODE.STANDBY_TCP) {
				tcpItems.push({ uid:row.uid, data:row.token });
			} else if (row.state == DEVICE_STATE.STANDBY && row.mode == DEVICE_MODE.STANDBY_UDP) {
				udpItems.push({ ip:row.ip, port:row.port, data:row.token });
			} else {
				util.log(util.format('[%s] invalid index:%d state:%s mode:%d for wake', TAG, i, row.state, row.mode));
			}
		} else {
			util.log(util.format('[%s] invalid index:%d token:%s for wake', TAG, i, row.token));
		}
	}

	if (context.srvDevTCP && tcpItems.length > 0) {
		context.srvDevTCP.send({ what: MSG.SELF_MSG_SEND_MULTICAST, items:tcpItems });
	}

	if (context.srvDevUDP && udpItems.length > 0) {
		context.srvDevUDP.send({ what: MSG.SELF_MSG_SEND_MULTICAST, items:udpItems });
	}
};

exports.deviceTCPWake = function(rows) {
	if (context.srvDevTCP) {
		var items = [];
		for (var i = 0; i < rows.length; i++) {
			var row = rows[i];

			if (row.state != DEVICE_STATE.STANDBY && row.mode != DEVICE_MODE.STANDBY_TCP) {
				util.log(util.format('[%s] invalid index:%d state:%s mode:%d for tcp_wake', TAG, i, row.state, row.mode));
				continue;
			}

			if (row.token) {
				items.push({ uid:row.uid, data:row.token });
			} else {
				util.log(util.format('[%s] invalid index:%d token:%s for tcp_wake', TAG, i, row.token));
			}
		}

		if (items.length > 0) {
			context.srvDevTCP.send({ what: MSG.SELF_MSG_SEND_MULTICAST, items:items });
		}
	}
};

exports.deviceUDPWake = function(rows) {
	if (context.srvDevUDP) {
		var items = [];
		for (var i = 0; i < rows.length; i++) {
			var row = rows[i];

			if (row.state != DEVICE_STATE.STANDBY && row.mode != DEVICE_MODE.STANDBY_UDP) {
				util.log(util.format('[%s] invalid index:%d state:%s mode:%d for udp_wake', TAG, i, row.state, row.mode));
				continue;
			}

			if (row.token) {
				items.push({ ip:row.ip, port:row.port, data:row.token });
			} else {
				util.log(util.format('[%s] invalid index:%d token:%s for udp_wake', TAG, i, row.token));
			}
		}
		if (items.length > 0) {
			context.srvDevUDP.send({ what: MSG.SELF_MSG_SEND_MULTICAST, items:items });
		}
	}
};

exports.deviceRename = function(row, newName) {
	var date = new Date();
	itemUpdate(DEFAULT_TAB_DEVICE_NAME, ['update_time', 'name'], [date, newName], ['id'], [row.id], function(err) {
		if (err) {
		} else {
			var result = {};
			row.name = newName;
			delete row.token;
			result.what = MSG.DEVICE_STATUS_UPDATE;
			result.items = [row];
			appSendMsgBroadcast(result);
		}
	});
};

exports.deviceTCPStandby = function(rows) {
	deviceSendMsgMulticast_1(rows, MSG.DEVICE_STANDBY_TCP);
};

exports.deviceUDPStandby = function(rows) {
	deviceSendMsgMulticast_1(rows, MSG.DEVICE_STANDBY_UDP);
};

exports.deviceAlarm = function(rows) {
	deviceSendMsgMulticast_1(rows, MSG.DEVICE_ALARM);
};

exports.deviceDisconnect = function(rows) {
	if (context.srvDevTCP && rows.length > 0) {
		var items = [];
		for (var i = 0; i < rows.length; i++) {
			var row = rows[i];
			items.push({ uid: row.uid });
		}
		context.srvDevTCP.send({ what: MSG.SELF_DEV_DISCONNECT_CLIENTS, items:items });
	}
};

exports.deviceUpdateTime = function(row, cb) {
	var date = new Date();
	itemUpdate(DEFAULT_TAB_DEVICE_NAME, ['update_time'], [date], ['id'], [row.id], function(err) {
		if (err) {
		} else {
			cb(null);
		}
	});
};

exports.deviceCloudEscape = function(msg) {
	if (config.cloud && (msg.state == DEVICE_STATE.ONLINE) && (msg.mode == DEVICE_MODE.STREAMING)) {
		msg.stream_id = context.streamID;

		if (context.streamID < 2000000000) {
        	context.streamID++;
        } else {
        	context.streamID = 0;
        }
	}
};

exports.deviceCloudCheck = function(msg) {
	if (config.cloud && (msg.state == DEVICE_STATE.ONLINE) && (msg.mode == DEVICE_MODE.STREAMING)) {
        appSendMsgBroadcast(msg);

		context.srvDevTCP.send({ what: MSG.SELF_MSG_SEND_SOLO, item: { uid:msg.uid, data:JSON.stringify({ what:msg.what, ret: 0, cloud: context.config.cloud, stream_id: msg.stream_id }) } });
    } else {
    	appSendMsgBroadcast(msg);
    }
};
