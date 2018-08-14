#
# copyIDSPcfg.sh
#
# History:
#       2016/01/17 - [CZ LIN] created file
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
# !/bin/sh

[ $# -ne 3 ] && { echo "Usage: $0 original_binary new_binary 720p/1080p"; exit 1; }
OrigSrc=$1
NewSrc=$2
section=$3
sizeIDsp=25704
tmpFile=half_mode4.bin

if [ ! -f $OrigSrc ]
then
    echo "file $OrigSrc not found"
    exit 1
fi

if [ ! -f $NewSrc ]
then
   echo "file $NewSrc not found"
   exit 1
fi

if [ "$section" = "720p" ]
then
   echo "copying $NewSrc to the 720p section of $OrigSrc ..."
   dd bs=$sizeIDsp skip=0 count=1 if=$OrigSrc of=$tmpFile
   cat $tmpFile $NewSrc > $OrigSrc
elif [ "$section" = "1080p" ]
then
   echo "copying $NewSrc to the 1080p section of $OrigSrc ..."
   dd bs=$sizeIDsp skip=1 count=1 if=$OrigSrc of=$tmpFile
   cat $NewSrc $tmpFile > $OrigSrc
else
   echo "not supported argument!"
   exit 1
fi

rm $tmpFile

