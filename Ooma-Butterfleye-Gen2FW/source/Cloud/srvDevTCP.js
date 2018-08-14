var net = require('net'),
    dgram = require('dgram'),
	util = require('util'),
	defs = require('./defs'),
    config = require('./config'),
    log4js = require('log4js').getLogger();
    fs = require('fs');

const TAG_DEV = 'srvDevTCP';
const TAG_IPC = 'srvIpc';

const MSG = defs.MSG;
const DEVICE_STATE = defs.DEVICE_STATE;
const DEVICE_MODE = defs.DEVICE_MODE;

var host2Dev = {};
var uid2Sock = {};

function deviceLogin(host, sock, msg) {
	const uid = msg.uid;
	if (uid) {
    	msgNotify(msg);
	} else {
		util.log(util.format('[%s] undefined uid when login', TAG_DEV));
	}
}

function deviceLogout(host, msg) {
    if (host2Dev[host]) {
        const uid = host2Dev[host];
        delete uid2Sock[uid];
        delete host2Dev[host];
        msg.uid = uid;
        msgNotify(msg);
    }
}

process.on('message', function(msg) {
	msgProc(msg);
});

function msgProc(msg) {
    // util.log(util.format('[%s] msg:%j', TAG_DEV, msg));

	switch (msg.what) {
	case MSG.SELF_SRV_START: {
		startDevSrv(msg.devPort);
        if(config.device.wdogEnabled){
            startIPCSrv(msg.ipcPort);
        }
    } break;
	case MSG.SELF_MSG_SEND_SOLO: {
        var item = msg.item;
		msgSend(item.uid, item.data);
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
    case MSG.SELF_DEV_DISCONNECT_CLIENTS: {
        var items = msg.items;
        for (var i = 0; i < items.length; i++) {
            clientDisconnect(items[i].uid);
        }
    } break;
	default:
        util.log(util.format('[%s] no handler for message: %j', TAG_DEV, msg));
        break;
	}
}

function startDevSrv(port) {
    var buffer = '';

    var server = net.createServer(function(sock) {
        const host = util.format('%s:%d', sock.remoteAddress, sock.remotePort);
        util.log(util.format('[%s] %s CONNECTED', TAG_DEV, host));
        sock.setKeepAlive();//disable keep-alive options
        sock.on('data', function(data) {
            // util.log(util.format('[%s] %s DATA - %s', TAG_DEV, host, data));
            if(!config.device.wdogEnabled && (data == '*')){
                sock.emit('message', {"what":MSG.DEVICE_KEEP_ALIVE_TCP,"ip":sock.remoteAddress,"port":sock.remotePort});
                return;
            }

            try {
                buffer += data;

                do {
                    var boundary = buffer.indexOf('{');
                    if (boundary === -1) {
                        buffer = '';
                        break;
                    } else if (boundary !== 0) {
                        buffer = buffer.substr(boundary);
                    }

                    boundary = buffer.indexOf('\n');
                    if (boundary === -1) {
                        break;
                    }

                    var input = buffer.substr(0, boundary);
                    buffer = buffer.substr(boundary + 1);
                    sock.emit('message', JSON.parse(input));
                } while (true);
            } catch(err) {
                util.log(util.format('[%s] %s DATA - %s', TAG_DEV, host, buffer));
            }
        });

        sock.on('close', function(data) {
            util.log(util.format('[%s] %s CLOSED - %s', TAG_DEV, host, data));

            if (host2Dev[host]) {
                const uid = host2Dev[host];
                delete uid2Sock[uid];
                delete host2Dev[host];
            }
        });

        sock.on('error', function(data) {
            util.log(util.format('[%s] %s ERROR - %j', TAG_DEV, host, data));
        });

        sock.on('message', function(msg) {
            const uid = msg.uid;
            if (uid) {
                clientDisconnectEx(uid, sock);
                host2Dev[host] = uid;
                uid2Sock[uid] = sock;
            }
            msg.ip = sock.remoteAddress;
            msg.port = sock.remotePort;

            switch (msg.what) {
                case MSG.DEVICE_LOGIN: {
                    deviceLogin(host, sock, msg);
                    if(msg['state']===1 && msg['mode']===2)
                    {
                        try{
                            var data = fs.readFileSync('iosIp','utf8');
                            var jdata = {};
                            jdata.what = defs.MSG.DEVICE_FAST_STREAMING;
                            jdata.ip = data;
                            msgSend(msg['uid'], JSON.stringify(jdata));
                        }
                        catch(err)
                        {
                            console.log('**************** catch error ************ message');
                            console.log(err.message);
                        }
                    }
            } break;
            case MSG.DEVICE_LOGOUT: {
            	deviceLogout(host, msg);
            } break;
            default:
            	if (host2Dev[host]) {
            		msg.uid = host2Dev[host];
            		msgNotify(msg);
            	}
            	break;
            }
        });
    });

    server.on('listening', function() {
        var address = server.address();
        util.log(util.format('[%s] listening on port: %d', TAG_DEV, address.port));
    });

    server.listen(port);
}

function startIPCSrv(port) {
    var server = dgram.createSocket('udp4');

    server.on('listening', function () {
        var address = server.address();
        util.log(util.format('[%s] running: %s:%d', TAG_IPC, address.address, address.port));
    });

    server.on('message', function (message, rinfo) {
        var str = message.toString();
        var index = str.indexOf('\n');
        if (index == -1) {
            return ;
        }

        str = str.substr(0, index);
        var msg = JSON.parse(str);
        util.log(util.format('[%s] msg %j', TAG_IPC, msg));

        switch (msg.what) {
        case MSG.DEVICE_KEEP_ALIVE_TCP: {
            var host = util.format('%s:%d', msg.ip, msg.port);
            util.log(util.format("[%s] DEVICE_KEEP_ALIVE_TCP %s", TAG_IPC, host));
            if (host2Dev[host]) {
                msg.uid = host2Dev[host];
                msgNotify(msg);
            }
        } break;
        case MSG.DEVICE_STANDBY_TCP: {
            var host = util.format('%s:%d', msg.ip, msg.port);
            util.log(util.format("[%s] DEVICE_STANDBY_TCP %s", TAG_IPC, host));
            if (host2Dev[host]) {
                msg.uid = host2Dev[host];
                msgNotify(msg);
            }
        } break;
        default:
            util.log(util.format('[%s] no handler for %j', TAG_IPC, msg));
            break;
        }
    });
    server.bind(port, '127.0.0.1');
    // server.bind(port);
}

// TODO
// maybe notify remote host by network someday
function msgNotify(msg) {
	process.send(msg);
}

function msgSend(uid, data) {
    	util.log(util.format('[%s] uid:%s data:%s', TAG_DEV, uid, data));

	if (uid2Sock[uid]) {
        const content = util.format('%s\n', data);
        uid2Sock[uid].write(content);
	}
}

function msgSendBoradcast(data) {
    util.log(util.format('[%s] data:%s', TAG_DEV, data));

    const content = util.format('%s\n', data);
    for (var uid in uid2Sock) {
        if (uid2Sock[uid]) {
            uid2Sock[uid].write(content);
        }
    }
}

function clientDisconnect(uid) {
    util.log(util.format('[%s] clientDisconnect uid:%s', TAG_DEV, uid));

    if (uid2Sock[uid]) {
        var sock = uid2Sock[uid];
        var host = util.format('%s:%d', sock.remoteAddress, sock.remotePort);
        delete uid2Sock[uid];
        delete host2Dev[host];

        try {
            sock.end();
        } catch (err) {
            util.log(util.format('[%s] uid:%s err:%s', TAG_DEV, uid, err));
        }
    }
}

function clientDisconnectEx(uid, newSock) {
    util.log(util.format('[%s] clientDisconnect uid:%s', TAG_DEV, uid));

    if (uid2Sock[uid]) {
        var sock = uid2Sock[uid];
        var host = util.format('%s:%d', sock.remoteAddress, sock.remotePort);
        delete uid2Sock[uid];
        delete host2Dev[host];

        if ((newSock.remoteAddress == sock.remoteAddress) && (newSock.remotePort == sock.remotePort)) {
            util.log(util.format('[%s] $$$$$$ newSock is the same as old and ignore the old one', TAG_DEV));
        } else {
            try {
                sock.end();
                sock.destroy();
            } catch (err) {
                util.log(util.format('[%s] uid:%s err:%s', TAG_DEV, uid, err));
            }
        }
    }
}

process.on('uncaughtException', function(err) {
    util.log(util.format('[%s] Caught exception: err:%s', TAG_DEV, err));
});
