PUSHD libvpx
set PATH=C:\msys64\usr\bin;C:\msys64\mingw64\bin;%PATH%
bash ./configure --target=x86_64-win64-vs16 --disable-vp9 --disable-vp8-decoder --disable-docs --disable-tools --disable-examples
make
msbuild.exe vpx.vcxproj -m -t:Build  -p:Configuration="Release" -p:Platform="x64"
POPD
COPY /Y libvpx\x64\Debug\vpxmdd.lib vpxmdd.lib
COPY /Y libvpx\x64\Release\vpxmd.lib vpxmd.lib
