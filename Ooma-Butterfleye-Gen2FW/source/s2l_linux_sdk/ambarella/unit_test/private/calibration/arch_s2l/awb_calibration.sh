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

source calib_utils.sh

kill_3a()
{
	HOLD=/tmp/hold.$$
	for proc in cali_awb test_image test_idsp
	do
		ps x| grep $proc | grep -v grep | awk '{print $1}' >> $HOLD
	done
	if [ -s $HOLD ]
	then
		while read LOOP
		do
			kill -9 $LOOP
		done < $HOLD
		rm /tmp/*.$$
	fi
}

# Get current AWB gain
get_current_awb_gain()
{
	kill_3a
	echo -n "(1) Adjust the environment light to LOW Color Temperature. Press Enter key to continue..."
	read keypress
	echo "# cali_awb -d -f $filename"
	$cali_awb_cmd -d -f $filename
	orig_low_r=`cat $filename | awk -F: 'NR==1{print $1}'`
	orig_low_g=`cat $filename | awk -F: 'NR==1{print $2}'`
	orig_low_b=`cat $filename | awk -F: 'NR==1{print $3}'`

	echo
	echo -n "(2) Adjust the environment light to HIGH Color Temperature. Press Enter key to continue..."
	read keypress
	echo "# cali_awb -d -f $filename"
	$cali_awb_cmd -d -f $filename
	orig_high_r=`cat $filename | awk -F: 'NR==2{print $1}'`
	orig_high_g=`cat $filename | awk -F: 'NR==2{print $2}'`
	orig_high_b=`cat $filename | awk -F: 'NR==2{print $3}'`
}

# Set the target AWB gain
set_target_awb_gain()
{
	case "$1" in
		d | D	)
			target_low_r=985
			target_low_g=1024
			target_low_b=3020
			target_high_r=1800
			target_high_g=1024
			target_high_b=1600
			;;
		*	)
			echo;
			echo "(1) Input the target Gain in LOW Color Temperature"
			echo -n "		Gain for red	:"
			read target_low_r
			echo -n "		Gain for green:"
			read target_low_g
			echo -n "		Gain for blue :"
			read target_low_b
			echo "(2) Input the target Gain in HIGH Color Temperature"
			echo -n "		Gain for red	:"
			read target_high_r
			echo -n "		Gain for green:"
			read target_high_g
			echo -n "		Gain for blue :"
			read target_high_b
			;;
	esac

	echo -e "$target_low_r:$target_low_g:$target_low_b" >> $filename
	echo -e "$target_high_r:$target_high_g:$target_high_b" >> $filename

	echo;
	echo "Target WB Gain"
	echo "	Low	Color Temp: R $target_low_r, G $target_low_g, B $target_low_b"
	echo "	High Color Temp: R $target_high_r, G $target_high_g, B $target_high_b"
	echo
}

# Implement AWB calibration
run_awb_calibration()
{
	kill_3a
	echo
	echo "		Original WB Gain"
	echo "				 Low Color Temp: R $orig_low_r, G $orig_low_g, B $orig_low_b"
	echo "				 High Color Temp: R $orig_high_r, G $orig_high_g, B $orig_high_b"
	echo "		Target WB Gain"
	echo "				 Low Color Temp: R $target_low_r, G $target_low_g, B $target_low_b"
	echo "				 High Color Temp: R $target_high_r, G $target_high_g, B $target_high_b"
	echo
	echo "# $cali_awb_cmd -l $filename &"
	$cali_awb_cmd -l $filename &
}

# Reset AWB gain
reset_awb_gain()
{
	kill_3a
	echo "# cali_awb -r"
	$cali_awb_cmd -r &
}

#_MAIN_

check_cmd AWB

filename=/ambarella/calibration/cali_awb.txt
if [ -e $filename ]
then
	rm -f $filename
fi

init_setting
init_sensor

echo;
echo "============================================"
echo "Start to do AWB calibration";
echo "============================================"
test_image -i 0 &
sleep 2
init_vinvout_mode0
echo

echo;echo;echo;
echo "============================================"
echo "Step 1: Get the current AWB gain."
echo "============================================"
echo -n "Please put the grey card in front of the camera and press Enter key to continue..."
read keypress
echo
get_current_awb_gain

echo;echo;echo;
echo "============================================"
echo "Step 2: Set the target White Balance gain."
echo "============================================"
echo -n "Press D (use the default values for example, 2800k & 6500k) or Enter key (input target WB gain)..."
read -n 1 select
set_target_awb_gain $select
sleep 1

echo;echo;echo;

echo "============================================"
echo "Step 3: Run awb_calibration."
echo "============================================"
echo -n "Press Enter key to continue..."
read keypress
run_awb_calibration
sleep 1


echo;echo;echo;
echo "============================================"
echo "Step 4: Verify AWB calibration effect."
echo "============================================"
echo -n "Press Enter key to reset AWB..."
read keypress
reset_awb_gain
sleep 1
echo
echo -n "Now you see the scene in orignal AWB. Press Enter key to run AWB Calibration..."
read keypress
run_awb_calibration
sleep 1

echo; echo "Thanks for using Ambarella calibration tool. "
echo "For any questions please send mail to ipcam-sdk-support@ambarella.com"


