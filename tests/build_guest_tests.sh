#!/bin/sh
# Run inside the image-builder service; products are outside the installed tree.
set -eu
. /work/.env
export PATH="/cache/out/host/bin:/cache/out/host/sbin:$PATH"
export PKG_CONFIG_SYSROOT_DIR=/cache/out/host/x86_64-buildroot-linux-gnu/sysroot
export PKG_CONFIG_LIBDIR="$PKG_CONFIG_SYSROOT_DIR/usr/lib/pkgconfig:$PKG_CONFIG_SYSROOT_DIR/usr/share/pkgconfig"
cmake -S /work -B /cache/out/validation \
    -DCMAKE_TOOLCHAIN_FILE=/cache/out/host/share/buildroot/toolchainfile.cmake \
    -DCMAKE_BUILD_TYPE=MinSizeRel -DBUILD_SHARED_LIBS=OFF -DTD_BUILD_TESTS=ON \
    -DCORE_SOURCE_DIR=/opt/bitcoin -DUR_SOURCE_DIR=/opt/bc-ur \
    -DBIP39_WORDLIST=/opt/bip39-english.txt -DBIP39_WORDLIST_SHA256="$BIP39_WORDLIST_SHA256" \n    -DBIP39_CHINESE_WORDLIST=/opt/bip39-chinese.txt -DBIP39_CHINESE_WORDLIST_SHA256="$BIP39_CHINESE_WORDLIST_SHA256" \n    -DUNIFONT_HEX=/opt/unifont.hex -DUNIFONT_HEX_SHA256="$UNIFONT_HEX_SHA256"
cmake --build /cache/out/validation --parallel 2 --target isolation-tests
