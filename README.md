# Kyty
ps4 & ps5 emulator
-----
The project is in its early stage.

[Vladimir M](mailto:inorirus@gmail.com)

Licensed under the MIT license.

### Building
Supported platforms:
- Windows 10 x64

Toolchains:
- Visual Studio + clang-cl + ninja
- Eclipse CDT + mingw-w64 + gcc/clang + ninja/mingw32-make

Supported versions:
Tool                            | Version
:------------                   | :------------
cmake                           |3.12
Visual Studio 2019              |16.10.3
clang                           |12.0.1
clang-cl                        |11.0.0
gcc (MinGW-W64 x86_64-posix-seh)|10.2.0
ninja                           |1.10.1
MinGW-w64                       |8.0.0
Eclipse CDT                     |10.3.0
Qt                              |5.15.0

Define environment variable named Qt5_DIR pointing to the proper version of Qt

MSVC compiler (cl.exe) is not supported!

External dependencies:
* Vulkan SDK 1.2.176.1
* Qt 5.15.0
