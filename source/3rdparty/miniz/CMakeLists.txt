file(GLOB miniz_src
    "src/*.c"
)

add_library(miniz STATIC ${miniz_src})

target_compile_definitions(miniz PRIVATE MINIZ_NO_STDIO MINIZ_NO_MALLOC)

if (MINGW)

endif()

if (MSVC)
endif()

if (ANDROID)
	#target_compile_options(lua PRIVATE -Wno-deprecated-declarations)
endif()




