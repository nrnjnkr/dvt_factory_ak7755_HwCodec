/*
 * factory_test.h
 *
 *  Created on: 28-Jul-2018
 *      Author: root
 */

#ifndef INC_FACTORY_TEST_H_
#define INC_FACTORY_TEST_H_

enum CMD_OUTPUT {
	CMD_SUCCESS = 'S',
	CMD_FAIL = 'X',
};


enum LED_CLR {

	LED_CLR_RED = 0,
	LED_CLR_GREEN = 1,
	LED_CLR_BLUE = 2,
};

typedef enum state {
	TEST_OFF = 0,
	TEST_ON = 1,
	TEST_Unknwon = 2,
}tCommandState;

typedef struct LED_STATES {
	int state;
}tLED;


typedef struct PIR {
	int state;
}tPIR;

typedef struct GSEN {
	int state;
}tGSEN;

typedef struct BUTTON {
	int state;
}tBUTTON;

typedef struct LOUD {
	int state;
}tLOUD;


typedef struct BATTERY {
	int state;
	int percent;
}tBATTERY;

typedef struct cable_connected {
	int state;
	int conncted;
}tUSB;

typedef struct power_measure {
	int state;
}tPowerTest;

typedef struct factory_test {
	int state;
	tLED led_red;
	tLED led_green;
	tLED led_blue;
	tPIR pir;
	tGSEN acl;
	tBUTTON key;
	tLOUD voice;
	tBATTERY bat;
	tUSB wire;
	tPowerTest standby;
}tFactoryTests;

typedef enum Commands {
	INIT_FACTORY_TEST = 'F',
	LED_RED = 'R',
	LED_GREEN = 'L',
	LED_BLUE = 'B',
	PIR = 'P',
	GSEN = 'S',
	KEY = 'K',
	VOICE = 'V',
	BAT = 'C',
	USB_TEST = 'U',
	CMD_STANDBY = 'Z',
}tFactoryCommand;

void Factory_init();
void Handle_Factory(tFactoryCommand commands);
void led_flash_color(uint8_t color);
void led_flash_off();
void SendCmdResponse(char response);

#endif /* INC_FACTORY_TEST_H_ */

