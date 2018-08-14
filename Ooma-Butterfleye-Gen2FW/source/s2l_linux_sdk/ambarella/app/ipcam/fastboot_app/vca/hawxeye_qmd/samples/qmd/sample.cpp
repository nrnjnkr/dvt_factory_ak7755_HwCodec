/**
 * @copyright 2016 HawXeye, Inc.  All rights reserved.
 */
#include <iostream>
#include <stdlib.h>

#include "hreturn.hpp"

extern "C" {
#include "QuickMotionDetector.h"
}

/**
 * @brief Test function that shows functionality of the
 *        <code>QuickMotionDetector</code>.
 *
 * @return Exit code for the process - 0 for success, else an error code.
 */
int main(int argc, char* argv[]) {
    // Initialize QuickMotionDetector
    int32_t retval = QuickMotionDetectorInitialize("./configs/s2lm/client.xml");
    if (retval != RETURN_SUCCESS) {
        std::cerr << "Engine initialization failed" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Engine initialized\n";

    // TODO: Replace these static frames with input queried
    // from camera, file, stream, etc.
    const int FRAME_WIDTH = 8;
    const int FRAME_HEIGHT = 8;
    unsigned char frame[] = {0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0};

    // Run detection on a frame.  Normally, QuickMotionDetectorExecute()
    // should be run in a tight loop, after the next frame is retrieved.
    // This function will return false until the first detection is found,
    // after which it will always return true.
    // Note: This is not enough input to return any detections.
    bool detections;
    retval = QuickMotionDetectorExecute(&detections, frame, FRAME_WIDTH, FRAME_HEIGHT);
    if (retval != RETURN_SUCCESS) {
        std::cerr << "QuickMotionDetectorExecute() failed" << std::endl;
        return EXIT_FAILURE;
    }

    if (detections) {
        std::cout << "Object detected\n";
    } else {
        std::cout << "Object not detected\n";
    }

    std::cout << "Exiting engine\n";
    retval = QuickMotionDetectorDestroy();
    if (retval != RETURN_SUCCESS) {
        std::cerr << "Engine exit failed" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
