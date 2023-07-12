#!/bin/sh

mkdir -p out
make ARCH=arm O=out -j$(nproc) "$@"
