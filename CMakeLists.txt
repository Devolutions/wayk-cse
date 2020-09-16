cmake_minimum_required(VERSION 3.0)

project(WaykCse C CXX)

set(WAYK_CSE_LOCAL_LIZARD false CACHE BOOL "Use local lizard distribution under lizard subfolder")

set(WAYK_CSE_NAME "Wayk Now" CACHE STRING "Wayk CSE application name")
set(WAYK_CSE_VENDOR "Devolutions Inc." CACHE STRING "Wayk CSE vendor")
set(WAYK_CSE_COPYRIGHT "Copyright 2020 ${WAYK_CSE_VENDOR} All Rights Reserved." CACHE STRING "Wayk CSE copyright")
set(WAYK_CSE_VERSION "1.0.0")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(conan)
include(WindowsRC)
include(MSVCRuntime)

set(MSVC_RUNTIME "static")
configure_msvc_runtime()

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(CONAN_PROFILE "windows-x86")
else()
	message(FATAL_ERROR "WaykNowPack MUST be built for 32-bit Windows")
endif()

if(CMAKE_HOST_APPLE)
    set(CONAN_BUILD_HOST "Macos")
    set(CONAN_BUILD_ARCH "x86_64")
elseif(CMAKE_HOST_WIN32)
    set(CONAN_BUILD_HOST "Windows")
    set(CONAN_BUILD_ARCH "x86_64")
elseif(CMAKE_HOST_UNIX)
    set(CONAN_BUILD_HOST "Linux")
    set(CONAN_BUILD_ARCH "x86_64")
endif()

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

if (NOT WAYK_CSE_LOCAL_LIZARD)
	conan_check()

	set(CONAN_BUILD_SETTING "never")

	conan_cmake_install(
		CONANFILE conanfile.txt
		PROFILE ${CONAN_PROFILE}
		GENERATORS cmake
		UPDATE
		BUILD never
		SETTINGS "os_build=${CONAN_BUILD_HOST};arch_build=${CONAN_BUILD_ARCH};build_type=${CMAKE_BUILD_TYPE}"
	)

	set(CONAN_DISABLE_CHECK_COMPILER ON)
	conan_load_buildinfo()
	conan_basic_setup(TARGETS NO_OUTPUT_DIRS)
	conan_output_dirs_setup()
else()
	message(STATUS "Using local lizard distribution")
endif()

set(MODULE_NAME "WaykCse")
set(MODULE_PREFIX "WAYK_CSE")

set(OUTPUT_NAME "WaykCse")

set(WAYK_NOW_ICON "${CMAKE_CURRENT_SOURCE_DIR}/res/WaykNow.ico")
set(WAYK_NOW_7ZIP "${CMAKE_CURRENT_SOURCE_DIR}/WaykNow.7z")

message(STATUS "Wayk Now Icon: ${WAYK_NOW_ICON}")
message(STATUS "Wayk Now 7zip: ${WAYK_NOW_7ZIP}")

# Configure resources

SET(CMAKE_RC_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/res/resource.h.in ${CMAKE_CURRENT_BINARY_DIR}/resource.h @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/res/resource.rc.in ${CMAKE_CURRENT_BINARY_DIR}/resource.rc @ONLY)

windows_rc_generate_version_info(
	NAME "${WAYK_CSE_NAME}" TYPE "APP"
	VERSION "${WAYK_CSE_VERSION}"
	FILENAME "WaykCse.exe"
	VENDOR "${WAYK_CSE_VENDOR}"
	COPYRIGHT "${WAYK_CSE_COPYRIGHT}"
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/version.rc")

set(${MODULE_PREFIX}_RESOURCES
	"${CMAKE_CURRENT_BINARY_DIR}/resource.h"
	"${CMAKE_CURRENT_BINARY_DIR}/resource.rc"
	"${CMAKE_CURRENT_BINARY_DIR}/version.rc")
source_group("Resources" FILES ${${MODULE_PREFIX}_RESOURCES})

set(${MODULE_PREFIX}_LIB_SOURCES
	src/cse_utils.c
	src/bundle.c
	src/cse_options.c)
set(${MODULE_PREFIX}_LIB_HEADERS
	include/cse/cse_utils.h
	include/cse/bundle.h
	include/cse/cse_options.h)

set(${MODULE_PREFIX}_SOURCES src/main.c)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_library(
	${MODULE_NAME}-lib
	STATIC
	${${MODULE_PREFIX}_LIB_SOURCES}
	${${MODULE_PREFIX}_RESOURCES}
	${${MODULE_PREFIX}_LIB_HEADERS})

if (WAYK_CSE_LOCAL_LIZARD)
	add_subdirectory(lizard)
	target_link_libraries(${MODULE_NAME}-lib lizard)
	target_include_directories(${MODULE_NAME}-lib PUBLIC lizard/include)
else()
	target_link_libraries(${MODULE_NAME}-lib ${CONAN_TARGETS})
endif()

target_include_directories(
	${MODULE_NAME}-lib
	PUBLIC
	"${CMAKE_CURRENT_LIST_DIR}/include"
	"${CMAKE_CURRENT_BINARY_DIR}")

add_executable(${MODULE_NAME} WIN32 ${${MODULE_PREFIX}_SOURCES} ${${MODULE_PREFIX}_RESOURCES})
target_link_libraries(${MODULE_NAME} PRIVATE ${MODULE_NAME}-lib)

enable_testing()

add_executable(${MODULE_NAME}-test-env-expand tests/env_expand.c)
target_link_libraries(${MODULE_NAME}-test-env-expand PUBLIC ${MODULE_NAME}-lib)
add_test(${MODULE_NAME}-test-env-expand ${MODULE_NAME}-test-env-expand)

set_target_properties(${MODULE_NAME} PROPERTIES OUTPUT_NAME "${OUTPUT_NAME}")
