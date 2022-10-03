[![Build status](https://ci.appveyor.com/api/projects/status/0du32fg9flol63to?svg=true)](https://ci.appveyor.com/project/InoriRus/kyty) [![CI](https://github.com/InoriRus/Kyty/actions/workflows/ci.yml/badge.svg)](https://github.com/InoriRus/Kyty/actions/workflows/ci.yml) 

# Kyty
## PS4 & PS5 emulator

---
The project is in its early stage.

[Vladimir M](mailto:inorirus@gmail.com)

Licensed under the MIT license.

---
It is possible to run some simple games for PS4 and homebrews for PS5.

Check out the [Community Compatibility List](https://docs.google.com/spreadsheets/d/1NSf_N57Qffy5CAr2QYpWi0jOMOHJjhXB9JsYNqPqGHk/edit), any contributions are appreciated!

There maybe graphical glitches, crashes, freezes and low FPS. It's OK for now.

Features that are not implemented:
- Audio input/output
- MP4 video
- Network
- Multi-user

Path to Savedata folder is hardcoded and can't be configured.
System parameters (language, date format, etc.) are also hardcoded.

---
### Screenshots
#### PS4
<img src="https://user-images.githubusercontent.com/7149418/169674296-4185e2da-99f9-4073-8ca9-19dc124c7459.png" width="400"> <img src="https://user-images.githubusercontent.com/7149418/169674298-df817d95-7288-46fe-a040-3c0a40c29a6b.png" width="400"> <img src="https://user-images.githubusercontent.com/7149418/169674301-37a3f947-76cd-4a9b-8c81-adec3d5d9c59.png" width="400"> <img src="https://user-images.githubusercontent.com/7149418/169674303-13edae7d-24d3-4ec6-ba94-586e13c69df5.png" width="400">
#### PS5
<img src="https://user-images.githubusercontent.com/7149418/185373811-3c12178d-d924-4da1-be7a-06ff6cb733b7.png" width="800">

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

## Donate

If you would like to support the project:

- Bitcoin: bc1qd66pjk3xj3hzvm379uxy470n533nnt2deenpea

<img src="https://user-images.githubusercontent.com/7149418/181066559-7f35befb-ad23-480c-9a75-3b663b1b9957.png" width="200">

## Hire me

**I'm available for hiring.** If you need C++ developer, please take a look at my [profile](https://github.com/InoriRus) and contact me by email

**Я в поисках работы.** Если вам нужен разработчик C++, пишите мне на почту.
