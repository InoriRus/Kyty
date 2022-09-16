set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)

add_subdirectory(sdl2 build_sdl2)

if (CLANG AND NOT ANDROID)
  target_compile_options(SDL2-static PRIVATE -Wno-pragma-pack -Wno-unused-command-line-argument -Wno-incompatible-function-pointer-types)
endif()

if (MSVC AND NOT CLANG)
  target_compile_options(SDL2-static PRIVATE /wd4028)
endif()

if (ANDROID)
  target_compile_options(SDL2-static PRIVATE -Wno-deprecated-declarations)
endif()
