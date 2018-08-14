exports.MSG_CAT = {
	CAT_RESERVED                    : 0x00,
    CAT_CLOUD                       : 0x01,
    CAT_DEVICE                      : 0x02,
    CAT_APP                         : 0x03,
    CAT_WEB                         : 0x04,

    CAT_DBG                         : 0xFE,
    CAT_SELF                        : 0xFF,
};

exports.MSG = {
	CLOUD_HOME    	      		    : 0x00010000,
    CLOUD_END					    : 0x0001FFFF,

	DEVICE_HOME     			    : 0x00020000,
    DEVICE_LOGIN				    : 0x00020001,
    DEVICE_LOGOUT				    : 0x00020002,
    DEVICE_BATTERY				    : 0x00020003,
    DEVICE_CHARGE				    : 0x00020004,
    DEVICE_DC 					    : 0x00020005,
    DEVICE_PIR  				    : 0x00020006,
    DEVICE_ALARM 				    : 0x00020007,
    DEVICE_SHUTDOWN				    : 0x00020008,
    DEVICE_STANDBY_TCP			    : 0x00020009,
    DEVICE_STANDBY_UDP			    : 0x0002000A,
    DEVICE_WAKEUP_TCP 			    : 0x0002000B,
    DEVICE_WAKEUP_UDP			    : 0x0002000C,
    DEVICE_STATUS_UPDATE		    : 0x0002000D,
    DEVICE_AGENT_CONNECTED		    : 0x0002000E,
    DEVICE_SYNC 				    : 0x0002000F,
    DEVICE_KEEP_ALIVE_TCP		    : 0x00020010,
    DEVICE_KEEP_ALIVE_UDP		    : 0x00020011,
    DEVICE_FAST_STREAMING           : 0x00020012,
    DEVICE_END 					    : 0x0002FFFF,

    APP_HOME                	    : 0x00030000,
    APP_LOGIN					    : 0x00030001,
    APP_LOGOUT					    : 0x00030002,
    APP_GET_DEVICE_LIST			    : 0x00030003,
    APP_GET_DEVICE_DETAIL		    : 0x00030004,
    APP_GET_EVENT_LIST			    : 0x00030005,
    APP_GET_EVENT_DETAIL		    : 0x00030006,
    APP_END 					    : 0x0003FFFF,

    DBG_HOME                        : 0x00FE0000,
    DBG_CANNOT_CONNECT_CLOUD	    : 0x00FE0001,
    DBG_DISCONNECT_DEVICE		    : 0x00FE0002,
    DBG_END 					    : 0x00FE0003,

    SELF_HOME                       : 0x00FF0000,
    SELF_SRV_START                  : 0x00FF0001,
    SELF_SRV_STOP                   : 0x00FF0002,

    SELF_MSG_SEND_SOLO              : 0x00FF0100,
    SELF_MSG_SEND_MULTICAST         : 0x00FF0101,
    SELF_MSG_SEND_BROADCAST         : 0x00FF0102,

    SELF_DEV_TCP_WAKE_SOLO          : 0x00FF0200,
    SELF_DEV_TCP_WAKE_MULTICAST     : 0x00FF0201,
    SELF_DEV_TCP_WAKE_BROADCAST     : 0x00FF0202,
    SELF_DEV_UDP_WAKE_SOLO          : 0x00FF0203,
    SELF_DEV_UDP_WAKE_MULTICAST     : 0x00FF0204,
    SELF_DEV_UDP_KEEP_ALIVE         : 0x00FF0205,
    SELF_DEV_TCP_KEEP_ALIVE         : 0x00FF0206,
    SELF_DEV_DISCONNECT_CLIENTS     : 0x00FF0207,

    SELF_APP_RESERVED               : 0x00FF0200,

    SELF_END                        : 0x00FF0000,
};

exports.DEVICE_TYPE = {
	CAMERA 			: 0
};

exports.DEVICE_STATE = {
	OFFLINE 		: 0,
    ONLINE  		: 1,
    STANDBY			: 2
};

exports.DEVICE_MODE = {
	INVALID 		: 0,
    RECORDING  		: 1,
    STREAMING		: 2,
    STANDBY_TCP 	: 3,
    STANDBY_UDP 	: 4
};

exports.EVENT_TYPE = {
    COMMON          : 0
};

exports.MEDIA_TYPE = {
    VIDEO           : 0,
    THUMBNAIL       : 1
};
