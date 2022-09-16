file(GLOB scripts_src
    "src/*.cpp"
)

#add_library(scripts_obj OBJECT ${scripts_src})
#add_library(scripts STATIC $<TARGET_OBJECTS:scripts_obj>)
add_library(scripts OBJECT ${scripts_src})

target_link_libraries(scripts core lua)

#target_include_directories(scripts PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

get_property(inc_headers TARGET scripts PROPERTY INCLUDE_DIRECTORIES)

list(APPEND inc_headers 
	${CMAKE_SOURCE_DIR}/3rdparty/lua/src 	
)

target_include_directories(scripts PRIVATE ${inc_headers})

list(APPEND check_headers
	${CMAKE_SOURCE_DIR}/include
)

clang_tidy_check(scripts "" "${check_headers}" "${inc_headers}")

include_what_you_use(scripts "${inc_headers}")



