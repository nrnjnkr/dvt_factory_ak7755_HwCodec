#!/bin/bash

CURDIR=$(pwd -L)

cd s2l_linux_sdk/ambarella/boards/btfl 
source ../../build/env/armv7ahf-linaro-gcc.env
make clean
