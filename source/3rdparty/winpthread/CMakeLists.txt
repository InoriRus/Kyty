if (MSVC AND CLANG)

add_library(winpthread SHARED IMPORTED GLOBAL)

set_target_properties(winpthread PROPERTIES IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/libwinpthread.dll.a")

endif()

