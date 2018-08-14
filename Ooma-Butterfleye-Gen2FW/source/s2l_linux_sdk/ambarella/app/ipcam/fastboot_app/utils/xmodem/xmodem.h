//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                                     _                      _             //
//        __  __ _ __ ___    ___    __| |  ___  _ __ ___     | |__          //
//        \ \/ /| '_ ` _ \  / _ \  / _` | / _ \| '_ ` _ \    | '_ \         //
//         >  < | | | | | || (_) || (_| ||  __/| | | | | | _ | | | |        //
//        /_/\_\|_| |_| |_| \___/  \__,_| \___||_| |_| |_|(_)|_| |_|        //
//                                                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//          Copyright (c) 2012 by S.F.T. Inc. - All rights reserved         //
//  Use, copying, and distribution of this software are licensed according  //
//    to the LGPLv2.1, or a BSD-like license, as appropriate (see below)    //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#ifdef ARDUINO
/** \mainpage S.F.T. XMODEM library (ARDUINO version)
  *
  * Copyright (c) 2012 by S.F.T. Inc. - All rights reserved\n
  *
  * The source files include DOXYGEN SUPPORT to properly document the library
  * Please excuse the additional comments necessary to make this work.
  * Instead, build the doxygen output and view the documentation, as
  * well as the code itself WITHOUT all of the doxygen markup comments.
  * \n
  * \n
  * This library was designed to work with POSIX-compliant operating systems
  * such as Linux, FreeBSD, and OSX, and also on Arduino microcontrollers.
  * The intent was to provide an identical code base for both ends of the
  * XMODEM transfer, compilable as either C or C++ code for maximum flexibility.
  *
  * Normally you will only need to use one of these two functions:\n
  * \n
  * \ref XSend() - send a file via XMODEM\n
  * \ref XReceive() - receive a file via XMODEM\n
  * \n
  * The rest of the documentation was provided to help you debug any problems,
  * or even to write your own library (as appropriate).\n
  *
  * LICENSE
  *
  * This software is licensed under either the LGPLv2 or a BSD-like license.
  * For more information, see\n
  *   http://opensource.org/licenses/BSD-2-Clause\n
  *   http://www.gnu.org/licenses/lgpl-2.1.html\n
  * and the above copyright notice.\n
  * \n
  * In short, you may use this software anyway you like, provided that you
  * do not hold S.F.T. Inc. responsible for consequential or inconsequential
  * damages resulting from use, modification, abuse, or anything else done
  * with this software, and you include the appropriate license (either LGPLv2
  * or a BSD-like license) and comply with the requirements of said license.\n
  * So, if you use a BSD-like license, you can copy the license template at
  * the abovementioned URL and sub in the copyright notice as shown above.
  * Or, you may use an LGPLv2 license, and then provide source files with a
  * re-distributed or derived work (including a complete re-write with this
  * library as a template).  A link back to the original source, of course,
  * would be appreciated but is not required.
**/
#else // ARDUINO
/** \mainpage S.F.T. XMODEM library
  *
  * Copyright (c) 2012 by S.F.T. Inc. - All rights reserved\n
  *
  * The source files include DOXYGEN SUPPORT to properly document the library
  * Please excuse the additional comments necessary to make this work.
  * Instead, build the doxygen output and view the documentation, as
  * well as the code itself WITHOUT all of the doxygen markup comments.
  * \n
  * \n
  * This library was designed to work with POSIX-compliant operating systems
  * such as Linux, FreeBSD, and OSX, and also on Arduino microcontrollers.
  * The intent was to provide an identical code base for both ends of the
  * XMODEM transfer, compilable as either C or C++ code for maximum flexibility.
  *
  * Normally you will only need to use one of these two functions:\n
  * \n
  * \ref XSend() - send a file via XMODEM\n
  * \ref XReceive() - receive a file via XMODEM\n
  * \n
  * The rest of the documentation was provided to help you debug any problems,
  * or even to write your own library (as appropriate).\n
  *
  * LICENSE
  *
  * This software is licensed under either the LGPLv2 or a BSD-like license.
  * For more information, see\n
  *   http://opensource.org/licenses/BSD-2-Clause\n
  *   http://www.gnu.org/licenses/lgpl-2.1.html\n
  * and the above copyright notice.\n
  * \n
  * In short, you may use this software anyway you like, provided that you
  * do not hold S.F.T. Inc. responsible for consequential or inconsequential
  * damages resulting from use, modification, abuse, or anything else done
  * with this software, and you include the appropriate license (either LGPLv2
  * or a BSD-like license) and comply with the requirements of said license.\n
  * So, if you use a BSD-like license, you can copy the license template at
  * the abovementioned URL and sub in the copyright notice as shown above.
  * Or, you may use an LGPLv2 license, and then provide source files with a
  * re-distributed or derived work (including a complete re-write with this
  * library as a template).  A link back to the original source, of course,
  * would be appreciated but is not required.
**/
#endif // ARDUINO

/** \file xmodem.h
  * \brief main header file for S.F.T. XMODEM library
  *
  * S.F.T. XMODEM library
**/

/** \defgroup xmodem_api XModem API
  * high-level API functions
*/

/** \defgroup xmodem_internal XModem Internal
  * internal support functions
*/

#ifdef STANDALONE
/** \defgroup xmodem_standalone XModem Stand-alone
  * internal 'standalone' functions, an example for POSIX implementation
*/
#endif // STANDALONE


// determine if arduino build, define ARDUINO if not already done

#if defined(__AVR__) || defined(AVR) || defined(__AVR) || defined(__AVR_ARCH__)
#ifndef ARDUINO
#define ARDUINO /* hopefully I cover all compiler variations */
#endif // ARDUINO
#endif // __AVR__


#include <stdlib.h>

// required include files
#ifdef ARDUINO
// arduino includes
#include <Arduino.h>
#include <SD.h>
#include <HardwareSerial.h> /* may already be included by 'Arduino.h' */
#include <avr/pgmspace.h>

#elif WIN32
// win32 includes
#include <Windows.h>
#include <io.h>
#else // POSIX
// posix includes
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h> // for IOCTL definitions
#include <memory.h>
#endif // OS-dependent includes


// required per-OS definitions
#ifdef ARDUINO

// file and serial types for Arduino
#define FILE_TYPE File
#define SERIAL_TYPE HardwareSerial *

#elif defined(WIN32) // WINDOWS

// file and serial types for WIN32
#define FILE_TYPE HANDLE
#define SERIAL_TYPE HANDLE

#else // POSIX

// file and serial types for POSIX
#define FILE_TYPE int
#define SERIAL_TYPE int

#endif // ARDUINO


// common definitions

#define SILENCE_TIMEOUT 5000 /* 5 seconds */
#define TOTAL_ERROR_COUNT 32
#define ACK_ERROR_COUNT 8


// Arduino build uses C++ so I must define functions properly

#ifdef ARDUINO

/** \ingroup xmodem_api
  * \brief Receive a file using XMODEM protocol (ARDUINO version)
  *
  * \param pSD A pointer to an SDClass object, such as &SD (the default SD library object is 'SD')
  * \param pSer A pointer to a HardwareSerial object, such as &Serial
  * \param szFilename A pointer to a (const) 0-byte terminated string containing the file name
  * \return A value of zero on success, negative on failure, positive if canceled
  *
  * Call this function to receive a file, passing the SD card's initialized SDClass object pointer,
  * and the pointer to the 'HardwareSerial' object to be used for serial communication, and the
  * name of the file to create from the XMODEM stream.  The function will return a value of zero on
  * success.  On failure or cancelation, the file will be deleted.\n
  * If the specified file exists before calling this function, it will be overwritten.  If you do not
  * want to unconditionally overwrite an existing file, you should test to see if it exists first
  * using the SD library.
  *
**/
short XReceive(SDClass *pSD, HardwareSerial *pSer, const char *szFilename);

/** \ingroup xmodem_api
  * \brief Send a file using XMODEM protocol (ARDUINO version)
  *
  * \param pSD A pointer to an SDClass object, such as &SD (the default SD library object is 'SD')
  * \param pSer A pointer to a HardwareSerial object, such as &Serial
  * \param szFilename A pointer to a (const) 0-byte terminated string containing the file name
  * \return A value of zero on success, negative on failure, positive if canceled
  *
  * Call this function to send a file, passing the SD card's initialized SDClass object pointer,
  * and the pointer to the 'HardwareSerial' object to be used for serial communication, and the
  * name of the file to send via the XMODEM stream.  The function will return a value of zero on
  * success.  If the file does not exist, the function will return a 'failure' value and cancel
  * the transfer.
  *
**/
int XSend(SDClass *pSD, HardwareSerial *pSer, const char *szFilename);

#ifdef DEBUG_CODE
const char *XMGetError(void);
#endif // DEBUG_CODE

#else // ARDUINO

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/** \ingroup xmodem_api
  * \brief Receive a file using XMODEM protocol
  *
  * \param hSer A 'HANDLE' for the open serial connection
  * \param szFilename A pointer to a (const) 0-byte terminated string containing the file name
  * \param nMode The file mode to be used on create (RWX bits)
  * \return A value of zero on success, negative on failure, positive if canceled
  *
  * Call this function to receive a file, passing the handle to the open serial connection, and the
  * name and mode of the file to create from the XMODEM stream.  The function will return a value of zero on
  * success.  On failure or cancelation, the file will be deleted.\n
  * If the specified file exists before calling this function, it will be overwritten.  If you do not
  * want to unconditionally overwrite an existing file, you should test to see if it exists first.
  *
**/
int XReceive(SERIAL_TYPE hSer, const char *szFilename, int nMode);

/** \ingroup xmodem_api
  * \brief Send a file using XMODEM protocol
  *
  * \param hSer A 'HANDLE' for the open serial connection
  * \param szFilename A pointer to a (const) 0-byte terminated string containing the file name
  * \return A value of zero on success, negative on failure, positive if canceled
  *
  * Call this function to receive a file, passing the handle to the open serial connection, and the
  * name and mode of the file to send via the XMODEM stream.  The function will return a value of zero on
  * success.  If the file does not exist, the function will return a 'failure' value and cancel
  * the transfer.
  *
**/
int XSend(SERIAL_TYPE hSer, const char *szFilename);

#ifdef DEBUG_CODE
const char *XMGetError(void);
#endif // DEBUG_CODE

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // ARDUINO


