var net = require('net'),
	util = require('util'),
	defs = require('./defs');

const TAG = 'srvAppTCP';


const MSG = defs.MSG;
const DEVICE_STATE = defs.DEVICE_STATE;
const DEVICE_MODE = defs.DEVICE_MODE;

var host2App = {};
var uid2Sock = {};

function appLogin(host, sock, msg) {
	const uid = msg.uid;
	if (uid) {
		host2App[host] = uid;
    	uid2Sock[uid] = sock;
        msg.ip = sock.remoteAddress;
        msg.port = sock.remotePort;
    	msgNotify(msg);
	} else {
		util.log(util.format('[%s] undefined uid when login', TAG));
	}
}

function appLogout(host, msg) {
    if (host2App[host]) {
        const uid = host2App[host];
        delete uid2Sock[uid];
        delete host2App[host];
        msg.uid = uid;
        msgNotify(msg);
    }
}

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
		msgSend(msg.uid, item.data);
	} break;
	case MSG.SELF_MSG_SEND_MULTICAST: {
		var items = msg.items;
		for (var i = 0; i < items.length; i++) {
			msgSend(items[i].uid, items[i].data);
		}
	} break;
    case MSG.SELF_MSG_SEND_BROADCAST: {
        var item = msg.item;
        msgSendBoradcast(item.data);
    } break;
	default: {
		util.log(util.format('[%s] no handler for message: %j', TAG, msg));
	} break;
	}
}

function startSrv(port) {
    var buffer = '';

    var server = net.createServer(function(sock) {
        const host = util.format('%s:%d', sock.remoteAddress, sock.remotePort);
        util.log(util.format('[%s] %s CONNECTED', TAG, host));

        sock.on('data', function(data) {
            // util.log(util.format('[%s] %s DATA - %s', TAG, host, data));

            try {
                buffer += data;

                do {
                    var boundary = buffer.indexOf('{');
                    if (boundary < 0) {
                        buffer = '';
                        break;
                    } else if (boundary > 0) {
                        buffer = buffer.substr(boundary);
                    }

                    boundary = buffer.indexOf('\n');
                    if (boundary < -1) {
                        break;
                    }

                    var input = buffer.substr(0, boundary);
                    buffer = buffer.substr(boundary + 1);
                    sock.emit('message', JSON.parse(input));
                } while (true);
            } catch(err) {
                util.log(util.format('[%s] %s DATA - %s', TAG, host, buffer));
                buffer = '';
            }
        });

        sock.on('close', function(data) {
            util.log(util.format('[%s] %s CLOSED - %s', TAG, host, data));

            appLogout(host, { what: MSG.APP_LOGOUT });
        });

        sock.on('error', function(data) {
            util.log(util.format('[%s] %s ERROR - %j', TAG, host, data));

            appLogout(host, { what: MSG.APP_LOGOUT });
        });

        sock.on('message', function(msg) {
            // util.log(util.format('[%s] %s MESSAGE - %j', TAG, host, msg));
            switch (msg.what) {
            case MSG.APP_LOGIN: {
            	appLogin(host, sock, msg);
            } break;
            case MSG.App_LOGOUT: {
            	appLogout(host, msg);
            } break;
            default:
            	if (hostApp[host]) {
            		msg.uid = hostApp[host];
            		msgNotify(msg);
            	}
            	break;
            }
        });
    });

    server.on('listening', function() {
        var address = server.address();
        util.log(util.format('[%s] listening on port: %d', TAG, address.port));
    });

    server.listen(port);
}

// TODO
// maybe notify remote host by network someday
function msgNotify(msg) {
	process.send(msg);
}

function msgSend(uid, data) {
    // util.log(util.format('[%s] uid:%s data:%s %s', TAG, uid, data, uid2Sock[uid]));

	if (uid2Sock[uid]) {
		const content = util.format('%s\n', data);
		uid2Sock[uid].write(content);
	}
}

function msgSendBoradcast(data) {
    // util.log(util.format('[%s] msgSendBoradcast data:%s', TAG, uid2Sock));

    var content = util.format('%s\n', data);
    for (var uid in uid2Sock) {
        if (uid2Sock[uid]) {
            var sock = uid2Sock[uid];
            var host = util.format('%s:%d', sock.remoteAddress, sock.remotePort);
			sock.write(content);
            //util.log(util.format('[%s] msgSendBoradcast %s uid:%s sock:%s data:%s', TAG, uid, host, sock, data));
        }
    }
}

process.on('uncaughtException', function(err) {
    util.log(util.format('[%s] Caught exception: err:%s', TAG, err));
});

function clientDisconnect(uid) {
    util.log(util.format('[%s] clientDisconnect uid:%s', TAG, uid));

    if (uid2Sock[uid]) {
        var sock = uid2Sock[uid];
        var host = util.format('%s:%d', sock.remoteAddress, sock.remotePort);
        delete uid2Sock[uid];
        delete host2App[host];

        try {
            sock.end();
        } catch (err) {
            util.log(util.format('[%s] uid:%s err:%s', TAG, uid, err));
        }
    }
}