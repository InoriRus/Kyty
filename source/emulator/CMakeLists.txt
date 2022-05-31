file(GLOB emulator_src
	src/*.cpp
	src/*.cpp
	src/Libs/*.cpp
	src/Graphics/*.cpp
	src/Graphics/Objects/*.cpp
	src/Kernel/*.cpp
	src/Loader/*.cpp
	include/*.h
)

if (MSVC AND CLANG)
	set_source_files_properties(${emulator_src} PROPERTIES COMPILE_FLAGS "-Wno-pragma-pack -Wno-deprecated-declarations -D_TIMESPEC_DEFINED")
endif()

add_library(emulator_obj OBJECT ${emulator_src})
add_library(emulator STATIC $<TARGET_OBJECTS:emulator_obj>)

target_link_libraries(emulator core math scripts lua unit_test vulkan-1 spirv-tools-opt spirv-tools easy_profiler)

target_include_directories(emulator PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

get_property(inc_headers TARGET emulator PROPERTY INCLUDE_DIRECTORIES)

list(APPEND inc_headers
	${CMAKE_SOURCE_DIR}/3rdparty/sdl2/include
	${CMAKE_SOURCE_DIR}/3rdparty/vulkan/include
	${CMAKE_SOURCE_DIR}/3rdparty/easy_profiler/include
	${CMAKE_SOURCE_DIR}/3rdparty/xxhash/include
	${CMAKE_SOURCE_DIR}/3rdparty/cpuinfo/include
	${CMAKE_SOURCE_DIR}/3rdparty/stb
)

if (MSVC AND CLANG)
	target_link_libraries(emulator winpthread)
	list(APPEND inc_headers 
		${CMAKE_SOURCE_DIR}/3rdparty/winpthread/include
	)
endif()

target_include_directories(emulator_obj PRIVATE ${inc_headers})

list(APPEND check_headers
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/include
)

clang_tidy_check(emulator_obj "" "${check_headers}" "${inc_headers}")

include_what_you_use(emulator_obj "${inc_headers}")

