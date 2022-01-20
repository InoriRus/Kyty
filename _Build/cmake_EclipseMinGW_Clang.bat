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

rem @ECHO ON

GOTO choice-%CH%

:choice-1
if !%2==! (
mkdir _DebugEclipseMinGWClang
cd _DebugEclipseMinGWClang
) else (
mkdir %2
cd %2
)
echo mingw32-make.exe >_build.bat
echo mingw32-make.exe install/strip >>_build.bat
cmake -G "Eclipse CDT4 - MinGW Makefiles" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Debug -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-2
if !%2==! (
mkdir _DebugFinalEclipseMinGWClang
cd _DebugFinalEclipseMinGWClang
) else (
mkdir %2
cd %2
)
echo mingw32-make.exe >_build.bat
echo mingw32-make.exe install/strip >>_build.bat
cmake -G "Eclipse CDT4 - MinGW Makefiles" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Debug -D KYTY_FINAL=1 -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-3
if !%2==! (
mkdir _ReleaseEclipseMinGWClang
cd _ReleaseEclipseMinGWClang
) else (
mkdir %2
cd %2
)
echo mingw32-make.exe >_build.bat
echo mingw32-make.exe install/strip >>_build.bat
cmake -G "Eclipse CDT4 - MinGW Makefiles" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-4
if !%2==! (
mkdir _ReleaseFinalEclipseMinGWClang
cd _ReleaseFinalEclipseMinGWClang
) else (
mkdir %2
cd %2
)
echo mingw32-make.exe >_build.bat
echo mingw32-make.exe install/strip >>_build.bat
cmake -G "Eclipse CDT4 - MinGW Makefiles" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Release -D KYTY_FINAL=1 -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-5
:End