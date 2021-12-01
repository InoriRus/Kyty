@ECHO OFF

ECHO 1.Debug
ECHO 2.Debug Final
ECHO 3.Release
ECHO 4.Release Final
ECHO 5.Exit
ECHO.

CHOICE /C 12345 /M "Enter your choice:"

@ECHO ON

GOTO choice-%errorlevel%

:choice-1
mkdir _DebugEclipseNinja
cd _DebugEclipseNinja
cmake -G "Eclipse CDT4 - Ninja" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -D CMAKE_BUILD_TYPE=Debug ../../source
GOTO End

:choice-2
mkdir _DebugFinalEclipseNinja
cd _DebugFinalEclipseNinja
cmake -G "Eclipse CDT4 - Ninja" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -D CMAKE_BUILD_TYPE=Debug -D KYTY_FINAL=1 ../../source
GOTO End

:choice-3
mkdir _ReleaseEclipseNinja
cd _ReleaseEclipseNinja
cmake -G "Eclipse CDT4 - Ninja" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -D CMAKE_BUILD_TYPE=Release ../../source
GOTO End

:choice-4
mkdir _ReleaseFinalEclipseNinja
cd _ReleaseFinalEclipseNinja
cmake -G "Eclipse CDT4 - Ninja" -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -D CMAKE_BUILD_TYPE=Release -D KYTY_FINAL=1 ../../source
GOTO End

:choice-5
:End