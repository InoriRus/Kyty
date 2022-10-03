#!/bin/bash

PS3='Please enter your choice: '
options=("Debug" "Debug Final" "Release" "Release Final" "Exit")
select opt in "${options[@]}"
do
    case $opt in
	"Debug")
	    DirName=_DebugLinuxEclipseNinjaClang
	    Options="-DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Debug -D CMAKE_INSTALL_PREFIX=_bin ../../source"
		;;
	"Debug Final")
	    DirName=_DebugFinalLinuxEclipseNinjaClang
	    Options="-DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Debug -D KYTY_FINAL=1 -D CMAKE_INSTALL_PREFIX=_bin ../../source"
		;;
	"Release")
	    DirName=_ReleaseLinuxEclipseNinjaClang
	    Options="-DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=_bin ../../source"
		;;
	"Release Final")
	    DirName=_ReleaseFinalLinuxEclipseNinjaClang
	    Options="-DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Release -D KYTY_FINAL=1 -D CMAKE_INSTALL_PREFIX=_bin ../../source"
		;;
    esac
    break
done

if [ -z "$DirName" ]; then
    exit
fi

mkdir $DirName
cd $DirName
echo ninja >_build
echo ninja install/strip >>_build
chmod +x _build
cmake -G "Eclipse CDT4 - Ninja" $Options
