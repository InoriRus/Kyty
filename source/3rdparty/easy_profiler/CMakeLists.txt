file(GLOB easy_profiler_src    	
	src/base_block_descriptor.cpp
    src/block.cpp
    src/block_descriptor.cpp
    src/easy_socket.cpp
    src/event_trace_win.cpp
    src/nonscoped_block.cpp
    src/profile_manager.cpp
    src/profiler.cpp
    src/reader.cpp
    src/serialized_block.cpp
    src/thread_storage.cpp
    src/writer.cpp
)


if (MINGW)
	if (CLANG)
		set_source_files_properties(${easy_profiler_src} PROPERTIES COMPILE_FLAGS "-DWINVER=0x0601 -D_WIN32_WINNT=0x0601 -DBUILD_WITH_EASY_PROFILER -DEASY_PROFILER_STATIC -DEASY_DEFAULT_PORT=28077 -DEASY_PROFILER_VERSION_MAJOR=2 -DEASY_PROFILER_VERSION_MINOR=1 -DEASY_PROFILER_VERSION_PATCH=0 -DSTRSAFE_NO_DEPRECATE -Wno-unused-variable -Wno-writable-strings -Wno-braced-scalar-init -Wno-defaulted-function-deleted -Wno-unknown-pragmas -Wno-reorder-ctor -femulated-tls")
	else()
		set_source_files_properties(${easy_profiler_src} PROPERTIES COMPILE_FLAGS "-DWINVER=0x0601 -D_WIN32_WINNT=0x0601 -DBUILD_WITH_EASY_PROFILER -DEASY_PROFILER_STATIC -DEASY_DEFAULT_PORT=28077 -DEASY_PROFILER_VERSION_MAJOR=2 -DEASY_PROFILER_VERSION_MINOR=1 -DEASY_PROFILER_VERSION_PATCH=0 -DSTRSAFE_NO_DEPRECATE -Wno-reorder-ctor -Wno-reorder -Wno-unknown-pragmas -Wno-format -Wno-class-memaccess")
	endif()
endif()

if (MSVC)
	if (CLANG)
		set_source_files_properties(${easy_profiler_src} PROPERTIES COMPILE_FLAGS "-DBUILD_WITH_EASY_PROFILER -DEASY_PROFILER_STATIC -DEASY_DEFAULT_PORT=28077 -DEASY_PROFILER_VERSION_MAJOR=2 -DEASY_PROFILER_VERSION_MINOR=1 -DEASY_PROFILER_VERSION_PATCH=0 -DSTRSAFE_NO_DEPRECATE -Wno-reorder-ctor -Wno-defaulted-function-deleted -Wno-braced-scalar-init -Wno-writable-strings -D_WINSOCK_DEPRECATED_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -Wno-format")
	else()
		set_source_files_properties(${easy_profiler_src} PROPERTIES COMPILE_FLAGS "-DEASY_PROFILER_STATIC")
	endif()
endif()

add_library(easy_profiler_obj OBJECT ${easy_profiler_src})
add_library(easy_profiler STATIC $<TARGET_OBJECTS:easy_profiler_obj>)

target_include_directories(easy_profiler PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_include_directories(easy_profiler_obj PRIVATE $<TARGET_PROPERTY:easy_profiler,INTERFACE_INCLUDE_DIRECTORIES>)

