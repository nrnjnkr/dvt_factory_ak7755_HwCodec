#!/bin/bash

##
## Copyright (c) 2016 Ambarella, Inc.
##
## This file and its contents ( "Software" ) are protected by intellectual
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
##
## This file is generated automatically!
##
## DO NOT modify it!
##

IN_SAFEMODE=0
SHOW_DETAIL=0
ROLL_BACK=0

check_patch_version ()
{
    MAJOR_VER=`patch --version 2>&1|grep "patch"|awk -F'patch' '{print $2}'|awk '{split($0, array, ".")} END{print array[1]}'`
    if [[ $MAJOR_VER -lt 2 ]]; then 
        echo -e "[31;1mPatch major version is lower than 2.Please update to >=2.7 version.[0m" && exit -1
    fi
    MINOR_VER=`patch --version 2>&1|grep "patch"|awk -F'patch' '{print $2}'|awk '{split($0, array, ".")} END{print array[2]}'`
    if [[ $MINOR_VER -lt 7 ]]; then
        echo -e "[31;1mPatch version is lower than 2.7.Please update to >=2.7 version.[0m" && exit -1
    fi
}

print_error ()
{
    div_str="===================================================================="
    echo -e "[31;1mERROR:[0m$1"
    if [[ $SHOW_DETAIL -eq 0 ]]; then
        echo -e "[31;1mOutput:[0m"
        echo -e "$2"
    fi
    echo -e "[33;1m$div_str[0m"
    exit 1
}

print_done ()
{
    div_str="===================================================================="
    echo -e "[32;1mSuccess:[0m$@"
    echo -e "[33;1m$div_str[0m"
}

print_apply ()
{
    echo -e "[35;1mApply patch for[0m [36;1m$1[0m"
}

backup_git ()
{
    backup_dir="/tmp/patch_backup/$1"
    `mkdir -p $backup_dir`
    if [ -d "../$1" ]; then
        `cp -rf ../$1 $backup_dir/..`
        echo -e "[36;1mBackup git:<$1> to folder<$backup_dir>[0m"
    fi
}

set_safemode ()
{

    echo -e "[36;1mRun apply.sh in safemode.[0m"
    echo -e "[36;1m	Backup folders will copy to path under /tmp/patch_backup.[0m"
    echo -e "[31;1m	Caution: files under /tmp/patch_backup will be deleted.[0m"
    IN_SAFEMODE=1
    `mkdir -p /tmp/patch_backup`
    `rm -rf /tmp/patch_backup/*`
}

set_rollback ()
{
    if [ ! -d "/tmp/patch_backup" ]; then
        echo -e "[31;1mCannot find /tmp/patch_backup folder![0m"
        exit -1
    fi
    ROLL_BACK=1
}

rollback_git ()
{
    echo -e "[32;1mRoll back git $1.[0m"
    if [ ! -d "/tmp/patch_backup/$1" ]; then
        return
    fi
    `rm -rf ../$1`
    `cp -r /tmp/patch_backup/$1 ../$1`
}

print_help ()
{
    echo -e "[32;1mUsage: apply.sh [-d|--detail][0m"
    echo -e "[36;1m	-d, --detail	display detail logs.[0m"
    echo -e "[36;1m	-h, --help	display this help infomation.[0m"
    exit 0
}


for arg in "$@"; do
    if [[ $arg == "-h" || $arg == "--help" ]]; then
        print_help
    elif [[ $arg == "-d" ]]; then
        SHOW_DETAIL=1
    elif [[ $arg == "--safemode" ]]; then
        set_safemode
    elif [[ $arg == "--rollback" ]]; then
        set_rollback
    fi
done

check_patch_version


############################################################
## Patch git <ambarella/amboot>
print_apply "<ambarella/amboot>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/amboot"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/amboot"; fi
chmod +x ambarella_amboot.sh
script_log=`./ambarella_amboot.sh`
if [[ $SHOW_DETAIL -ne 0 ]]; then echo -e $script_log; fi

if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/amboot -p1 < ambarella.amboot.patch
    output=""
else
    output=`patch -d ../ambarella/amboot -p1 < ambarella.amboot.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/amboot"; else print_error "ambarella/amboot" "$output"; fi
fi

############################################################
## Patch git <ambarella/app/cloud>
print_apply "<ambarella/app/cloud>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/app/cloud"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/app/cloud"; fi
if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/app/cloud -p1 < ambarella.app.cloud.patch
    output=""
else
    output=`patch -d ../ambarella/app/cloud -p1 < ambarella.app.cloud.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/app/cloud"; else print_error "ambarella/app/cloud" "$output"; fi
fi

############################################################
## Patch git <ambarella/app/ipcam>
print_apply "<ambarella/app/ipcam>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/app/ipcam"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/app/ipcam"; fi
chmod +x ambarella_app_ipcam.sh
script_log=`./ambarella_app_ipcam.sh`
if [[ $SHOW_DETAIL -ne 0 ]]; then echo -e $script_log; fi

if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/app/ipcam -p1 < ambarella.app.ipcam.patch
    output=""
else
    output=`patch -d ../ambarella/app/ipcam -p1 < ambarella.app.ipcam.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/app/ipcam"; else print_error "ambarella/app/ipcam" "$output"; fi
fi

############################################################
## Patch git <ambarella/app/smartcam>
print_apply "<ambarella/app/smartcam>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/app/smartcam"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/app/smartcam"; fi
chmod +x ambarella_app_smartcam.sh
script_log=`./ambarella_app_smartcam.sh`
if [[ $SHOW_DETAIL -ne 0 ]]; then echo -e $script_log; fi

if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/app/smartcam -p1 < ambarella.app.smartcam.patch
    output=""
else
    output=`patch -d ../ambarella/app/smartcam -p1 < ambarella.app.smartcam.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/app/smartcam"; else print_error "ambarella/app/smartcam" "$output"; fi
fi

############################################################
## Patch git <ambarella/app/utility>
print_apply "<ambarella/app/utility>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/app/utility"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/app/utility"; fi
chmod +x ambarella_app_utility.sh
script_log=`./ambarella_app_utility.sh`
if [[ $SHOW_DETAIL -ne 0 ]]; then echo -e $script_log; fi

if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/app/utility -p1 < ambarella.app.utility.patch
    output=""
else
    output=`patch -d ../ambarella/app/utility -p1 < ambarella.app.utility.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/app/utility"; else print_error "ambarella/app/utility" "$output"; fi
fi

############################################################
## Patch git <ambarella/boards/hawthorn>
print_apply "<ambarella/boards/hawthorn>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/boards/hawthorn"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/boards/hawthorn"; fi
chmod +x ambarella_boards_hawthorn.sh
script_log=`./ambarella_boards_hawthorn.sh`
if [[ $SHOW_DETAIL -ne 0 ]]; then echo -e $script_log; fi

if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/boards/hawthorn -p1 < ambarella.boards.hawthorn.patch
    output=""
else
    output=`patch -d ../ambarella/boards/hawthorn -p1 < ambarella.boards.hawthorn.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/boards/hawthorn"; else print_error "ambarella/boards/hawthorn" "$output"; fi
fi

############################################################
## Patch git <ambarella/boards/s2lm_ironman>
print_apply "<ambarella/boards/s2lm_ironman>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/boards/s2lm_ironman"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/boards/s2lm_ironman"; fi
if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/boards/s2lm_ironman -p1 < ambarella.boards.s2lm_ironman.patch
    output=""
else
    output=`patch -d ../ambarella/boards/s2lm_ironman -p1 < ambarella.boards.s2lm_ironman.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/boards/s2lm_ironman"; else print_error "ambarella/boards/s2lm_ironman" "$output"; fi
fi

############################################################
## Patch git <ambarella/boards/s2lm_kiwi>
print_apply "<ambarella/boards/s2lm_kiwi>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/boards/s2lm_kiwi"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/boards/s2lm_kiwi"; fi
chmod +x ambarella_boards_s2lm_kiwi.sh
script_log=`./ambarella_boards_s2lm_kiwi.sh`
if [[ $SHOW_DETAIL -ne 0 ]]; then echo -e $script_log; fi

if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/boards/s2lm_kiwi -p1 < ambarella.boards.s2lm_kiwi.patch
    output=""
else
    output=`patch -d ../ambarella/boards/s2lm_kiwi -p1 < ambarella.boards.s2lm_kiwi.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/boards/s2lm_kiwi"; else print_error "ambarella/boards/s2lm_kiwi" "$output"; fi
fi

############################################################
## Patch git <ambarella/build>
print_apply "<ambarella/build>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/build"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/build"; fi
chmod +x ambarella_build.sh
script_log=`./ambarella_build.sh`
if [[ $SHOW_DETAIL -ne 0 ]]; then echo -e $script_log; fi

if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/build -p1 < ambarella.build.patch
    output=""
else
    output=`patch -d ../ambarella/build -p1 < ambarella.build.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/build"; else print_error "ambarella/build" "$output"; fi
fi

############################################################
## Patch git <ambarella/include>
print_apply "<ambarella/include>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/include"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/include"; fi
if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/include -p1 < ambarella.include.patch
    output=""
else
    output=`patch -d ../ambarella/include -p1 < ambarella.include.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/include"; else print_error "ambarella/include" "$output"; fi
fi

############################################################
## Patch git <ambarella/kernel/external>
print_apply "<ambarella/kernel/external>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/kernel/external"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/kernel/external"; fi
if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/kernel/external -p1 < ambarella.kernel.external.patch
    output=""
else
    output=`patch -d ../ambarella/kernel/external -p1 < ambarella.kernel.external.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/kernel/external"; else print_error "ambarella/kernel/external" "$output"; fi
fi

############################################################
## Patch git <ambarella/kernel/linux>
print_apply "<ambarella/kernel/linux>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/kernel/linux-3.10"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/kernel/linux-3.10"; fi
chmod +x ambarella_kernel_linux.sh
script_log=`./ambarella_kernel_linux.sh`
if [[ $SHOW_DETAIL -ne 0 ]]; then echo -e $script_log; fi

if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/kernel/linux -p1 < ambarella.kernel.linux.patch
    output=""
else
    output=`patch -d ../ambarella/kernel/linux -p1 < ambarella.kernel.linux.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/kernel/linux"; else print_error "ambarella/kernel/linux" "$output"; fi
fi

############################################################
## Patch git <ambarella/kernel/private>
print_apply "<ambarella/kernel/private>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/kernel/private"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/kernel/private"; fi
chmod +x ambarella_kernel_private.sh
script_log=`./ambarella_kernel_private.sh`
if [[ $SHOW_DETAIL -ne 0 ]]; then echo -e $script_log; fi

if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/kernel/private -p1 < ambarella.kernel.private.patch
    output=""
else
    output=`patch -d ../ambarella/kernel/private -p1 < ambarella.kernel.private.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/kernel/private"; else print_error "ambarella/kernel/private" "$output"; fi
fi

############################################################
## Patch git <ambarella/license>
print_apply "<ambarella/license>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/license"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/license"; fi
chmod +x ambarella_license.sh
script_log=`./ambarella_license.sh`
if [[ $SHOW_DETAIL -ne 0 ]]; then echo -e $script_log; fi

if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/license -p1 < ambarella.license.patch
    output=""
else
    output=`patch -d ../ambarella/license -p1 < ambarella.license.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/license"; else print_error "ambarella/license" "$output"; fi
fi

############################################################
## Patch git <ambarella/oryx>
print_apply "<ambarella/oryx>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/oryx"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/oryx"; fi
chmod +x ambarella_oryx.sh
script_log=`./ambarella_oryx.sh`
if [[ $SHOW_DETAIL -ne 0 ]]; then echo -e $script_log; fi

if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/oryx -p1 < ambarella.oryx.patch
    output=""
else
    output=`patch -d ../ambarella/oryx -p1 < ambarella.oryx.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/oryx"; else print_error "ambarella/oryx" "$output"; fi
fi

############################################################
## Patch git <ambarella/packages/bsreader>
print_apply "<ambarella/packages/bsreader>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/packages/bsreader"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/packages/bsreader"; fi
if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/packages/bsreader -p1 < ambarella.packages.bsreader.patch
    output=""
else
    output=`patch -d ../ambarella/packages/bsreader -p1 < ambarella.packages.bsreader.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/packages/bsreader"; else print_error "ambarella/packages/bsreader" "$output"; fi
fi

############################################################
## Patch git <ambarella/packages/data_transfer>
print_apply "<ambarella/packages/data_transfer>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/packages/data_transfer"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/packages/data_transfer"; fi
if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/packages/data_transfer -p1 < ambarella.packages.data_transfer.patch
    output=""
else
    output=`patch -d ../ambarella/packages/data_transfer -p1 < ambarella.packages.data_transfer.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/packages/data_transfer"; else print_error "ambarella/packages/data_transfer" "$output"; fi
fi
## Copy files for git <ambarella/packages/img_algo>
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/packages/img_algo"; else
echo -e "[36;1mCopy files for git <ambarella/packages/img_algo>[0m"
mkdir -p ../ambarella/prebuild/imgproc
cp -rf libimg_algo_s2l.a ../ambarella/prebuild/imgproc/img_lib

fi

############################################################
## Patch git <ambarella/packages/img_mw>
print_apply "<ambarella/packages/img_mw>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/packages/img_mw"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/packages/img_mw"; fi
if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/packages/img_mw -p1 < ambarella.packages.img_mw.patch
    output=""
else
    output=`patch -d ../ambarella/packages/img_mw -p1 < ambarella.packages.img_mw.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/packages/img_mw"; else print_error "ambarella/packages/img_mw" "$output"; fi
fi

############################################################
## Patch git <ambarella/packages/low_bitrate>
print_apply "<ambarella/packages/low_bitrate>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/packages/low_bitrate"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/packages/low_bitrate"; fi
if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/packages/low_bitrate -p1 < ambarella.packages.low_bitrate.patch
    output=""
else
    output=`patch -d ../ambarella/packages/low_bitrate -p1 < ambarella.packages.low_bitrate.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/packages/low_bitrate"; else print_error "ambarella/packages/low_bitrate" "$output"; fi
fi

############################################################
## Patch git <ambarella/packages/main_preproc>
print_apply "<ambarella/packages/main_preproc>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/packages/main_preproc"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/packages/main_preproc"; fi
if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/packages/main_preproc -p1 < ambarella.packages.main_preproc.patch
    output=""
else
    output=`patch -d ../ambarella/packages/main_preproc -p1 < ambarella.packages.main_preproc.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/packages/main_preproc"; else print_error "ambarella/packages/main_preproc" "$output"; fi
fi

############################################################
## Patch git <ambarella/packages/md_motbuf>
print_apply "<ambarella/packages/md_motbuf>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/packages/md_motbuf"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/packages/md_motbuf"; fi
if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/packages/md_motbuf -p1 < ambarella.packages.md_motbuf.patch
    output=""
else
    output=`patch -d ../ambarella/packages/md_motbuf -p1 < ambarella.packages.md_motbuf.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/packages/md_motbuf"; else print_error "ambarella/packages/md_motbuf" "$output"; fi
fi

############################################################
## Patch git <ambarella/packages/security>
print_apply "<ambarella/packages/security>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/packages/security"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/packages/security"; fi
chmod +x ambarella_packages_security.sh
script_log=`./ambarella_packages_security.sh`
if [[ $SHOW_DETAIL -ne 0 ]]; then echo -e $script_log; fi

if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/packages/security -p1 < ambarella.packages.security.patch
    output=""
else
    output=`patch -d ../ambarella/packages/security -p1 < ambarella.packages.security.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/packages/security"; else print_error "ambarella/packages/security" "$output"; fi
fi

############################################################
## Patch git <ambarella/packages/textinsert>
print_apply "<ambarella/packages/textinsert>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/packages/textinsert"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/packages/textinsert"; fi
if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/packages/textinsert -p1 < ambarella.packages.textinsert.patch
    output=""
else
    output=`patch -d ../ambarella/packages/textinsert -p1 < ambarella.packages.textinsert.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/packages/textinsert"; else print_error "ambarella/packages/textinsert" "$output"; fi
fi

############################################################
## Patch git <ambarella/packages/utils>
print_apply "<ambarella/packages/utils>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/packages/utils"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/packages/utils"; fi
if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/packages/utils -p1 < ambarella.packages.utils.patch
    output=""
else
    output=`patch -d ../ambarella/packages/utils -p1 < ambarella.packages.utils.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/packages/utils"; else print_error "ambarella/packages/utils" "$output"; fi
fi

############################################################
## Patch git <ambarella/prebuild/ambarella>
print_apply "<ambarella/prebuild/ambarella>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/prebuild/ambarella"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/prebuild/ambarella"; fi
mkdir -p ../ambarella/prebuild/ambarella
chmod +x ambarella_prebuild_ambarella.sh
script_log=`./ambarella_prebuild_ambarella.sh`
if [[ $SHOW_DETAIL -ne 0 ]]; then echo -e $script_log; fi

if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/prebuild/ambarella -p1 < ambarella.prebuild.ambarella.patch
    output=""
else
    output=`patch -d ../ambarella/prebuild/ambarella -p1 < ambarella.prebuild.ambarella.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/prebuild/ambarella"; else print_error "ambarella/prebuild/ambarella" "$output"; fi
fi

############################################################
## Patch git <ambarella/prebuild/imgproc>
print_apply "<ambarella/prebuild/imgproc>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/prebuild/imgproc"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/prebuild/imgproc"; fi
chmod +x ambarella_prebuild_imgproc.sh
script_log=`./ambarella_prebuild_imgproc.sh`
if [[ $SHOW_DETAIL -ne 0 ]]; then echo -e $script_log; fi

if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/prebuild/imgproc -p1 < ambarella.prebuild.imgproc.patch
    output=""
else
    output=`patch -d ../ambarella/prebuild/imgproc -p1 < ambarella.prebuild.imgproc.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/prebuild/imgproc"; else print_error "ambarella/prebuild/imgproc" "$output"; fi
fi

############################################################
## Patch git <ambarella/prebuild/sys_data>
print_apply "<ambarella/prebuild/sys_data>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/prebuild/sys_data"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/prebuild/sys_data"; fi
chmod +x ambarella_prebuild_sys_data.sh
script_log=`./ambarella_prebuild_sys_data.sh`
if [[ $SHOW_DETAIL -ne 0 ]]; then echo -e $script_log; fi

if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/prebuild/sys_data -p1 < ambarella.prebuild.sys_data.patch
    output=""
else
    output=`patch -d ../ambarella/prebuild/sys_data -p1 < ambarella.prebuild.sys_data.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/prebuild/sys_data"; else print_error "ambarella/prebuild/sys_data" "$output"; fi
fi

############################################################
## Patch git <ambarella/prebuild/third-party>
print_apply "<ambarella/prebuild/third-party>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/prebuild/third-party"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/prebuild/third-party"; fi
chmod +x ambarella_prebuild_third-party.sh
script_log=`./ambarella_prebuild_third-party.sh`
if [[ $SHOW_DETAIL -ne 0 ]]; then echo -e $script_log; fi

if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/prebuild/third-party -p1 < ambarella.prebuild.third-party.patch
    output=""
else
    output=`patch -d ../ambarella/prebuild/third-party -p1 < ambarella.prebuild.third-party.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/prebuild/third-party"; else print_error "ambarella/prebuild/third-party" "$output"; fi
fi

############################################################
## Patch git <ambarella/rootfs>
print_apply "<ambarella/rootfs>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/rootfs"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/rootfs"; fi
chmod +x ambarella_rootfs.sh
script_log=`./ambarella_rootfs.sh`
if [[ $SHOW_DETAIL -ne 0 ]]; then echo -e $script_log; fi

if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/rootfs -p1 < ambarella.rootfs.patch
    output=""
else
    output=`patch -d ../ambarella/rootfs -p1 < ambarella.rootfs.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/rootfs"; else print_error "ambarella/rootfs" "$output"; fi
fi

############################################################
## Patch git <ambarella/unit_test/linux>
print_apply "<ambarella/unit_test/linux>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/unit_test/linux"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/unit_test/linux"; fi
if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/unit_test/linux -p1 < ambarella.unit_test.linux.patch
    output=""
else
    output=`patch -d ../ambarella/unit_test/linux -p1 < ambarella.unit_test.linux.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/unit_test/linux"; else print_error "ambarella/unit_test/linux" "$output"; fi
fi

############################################################
## Patch git <ambarella/unit_test/private>
print_apply "<ambarella/unit_test/private>"
if [[ $ROLL_BACK -eq 1 ]]; then rollback_git "ambarella/unit_test/private"; else
if [[ $IN_SAFEMODE -eq 1 ]]; then backup_git "ambarella/unit_test/private"; fi
chmod +x ambarella_unit_test_private.sh
script_log=`./ambarella_unit_test_private.sh`
if [[ $SHOW_DETAIL -ne 0 ]]; then echo -e $script_log; fi

if [[ $SHOW_DETAIL -ne 0 ]]; then
    patch -d ../ambarella/unit_test/private -p1 < ambarella.unit_test.private.patch
    output=""
else
    output=`patch -d ../ambarella/unit_test/private -p1 < ambarella.unit_test.private.patch`
fi
if [[ $? -eq 0 ]]; then print_done "ambarella/unit_test/private"; else print_error "ambarella/unit_test/private" "$output"; fi
fi

############################################################
cp -rf ambarella/boards/s2lm_elektra ../ambarella/boards/
cp -rf ambarella/prebuild/imgproc/img_data/arch_s2l/adj_params/* ../ambarella/prebuild/imgproc/img_data/arch_s2l/adj_params
cp ambarella/app/ipcam/fastboot_app/bpi_unit_test/bpi_test_adc.cpp ../ambarella/app/ipcam/fastboot_app/bpi_unit_test/
cp ambarella/app/ipcam/fastboot_app/network_manager/make.inc ../ambarella/app/ipcam/fastboot_app/network_manager/
rm -rf ../ambarella/app/ipcam/fastboot_app/elektra_boot 
rm -rf ../ambarella/app/ipcam/fastboot_app/darwin_injector
