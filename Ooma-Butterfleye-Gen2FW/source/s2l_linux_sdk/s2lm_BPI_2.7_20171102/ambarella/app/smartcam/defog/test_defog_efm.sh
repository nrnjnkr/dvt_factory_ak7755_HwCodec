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

init.sh --ov4689
sleep 2s

test_encode -i0 -f 15 -X --bsize 1080p --bmaxsize 1080p -Y --bsize 240x136 --bmaxsize 240x136 -Z 0 --dframe 5 --bsize 1080p --bmaxsize 1080p -A --smaxsize 1080p
sleep 1s

test_image -i0 &
sleep 1s

test_defog_file2enc -f /home/zhuqi/1080p.nv12 -z 1920x1080 -F /home/zhuqi/240_136.nv12 -Z 240x136 -n 1&
#Parameters: T0: -t 10~50, hps : -p 1~7
sleep 1s

rtsp_server &
#test_stream -t
sleep 1s

test_encode -A -h 1080p -b 4 -e


