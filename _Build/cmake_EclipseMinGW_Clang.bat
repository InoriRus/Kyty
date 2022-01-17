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
mkdir _DebugEclipseMinGWClang
cd _DebugEclipseMinGWClang
echo mingw32-make.exe >_build.bat
echo mingw32-make.exe install/strip >>_build.bat
cmake -G "Eclipse CDT4 - MinGW Makefiles" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Debug ../../source
GOTO End

:choice-2
mkdir _DebugFinalEclipseMinGWClang
cd _DebugFinalEclipseMinGWClang
echo mingw32-make.exe >_build.bat
echo mingw32-make.exe install/strip >>_build.bat
cmake -G "Eclipse CDT4 - MinGW Makefiles" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Debug -D KYTY_FINAL=1 ../../source
GOTO End

:choice-3
mkdir _ReleaseEclipseMinGWClang
cd _ReleaseEclipseMinGWClang
echo mingw32-make.exe >_build.bat
echo mingw32-make.exe install/strip >>_build.bat
cmake -G "Eclipse CDT4 - MinGW Makefiles" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Release ../../source
GOTO End

:choice-4
mkdir _ReleaseFinalEclipseMinGWClang
cd _ReleaseFinalEclipseMinGWClang
echo mingw32-make.exe >_build.bat
echo mingw32-make.exe install/strip >>_build.bat
cmake -G "Eclipse CDT4 - MinGW Makefiles" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Release -D KYTY_FINAL=1 ../../source
GOTO End

:choice-5
:End