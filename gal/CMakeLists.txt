project(
		gal
		LANGUAGES CXX
)

set(
		${PROJECT_NAME}_SOURCE
		src/ast_node.cpp

		src/gal.cpp
)

add_executable(
		${PROJECT_NAME}
		${${PROJECT_NAME}_SOURCE}
)

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

include(${GAL_MODULE_PATH}/config_build_type.cmake)
