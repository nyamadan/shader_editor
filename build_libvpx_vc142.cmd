cd libvpx
bash ./configure --target=x86_64-win64-vs15 --disable-vp9 --disable-vp8-decoder --disable-docs --disable-tools --disable-examples
make || msbuild vpx.vcxproj -m -t:Build -p:Configuration=Release -p:Platform=x64 -p:PlatformToolset=v142
cd ..