
find_package(Qt5 COMPONENTS Widgets LinguistTools REQUIRED)

set(CMAKE_AUTOUIC_SEARCH_PATHS forms)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB launcher_src
	"include/*.h"
	"src/*.cpp"
)

file(GLOB launcher_forms
	"forms/*.ui"
)

file(GLOB launcher_res
	"res/*.qrc"
)

set(launcher_ts "ts/launcher_ru_RU.ts")

add_executable(launcher WIN32
	${launcher_src}
	${launcher_forms}
	${launcher_res}
	${launcher_ts}
)


target_link_libraries(launcher Qt5::Widgets)
target_link_libraries(launcher psapi)
target_link_libraries(launcher setupapi)
if (CLANG AND NOT MSVC)
target_link_libraries(launcher pthread)
endif()
#if (CLANG AND MSVC)
#target_link_libraries(launcher winpthread)
#endif()
target_include_directories(launcher PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/src")
set_target_properties(launcher PROPERTIES AUTOUIC ON AUTOMOC ON AUTORCC ON)

if(MINGW)
	set_target_properties(launcher PROPERTIES LINK_FLAGS "${KYTY_LD_OPTIONS}")
endif()

add_dependencies(launcher KytyGitVersion)  

set(launcher_name "launcher")

set_target_properties(launcher PROPERTIES OUTPUT_NAME ${launcher_name})

install(TARGETS launcher DESTINATION .)

if(WIN32)
	find_program(QT_WINDEPLOYQT NAMES windeployqt PATHS "${Qt5_DIR}/../../../bin")

	if(NOT QT_WINDEPLOYQT)
		message(FATAL_ERROR "Could not find windeployqt")
	endif()

	install(CODE "
			execute_process(
				COMMAND ${QT_WINDEPLOYQT} \"\${CMAKE_INSTALL_PREFIX}/${launcher_name}.exe\"
			--no-svg
			--no-angle
			--no-opengl
			--no-opengl-sw
			--no-system-d3d-compiler
					--no-translations
			)
	")
endif()



qt5_create_translation(QM_FILES ${launcher_src} ${launcher_forms} ${launcher_ts} OPTIONS -I ${CMAKE_SOURCE_DIR}/launcher/include)

set(tidy_dirs "${CMAKE_SOURCE_DIR}/launcher/include") 
set(iwyu_maps "${CMAKE_SOURCE_DIR}/launcher/utils/qt5_15.imp")

get_property(inc_headers TARGET launcher PROPERTY INCLUDE_DIRECTORIES)

list(APPEND inc_headers
	${CMAKE_SOURCE_DIR}/launcher
	${CMAKE_BINARY_DIR}/launcher/launcher_autogen/include
	${Qt5_DIR}/../../../include
	${Qt5_DIR}/../../../include/QtWidgets
	${Qt5_DIR}/../../../include/QtCore
	${Qt5_DIR}/../../../include/QtGui
)

include_what_you_use_with_mappings(launcher "${inc_headers}" "${iwyu_maps}")

clang_tidy_check(launcher "" "${tidy_dirs}" "${inc_headers}")
