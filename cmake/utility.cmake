function(set_default_compiler_options target)
	# C++23 mode.
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

	# If windows, set the standard defines.
	if(WIN32)
		target_compile_definitions(${target} PUBLIC _WIN32 WIN32 _WINDOWS NOMINMAX)

		# If 64-bit windows...
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			target_compile_definitions(${target} PUBLIC _WIN64 WIN64)
		endif()
	endif()
endfunction()

function(generate_iconfig)
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
	set(APP_VENDOR "OpenGraal")

	STRING(REGEX REPLACE " " "-" VER_CPACK ${VER_FULL})
	STRING(REGEX REPLACE "[\(]" "" VER_CPACK ${VER_CPACK})
	STRING(REGEX REPLACE "[\)]" "" VER_CPACK ${VER_CPACK})
	STRING(REGEX REPLACE "(-[0-9]+:[0-9]+)" "" VER_CPACK ${VER_CPACK})

	# Generate version header from the above
	message("Generating IConfig.h")
	configure_file(
		${PROJECT_SOURCE_DIR}/server/include/IConfig.h.in
		${PROJECT_BINARY_DIR}/server/include/IConfig.h
	)
endfunction()
