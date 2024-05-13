#!/usr/bin/env bash

PS3='Please enter your choice: '
options=("Debug" "Debug Final" "Release" "Release Final" "Exit")
select opt in "${options[@]}"
do
    case $opt in
	"Debug")
	    DirName=_DebugMacOSMakeGcc
	    Options="-DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -D CMAKE_BUILD_TYPE=Debug -D CMAKE_INSTALL_PREFIX=_bin ../../source"
		;;
	"Debug Final")
	    DirName=_DebugFinalMacOSMakeGcc
	    Options="-DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -D CMAKE_BUILD_TYPE=Debug -D KYTY_FINAL=1 -D CMAKE_INSTALL_PREFIX=_bin ../../source"
		;;
	"Release")
	    DirName=_ReleaseMacOSMakeGcc
	    Options="-DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=_bin ../../source"
		;;
	"Release Final")
	    DirName=_ReleaseFinalMacOSMakeGcc
	    Options="-DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -D CMAKE_BUILD_TYPE=Release -D KYTY_FINAL=1 -D CMAKE_INSTALL_PREFIX=_bin ../../source"
		;;
    esac
    break
done

if [ -z "$DirName" ]; then
    exit
fi

mkdir $DirName
cd $DirName
echo make >_build
echo make install/strip >>_build
chmod +x _build
cmake -G "Unix Makefiles" $Options
