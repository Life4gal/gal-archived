project(
		gal-COMPILER
		LANGUAGES CXX
)

set(
		${PROJECT_NAME}_SOURCE

		src/bytecode_builder.cpp
		src/compiler.cpp
)

add_library(
			${PROJECT_NAME}
			STATIC
			${${PROJECT_NAME}_SOURCE}
)

add_library(
		gal::COMPILER
		ALIAS
		${PROJECT_NAME}
)

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

target_link_libraries(
			${PROJECT_NAME}
			PRIVATE
			gal::UTILS
			gal::AST
)

include(${GAL_MODULE_PATH}/config_build_type.cmake)
