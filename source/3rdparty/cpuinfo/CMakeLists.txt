set(cpuinfo_src
	src/init.c
	src/api.c
	src/cache.c
	deps/clog/src/clog.c
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i[3-6]86|AMD64|x86(_64)?)$")
	list(APPEND cpuinfo_src
		src/x86/init.c
		src/x86/info.c
		src/x86/vendor.c
		src/x86/uarch.c
		src/x86/name.c
		src/x86/topology.c
		src/x86/isa.c
		src/x86/cache/init.c
		src/x86/cache/descriptor.c
		src/x86/cache/deterministic.c)
	if(LINUX OR ANDROID)
		list(APPEND cpuinfo_src src/x86/linux/init.c src/x86/linux/cpuinfo.c)
	else()
		list(APPEND cpuinfo_src src/x86/windows/init.c)
	endif()
	
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(armv[5-8].*|aarch64|arm64)$")
	list(APPEND cpuinfo_src src/arm/uarch.c src/arm/cache.c)
	
	if(LINUX OR ANDROID)
		list(APPEND cpuinfo_src
			src/arm/linux/init.c
			src/arm/linux/cpuinfo.c
			src/arm/linux/clusters.c
			src/arm/linux/chipset.c
			src/arm/linux/midr.c
			src/arm/linux/hwcap.c)
		
		if(CMAKE_SYSTEM_PROCESSOR MATCHES "^armv[5-8]")
			list(APPEND cpuinfo_src src/arm/linux/aarch32-isa.c)
			
			if(ANDROID AND ANDROID_ABI STREQUAL "armeabi")
				set_source_files_properties(src/arm/linux/aarch32-isa.c PROPERTIES COMPILE_FLAGS -marm)
			endif()
			
		elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
			list(APPEND cpuinfo_src src/arm/linux/aarch64-isa.c)
		endif()
		
	endif()
	
	if(ANDROID)
		list(APPEND cpuinfo_src src/arm/android/properties.c)
	endif()
	
endif()

if(LINUX OR ANDROID)
	list(APPEND cpuinfo_src
		src/linux/smallfile.c
		src/linux/multiline.c
		src/linux/cpulist.c
		src/linux/processors.c)
endif()

if (LINUX)
	set_source_files_properties(${cpuinfo_src} PROPERTIES COMPILE_FLAGS "-D_GNU_SOURCE=1")
endif()

include_directories(include src deps/clog/include)

add_library(cpuinfo STATIC ${cpuinfo_src})

if (MINGW)
	if (CLANG)
		target_compile_options(cpuinfo PRIVATE -Wno-unused-variable -Wno-implicit-function-declaration)
	else()
		target_compile_options(cpuinfo PRIVATE -Wno-maybe-uninitialized -Wno-unused-variable -Wno-implicit-function-declaration -Wno-format -Wno-format-extra-args)
	endif()
elseif (LINUX)
	if (CLANG)		
	else()
		target_compile_options(cpuinfo PRIVATE -Wno-unused-result)
	endif()
elseif (MSVC)
	if (CLANG)
		target_compile_options(cpuinfo PRIVATE -Wno-unused-variable -Wno-deprecated-declarations -Wno-implicit-function-declaration)
	else()		
	endif()
elseif(ANDROID)
	target_compile_options(cpuinfo PRIVATE -Wno-implicit-function-declaration)
endif()


target_include_directories(cpuinfo PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")



