file(GLOB lua_src
    "include/*.h"
    "src/*.c"
)

if (MINGW)
#set_source_files_properties(${lua_src} PROPERTIES COMPILE_FLAGS "-DLUA_BUILD_AS_DLL")
endif()

if (MSVC)
#set_source_files_properties(${lua_src} PROPERTIES LANGUAGE CXX )
set_source_files_properties(${lua_src} PROPERTIES COMPILE_FLAGS "-D_CRT_SECURE_NO_WARNINGS")
endif()

configure_file(
  luaconf_windows.h
  ${PROJECT_BINARY_DIR}/luaconf.h
  COPYONLY
  )


#add_library(lua STATIC ${lua_src})
add_library(lua_obj OBJECT ${lua_src})
add_library(lua STATIC $<TARGET_OBJECTS:lua_obj>)

