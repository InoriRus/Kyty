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
mkdir _BuildToolsDebugNinjaClang
cd _BuildToolsDebugNinjaClang
) else (
mkdir %2
cd %2
)
echo ninja >_build.bat
echo ninja install/strip >>_build.bat
cmake -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Debug -D KYTY_PROJECT_NAME:STRING=Build_Tools -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-2
if !%2==! (
mkdir _BuildToolsDebugFinalNinjaClang
cd _BuildToolsDebugFinalNinjaClang
) else (
mkdir %2
cd %2
)
echo ninja >_build.bat
echo ninja install/strip >>_build.bat
cmake -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Debug -D KYTY_PROJECT_NAME:STRING=Build_Tools -D KYTY_FINAL=1 -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-3
if !%2==! (
mkdir _BuildToolsReleaseNinjaClang
cd _BuildToolsReleaseNinjaClang
) else (
mkdir %2
cd %2
)
echo ninja >_build.bat
echo ninja install/strip >>_build.bat
cmake -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Release -D KYTY_PROJECT_NAME:STRING=Build_Tools -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-4
if !%2==! (
mkdir _BuildToolsReleaseFinalNinjaClang
cd _BuildToolsReleaseFinalNinjaClang
) else (
mkdir %2
cd %2
)
echo ninja >_build.bat
echo ninja install/strip >>_build.bat
cmake -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Release -D KYTY_PROJECT_NAME:STRING=Build_Tools -D KYTY_FINAL=1 -D CMAKE_INSTALL_PREFIX=_bin ../../source
GOTO End

:choice-5
:End