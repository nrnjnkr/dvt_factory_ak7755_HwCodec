var dgram = require('dgram'),
	util = require('util'),
	defs = require('./defs');

const TAG = 'srvDevUDP';
const MSG = defs.MSG;
const DEVICE_STATE = defs.DEVICE_STATE;
const DEVICE_MODE = defs.DEVICE_MODE;

var server;

process.on('message', function(msg) {
	msgProc(msg);
});

function msgProc(msg) {
	switch (msg.what) {
	case MSG.SELF_SRV_START: {
		startSrv(msg.port);
	} break;
	case MSG.SELF_MSG_SEND_SOLO: {
		var item = msg.item;
		msgSend(item.ip, item.port, item.data);
	} break;
	case MSG.SELF_MSG_SEND_MULTICAST: {
		var items = msg.items;
		for (var i = 0; i < items.length; i++) {
			msgSend(items[i].ip, items[i].port, items[i].data);
		}
	} break;
	default: {
		util.log(util.format('[%s] no handler for message: %j', TAG, msg));
	} break;
	}
}


function startSrv(port) {
	server = dgram.createSocket('udp4');

	server.on('listening', function() {
	    var address = server.address();
	    util.log(util.format('[%s] listening on port: %d', TAG, address.port));
	});

	server.on('message', function (message, rinfo) {
	    var newMsg = {};

		var str = message.toString();
		var index = str.indexOf('\n');
		if (index !== -1) {
			str = str.substr(0, index);
			var json = JSON.parse(str);

			newMsg.uid = json.uid;
			newMsg.token = json.token;
			newMsg.state = json.state;
			newMsg.mode = json.mode;

			// TODO
			// remove extra udp communication
                        util.log(util.format('[%s] running, -- udp server[%s:%d] , uuid[%s], ip [%s], port[%d]', TAG,server.address().address,server.address().port,json.uid,rinfo.address,rinfo.port));
			return ;
		} else {
			var items = str.split(';');
			for (var i = 0; i < items.length; i++) {
				var kv = items[i].split('=');
				if (kv.length == 2) {
					switch (kv[0]) {
					case 'uuid': 	newMsg['uid'] = kv[1]; break;
					case 'wake': 	newMsg['token'] = kv[1]; break;
					default: 		newMsg[kv[0]] = kv[1]; break;
					}
				}
			}
			newMsg.state = DEVICE_STATE.STANDBY;
			newMsg.mode = DEVICE_MODE.STANDBY_UDP;
                        util.log(util.format('*****************************udp_server[%s:%d] uuid[%s]  ip:port %s:%d',server.address().address,server.address().port,newMsg['uid'],rinfo.address, rinfo.port));
		}

		newMsg.what = MSG.DEVICE_KEEP_ALIVE_UDP;
		newMsg.ip = rinfo.address;
	    newMsg.port = rinfo.port;

	    msgNotify(newMsg);
	});
	server.bind(port);
}

// TODO
// maybe notify remote host by network someday
function msgNotify(msg) {
	process.send(msg);
}

function msgSend(ip, port, data) {
    util.log(util.format('[%s] server[%s:%d] host:%s:%d data:%j', TAG, server.address().address,server.address().port,ip, port, data));

	var buf = new Buffer(data);
	server.send(buf, 0, buf.length, port, ip, function(err, bytes) {
	    if (err) throw err;
	    util.log(util.format('[%s] UDP data %s sent %d to: %s:%d', TAG, data, bytes, ip, port));
	});
}

process.on('uncaughtException', function(err) {
    util.log(util.format('[%s] Caught exception: err:%s', TAG, err));
});
