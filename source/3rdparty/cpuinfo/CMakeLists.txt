file(GLOB cpuinfo_src
  src/init.c
  src/api.c
  src/cache.c
  src/x86/init.c
  src/x86/info.c
  src/x86/vendor.c
  src/x86/uarch.c
  src/x86/name.c
  src/x86/topology.c
  src/x86/isa.c
  src/x86/cache/init.c
  src/x86/cache/descriptor.c
  src/x86/cache/deterministic.c
  src/x86/windows/init.c
  deps/clog/src/clog.c
)

if (MINGW)
	if (CLANG)
		set_source_files_properties(${cpuinfo_src} PROPERTIES COMPILE_FLAGS "-Wno-unused-variable -Wno-implicit-function-declaration")
	else()
		set_source_files_properties(${cpuinfo_src} PROPERTIES COMPILE_FLAGS "-Wno-unused-variable -Wno-implicit-function-declaration -Wno-format -Wno-format-extra-args")
	endif()
endif()

if (MSVC)
	if (CLANG)
		set_source_files_properties(${cpuinfo_src} PROPERTIES COMPILE_FLAGS "-Wno-unused-variable -Wno-deprecated-declarations -Wno-implicit-function-declaration")
	else()
		set_source_files_properties(${cpuinfo_src} PROPERTIES COMPILE_FLAGS "")
	endif()
endif()

include_directories(include src deps/clog/include)

#add_library(cpuinfo STATIC ${cpuinfo_src})
add_library(cpuinfo_obj OBJECT ${cpuinfo_src})
add_library(cpuinfo STATIC $<TARGET_OBJECTS:cpuinfo_obj>)


target_include_directories(cpuinfo PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_include_directories(cpuinfo_obj PRIVATE $<TARGET_PROPERTY:cpuinfo,INTERFACE_INCLUDE_DIRECTORIES>)

