#!/usr/bin/env node
var os = require('os');
var platform = os.platform();
var ifaces=os.networkInterfaces();
var async = require('async'),
	child_process = require('child_process'),
	fs = require('fs'),
	path = require('path'),
	util = require('util'),
	pkg = require('./package'),
	defs = require('./defs'),
	db = require('./db'),
	app = require('./app'),
	debug = require('debug')('nodejs-demo:server'),
	http = require('http');

const TAG = 'main';
const MSG = defs.MSG;
const DEVICE_TYPE = defs.DEVICE_TYPE;
const DEVICE_STATE = defs.DEVICE_STATE;
const DEVICE_MODE = defs.DEVICE_MODE;
var taskMap = {
	'printInfo': 		printInfo,
    'dbSetup': 			dbSetup,
    'dbConnect': 		dbConnect,
    'dirSetup': 		dirSetup,
	'srvDevTCP': 		srvDevTCP,
	'srvDevUDP': 		srvDevUDP,
	'srvAppTCP': 		srvAppTCP,
	'srvMedia': 		srvMedia,
	'monitor': 	        monitor,
};

var tasks = ['printInfo', 'dbSetup', 'dbConnect', 'dirSetup', 'srvDevTCP', 'srvDevUDP', 'srvAppTCP', 'srvMedia', 'monitor'];
async.eachSeries(tasks, function(item, cb) {
    	taskMap[item](db.context, cb);
	},
	function (err) {
    	if (err) throw err;
	}
);

/*
 	steps:
 	0. printInfo
 	1. dbSetup			db
 	2. dbConnect
 	3. dirSetup
 	4. srvDevTCP		dev tcp server
 	5. srvDevUDP		dev udp server
 	6. srvAppTCP		app tcp server
 	7. srvMedia			media web server
 	8. monitor 			parent monitors all child
 */

////// printInfo
function printInfo(context, cb) {
	util.log('[name]          ' + pkg.name);
	util.log('[version]       ' + pkg.version);
	util.log('[description]   ' + pkg.description);
	util.log('[license]       ' + pkg.license);
	console.log('');

	cb(null);
}

////// dbSetup
function dbSetup(context, cb) {
	db.setup(context.config.db.path, function(err) {
		if (err) {
			util.log(util.format('[%s] dbSetup %s', TAG, err));
		} else {
			util.log(util.format('[%s] dbSetup success', TAG));
		}
		cb(err);
	});
}

////// dbConnect
function dbConnect(context, cb) {
	db.connect(context.config.db.path, function(err) {
		if (err) {
			util.log(util.format('[%s] dbConnect %s', TAG, err));
		} else {
			util.log(util.format('[%s] dbConnect success', TAG));
		}
		cb(err);
	});
}

////// dirSetup
function dirSetup(context, cb) {
	const publicPath = context.config.publicPath;
	if (!fs.existsSync(publicPath)) {
		fs.mkdirSync(publicPath);
	}

	const devicesPath = path.join(publicPath, context.config.device.rootDir);
	if (!fs.existsSync(devicesPath)) {
		fs.mkdirSync(devicesPath);
	}

	const uploadPath = path.join(publicPath, context.config.device.uploadDir);
	if (!fs.existsSync(uploadPath)) {
		fs.mkdirSync(uploadPath);
	}

	util.log(util.format('[%s] dirSetup success', TAG));
	cb(null);
}

////// srvDevTCP
function srvDevTCP(context, cb) {
	var child = child_process.fork('./srvDevTCP.js');
	child.on('message', function(msg) {
		util.log(util.format('[%s] srvDevTCP: %j', TAG, msg));
		srvDevTCPMsgProc(context, msg);
	});
	child.send({ what: MSG.SELF_SRV_START, devPort: context.config.device.port, ipcPort:context.config.ipc.port });
	context.srvDevTCP = child;
	cb(null);
}

////// srvDevUDP
function srvDevUDP(context, cb) {
	const child = child_process.fork('./srvDevUDP.js');
	child.on('message', function(msg) {
	  	util.log(util.format('[%s] srvDevUDP: %j', TAG, msg));
	  	srvDevUDPMsgProc(context, msg);
	});
	child.send({ what: MSG.SELF_SRV_START, port: context.config.device.port });
	context.srvDevUDP = child;
	cb(null);
}

////// srvAppTCP
function srvAppTCP(context, cb) {
	const child = child_process.fork('./srvAppTCP.js');
	child.on('message', function(msg) {
		// util.log(util.format('[%s] srvAppTCP: %j', TAG, msg));
		srvAppTCPMsgProc(context, msg);
	});
	child.send({ what: MSG.SELF_SRV_START, port: context.config.app.port});
	context.srvAppTCP = child;
	cb(null);
}

////// srvMedia
function srvMedia(context, cb) {
	var port = normalizePort(process.env.PORT || context.config.web.port);
	app.set('port', port);

	var server = http.createServer(app);
	server.listen(port);

	server.on('error', function(error) {
		if (error.syscall !== 'listen') {
	    	throw error;
	  	}

	  	var bind = typeof port === 'string'
	    	? 'Pipe ' + port
	    	: 'Port ' + port;

		// handle specific listen errors with friendly messages
		switch (error.code) {
		case 'EACCES':
			util.error(bind + ' requires elevated privileges');
			process.exit(1);
			break;
		case 'EADDRINUSE':
			util.error(bind + ' is already in use');
			process.exit(1);
			break;
		default:
			throw error;
		}
	});

	server.on('listening', function onListening() {
		var addr = server.address();
		var bind = typeof addr === 'string'
			? 'pipe ' + addr
			: 'port ' + addr.port;
		debug('Listening on ' + bind);
	});

	try {
		var links = '';
		for (var dev in ifaces) {
			var ifaces_size = ifaces[dev].length;
			for (var i = 0; i < ifaces_size; i++) {
				var details = ifaces[dev][i];
				if ((details.family =='IPv4') && (details.address != '127.0.0.1')) {
					if(links != ''){
						links += '  |  ' + 'http://' + details.address + ':' + context.config.web.port;
					}else{
						links += 'http://' + details.address + ':' + context.config.web.port;
					}
				}
			}
		}
		util.log(util.format('[%s] available web links -->  %s', TAG, links));
	} catch (e) {
		util.log(util.format('[%s] get web links failed', TAG));
	}

	cb(null);
}

////// monitor
function monitor(context, cb) {
	if (context.config.device.wdogEnabled) {
		child_process.exec('wdog', function(err, stdout, stderr) {
		});
	}

	setInterval(periodicCheckProc, context.config.timer.period*1000);

	cb(null);
}

function srvDevTCPMsgProc(context, msg) {
	const child = context.srvDevTCP;

	switch (msg.what) {
    case MSG.DEVICE_LOGIN: {
    	util.log(util.format('[%s] DEVICE_LOGIN %j', TAG, msg));

		db.deviceCloudEscape(msg);

        db.deviceStatusUpdate(msg,
			function(err) {
				db.deviceCloudCheck(msg);
				delete msg.token;
			}
		);
    } break;
    case MSG.DEVICE_LOGOUT: {
    	db.deviceItemByUID(msg.uid, function(err, rows) {
			if (err) {
				util.log(util.format('[%s] deviceItemByUID what:%d failure - %s', TAG, msg.what, err));
			} else {
				if (rows.length > 0) {
					util.log(util.format('[%s] DEVICE_LOGOUT %j', TAG, rows[0]));

					var row = rows[0];
					rows[0].state = DEVICE_STATE.STANDBY;
					rows[0].mode = DEVICE_MODE.INVALID;
					rows[0].token = '';

					db.deviceStatusUpdate(rows[0], function(err) {
						db.appSendMsgBroadcast(msg);
					});
				}
			}
		});
    } break;
    case MSG.DEVICE_BATTERY:
    case MSG.DEVICE_CHARGE:
    case MSG.DEVICE_DC:
   	case MSG.DEVICE_PIR: {
   		db.appSendMsgBroadcast(msg);
   	} break;
    case MSG.DEVICE_STANDBY_TCP:{
		util.log(util.format('[%s] DEVICE_STANDBY_TCP %j', TAG, msg));
		setTimeout(function() {
			db.deviceItemByUID(msg.uid, function(err, rows) {
				if (err) {
					util.log(util.format('[%s] deviceItemByUID what:%d failure - %s', TAG, msg.what, err));
				} else {
					if (rows.length > 0) {
						rows[0].state = DEVICE_STATE.STANDBY;
						rows[0].mode = DEVICE_MODE.STANDBY_TCP;
						rows[0].ip = msg.ip;
						rows[0].port = msg.port;
						rows[0].token = msg.token;
						db.deviceStatusUpdate(rows[0], function(err) {
							db.appSendMsgBroadcast(msg);
						});
					}else{
						util.log(util.format('[%s] DEVICE_STANDBY_TCP device not exist in db', TAG));
					}
				}
			});
		}, 500);
	} break;

	case MSG.DEVICE_KEEP_ALIVE_TCP: {
        util.log(util.format('[%s] DEVICE_KEEP_ALIVE_TCP %j', TAG, msg));
		setTimeout(function() {
			db.deviceItemByUID(msg.uid, function(err, rows) {
				if (err) {
					util.log(util.format('[%s] deviceItemByUID what:%d failure - %s', TAG, msg.what, err));
				} else {
					db.deviceUpdateTime(rows[0], function(err) {
						msg.state = DEVICE_STATE.STANDBY;
						msg.mode = DEVICE_MODE.STANDBY_TCP;
						db.appSendMsgBroadcast(msg);
					});
				}
			})
		}, 500);
   	} break;
    default:
        util.log(util.format('[%s] srvDevTCP no handler for message: %j', TAG, msg));
        break;
    }
}

function srvDevUDPMsgProc(context, msg) {
	const child = context.srvDevUDP;

	switch (msg.what) {
	case MSG.DEVICE_KEEP_ALIVE_UDP: {
		db.deviceStatusUpdate(msg, function(err) {
			if (err) {
				util.log(util.format('[%s] what:%d failure - %s', TAG, msg.what, err));
			} else {
				delete msg.token;
				db.appSendMsgBroadcast(msg);
			}
		});
	} break;
	default:
        util.log(util.format('[%s] srvDevUDP no handler for message: %j', TAG, msg));
        break;
	}
}

function srvAppTCPMsgProc(context, msg) {
	const child = context.srvAppTCP;

	switch (msg.what) {
    case MSG.APP_LOGIN: {

    } break;
    case MSG.APP_LOGOUT: {

    } break;
    case MSG.APP_GET_DEVICE_LIST: {
    	db.deviceList(function(err, rows) {
    		var result = { ret: -1 };
			if (err) {
				util.log(util.format('[%s] what:%d failure - %s', TAG, msg.what, err));
			} else {
				result.what = msg.what;
				result.items = rows;
				result.ret = 0;
			}
			db.appSendMsgSolo(msg.uid, result);
		});
    } break;
    case MSG.APP_GET_DEVICE_DETAIL: {
    	db.deviceItem(msg.id, function(err, rows) {
    		var result = { ret: -1 };
			if (err) {
				util.log(util.format('[%s] what:%d failure - %s', TAG, msg.what, err));
			} else {
				result.what = msg.what;
				result.items = rows;
				result.ret = 0;
			}
			db.appSendMsgSolo(msg.uid, result);
		});
    } break;
    case MSG.APP_GET_EVENT_LIST: {
    	db.eventListByDevID(msg.id, function(err, rows) {
    		var result = { ret: -1 };
			if (err) {
				util.log(util.format('[%s] what:%d failure - %s', TAG, msg.what, err));
			} else {
				result.what = msg.what;
				result.items = rows;
				result.ret = 0;
			}
			db.appSendMsgSolo(msg.uid, result);
		});
    } break;
    case MSG.APP_GET_EVENT_DETAIL: {
    	db.mediaListByEventID(msg.id, function(err, rows) {
    		var result = { ret: -1 };
			if (err) {
				util.log(util.format('[%s] what:%d failure - %s', TAG, msg.what, err));
			} else {
				result.what = msg.what;
				result.items = rows;
				result.ret = 0;
			}
			db.appSendMsgSolo(msg.uid, result);
		});
    } break;
    case MSG.DEVICE_STANDBY_TCP:
    case MSG.DEVICE_STANDBY_UDP:
    case MSG.DEVICE_ALARM:
    case MSG.DEVICE_SHUTDOWN: {
    	db.deviceItem(msg.id, function(err, rows) {
			if (err) {
				util.log(util.format('[%s] what:%d failure - %s', TAG, msg.what, err));
			} else {
				if (rows.length > 0) {
					db.deviceSendMsgMulticast(rows, msg.what);
				} else {
					util.log(util.format('[%s] cannot find device what:%d id:%d', TAG, msg.what, msg.id));
				}
			}
		});
    } break;
    case MSG.DEVICE_WAKEUP_TCP: {
    	db.deviceItem(msg.id, function(err, rows) {
			if (err) {
				util.log(util.format('[%s] what:%d failure - %s', TAG, msg.what, err));
			} else {
				if (rows.length > 0) {
					db.deviceTCPWake(rows);
				} else {
					util.log(util.format('[%s] cannot find device what:%d id:%d', TAG, msg.what, msg.id));
				}
			}
		});
    } break;
   	case MSG.DEVICE_WAKEUP_UDP: {
   		db.deviceItem(msg.id, function(err, rows) {
			if (err) {
				util.log(util.format('[%s] what:%d failure - %s', TAG, msg.what, err));
			} else {
				if (rows.length > 0) {
					db.deviceUDPWake(rows);
				} else {
					util.log(util.format('[%s] cannot find device what:%d id:%d', TAG, msg.what, msg.id));
				}
			}
		});
   	} break;
   	case MSG.DBG_DISCONNECT_DEVICE: {
   		db.deviceItem(msg.id, function(err, rows) {
			if (err) {
				util.log(util.format('[%s] what:%d failure - %s', TAG, msg.what, err));
			} else {
				if (rows.length > 0) {
					db.deviceDisconnect(rows);
				} else {
					util.log(util.format('[%s] cannot find device what:%d id:%d', TAG, msg.what, msg.id));
				}
			}
		});
   	} break;
    default:
        util.log(util.format('[%s] srvAppTCP no handler for message: %j', TAG, msg));
        break;
    }
}

function periodicCheckProc() {
	// util.log(util.format('[%s] periodicCheckProc', TAG));

	db.deviceStatusCheck(function(err, rows) {
		if (err) {
			// util.log(util.format('[%s] periodicCheckProc err - %s', TAG, err));
		} else if (rows.length > 0) {
			util.log(util.format('[%s] periodicCheckProc %j', TAG, rows));
			var result = { };
			result.what = MSG.DEVICE_STATUS_UPDATE;
			result.items = rows;
			db.appSendMsgBroadcast(result);
		}
	});
}

/**
 * Normalize a port into a number, string, or false.
 */

function normalizePort(val) {
	var port = parseInt(val, 10);

	if (isNaN(port)) {
		// named pipe
		return val;
	}

	if (port >= 0) {
		// port number
		return port;
	}

	return false;
}
