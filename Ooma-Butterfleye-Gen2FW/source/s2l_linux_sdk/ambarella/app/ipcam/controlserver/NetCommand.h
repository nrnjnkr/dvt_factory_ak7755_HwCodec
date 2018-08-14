/**********************************************************************
 * NetCommand.h
 *
 * Histroy:
 *  2011年03月22日 - [Yupeng Chang] created file
 *
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef APP_IPCAM_CONTROLSERVER_NETCOMMAND_H
#define APP_IPCAM_CONTROLSERVER_NETCOMMAND_H
/* DHCP, STATIC - use UDP multicast
 * QUERY        - use UDP unicast
 * RTSP         - use UDP unicast
 * REBOOT       - use UDP unicast
 * GET_*        - use UDP unicast
 * SET_*        - use UDP unicast
 */
enum NetCmdType {/* Network Related */
                 NOACTION = 0,
                 DHCP     = 1,
                 STATIC   = 2,
                 QUERY    = 3,
                 RTSP     = 4,
                 REBOOT   = 5,
                 /* Video Related */
                 GET_ENCODE_SETTING = 6,
                 GET_IMAGE_SETTING  = 7,
                 GET_PRIVACY_MASK_SETTING = 8,
                 GET_VIN_VOUT_SETTING     = 9,
                 SET_ENCODE_SETTING       = 10,
                 SET_IMAGE_SETTING        = 11,
                 SET_PRIVACY_MASK_SETTING = 12,
                 SET_VIN_VOUT_SETTING     = 13,
                 NETBOOT   = 14};
#endif //APP_IPCAM_CONTROLSERVER_NETCOMMAND_H

