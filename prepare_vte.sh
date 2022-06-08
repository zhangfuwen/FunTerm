#!/usr/bin/env bash

[[ ! -d thirdparty ]] && mkdir thirdparty
cd thirdparty
[[ ! -d vte ]] && git clone https://github.com/zhangfuwen/vte
mkdir build
mkdir install
cd vte
meson -Dprefix=$(pwd)/../install ../build
ninja -C ../build install
