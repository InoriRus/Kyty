file(GLOB lua_src
    "src/*.c"
)

list(REMOVE_ITEM lua_src ${CMAKE_CURRENT_SOURCE_DIR}/src/luac.c)

add_library(lua STATIC ${lua_src})

if (MINGW)
	#set_source_files_properties(${lua_src} PROPERTIES COMPILE_FLAGS "-DLUA_BUILD_AS_DLL")
endif()

if (MSVC)
	#set_source_files_properties(${lua_src} PROPERTIES LANGUAGE CXX )
	#set_source_files_properties(${lua_src} PROPERTIES COMPILE_FLAGS "-D_CRT_SECURE_NO_WARNINGS")
endif()

if (ANDROID)
	target_compile_options(lua PRIVATE -Wno-deprecated-declarations)
endif()


#configure_file(
#  luaconf_windows.h
#  ${PROJECT_BINARY_DIR}/luaconf.h
#  COPYONLY
#  )





