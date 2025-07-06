#
#  CMakeLists.txt
#
#  Copyright 2019 死体
#
#  This file is part of GS2Emu.
#
#  GS2Emu is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  GS2Emu is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with GS2Emu.  If not, see <http://www.gnu.org/licenses/>.
#

cmake_minimum_required(VERSION 3.24)

# Add custom CMake modules path for external dependencies.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/dependencies")

# Includes.
include(FetchContent)
include(utility)

# Set build type.
set(default_build_type "MinSizeRel")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
	set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
		STRING "Choose the type of build." FORCE)

	# Set the possible values of build type for cmake-gui
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
		"Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Enable Hot Reload for MSVC compilers if supported.
if(POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)	# CMP0141: MSVC debug information format.
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Policies.
cmake_policy(SET CMP0054 NEW)	# CMP0054: Only interpret if() arguments as variables or keywords when unquoted.
cmake_policy(SET CMP0074 NEW)	# CMP0074: find_package uses PackageName_ROOT variables.
cmake_policy(SET CMP0077 NEW)	# CMP0077: option() honors normal variables.
cmake_policy(SET CMP0091 NEW)	# CMP0091: MSVC runtime library flags are selected by an abstraction.
cmake_policy(SET CMP0177 NEW)	# CMP0177: install() DESTINATION paths are normalized.
cmake_policy(SET CMP0144 NEW)	# CMP0144: Also search <PACKAGENAME>_ROOT as well as <PackageName>_ROOT.

# Get submodules.
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
	# Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if (GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if (NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

# vcpkg configuration.
set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "vcpkg triplet")
set(VCPKG_INSTALLED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/vcpkg_installed")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# GS2 project.
project(GS2Emu VERSION 4.0.0 DESCRIPTION "Graal Online v1.411 to v6.037 compatible server" LANGUAGES C CXX)

# Lowercase project name for binaries and packaging.
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

# Adhere to GNU filesystem layout conventions.
include(GNUInstallDirs)

# Debug build postfix.
set(CMAKE_DEBUG_POSTFIX _d)
set(BUILD_SHARED_LIBS OFF)

# Directories.
set(CMAKE_BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/lib")
set(CPACK_PACKAGE_DIRECTORY ${PROJECT_SOURCE_DIR}/dist)

# Dependency and FetchContent setup.
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
set(DEPENDENCY_DIRECTORY "${PROJECT_SOURCE_DIR}/dependencies")
set(FETCHCONTENT_BASE_DIR "${DEPENDENCY_DIRECTORY}/fc")
set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE ALWAYS)

# System defines.
if(MSVC)
	add_compile_definitions(UNICODE)
endif()

# Generate the IConfig.h file.
generate_iconfig()

# Static runtime.
option(STATIC "Compile as a static runtime." ON)
if(STATIC)
	message("Compile as static runtime")
	add_definitions(-DSTATICLIB)
	set(BUILD_SHARED_LIBS FALSE CACHE BOOL "-" FORCE)
else()
	message("Don't compile as a static runtime")
	set(BUILD_SHARED_LIBS TRUE CACHE BOOL "-" FORCE)
endif()

# Dependency: Threads.
if(NOT WIN32)
	# Prefer static linkage.
	if(STATIC)
		set(SUFFIXES_ORIG ${CMAKE_FIND_LIBRARY_SUFFIXES})
		set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
	endif()

	find_package(Threads REQUIRED)

	# Restore library suffixes.
	if(STATIC)
		set(CMAKE_FIND_LIBRARY_SUFFIXES ${SUFFIXES_ORIG})
	endif()
endif()

# Fetch.
add_subdirectory(${PROJECT_SOURCE_DIR}/dependencies/gs2compiler EXCLUDE_FROM_ALL)

# Server.
add_subdirectory(${PROJECT_SOURCE_DIR}/bin)
add_subdirectory(${PROJECT_SOURCE_DIR}/server)

###-----------------------###

# Packaging.
if(APPLE)
	set(CPACK_GENERATOR DragNDrop)
	set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME} ${VER_FULL}")
	set(CPACK_DMG_DS_STORE_SETUP_SCRIPT ${PROJECT_SOURCE_DIR}/resources/packaging/osx/DMGSetup.scpt)
	set(CPACK_DMG_BACKGROUND_IMAGE ${PROJECT_SOURCE_DIR}/resources/packaging/osx/DMGBackground.tif)
elseif(WIN32)
	set(CPACK_GENERATOR "ZIP")
else()
	set(CPACK_GENERATOR TGZ)
endif()

# More packaging.
set(CPACK_PACKAGE_NAME ${PROJECT_NAME_LOWER})
set(CPACK_PACKAGE_VENDOR "OpenGraal Team")
set(CPACK_PACKAGE_CONTACT "opengraal")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${VER_CPACK}")
set(CPACK_PACKAGE_VERSION_MAJOR ${VER_X})
set(CPACK_PACKAGE_VERSION_MINOR ${VER_Y})
set(CPACK_PACKAGE_VERSION_PATCH ${VER_Z}${VER_EXTRA})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_DESCRIPTION_FILE ${PROJECT_SOURCE_DIR}/README.md)
set(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/README.md)
set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/LICENSE.md)
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${VER_CPACK}-src")
set(CPACK_SOURCE_IGNORE_FILES "/build/;/.bzr/;~$;${CPACK_SOURCE_IGNORE_FILES}")
include(CPack)
