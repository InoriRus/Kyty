file(GLOB zstd_src
	lib/common/*.c
	lib/compress/*.c
	lib/decompress/*.c
	lib/dictBuilder/*.c
	lib/deprecated/*.c
	lib/legacy/*.c
	lib/common/*.h
	lib/compress/*.h
	lib/decompress/*.h
	lib/dictBuilder/*.h
	lib/deprecated/*.h
	lib/legacy/*.h
	lib/zstd.h
)

if (MINGW)
	if (CLANG)
		set_source_files_properties(${zstd_src} PROPERTIES COMPILE_FLAGS "")
	else()
		set_source_files_properties(${zstd_src} PROPERTIES COMPILE_FLAGS "")
	endif()
endif()

if (MSVC)
	if (CLANG)
		set_source_files_properties(${zstd_src} PROPERTIES COMPILE_FLAGS "-Wno-unused-function")
	else()
		set_source_files_properties(${zstd_src} PROPERTIES COMPILE_FLAGS "")
	endif()
endif()

include_directories(lib lib/common lib/legacy)

#add_library(zstd STATIC ${zstd_src})
add_library(zstd_obj OBJECT ${zstd_src})
add_library(zstd STATIC $<TARGET_OBJECTS:zstd_obj>)


target_include_directories(zstd PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lib")

target_include_directories(zstd_obj PRIVATE $<TARGET_PROPERTY:zstd,INTERFACE_INCLUDE_DIRECTORIES>)

