# Download and unpack range-v3 at configure time
configure_file(${GAL_CONFIG_PATH}/range-v3.in range-v3-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
		RESULT_VARIABLE result
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/range-v3-download)
if (result)
	message(FATAL_ERROR "CMake step for range-v3 failed: ${result}")
endif ()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
		RESULT_VARIABLE result
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/range-v3-download)
if (result)
	message(FATAL_ERROR "Build step for range-v3 failed: ${result}")
endif ()

# Add range-v3 directly to our build.
add_subdirectory(
		${CMAKE_CURRENT_BINARY_DIR}/range-v3-src
		${CMAKE_CURRENT_BINARY_DIR}/range-v3-build
		EXCLUDE_FROM_ALL
)

set(RANGES_BUILD_CALENDAR_EXAMPLE OFF)
set(RANGES_ASAN OFF)
set(RANGES_MSAN OFF)
set(RANGES_ASSERTIONS ON)
set(RANGES_DEBUG_INFO ON)
set(RANGES_MODULES ON)
set(RANGES_NATIVE OFF)
set(RANGES_VERBOSE_BUILD OFF)
set(RANGES_LLVM_POLLY OFF)
set(RANGES_ENABLE_WERROR ON)
set(RANGES_PREFER_REAL_CONCEPTS ON)
set(RANGES_DEEP_STL_INTEGRATION OFF)
set(RANGE_V3_HEADER_CHECKS OFF)
set(RANGE_V3_PERF OFF)
