#!/bin/bash

# Copyright: 2017 HawXeye, Inc.

# This script must be sourced
if [ "$0" = "$BASH_SOURCE" ] ; then
    echo "ERROR: You must source this script, not run it" 1>&2
    echo "Usage: . ./set_env.sh"
    exit
fi

# Compiler location
# TODO: Set with the location for your compiler
export S2LM_CXX="/usr/local/linaro-armv7ahf-2017.02-gcc6.3/bin/arm-linux-gnueabihf-g++"
if [ -z ${S2LM_CXX} ] ; then
    echo "ERROR: You must manually set S2LM_CXX in this script before running" 1>&2
    echo "See README.md for details"
    return
fi

# C++ flags
export S2LM_CXXFLAGS="-mcpu=cortex-a9 -mfpu=neon -O3 -std=c++11 -Werror -ftree-vectorize -ftree-vectorizer-verbose=0"

# ABI change to GCC 5 breaks std::string when libhawxeye.so is
# compiled with GCC 4.  This macro ensures backwards compatiblity
# with libhawxeye.so when building with GCC 5.
# Please see https://gcc.gnu.org/gcc-5/changes.html#libstdcxx
export S2LM_CXXFLAGS="$S2LM_CXXFLAGS -D_GLIBCXX_USE_CXX11_ABI=0"

