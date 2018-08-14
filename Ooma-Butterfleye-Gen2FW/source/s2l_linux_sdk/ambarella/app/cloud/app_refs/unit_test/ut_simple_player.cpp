/**
 * ut_simple_player.cpp
 *
 * History:
 *    2015/01/10 - [Zhi He] create file
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


#include "stdlib.h"
#include "stdio.h"

#include "simple_player_if.h"

using namespace mw_cg;

int main(int argc, char **argv)
{
    int ret = 0;
    mw_cg::ISimplePlayer *thiz = gfCreateSimplePlayer();

    if (!thiz) {
        printf("gfCreateSimplePlayer() fail\n");
        return (-1);
    }

    ret = thiz->Initialize();
    if (0 > ret) {
        printf("thiz->Initialize() fail, ret %d\n", ret);
        return (-2);
    }

    char *url = NULL;
    if (argc >= 2) {
        url = argv[1];
    }

    ret = thiz->Play(url);
    if (0 > ret) {
        printf("thiz->Play() fail, ret %d\n", ret);
        return (-3);
    }

    printf("press 'q' to quit\n");
    char input_c = 0;
    while (1) {
        sscanf("%c", &input_c);
        if ('q' == input_c) {
            break;
        }
    }

    thiz->Destroy();
    thiz = NULL;

    return 0;
}

