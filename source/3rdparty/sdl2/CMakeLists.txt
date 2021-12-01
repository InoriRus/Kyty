
file(GLOB sdl2_src
  include/*.h
  src/*.c
  src/atomic/*.c
  src/audio/*.c
  src/audio/directsound/*.c
  src/audio/dummy/*.c
  src/audio/winmm/*.c
  src/audio/disk/*.c
  src/cpuinfo/*.c
  src/dynapi/*.c
  src/events/*.c
  src/file/*.c
  src/libm/*.c
  src/render/*.c
  src/render/*/*.c
  src/stdlib/*.c
  src/thread/*.c
  src/timer/*.c
  src/sensor/*.c
  src/sensor/dummy/*.c
  src/video/*.c
  src/video/windows/*.c
  src/video/dummy/*.c
  src/video/yuv2rgb/*.c
  src/core/windows/*.c
  src/timer/windows/*.c
  src/loadso/windows/*.c
  src/filesystem/windows/*.c
  src/thread/windows/SDL_sysmutex.c
  src/thread/windows/SDL_syssem.c
  src/thread/windows/SDL_systhread.c
  src/thread/windows/SDL_systls.c
  src/thread/generic/SDL_syscond.c
  src/hidapi/windows/*.c
  src/joystick/dummy/*.c
  src/joystick/windows/*.c
  src/joystick/hidapi/*.c
  src/joystick/*.c
  src/haptic/dummy/*.c
  src/haptic/windows/*.c
  src/haptic/*.c
  src/power/*.c
  src/power/windows/*.c
  src/*.h
  src/atomic/*.h
  src/audio/*.h
  src/audio/directsound/*.h
  src/audio/dummy/*.h
  src/audio/winmm/*.h
  src/audio/disk/*.h
  src/cpuinfo/*.h
  src/dynapi/*.h
  src/events/*.h
  src/file/*.h
  src/libm/*.h
  src/render/*.h
  src/render/*/*.h
  src/stdlib/*.h
  src/thread/*.h
  src/timer/*.h
  src/sensor/*.h
  src/sensor/dummy/*.h
  src/video/*.h
  src/video/windows/*.h
  src/video/dummy/*.h
  src/video/yuv2rgb/*.h
  src/core/windows/*.h
  src/timer/windows/*.h
  src/loadso/windows/*.h
  src/filesystem/windows/*.h
  src/joystick/dummy/*.h
  src/joystick/*.h
  src/haptic/dummy/*.h
  src/haptic/*.h
  src/power/*.h
  src/power/windows/*.h
)

if (MINGW)
	if (CLANG)
		set_source_files_properties(${sdl2_src} PROPERTIES COMPILE_FLAGS "-Wno-pragma-pack -Wno-shadow -Wno-unknown-pragmas -Wno-unused-local-typedefs -DHAVE_LIBC -DUSING_GENERATED_CONFIG_H -D_FLOAT_H___")
	else()
		set_source_files_properties(${sdl2_src} PROPERTIES COMPILE_FLAGS "-Wno-shadow -Wno-unused-but-set-variable -Wno-unknown-pragmas -Wno-unused-local-typedefs -Wno-maybe-uninitialized -DHAVE_LIBC -DUSING_GENERATED_CONFIG_H")
	endif()
endif()

if (MSVC)
	if (CLANG)	
		set_source_files_properties(${sdl2_src} PROPERTIES COMPILE_FLAGS "-Wno-implicit-function-declaration -Wno-pragma-pack -Wno-shadow -Wno-unknown-pragmas -Wno-unused-local-typedefs -DHAVE_LIBC -DUSING_GENERATED_CONFIG_H -D_FLOAT_H___")
		set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/src/cpuinfo/SDL_cpuinfo.c" PROPERTIES COMPILE_FLAGS "/FI intrin.h -Wno-implicit-function-declaration -Wno-pragma-pack -Wno-shadow -Wno-unknown-pragmas -Wno-unused-local-typedefs -DHAVE_LIBC -DUSING_GENERATED_CONFIG_H -D_FLOAT_H___")
	else()
		set_source_files_properties(${sdl2_src} PROPERTIES COMPILE_FLAGS "-DHAVE_LIBC -D_CRT_SECURE_NO_WARNINGS /wd4996 -DUSING_GENERATED_CONFIG_H")
	endif()
endif()

configure_file(
  SDL_config_windows_altered.h
  ${PROJECT_BINARY_DIR}/SDL_config.h
  COPYONLY
  )

configure_file(
  SDL_dynapi_altered.h
  ${PROJECT_BINARY_DIR}/dynapi/SDL_dynapi.h
  COPYONLY
  )


#add_library(sdl2 STATIC ${sdl2_src})
add_library(sdl2_obj OBJECT ${sdl2_src})
add_library(sdl2 STATIC $<TARGET_OBJECTS:sdl2_obj>)


target_include_directories(sdl2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(sdl2 user32 gdi32 winmm imm32 ole32 oleaut32 version uuid)

target_include_directories(sdl2_obj PRIVATE $<TARGET_PROPERTY:sdl2,INTERFACE_INCLUDE_DIRECTORIES>)
