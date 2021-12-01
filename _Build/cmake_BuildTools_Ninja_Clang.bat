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
mkdir _BuildToolsDebugNinjaClang
cd _BuildToolsDebugNinjaClang
cmake -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Debug -D KYTY_PROJECT_NAME:STRING=Build_Tools ../../source
GOTO End

:choice-2
mkdir _BuildToolsDebugFinalNinjaClang
cd _BuildToolsDebugFinalNinjaClang
cmake -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Debug -D KYTY_PROJECT_NAME:STRING=Build_Tools -D KYTY_FINAL=1 ../../source
GOTO End

:choice-3
mkdir _BuildToolsReleaseNinjaClang
cd _BuildToolsReleaseNinjaClang
cmake -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Release -D KYTY_PROJECT_NAME:STRING=Build_Tools ../../source
GOTO End

:choice-4
mkdir _BuildToolsReleaseFinalNinjaClang
cd _BuildToolsReleaseFinalNinjaClang
cmake -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Release -D KYTY_PROJECT_NAME:STRING=Build_Tools -D KYTY_FINAL=1 ../../source
GOTO End

:choice-5
:End