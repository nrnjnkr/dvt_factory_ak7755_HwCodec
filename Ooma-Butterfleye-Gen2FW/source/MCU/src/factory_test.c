#include <stdio.h>

#include "em_gpio.h"
#include "config.h"
#include "em_leuart.h"
#include "am_common.h"
#include "em_device.h"
#include "factory_test.h"


tFactoryTests factory;

tFactoryCommand cmd;

void Factory_init() {

	factory.state = TEST_OFF;

	factory.led_red.state = TEST_OFF;
	factory.led_green.state = TEST_OFF;
	factory.led_blue.state = TEST_OFF;
	factory.pir.state = TEST_OFF;
	factory.acl.state = TEST_OFF;
	factory.key.state = TEST_OFF;
	factory.voice.state = TEST_OFF;
	factory.bat.state = TEST_OFF;
	factory.wire.state = TEST_OFF;
}

void SendInt(uint32_t val) {

	LEUART_Tx(LEUART0, val);
	LEUART_Tx(LEUART0, (val>>8));
	LEUART_Tx(LEUART0, (val>>16));
	LEUART_Tx(LEUART0, (val>>24));

}
void SendCmdResponse(char response) {

	//Command response
	LEUART_Tx(LEUART0, 'R');
	LEUART_Tx(LEUART0, response);
}

void WaitCmd2Finish(int state) {

	int waitcount = 100;
	while (waitcount--) {

		Delay(50);
		if(state == TEST_OFF)
			return;
	}

	//Test case failed
	LEUART_Tx(LEUART0, 'R');
	LEUART_Tx(LEUART0, 'F');
	return;
}

void Handle_Factory(tFactoryCommand commands) {

	if(factory.state == TEST_ON ) {
		switch(commands) {

			case INIT_FACTORY_TEST:
				factory.state = TEST_ON;
				break;

			case LED_RED:
				if(factory.led_red.state == TEST_OFF) {
					led_flash_color(LED_CLR_RED);
					factory.led_red.state = TEST_ON;
				} else {

					led_flash_off();
					factory.led_red.state = TEST_OFF;
				}
				if(factory.led_green.state == TEST_ON)
				factory.led_green.state = TEST_OFF;
				if (factory.led_blue.state == TEST_ON)
				factory.led_blue.state = TEST_OFF;
				SendCmdResponse(CMD_SUCCESS);
				break;

			case LED_GREEN:
				if(factory.led_green.state == TEST_OFF) {
					led_flash_color(LED_CLR_GREEN);
					factory.led_green.state = TEST_ON;
				} else {
						led_flash_off();
						factory.led_green.state = TEST_OFF;
				}
				if(factory.led_red.state == TEST_ON)
				factory.led_red.state = TEST_OFF;
				if (factory.led_blue.state == TEST_ON)
				factory.led_blue.state = TEST_OFF;
				SendCmdResponse(CMD_SUCCESS);
				break;

			case LED_BLUE:
				if(factory.led_blue.state == TEST_OFF) {
					led_flash_color(LED_CLR_BLUE);
					factory.led_blue.state = TEST_ON;
				} else {
					led_flash_off();
					factory.led_blue.state = TEST_OFF;
				}
				if(factory.led_red.state == TEST_ON)
					factory.led_red.state = TEST_OFF;
				if (factory.led_green.state == TEST_ON)
					factory.led_green.state = TEST_OFF;
				SendCmdResponse(CMD_SUCCESS);
				break;

			case PIR:
				if(factory.pir.state == TEST_OFF){
					factory.pir.state = TEST_ON;
					//WaitCmd2Finish(factory.pir.state);
					//SendCmdResponse(resp);
				}
				break;

			case GSEN:
				if(factory.acl.state == TEST_OFF) {
					factory.acl.state = TEST_ON;
				}
				break;

			case KEY:
				if(factory.key.state == TEST_OFF) {
					factory.key.state = TEST_ON;
				}
				break;

			case VOICE:
					factory.voice.state = TEST_ON;
				break;
			case BAT: {
					uint32_t vol = fuelgauge_get_voltage_mV();
					uint32_t soc = fuelgauge_get_state_of_charge_percent();
					LEUART_Tx(LEUART0, 'R');
					SendInt(vol);
					SendInt(soc);
				}break;

			case USB_TEST:
				if(factory.wire.state == TEST_OFF) {
					factory.wire.state = TEST_ON;
					//WaitCmd2Finish(factory.wire.state);
					//SendCmdResponse(resp);
				}
				break;
			case CMD_STANDBY:
				factory.standby.state = TEST_ON;
				break;
		}
	}
	factory.state = TEST_OFF;
}
