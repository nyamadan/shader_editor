PUSHD libvpx
set PATH=C:\msys64\usr\bin;C:\msys64\mingw64\bin;%PATH%
bash ./configure --target=x86_64-win64-vs16 --disable-vp9 --disable-vp8-decoder --disable-docs --disable-tools --disable-examples --disable-unit-tests --disable-decode-perf-tests --disable-encode-perf-tests
make
POPD
COPY /Y libvpx\x64\Debug\vpxmdd.lib vpxmdd.lib
COPY /Y libvpx\x64\Release\vpxmd.lib vpxmd.lib