cmake_minimum_required(VERSION 3.20)

set(
		GAL_LANG_MAJOR_VERSION
		0
)

set(
		GAL_LANG_MINOR_VERSION
		6
)

set(
		GAL_LANG_PATCH_VERSION
		0
)

set(GAL_LANG_VERSION ${GAL_LANG_MAJOR_VERSION}.${GAL_LANG_MINOR_VERSION}.${GAL_LANG_PATCH_VERSION})

project(
		gal-lang
		VERSION ${GAL_LANG_VERSION}
		DESCRIPTION "gal language"
		HOMEPAGE_URL "https://github.com/Life4gal/gal"
		LANGUAGES CXX
)

set(GAL_LANG_COMPILER_NAME ${CMAKE_CXX_COMPILER_ID})
set(GAL_LANG_COMPILER_VERSION ${GAL_LANG_COMPILER_NAME}.${CMAKE_CXX_COMPILER_VERSION})

set(GAL_GLOBAL_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(GAL_CONFIG_PATH ${GAL_GLOBAL_SOURCE_PATH}/config)
set(GAL_MODULE_PATH ${GAL_GLOBAL_SOURCE_PATH}/cmake)
set(GAL_3RDPARTY_PATH ${GAL_GLOBAL_SOURCE_PATH}/3rd-party)

option(BUILD_GAL_AS_SHARED "build ${PROJECT_NAME} as a shared module.(otherwise, as a static module)" OFF)
option(BUILD_GAL_TEST_CASES "build ${PROJECT_NAME} test cases" ON)
option(BUILD_GAL_EXE_EXAMPLE "build ${PROJECT_NAME} exe example" ON)

add_subdirectory(gal)

# todo: move gal utils to other place
if (${BUILD_GAL_TEST_CASES})
	message("${PROJECT_NAME} info: build ${PROJECT_NAME}'s test cases.")
	enable_testing()
	add_subdirectory(test)
else ()
	message("${PROJECT_NAME} info: drop ${PROJECT_NAME}'s test cases.")
endif (${BUILD_GAL_TEST_CASES})

if (${BUILD_GAL_EXE_EXAMPLE})
	add_subdirectory(gal_exe)
else()
	message("${PROJECT_NAME} info: drop ${PROJECT_NAME}'s exe example.")
endif (${BUILD_GAL_EXE_EXAMPLE})
