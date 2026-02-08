# Allow CC environment variable to set the C compiler, otherwise default to gcc-15 from Homebrew.
if(DEFINED ENV{CC})
	message(STATUS "Using C compiler from CC environment variable: $ENV{CC}")
	set(CMAKE_C_COMPILER "$ENV{CC}")
else()
	message(STATUS "CC environment variable not set, defaulting to /opt/homebrew/bin/gcc-15")
	set(CMAKE_C_COMPILER "/opt/homebrew/bin/gcc-15")
endif()

# Allow CXX environment variable to set the C++ compiler, otherwise default to g++-15 from Homebrew.
if(DEFINED ENV{CXX})
	message(STATUS "Using C++ compiler from CXX environment variable: $ENV{CXX}")
	set(CMAKE_CXX_COMPILER "$ENV{CXX}")
else()
	message(STATUS "CXX environment variable not set, defaulting to /opt/homebrew/bin/g++-15")
	set(CMAKE_CXX_COMPILER "/opt/homebrew/bin/g++-15")
endif()

# If the CMAKE_OSX_SYSROOT variable is not defined, set it to the 26.2 macOS SDK path.
if(NOT DEFINED CMAKE_OSX_SYSROOT)
	set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX26.2.sdk)
endif()

#-------------------------------------------------------------------------------
# /scripts/toolchains/osx.cmake
#-------------------------------------------------------------------------------

if(NOT _VCPKG_OSX_TOOLCHAIN)
    set(_VCPKG_OSX_TOOLCHAIN 1)

    if(POLICY CMP0056)
        cmake_policy(SET CMP0056 NEW)
    endif()
    if(POLICY CMP0066)
        cmake_policy(SET CMP0066 NEW)
    endif()
    if(POLICY CMP0067)
        cmake_policy(SET CMP0067 NEW)
    endif()
    if(POLICY CMP0137)
        cmake_policy(SET CMP0137 NEW)
    endif()
    list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
        VCPKG_CRT_LINKAGE VCPKG_TARGET_ARCHITECTURE
        VCPKG_C_FLAGS VCPKG_CXX_FLAGS
        VCPKG_C_FLAGS_DEBUG VCPKG_CXX_FLAGS_DEBUG
        VCPKG_C_FLAGS_RELEASE VCPKG_CXX_FLAGS_RELEASE
        VCPKG_LINKER_FLAGS VCPKG_LINKER_FLAGS_RELEASE VCPKG_LINKER_FLAGS_DEBUG
    )

    set(CMAKE_SYSTEM_NAME Darwin CACHE STRING "")

    set(CMAKE_MACOSX_RPATH ON CACHE BOOL "")

    if(NOT DEFINED CMAKE_SYSTEM_PROCESSOR)
        if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
           set(CMAKE_SYSTEM_PROCESSOR x86_64 CACHE STRING "")
        elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
           set(CMAKE_SYSTEM_PROCESSOR x86 CACHE STRING "")
        elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
           set(CMAKE_SYSTEM_PROCESSOR arm64 CACHE STRING "")
        else()
           set(CMAKE_SYSTEM_PROCESSOR "${CMAKE_HOST_SYSTEM_PROCESSOR}" CACHE STRING "")
        endif()
    endif()

    if(DEFINED VCPKG_CMAKE_SYSTEM_VERSION)
        set(CMAKE_SYSTEM_VERSION "${VCPKG_CMAKE_SYSTEM_VERSION}" CACHE STRING "" FORCE)
    endif()

    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL CMAKE_HOST_SYSTEM_PROCESSOR)
            set(CMAKE_CROSSCOMPILING OFF CACHE STRING "")
        elseif(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
            # arm64 macOS can run x64 binaries
            set(CMAKE_CROSSCOMPILING OFF CACHE STRING "")
        endif()

        if(NOT DEFINED CMAKE_SYSTEM_VERSION)
            set(CMAKE_SYSTEM_VERSION "${CMAKE_HOST_SYSTEM_VERSION}" CACHE STRING "")
        endif()
    endif()

    string(APPEND CMAKE_C_FLAGS_INIT " -fPIC ${VCPKG_C_FLAGS} ")
    string(APPEND CMAKE_CXX_FLAGS_INIT " -fPIC ${VCPKG_CXX_FLAGS} ")
    string(APPEND CMAKE_C_FLAGS_DEBUG_INIT " ${VCPKG_C_FLAGS_DEBUG} ")
    string(APPEND CMAKE_CXX_FLAGS_DEBUG_INIT " ${VCPKG_CXX_FLAGS_DEBUG} ")
    string(APPEND CMAKE_C_FLAGS_RELEASE_INIT " ${VCPKG_C_FLAGS_RELEASE} ")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE_INIT " ${VCPKG_CXX_FLAGS_RELEASE} ")

    string(APPEND CMAKE_MODULE_LINKER_FLAGS_INIT " ${VCPKG_LINKER_FLAGS} ")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_INIT " ${VCPKG_LINKER_FLAGS} ")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT " ${VCPKG_LINKER_FLAGS} ")
    string(APPEND CMAKE_MODULE_LINKER_FLAGS_DEBUG_INIT " ${VCPKG_LINKER_FLAGS_DEBUG} ")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_DEBUG_INIT " ${VCPKG_LINKER_FLAGS_DEBUG} ")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT " ${VCPKG_LINKER_FLAGS_DEBUG} ")
    string(APPEND CMAKE_MODULE_LINKER_FLAGS_RELEASE_INIT " ${VCPKG_LINKER_FLAGS_RELEASE} ")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE_INIT " ${VCPKG_LINKER_FLAGS_RELEASE} ")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT " ${VCPKG_LINKER_FLAGS_RELEASE} ")
endif()
