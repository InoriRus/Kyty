[![Build status](https://ci.appveyor.com/api/projects/status/0du32fg9flol63to?svg=true)](https://ci.appveyor.com/project/InoriRus/kyty) [![CI](https://github.com/InoriRus/Kyty/actions/workflows/ci.yml/badge.svg)](https://github.com/InoriRus/Kyty/actions/workflows/ci.yml)

# Kyty
## PS4 & PS5 emulator

---
The project is in its early stage.

[Vladimir M](mailto:inorirus@gmail.com)

Licensed under the MIT license.

---
Graphics for PS5 is under development.

It is possible to run some simple games for PS4

There maybe graphics glitches, crashes, freezes and low FPS. It's OK for now.

Features that are not implemented:
- Audio input/output
- MP4 video
- Network
- Multi-user

Path to Savedata folder is hardcoded and can't be configured.
System parameters (language, date format, etc.) are also hardcoded.

---
### Screenshots
<img src="https://user-images.githubusercontent.com/7149418/169674296-4185e2da-99f9-4073-8ca9-19dc124c7459.png" width="400"> <img src="https://user-images.githubusercontent.com/7149418/169674298-df817d95-7288-46fe-a040-3c0a40c29a6b.png" width="400"> <img src="https://user-images.githubusercontent.com/7149418/169674301-37a3f947-76cd-4a9b-8c81-adec3d5d9c59.png" width="400"> <img src="https://user-images.githubusercontent.com/7149418/169674303-13edae7d-24d3-4ec6-ba94-586e13c69df5.png" width="400">

---
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
* Vulkan SDK 1.2.198.1
* Qt 5.15.0
