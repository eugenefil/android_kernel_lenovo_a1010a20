#!/bin/sh

mkdir -p out
# tip: if arm-eabi- cross compiler from defconfig is unwanted, override it
# with CROSS_COMPILE=<prefix> argument to this script (e.g. use empty
# CROSS_COMPILE= to not use cross compiler)
make ARCH=arm O=out -j$(nproc) "$@"
