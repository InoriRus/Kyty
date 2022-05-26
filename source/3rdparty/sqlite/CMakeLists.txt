file(GLOB sqlite_src    	
	"src/altered_sqlite3secure.c"
)


if (MINGW)
	if (CLANG)
		if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0.0)
			set_source_files_properties(${sqlite_src} PROPERTIES COMPILE_FLAGS "-Wno-unused-but-set-variable -Wno-unused-function -Wno-unknown-pragmas -Wno-unused-variable -Wno-sometimes-uninitialized")
		else()
			set_source_files_properties(${sqlite_src} PROPERTIES COMPILE_FLAGS "-Wno-unused-function -Wno-unknown-pragmas -Wno-unused-variable -Wno-sometimes-uninitialized")
		endif()		
	else()
		set_source_files_properties(${sqlite_src} PROPERTIES COMPILE_FLAGS "-Wno-misleading-indentation -Wno-return-local-addr -Wno-unused-but-set-variable -Wno-unused-function -Wno-unknown-pragmas -Wno-unused-variable -Wno-maybe-uninitialized -Wno-array-bounds")
	endif()
endif()

if (MSVC)
	if (CLANG)
		set_source_files_properties(${sqlite_src} PROPERTIES COMPILE_FLAGS "-D_CRT_SECURE_NO_WARNINGS -Wno-builtin-macro-redefined -Wno-unused-function -Wno-unknown-pragmas -Wno-unused-variable -Wno-sometimes-uninitialized")
	else()
		set_source_files_properties(${sqlite_src} PROPERTIES COMPILE_FLAGS "-D_CRT_SECURE_NO_WARNINGS /wd4267 /wd4101")
	endif()
endif()

#add_library(sqlite STATIC ${sqlite_src})
add_library(sqlite_obj OBJECT ${sqlite_src})
add_library(sqlite STATIC $<TARGET_OBJECTS:sqlite_obj>)

target_include_directories(sqlite PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_include_directories(sqlite_obj PRIVATE $<TARGET_PROPERTY:sqlite,INTERFACE_INCLUDE_DIRECTORIES>)

