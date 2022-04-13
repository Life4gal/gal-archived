project(
		gal-EXE
		LANGUAGES CXX
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 20)

set(
		${PROJECT_NAME}_SOURCE
		
		src/main.cpp
)

add_executable(
		${PROJECT_NAME}
		${${PROJECT_NAME}_SOURCE}
)

target_compile_options(
	${PROJECT_NAME}
	PRIVATE

	$<$<CXX_COMPILER_ID:MSVC>:/bigobj>
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wa,-mbig-obj>
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
		# gal::UTILS
		gal::CORE
)

include(${GAL_MODULE_PATH}/config_build_type.cmake)
BuildAsPrivate()
