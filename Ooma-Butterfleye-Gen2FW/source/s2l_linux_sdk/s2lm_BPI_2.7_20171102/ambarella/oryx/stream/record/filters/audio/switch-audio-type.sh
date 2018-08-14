#!/bin/sh
## /S2l/oryx/stream/record/filters/audio/switch-audio-type.sh
##
## History:
##   Apr 14, 2017 - [ypchang] created file
##
## Copyright (c) 2015 Ambarella, Inc.
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

if [ $# -ne 2 ]
then
    echo "Usage: $0 [48k|16k|8k] [u8|s16le|s16be|s24le|s24be|s32le|s32be]"
    exit 1
fi

SAMPLE_RATE=$(echo $1 | tr '[:upper:]' '[:lower:]')
SAMPLE_TYPE=$(echo $2 | tr '[:upper:]' '[:lower:]')

FILTER_CONF=/etc/oryx/stream/filter/filter-audio-source-${SAMPLE_RATE}.acs
PA_CONF=/etc/pulse/daemon.conf
PA_INIT=/etc/init.d/S80pulseaudio

sed -i -e "s/\(sample_format.*=\).*/\1 \"${SAMPLE_TYPE}\",/g" ${FILTER_CONF}

if sed -i -e "s/\(default-sample-format.*=\).*/\1 ${SAMPLE_TYPE}/g" ${PA_CONF};
then
    if [ -e ${PA_INIT} ]
    then
        ${PA_INIT} restart
    else
        systemctl restart pulseaudio
    fi
fi
