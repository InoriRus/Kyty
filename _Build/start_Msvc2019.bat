set MSVC=C:/Program Files (x86)/Microsoft Visual Studio/2019/Community
set PATH=C:\Windows\system32;C:\Windows;%MSVC%\Common7\IDE\CommonExtensions\Microsoft\TeamFoundation\Team Explorer\Git\cmd;
set VCVARSALL="%MSVC%/VC/Auxiliary/Build/vcvarsall.bat"
call %VCVARSALL% x64
start "" "%MSVC%\Common7\IDE\devenv.exe" ..