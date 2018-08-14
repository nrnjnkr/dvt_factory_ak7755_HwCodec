// COPYRIGHT (C) 2018 TEKNIQUE LIMITED
// ALL RIGHTS RESERVED. FOR LICENSING INFORMATION CONTACT LICENSE@TEKNIQUE.COM

// INCLUDES //////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_emu.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_pcnt.h"
#include "em_rtc.h"
#include "em_leuart.h"
#include "em_i2c.h"
#include "em_timer.h"
#include "em_adc.h"

#include "config.h"

#include "am_base.h"
#include "am_uart.h"
#include "am_utility.h"
#include "pir.h"
#include "potentiometer.h"
#include "accelerometer.h"
#include "fuel_gauge.h"
#include "tek_msg.h"

#include "factory_test.h"
extern tFactoryTests factory;
// DEFINES ///////////////////////////////////////////////////////////////////
//  8     |  8   | 16   |    8    | 8        |       |   16    (bits)
// HEADER | TYPE | LEN {| SUBTYPE | SUBTYPE1 | ......|} CRC16
#define MAX_DATA_LEN                            64          // Max length of {}
#define MAX_MSG_LEN                             (MAX_DATA_LEN + 6)
#define MIN_MSG_LEN                             8           // 1 + 1 + 2 + 2 + 2

#define MSG_HEADER                              0x7e        // convert to 7d 5e
#define MSG_HEADER_WRAP0                        0x7d        // convert to 7d 5d
#define MSG_HEADER_WRAP1                        0x5e
#define MSG_HEADER_WRAP2                        0x5d

#define MSG_HEADER_POS                          0
#define MSG_TYPE_POS                            1   // msg_type_t
#define MSG_LEN_POS                             2   // length of {}
#define MSG_DATA_POS                            4   // start of {}

// Offset of {
#define MSG_SUB_TYPE_OFFSET                     0   // msg_sub_type_t
#define MSG_SUB_TYPE1_OFFSET                    1   // msg_pir_type_t | msg_accel_type_t ...
#define MSG_USER_DATA_OFFSET                    2   // Data offset inside of {}

#define CONVERT_TO_INT32(d)                     (int32_t)(((d)[3]<<24)|((d)[2]<<16)|((d)[1]<<8)|(d)[0])
#define CONVERT_INT8_TO_INT32(d)                (int32_t)()

#define RESET_MSG_BUF() do { \
    memset(&msg_rx_buf, 0, sizeof(msg_rx_buf)); \
    msg_rx_buf.status = MSG_STATUS_GET_HEADER; \
} while (0)

#define CHECK_WRAP_DATA(data, ret) do {\
    ret = 0; \
    if (msg_rx_buf.wrap_flag == 1) { \
        if (data == MSG_HEADER_WRAP1) { data = MSG_HEADER; } \
        else if (data == MSG_HEADER_WRAP2) { data = MSG_HEADER_WRAP0; } \
        else { ret = -1; } \
        msg_rx_buf.wrap_flag = 0; \
    } else { \
        if (data == MSG_HEADER_WRAP0) { msg_rx_buf.wrap_flag = 1; ret = 1; } \
    } \
} while (0)

// TYPEDEFS, STRUCTS, ENUMS //////////////////////////////////////////////////
        
typedef
enum _msg_status_t {
    MSG_STATUS_GET_HEADER,
    MSG_STATUS_GET_TYPE,
    MSG_STATUS_GET_LEN0,
    MSG_STATUS_GET_LEN1,
    MSG_STATUS_GET_DATA,
    MSG_STATUS_GET_CRC,
    MSG_STATUS_MAX
} msg_status_t;

typedef
struct _msg_buf_t {
    uint16_t        msg_len;            // parser frome msg
    uint16_t        cur_len;            // actual data pushed to buf
    uint8_t         wrap_flag;
    uint8_t         buf[MAX_DATA_LEN];
    uint8_t         crc[2];
    msg_type_t      msg_type;
    msg_status_t    status;
} msg_buf_t;

// GLOBAL DATA ///////////////////////////////////////////////////////////////
extern CircularBuffer _rxBuf;

// EXTERNAL FUNCTION /////////////////////////////////////////////////////////
void mcu_uart_sleep_trigger(void);
uint8_t mcu_get_wakeup_trigger(void);


// PRIVATE VARIABLES /////////////////////////////////////////////////////////
msg_buf_t msg_rx_buf = {0};
uint8_t   msg_tx_buf[MAX_MSG_LEN] = {0};
const static unsigned short crc_table[256] = {
0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5,
0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b,
0xc18c, 0xd1ad, 0xe1ce, 0xf1ef, 0x1231, 0x0210,
0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c,
0xf3ff, 0xe3de, 0x2462, 0x3443, 0x0420, 0x1401,
0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b,
0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6,
0x5695, 0x46b4, 0xb75b, 0xa77a, 0x9719, 0x8738,
0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 0x48c4, 0x58e5,
0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969,
0xa90a, 0xb92b, 0x5af5, 0x4ad4, 0x7ab7, 0x6a96,
0x1a71, 0x0a50, 0x3a33, 0x2a12, 0xdbfd, 0xcbdc,
0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03,
0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd,
0xad2a, 0xbd0b, 0x8d68, 0x9d49, 0x7e97, 0x6eb6,
0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a,
0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb,
0xd10c, 0xc12d, 0xf14e, 0xe16f, 0x1080, 0x00a1,
0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c,
0xe37f, 0xf35e, 0x02b1, 0x1290, 0x22f3, 0x32d2,
0x4235, 0x5214, 0x6277, 0x7256, 0xb5ea, 0xa5cb,
0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447,
0x5424, 0x4405, 0xa7db, 0xb7fa, 0x8799, 0x97b8,
0xe75f, 0xf77e, 0xc71d, 0xd73c, 0x26d3, 0x36f2,
0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9,
0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827,
0x18c0, 0x08e1, 0x3882, 0x28a3, 0xcb7d, 0xdb5c,
0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0,
0x2ab3, 0x3a92, 0xfd2e, 0xed0f, 0xdd6c, 0xcd4d,
0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07,
0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba,
0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74,
0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};
// PRIVATE FUNCTION DECLARATIONS /////////////////////////////////////////////
static uint16_t crc_16( const unsigned char *input_str, uint32_t num_bytes )
{
	uint16_t crc;
	const unsigned char *ptr;
	uint32_t a;

	crc = 0x0000;
	ptr = input_str;

	if ( ptr != NULL ) for (a=0; a<num_bytes; a++) {
		crc = (crc >> 8) ^ crc_table[ (crc ^ (uint16_t) *ptr++) & 0x00FF ];
	}

	return crc;
}
#if 0
static uint16_t update_crc_16( uint16_t crc, unsigned char c )
{
	return (crc >> 8) ^ crc_table[ (crc ^ (uint16_t) c) & 0x00FF ];
}
#endif

static void handle_fuelgauge_cmd(uint8_t cmd)
{
    switch (cmd)
    {
    case FUELGAUGE_PROBE:
    {
        uint8_t resp = fuelgauge_probe() == 0 ? MCU_MSG_RSP_OK : MCU_MSG_RSP_NG;
        tek_msg_encode(MSG_TYPE_RSP, MSG_SUB_TYPE_FUELGAUGE, cmd,
                       (uint8_t *) &resp, sizeof(resp));
    }
        break;
    case FUELGAUGE_DEVICE_NAME:
    {
        const char *name = fuelgauge_get_name();
        tek_msg_encode(MSG_TYPE_RSP, MSG_SUB_TYPE_FUELGAUGE, cmd,
                       (uint8_t *) name, strlen(name));
    }
        break;
    case FUELGAUGE_VOLTAGE:
    {
        uint32_t vol = fuelgauge_get_voltage_mV();
        tek_msg_encode(MSG_TYPE_RSP, MSG_SUB_TYPE_FUELGAUGE, cmd,
                       (uint8_t *) &vol, sizeof(vol));
    }
        break;
    case FUELGAUGE_FULL_CAPACITY:
    {
        uint32_t cap = fuelgauge_get_full_capacity_mAh();
        tek_msg_encode(MSG_TYPE_RSP, MSG_SUB_TYPE_FUELGAUGE, cmd,
                       (uint8_t *) &cap, sizeof(cap));
    }
        break;
    case FUELGAUGE_REMAINING_CAPACITY:
    {
        uint32_t cap = fuelgauge_get_remaining_capacity_mAh();
        tek_msg_encode(MSG_TYPE_RSP, MSG_SUB_TYPE_FUELGAUGE, cmd,
                       (uint8_t *) &cap, sizeof(cap));
    }
        break;
    case FUELGAUGE_STATE_OF_CHARGE:
    {
        uint32_t soc = fuelgauge_get_state_of_charge_percent();
        tek_msg_encode(MSG_TYPE_RSP, MSG_SUB_TYPE_FUELGAUGE, cmd,
                       (uint8_t *) &soc, sizeof(soc));
    }
        break;
    case FUELGAUGE_STATE_OF_HEALTH:
    {
        uint32_t soh = fuelgauge_get_state_of_health_percent();
        tek_msg_encode(MSG_TYPE_RSP, MSG_SUB_TYPE_FUELGAUGE, cmd,
                       (uint8_t *) &soh, sizeof(soh));
    }
        break;
    case FUELGAUGE_TEMPERATURE:
    {
        float temp = fuelgauge_get_temperature_c();
        tek_msg_encode(MSG_TYPE_RSP, MSG_SUB_TYPE_FUELGAUGE, cmd,
                       (uint8_t *) &temp, sizeof(temp));
    }
        break;
    case FUELGAUGE_AVERAGE_CURRENT:
    {
        int32_t cur = fuelgauge_get_average_current_mA();
        tek_msg_encode(MSG_TYPE_RSP, MSG_SUB_TYPE_FUELGAUGE, cmd,
                       (uint8_t *) &cur, sizeof(cur));
    }
        break;
    default:
    {
        uint8_t resp = MCU_MSG_RSP_NOT_IMPLEMENTED;
        tek_msg_encode(MSG_TYPE_RSP, MSG_SUB_TYPE_FUELGAUGE, cmd,
                       (uint8_t *) &resp, sizeof(resp));
    }
        break;
    }
}

static int execute_cmd()
{
    uint8_t *p_d = msg_rx_buf.buf;
    int i;

    tek_print("Recv :");
    for (i=0; i<msg_rx_buf.msg_len; i++) {
        tek_print("%02x ", p_d[i]);
    }
    tek_print("\r\n");

    switch (p_d[MSG_SUB_TYPE_OFFSET]) {
        // PIR ////////////////////////////////////////////////////////////////////
        case MSG_SUB_TYPE_PIR:
            switch (p_d[MSG_SUB_TYPE1_OFFSET]) {
                case PIR_PROBE:
                {
                    uint8_t response = 0;
                    int ret;
                    ret = pir_probe();
                    response = (ret == 0)?MCU_MSG_RSP_OK:MCU_MSG_RSP_NG;
                    tek_msg_encode(MSG_TYPE_RSP,
                                    MSG_SUB_TYPE_PIR,
                                    PIR_PROBE,
                                    &response,
                                    1);
                    break;
                }
                case PIR_SENS_SET:
                {
                    uint8_t sensitivity = p_d[MSG_USER_DATA_OFFSET];
                    uint8_t response = 0;
                    int ret;
                    ret = pir_sensitivity_set(sensitivity);
                    response = (ret == 0)?MCU_MSG_RSP_OK:MCU_MSG_RSP_NG;
                    tek_msg_encode(MSG_TYPE_RSP,
                                    MSG_SUB_TYPE_PIR,
                                    PIR_SENS_SET,
                                    &response,
                                    1);
                    break;
                }
                case PIR_SENS_GET:
                {
                    uint8_t sensitivity;
                    sensitivity = pir_sensitivity_get();
                    tek_msg_encode(MSG_TYPE_RSP,
                                    MSG_SUB_TYPE_PIR,
                                    PIR_SENS_GET,
                                    &sensitivity,
                                    1);
                    break;
                }
                default:
                    break;
            }
            break;
        // VOICE //////////////////////////////////////////////////////////////////
        case MSG_SUB_TYPE_VOICE:
            switch (p_d[MSG_SUB_TYPE1_OFFSET]) {
                case VOICE_SENS_SET:
                {
                    uint8_t sensitivity = p_d[MSG_USER_DATA_OFFSET];
                    uint8_t ret = 0;
                    ret = potentiometer_set(sensitivity);
                    ret = (ret == 0)?MCU_MSG_RSP_OK:MCU_MSG_RSP_NG;
                    tek_msg_encode(MSG_TYPE_RSP,
                                    MSG_SUB_TYPE_VOICE,
                                    VOICE_SENS_SET,
                                    &ret,
                                    1);
                    break;
                }
                case VOICE_SENS_GET:
                {
                    uint8_t sensitivity;
                    sensitivity = potentiometer_get();
                    tek_msg_encode(MSG_TYPE_RSP,
                                    MSG_SUB_TYPE_VOICE,
                                    VOICE_SENS_GET,
                                    &sensitivity,
                                    1);
                    break;
                }
                default:
                    break;
            }
            break;
        // CHARGER ////////////////////////////////////////////////////////////////
        case MSG_SUB_TYPE_CHARGER:
            break;
        // MCU ////////////////////////////////////////////////////////////////////
        case MSG_SUB_TYPE_MCU:
            switch (p_d[MSG_SUB_TYPE1_OFFSET]) {
                case MCU_SLEEP:
                {
                    mcu_uart_sleep_trigger();
                    break;
                }
                case MCU_WAKEUP_TRIGGER:
                {
                    uint8_t wakeup_trigger;
                    wakeup_trigger = mcu_get_wakeup_trigger();
                    tek_msg_encode(MSG_TYPE_RSP,
                                    MSG_SUB_TYPE_MCU,
                                    MCU_WAKEUP_TRIGGER,
                                    &wakeup_trigger,
                                    1);
                    break;
                }
                default:
                    break;
            }
            break;
        // ACCEL //////////////////////////////////////////////////////////////////
        case MSG_SUB_TYPE_ACCEL:
            switch (p_d[MSG_SUB_TYPE1_OFFSET]) {
                case ACCEL_PROBE:
                {
                    uint8_t response;
                    int ret;

                    ret = accelerometer_probe();
                    response = (ret == 0)?MCU_MSG_RSP_OK:MCU_MSG_RSP_NG;
                    tek_msg_encode(MSG_TYPE_RSP,
                                    MSG_SUB_TYPE_ACCEL,
                                    ACCEL_PROBE,
                                    &response,
                                    1);
                    break;
                }
                case ACCEL_OFFSET_SET:
                {
                    uint8_t response = MCU_MSG_RSP_NG;
                    int ret;
                    int8_t x, y, z;

                    if (msg_rx_buf.msg_len == 5)
                    {
                        x = (int8_t)p_d[MSG_USER_DATA_OFFSET];
                        y = (int8_t)p_d[MSG_USER_DATA_OFFSET+1];
                        z = (int8_t)p_d[MSG_USER_DATA_OFFSET+2];
                        ret = accelerometer_offset_set(x, y, z);
                        response = (ret == 0)?MCU_MSG_RSP_OK:MCU_MSG_RSP_NG;
                    }

                    tek_msg_encode(MSG_TYPE_RSP,
                                    MSG_SUB_TYPE_ACCEL,
                                    ACCEL_OFFSET_SET,
                                    &response,
                                    1);
                    break;
                }
                case ACCEL_OFFSET_GET:
                {
                    int ret;
                    int8_t x, y, z;
                    uint8_t data[3];

                    ret = accelerometer_offset_get(&x, &y, &z);
                    if (ret == 0)
                    {
                        data[0] = (uint8_t)x;
                        data[1] = (uint8_t)y;
                        data[2] = (uint8_t)z;
                        tek_msg_encode(MSG_TYPE_RSP,
                                        MSG_SUB_TYPE_ACCEL,
                                        ACCEL_OFFSET_GET,
                                        data,
                                        3);
                    }
                    else
                    {
                        uint8_t response = MCU_MSG_RSP_NG;
                        tek_msg_encode(MSG_TYPE_RSP,
                                        MSG_SUB_TYPE_ACCEL,
                                        ACCEL_OFFSET_GET,
                                        &response,
                                        1);
                    }
                    break;
                }
                case ACCEL_ACC_VALUE_GET:
                {
                    int ret;
                    int16_t x, y, z;
                    uint8_t data[6];

                    ret = accelerometer_accel_get(&x, &y, &z);
                    if (ret == 0)
                    {
                        // Convert to network order
                        data[0] = (uint8_t)(x >> 8);
                        data[1] = (uint8_t)(x & 0xff);
                        data[2] = (uint8_t)(y >> 8);
                        data[3] = (uint8_t)(y & 0xff);
                        data[4] = (uint8_t)(z >> 8);
                        data[5] = (uint8_t)(z & 0xff);
                        tek_msg_encode(MSG_TYPE_RSP,
                                        MSG_SUB_TYPE_ACCEL,
                                        ACCEL_ACC_VALUE_GET,
                                        data,
                                        6);
                    }
                    else
                    {
                        uint8_t response = MCU_MSG_RSP_NG;
                        tek_msg_encode(MSG_TYPE_RSP,
                                        MSG_SUB_TYPE_ACCEL,
                                        ACCEL_ACC_VALUE_GET,
                                        &response,
                                        1);
                    }
                    break;
                }
                case ACCEL_TEMP_VALUE_GET:
                {
                    int8_t temp;

                    accelerometer_temperature_get(&temp);
                    tek_msg_encode(MSG_TYPE_RSP,
                                    MSG_SUB_TYPE_ACCEL,
                                    ACCEL_TEMP_VALUE_GET,
                                    (uint8_t *)&temp,
                                    1);
                    break;
                }
                case ACCEL_RANGE_SET:
                {
                    acc_accel_range_t range;
                    int ret;
                    uint8_t response;
                    uint8_t data = p_d[MSG_USER_DATA_OFFSET];

                    switch (data) {
                        case 2:
                            range = ACC_RANGE_2G;
                            break;
                        case 4:
                            range = ACC_RANGE_4G;
                            break;
                        case 8:
                            range = ACC_RANGE_8G;
                            break;
                        case 16:
                            range = ACC_RANGE_16G;
                            break;
                        default:
                            data = 0;
                            break;
                    }
                    ret = accelerometer_range_set(range);
                    response = (ret == 0)?MCU_MSG_RSP_OK:MCU_MSG_RSP_NG;
                    tek_msg_encode(MSG_TYPE_RSP,
                                    MSG_SUB_TYPE_ACCEL,
                                    ACCEL_RANGE_SET,
                                    &response,
                                    1);
                    break;
                }
                case ACCEL_RANGE_GET:
                {
                    acc_accel_range_t range;
                    uint8_t data;

                    accelerometer_range_get(&range);
                    switch (range) {
                        case ACC_RANGE_2G:
                            data = 2;
                            break;
                        case ACC_RANGE_4G:
                            data = 4;
                            break;
                        case ACC_RANGE_8G:
                            data = 8;
                            break;
                        case ACC_RANGE_16G:
                            data = 16;
                            break;
                        default:
                            data = 0;
                            break;
                    }
                    tek_msg_encode(MSG_TYPE_RSP,
                                    MSG_SUB_TYPE_ACCEL,
                                    ACCEL_RANGE_GET,
                                    &data,
                                    1);
                    break;
                }
                default:
                    break;
            }
            break;
        // FUELGAUGE //////////////////////////////////////////////////////////////////
        case MSG_SUB_TYPE_FUELGAUGE:
            handle_fuelgauge_cmd(p_d[MSG_SUB_TYPE1_OFFSET]);
            break;
        default:
        {
            uint8_t resp = MCU_MSG_RSP_NOT_IMPLEMENTED;
            tek_msg_encode(MSG_TYPE_RSP, p_d[MSG_SUB_TYPE_OFFSET], p_d[MSG_SUB_TYPE1_OFFSET],
                           &resp, sizeof(uint8_t));
        }
            break;
    }

    return 0;
}

static int push_to_parser(uint8_t data)
{
    int8_t ret;

    tek_print("-> push_to_parser %02x, %d\r\n", data, msg_rx_buf.status);

    if (data == MSG_HEADER) {
        // no 0x7e in data
        RESET_MSG_BUF();
    }

    switch (msg_rx_buf.status) {
        case MSG_STATUS_GET_HEADER:
            if (data == MSG_HEADER) {
                msg_rx_buf.status = MSG_STATUS_GET_TYPE;        
            } else {
                goto ERR;
            }
            break;
        case MSG_STATUS_GET_TYPE:
            // suppose no 0x7d in type
            if (data < MSG_TYPE_MAX) {
                msg_rx_buf.status = MSG_STATUS_GET_LEN0;
                msg_rx_buf.msg_type = (msg_type_t)data;
            } else {
                goto ERR;
            }
            break;
        case MSG_STATUS_GET_LEN0:
        case MSG_STATUS_GET_LEN1:
            CHECK_WRAP_DATA(data, ret);
            if (ret == 0) {
                if (msg_rx_buf.status == MSG_STATUS_GET_LEN0) {
                    msg_rx_buf.msg_len = (uint16_t)((data << 8)&0xff00);
                    msg_rx_buf.status = MSG_STATUS_GET_LEN1;
                } else {
                    msg_rx_buf.msg_len |= (uint16_t)data;
                    msg_rx_buf.status = MSG_STATUS_GET_DATA;
                }
            } else if (ret < 0) {
                goto ERR;
            }
            break;
        case MSG_STATUS_GET_DATA:
            CHECK_WRAP_DATA(data, ret);
            if (ret == 0) {
                msg_rx_buf.buf[msg_rx_buf.cur_len++] = data;
                if (msg_rx_buf.cur_len == msg_rx_buf.msg_len) {
                    msg_rx_buf.status = MSG_STATUS_GET_CRC;
                    msg_rx_buf.cur_len = 0;
                }
            } else if (ret < 0) {
                goto ERR;
            }
            break;
        case MSG_STATUS_GET_CRC:
            CHECK_WRAP_DATA(data, ret);
            if (ret == 0) {
                uint16_t cacl_crc, msg_crc;
                msg_rx_buf.crc[msg_rx_buf.cur_len++] = data;
                if (msg_rx_buf.cur_len >= 2) {
                    msg_rx_buf.status = MSG_STATUS_GET_HEADER;
                    cacl_crc = crc_16(msg_rx_buf.buf, msg_rx_buf.msg_len);
                    msg_crc = msg_rx_buf.crc[0]<<8 | msg_rx_buf.crc[1];
                    tek_print("crc %x vs %x\r\n", msg_crc, cacl_crc);
                    if (cacl_crc == msg_crc) {
                        execute_cmd();
                    }
                }
            } else if (ret < 0) {
                goto ERR;
            }
            break;
        default:
            break;
    }

    return 0;

ERR:
    tek_print("Error\r\n");
    RESET_MSG_BUF();
    return -1;

}

// PUBLIC FUNCTION DEFINITIONS ///////////////////////////////////////////////
int tek_msg_init()
{
    RESET_MSG_BUF();

    return 0;
}

void myparser(uint8_t data) {

	if(factory.state == TEST_ON ) {
		Handle_Factory(data);
		return;
	}
	switch(data) {
		//Factory command
		case INIT_FACTORY_TEST:
			factory.state = TEST_ON;
			break;
		default:
			//Unknown command:
		break;
	}

}

int UartHandler()
{
    if (_rxBuf.pendingBytes == 0)
        return -1;

    while (_rxBuf.pendingBytes > 0) {
        uint8_t data = _rxBuf.data[_rxBuf.rdI];
        _rxBuf.rdI = (_rxBuf.rdI + 1) % BUFFERSIZE;
        _rxBuf.pendingBytes--;

        myparser(data);
    }

    return 0;
}

int tek_msg_encode(msg_type_t msg_type,
                            msg_sub_type_t sub_type,
                            uint8_t sub_type1,
                            uint8_t * data,
                            uint16_t len)
{
    uint16_t crc;
    uint8_t tmp_buf[MAX_MSG_LEN] = {0};
    int i, j;
    uint16_t msg_len = len + 2;

    memset(tmp_buf, 0, sizeof(tmp_buf));
    tmp_buf[MSG_HEADER_POS]     = MSG_HEADER;
    tmp_buf[MSG_TYPE_POS]       = msg_type;
    tmp_buf[MSG_LEN_POS]        = (uint8_t)((msg_len >> 8) & 0xff); // add subtype and subtype1
    tmp_buf[MSG_LEN_POS+1]      = (uint8_t)(msg_len & 0xff); // add subtype and subtype1
    tmp_buf[MSG_DATA_POS + MSG_SUB_TYPE_OFFSET]         = sub_type;
    tmp_buf[MSG_DATA_POS + MSG_SUB_TYPE1_OFFSET]        = sub_type1;
    if (len > 0) {
        memcpy(&tmp_buf[MSG_DATA_POS + MSG_USER_DATA_OFFSET], data, len);
    }

    crc = crc_16(&tmp_buf[MSG_DATA_POS], msg_len);
    tmp_buf[MSG_DATA_POS + msg_len]        = (crc >> 8) & 0xff;
    tmp_buf[MSG_DATA_POS + msg_len + 1]    = crc & 0xff;

    // Convert 0x7e and 0x7d
    msg_tx_buf[0] = tmp_buf[0];
    for (i=1,j=1; i<msg_len+6; i++, j++)
    {
        if (tmp_buf[i] == MSG_HEADER) {
            msg_tx_buf[j++] = MSG_HEADER_WRAP0;
            msg_tx_buf[j] = MSG_HEADER_WRAP1;
        } else if (tmp_buf[i] == MSG_HEADER_WRAP0) {
            msg_tx_buf[j++] = MSG_HEADER_WRAP0;
            msg_tx_buf[j] = MSG_HEADER_WRAP2;
        } else {
            msg_tx_buf[j] = tmp_buf[i];
        }
    }
    tek_send_msg(msg_tx_buf, j);
#if 0    
    tek_print("Send :");
    for (i=0; i<j; i++) {
        tek_print("%02x ", msg_tx_buf[i]);
    }
    tek_print("\r\n");
#endif
    return 0;
}

