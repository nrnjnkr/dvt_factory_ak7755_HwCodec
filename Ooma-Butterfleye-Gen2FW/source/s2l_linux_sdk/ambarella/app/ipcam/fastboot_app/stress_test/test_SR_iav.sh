#!/bin/bash
#
# test_SR_iav.inc
#
# History:
#       2016/09/13 - [j Yi] created file
#
# Copyright (c) 2016 Ambarella, Inc.
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


init(){
    echo "init"
    if [ "$1" == "h264" ];then
        mode='-h'
    elif [ "$1" == "h265" ];then
        mode='-H'
    else
        echo "mode wrong, choose from h264 or h265"
        exit 1
    fi
    path="/tmp/stress-test/"
    mkdir -p ${path}
    test_image -i 0 &
    sleep 1
    test_stream -f ${path} &
    test_encode -i 1080p -J --btype enc --bsize 720p -A ${mode} 1080p
}

clear_arg(){
    ps | grep test_image | awk '{print$1}' | xargs kill &>/dev/null
    ps | grep test_stream | awk '{print$1}' | xargs kill &>/dev/null
}

once(){
    echo "once"
    test_encode -A -e
    sleep 1
    test_encode -A -s
    clear_arg
}

with_rm(){
    while [[ ${count} -ne ${TotalNum} ]]
    do
        echo "############################START AGAIN############################"
        echo "This is No.${count}"
        echo "with_rm"
        sleep 1
        count=`expr ${count} + 1`
        echo mem > /sys/power/state
        rm -Rf ${path}*
    done
}

without_rm(){
    while [[ ${count} -ne ${TotalNum} ]]
    do
        echo "############################START AGAIN############################"
        echo "This is No.${count}"
        echo "without_rm"
        sleep 1
        count=`expr ${count} + 1`
        echo mem > /sys/power/state
    done
}

loop(){
    echo "loop"
    if [ $1 -eq 0 ];then
        clear_arg
        exit 1
    fi
    `expr $1 + 0 &>/dev/null`
    if [ $? -ne 0 ];then
        TotalNum=-1
        rm_flag=1
    else
        if [ $1 -lt -1 ];then
            echo "please enter a number that bigger than -1."
            clear_arg
            exit 1
        fi
        TotalNum=$1
        if [ "$2" == "rm" ];then
            rm_flag=1
        else
            rm_flag=0
        fi
    fi
    count=0
    echo "total number is ${TotalNum}"
    test_encode -A -e
    if [ ${rm_flag} -eq 1 ];then
        with_rm
    else
        without_rm
    fi
    test_encode -A -s
    clear_arg
}

usage(){
    echo "Usage:"
    echo "test_iav.sh init [h264|h265] [once|loop] <T> <rm>"
    echo -e "\tinit [h264|h265]: initialize parameters"
    echo -e "\tonce: encode for one time"
    echo -e "\tloop <T> <rm>: encode for <T> times. if T is not set, then it will go into an endless loop."
    echo -e "\t\tif rm is not set, then recording files won't be deleted."
    echo -e "\tclr: clear initialized parameters"
    echo "Try examples:"
    echo -e "\ttest_iav.sh init h264 once"
    echo -e "\ttest_iav.sh init h264 loop 100 rm (force deleting recording files)"
    echo -e "\ttest_iav.sh init h264 loop"
    echo -e "\ttest_iav.sh init h264 (initialize only)"
    echo -e "\ttest_iav.sh clr (clear only)"
}


case $1 in
init)
    init $2
    if [ "$3" == "once" ];then
        once
    elif [ "$3" == "loop" ];then
        loop $4 $5
    fi
    ;;
once)
    once
    ;;
loop)
    loop $2 $3
    ;;
clr)
    clear_arg
    ;;
*)
    usage
    ;;
esac



