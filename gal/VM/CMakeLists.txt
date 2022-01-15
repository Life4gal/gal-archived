project(
		gal-VM
		LANGUAGES CXX
)

set(
		${PROJECT_NAME}_SOURCE

		src/gal.cpp
		src/state.cpp
		src/object.cpp
		src/memory.cpp
)

add_library(
			${PROJECT_NAME}
			STATIC
			${${PROJECT_NAME}_SOURCE}
)

add_library(
		gal::VM
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
			gal::CONFIG
			gal::UTILS
)

include(${GAL_MODULE_PATH}/config_build_type.cmake)
