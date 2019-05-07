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

cmake_minimum_required(VERSION 2.8.5)
project(GS2Emu C CXX)

set(CMAKE_DEBUG_POSTFIX _d)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# Second, for multi-config builds (e.g. msvc)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
	string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
	set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_SOURCE_DIR}/lib )
	set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_SOURCE_DIR}/lib )
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_SOURCE_DIR}/bin )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

link_directories(${PROJECT_SOURCE_DIR}/lib)

# Adhere to GNU filesystem layout conventions
include(GNUInstallDirs)

# Lowercase project name for binaries and packaging
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

# Additional CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

# Version number in format X.YY.ZZ
set(VER_X 2)
set(VER_Y 6)
set(VER_Z x)
option(VER_EXTRA "Extra version string" " GIT NPC Server")
set(VER_FULL "${VER_X}.${VER_Y}.${VER_Z}${VER_EXTRA}")

STRING(REGEX REPLACE " " "-" VER_CPACK ${VER_FULL})

# Generate version header from the above
configure_file(
	${PROJECT_SOURCE_DIR}/server/include/IConfig.h.in
	${PROJECT_BINARY_DIR}/server/include/IConfig.h
)

# Force SDL if requested
option(NOSTATIC "Don't compile as a static runtime." OFF)
if(NOSTATIC)
	message("Don't compile as a static runtime.")
	set(BUILD_SHARED_LIBS TRUE CACHE BOOL "-" FORCE)
else()
	message("Compile as static runtime.")
	add_definitions(-DSTATICLIB)
	set(BUILD_SHARED_LIBS FALSE CACHE BOOL "-" FORCE)
endif()

option(V8NPCSERVER "Compile built-in V8 NPC-Server." OFF)
if(V8NPCSERVER)
	message("Enabling built-in V8 NPC-Server.")
	add_definitions(-DV8NPCSERVER)
	find_package(V8 REQUIRED)
else()
	message("Disabling built-in V8 NPC-Server.")
endif()

option(NOUPNP "Don't compile with UPNP support" OFF)
if(NOT NOUPNP)
	message("Enabling UPNP support.")
	add_definitions(-DUPNP)
else()
	message("Disabling UPNP support.")
endif()

# Packaging
if(APPLE)
	set(CPACK_GENERATOR DragNDrop)
	set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME} ${VER_FULL}")
	set(
		CPACK_DMG_DS_STORE_SETUP_SCRIPT
		${PROJECT_SOURCE_DIR}/resources/packaging/osx/DMGSetup.scpt
	)
	set(
		CPACK_DMG_BACKGROUND_IMAGE
		${PROJECT_SOURCE_DIR}/resources/packaging/osx/DMGBackground.tif
	)
elseif(WIN32)
	set(CPACK_GENERATOR ZIP)
elseif(AMIGA)
	set(CPACK_GENERATOR LHA)
else()
	set(CPACK_GENERATOR TGZ)
endif()

set(CPACK_PACKAGE_NAME ${PROJECT_NAME_LOWER})
set(CPACK_PACKAGE_VENDOR "Graal Reborn Team")
set(CPACK_PACKAGE_CONTACT "graal.in")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${VER_CPACK}")
set(CPACK_PACKAGE_VERSION_MAJOR ${VER_X})
set(CPACK_PACKAGE_VERSION_MINOR ${VER_Y})
set(CPACK_PACKAGE_VERSION_PATCH ${VER_Z}${VER_EXTRA})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Graal Online v1.411 to v5.002 compatible server")
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${VER_CPACK}-src")
set(CPACK_SOURCE_IGNORE_FILES "/build/;/.bzr/;~$;${CPACK_SOURCE_IGNORE_FILES}")
include(CPack)

if(APPLE)
	# Warn if deployment target isn't set to Lion
	if(NOT CMAKE_OSX_DEPLOYMENT_TARGET MATCHES "10.7")
		message(WARNING
			"Your deployment target is either unset or not set to \"10.7\", "
			"which means that the binaries produced may not run on earlier "
			"versions of macOS.\n"
			"Please re-run CMake with '-DCMAKE_OSX_DEPLOYMENT_TARGET=10.7' "
			"or change the variable in the CMake GUI to target Lion and newer."
		)
	endif()

	# Set variables for generating the Info.plist file
	set(MACOSX_BUNDLE_BUNDLE_VERSION "${VER_FULL}")
	set(MACOSX_BUNDLE_EXECUTABLE ${PROJECT_NAME})
	set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.GraalReborn.GS2Emu")
	set(MACOSX_BUNDLE_NSMAIN_NIB_FILE "Application")
	set(MACOSX_BUNDLE_ICON_FILE "carton")
	set(MACOSX_BUNDLE_NAME ${PROJECT_NAME})
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${VER_FULL}")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -std=gnu++17")
elseif(WIN32)
	# Visual C++ Compiler options
	if(MSVC)
		# Warn if platform toolset may not be targetting Windows XP upwards
		if(NOT CMAKE_VS_PLATFORM_TOOLSET MATCHES "xp")
			message(WARNING
				"Your currently-selected platform toolset may generate "
				"executables which are incompatible with Windows XP.\n"
				"Please set your toolset to be one of v110_xp, v120_xp or "
				"v140_xp for VS2012, VS2013, and VS2015 respectively.\n"
				"You can do so with the '-T' argument to CMake, or by entering "
				"it in the CMake GUI."
			)
		endif()

		# Suppress secure string function warnings
		add_definitions(-D_CRT_SECURE_NO_WARNINGS)

		include(CheckCXXCompilerFlag)
		CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
		if (_cpp_latest_flag_supported)
			add_compile_options("/std:c++latest")
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE")
		endif()

		# Enable parallel compilation
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
		
		# Enable static linkage of the Microsoft Visual C/C++ Runtime
		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
		set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MTd")
		set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} /MT")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
		set(
			CMAKE_CXX_FLAGS_RELWITHDEBINFO
			"${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MTd"
		)
		set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MT")
		cmake_policy(SET CMP0043 NEW)
	endif()

	if( MINGW )
		add_definitions(-D__STDC_FORMAT_MACROS)
		add_definitions(-D__USE_MINGW_ANSI_STDIO=1)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17")
	endif()

	# Prevent Windows.h from clashing with the Standard Template Library so we
	# can use std::min/std::max (see https://support.microsoft.com/kb/143208)
	add_definitions(-DNOMINMAX)

elseif(AROS)
	# AROS will provide the SDL dependency via makefile.
elseif(AMIGA)
	#find_package(SDL 1.2 REQUIRED)
else()
	if(NOT NOSTATIC)
	#	SET(CMAKE_EXE_LINKER_FLAGS "-static")
	endif()

	add_definitions(-D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=1)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17")

endif()

find_package(ZLIB)
if(NOT ZLIB_FOUND)
	message("Zlib not found in system. Compiling it ourselves.")
	add_subdirectory(${PROJECT_SOURCE_DIR}/dependencies/zlib EXCLUDE_FROM_ALL)
	if(NOSTATIC)
		set_target_properties(zlibstatic minigzip example PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
	else()
		set_target_properties(zlib minigzip example PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
	endif()
endif()

find_package(BZip2)
if(NOT BZip2_FOUND)
	message("Bzip2 not found in system. Compiling it ourselves.")
	add_subdirectory(${PROJECT_SOURCE_DIR}/dependencies/bzip2)
endif()

if(NOT NOUPNP)
	find_package(Miniupnpc)

	if(NOT MINIUPNPC_FOUND)
		message("MiniUPNPC not found in system. Compiling it ourselves.")

		if(NOT NOSTATIC)
			add_definitions(-DMINIUPNP_STATICLIB)
			set(UPNPC_BUILD_STATIC TRUE CACHE BOOL "-" FORCE)
			set(UPNPC_BUILD_SHARED FALSE CACHE BOOL "-" FORCE)
		else()
			set(UPNPC_BUILD_STATIC FALSE CACHE BOOL "-" FORCE)
			set(UPNPC_BUILD_SHARED TRUE CACHE BOOL "-" FORCE)
		endif()

		set(UPNPC_BUILD_TESTS FALSE CACHE BOOL "-" FORCE)
		set(UPNPC_BUILD_SAMPLE FALSE CACHE BOOL "-" FORCE)
		set(NO_GETADDRINFO FALSE CACHE BOOL "-" FORCE)
		set(UPNPC_NO_INSTALL TRUE CACHE BOOL "-" FORCE)

		add_subdirectory(${PROJECT_SOURCE_DIR}/dependencies/miniupnp/miniupnpc EXCLUDE_FROM_ALL)
	endif()
endif()

# Prefer static linkage under OS X for libraries located with find_package()
if(APPLE)
	set(SUFFIXES_ORIG ${CMAKE_FIND_LIBRARY_SUFFIXES})
	set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif()

find_package(Threads REQUIRED)

# Restore library suffixes
if(APPLE)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${SUFFIXES_ORIG})
endif()

add_subdirectory(${PROJECT_SOURCE_DIR}/bin)
add_subdirectory(${PROJECT_SOURCE_DIR}/server/src)