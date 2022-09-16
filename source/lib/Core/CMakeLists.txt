file(GLOB core_src
    "src/*.cpp"
)

if (MSVC AND CLANG)
	set_source_files_properties(${core_src} PROPERTIES COMPILE_FLAGS "-Wno-pragma-pack")
endif()

#add_library(core_obj OBJECT ${core_src})
add_library(core STATIC ${core_src})
#add_library(core STATIC $<TARGET_OBJECTS:core_obj>)

target_link_libraries(core sys math SDL2-static sqlite lzma zstd miniz)

get_property(inc_headers TARGET core PROPERTY INCLUDE_DIRECTORIES)
list(APPEND inc_headers 
	${CMAKE_SOURCE_DIR}/3rdparty/sdl2/sdl2/include 	
	${CMAKE_SOURCE_DIR}/3rdparty/sqlite/include
	${CMAKE_SOURCE_DIR}/3rdparty/lzma/include
	${CMAKE_SOURCE_DIR}/3rdparty/zstd/lib
	${CMAKE_SOURCE_DIR}/3rdparty/miniz/src	
)

target_include_directories(core PRIVATE ${inc_headers})
#target_include_directories(core_obj PRIVATE ${inc_headers})

list(APPEND check_headers
	${CMAKE_SOURCE_DIR}/include 
)

#clang_tidy_check(core_obj "" "${check_headers}" "${inc_headers}")
clang_tidy_check(core "" "${check_headers}" "${inc_headers}")

#include_what_you_use(core_obj "${inc_headers}")
include_what_you_use(core "${inc_headers}")


