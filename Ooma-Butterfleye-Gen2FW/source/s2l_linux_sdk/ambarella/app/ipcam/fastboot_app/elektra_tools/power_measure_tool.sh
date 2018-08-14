#!/bin/bash
# power_measure_tool.sh
#
# History:
#       2015/07/17 - [niu zhifeng] created file
#
# Copyright (c) 2015 Ambarella, Inc.
#
# This file and its contents ("Software") are protected by intellectual
# property rights including, without limitation, U.S. and/or foreign
# copyrights. This Software is also the confidential and proprietary
# information of Ambarella, Inc. and its licensors. You may not use, reproduce,
# disclose, distribute, modify, or otherwise prepare derivative works of this
# Software or any portion thereof except pursuant to a signed license agreement
# or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
# In the absence of such an agreement, you agree to promptly notify and return
# this Software to Ambarella, Inc.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
# MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
#
LOOP_LIMIT=3;
VIN_720P_FLAG='no';
clock_prefix="";

cortex_top="";
drm_top="";
drm_min=360;
core_top="";
idsp_top="";

baseline="";
step="";

ask_continue()
{
	echo ""
	MSG="Do you want to continue to step to <$1>? [y/n]"
	read -p "$MSG" ifcontinue
	echo ${ifcontinue}"y"
	if [ ${ifcontinue}"y" == "yy" ]; then
		return 0
	else
		return 1
	fi
}

ask_continue_with_mode()
{
	echo ""
	MSG="Enter 'r' to step into <$1>, 's' to step into <$2>: "
	read -p "$MSG" mode
	echo ${mode}"y"
	if [ ${mode}"y" = "ry" ]; then
		return 0
	elif [ ${mode}"y" = "sy" ]; then
		return 1
	else
		return 2
	fi
}

handle_multi_times_invalid_input()
{
    local time=$1;
    if [ ${time} -le 0 ];then
        echo "Your chance run out then the $0 will exit";
        exit 1;
    fi
}

canceled_or_failed()
{

	echo "#######################################"
	echo ""
	if [ -n "$1" ]; then
		echo "failed with message: $1"
		echo "exit now .."
		exit 1
	else
		echo "interrupt .."
		echo "exit now .."
		exit 0
	fi
}

usage_step()
{
        echo " "
        echo " "
        echo "########################################################################################################"
        echo "elektra board can work in streaming mode and recording mode."
        echo "this tool can drive the board to run its function set step by step"
        echo "using this tool to: "
        echo "1. understand the detail stages when elektra board works"
        echo "2. track every step and do some analysis, such as power consumption analysis, performance analyisis etc."
        local option;
        local time=$LOOP_LIMIT;
        while [ ${time} -gt 0 ]
        do
            echo;
            read -p "will you set clocks,available option y/n,default YES(enter)" option;
            if [ "${option}" = "" -o "${option}" = "y" -o "${option}" = "Y" ];then
                return 0;
            elif [ "${option}" = "n" -o "${option}" = "N" ];then
                return 1;
            else
                time=$((time-1));
                echo "ERROR! option:${option} not support present! the rest of chance ${time}";
            fi
        done
        handle_multi_times_invalid_input $time;
}

init_environment()
{
    echo "####### initialize environment ######"
    echo ""
    read -p "please input the sensor type ov4689/imx322, default imx322 (just ENTER)" sensor_type

    if [ "${sensor_type}" = "" ];then
        sensor_type="imx322";
    fi

    echo "$sensor_type"y
    init.sh "--$sensor_type"

    if [ $? -ne 0 ]; then
        canceled_or_failed "init.sh failed"
    fi

    ask_continue "start test_image"
    return $?
}

base_step_start_sensor()
{
    echo "########### start test_image ############"
    echo "start test_image .."

    test_image -i0  > /dev/null &

    if [ $? -ne 0 ]; then
        canceled_or_failed "test_image falied"
    fi

    echo "start test_image done .."
    ask_continue "start dsp"
    return $?
}

select_DSP_mode()
{
    local mode;
    local time=${LOOP_LIMIT};
    while [ ${time} -gt 0 ]
    do
        echo;
        echo "########## select DSP work mode,default:0  #########";

        read  -p "available options:'0' , '4' " mode;
        if [ "${mode}" = "0" -o "${mode}" = "" ];then
            return 0;
        elif [ "${mode}" = "4" ];then
            return 4;
        else
            time=$((time-1));
            echo "ERROR! mode:${mode} not support present! the rest of chance ${time}";
        fi
    done
    handle_multi_times_invalid_input $time;
}

select_resolution()
{
    local object="$1";
    local resolution;
    local time=${LOOP_LIMIT};
    if [ "${object}" != "stream_A" -a "${object}" != "VIN" ];then
        echo "ERROR!config object:${object} resolution illegal!" ;
        return -1;
    fi

    if [ "${object}" = "stream_A" -a "${VIN_720P_FLAG}" = "yes" ];then
        return 7;
    fi

    while [ ${time} -gt 0 ]
    do
        echo;
        echo "############ select $object resolution,default:1080p ############"

        read  -p "available options: '1080p','720p' " resolution;
        if [ "${resolution}" = "1080p" -o "${resolution}" = '1080' -o "${resolution}" = "" ];then
            return 1;
        elif [ "${resolution}" = "720p" -o "${resolution}" = '720' ];then
            return 7;
        else
            time=$((time-1));
            echo "ERROR! resolution:${resolution} not support present! the rest of chance:${time}";
        fi
    done

    handle_multi_times_invalid_input $time;
}

select_streamA_bitrate()
{
    local bitrate;
    local time=${LOOP_LIMIT};
    while [ ${time} -gt 0 ]
    do
        echo;
        echo "############ select stream_A bitrate,default:4M #############"

        read  -p "available options:4M,1M " bitrate;
        if [ "${bitrate}" = '4M' -o "${bitrate}" = '4m' -o "${bitrate}" = '4' -o "${bitrate}" = "" ];then
            return 4;
        elif [ "${bitrate}" = '1M' -o "${bitrate}" = '1m' -o "${bitrate}" = '1' ];then
            return 1;
        else
            time=$((time-1));
            echo "ERROR! stream_A bitrate:${bitrate} not support present! the rest of chance:${time}";
        fi
    done

    handle_multi_times_invalid_input $time;
}

select_VIN()
{

    local frame;
    local resolution;
    local mode;
    local time=${LOOP_LIMIT};
    while [ ${time} -gt 0 ]
    do
        echo;
        echo "############ select VIN resolution, 0 meaning use default resolution and fps ############";
#        read -n5 -p "available options: '1080p','720p'" resolution;
        read  -p "available options: '1080p','720p' " resolution;
        if [ "${resolution}" = '0' -o "${resolution}" = "" ];then
            select_DSP_mode;
            mode=$?;
            if [ ${mode} -eq 0 ];then
                echo CMD:"test_encode -i0 -V480i -C --smaxsize 720p";
                test_encode -i0 -V480i -C --smaxsize 720p;
            else
                echo CMD:"test_encode -i0 -V480i -C --smaxsize 720p --enc-mode 4  --lens-warp 1 && test_ldc -F 135 -R 960 -m 1 --zh 8/10 --zv 8/10";
                test_encode -i0 -V480i -C --smaxsize 720p --enc-mode 4  --lens-warp 1 && test_ldc -F 135 -R 960 -m 1 --zh 8/10 --zv 8/10;
            fi

            if [ $? -eq 0 ];then
                echo "IAV enter preview  state Succeed!";
                test_encode --show-enc-mode;
                return 0;
            else
                echo "ERROR! set IAV preview failed";
                echo "DSP/IAV status:";
                test_encode  --show-sys;
                exit -1;
            fi

        elif [ "${resolution}" = "1080p" -o "${resolution}" = "1080" -o "${resolution}" = "720p" -o "${resolution}" = "720" ];then
            {
                local time=${LOOP_LIMIT};
                while [ ${time} -gt 0 ]
                do
                    echo
                    echo "########### select VIN frame per second,default:30 #############";

                    read -p "available options: 'opsitive integer' " frame;

#convert frame null to default value;
                    if [ "${frame}" = "" ];then
                        frame=0;
                    fi

                    if [ "${resolution}" = "1080" -o "${resolution}" = "720" ];then
                        resolution=${resolution}p;
                    fi
#end convert

                    if [ ${frame} -ge 0 ];then
                        select_DSP_mode;
                        mode=$?;
                        if [ ${mode} -eq 0 ];then
                            if [ ${resolution} = "720p" ];then
                                VIN_720P_FLAG='yes';
                                echo CMD:"test_encode -i ${resolution} -f ${frame} -V480i -C --smaxsize 720p -X --bsize 720p";
                                test_encode -i ${resolution} -f ${frame} -V480i -C --smaxsize 720p -X --bsize 720p;
                            else
                                VIN_720P_FLAG='no';
                                echo CMD:"test_encode -i ${resolution} -f ${frame} -V480i -C --smaxsize 720p";
                                test_encode -i ${resolution} -f ${frame} -V480i -C --smaxsize 720p;
                            fi
                        else
                            if [ ${resolution} = "720p" ];then
                                VIN_720P_FLAG='yes';
                                echo CMD:"test_encode -i ${resolution} -f ${frame} -V480i -C --smaxsize 720p  -X --bsize 720p --enc-mode 4  --lens-warp 1 && test_ldc -F 135 -R 960 -m 1 --zh 8/10 --zv 8/10";
                                test_encode -i ${resolution} -f ${frame} -V480i -C --smaxsize 720p  -X --bsize 720p --enc-mode 4  --lens-warp 1 && test_ldc -F 135 -R 960 -m 1 --zh 8/10 --zv 8/10;
                            else
                                VIN_720P_FLAG='no';
                                echo CMD:"test_encode -i ${resolution} -f ${frame} -V480i -C --smaxsize 720p --enc-mode 4  --lens-warp 1 && test_ldc -F 135 -R 960 -m 1 --zh 8/10 --zv 8/10";
                                test_encode -i ${resolution} -f ${frame} -V480i -C --smaxsize 720p --enc-mode 4  --lens-warp 1 && test_ldc -F 135 -R 960 -m 1 --zh 8/10 --zv 8/10;
                            fi
                        fi

                        if [ $? -eq 0 ];then
                            echo "IAV enter preview state Succeed !";
                            test_encode --show-enc-mode;
                            return 0   ;
                        else
                            echo "ERROR! set IAV preview failed";
                            echo "DSP/IAV status:";
                            test_encode  --show-sys;
                            exit -1;
                        fi
                    else
                        time=$((time-1));
                        echo "ERROR! VIN frame: ${frame} not support present! the rest of chance:${time}";
                    fi
                done
                handle_multi_times_invalid_input $time;
            }
        else
            time=$((time-1));
            echo "ERROR! resolution:${resolution} not support present! the rest of chance:${time}";
        fi
    done
    handle_multi_times_invalid_input $time;
}

base_step_start_dsp()
{
    local ret;
    local resolution;
    local bitrate;

    echo "######### start dsp,DSP/IAV status:################"
    test_encode  --show-sys;

    ask_continue "enter preview";
    ret=$?;
    if [ ${ret} -eq 0 ];then
        select_VIN;
        ret=$?;
        if [ ${ret} -ne 0 ];then
            echo "select_VIN return failed!";
            exit -1;
        fi
    else
        canceled_or_failed;
    fi

    echo "DSP/IAV status:";
    test_encode  --show-sys;

    ask_continue "enter encoding";
    ret=$?;
    if [ ${ret} -eq 0 ];then

        select_resolution "stream_A";
        resolution=$?;
        if [ "${resolution}" = "1" ];then
            resolution=1080p;
        elif [ "${resolution}" = "7" ];then
            resolution=720p;
        else
            canceled_or_failed "set stream_A resolution";
        fi

        select_streamA_bitrate;
        bitrate=$?;
        if [ "${bitrate}" = "4" ];then
            bitrate=4000000;
        elif [ "${bitrate}" = "1" ];then
            bitrate=1000000;
        else
            canceled_or_failed "set stream_A bitrate";
        fi
    else
        canceled_or_failed;
    fi


        echo CMD:"test_encode -A -h ${resolution}  --bitrate ${bitrate}  -e";
        test_encode -A -h ${resolution}  --bitrate ${bitrate}  -e;

	if [ $? -ne 0 ]; then
		canceled_or_failed "test_encode failed"
	fi

    echo "DSP/IAV status:";
    test_encode  --show-sys;

	echo "start dsp done .."
        ask_continue_with_mode "recording mode" "streaming mode"
	return $?
}

load_wifi_drive()
{
	echo "########## load WiFi module #########"
	echo "start to load WiFi module .."

	modprobe bcmdhd
	if [ $? -ne 0 ]; then
		canceled_or_failed "modprobe bcmdhd failed"
	fi
	ifconfig wlan0 up

	echo "WiFi module load done .."
	ask_continue "connect to ap"

	return $?
}

connect_ap()
{
	echo "########### connect AP ###############"
	echo "start to connect ap .."
	read -p "Please input AP ID and password [APID <PWD>]: " APID PWD
	echo "APID {$APID}"
	echo "PWD {$PWD}"

	wifi_setup.sh sta nl80211 $APID $PWD

	if [ $? -ne 0 ]; then
		canceled_or_failed "connect ap failed"
	fi
	echo "ap connect done .."
	return $?
}

load_sdcard_drive()
{
	echo "########### load sdcard ###############"
	mkdir -p /tmp/sdcard

	echo "$1"

	if [ "$1"y = sy ]; then
		echo "start to load sdcard .."
		modprobe ehci_hcd
		echo host > /proc/ambarella/usbphy0
		modprobe scsi_mod
		modprobe usb_storage
		modprobe sd_mod
		modprobe fat
		modprobe vfat
		local param=`/bin/ls -l /dev|grep sda1`
		local timeout=20
		echo "wait sda1 ..."
		while [ -z "$param" ] && [ "$timeout" -gt 0 ];
		do
			sleep 0.2
			param=`/bin/ls -l /dev|grep sda1`
			let timeout-=1
		done
		echo "wait sda1 done. [$timeout, $param]"
		mount -t vfat /dev/sda1 /tmp/sdcard
		if [ $? -ne 0 ]; then
			canceled_or_failed "mount sdcard failed"
		fi
		echo "sdcard load done .."
	else
		echo "sdcard skipped."
	fi
	return $?
}

load_audio_drive()
{
	echo "start load audio module ..."
	echo 6 > /proc/ambarella/dma
	echo 7 > /proc/ambarella/dma

	modprobe i2c-dev
	modprobe snd-soc-core pmdown_time=300
	modprobe snd-soc-ambarella
	modprobe snd-soc-ambarella-i2s
	modprobe snd-soc-wm8974-amb
	modprobe snd-soc-ambdummy
	modprobe snd-soc-amba-board
	echo "load audio module done .."
}
step_recording()
{
	echo "########### recording mode #############"
        local uploading=0
	read -p "Enter 's' to record into sdcard, 'm' to record into memory: " mode

	load_sdcard_drive $mode

	read -p "Enter 'u ip_address' to enable uploading and specify the cloud server address, 'n' to disable the uploading: " mode address
	echo "$mode"y
	echo "$address"y
	if [ "$mode"y = uy ]; then
		uploading=1
		############# load wifi drive #################
		load_wifi_drive
		############## connect to ap ##################
		if [ $? -eq 0 ]; then
			connect_ap
		fi
	fi

	read -p "Enter 'a' to enable audio,'n' to disable audio: " audio

	if [ ${audio}"y" == "ay" ]; then
		load_audio_drive
		audio="-"${audio}
	else
		echo "skip to load audio module"
		audio=""
	fi
	echo "audio flag = "${audio}
	ask_continue "start recording"

	if [ "$?" -eq 0 ]; then
		echo "########### recoring #############"
		if [ $uploading -eq 1 ]; then
			echo "start recording and uploading ..."
			recording_test -u ${address}":6024" $audio
		else
			echo "start recording ..."
			recording_test $audio
		fi
	fi

        echo "steps are done, recording mode works now"
}

step_streaming()
{
        echo "########## streaming mode ##############"
        ask_continue "load WiFi module"

	##############load WiFi drive#################

	if [ "$?" -eq 0 ];then
		load_wifi_drive
	else
		canceled_or_failed
	fi

	##############connect to ap####################

	if [ "$?" -eq 0 ]; then
		connect_ap
	else
		canceled_or_failed
	fi

	###############load audio drive ###############
	read -p "Enter 'a' to enable audio,'n' to disable audio: " audio

        if [ ${audio}"y" == "ay" ]; then
                load_audio_drive
                audio="-"${audio}
        else
                echo "skip to load audio module"
                audio=""
        fi
	echo "audio flag = "${audio}
	###############start rtsp server###############

       ask_continue "start rtsp server"

	if [ "$?" -eq 0 ]; then
		streaming_test $audio
	else
		canceled_or_failed
	fi
}

extract_baseline_step()
{

        local address=$1;
        local object=$2;
        local clock_table1=`cat /proc/ambarella/clock`;
        local general_reg=`amba_debug  -r ${address}|sed '1d' |awk '{print $2}'`;
        general_reg=`echo ${general_reg}`;
        local general_reg_suffix=${general_reg:4:5};
        local save=${general_reg:0:9};
        general_reg=${general_reg:0:4};

        local general_reg_val1=`echo "${clock_table1}"|grep "${object}" | awk '{print $2}'`;
        general_reg_val1=$((general_reg_val1/1000000));

        echo "general_reg:${general_reg}";
        echo "general_reg_val1:${general_reg_val1}";
        local general_command_set="amba_debug -w ${address} -d `printf "%#04x" $((general_reg-1))`${general_reg_suffix}1";
        local general_command_res="amba_debug -w ${address}-d `printf "%#04x" $((general_reg-1))`${general_reg_suffix}0";

        local resume1="amba_debug -w ${address} -d ${save}1";
        local resume0="amba_debug -w ${address} -d ${save}0";
        ${general_command_set} && ${general_command_res};
        if [ $? -eq 0 ];then
            echo "set $1 ok"
        else
            echo "set $1 fail";
            return 1;
        fi

        local clock_table2=`cat /proc/ambarella/clock`;
        local general_reg_val2=`echo "${clock_table2}"|grep ${object} | awk '{print $2}'`;
        general_reg_val2=$((general_reg_val2/1000000));

        step=$((general_reg_val1-general_reg_val2));
        baseline=$((general_reg_val1-(step \* general_reg)));

        echo "$1 step:${step}";
        echo "$1 baseline:${baseline}";

        ${resume1} && ${resume0};
        if [ $? -eq 0 ];then
            echo "$1 resume ok";
        else
            echo "$1 resume fail";
            return 1;
        fi
        return 0;

}

check_clock()
{
    if [ "$2" = "" ];then
        echo "ERROR! lack parameter!";
        return 1;
    fi

    local clock=$1;
    if [ ${#clock} -ne 2 ] && [ ${#clock} -ne 3 ] ;then
        echo "ERROR! please type 2-3 bit number!";
        return 1;
    fi

    local tmp;
    local tmp_num;
    local object=$2;
    local number1=${clock:0:1};
    local number2=${clock:1:1};
    local number3=${clock:2:1};

    for tmp_num in $number1 $number2 $number3
    do
        tmp=`printf "%d" "'${tmp_num}"`;
        if [ ${tmp} -ge 48 -a ${tmp} -le 57 ]; then
            :;
        else
            return 1;
        fi
    done

    case ${object} in
        ARM)
            extract_baseline_step "0xec170264" "gclk_cortex";
            ;;
        DSP)
            extract_baseline_step "0xec170000" "gclk_core"
            ;;
        iDSP)
            extract_baseline_step "0xec1700e4" "gclk_idsp"
            ;;
        *)
            echo "ERROR! no match object";
            return 1;
            ;;
    esac

    if [ $? -eq 0 ];then
        :;
    else
        return 1;
    fi

    if [ ${clock} -lt ${baseline} ];then
        return 1;
    fi
    tmp=$(((clock-baseline)/step));
    clock_prefix=`printf "%02x" $((tmp))`;

    echo "clock_prefix:${clock_prefix}";
    return 0;
}

re_decrease_dram()
{

    local time;
    local valid_decrease_num=$(((drm_top-drm_min-12)/12));
    local option;
    local intp=`amba_debug -r 0xec1700dc|awk '{print $2}'`;
    intp=`echo ${intp}`;
    local part_value=${intp:4:5};
    intp=${intp:0:4};
    intp=`printf "%#x" $((intp - 1))`;
    while [ ${valid_decrease_num} -gt 0 ]
    do
        local command_change="amba_debug  -w 0xec1700dc -d ${intp}${part_value}1";
        local command_resume="amba_debug  -w 0xec1700dc -d ${intp}${part_value}0";
        echo "#############  Do you want to decrease the DRAM clock again? #############";
        echo "the step of clock decrease is 12 MHZ";
        echo "the remain valid_decrease_num :${valid_decrease_num}";
        {
            time=${LOOP_LIMIT};
            while [ ${time} -gt 0 ]
            do
                read -p "available option:y/n default YES(ENTER)" option;
                if [ "${option}" = "" -o "${option}" = "y" -o "${option}" = "Y" ];then
                    echo "command: ${command_change} ${command_resume}";

                    ${command_change};

                    if [ $? -eq 0 ];then
                        ${command_resume};
                    else
                        return 1;
                    fi

                    if [ $? -ne 0 ];then
                        return 1;
                    fi
                    cat /proc/ambarella/clock;
                    valid_decrease_num=$((valid_decrease_num-1));
                    intp=`printf "%#x" $((intp-1))`;
                    break;
                elif [ "${option}" = "n" -o "${option}" = "N" ];then
                    return 0;
                else
                    time=$((time-1));
                    echo "ERROR! option:${option} not support present! the rest of chance:${time}";
                fi
            done
            if [ ${time} -le 0 ];then
                handle_multi_times_invalid_input ${time};
            fi
        }
        unset command_change command_resume;
    done
    echo " DRAM has reached the stable lowest clock! ";
    return 0;
}

do_set_clock()
{
    if [ "$1" = "" ];then
        echo "ERROR! lack parameter!";
        return 1;
    fi

    local object=$1;
    local time=$LOOP_LIMIT;
    local clock;
    local default_clock;
    local range;
    local multiple;
    local register;
    local address;
    local part_value;
    local command_set;
    local command_res;
    local default_register_value;

    case "${object}" in
        ARM)
            default_clock=${cortex_top};
            register=`amba_debug  -r 0xec170264`;
#            formula='24*(x*16+y+1)';
            address=0xec170264;
            range="96-${cortex_top}";
            multiple="24";
            ;;
        DRAM)
            default_clock=${drm_top};
            register=`amba_debug  -r 0xec1700dc`;
#            formula='12*(x*16+y+1)';
            address='0xec1700dc';
            range="${drm_min}-${drm_top}";
            multiple="12";
            ;;
        DSP)
            default_clock=${core_top};
            register=`amba_debug  -r 0xec170000`;
#            formula='6*(x*16+y+1)';
            address='0xec170000';
            range="96-${core_top}";
            multiple="6";
            ;;
        iDSP)
            default_clock=${idsp_top};
            register=`amba_debug  -r 0xec1700e4`;
#            formula='6*(x*16+y+1)';
            address='0xec1700e4';
            range="90-${idsp_top}";
            multiple="6";
            ;;
        *)
            echo "ERROR! no match object!";
            return 1;
    esac

            default_register_value=`echo ${register}|awk '{print $2}'`;
            default_register_value=`echo ${default_register_value}`;
            part_value=${default_register_value:4:5};
    echo "############### the $1 default clock is ${default_clock}MHZ ,controling register value:${register} ###############";
    if [ "${object}" != "DRAM" ];then
        while [ ${time} -gt 0 ]
        do
            echo;
            read -p "Please set a appropriate clock referencing the range: ${range},and the clock should be the multiple of ${multiple} just enter clock:" clock;
            check_clock ${clock} ${object};
            if [ $? -eq 0 ];then
                command_set="amba_debug  -w ${address} -d 0x${clock_prefix}${part_value}1";
                command_res="amba_debug -w ${address} -d 0x${clock_prefix}${part_value}0";
                break;
            else
                time=$((time-1));
                echo "clock:${clock} not support present! the rest of chance:${time}";
            fi
        done
        if [ ${time} -le 0 ];then
            handle_multi_times_invalid_input "${time}";
        fi
    else
        command_set="amba_debug  -w 0xec1700dc -d `printf "%#x" $((${default_register_value:0:4}-1))`${part_value}1";
        command_res="amba_debug  -w 0xec1700dc -d `printf "%#x" $((${default_register_value:0:4}-1))`${part_value}0";
    fi

    echo "command: ${command_set} ${command_res}";

    ${command_set};
    if [ $? -eq 0 ];then
        ${command_res};
    else
        echo "ERROR! first command fail";
        return 1;
    fi

    if [ $? -eq 0 ];then
        cat /proc/ambarella/clock;
        if [ "${object}" = "DRAM" ];then
            re_decrease_dram;
            if [ $? -ne 0 ];then
                return 1;
            fi
        fi
        return 0;
    else
        return 1;
    fi
}

set_clock()
{
    local option;
    local time=${LOOP_LIMIT};
    while [ $time -gt 0 ]
    do
        echo;
        echo "############### Do you want to set the $1 clock? #############";
        if [ "$1" = "DRAM" ];then
            echo "the step of clock decrease is 12 MHZ";
        fi
        read -p "available options: y/n,default option:YES(encter)" option;
        if [ "${option}" = "" -o "${option}" = "y" -o "${option}" = "Y" ];then
            case "$1" in
                'Cortex(ARM)')
                    do_set_clock ARM;;
                'DRAM')
                    do_set_clock DRAM;;
                'DSP')
                    do_set_clock DSP;;
                'iDSP')
                    do_set_clock iDSP;;
                *)
                    echo "ERROR!set_clock no such case!"
                    canceled_or_failed "set_clock case";;
            esac
            return $?;
        elif [ "${option}" = "n" -o "${option}" = "N" ];then
            echo "!!!!!!!! ignore the $1 clock !!!!!!!!!";
            return 0;
        else
            time=$((time-1));
            echo "ERROR! option:${option} not support present! the rest of chance ${time}";
        fi
    done
    handle_multi_times_invalid_input ${time};
}

start()
{
	    stty erase ^?

        ###########################usage##################################
        usage_step
        ###########################set clocks#############################
        if [ $? -eq 0 ];then
             local clock_table=`cat /proc/ambarella/clock`
             for var in "gclk_cortex" "gclk_ddr" "gclk_core" "gclk_idsp"
             do
                 local tmp_value=`echo "${clock_table}"  |grep -w  ${var}  | awk  '{print $2}'`
                 if [ -z "${tmp_value}" ];then
                     echo "ERROR ! no corresponding item:${var}";
                     exit 1;
                 else
                     case "${var}" in
                         "gclk_cortex")
                             cortex_top=${tmp_value}
                             cortex_top=$((cortex_top/1000000))
                             ;;
                         "gclk_ddr")
                             drm_top=${tmp_value}
                             drm_top=$((drm_top/1000000))
                             ;;
                         "gclk_core")
                             core_top=${tmp_value}
                             core_top=$((core_top/1000000))
                             ;;
                         "gclk_idsp")
                             idsp_top=${tmp_value}
                             idsp_top=$((idsp_top/1000000))
                             ;;
                     esac
                 fi
             done
            echo "${clock_table}"
            for var in "Cortex(ARM)" "DRAM " "DSP " "iDSP";
            do
                set_clock $var;
                if [ $? -ne 0 ];then
                    canceled_or_failed "${var}";
                fi
            done
        fi
        ask_continue "initialize environment"
        ##################initialize environment##########################

        if [ "$?" -eq 0 ];then
                init_environment
        else
                canceled_or_failed
        fi
        ######################start sensor################################

        if [ "$?" -eq 0 ]; then
                base_step_start_sensor
        else
                canceled_or_failed
        fi
        #######################start dsp##################################

        if [ "$?" -eq 0 ]; then
                base_step_start_dsp
        else
                canceled_or_failed
        fi
        ######################chose the work mode########################

        if [ "$?" -eq 0 ]; then
            step_recording
        elif [ "$?" -eq 1 ]; then
            step_streaming
        else
            canceled_or_failed
        fi
}


start
