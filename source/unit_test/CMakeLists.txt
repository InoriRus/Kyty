file(GLOB unit_test_src
    "include/*.h"
    "src/*.cpp"
    "src/core/*.cpp"
)

if (MSVC AND CLANG)
	set_source_files_properties(${unit_test_src} PROPERTIES COMPILE_FLAGS "-Wno-pragma-pack")
endif()

add_library(unit_test STATIC ${unit_test_src})

target_link_libraries(unit_test core)
target_link_libraries(unit_test math)

#target_include_directories(unit_test PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

list(APPEND inc_headers 
	${PROJECT_BINARY_DIR}
	${PROJECT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/3rdparty/gtest/include
	${CMAKE_SOURCE_DIR}/3rdparty/gtest
)

list(APPEND check_headers
	${CMAKE_SOURCE_DIR}/include
)

clang_tidy_check(unit_test "" "${check_headers}" "${inc_headers}")

include_what_you_use(unit_test "${inc_headers}")


