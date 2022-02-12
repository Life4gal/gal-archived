project(
		gal-CORE
		LANGUAGES CXX
)

add_library(
	${PROJECT_NAME}
	INTERFACE
)

target_include_directories(
	${PROJECT_NAME}
	INTERFACE 
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(
		gal::CORE
		ALIAS
		${PROJECT_NAME}
)

if(MSVC)
	target_compile_features(
			${PROJECT_NAME}
			INTERFACE 
			cxx_std_23
	)
else()
	target_compile_features(
			${PROJECT_NAME}
			INTERFACE 
			cxx_std_20
	)
endif(MSVC)

target_link_libraries(
			${PROJECT_NAME}
			INTERFACE 
			gal::UTILS
)

include(${GAL_MODULE_PATH}/config_build_type.cmake)
BuildAsInterface()
