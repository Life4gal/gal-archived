# Download and unpack fmtlib at configure time
configure_file(${GAL_CONFIG_PATH}/fmtlib.in fmtlib-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
		RESULT_VARIABLE result
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fmtlib-download)
if (result)
	message(FATAL_ERROR "CMake step for fmtlib failed: ${result}")
endif ()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
		RESULT_VARIABLE result
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fmtlib-download)
if (result)
	message(FATAL_ERROR "Build step for fmtlib failed: ${result}")
endif ()

# Add fmtlib directly to our build.
add_subdirectory(
		${CMAKE_CURRENT_BINARY_DIR}/fmt-src
		${CMAKE_CURRENT_BINARY_DIR}/fmt-build
		EXCLUDE_FROM_ALL
)

set(FMT_PEDANTIC ON)
set(FMT_WERROR ON)
set(FMT_DOC OFF)
set(FMT_INSTALL ON)
set(FMT_TEST OFF)
