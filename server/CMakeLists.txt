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
	"include/common.h"
	"include/FileSystem.h"
	"include/IConfig.h.in"
	"include/main.h"
	"include/Server.h"
	"include/ServerList.h"
	"include/UpdatePackage.h"
	"include/windresrc.h.in"
	"include/animation/GameAni.h"
	"include/level/Level.h"
	"include/level/LevelBaddy.h"
	"include/level/LevelBoardChange.h"
	"include/level/LevelChest.h"
	"include/level/LevelHorse.h"
	"include/level/LevelItem.h"
	"include/level/LevelLink.h"
	"include/level/LevelSign.h"
	"include/level/LevelTiles.h"
	"include/level/Map.h"
	"include/level/tiletypes.h"
	"include/misc/UPNP.h"
	"include/misc/WordFilter.h"
	"include/network/IPacketHandler.h"
	"include/npcserver/NPCServer.h"
	"include/npcserver/PlayerNpcServer.h"
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
	"include/scripting/ScriptOrigin.h"
	"include/scripting/ScriptSystem.h"
	"include/scripting/ScriptTypes.h"
	"include/scripting/SourceCode.h"
	"include/scripting/gs1/ScriptEngineGS1.h"
	"include/scripting/gs2/ScriptEngineGS2.h"
	"include/utilities/CommandDispatcher.h"
	"include/utilities/FilePermissions.h"
	"include/utilities/FlagContainer.h"
	"include/utilities/IdGenerator.h"
	"include/utilities/inplace_vector.h"
	"include/utilities/Log.h"
	"include/utilities/ResourceManager.h"
	"include/utilities/StringUtils.h"
	"include/utilities/TimeoutGenerator.h"
	"include/utilities/TimeUnits.h"
	"src/Account.cpp"
	"src/FileSystem.cpp"
	"src/main.cpp"
	"src/Server.cpp"
	"src/ServerList.cpp"
	"src/TriggerCommandHandlers.cpp"
	"src/UpdatePackage.cpp"
	"src/versions.txt"
	"src/animation/GameAni.cpp"
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
	"src/npcserver/PlayerNpcServer.cpp"
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
	"src/scripting/ScriptSystem.cpp"
	"src/scripting/SourceCode.cpp"
	"src/scripting/gs2/ScriptEngineGS2.cpp"
	"src/utilities/FilePermissions.cpp"
	"src/utilities/Log.cpp"
	"src/utilities/StringUtils.cpp"
)

# Create the executable target.
set(TARGET_NAME "${PROJECT_NAME_LOWER}")
add_executable(${TARGET_NAME})

# Add the sources.
target_sources(${TARGET_NAME} PUBLIC ${SOURCES})
target_sources(${TARGET_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS "include" "${PROJECT_BINARY_DIR}/server/include")

# Configure our compiler options.
set_default_compiler_options(${TARGET_NAME})

# Packet logging.
option(PACKET_LOGGING "Enable debug packet logging." OFF)
if(PACKET_LOGGING)
	target_compile_definitions(${TARGET_NAME} PUBLIC PACKETLOGGING)
endif()

# Dependency: antlr4.
find_package(antlr4-runtime CONFIG REQUIRED)
target_link_libraries(${TARGET_NAME} PRIVATE antlr4_static)

# Dependency: miniupnpc.
option(UPNP "Enable UPNP support." ON)
if(UPNP)
	find_package(miniupnpc CONFIG REQUIRED)
	target_link_libraries(${TARGET_NAME} PRIVATE miniupnpc::miniupnpc miniupnpc::miniupnpc-private)
endif()

# Dependency: gs2lib.
FetchContent_Declare(gs2lib
	GIT_REPOSITORY https://xtjoeytx@bitbucket.org/xtjoeytx/gs2lib.git
	GIT_TAG        fetchcontent
	#URL "${PROJECT_SOURCE_DIR}/../gs2lib"
)
FetchContent_MakeAvailable(gs2lib)
target_link_libraries(${TARGET_NAME} PRIVATE gs2lib)
target_include_directories(${TARGET_NAME} PRIVATE "${gs2lib_SOURCE_DIR}/include")

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

target_link_libraries(${TARGET_NAME} ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(${TARGET_NAME} PUBLIC ${GS2LIB_INCLUDE_DIRECTORY})
target_include_directories(${TARGET_NAME} PUBLIC ${GS2COMPILER_INCLUDE_DIRECTORY})

add_dependencies(${TARGET_NAME} gs2compiler)
target_link_libraries(${TARGET_NAME} PRIVATE gs2compiler)

if (MINGW)
	target_link_options(${TARGET_NAME} PRIVATE -static -fstack-protector)
	target_link_libraries(${TARGET_NAME} -static-libgcc -static-libstdc++)
	target_link_options(${TARGET_NAME_OLD} PRIVATE -static -fstack-protector)
	target_link_libraries(${TARGET_NAME_OLD} PUBLIC -static-libgcc -static-libstdc++)

	install(CODE "set(MY_EXE \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME_OLD}.exe\")")

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

if(WIN32)
	target_link_libraries(${TARGET_NAME} PRIVATE ws2_32 wsock32 iphlpapi)
endif()
