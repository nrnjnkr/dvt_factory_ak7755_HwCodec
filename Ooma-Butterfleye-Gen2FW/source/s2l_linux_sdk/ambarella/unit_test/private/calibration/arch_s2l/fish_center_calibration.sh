#!/bin/sh
## History:
##
## 2016/12/30 - [JianTang] Created file
##
## Copyright (c) 2016 Ambarella, Inc.
##
## This file and its contents ("Software") are protected by intellectual
## property rights including, without limitation, U.S. and/or foreign
## copyrights. This Software is also the confidential and proprietary
## information of Ambarella, Inc. and its licensors. You may not use, reproduce,
## disclose, distribute, modify, or otherwise prepare derivative works of this
## Software or any portion thereof except pursuant to a signed license agreement
## or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
## In the absence of such an agreement, you agree to promptly notify and return
## this Software to Ambarella, Inc.
##
## THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
## INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
## MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
## IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
## INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
## (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
## LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
##

source calib_utils.sh

fec_init_setting()
{
	echo;
	echo "============================================"
	echo "Start to do fisheye center calibration";
	echo "============================================"

  init_setting

	CALIBRATION_FILE_PREFIX=cali_fisheye_center
	CALIBRATION_FILE_PREFIX=${CALIBRATION_FILES_PATH}/${CALIBRATION_FILE_PREFIX}
	CALIBRATION_FILE_NAME=${CALIBRATION_FILE_PREFIX}_prev_M_${width}x${height}.yuv

	if [ -e ${CALIBRATION_FILE_PREFIX} ]; then
		rm -f ${CALIBRATION_FILE_PREFIX}
	fi

}

fec_init_vinvout2()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 1: switch to encode mode 2."
	echo;echo "if VIN >= 8M, set 15fps for the DSP bandwidth limitation"
	echo "============================================"
	echo;

 init_vinvout_mode2

{
$test_image_cmd -i1 <<EOF
e
e
120
EOF
} &
	sleep 3
}

# Step 5: capture yuv file
capture_yuv()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 2: Capture YUV file"
	echo "============================================"
	echo; echo -n "Please make sure the image is focused and cover the lens inside the light box, then press enter key to continue..."
	read keypress

	exe_cmd=$test_yuv_cmd" -b 0 -Y -f "$CALIBRATION_FILE_PREFIX
	echo "# "$exe_cmd
	$exe_cmd
	check_feedback
}

# Step 6: do circle center detection
detect_center()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 3: Detect circel center"
	echo "============================================"
	echo;
	exe_cmd=$cali_center_cmd" -f "${CALIBRATION_FILE_NAME}" -w "$width" -h "$height
	echo "# "$exe_cmd
	$exe_cmd
	check_feedback
}

# Step 7: Decide if do the test one more time
loop_calibration()
{
	again=1
	while [ $again != 0 ]
	do
		accept=0
		while [ $accept != 1 ]
		do
			echo; echo "Run one more test? (Y/N)"
			echo; echo -n "please choose:"
			read action
			case $action in
				y|Y)
					accept=1
					;;
				n|N)
					accept=1
					again=0
					;;
				*)
					;;
			esac
		done
		if [ $again != 0 ]; then
			capture_yuv
			detect_center
		fi
	done
}

finish_calibration()
{
	killall test_idsp
	killall test_image
	echo;echo;
	echo "============================================"
	echo "Step 4: Get the center."
	echo "============================================"
	echo;
	echo "If failed to find center, please check:"
	echo "	1. if the lens is a fisheye lens."
	echo "	2. if the lens is focused or mounted in the right way."
	echo "	3. if the light is not bright enough."
	echo; echo "Thanks for using Ambarella calibration tool. "
	echo "For any questions please send mail to ipcam-sdk-support@ambarella.com"
}

#####main
check_cmd FEC
fec_init_setting
init_sensor
sleep 1
fec_init_vinvout2
capture_yuv
detect_center
loop_calibration
finish_calibration

