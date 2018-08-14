var os = require('os');

module.exports = {
	'platform':				os.platform(),
	'version': 				'BPCamSrv_v0.5.4@Ambarella',
	'cloud': 				false,

	'rootPath': 			__dirname,
	'publicPath': 			__dirname + '/public',

	'web': {
		'port': 			6024,
	},

	'db': {
		'path': 			__dirname + '/bpcam.db'
	},

	'device': {
		'port': 			8888,
		'onlineTimeout':    		360,
		'standbyTimeout': 		50,
		'rootDir': 			'devices',
		'uploadDir': 		'devices/tmp',
		'videoDir': 		'video',
		'thumbailDir': 		'thumbail',
		'snapshotDir': 		'snapshot',
		'defaultDir': 		'default',
		'wdogEnabled': 		false
	},

	'app': {
		'port': 			8080,
		'timeout': 			16
	},

	'timer': {
		'period': 			10
	},

	'ipc': {
		'port': 			9999,
		'bin': 				__dirname + '/bin'
	},

	'api': {
		list: ['/api/v1']
	},

	'player': {
		list: ['/rtsp']
	}
};
