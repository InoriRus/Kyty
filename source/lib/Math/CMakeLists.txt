file(GLOB math_src
    "src/*.cpp"
)

add_library(math_obj OBJECT ${math_src})
add_library(math STATIC $<TARGET_OBJECTS:math_obj>)

target_link_libraries(math core)
target_link_libraries(math rijndael)

get_property(inc_headers TARGET math PROPERTY INCLUDE_DIRECTORIES)

target_include_directories(math_obj PRIVATE ${inc_headers})

list(APPEND check_headers
	${CMAKE_SOURCE_DIR}/include
)

clang_tidy_check(math_obj "" "${check_headers}" "${inc_headers}")

include_what_you_use(math_obj "${inc_headers}")


