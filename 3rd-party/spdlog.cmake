# Download and unpack spdlog at configure time
configure_file(${GAL_CONFIG_PATH}/spdlog.in spdlog-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
		RESULT_VARIABLE result
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/spdlog-download)
if (result)
	message(FATAL_ERROR "CMake step for spdlog failed: ${result}")
endif ()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
		RESULT_VARIABLE result
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/spdlog-download)
if (result)
	message(FATAL_ERROR "Build step for spdlog failed: ${result}")
endif ()

# Add spdlog directly to our build.
add_subdirectory(
		${CMAKE_CURRENT_BINARY_DIR}/spdlog-src
		${CMAKE_CURRENT_BINARY_DIR}/spdlog-build
		EXCLUDE_FROM_ALL
)

set(SPDLOG_BUILD_EXAMPLE  OFF)
set(SPDLOG_BUILD_TESTS  OFF)
set(SPDLOG_BUILD_TESTS_HO  OFF)
set(SPDLOG_INSTALL  OFF)
set(SPDLOG_FMT_EXTERNAL  ON)
