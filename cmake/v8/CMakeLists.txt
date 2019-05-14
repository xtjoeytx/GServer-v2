
set(BASE ${CMAKE_SOURCE_DIR}/dependencies/v8)
set(BASE_TOOLS ${CMAKE_SOURCE_DIR}/dependencies/depot_tools)
project(v8)

if(NOT EXISTS "${BASE}")
	add_custom_command(
		COMMAND
			${BASE_TOOLS}/fetch
			v8
		WORKING_DIRECTORY
			${BASE}/../
		DEPENDS
			${BASE_TOOLS}/fetch
		OUTPUT
			${BASE}/output-fetch.txt
	)
else()
	add_custom_command(
		COMMAND
			${BASE_TOOLS}/gclient sync -D
		WORKING_DIRECTORY
			${BASE}/
		DEPENDS
			${BASE_TOOLS}/gclient
		OUTPUT
			${BASE}/output-fetch.txt
	)
endif()

add_custom_command(
	COMMAND
		git
		checkout
		refs/tags/7.4.288.26
	WORKING_DIRECTORY
		${BASE}/
	DEPENDS
		${BASE_TOOLS}/gclient
		${BASE}/output-fetch.txt
	OUTPUT
		${BASE}/output-gclient1.txt
)

add_custom_command(
	COMMAND
		${BASE_TOOLS}/gclient sync -D
	WORKING_DIRECTORY
		${BASE}/
	DEPENDS
		${BASE_TOOLS}/gclient
		${BASE}/output-gclient1.txt
	OUTPUT
		${BASE}/output-gclient2.txt
)

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
	set(V8_TARGET_CPU "arm")
else()
	set(V8_TARGET_CPU "x64")
endif()

if(APPLE)
	set(V8_TARGET_OS "mac")
elseif(WIN32)
	set(V8_TARGET_OS "win")
else()
	set(V8_TARGET_OS "linux")
endif()

add_custom_command(
	COMMAND
		${BASE}/build/linux/sysroot_scripts/install-sysroot.py --arch=${V8_TARGET_CPU}
	WORKING_DIRECTORY
		${BASE}/
	DEPENDS
		${BASE}/tools/dev/v8gen.py
		${BASE}/output-gclient2.txt
	OUTPUT
		${BASE}/output-gclient3.txt
)

add_custom_command(
	COMMAND
		${BASE_TOOLS}/gn
		gen
		out.gn/${V8_TARGET_CPU}-${V8_TARGET_OS}.release.sample
		--args='is_component_build=false is_debug=false target_cpu="${V8_TARGET_CPU}" use_custom_libcxx=false v8_monolithic=true v8_use_external_startup_data=false'
	WORKING_DIRECTORY
		${BASE}/
	DEPENDS
		${BASE}/tools/dev/v8gen.py
		${BASE}/output-gclient3.txt
	OUTPUT
		${BASE}/output-v8gen.txt
)

add_custom_target(
	v8
	COMMAND
		${BASE_TOOLS}/ninja -j 8
		-C out.gn/${V8_TARGET_CPU}-${V8_TARGET_OS}.release.sample
		v8_monolith
	WORKING_DIRECTORY
		${BASE}/
	DEPENDS
		${BASE}/output-v8gen.txt
)

set(V8_DIR ${BASE})
set(V8_INCLUDE_DIR "${BASE}/include" PARENT_SCOPE)
set(V8_LIBRARY "${BASE}/out.gn/${V8_TARGET_CPU}-${V8_TARGET_OS}.release.sample/obj/libv8_monolith.a" PARENT_SCOPE)
