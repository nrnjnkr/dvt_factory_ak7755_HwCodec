
#include <stdio.h>
#include "factory_tests.h"

char* testname(int i) {

    switch(i) {
        case AM_MP_ETHERNET:
            return "eth";
        case AM_MP_BUTTON:
            return "button";
        case AM_MP_LED:
            return "LED's";
        case AM_MP_IRLED:
            return "IRLED";
        case AM_MP_LIGHT_SENSOR:
            return "light_sensor";
        case AM_MP_MAC_ADDR:
            return "mac_addr";
        case AM_MP_WIFI_STATION:
            return "wifi";
        case AM_MP_MIC:
            return "mic";
        case AM_MP_SPEAKER:
            return "speaker";
        case AM_MP_SENSOR:
            return "sensor";
        case AM_MP_LENS_FOCUS:
            return "lens_focus";
        case AM_MP_LENS_SHADING_CALIBRATION:
            return "lens_shad";
        case AM_MP_BAD_PIXEL_CALIBRATION:
            return "bad_pixel";
        case AM_MP_FW_INFO:
            return "get_firmware_info";
        case AM_MP_SDCARD:
            return "sdcard";
        case AM_MP_IRCUT:
            return "ircut";
        case AM_MP_COMLETE_TEST:
            return "all";
        case AM_MP_MCU_TESTS:
            return "Mcu Initialization";
        case AM_MP_PIR:
            return "PIR";
        case AM_MP_GSENS:
            return "G-Sensor";
        case AM_MP_VOICE_DECT:
            return "Voice Detect";
        case AM_MP_BATTERY:
            return "Battery";
        case AM_MP_USB:
            return "USB";
        case AM_MP_BLUETOOTH:
            return "Bluetooth";
        case AM_MP_LOAD_KEY:
            return "DownloadKeys";
        case AM_MP_STANDBY:
            return "StandBy";
        default:
            return "Unknown";

    }
}


am_mp_result_t test(int fd, char *cmd_name, char *argv[], int argc) {

    factory_handler cmd_handler = NULL;
    if(!strcmp(cmd_name, "eth")) {
        cmd_handler = test_cmd[AM_MP_ETHERNET];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "button")) {
        cmd_handler = test_cmd[AM_MP_BUTTON];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "led")) {

        cmd_handler = test_cmd[AM_MP_LED];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "irled")) {
        cmd_handler = test_cmd[AM_MP_IRLED];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "light_sensor")) {
        cmd_handler = test_cmd[AM_MP_LIGHT_SENSOR];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "mac_addr")) {
        cmd_handler = test_cmd[AM_MP_MAC_ADDR];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "wifi")) {
        cmd_handler = test_cmd[AM_MP_WIFI_STATION];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "mic")) {
        cmd_handler = test_cmd[AM_MP_MIC];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "speaker")) {
        cmd_handler = test_cmd[AM_MP_SPEAKER];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "sensor")) {
        cmd_handler = test_cmd[AM_MP_SENSOR];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "lens_focus")) {
        cmd_handler = test_cmd[AM_MP_LENS_FOCUS];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "lens_shad")) {
        cmd_handler = test_cmd[AM_MP_LENS_SHADING_CALIBRATION];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "bad_pixel")) {
        cmd_handler = test_cmd[AM_MP_BAD_PIXEL_CALIBRATION];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "get_firmware_info")) {
        cmd_handler = test_cmd[AM_MP_FW_INFO];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "sdcard")) {
        cmd_handler = test_cmd[AM_MP_SDCARD];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "ircut")) {
        cmd_handler = test_cmd[AM_MP_IRCUT];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "all")) {
        cmd_handler = test_cmd[AM_MP_COMLETE_TEST];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "mcu")) {
        cmd_handler = test_cmd[AM_MP_MCU_TESTS];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "pir")) {
        cmd_handler = test_cmd[AM_MP_PIR];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "gsensor")) {
        cmd_handler = test_cmd[AM_MP_GSENS];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "voice_detect")) {
        cmd_handler = test_cmd[AM_MP_VOICE_DECT];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "battery")) {
        cmd_handler = test_cmd[AM_MP_BATTERY];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "usb")) {
        cmd_handler = test_cmd[AM_MP_USB];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "bluetooth")) {
        cmd_handler = test_cmd[AM_MP_BLUETOOTH];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "keydownload")) {
        cmd_handler = test_cmd[AM_MP_LOAD_KEY];
        return cmd_handler(fd,argv,argc);
    }
    if(!strcmp(cmd_name, "standby")) {
        cmd_handler = test_cmd[AM_MP_STANDBY];
        return cmd_handler(fd,argv,argc);
    }
}
