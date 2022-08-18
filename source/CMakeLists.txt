cmake_minimum_required(VERSION 3.12)

project(Kyty)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(utils.cmake)

	remove_generator_macros(CXX __cplusplus)
	remove_generator_macros(CXX __clang_version__)
	remove_generator_macros(C __clang_version__)
	remove_generator_macros(CXX __VERSION__)
	remove_generator_macros(C __VERSION__)
	
	add_generator_macros(CXX __cplusplus "201703L")

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

include(TestBigEndian)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(KYTY_BITNESS 64)
else()
	set(KYTY_BITNESS 32)
endif()

test_big_endian(BIG_ENDIAN)
if(BIG_ENDIAN)
	set(KYTY_ENDIAN KYTY_ENDIAN_BIG)
else()
	set(KYTY_ENDIAN KYTY_ENDIAN_LITTLE)
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
	set(KYTY_BUILD KYTY_BUILD_DEBUG)
else()
	set(KYTY_BUILD KYTY_BUILD_RELEASE)
endif()

# Platform-specific configurations.
if(MINGW)
	if(CMAKE_CXX_COMPILER_ID MATCHES "(C|c?)lang")
		set(CLANG 1)
		set(KYTY_COMPILER CLANG)
		set(KYTY_LINKER LLD)
		if ((CMAKE_AR MATCHES "llvm") AND (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 12.0.0) AND (CMAKE_CXX_COMPILER MATCHES "winlibs"))
			string(REPLACE "llvm-ar.exe" "ar.exe" CMAKE_AR ${CMAKE_AR}) # llvm-ar.exe 11.0.0 from winlibs is broken
		endif()
	else()
		set(KYTY_COMPILER MINGW)
		set(KYTY_LINKER LD)
	endif()	

	set(KYTY_PLATFORM KYTY_PLATFORM_WINDOWS)
elseif(WIN32)
	# Assume it is MSVC if we are on Windows with non-MinGW.
	if(CMAKE_CXX_COMPILER_ID MATCHES "(C|c?)lang")
		set(CLANG 1)
		set(KYTY_COMPILER CLANG)
		set(KYTY_LINKER LLD_LINK)
	else()
		set(KYTY_COMPILER MSVC)
		set(KYTY_LINKER LINK)
	endif()

	set(KYTY_PLATFORM KYTY_PLATFORM_WINDOWS)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(KYTY_COMPILER GCC)
	set(KYTY_LINKER LD)
	set(KYTY_PLATFORM KYTY_PLATFORM_LINUX)
else()
	message(FATAL_ERROR "The target platform is not supported")
endif()

set(KYTY_PROJECT_NAME "Emulator" CACHE STRING "Project name")

string(TOUPPER ${KYTY_PROJECT_NAME} KYTY_PROJECT)
string(TOLOWER ${KYTY_COMPILER} KYTY_COMPILER_ID)
string(TOLOWER ${KYTY_LINKER} KYTY_LINKER_ID)

if (CLANG AND (KYTY_LINKER STREQUAL LLD))
	set(KYTY_LD_OPTIONS "-fuse-ld=lld")
endif()

if (KYTY_LINKER STREQUAL LD)
	set(KYTY_LD_OPTIONS "-Wl,--image-base=0x100000000000")
endif()

project(Kyty${KYTY_PROJECT_NAME}${CMAKE_BUILD_TYPE}${KYTY_COMPILER} VERSION 0.2.0)

include(src_script.cmake)

include_directories(	
	3rdparty/gtest/include
	3rdparty/gtest
	3rdparty/lua/include
	3rdparty/rijndael/source
	3rdparty/miniz
	3rdparty/magic_enum/include
	${PROJECT_BINARY_DIR}
	include
    )

set(KYTY_VERSION "${PROJECT_VERSION}")

configure_file(
  ${PROJECT_SOURCE_DIR}/cmake_config.h.in
  ${PROJECT_BINARY_DIR}/cmake_config.h
  )

find_package(Git)
  
add_custom_target( KytyGitVersion
    COMMAND ${CMAKE_COMMAND} 
	-D INPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/KytyGitVersion.h.in
    -D OUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/KytyGitVersion.h
	-D GIT_EXECUTABLE=${GIT_EXECUTABLE}
	-P ${CMAKE_CURRENT_SOURCE_DIR}/generate_version.cmake
	COMMENT "Generate KytyGitVersion.h"
)

if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 12.0.0)
	list(APPEND KYTY_IWYU 
		emulator_obj
		core_obj
		math_obj
		scripts_obj
		sys_obj
		#launcher
		unit_test
	)
	list(APPEND KYTY_CLANG_TYDY 
		emulator_obj
		#core_obj
		#math_obj
		#scripts_obj
		#sys_obj
		launcher
		#unit_test
	)
endif()

set(KYTY_LIB_HEADERS ${PROJECT_SOURCE_DIR}/include)
  
config_compiler_and_linker()  
  
add_subdirectory(3rdparty)
add_subdirectory(emulator)
add_subdirectory(unit_test)
if(NOT ${KYTY_PROJECT_NAME} MATCHES "Build_Tools")
	add_subdirectory(launcher)
endif()
add_subdirectory(lib)

add_executable(fc_script ${KYTY_SCRIPT_SRC}) 

if(MINGW)
	SET(UNIT_TEST_LIB -Wl,--whole-archive unit_test -Wl,--no-whole-archive)
endif()	

if(MSVC)
	SET(UNIT_TEST_LIB unit_test)
endif()	

target_link_libraries(fc_script ${UNIT_TEST_LIB})
target_link_libraries(fc_script core)
target_link_libraries(fc_script sys)
target_link_libraries(fc_script math)
target_link_libraries(fc_script scripts)
target_link_libraries(fc_script lua)
target_link_libraries(fc_script opengl32)
target_link_libraries(fc_script iphlpapi)
target_link_libraries(fc_script sdl2)
target_link_libraries(fc_script setupapi)
target_link_libraries(fc_script emulator)
target_link_libraries(fc_script rijndael)
target_link_libraries(fc_script lzma)
target_link_libraries(fc_script sqlite)
target_link_libraries(fc_script zstd)
target_link_libraries(fc_script easy_profiler)
target_link_libraries(fc_script ws2_32)
target_link_libraries(fc_script psapi)
target_link_libraries(fc_script cpuinfo)
if (CLANG AND NOT MSVC)
target_link_libraries(fc_script pthread)
endif()
if (CLANG AND MSVC)
target_link_libraries(fc_script winpthread)
endif()

set(KYTY_SCRIPT_BIN "../../__bin/fc_script")

if(MSVC)
	set_target_properties(fc_script PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(SolutionDir)$(Configuration)/")
	set_target_properties(fc_script
		PROPERTIES
		LINK_FLAGS "/DYNAMICBASE:NO /MAP:fc_script_msvc_${KYTY_BITNESS}.map"
	)
	add_custom_command(TARGET fc_script POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different fc_script_msvc_${KYTY_BITNESS}.map $<TARGET_FILE_DIR:fc_script>/fc_script_${KYTY_COMPILER_ID}_${KYTY_LINKER_ID}_${KYTY_BITNESS}.map)
	add_custom_command(TARGET fc_script POST_BUILD
		COMMAND ${KYTY_SCRIPT_BIN} ${PROJECT_SOURCE_DIR}/map_to_csv.lua $<TARGET_FILE_DIR:fc_script>/fc_script ${KYTY_COMPILER_ID} ${KYTY_BITNESS} ${KYTY_LINKER_ID})
	add_custom_command(TARGET fc_script POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/3rdparty/winpthread/bin/libwinpthread-1.dll" $<TARGET_FILE_DIR:fc_script>/libwinpthread-1.dll)
endif()

if(MINGW)
	set_target_properties(fc_script PROPERTIES LINK_FLAGS "${KYTY_LD_OPTIONS} -Wl,-Map=fc_script_${KYTY_COMPILER_ID}_${KYTY_LINKER_ID}_${KYTY_BITNESS}.map")
	add_custom_command(TARGET fc_script POST_BUILD COMMAND ${KYTY_SCRIPT_BIN} ${PROJECT_SOURCE_DIR}/map_to_csv.lua fc_script ${KYTY_COMPILER_ID} ${KYTY_BITNESS} ${KYTY_LINKER_ID})
endif()

add_dependencies(fc_script KytyGitVersion)  

install(TARGETS fc_script DESTINATION .)
if(NOT ${KYTY_PROJECT_NAME} MATCHES "Build_Tools")
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fc_script_${KYTY_COMPILER_ID}_${KYTY_LINKER_ID}_${KYTY_BITNESS}.csv DESTINATION .)
	if(MSVC)
		install(FILES "3rdparty/winpthread/bin/libwinpthread-1.dll" DESTINATION .)
	endif()
endif()
		
if(MSVC)
    set(CMAKE_USE_RELATIVE_PATHS ON)
endif()



