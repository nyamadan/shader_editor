PUSHD libvpx
bash -e "./configure --target=x86_64-win64-vs15 --disable-vp9 --disable-vp8-decoder --disable-docs --disable-tools --disable-examples && make"
IF NOT %ERRORLEVEL% == 0 (
  msbuild vpx.vcxproj -m -t:Build -p:Configuration=Release -p:Platform=x64 -p:PlatformToolset=v142
)
POPD
exit %ERRORLEVEL%
