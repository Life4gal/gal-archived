project(
		gal
		LANGUAGES CXX
)

set(
		${PROJECT_NAME}_SOURCE

		include/allocator.hpp
		include/gal.hpp

		src/utils/assert.cpp
)

if (${BUILD_GAL_AS_SHARED})
	message("${PROJECT_NAME} info: build ${PROJECT_NAME} as a shared module.")
	add_library(
			${PROJECT_NAME}
			SHARED
			${${PROJECT_NAME}_SOURCE}
	)
	if (MSVC)
		# todo: Maybe we don’t need to export all symbols
		set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
	endif ()
else ()
	message("${PROJECT_NAME} info: build ${PROJECT_NAME} as a static module.")
	add_library(
			${PROJECT_NAME}
			STATIC
			${${PROJECT_NAME}_SOURCE}
	)
endif (${BUILD_GAL_AS_SHARED})

target_include_directories(
		${PROJECT_NAME}
		PUBLIC
		${PROJECT_SOURCE_DIR}/include
)

target_compile_features(
		${PROJECT_NAME}
		PRIVATE
		cxx_std_20
)

include(CheckCXXSourceCompiles)
CHECK_CXX_SOURCE_COMPILES(
		"
		#include <format>

		int main()
		{
			std::cout << std::format(\"hello {}!\", \"world\");
		}
	"
		COMPILER_SUPPORT_FORMAT
)

if (${COMPILER_SUPPORT_FORMAT})
	message("${PROJECT_NAME} info: current compiler ${CMAKE_CXX_COMPILER} support std::format.")
else ()
	message("${PROJECT_NAME} info: current compiler ${CMAKE_CXX_COMPILER} not support std::format, use fmtlib instead now.")
	target_compile_definitions(
			${PROJECT_NAME}
			PUBLIC # todo: should be PRIVATE
			GAL_FMT_NOT_SUPPORT
	)
	include(${GAL_3RDPARTY_PATH}/fmtlib.cmake)
	if (${BUILD_GAL_FMTLIB_PUBLIC})
		message("${PROJECT_NAME} info: build fmtlib as a public module of ${PROJECT_NAME}.")
		target_link_libraries(
				${PROJECT_NAME}
				PUBLIC
				fmt
		)
	else ()
		message("${PROJECT_NAME} info: build fmtlib as a private module of ${PROJECT_NAME}.")
		target_link_libraries(
				${PROJECT_NAME}
				PRIVATE
				fmt
		)
	endif (${BUILD_GAL_FMTLIB_PUBLIC})
endif (${COMPILER_SUPPORT_FORMAT})

include(${GAL_MODULE_PATH}/config_build_type.cmake)
