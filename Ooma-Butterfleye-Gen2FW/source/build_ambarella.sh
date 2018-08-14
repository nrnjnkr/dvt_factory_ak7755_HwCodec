#!/bin/bash

CURDIR=$(pwd -L)

cd s2l_linux_sdk/ambarella/boards/btfl
source ../../build/env/armv7ahf-linaro-gcc.env
make s2lm_btfl_evt_non_bpi_config
make defconfig_public_linux
make -j8
