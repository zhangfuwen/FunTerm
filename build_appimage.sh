#!/bin/bash

mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -G "Ninja"
ninja
DESTDIR=AppDir ninja install
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20220822-1/linuxdeploy-x86_64.AppImage
chmod +x ./linuxdeploy-x86_64.AppImage
./linuxdeploy-x86_64.AppImage --appdir ./AppDir --output appimage
rm ./linuxdeploy-x86_64.AppImage