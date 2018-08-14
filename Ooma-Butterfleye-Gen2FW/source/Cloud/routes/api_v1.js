var express = require('express');
var router = express.Router();
var log4js = require('log4js');
var logger = log4js.getLogger();
var util = require('util'),
	config = require('../config'),
	db = require('../db'),
    fsx = require('fs');

const TAG = 'router/device';
const VERSION = config.api.list[0];

var list = [
	{ isLink:true,  name:'/devices'},
	{ isLink:true,  name:'/devices/delete'},
	{ isLink:true,  name:'/devices/alarm'},
	{ isLink:true,  name:'/devices/wake'},
	{ isLink:true,  name:'/devices/wake_udp'},
	{ isLink:true,  name:'/devices/wake_tcp'},
	{ isLink:true,  name:'/devices/standby_tcp'},
	{ isLink:true,  name:'/devices/standby_udp'},

	{ isLink:false, name:'/device/:id'},
	{ isLink:false, name:'/device/:id/delete'},
	{ isLink:false, name:'/device/:id/alarm'},
	{ isLink:false, name:'/device/:id/event' },
	{ isLink:false, name:'/device/:id/event/delete' },
	{ isLink:false, name:'/device/:id/disconnect' },
	{ isLink:false, name:'/device/:id/wake' },
	{ isLink:false, name:'/device/:id/wake_udp' },
	{ isLink:false, name:'/device/:id/wake_tcp' },
	{ isLink:false, name:'/device/:id/standby_udp' },
	{ isLink:false, name:'/device/:id/standby_tcp' },
	{ isLink:false, name:'/device/:id/rename/:name' },

	{ isLink:true,  name:'/events' },
	{ isLink:false, name:'/event/:id' },
	{ isLink:false, name:'/event/:id/delete'},
	{ isLink:false, name:'/event/:id/media' },
	{ isLink:false, name:'/event/:id/thumbnail' },

	{ isLink:true,  name:'/medias' },
	{ isLink:false, name:'/media/:id' },
	{ isLink:false, name:'/media/:id/delete'},

	{ isLink:true,  ignorePrefix:true,  name:'/upload'},
];

router.get('/', function(req, res, next) {
	res.render('api_v1.ejs', { title: 'api_v1', version:VERSION, list:list });
});

/* GET devices listing. */
router.get('/device', function(req, res, next) {
	res.redirect(VERSION + '/devices');
});

router.get('/devices', function(req, res, next) {
	db.deviceList(function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] deviceAll completed - %s', TAG, err));
			result.err = err;
		} else {
			if (config.cloud) {
				result.cloud = config.cloud;
			}
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/devices/delete', function(req, res, next) {
	db.devicesDelete(function(err) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] devicesDelete completed - %s', TAG, err));
			result.err = err;
		} else {
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/devices/alarm', function(req, res, next) {
	db.deviceList(function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] deviceAll completed - %s', TAG, err));
			result.err = err;
		} else {
			db.deviceAlarm(rows);
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/devices/wake', function(req, res, next) {
  	db.deviceList(function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] deviceItem completed - %s', TAG, err));
			result.err = err;
		} else {
			db.deviceWake(rows);
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/devices/wake_udp', function(req, res, next) {
  	db.deviceList(function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] deviceList completed - %s', TAG, err));
			result.err = err;
		} else {
			db.deviceUDPWake(rows);
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/devices/wake_tcp', function(req, res, next) {
  	db.deviceList(function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] deviceList completed - %s', TAG, err));
			result.err = err;
		} else {
			db.deviceTCPWake(rows);
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/devices/standby_udp', function(req, res, next) {
  	db.deviceList(function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] deviceList completed - %s', TAG, err));
			result.err = err;
		} else {
			db.deviceUDPStandby(rows);
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/devices/standby_tcp', function(req, res, next) {
  	db.deviceList(function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] deviceList completed - %s', TAG, err));
			result.err = err;
		} else {
			db.deviceTCPStandby(rows);
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/device/:id', function(req, res, next) {
  	db.deviceItem(req.params.id, function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] deviceItem completed - %s', TAG, err));
			ret.err = err;
		} else {
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/device/:id/delete', function(req, res, next) {
  	db.deviceDelete(req.params.id, function(err) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] deviceDelete completed - %s', TAG, err));
			ret.err = err;
		} else {
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/device/:id/wake', function(req, res, next) {
  	db.deviceItem(req.params.id, function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] deviceItem completed - %s', TAG, err));
			ret.err = err;
		} else {
			db.deviceWake(rows);
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/device/:id/wake_udp', function(req, res, next) {
  	db.deviceItem(req.params.id, function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] deviceItem completed - %s', TAG, err));
			ret.err = err;
		} else {
			db.deviceUDPWake(rows);
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/device/:id/wake_tcp', function(req, res, next) {
  	db.deviceItem(req.params.id, function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] deviceItem completed - %s', TAG, err));
			ret.err = err;
		} else {
            try{
                fsx.writeFileSync('iosIp',req.connection.remoteAddress,'utf8');
            }
            catch(err)
            {
                console.log(err.message);
            }

			db.deviceTCPWake(rows);
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/device/:id/standby_udp', function(req, res, next) {
  	db.deviceItem(req.params.id, function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] deviceItem completed - %s', TAG, err));
			ret.err = err;
		} else {
			db.deviceUDPStandby(rows);
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/device/:id/standby_tcp', function(req, res, next) {
  	db.deviceItem(req.params.id, function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] deviceItem completed - %s', TAG, err));
			ret.err = err;
		} else {
			db.deviceTCPStandby(rows);
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/device/:id/rename/:name', function(req, res, next) {
  	db.deviceItem(req.params.id, function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] deviceItem completed - %s', TAG, err));
			ret.err = err;
		} else {
			db.deviceRename(rows[0], req.params.name);
			rows[0].name = req.params.name;
			delete rows[0].token;
			result.items = [rows[0]];
			result.ret = 0;
		}
		res.send(result);
	});
});

/* GET events listing. */
router.get('/event', function(req, res, next) {
	res.redirect(VERSION + '/events');
});

router.get('/events', function(req, res, next) {
	db.eventList(function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] eventAll completed - %s', TAG, err));
			ret.err = err;
		} else {
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/event/:id', function(req, res, next) {
  	db.eventItem(req.params.id, function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] eventItem completed - %s', TAG, err));
			ret.err = err;
		} else {
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/event/:id/delete', function(req, res, next) {
  	db.eventDelete(req.params.id, function(err) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] eventItem completed - %s', TAG, err));
			ret.err = err;
		} else {
			result.ret = 0;
		}
		res.send(result);
	});
});

/* GET medias listing. */
router.get('/media', function(req, res, next) {
	res.redirect(VERSION + '/medias');
});

router.get('/medias', function(req, res, next) {
	db.mediaList(function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] mediaAll completed - %s', TAG, err));
			ret.err = err;
		} else {
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/media/:id', function(req, res, next) {
  	db.mediaItem(req.params.id, function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] mediaItem completed - %s', TAG, err));
			ret.err = err;
		} else {
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/media/:id/delete', function(req, res, next) {
  	db.mediaDelete(req.params.id, function(err) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] mediaDelete completed - %s', TAG, err));
		} else {
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/device/:id/alarm', function(req, res, next) {
	db.deviceItem(req.params.id, function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] deviceItem completed - %s', TAG, err));
			result.err = err;
		} else {
			db.deviceAlarm(rows);
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/device/:id/event', function(req, res, next) {
  	db.eventListByDevID(req.params.id, function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] eventListByDevID completed - %s', TAG, err));
			result.err = err;
		} else {
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/device/:id/event/delete', function(req, res, next) {
  	db.eventDeleteByDevID(req.params.id, function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] eventDeleteByDevID completed - %s', TAG, err));
			result.err = err;
		} else {
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/device/:id/disconnect', function(req, res, next) {
  	db.deviceItem(req.params.id, function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] deviceItem completed - %s', TAG, err));
			result.err = err;
		} else {
			db.deviceDisconnect(rows);
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/event/:id/media', function(req, res, next) {
  	db.mediaListByEventID(req.params.id, function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] mediaListByEventID completed - %s', TAG, err));
		} else {
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

router.get('/event/:id/thumbnail', function(req, res, next) {
  	db.mediaThumbnailByEventID(req.params.id, function(err, rows) {
		var result = { ret: -1 };
		if (err) {
			util.log(util.format('[%s] mediaThumbnailByEventID completed - %s', TAG, err));
		} else {
			result.items = rows;
			result.ret = 0;
		}
		res.send(result);
	});
});

module.exports = router;
