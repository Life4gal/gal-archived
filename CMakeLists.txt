cmake_minimum_required(VERSION 3.13)

set(
		GAL_MAJOR_VERSION
		0
)

set(
		GAL_MINOR_VERSION
		0
)

set(
		GAL_PATCH_VERSION
		1
)

set(
		GAL_VERSION
		${GAL_MAJOR_VERSION}.${GAL_MINOR_VERSION}.${GAL_PATCH_VERSION}
)

project(
		gal-lang
		VERSION ${GAL_VERSION}
		DESCRIPTION "gal language"
		HOMEPAGE_URL "https://github.com/Life4gal/gal"
		LANGUAGES CXX
)

set(GAL_GLOBAL_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(GAL_CONFIG_PATH ${GAL_GLOBAL_SOURCE_PATH}/config)
set(GAL_MODULE_PATH ${GAL_GLOBAL_SOURCE_PATH}/cmake)
set(GAL_3RDPARTY_PATH ${GAL_GLOBAL_SOURCE_PATH}/3rd-party)

option(BUILD_GAL_AS_SHARED "build ${PROJECT_NAME} as a shared module.(otherwise, as a static module)" OFF)
option(BUILD_GAL_TEST_CASES "build ${PROJECT_NAME} test cases" ON)
option(BUILD_GAL_FMTLIB_PUBLIC "let fmtlib as a public module of ${PROJECT_NAME}.(if std::format is not support)" ON)

add_subdirectory(gal)

if (${BUILD_GAL_TEST_CASES})
	message("${PROJECT_NAME} info: build ${PROJECT_NAME}'s test cases.")
	add_subdirectory(test)
else ()
	message("${PROJECT_NAME} info: drop ${PROJECT_NAME}'s test cases.")
endif (${BUILD_GAL_TEST_CASES})
