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
    CUPNP.cpp
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
    ../include/CUPNP.h
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
else()
    # Linux/other UNIX get a lower-case binary name
    set(TARGET_NAME ${PROJECT_NAME_LOWER})
endif()

if(V8NPCSERVER)
    include_directories(${V8_INCLUDE_DIR})
endif()

if(APPLE)
    if(NOT NOUPNP)
        include_directories(${PROJECT_SOURCE_DIR}/dependencies/miniupnp ${PROJECT_SOURCE_DIR}/dependencies/miniupnp/miniupnpc)
    endif()
    add_executable(${TARGET_NAME} MACOSX_BUNDLE ${SOURCES} ${HEADERS})

    # Xcode can deal with Interface Builder xibs automatically - if we are not
    # generating for Xcode, then we must manually compile and install any xibs
    if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
        # Locate ibtool
        find_program(IBTOOL ibtool HINTS /usr/bin ${OSX_DEVELOPER_ROOT}/usr/bin)
        if(IBTOOL STREQUAL "IBTOOL-NOTFOUND")
            message(SEND_ERROR "Unable to find ibtool. Is Xcode installed?")
        endif()

        # The Interface Builder xib file to be compiled
        set(
            XIB_FILE
            ${PROJECT_SOURCE_DIR}/src/tracker/cocoa/resources/Application.xib
        )

        # Destination for compiled xib
        set(RESOURCES_DIR $<TARGET_FILE_DIR:${TARGET_NAME}>/../Resources)

        # Ensure the destination directory of the compiled xib exists
        add_custom_command(
            TARGET ${TARGET_NAME} PRE_BUILD COMMAND mkdir -p ${RESOURCES_DIR}
        )

        # Compile the xib file
        add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD COMMAND ${IBTOOL} --errors
            --warnings --notices --output-format human-readable-text
            --minimum-deployment-target 10.7 --compile
            ${RESOURCES_DIR}/${MACOSX_BUNDLE_NSMAIN_NIB_FILE}.nib ${XIB_FILE}
            COMMENT "Compiling ${XIB_FILE}"
        )
    endif()

    # Pass in the Info.plist template, whose variables are defined in the
    # top-level CMakeLists.txt
#    set_target_properties(
#        ${TARGET_NAME}
#        PROPERTIES
#        MACOSX_BUNDLE_INFO_PLIST
#        ${PROJECT_SOURCE_DIR}/ser/tracker/cocoa/resources/Info.plist.in
#    )

    # Enable ARC (automatic reference counting) for OS X build
    set_property(
        TARGET ${TARGET_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc"
    )

    target_link_libraries(${TARGET_NAME} bzip2 z ${CMAKE_THREAD_LIBS_INIT})
    if(NOT NOUPNP)
        if(NOSTATIC)
            add_dependencies(${TARGET_NAME} libminiupnpc-shared)
        else()
            add_dependencies(${TARGET_NAME} libminiupnpc-static)
        endif()
        target_link_libraries(${TARGET_NAME} miniupnpc)
    endif()
elseif(WIN32)

    include_directories(${PROJECT_SOURCE_DIR}/dependencies/miniupnp ${PROJECT_SOURCE_DIR}/dependencies/bzip2 ${PROJECT_SOURCE_DIR}/dependencies/zlib ${PROJECT_SOURCE_DIR}/dependencies/miniupnp/miniupnpc)

    add_executable(${TARGET_NAME} WIN32 ${SOURCES} ${HEADERS})

	set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
	set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
	set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
	set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
	set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
	set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")

    add_dependencies(${TARGET_NAME} bzip2 z)
	target_link_libraries(${TARGET_NAME} z bzip2 ${CMAKE_THREAD_LIBS_INIT})

	if(NOT NOUPNP)
		if(NOSTATIC)
			add_dependencies(${TARGET_NAME} libminiupnpc-shared)
		else()
			add_dependencies(${TARGET_NAME} libminiupnpc-static)
		endif()
		target_link_libraries(${TARGET_NAME} libminiupnpc ws2_32 wsock32 iphlpapi)
	endif()
elseif(AROS)
    include_directories(${PROJECT_SOURCE_DIR}/dependencies/miniupnp ${PROJECT_SOURCE_DIR}/dependencies/bzip2 ${PROJECT_SOURCE_DIR}/dependencies/zlib ${PROJECT_SOURCE_DIR}/dependencies/miniupnp/miniupnpc)
    add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS})
    add_dependencies(${TARGET_NAME} bzip2 z miniupnpc)

    target_link_libraries(${TARGET_NAME} ${ZLIB_LIBRARIES} ${BZIP2_LIBRARIES} ${MINIUPNP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
elseif(AMIGA)
    include_directories(${PROJECT_SOURCE_DIR}/dependencies/miniupnp ${PROJECT_SOURCE_DIR}/dependencies/bzip2 ${PROJECT_SOURCE_DIR}/dependencies/zlib ${PROJECT_SOURCE_DIR}/dependencies/miniupnp/miniupnpc)
    add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS})
    add_dependencies(${TARGET_NAME} bzip2 z miniupnpc)

    if(MORPHOS OR AMIGAOS3 AND NOT WARPOS)
        target_link_libraries(${TARGET_NAME} ${ZLIB_LIBRARIES} ${BZIP2_LIBRARIES} ${MINIUPNP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
    else()
        target_link_libraries(${TARGET_NAME} ${ZLIB_LIBRARIES} ${BZIP2_LIBRARIES} ${MINIUPNP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
    endif()
else()
    add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS})

    target_link_libraries(${TARGET_NAME} ${ZLIB_LIBRARIES} ${BZIP2_LIBRARIES} ${MINIUPNP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif()

if(V8NPCSERVER)
	target_link_libraries(${TARGET_NAME} ${V8_LIBBASE_LIBRARY} ${V8_LIBPLATFORM_LIBRARY})
endif()

set(INSTALL_DEST .)

install(TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DEST})
