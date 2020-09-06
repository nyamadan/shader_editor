CALL "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

PUSHD libvpx
SET PATH=%MSYS2_PATH%\usr\bin;%MSYS2_PATH%\mingw64\bin;%PATH%
where bash
bash ./configure --target=x86_64-win64-vs16 --disable-vp9 --disable-vp8-decoder --disable-docs --disable-tools --disable-examples --disable-unit-tests --disable-decode-perf-tests --disable-encode-perf-tests
make
POPD
COPY /Y libvpx\x64\Debug\vpxmdd.lib vpxmdd.lib
COPY /Y libvpx\x64\Release\vpxmd.lib vpxmd.lib
