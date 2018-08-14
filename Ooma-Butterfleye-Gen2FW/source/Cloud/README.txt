{
	"Info": {
		"name": 		"BPCamSvr",
		"version": 		"0.5.4",
		"private": 		true,
		"description": 	"File Upload & Share Web Server for Battery Powered Camera",
		"license": 		"Copyright 2016 Ambarella Inc. All rights reserved.",
		"timestamp":	"201608251021"
	},

	"Setup": {
		"pre-requistue":	"Windows 7 32-bit or Windows 7 64-bit or Ubuntu Linux 64-bit or Ubuntu Linux 32-bit or MacOSX 64-bit",

		"for *nix": {
			"run":				"sudo ./run",
			"clean":			"sudo ./clean"
		},

		"for macOSX": {
			"run":				"./run",
			"clean":			"./clean"
		},

		"for Windows": {
			"run":				"click run.bat",
			"clean":			"click clean.bat"
		}
	},

	"Usage": {
		"HomePage": 	"Access http://server_ip:6024 using Browser",

		"API call":		"e.g:
							1. http://server_ip:6024/api/v1/devices to get device list
							2. replace :id with the real item's value in table
							   http://server_ip:6024/api/v1/device/1 to get device item, whose id is 1
						",

		"APIs":			[
							"/api/v1/devices",
							"/api/v1/devices/delete",
							"/api/v1/devices/alarm",
							"/api/v1/devices/wake",
							"/api/v1/devices/wake_udp",
							"/api/v1/devices/wake_tcp",
							"/api/v1/devices/standby_tcp",
							"/api/v1/devices/standby_udp",
							"/api/v1/device/:id",
							"/api/v1/device/:id/delete",
							"/api/v1/device/:id/alarm",
							"/api/v1/device/:id/event",
							"/api/v1/device/:id/event/delete",
							"/api/v1/device/:id/disconnect",
							"/api/v1/device/:id/wake",
							"/api/v1/device/:id/wake_udp",
							"/api/v1/device/:id/wake_tcp",
							"/api/v1/device/:id/standby_udp",
							"/api/v1/device/:id/standby_tcp",
							"/api/v1/device/:id/rename/:name",
							"/api/v1/events",
							"/api/v1/event/:id",
							"/api/v1/event/:id/delete",
							"/api/v1/event/:id/media",
							"/api/v1/event/:id/thumbnail",
							"/api/v1/medias",
							"/api/v1/media/:id",
							"/api/v1/media/:id/delete",
							"/upload"
						]
	}
}
