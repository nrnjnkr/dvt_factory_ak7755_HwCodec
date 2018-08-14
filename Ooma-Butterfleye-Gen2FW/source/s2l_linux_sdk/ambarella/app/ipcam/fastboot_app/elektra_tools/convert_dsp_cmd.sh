#
# convert_dsp_cmd.sh
#
# History:
#       2015/12/15 - [CZ LIN] created file
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

[ $# -eq 0 ] && { echo "Usage: $0 dsp_cmd_log_file"; exit 1; }
dsp_log=$1
#determine the last occurrence of set_warp_control
tmpline=`grep -n SET_WARP_CONTROL "$1" | tail -1 | cut -d : -f 1 `
Beginline=$(($tmpline+1))
Endline=$(($Beginline+31))
i=0

# fetch the specified lines and value
results=`awk "NR==$Beginline,NR==$Endline" $dsp_log | awk '{print $NF}'`
for item in $results
do
   eval "config$i=$item"
   i=$((i+1))
done

# overwrite some of the entries
config0=SET_WARP_CONTROL
config16='(u32)warp_ctrl_h_table'
config17='(u32)warp_ctrl_v_table'
config31='(u32)warp_ctrl_me1_table'
printf "static set_warp_control_t set_warp_control_mode4 = {
	.cmd_code = ${config0},
	.zoom_x = ${config1},
	.zoom_y = ${config2},
	.x_center_offset = ${config3},
	.y_center_offset = ${config4},
	.actual_left_top_x = ${config5},
	.actual_left_top_y = ${config6},
	.actual_right_bot_x = ${config7},
	.actual_right_bot_y = ${config8},
	.dummy_window_x_left = ${config9},
	.dummy_window_y_top = ${config10},
	.dummy_window_width = ${config11},
	.dummy_window_height = ${config12},
	.cfa_output_width = ${config13},
	.cfa_output_height = ${config14},
	.warp_control = ${config15},
	.warp_horizontal_table_address = ${config16},
	.warp_vertical_table_address = ${config17},
	.grid_array_width = ${config18},
	.grid_array_height = ${config19},
	.horz_grid_spacing_exponent = ${config20},
	.vert_grid_spacing_exponent = ${config21},
	.vert_warp_enable = ${config22},
	.vert_warp_grid_array_width = ${config23},
	.vert_warp_grid_array_height = ${config24},
	.vert_warp_horz_grid_spacing_exponent = ${config25},
	.vert_warp_vert_grid_spacing_exponent = ${config26},
	.ME1_vwarp_grid_array_width = ${config27},
	.ME1_vwarp_grid_array_height = ${config28},
	.ME1_vwarp_horz_grid_spacing_exponent = ${config29},
	.ME1_vwarp_vert_grid_spacing_exponent = ${config30},
	.ME1_vwarp_table_address = ${config31}
};"
