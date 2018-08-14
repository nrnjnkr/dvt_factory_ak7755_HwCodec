#!/bin/sh
## History:
##
## 2016/12/30 - [Jingyang Qiu] Created file
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

#####$1: generate param files, $2 source S file,
#####$3 source C file,

if [ $1 ]; then
	TARGET_C_FILE=$1
else
	TARGET_C_FILE=
fi

if [ $2 ]; then
	SOURCE_S_FILE=$2
else
	SOURCE_S_FILE=
fi

if [ $3 ]; then
	SOURCE_C_FILE=$3
else
	SOURCE_C_FILE=
fi

tmp_aaa_file="${SOURCE_S_FILE}.tmp"

source_file_nodir=`echo ${TARGET_C_FILE} | sed 's/^.*\///g'`

###   generate tmp header file for _param_adv.c ###
generate_adv_header_file()
{
echo "" >> $tmp_aaa_file

## TOTAL_FILTER_NUM define in img_abs_filter.h

echo "#define SIZE_TOTAL_LISTS		(TOTAL_FILTER_NUM)" >> $tmp_aaa_file
echo "u32 size_lists[SIZE_TOTAL_LISTS] = {" >> $tmp_aaa_file

VAR_NAME_LIST=`grep ".size" ${SOURCE_S_FILE} | awk '{ print $3}'`
for var_name in $VAR_NAME_LIST
do
	echo "${var_name}," >>  $tmp_aaa_file
done

echo "};" >> $tmp_aaa_file
}

generate_lens_header_file()
{
echo "" >> $tmp_aaa_file

## TOTAL_FILTER_NUM define in img_abs_filter.h

echo "#define SIZE_TOTAL_LISTS		(TOTAL_FILTER_NUM)" >> $tmp_aaa_file
echo "const u32 size_lists[SIZE_TOTAL_LISTS] = {" >> $tmp_aaa_file

VAR_NAME_LIST=`grep ".size" ${SOURCE_S_FILE} | awk '{ print $3}'`
for var_name in $VAR_NAME_LIST
do
	echo "${var_name}," >>  $tmp_aaa_file
done

echo "};" >> $tmp_aaa_file
}

generate_file()
{
if [ `echo ${SOURCE_S_FILE} | grep "piris_param"` ] ; then
	generate_lens_header_file
	sed -e '/img_adv_struct_arch.h/r '$tmp_aaa_file'' ${SOURCE_C_FILE} > ${TARGET_C_FILE}
else
	generate_adv_header_file
	if [ `echo ${SOURCE_S_FILE} | grep "adj_param"` ] ; then
		sed -e '/img_abs_filter.h/r '$tmp_aaa_file'' ${SOURCE_C_FILE} > ${TARGET_C_FILE}
	elif [ `echo ${SOURCE_S_FILE} | grep "aeb_param"` ] ; then
		sed -e '/img_adv_struct_arch.h/r '$tmp_aaa_file'' ${SOURCE_C_FILE} > ${TARGET_C_FILE}

	fi
fi
rm $tmp_aaa_file -f
rm ${SOURCE_S_FILE} -f
rm ${SOURCE_S_FILE}.list -f
}

generate_file

