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
	"${PROJECT_BINARY_DIR}/server/include/IConfig.h"
	"include/Account.h"
	"include/BabyDI.h"
	"include/FileSystem.h"
	"include/IConfig.h.in"
	"include/main.h"
	"include/Server.h"
	"include/ServerList.h"
	"include/UpdatePackage.h"
	"include/windresrc.h.in"
	"include/animation/GameAni.h"
	"include/level/Level.h"
	"include/level/LevelArrow.h"
	"include/level/LevelBaddy.h"
	"include/level/LevelBoardChange.h"
	"include/level/LevelBomb.h"
	"include/level/LevelChest.h"
	"include/level/LevelExplosion.h"
	"include/level/LevelHorse.h"
	"include/level/LevelItem.h"
	"include/level/LevelLink.h"
	"include/level/LevelSign.h"
	"include/level/LevelTiles.h"
	"include/level/Map.h"
	"include/level/tiletypes.h"
	"include/loader/IAccountLoader.h"
	"include/loader/INPCLoader.h"
	"include/loader/LevelLoader.h"
	"include/loader/flatfile/FlatFileAccountLoader.h"
	"include/loader/flatfile/FlatFileNPCLoader.h"
	"include/misc/UPNP.h"
	"include/misc/WordFilter.h"
	"include/network/IPacketHandler.h"
	"include/npcserver/NPCServer.h"
	"include/npcserver/PlayerNPCServer.h"
	"include/object/Character.h"
	"include/object/NPC.h"
	"include/object/Player.h"
	"include/object/Weapon.h"
	"include/player/PlayerClient.h"
	"include/player/PlayerLogin.h"
	"include/player/PlayerNC.h"
	"include/player/PlayerProps.h"
	"include/player/PlayerRC.h"
	"include/scripting/GS2ScriptManager.h"
	"include/scripting/IScriptEngine.h"
	"include/scripting/ScriptClass.h"
	"include/scripting/ScriptContainers.h"
	"include/scripting/ScriptSystem.h"
	"include/scripting/ScriptTypes.h"
	"include/scripting/Script.h"
	"include/scripting/gs1/GS1Commands.h"
	"include/scripting/gs1/GS1ErrorListener.h"
	"include/scripting/gs1/GS1Flags.h"
	"include/scripting/gs1/GS1Functions.h"
	"include/scripting/gs1/GS1MessageCodes.h"
	"include/scripting/gs1/GS1Variables.h"
	"include/scripting/gs1/GS1Visitor.h"
	"include/scripting/gs1/ScriptEngineGS1.h"
	"include/scripting/gs2/ScriptEngineGS2.h"
	"include/utilities/CommandDispatcher.h"
	"include/utilities/CommonTypes.h"
	"include/utilities/Extents.h"
	"include/utilities/FilePermissions.h"
	"include/utilities/IdGenerator.h"
	"include/utilities/inplace_vector.h"
	"include/utilities/Log.h"
	"include/utilities/PropsContainer.h"
	"include/utilities/ResourceManager.h"
	"include/utilities/StringUtils.h"
	"include/utilities/TimeoutGenerator.h"
	"src/FileSystem.cpp"
	"src/main.cpp"
	"src/Server.cpp"
	"src/ServerList.cpp"
	"src/TriggerCommandHandlers.cpp"
	"src/UpdatePackage.cpp"
	"src/versions.txt"
	"src/animation/GameAni.cpp"
	"src/loader/LevelLoader.cpp"
	"src/loader/flatfile/FlatFileAccountLoader.cpp"
	"src/loader/flatfile/FlatFileNPCLoader.cpp"
	"src/level/Level.cpp"
	"src/level/LevelBaddy.cpp"
	"src/level/LevelBoardChange.cpp"
	"src/level/LevelItem.cpp"
	"src/level/LevelLink.cpp"
	"src/level/LevelSign.cpp"
	"src/level/Map.cpp"
	"src/misc/UPNP.cpp"
	"src/misc/WordFilter.cpp"
	"src/npcserver/NPCServer.cpp"
	"src/npcserver/PlayerNPCServer.cpp"
	"src/object/NPC.cpp"
	"src/object/Weapon.cpp"
	"src/player/Player.cpp"
	"src/player/PlayerClient.cpp"
	"src/player/PlayerExternalPlayers.cpp"
	"src/player/PlayerLogin.cpp"
	"src/player/PlayerNC.cpp"
	"src/player/PlayerProps.cpp"
	"src/player/PlayerRC.cpp"
	"src/player/PlayerRequestText.cpp"
	"src/player/packets/PlayerClientPackets.cpp"
	"src/player/packets/PlayerNCPackets.cpp"
	"src/player/packets/PlayerRCPackets.cpp"
	"src/scripting/GS2ScriptManager.cpp"
	"src/scripting/ScriptClass.cpp"
	"src/scripting/ScriptContainers.cpp"
	"src/scripting/ScriptSystem.cpp"
	"src/scripting/Script.cpp"
	"src/scripting/gs1/GS1Commands.cpp"
	"src/scripting/gs1/GS1Flags.cpp"
	"src/scripting/gs1/GS1Functions.cpp"
	"src/scripting/gs1/GS1MessageCodes.cpp"
	"src/scripting/gs1/GS1Variables.cpp"
	"src/scripting/gs1/GS1Visitor.cpp"
	"src/scripting/gs1/ScriptEngineGS1.cpp"
	"src/scripting/gs2/ScriptEngineGS2.cpp"
	"src/utilities/FilePermissions.cpp"
	"src/utilities/Log.cpp"
	"src/utilities/PropsContainer.cpp"
	"src/utilities/StringUtils.cpp"
)

# Create the executable target.
set(TARGET_NAME "${PROJECT_NAME_LOWER}")
add_executable(${TARGET_NAME})
set_target_properties(${TARGET_NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
target_link_directories(${TARGET_NAME} PRIVATE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

# Add the sources.
target_sources(${TARGET_NAME} PUBLIC ${SOURCES})
target_sources(${TARGET_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS "include" "${PROJECT_BINARY_DIR}/server/include")

# Configure our compiler options.
set_default_compiler_options(${TARGET_NAME} FALSE)

if (TESTS)
	set(APP_LIBRARY_NAME "lib${TARGET_NAME}")
	set(APP_LIBRARY_NAME_TESTREF "${APP_LIBRARY_NAME}" PARENT_SCOPE)
	add_library(${APP_LIBRARY_NAME})
	set_target_properties(${APP_LIBRARY_NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
	target_link_directories(${APP_LIBRARY_NAME} PRIVATE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

	# Add the sources.
	target_sources(${APP_LIBRARY_NAME} PUBLIC ${SOURCES})
	target_sources(${APP_LIBRARY_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS "include" "${PROJECT_BINARY_DIR}/server/include")

	# Configure our compiler options.
	set_default_compiler_options(${APP_LIBRARY_NAME} TRUE)
endif()

# Packet logging.
option(PACKET_LOGGING "Enable debug packet logging." OFF)
if(PACKET_LOGGING)
	target_compile_definitions(${TARGET_NAME} PUBLIC PACKETLOGGING)
endif()

# Dependency: zlib.
find_package(ZLIB REQUIRED)
target_link_libraries(${TARGET_NAME} PRIVATE ZLIB::ZLIB)

# Dependency: bzip2
find_package(BZip2 REQUIRED)
target_link_libraries(${TARGET_NAME} PRIVATE BZip2::BZip2)

# Dependency: antlr4.
find_package(antlr4-runtime CONFIG REQUIRED)
find_package(antlr4-generator CONFIG REQUIRED)
target_link_directories(${TARGET_NAME} PRIVATE ${ANTLR4_LIBRARY_DIR})
target_include_directories(${TARGET_NAME} PRIVATE ${ANTLR4_INCLUDE_DIR})
target_link_libraries(${TARGET_NAME} PRIVATE antlr4_static Threads::Threads)
if(TESTS)
	target_link_directories(${APP_LIBRARY_NAME} PUBLIC ${ANTLR4_LIBRARY_DIR})
	target_include_directories(${APP_LIBRARY_NAME} PUBLIC ${ANTLR4_INCLUDE_DIR})
	target_link_libraries(${APP_LIBRARY_NAME} PUBLIC antlr4_static)
endif()
FetchContent_Declare(AntlrJar
	URL				https://www.antlr.org/download/antlr-4.13.2-complete.jar
	DOWNLOAD_DIR	"${FETCHCONTENT_BASE_DIR}/antlr-jar"
	DOWNLOAD_NAME	"antlr.jar"
	DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(AntlrJar)
set(ANTLR4_JAR_LOCATION "${FETCHCONTENT_BASE_DIR}/antlr-jar/antlr.jar")

# Dependency: miniupnpc.
option(UPNP "Enable UPNP support." ON)
if(UPNP)
	find_package(miniupnpc CONFIG REQUIRED)
	target_compile_definitions(${TARGET_NAME} PUBLIC ENABLE_UPNP)
	target_link_libraries(${TARGET_NAME} PRIVATE miniupnpc::miniupnpc miniupnpc::miniupnpc-private)
endif()

# Dependency: gs2lib.
FetchContent_Declare(gs2lib
	GIT_REPOSITORY https://xtjoeytx@bitbucket.org/xtjoeytx/gs2lib.git
	GIT_TAG        fetchcontent
	#URL "${PROJECT_SOURCE_DIR}/../gs2lib"
)
FetchContent_MakeAvailable(gs2lib)
add_dependencies(${TARGET_NAME} gs2lib)
target_link_libraries(${TARGET_NAME} PUBLIC gs2lib)
target_include_directories(${TARGET_NAME} PUBLIC "${gs2lib_SOURCE_DIR}/include")

if(TESTS)
	target_link_libraries(${APP_LIBRARY_NAME} PUBLIC gs2lib)
	target_include_directories(${APP_LIBRARY_NAME} PUBLIC "${gs2lib_SOURCE_DIR}/include")
endif ()

# Dependency: libtommath
#[[
FetchContent_Declare(libtommath
	GIT_REPOSITORY https://github.com/libtom/libtommath.git
	GIT_TAG        develop
)
FetchContent_MakeAvailable(libtommath)
#target_link_libraries(${TARGET_NAME} PRIVATE libtommath)
target_include_directories(${TARGET_NAME} PRIVATE "${libtommath_SOURCE_DIR}")

# Dependency: libtomcrypt-preagonal.
FetchContent_Declare(tomcrypt
	GIT_REPOSITORY https://github.com/Preagonal/libtomcrypt.git
	GIT_TAG        develop
	FIND_PACKAGE_ARGS CONFIG
)
FetchContent_MakeAvailable(tomcrypt)
target_link_libraries(${TARGET_NAME} PRIVATE tomcrypt$<$<CONFIG:Debug>:${CMAKE_DEBUG_POSTFIX}>)
target_include_directories(${TARGET_NAME} PRIVATE "${libtomcrypt_SOURCE_DIR}/src/headers")
add_dependencies(${TARGET_NAME} libtomcrypt)
]]

# Generate grammar.
antlr4_generate(GS1Lexer "${CMAKE_CURRENT_SOURCE_DIR}/src/scripting/gs1/grammar/GS1Lexer.g4" LEXER
	FALSE						# 4: Generate listener
	FALSE						# 5: Generate visitor
	"preagonal::gs1::grammar"	# 6: Namespace
)
message("${CMAKE_CURRENT_SOURCE_DIR}/src/scripting/gs1/GS1Parser.g4")
antlr4_generate(GS1Parser "${CMAKE_CURRENT_SOURCE_DIR}/src/scripting/gs1/grammar/GS1Parser.g4" PARSER
	FALSE						# 4: Generate listener
	TRUE						# 5: Generate visitor
	"preagonal::gs1::grammar"	# 6: Namespace
	"${ANTLR4_TOKEN_FILES_GS1Lexer}"		# 7: Dependency - Token files
	"${ANTLR4_TOKEN_DIRECTORY_GS1Lexer}"	# 8: Location of grammars - Token directory
)
target_include_directories(${TARGET_NAME} PRIVATE ${ANTLR4_INCLUDE_DIR_GS1Lexer} ${ANTLR4_INCLUDE_DIR_GS1Parser})
target_sources(${TARGET_NAME} PRIVATE ${ANTLR4_SRC_FILES_GS1Lexer} ${ANTLR4_SRC_FILES_GS1Parser})
if(TESTS)
	target_include_directories(${APP_LIBRARY_NAME} PUBLIC ${ANTLR4_INCLUDE_DIR_GS1Lexer} ${ANTLR4_INCLUDE_DIR_GS1Parser})
	target_sources(${APP_LIBRARY_NAME} PUBLIC ${ANTLR4_SRC_FILES_GS1Lexer} ${ANTLR4_SRC_FILES_GS1Parser})
endif()

# Windows libraries.
if(WIN32)
	target_link_libraries(${TARGET_NAME} PRIVATE ws2_32 wsock32 iphlpapi)
endif()

# ???
if(APPLE)
	# Enable ARC (automatic reference counting) for OS X build
	set_property(
		TARGET ${TARGET_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc"
	)
elseif(WIN32)
	if(MINGW)
		# Generate version header from the above
		configure_file(
			${PROJECT_SOURCE_DIR}/server/include/windresrc.h.in
			${PROJECT_BINARY_DIR}/windresrc.h
		)
		configure_file(
			${PROJECT_SOURCE_DIR}/my.rc.in
			${PROJECT_BINARY_DIR}/main.rc
		)
		file(COPY
			${PROJECT_SOURCE_DIR}/gs2emu.ico
			DESTINATION
			${PROJECT_BINARY_DIR}/
		)
		target_sources(${TARGET_NAME}
			PUBLIC
			${PROJECT_BINARY_DIR}/windresrc.h
			${PROJECT_BINARY_DIR}/main.rc
		)

		target_compile_definitions(${TARGET_NAME} PRIVATE _WIN32_WINNT=0x600)
		set(CMAKE_RC_COMPILER_INIT windres)

		ENABLE_LANGUAGE(RC)
		SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
	endif()

	if(MSVC)
		set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
		set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
		set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
		set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
		set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
		set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE")
		set_target_properties(${TARGET_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
	endif()
endif()

target_link_libraries(${TARGET_NAME} PRIVATE ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(${TARGET_NAME} PUBLIC ${GS2LIB_INCLUDE_DIRECTORY})
target_include_directories(${TARGET_NAME} PUBLIC ${GS2COMPILER_INCLUDE_DIRECTORY})

add_dependencies(${TARGET_NAME} gs2compiler)
target_link_libraries(${TARGET_NAME} PRIVATE gs2compiler)

if(TESTS)
	target_link_libraries(${APP_LIBRARY_NAME} PRIVATE gs2compiler)
	target_link_libraries(${APP_LIBRARY_NAME} PRIVATE ${CMAKE_THREAD_LIBS_INIT})
	target_include_directories(${APP_LIBRARY_NAME} PUBLIC ${GS2LIB_INCLUDE_DIRECTORY})
	target_include_directories(${APP_LIBRARY_NAME} PUBLIC ${GS2COMPILER_INCLUDE_DIRECTORY})
endif()

if (MINGW)
	target_link_options(${TARGET_NAME} PRIVATE -static -fstack-protector)
	target_link_libraries(${TARGET_NAME} PRIVATE -static-libgcc)

	#target_link_options(${TARGET_NAME_OLD} PRIVATE -static -fstack-protector)
	#target_link_libraries(${TARGET_NAME_OLD} PUBLIC -static-libgcc)

	install(CODE "set(MY_EXE \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}.exe\")")

	# Transfer the value of ${MY_DEPENDENCY_PATHS} into the install script
	install(CODE "set(MY_DEPENDENCY_PATHS \"${CMAKE_FIND_DLL_PATH}\")")
	install(CODE [[
		set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "windows+pe")
		set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL "objdump")
		set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND "./objdump_unix2dos.sh")

		write_file("./objdump_unix2dos.sh" "${CMAKE_OBJDUMP} $@ | unix2dos")
		file(CHMOD "./objdump_unix2dos.sh" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)

		function(install_library_with_deps LIBRARY)
			message("Getting dependencies for ${LIBRARY}")

			file(INSTALL
				DESTINATION "${CMAKE_INSTALL_PREFIX}/"
				TYPE SHARED_LIBRARY
				FOLLOW_SYMLINK_CHAIN
				FILES "${LIBRARY}"
			)
			file(GET_RUNTIME_DEPENDENCIES
				LIBRARIES ${LIBRARY}
				RESOLVED_DEPENDENCIES_VAR RESOLVED_DEPS
				UNRESOLVED_DEPENDENCIES_VAR UNRESOLVED_DEPS
				DIRECTORIES ${MY_DEPENDENCY_PATHS}
			)
			foreach(FILE ${RESOLVED_DEPS})
				if(NOT IS_SYMLINK ${FILE})
					install_library_with_deps(${FILE})
				else()
					message( "Symlink ${LIBRARY}: ${FILE}")
				endif()
			endforeach()
			foreach(FILE ${UNRESOLVED_DEPS})
				message( "Unresolved from ${LIBRARY}: ${FILE}")
			endforeach()
		endfunction()
		message("Getting dependencies for ${MY_EXE}")
		file(GET_RUNTIME_DEPENDENCIES
			EXECUTABLES ${MY_EXE}
			RESOLVED_DEPENDENCIES_VAR RESOLVED_DEPS
			UNRESOLVED_DEPENDENCIES_VAR UNRESOLVED_DEPS
			DIRECTORIES ${MY_DEPENDENCY_PATHS}
		)
		foreach(FILE ${RESOLVED_DEPS})
			install_library_with_deps(${FILE})
		endforeach()
		foreach(FILE ${UNRESOLVED_DEPS})
			message( "Unresolved: ${FILE}")
		endforeach()
	]])
endif()

set(INSTALL_DEST .)
install(TARGETS ${TARGET_NAME} RUNTIME DESTINATION ${INSTALL_DEST})
