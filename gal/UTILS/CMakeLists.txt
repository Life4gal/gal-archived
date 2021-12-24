project(
		gal-UTILS
		LANGUAGES CXX
)

set(
		${PROJECT_NAME}_SOURCE

		src/assert.cpp
		src/point.cpp
)

message("${PROJECT_NAME} info: build ${PROJECT_NAME} as a shared module.")

add_library(
		${PROJECT_NAME}
		# SHARED
		# todo: it should be shared
		STATIC
		${${PROJECT_NAME}_SOURCE}
)

add_library(
		gal::UTILS
		ALIAS
		${PROJECT_NAME}
)

if (MSVC)
	# todo: Maybe we don¡¯t need to export all symbols
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif ()

target_include_directories(
		${PROJECT_NAME}
		PUBLIC
		${PROJECT_SOURCE_DIR}/include
)

if(MSVC)
	target_compile_features(
			${PROJECT_NAME}
			PRIVATE
			cxx_std_23
	)
else()
	target_compile_features(
			${PROJECT_NAME}
			PRIVATE
			cxx_std_20
	)
endif(MSVC)

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
	message("${PROJECT_NAME} info: build fmtlib as a public module of ${PROJECT_NAME}.")
	target_link_libraries(
			${PROJECT_NAME}
			PUBLIC
			fmt
	)
endif (${COMPILER_SUPPORT_FORMAT})

include(${GAL_MODULE_PATH}/config_build_type.cmake)
