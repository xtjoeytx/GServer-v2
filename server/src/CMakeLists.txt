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
    CEncryption.cpp
    CFileQueue.cpp
    CFileSystem.cpp
    CLog.cpp
    CPluginManager.cpp
    CSettings.cpp
    CSocket.cpp
    CString.cpp
    CTranslationManager.cpp
    CWordFilter.cpp
    IUtil.cpp
    main.cpp
    md5.cpp
    TAccount.cpp
    TLevel.cpp
    TLevelBaddy.cpp
    TLevelBoardChange.cpp
    TLevelChest.cpp
    TLevelHorse.cpp
    TLevelItem.cpp
    TLevelLink.cpp
    TLevelSign.cpp
    TMap.cpp
    TNPC.cpp
    TPlayer.cpp
    TPlayerLogin.cpp
    TPlayerNC.cpp
    TPlayerProps.cpp
    TPlayerRC.cpp
    TServer.cpp
    TServerList.cpp
    TWeapon.cpp
)

set(
    HEADERS
    ${PROJECT_BINARY_DIR}/server/include/IConfig.h
    ../include/CEncryption.h
    ../include/CFileQueue.h
    ../include/CFileSystem.h
    ../include/CLog.h
    ../include/CPlugin.h
    ../include/CPluginManager.h
    ../include/CSettings.h
    ../include/CSocket.h
    ../include/CString.h
    ../include/CTimeout.h
    ../include/CTranslationManager.h
    ../include/CWordFilter.h
    ../include/IConfig.h
    ../include/IConfig.h.in
    ../include/IDebug.h
    ../include/IEnums.h
    ../include/IUtil.h
    ../include/main.h
    ../include/md5.h
    ../include/TAccount.h
    ../include/TLevel.h
    ../include/TLevelBaddy.h
    ../include/TLevelBoardChange.h
    ../include/TLevelChest.h
    ../include/TLevelHorse.h
    ../include/TLevelItem.h
    ../include/TLevelLink.h
    ../include/TLevelSign.h
    ../include/TMap.h
    ../include/TNPC.h
    ../include/TPlayer.h
    ../include/TServer.h
    ../include/TServerList.h
    ../include/TWeapon.h
)


if(NOT NOUPNP)
	list(
		APPEND
		SOURCES
		CUPNP.cpp
	)

	list(
		APPEND
		HEADERS
		../include/CUPNP.h
	)
endif()

if(V8NPCSERVER)
	list(
		APPEND
		SOURCES
		CScriptEngine.cpp
		script/V8FunctionsImpl.cpp
		script/V8NPCImpl.cpp
		script/V8PlayerImpl.cpp
		script/V8ScriptEnv.cpp
		script/V8ServerImpl.cpp
	)

	list(
		APPEND
		HEADERS
		../include/CScriptEngine.h
		../include/script/ScriptAction.h
		../include/script/ScriptArguments.h
		../include/script/ScriptEnv.h
		../include/script/ScriptFactory.h
		../include/script/ScriptFunction.h
		../include/script/ScriptRunError.h
		../include/script/ScriptWrapped.h
		../include/script/V8Macros.h
		../include/script/V8ScriptArguments.h
		../include/script/V8ScriptEnv.h
		../include/script/V8ScriptFunction.h
		../include/script/V8ScriptWrapped.h
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
	set(TARGET_NAME ${PROJECT_NAME_LOWER}.html)
else()
    # Linux/other UNIX get a lower-case binary name
    set(TARGET_NAME ${PROJECT_NAME_LOWER})
endif()

if(V8NPCSERVER)
    include_directories(${V8_INCLUDE_DIR})
endif()

if(NOT NOUPNP AND NOT MINIUPNPC_FOUND)
    include_directories(${PROJECT_SOURCE_DIR}/dependencies/miniupnp ${PROJECT_SOURCE_DIR}/dependencies/miniupnp/miniupnpc)
endif()

if(NOT ZLIB_FOUND)
	include_directories(${PROJECT_BINARY_DIR}/dependencies/zlib ${PROJECT_SOURCE_DIR}/dependencies/zlib)
endif()

if(NOT BZip2_FOUND)
	include_directories(${PROJECT_SOURCE_DIR}/dependencies/bzip2)
endif()

check_symbol_exists(inet_pton "ws2tcpip.h" HAVE_INET_PTON)
if (NOT HAVE_INET_PTON)
	check_function_exists(inet_pton HAVE_INET_PTON)
endif()

if (HAVE_INET_PTON)
	add_definition(-DHAVE_INET_PTON)
endif()

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

if(NOT ZLIB_FOUND AND NOT NOSTATIC)
	add_dependencies(${TARGET_NAME} zlibstatic)
	target_link_libraries(${TARGET_NAME} zlibstatic)
elseif(NOT ZLIB_FOUND AND NOSTATIC)
	add_dependencies(${TARGET_NAME} zlib)
	target_link_libraries(${TARGET_NAME} zlib)
else()
	target_link_libraries(${TARGET_NAME} ${ZLIB_LIBRARIES})
endif()

if(NOT BZip2_FOUND)
	add_dependencies(${TARGET_NAME} bzip2)
	target_link_libraries(${TARGET_NAME} bzip2)
else()
	target_link_libraries(${TARGET_NAME} ${BZIP2_LIBRARIES})
endif()

target_link_libraries(${TARGET_NAME} ${CMAKE_THREAD_LIBS_INIT})

if(WIN32)
	target_link_libraries(${TARGET_NAME} ws2_32 wsock32 iphlpapi)
endif()

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
		target_link_libraries(${TARGET_NAME} ${V8_LIBRARY2})
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
