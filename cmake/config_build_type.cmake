if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message("${PROJECT_NAME} info: Setting build type to 'RelWithDebInfo' as none was specified.")
	set(
			CMAKE_BUILD_TYPE RelWithDebInfo
			CACHE STRING "Choose the type of build." FORCE
	)
	set_property(
			CACHE
			CMAKE_BUILD_TYPE
			PROPERTY STRINGS 
			"Debug" "Release" "MinSizeRel" "RelWithDebInfo"
	)
else()
	message("${PROJECT_NAME} info: Current build type is: ${CMAKE_BUILD_TYPE}")
endif(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_options(
			${PROJECT_NAME}
			PRIVATE
			$<$<CXX_COMPILER_ID:MSVC>:/MDd /Zi /Ob0 /Od /RTC1 /W4 /WX>
			$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O0 -g -Wall -Wextra -Wpedantic -Werror>
	)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
	target_compile_options(
			${PROJECT_NAME}
			PRIVATE
			$<$<CXX_COMPILER_ID:MSVC>:/MD /O2 /Ob2 /DNDEBUG /W4 /WX>
			$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror>
	)
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
	target_compile_options(
			${PROJECT_NAME}
			PRIVATE
			$<$<CXX_COMPILER_ID:MSVC>:/MD /O1 /Ob1 /DNDEBUG /W4 /WX>
			$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Os -DNDEBUG -Wall -Wextra -Wpedantic -Werror>
	)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
	target_compile_options(
			${PROJECT_NAME}
			PRIVATE
			$<$<CXX_COMPILER_ID:MSVC>:/MD /Zi /O2 /Ob1 /DNDEBUG /W4 /WX>
			$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O2 -g -DNDEBUG -Wall -Wextra -Wpedantic -Werror>
	)
else()
	message(FATAL_ERROR "${PROJECT_NAME} info: Unsupported build type")
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")
