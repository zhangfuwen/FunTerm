#!/usr/bin/env bash

if [[ -d thirdparty ]]; then
  rm -rf thirdparty
fi

mkdir thirdparty
cd thirdparty
git clone https://github.com/zhangfuwen/vte
mkdir build
mkdir install
cd vte
meson -Dprefix=$(pwd)/../install ../build
ninja -C ../build install
