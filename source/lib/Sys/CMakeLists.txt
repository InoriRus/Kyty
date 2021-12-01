file(GLOB sys_src
    "src/*.cpp"
)

add_library(sys_obj OBJECT ${sys_src})
add_library(sys STATIC $<TARGET_OBJECTS:sys_obj>)

target_link_libraries(sys core)

get_property(inc_headers TARGET sys PROPERTY INCLUDE_DIRECTORIES)

target_include_directories(sys_obj PRIVATE ${inc_headers})

list(APPEND check_headers
	${CMAKE_SOURCE_DIR}/include
)

clang_tidy_check(sys_obj "" "${check_headers}" "${inc_headers}")

include_what_you_use(sys_obj "${inc_headers}")



