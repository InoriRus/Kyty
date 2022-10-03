file(GLOB sys_src
    "src/*.cpp"
)

add_library(sys STATIC ${sys_src})

target_link_libraries(sys core cpuinfo)

get_property(inc_headers TARGET sys PROPERTY INCLUDE_DIRECTORIES)

list(APPEND check_headers
	${CMAKE_SOURCE_DIR}/include
)

list(APPEND inc_headers
	${CMAKE_SOURCE_DIR}/3rdparty/sdl2/sdl2/include
	${CMAKE_SOURCE_DIR}/3rdparty/cpuinfo/include
)

clang_tidy_check(sys "" "${check_headers}" "${inc_headers}")
#clang_tidy_fix(sys_obj "{Checks: '-*,cppcoreguidelines-init-variables'}" "${check_headers}" "${inc_headers}")

include_what_you_use(sys "${inc_headers}")



