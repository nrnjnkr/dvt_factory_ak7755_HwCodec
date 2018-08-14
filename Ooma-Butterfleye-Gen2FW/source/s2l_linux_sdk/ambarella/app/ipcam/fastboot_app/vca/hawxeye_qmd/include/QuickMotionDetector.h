/**
 * @file QuickMotionDetector.h
 *
 * @brief Header file for HawXeye's Quick Motion Detection engine.
 *
 * @copyright 2016 HawXeye, Inc.  All rights reserved.
 */

#ifndef QMD_QUICKMOTIONDETECTOR_H_
#define QMD_QUICKMOTIONDETECTOR_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

    /**
     * @brief Initialize Quick Motion Detection engine.
     *
     * @param[in] configFileName path to client configuration XML file
     *
     * @return <code>RETURN_SUCCESS</code> upon successful execution, and
     *         <code>RETURN_FAILURE</code> upon failure.
     */
int32_t QuickMotionDetectorInitialize(const char* configFileName);

    /**
     * @brief Execute the core functionality of Quick Motion Detection.
     *
     * <p>This function performs object detection using the input 8-bit
     * Y-component frame.  Once an object is detected, this function will
     * continue to return true.</p>
     *
     * @param[out] detected TRUE if an object was detected.
     * @param[in] frame Y-component frame to run object detection on.
     * @param[in] width width of the frame in pixels.
     * @param[in] height height of the frame in pixels.
     *
     * @return <code>RETURN_SUCCESS</code> upon successful execution, and
     *         <code>RETURN_FAILURE</code> upon failure.
     */
int32_t QuickMotionDetectorExecute(bool* detected,
                                   unsigned char* frame,
                                   uint32_t width,
                                   uint32_t height);

    /**
     * @brief Destroy Quick Motion Detection engine.
     *
     * @return <code>RETURN_SUCCESS</code> upon successful execution, and
     *         <code>RETURN_FAILURE</code> upon failure.
     */
int32_t QuickMotionDetectorDestroy();

#ifdef __cplusplus
}
#endif


#endif
