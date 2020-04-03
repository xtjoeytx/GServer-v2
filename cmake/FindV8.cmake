# Courtesy of: https://raw.githubusercontent.com/gwaldron/osgearth/master/CMakeModules/FindV8.cmake
#
# Locate V8
# This module defines
# V8_LIBRARY
# V8_FOUND, if false, do not try to link to V8
# V8_INCLUDE_DIR, where to find the headers

IF (NOT $ENV{V8_DIR} STREQUAL "")
	SET(V8_DIR $ENV{V8_DIR})
ENDIF()


SET(V8_LIBRARY_SEARCH_PATHS
	${V8_DIR}/
	${V8_DIR}/lib/
	${V8_DIR}/build/Release/lib/
	${V8_DIR}/build/Release/lib/third_party/icu/
	${V8_DIR}/build/Release/obj/
	${V8_DIR}/build/Release/obj/third_party/icu/
	${V8_DIR}/out/ia32.release/lib.target/
	${V8_DIR}/out/ia32.release/lib.target/third_party/icu/
	${V8_DIR}/out/ia32.release/obj/
	${V8_DIR}/out/ia32.release/obj/third_party/icu/
	${V8_DIR}/out.gn/ia32.release/lib.target/
	${V8_DIR}/out.gn/ia32.release/lib.target/third_party/icu/
	${V8_DIR}/out.gn/ia32.release/obj/
	${V8_DIR}/out.gn/ia32.release/obj/third_party/icu/
	${V8_DIR}/out/x64.release/lib.target/
	${V8_DIR}/out/x64.release/lib.target/third_party/icu/
	${V8_DIR}/out/x64.release/obj/
	${V8_DIR}/out/x64.release/obj/third_party/icu/
	${V8_DIR}/out.gn/x64.release/lib.target/
	${V8_DIR}/out.gn/x64.release/lib.target/third_party/icu/
	${V8_DIR}/out.gn/x64.release/obj/
	${V8_DIR}/out.gn/x64.release/obj/third_party/icu/
	${V8_DIR}/out.gn/x64.release.sample/lib.target/
	${V8_DIR}/out.gn/x64.release.sample/lib.target/third_party/icu/
	${V8_DIR}/out.gn/x64.release.sample/obj/
	${V8_DIR}/out.gn/x64.release.sample/obj/third_party/icu/
	${PROJECT_SOURCE_DIR}/dependencies/v8/out.gn/x64.release/lib.target/
	${PROJECT_SOURCE_DIR}/dependencies/v8/out.gn/x64.release/lib.target/third_party/icu/
	${PROJECT_SOURCE_DIR}/dependencies/v8/out.gn/x64.release/obj/
	${PROJECT_SOURCE_DIR}/dependencies/v8/out.gn/x64.release/obj/third_party/icu/
	${PROJECT_SOURCE_DIR}/dependencies/v8/out.gn/x64.release.sample/lib.target/
	${PROJECT_SOURCE_DIR}/dependencies/v8/out.gn/x64.release.sample/lib.target/third_party/icu/
	${PROJECT_SOURCE_DIR}/dependencies/v8/out.gn/x64.release.sample/obj/
	${PROJECT_SOURCE_DIR}/dependencies/v8/out.gn/x64.release.sample/obj/third_party/icu/
	${PROJECT_SOURCE_DIR}/dependencies/v8/out.gn/arm.release/lib.target/
	${PROJECT_SOURCE_DIR}/dependencies/v8/out.gn/arm.release/lib.target/third_party/icu/
	${PROJECT_SOURCE_DIR}/dependencies/v8/out.gn/arm.release/obj/
	${PROJECT_SOURCE_DIR}/dependencies/v8/out.gn/arm.release/obj/third_party/icu/
	${PROJECT_SOURCE_DIR}/dependencies/v8/out.gn/arm.release.sample/lib.target/
	${PROJECT_SOURCE_DIR}/dependencies/v8/out.gn/arm.release.sample/lib.target/third_party/icu/
	${PROJECT_SOURCE_DIR}/dependencies/v8/out.gn/arm.release.sample/obj/
	${PROJECT_SOURCE_DIR}/dependencies/v8/out.gn/arm.release.sample/obj/third_party/icu/
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local/lib
	/usr/lib
	/sw/lib
	/opt/local/lib
	/opt/csw/lib
	/opt/lib
	/usr/freeware/lib64
)

FIND_PATH(V8_INCLUDE_DIR v8.h
	${V8_DIR}
	${V8_DIR}/include
	${PROJECT_SOURCE_DIR}/dependencies/v8
	${PROJECT_SOURCE_DIR}/dependencies/v8/include
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local/include
	/usr/include
	/sw/include # Fink
	/opt/local/include # DarwinPorts
	/opt/csw/include # Blastwave
	/opt/include
	/usr/freeware/include
	/devel
)

FIND_LIBRARY(V8_LIBRARY
	NAMES libv8_monolith.a v8_monolith.a v8_monolith libv8_monolith
	PATHS ${V8_LIBRARY_SEARCH_PATHS}
)

SET(V8_FOUND "NO")

IF (NOT UNIX)
	IF (V8_LIBRARY AND V8_INCLUDE_DIR)
		SET(V8_FOUND "YES")
	ENDIF()
ELSEIF(V8_LIBRARY AND V8_INCLUDE_DIR)
	SET(V8_FOUND "YES")
ENDIF(NOT UNIX)

