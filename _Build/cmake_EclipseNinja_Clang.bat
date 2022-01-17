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

@ECHO ON

GOTO choice-%CH%

:choice-1
mkdir _DebugEclipseNinjaClang
cd _DebugEclipseNinjaClang
echo ninja >_build.bat
echo ninja install/strip >>_build.bat
cmake -G "Eclipse CDT4 - Ninja" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Debug -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-2
mkdir _DebugFinalEclipseNinjaClang
cd _DebugFinalEclipseNinjaClang
echo ninja >_build.bat
echo ninja install/strip >>_build.bat
cmake -G "Eclipse CDT4 - Ninja" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Debug -D KYTY_FINAL=1 -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-3
mkdir _ReleaseEclipseNinjaClang
cd _ReleaseEclipseNinjaClang
echo ninja >_build.bat
echo ninja install/strip >>_build.bat
cmake -G "Eclipse CDT4 - Ninja" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-4
mkdir _ReleaseFinalEclipseNinjaClang
cd _ReleaseFinalEclipseNinjaClang
echo ninja >_build.bat
echo ninja install/strip >>_build.bat
cmake -G "Eclipse CDT4 - Ninja" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Release -D KYTY_FINAL=1 -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-5
:End