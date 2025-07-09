function(set_default_compiler_options target)
	# C++23 mode.
	set(CMAKE_CXX_STANDARD 23)
	set(CMAKE_CXX_STANDARD_REQUIRED True)
	target_compile_features(${target} PUBLIC cxx_std_23)
	set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
	if(MSVC)
		if(MSVC_VERSION GREATER_EQUAL 1910)
			target_compile_options(${target} PUBLIC "/permissive-")
		endif()
		if(MSVC_VERSION GREATER_EQUAL 1914)
			target_compile_options(${target} PUBLIC "/Zc:__cplusplus")
		endif()
		if(MSVC_VERSION GREATER_EQUAL 1925)
			target_compile_options(${target} PUBLIC "/Zc:preprocessor")
		endif()
	endif()

	# Compiler options.
	set_target_properties(${target} PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
	if(MSVC)
		target_compile_options(${target} PUBLIC
			"$<$<CONFIG:Release>:/guard:cf>"		# Control Flow Guard
			"$<$<CONFIG:Release>:/Qspectre>"		# Spectre Mitigation
		)
	endif()

	# Ignore attribute warnings.
	if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
		target_compile_options(${target} PRIVATE
			-Wno-attributes
			-Wno-narrowing
		)
	endif()

	# Debug definitions.
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		target_compile_definitions(${target} PUBLIC DEBUG _DEBUG)
	else()
		target_compile_definitions(${target} PUBLIC NDEBUG _NDEBUG)
	endif()

	# If windows, set the standard defines.
	if(WIN32)
		target_compile_definitions(${target} PUBLIC _WIN32 WIN32 _WINDOWS NOMINMAX)

		# If 64-bit windows...
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			target_compile_definitions(${target} PUBLIC _WIN64 WIN64)
		endif()
	endif()
endfunction()

MACRO(setup_versioning_data)
	# Version number in format X.YY.ZZ
	string(REPLACE "." ";" VERSION_LIST ${PROJECT_VERSION})
	list(GET VERSION_LIST 0 VER_X)
	list(GET VERSION_LIST 1 VER_Y)
	list(GET VERSION_LIST 2 VER_Z)
	set(VER_EXTRA "-beta" CACHE STRING "Extra version")

	# Build date Information
	string(TIMESTAMP VER_YEAR "%Y")
	string(TIMESTAMP VER_MONTH "%m")
	string(TIMESTAMP VER_DAY "%d")
	string(TIMESTAMP VER_HOUR "%H")
	string(TIMESTAMP VER_MINUTE "%M")

	set(VER_EXTRA "${VER_EXTRA} (${VER_YEAR}-${VER_MONTH}-${VER_DAY} ${VER_HOUR}:${VER_MINUTE})")
	set(VER_FULL "${VER_X}.${VER_Y}.${VER_Z}${VER_EXTRA}")

	set(APP_CREDITS "Joey, Nalin, Codr, and Cadavre")
	set(APP_VENDOR "Preagonal")

	STRING(REGEX REPLACE " " "-" VER_CPACK ${VER_FULL})
	STRING(REGEX REPLACE "[\(]" "" VER_CPACK ${VER_CPACK})
	STRING(REGEX REPLACE "[\)]" "" VER_CPACK ${VER_CPACK})
	STRING(REGEX REPLACE "(-[0-9]+:[0-9]+)" "" VER_CPACK ${VER_CPACK})
ENDMACRO()

function(generate_iconfig)
	# Generate version header from the above
	message(STATUS "[${PROJECT_NAME}] Generating IConfig.h")
	configure_file(
		${PROJECT_SOURCE_DIR}/server/include/IConfig.h.in
		${PROJECT_BINARY_DIR}/server/include/IConfig.h
	)
endfunction()

MACRO(subdir_list result curdir)
	FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
	SET(dirlist "")
	FOREACH(child ${children})
		IF(IS_DIRECTORY ${curdir}/${child})
			LIST(APPEND dirlist ${child})
		ENDIF()
	ENDFOREACH()
	SET(${result} ${dirlist})
ENDMACRO()

function(add_test_og TARGET_NAME TARGET_PATH)
	cmake_minimum_required(VERSION 3.22)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

	file(GLOB_RECURSE TESTS "${TARGET_PATH}/${TARGET_NAME}/*.cpp")

	add_executable(${TARGET_NAME} ${TESTS})
	target_compile_features(${TARGET_NAME} PUBLIC cxx_std_23)
	set_target_properties(${TARGET_NAME} PROPERTIES CXX_EXTENSIONS OFF)
	target_link_libraries(${TARGET_NAME} PRIVATE gs2lib ${APP_LIBRARY_NAME_TESTREF} Catch2::Catch2WithMain)
	target_include_directories(${TARGET_NAME} PRIVATE "${gs2lib_SOURCE_DIR}/include")
	target_include_directories(${APP_LIBRARY_NAME_TESTREF} PRIVATE "${gs2lib_SOURCE_DIR}/include")
	target_link_options(${TARGET_NAME} PRIVATE -static -fstack-protector)
	target_link_libraries(${TARGET_NAME} PUBLIC -static-libgcc -static-libstdc++)

	target_include_directories(${TARGET_NAME} PUBLIC ${GS2LIB_INCLUDE_DIRECTORY})

	target_include_directories(${TARGET_NAME} PUBLIC ${GS2COMPILER_INCLUDE_DIRECTORY})

	target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include)
	target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/level)
	target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/scripting)
	target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/scripting/v8)
	target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/scripting/interface)
	target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/misc)
	target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/utilities)
	target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/server/include/animation)

	add_dependencies(${TARGET_NAME} gs2lib ${APP_LIBRARY_NAME_TESTREF})

	list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

	include(CTest)
	include(Catch)
	catch_discover_tests(${TARGET_NAME})

	message(STATUS "Added test ${TARGET_NAME}")
endfunction()