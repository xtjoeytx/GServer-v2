set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES arm64)

set(VCPKG_CXX_FLAGS_RELEASE " -Wno-error=type-limits -Wno-type-limits -Wno-error=stringop-overflow -Wno-stringop-overflow ")
set(VCPKG_C_FLAGS_RELEASE " -Wno-error=type-limits -Wno-type-limits -Wno-error=stringop-overflow -Wno-stringop-overflow ")

set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/osx-gcc-15-toolchain.cmake")
message(STATUS "Using chainload toolchain file: ${VCPKG_CHAINLOAD_TOOLCHAIN_FILE}")
