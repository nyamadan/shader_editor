#! /usr/bin/bash
pushd libvpx
./configure --target=x86_64-win64-gcc --disable-vp9 --disable-vp8-decoder --disable-docs --disable-tools --disable-examples  --disable-unit-tests --disable-decode-perf-tests --disable-encode-perf-tests
make
popd
cp -fv ./libvpx/libvpx.a libvpx.a
