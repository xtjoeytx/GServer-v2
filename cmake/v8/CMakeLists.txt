
set(BASE ${CMAKE_SOURCE_DIR}/dependencies/v8)
set(BASE_TOOLS ${CMAKE_SOURCE_DIR}/dependencies/depot_tools)
project(v8)

if(NOT EXISTS "${BASE}/../.gclient")
	add_custom_command(
		COMMAND
			rm -rf ${BASE}/../.gclient ${BASE}/../.gclient_entries ${BASE} 
		WORKING_DIRECTORY
			${BASE}/../
		DEPENDS
			${BASE_TOOLS}/fetch
		OUTPUT
			${BASE}/output-rm.txt
	)

	add_custom_command(
		COMMAND
			${BASE_TOOLS}/fetch
			v8
		WORKING_DIRECTORY
			${BASE}/../
		DEPENDS
			${BASE_TOOLS}/fetch
			${BASE}/output-rm.txt
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

add_custom_command(
    COMMAND
        ${PYTHON_EXECUTABLE}
        ${BASE}/tools/dev/v8gen.py
		x64.release.sample
	WORKING_DIRECTORY
		${BASE}/
    DEPENDS
        ${BASE}/tools/dev/v8gen.py
		${BASE}/output-gclient2.txt
    OUTPUT
        ${BASE}/output-v8gen.txt
)

add_custom_target(
	v8
	COMMAND
		${BASE_TOOLS}/ninja -j 8 
		-C out.gn/x64.release.sample
		v8_monolith
	WORKING_DIRECTORY
		${BASE}/
	DEPENDS
		${BASE}/output-v8gen.txt
)

set(V8_DIR ${BASE})
set(V8_INCLUDE_DIR "${BASE}/include" PARENT_SCOPE)
set(V8_LIBRARY "${BASE}/out.gn/x64.release.sample/obj/libv8_monolith.a" PARENT_SCOPE)
