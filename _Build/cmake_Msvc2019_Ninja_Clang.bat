@ECHO OFF

if not !%1==! goto :with_arg

ECHO 1.Debug
ECHO 2.Debug Final
ECHO 3.Release
ECHO 4.Release Final
ECHO 5.Exit
ECHO.

CHOICE /C 12345 /M "Enter your choice:"

IF %errorlevel% EQU 5 goto :End
goto :without_arg

:with_arg
set CH=%1
goto :Start
:without_arg
set CH=%errorlevel%
:Start

set MSVC=C:/Program Files (x86)/Microsoft Visual Studio/2019/Community
set PATH=C:\Windows\system32;C:\Windows;%MSVC%\Common7\IDE\CommonExtensions\Microsoft\TeamFoundation\Team Explorer\Git\cmd;
set VCVARSALL="%MSVC%/VC/Auxiliary/Build/vcvarsall.bat"
call %VCVARSALL% x64
set CFLAGS=-m64 -fdiagnostics-absolute-paths
set CXXFLAGS=-m64 -fdiagnostics-absolute-paths

rem @ECHO ON

GOTO choice-%CH%

:choice-1
if !%2==! (
mkdir _DebugMsvc2019NinjaClang
cd _DebugMsvc2019NinjaClang
) else (
mkdir %2
cd %2
)
set >env.txt
echo call %VCVARSALL% x64 >_build.bat
echo ninja >>_build.bat
echo ninja install >>_build.bat
where cmake.exe >cmake.txt
cmake -G "Ninja" -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -D CMAKE_BUILD_TYPE=Debug -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-2
if !%2==! (
mkdir _DebugFinalMsvc2019NinjaClang
cd _DebugFinalMsvc2019NinjaClang
) else (
mkdir %2
cd %2
)
set >env.txt
echo call %VCVARSALL% x64 >_build.bat
echo ninja >>_build.bat
echo ninja install >>_build.bat
where cmake.exe >cmake.txt
cmake -G "Ninja" -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -D CMAKE_BUILD_TYPE=Debug -D KYTY_FINAL=1 -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-3
if !%2==! (
mkdir _ReleaseMsvc2019NinjaClang
cd _ReleaseMsvc2019NinjaClang
) else (
mkdir %2
cd %2
)
set >env.txt
echo call %VCVARSALL% x64 >_build.bat
echo ninja >>_build.bat
echo ninja install >>_build.bat
where cmake.exe >cmake.txt
cmake -G "Ninja" -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-4
if !%2==! (
mkdir _ReleaseFinalMsvc2019NinjaClang
cd _ReleaseFinalMsvc2019NinjaClang
) else (
mkdir %2
cd %2
)
set >env.txt
echo call %VCVARSALL% x64 >_build.bat
echo ninja >>_build.bat
echo ninja install >>_build.bat
where cmake.exe >cmake.txt
cmake -G "Ninja" -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -D CMAKE_BUILD_TYPE=Release -D KYTY_FINAL=1 -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-5
:End