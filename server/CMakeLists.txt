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
	src/main.cpp
	src/TAccount.cpp
	src/TMap.cpp
	src/TNPC.cpp
	src/TScriptClass.cpp
	src/TServer.cpp
	src/TServerList.cpp
	src/TUpdatePackage.cpp
	src/TWeapon.cpp
	src/Scripting/GS2ScriptManager.cpp
	src/TriggerCommandHandlers.cpp
	${PROJECT_SOURCE_DIR}/bin/servers/default/bootstrap.js
)

file(GLOB TPLAYER_SOURCES src/TPlayer/**.cpp)
list(APPEND SOURCES ${TPLAYER_SOURCES})

file(GLOB TLEVEL_SOURCES src/TLevel/**.cpp)
list(APPEND SOURCES ${TLEVEL_SOURCES})

file(GLOB MISC_SOURCES src/Misc/**.cpp)
list(APPEND SOURCES ${MISC_SOURCES})

file(GLOB UTILITIES_SOURCES src/utilities/**.cpp)
list(APPEND SOURCES ${UTILITIES_SOURCES})

file(GLOB ANIMATION_SOURCES src/Animation/**.cpp)
list(APPEND SOURCES ${ANIMATION_SOURCES})

include(bin2h)

set(EXE_HEADERS "")
set(
	HEADERS
	${PROJECT_BINARY_DIR}/server/include/IConfig.h
	include/CFileSystem.h
	include/main.h
	include/TAccount.h
	include/TMap.h
	include/TNPC.h
	include/TPlayer.h
	include/TScriptClass.h
	include/TServer.h
	include/TServerList.h
	include/TUpdatePackage.h
	include/TWeapon.h
	include/Scripting/GS2ScriptManager.h
	include/Scripting/ScriptOrigin.h
	include/Scripting/SourceCode.h)

file(GLOB TLEVEL_HEADERS include/TLevel/**.h)
list(APPEND HEADERS ${TLEVEL_HEADERS})

file(GLOB MISC_HEADERS include/Misc/**.h)
list(APPEND HEADERS ${MISC_HEADERS})

file(GLOB UTILITIES_HEADERS include/utilities/**.h)
list(APPEND HEADERS ${UTILITIES_HEADERS})

file(GLOB ANIMATION_HEADERS include/Animation/**.h)
list(APPEND HEADERS ${ANIMATION_HEADERS})

if(V8NPCSERVER)
	# Headers for script library interface
	file(GLOB SCRIPT_INTERFACE_HEADERS include/Scripting/interface/**.h)
	list(APPEND HEADERS ${SCRIPT_INTERFACE_HEADERS})

	# Headers for script library v8 implementation
	file(GLOB SCRIPT_V8_HEADERS include/Scripting/v8/**.h)
	list(APPEND HEADERS ${SCRIPT_V8_HEADERS})

	# Source for script library v8 implementation
	list(
		APPEND
		SOURCES
		src/Scripting/v8/V8ScriptEnv.cpp
	)

	# GServer specific headers for implementation
	list(
		APPEND
		HEADERS
		${PROJECT_BINARY_DIR}/server/include/EmbeddedBootstrapScript.h
		include/Scripting/CScriptEngine.h
		include/Scripting/ScriptAction.h
		include/Scripting/ScriptExecutionContext.h
		include/Scripting/ScriptFactory.h
		include/Scripting/v8/V8ScriptWrappers.h
	)

	# GServer specific source for implementation
	list(
		APPEND
		SOURCES
		src/Scripting/CScriptEngine.cpp
		src/Scripting/v8/V8EnvironmentImpl.cpp
		src/Scripting/v8/V8FunctionsImpl.cpp
		src/Scripting/v8/V8LevelImpl.cpp
		src/Scripting/v8/V8LevelLinkImpl.cpp
		src/Scripting/v8/V8LevelSignImpl.cpp
		src/Scripting/v8/V8LevelChestImpl.cpp
		src/Scripting/v8/V8NPCImpl.cpp
		src/Scripting/v8/V8PlayerImpl.cpp
		src/Scripting/v8/V8ScriptEnv.cpp
		src/Scripting/v8/V8ServerImpl.cpp
		src/Scripting/v8/V8WeaponImpl.cpp
	)
endif()

include_directories(
	# Include the CMake-generated version header from the build directory
	${PROJECT_BINARY_DIR}/server/include
	${PROJECT_SOURCE_DIR}/server/include
	${PROJECT_SOURCE_DIR}/server/include/Misc
	${PROJECT_SOURCE_DIR}/server/include/TLevel
	${PROJECT_SOURCE_DIR}/server/include/utilities
	${PROJECT_SOURCE_DIR}/server/include/Scripting
	${PROJECT_SOURCE_DIR}/server/include/Scripting/interface
	${PROJECT_SOURCE_DIR}/server/include/Scripting/v8
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

set(TARGET_NAME_OLD ${TARGET_NAME})
set(TARGET_NAME ${TARGET_NAME}_lib)

if(UPNP AND NOT MINIUPNPC_FOUND)
	include_directories(${PROJECT_SOURCE_DIR}/dependencies/miniupnp ${PROJECT_SOURCE_DIR}/dependencies/miniupnp/miniupnpc)
endif()


include_directories(${PROJECT_SOURCE_DIR}/dependencies/gs2lib/include)

if(APPLE)
	add_library(${TARGET_NAME} STATIC ${SOURCES} ${HEADERS})

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
		list(APPEND EXE_HEADERS
				${PROJECT_BINARY_DIR}/windresrc.h
				${PROJECT_BINARY_DIR}/main.rc
				)
		set(CMAKE_RC_COMPILER_INIT windres)

		ENABLE_LANGUAGE(RC)
		SET(CMAKE_RC_COMPILE_OBJECT
				"<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
	endif()

	add_library(${TARGET_NAME} STATIC ${SOURCES} ${HEADERS})

	if(MINGW)
		if(V8NPCSERVER)
			set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_FLAGS "-DV8_COMPRESS_POINTERS -DV8_31BIT_SMIS_ON_64BIT_ARCH")
		endif()
	endif()
	if(MSVC)
		set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
		set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
		set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
		set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
		set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
		set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE")
		set_target_properties(${TARGET_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

		if(V8NPCSERVER)
			set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_FLAGS "-DV8_COMPRESS_POINTERS -DV8_31BIT_SMIS_ON_64BIT_ARCH")

			# Using MSVC now looks for V8 as a nuget package first
			# Looking in lib folders if not found
			if(NOT V8_FOUND)
				message("Findv8 failed, looking for v8 in supplied folders")
				target_link_libraries(${TARGET_NAME} v8.dll.lib v8_libbase.dll.lib v8_libplatform.dll.lib)
			endif()
		endif()
	endif()
elseif(EMSCRIPTEN)
	add_library(${TARGET_NAME} STATIC ${SOURCES} ${HEADERS})
else()
	add_library(${TARGET_NAME} STATIC ${SOURCES} ${HEADERS})
endif()

target_compile_definitions(${TARGET_NAME} PRIVATE NOMAIN)

add_executable(${TARGET_NAME_OLD} src/main.cpp ${EXE_HEADERS})
target_link_libraries(${TARGET_NAME_OLD} PUBLIC ${TARGET_NAME})

target_link_libraries(${TARGET_NAME} ${CMAKE_THREAD_LIBS_INIT})

target_include_directories(${TARGET_NAME} PUBLIC ${GS2LIB_INCLUDE_DIRECTORY})

target_include_directories(${TARGET_NAME} PUBLIC ${GS2COMPILER_INCLUDE_DIRECTORY})

add_dependencies(${TARGET_NAME} gs2compiler)
target_link_libraries(${TARGET_NAME} gs2compiler)
add_dependencies(${TARGET_NAME} gs2lib)
target_link_libraries(${TARGET_NAME} gs2lib)

if(UPNP)
	if(NOT MINIUPNPC_FOUND)
		if(NOT STATIC)
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

if(V8NPCSERVER)
	if(NOT V8_FOUND)
		add_dependencies(${TARGET_NAME} v8)
		target_link_libraries(${TARGET_NAME} ${V8_LIBRARY})
	else()
		if(V8_LIBRARY)
			target_link_libraries(${TARGET_NAME} ${V8_LIBRARY})
		else()
			target_link_libraries(${TARGET_NAME} ${V8_MAIN_LIBRARY} ${V8_BASE_LIBRARY} ${V8_PLATFORM_LIBRARY})
			set(INSTALL_DEST .)
			install(FILES ${V8_REDIST_LIBS} DESTINATION ${INSTALL_DEST})
		endif()
	endif()

	message("V8 include: ${V8_INCLUDE_DIR}")
	include_directories(${V8_INCLUDE_DIR})

	target_link_libraries(${TARGET_NAME} httplib::httplib OpenSSL::SSL OpenSSL::Crypto)

	if(zstd_FOUND)
		target_link_libraries(${TARGET_NAME} zstd)
	endif()

	add_custom_command(
			OUTPUT ${PROJECT_BINARY_DIR}/server/include/EmbeddedBootstrapScript.h
			COMMAND ${CMAKE_COMMAND}
			-DSOURCE_FILE=${PROJECT_SOURCE_DIR}/bin/servers/default/bootstrap.js
			-DHEADER_FILE=${PROJECT_BINARY_DIR}/server/include/EmbeddedBootstrapScript.h
			-DVARIABLE_NAME=JSBOOTSTRAPSCRIPT
			-P "${CMAKE_SOURCE_DIR}/cmake/generate_header_file.cmake"
			COMMENT "Generating \"bootstrap.js\" header file..."
			WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
			DEPENDS "${PROJECT_SOURCE_DIR}/bin/servers/default/bootstrap.js"
			VERBATIM
	)

	add_custom_target(bootstrap_js_to_h DEPENDS ${PROJECT_BINARY_DIR}/server/include/EmbeddedBootstrapScript.h)
	add_dependencies(${TARGET_NAME} bootstrap_js_to_h)
endif()

if(WIN32)
	target_link_libraries(${TARGET_NAME} ws2_32 wsock32 iphlpapi)
endif()

set(APP_LIBRARY_NAME
		"${TARGET_NAME}"
		PARENT_SCOPE)

file(GLOB TEXT
	"${PROJECT_NAME_LOWER}.wasm"
)

set(INSTALL_DEST ".")

install(FILES ${TEXT} DESTINATION ${INSTALL_DEST})

set(INSTALL_DEST .)

install(TARGETS ${TARGET_NAME_OLD} DESTINATION ${INSTALL_DEST})

