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

macro(BuildAsPublic)
	target_compile_options(
		${PROJECT_NAME}
		PUBLIC
		
		$<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
		$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>

		$<$<NOT:$<CONFIG:Debug>>: $<$<CXX_COMPILER_ID:MSVC>:/DNDEBUG> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-DNDEBUG>>

		$<$<CONFIG:Debug>: $<$<CXX_COMPILER_ID:MSVC>:/MDd /Zi /Ob0 /Od /RTC1> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O0 -g>>
		$<$<CONFIG:Release>: $<$<CXX_COMPILER_ID:MSVC>:/MD /O2 /Ob2> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O3>>
		$<$<CONFIG:RelWithDebInfo>: $<$<CXX_COMPILER_ID:MSVC>:/MD /Zi /O2 /Ob1> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O2 -g>>
		$<$<CONFIG:MinSizeRel>: $<$<CXX_COMPILER_ID:MSVC>:/MD /O1 /Ob1> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Os>>
	)
endmacro(BuildAsPublic)

macro(BuildAsPrivate)
	target_compile_options(
		${PROJECT_NAME}
		PRIVATE
		
		$<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
		$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>

		$<$<NOT:$<CONFIG:Debug>>: $<$<CXX_COMPILER_ID:MSVC>:/DNDEBUG> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-DNDEBUG>>

		$<$<CONFIG:Debug>: $<$<CXX_COMPILER_ID:MSVC>:/MDd /Zi /Ob0 /Od /RTC1> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O0 -g>>
		$<$<CONFIG:Release>: $<$<CXX_COMPILER_ID:MSVC>:/MD /O2 /Ob2> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O3>>
		$<$<CONFIG:RelWithDebInfo>: $<$<CXX_COMPILER_ID:MSVC>:/MD /Zi /O2 /Ob1> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O2 -g>>
		$<$<CONFIG:MinSizeRel>: $<$<CXX_COMPILER_ID:MSVC>:/MD /O1 /Ob1> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Os>>
	)
endmacro(BuildAsPrivate)

macro(BuildAsInterface)
	target_compile_options(
		${PROJECT_NAME}
		INTERFACE
		
		$<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
		$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>

		$<$<NOT:$<CONFIG:Debug>>: $<$<CXX_COMPILER_ID:MSVC>:/DNDEBUG> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-DNDEBUG>>

		$<$<CONFIG:Debug>: $<$<CXX_COMPILER_ID:MSVC>:/MDd /Zi /Ob0 /Od /RTC1> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O0 -g>>
		$<$<CONFIG:Release>: $<$<CXX_COMPILER_ID:MSVC>:/MD /O2 /Ob2> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O3>>
		$<$<CONFIG:RelWithDebInfo>: $<$<CXX_COMPILER_ID:MSVC>:/MD /Zi /O2 /Ob1> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O2 -g>>
		$<$<CONFIG:MinSizeRel>: $<$<CXX_COMPILER_ID:MSVC>:/MD /O1 /Ob1> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Os>>
	)
endmacro(BuildAsInterface)
