#
#  server/src/CMakeLists.txt
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

include(CheckFunctionExists)
include(CheckSymbolExists)

set(
	SOURCES
	src/CFileSystem.cpp
	src/CWordFilter.cpp
	src/main.cpp
	src/TAccount.cpp
	src/TLevel.cpp
	src/TLevelBaddy.cpp
	src/TLevelBoardChange.cpp
	src/TLevelHorse.cpp
	src/TLevelItem.cpp
	src/TLevelLink.cpp
	src/TLevelSign.cpp
	src/TMap.cpp
	src/TNPC.cpp
	src/TPlayer.cpp
	src/TPlayerLogin.cpp
	src/TPlayerNC.cpp
	src/TPlayerProps.cpp
	src/TPlayerRC.cpp
	src/TServer.cpp
	src/TServerList.cpp
	src/TWeapon.cpp
)

set(
	HEADERS
	${PROJECT_BINARY_DIR}/server/include/IConfig.h
	include/CFileSystem.h
	include/CWordFilter.h
	include/main.h
	include/TAccount.h
	include/TLevel.h
	include/TLevelBaddy.h
	include/TLevelBoardChange.h
	include/TLevelChest.h
	include/TLevelHorse.h
	include/TLevelItem.h
	include/TLevelLink.h
	include/TLevelSign.h
	include/TMap.h
	include/TNPC.h
	include/TPlayer.h
	include/TServer.h
	include/TServerList.h
	include/TWeapon.h
)

if(NOT NOUPNP)
	list(
		APPEND
		SOURCES
		src/CUPNP.cpp
	)

	list(
		APPEND
		HEADERS
		include/CUPNP.h
	)
endif()

if(V8NPCSERVER)
	# Headers for script library interface
	list(
		APPEND
		HEADERS
		include/script/ScriptArguments.h
		include/script/ScriptBindings.h
		include/script/ScriptEnv.h
		include/script/ScriptFunction.h
		include/script/ScriptUtils.h
		include/script/ScriptWrapped.h
	)

	# Headers for script library v8 implementation
	list(
		APPEND
		HEADERS
		include/script/V8ScriptArguments.h
		include/script/V8ScriptEnv.h
		include/script/V8ScriptFunction.h
		include/script/V8ScriptUtils.h
		include/script/V8ScriptWrapped.h
	)

	# Source for script library v8 implementation
	list(
		APPEND
		SOURCES
		src/script/V8ScriptEnv.cpp
	)

	# GServer specific headers for implementation
	list(
		APPEND
		HEADERS
		include/CScriptEngine.h
		include/script/ScriptAction.h
		include/script/ScriptExecutionContext.h
		include/script/ScriptFactory.h
	)

	# GServer specific source for implementation
	list(
		APPEND
		SOURCES
		src/CScriptEngine.cpp
		src/script/V8EnvironmentImpl.cpp
		src/script/V8FunctionsImpl.cpp
		src/script/V8LevelImpl.cpp
		src/script/V8NPCImpl.cpp
		src/script/V8PlayerImpl.cpp
		src/script/V8ScriptEnv.cpp
		src/script/V8ServerImpl.cpp
		src/script/V8WeaponImpl.cpp
	)
endif()

include_directories(
	# Include the CMake-generated version header from the build directory
	${PROJECT_BINARY_DIR}/server/include
	${PROJECT_SOURCE_DIR}/server/include
	${PROJECT_SOURCE_DIR}/server/include/script
)

# Set target names for the executables
if(APPLE OR WIN32)
	# OS X and Windows get a mixed-case binary name
	set(TARGET_NAME ${PROJECT_NAME})
elseif(EMSCRIPTEN)
	set(TARGET_NAME ${PROJECT_NAME_LOWER})
else()
	# Linux/other UNIX get a lower-case binary name
	set(TARGET_NAME ${PROJECT_NAME_LOWER})
endif()

if(V8NPCSERVER)
	message("V8 include: ${V8_INCLUDE_DIR}")
	include_directories(${V8_INCLUDE_DIR})
endif()

if(NOT NOUPNP AND NOT MINIUPNPC_FOUND)
	include_directories(${PROJECT_SOURCE_DIR}/dependencies/miniupnp ${PROJECT_SOURCE_DIR}/dependencies/miniupnp/miniupnpc)
endif()

include_directories(${PROJECT_SOURCE_DIR}/dependencies/gs2lib/include)

if(APPLE)
	add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS})

	# Enable ARC (automatic reference counting) for OS X build
	set_property(
		TARGET ${TARGET_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc"
	)
elseif(WIN32)
	add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS})
	if(MSVC)
		set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
		set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
		set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
		set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
		set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
		set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE")
		set_target_properties(${TARGET_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

		if(V8NPCSERVER)
			target_link_libraries(${TARGET_NAME} v8.dll.lib v8_libbase.dll.lib v8_libplatform.dll.lib)
		endif()
	endif()
elseif(EMSCRIPTEN)
	add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS})
else()
	add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS})
endif()

target_link_libraries(${TARGET_NAME} ${CMAKE_THREAD_LIBS_INIT})

if(WIN32)
	target_link_libraries(${TARGET_NAME} ws2_32 wsock32 iphlpapi)
endif()

add_dependencies(${TARGET_NAME} gs2lib)
target_link_libraries(${TARGET_NAME} gs2lib)

if(NOT NOUPNP)
	if(NOT MINIUPNPC_FOUND)
		if(NOSTATIC)
			add_dependencies(${TARGET_NAME} libminiupnpc-shared)
			target_link_libraries(${TARGET_NAME} libminiupnpc-shared)
		else()
			add_dependencies(${TARGET_NAME} libminiupnpc-static)
			target_link_libraries(${TARGET_NAME} libminiupnpc-static)
		endif()
	else()
		target_link_libraries(${TARGET_NAME} ${MINIUPNP_LIBRARIES})
	endif()
endif()

if(V8NPCSERVER)
	if(NOT V8_FOUND)
		add_dependencies(${TARGET_NAME} v8)
		target_link_libraries(${TARGET_NAME} ${V8_LIBRARY})
	else()
		target_link_libraries(${TARGET_NAME} ${V8_LIBRARY})
	endif()
endif()

file(GLOB TEXT
	"${PROJECT_NAME_LOWER}.wasm"
)

set(INSTALL_DEST ".")

install(FILES ${TEXT} DESTINATION ${INSTALL_DEST})

set(INSTALL_DEST .)

install(TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DEST})
