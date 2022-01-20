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
mkdir _DebugEclipseNinja
cd _DebugEclipseNinja
) else (
mkdir %2
cd %2
)
echo ninja >_build.bat
echo ninja install/strip >>_build.bat
cmake -G "Eclipse CDT4 - Ninja" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -D CMAKE_BUILD_TYPE=Debug -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-2
if !%2==! (
mkdir _DebugFinalEclipseNinja
cd _DebugFinalEclipseNinja
) else (
mkdir %2
cd %2
)
echo ninja >_build.bat
echo ninja install/strip >>_build.bat
cmake -G "Eclipse CDT4 - Ninja" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -D CMAKE_BUILD_TYPE=Debug -D KYTY_FINAL=1 -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-3
if !%2==! (
mkdir _ReleaseEclipseNinja
cd _ReleaseEclipseNinja
) else (
mkdir %2
cd %2
)
echo ninja >_build.bat
echo ninja install/strip >>_build.bat
cmake -G "Eclipse CDT4 - Ninja" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-4
if !%2==! (
mkdir _ReleaseFinalEclipseNinja
cd _ReleaseFinalEclipseNinja
) else (
mkdir %2
cd %2
)
echo ninja >_build.bat
echo ninja install/strip >>_build.bat
cmake -G "Eclipse CDT4 - Ninja" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -D CMAKE_BUILD_TYPE=Release -D KYTY_FINAL=1 -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-5
:End