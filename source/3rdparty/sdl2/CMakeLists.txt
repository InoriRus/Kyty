set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)

add_subdirectory(sdl2 build_sdl2)

if (CLANG AND NOT ANDROID)
	target_compile_options(SDL2-static PRIVATE -Wno-pragma-pack -Wno-unused-command-line-argument -Wno-unused-function -Wno-incompatible-function-pointer-types -Wno-unused-const-variable)
	if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0.0)
		target_compile_options(SDL2-static PRIVATE -Wno-unused-but-set-variable)
	endif()
endif()

if (MSVC AND NOT CLANG)
	target_compile_options(SDL2-static PRIVATE /wd4028)
endif()

if (ANDROID)
	target_compile_options(SDL2-static PRIVATE -Wno-deprecated-declarations)
endif()

if (LINUX)
    if(CLANG)
	target_compile_definitions(SDL2-static PRIVATE SDL_VIDEO_DRIVER_X11_SUPPORTS_GENERIC_EVENTS)
	target_compile_options(SDL2-static PRIVATE -Wno-unused-variable)
    else()
	target_compile_options(SDL2-static PRIVATE -Wno-unused-variable -Wno-unused-function)
    endif()
endif()