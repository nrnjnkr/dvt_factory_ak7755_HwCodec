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


#define all the variables here

source `which calib_utils.sh`

lsc_init_setting()
{
	echo;
	echo "============================================"
	echo "Start to do lens shading calibration";
	echo "============================================"

	init_setting

	CALIBRATION_FILE_NAME="$width"x"$height"_cali_lens_shading.bin
	CALIBRATION_FILE_NAME=${CALIBRATION_FILES_PATH}/${CALIBRATION_FILE_NAME}

	if [ -e ${CALIBRATION_FILE_NAME} ]; then
		rm -f ${CALIBRATION_FILE_NAME}
	fi
}

lsc_init_vinvout()
{
	echo;echo;echo;
	echo "============================================"
	echo "Step 1: switch to enc-mode 0."
	echo "============================================"
	echo;

	init_vinvout_mode0
}

setAntiFlickerMode() {
	accept=0
	echo "=============================================="
	echo "Step 1: Set anti-flicker mode."
	echo "=============================================="
	echo
	while [ $accept -ne 1 ]
	do
		echo "1:  50 Hz"
		echo "2:  60 Hz"
		echo -n "choose anti-flicker mode:"
		read select
		case $select in
			1)
				flckerMode=50
				accept=1
				;;
			2)
				flckerMode=60
				accept=1
				;;
			*)
				echo "Please input valid number, without any other keypress(backspace or delete)!"
				accept=0
				;;
		esac
	done

	return 0
}

setAETargetValue()
{
	accept=0
	echo "=============================================="
	echo "Step 2: Set AE target value."
	echo "=============================================="
	while [ $accept -ne 1 ]
	do
		echo "1:  1024 (1X)"
		echo "2:  2048 (2X)"
		echo "3:  3072 (3X)"
		echo "4:  4096 (4X)"
		echo -n "set AE target (default is 1X): "
		read select
		case $select in
			1)
				aeTarget=1024
				accept=1
				;;
			2)
				aeTarget=2048
				accept=1
				;;
			3)
				aeTarget=3072
				accept=1
				;;
			4)
				aeTarget=4096
				accept=1
				;;
			*)
				accept=0
				echo "Please input valid number, without any other keypress(backspace or delete)!"
				;;
		esac
	done
	return 0
}

#_MAIN_
check_cmd LSC
lsc_init_setting
init_sensor
sleep 1
lsc_init_vinvout
echo

setAntiFlickerMode
echo;echo
setAETargetValue
echo;echo

echo "=============================================="
echo "Step 4: Detect lens shading."
echo "=============================================="
echo -n "Please aim the lens at the light box with moderate brightness, then press enter key to continue..."
read keypress

echo
echo "# cali_lens_shading -d -f ${CALIBRATION_FILE_NAME}"
cali_lens_shading -d -f ${CALIBRATION_FILE_NAME}

check_feedback

echo; echo "lens shading detection finished, result saved in ${CALIBRATION_FILE_NAME}"

echo;echo;echo;
echo "=============================================="
echo "Step 5: Verify lens shading calibration effect."
echo "=============================================="
echo -n "Now verify the calibration result. Press enter key to continue..."
read keypress

echo
if [ $bheight -ge "1080" ]; then
	echo "# test_encode -i"$width"x"$height" -f 0 -V480p --hdmi --enc-mode 0 -X --bsize 1080p --bmaxsize 1080p"
	$test_encode_cmd -i"$width"x"$height" -f 0 -V480p --hdmi --enc-mode 0 -X --bsize 1080p --bmaxsize 1080p
else
	echo "# test_encode -i "$width"x"$height" --enc-mode 0  -X --bsize "$width"x"$height" --bmaxsize "$width"x"$height""
	$test_encode_cmd -i"$width"x"$height" --enc-mode 0 -X --bsize "$width"x"$height" --bmaxsize "$width"x"$height"
fi

echo "# cali_lens_shading -r"
$cali_lens_shading_cmd -r

echo; echo -n "Now you can see the lens shading. Press enter key to correct lens shading..."
read keypress
echo "# cali_lens_shading -c -f ${CALIBRATION_FILE_NAME}"
$cali_lens_shading_cmd -c -f ${CALIBRATION_FILE_NAME}

echo; echo "Check the screen if shading in the corners disappears"
read keypress
echo; echo "Thanks for using Ambarella calibration tool. "
echo "For any questions please send mail to ipcam-sdk-support@ambarella.com"

