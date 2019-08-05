PUSHD mp4v2\mp4v2-Win
msbuild.exe mp4v2.sln -m -t:Build -p:Configuration=Release -p:Platform=x64 -p:PlatformToolset=v142 -p:WindowsTargetPlatformVersion=10.0
POPD
exit 0