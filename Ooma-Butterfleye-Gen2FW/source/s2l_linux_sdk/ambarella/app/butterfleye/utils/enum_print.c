#include "log.h"
#include "utils.h"

/* Warning: If you make any change to these enums under utils.h", and do not
 * make a corresponding change here", your logs will be wrong */

const char * gGPIO_Direction[] =
{
    "kGPIO_DirectionIn", // 0x0
    "kGPIO_DirectionOut", // 0x1
};

const char * gBOOT_GpioBootVal[] =
{
    "kBOOT_Config = 0x00",                                                        
    "kBOOT_PirTrigger = 0x01",                                                    
    "kBOOT_AccelerometerTrigger = 0x02",                                          
    "kBOOT_LoudNoiseTrigger = 0x03",                                              
    "kBOOT_WiFiWakeup = 0x04",                                                    
    "kBOOT_Button = 0x05",                                                        
    "kBOOT_Unknown = 0xFF",
};

const char * gBOOT_BootReason[] =
{
    "kBOOT_BootReasonConfig",                                                     
    "kBOOT_BootReasonPir",                                                        
    "kBOOT_BootReasonAccelerometer",                                              
    "kBOOT_BootReasonLoudNoise",                                                  
    "kBOOT_BootReasonWifiWakeupCloud",                                            
    "kBOOT_BootReasonButton",                                                     
    "kBOOT_BootReasonUnknown", 
};

const char * gAPP_ThreadId[] =
{
    "kAPP_InvalidThreadId = 0",
    "kAPP_MainAppThread",
    "kAPP_UartRxThread",
    "kAPP_UartTxThread",
    "kAPP_EventManagerThread",
    "kAPP_FileUploadThread",
    "kAPP_RecordingStreamingThread",
    "kAPP_CloudThread",
    "kAPP_WifiThread",
    "kAPP_BluetoothThread",
    "kAPP_KeepAliveToMcuThread",
    "kAPP_McuLoggerThread",
};

const char * gMSGQ_Message[] =
{
    "kMSGQ_CmdNoneStatusNone = 0",
    "kMSGQ_StatusBootReason",

    "kMSGQ_StatusThreadStarted",
    "kMSGQ_StatusThreadExited",
    "kMSGQ_CmdExitThread",

    "kMSGQ_CmdSendMsgToMcu",
    "kMSGQ_StatusMsgReceivedFromMcu",

    "kMSGQ_CmdStartFileUpload",
    "kMSGQ_CmdStopFileUpload",
    "kMSGQ_StatusFileUploadStarted",
    "kMSGQ_StatusFileUploadStopped",

    "kMSGQ_CmdStartRecording",
    "kMSGQ_CmdStopRecording",
    "kMSGQ_CmdStartStreaming",
    "kMSGQ_CmdStopStreaming",
    "kMSGQ_StatusRecordingStarted",
    "kMSGQ_StatusRecordingStopped",
    "kMSGQ_StatusStreamingStarted",
    "kMSGQ_StatusStreamingStopped",

    "kMSGQ_CmdCloudCheckConnection",
    "kMSGQ_StatusCloudConnectionAlive",
    "kMSGQ_StatusCloudConnectionLost",
    "kMSGQ_CmdCloudCheckWakeupReason",
    "kMSGQ_StatusCloudWakeupStartStream",
    "kMSGQ_CmdGetBatteryStatus",
    "kMSGQ_StatusCloudBatteryStatus",

    "kMSGQ_CmdWifiSetupConnection",
    "kMSGQ_CmdWifiCheckConnection",
    "kMSGQ_StatusWifiConnected",
    "kMSGQ_StatusWifiDisconnected",

};
