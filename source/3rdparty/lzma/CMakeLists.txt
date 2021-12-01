file(GLOB lzma_src
    "C/*.c"
    "C/*.h"
	"include/*.h"
)

if (CLANG)
	set_source_files_properties(${lzma_src} PROPERTIES COMPILE_FLAGS "-Wno-misleading-indentation -D_7ZIP_ST")
else()
	set_source_files_properties(${lzma_src} PROPERTIES COMPILE_FLAGS "-D_7ZIP_ST")
endif()


include_directories("include")

#add_library(lzma STATIC ${lzma_src})
add_library(lzma_obj OBJECT ${lzma_src})
add_library(lzma STATIC $<TARGET_OBJECTS:lzma_obj>)

target_include_directories(lzma PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(lzma_obj PRIVATE $<TARGET_PROPERTY:lzma,INTERFACE_INCLUDE_DIRECTORIES>)

