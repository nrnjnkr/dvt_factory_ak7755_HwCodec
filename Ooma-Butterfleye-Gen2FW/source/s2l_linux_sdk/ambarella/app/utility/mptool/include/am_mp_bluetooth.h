/*
 * Bluetooth Test Inteface 
 *
 */

enum {
    START_ADV = 0,
    STOP_ADV,
};

am_mp_err_t mptool_bluetooth_handler(am_mp_msg_t *from_msg, 
        am_mp_msg_t *to_msg);

