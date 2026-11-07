find_program(DOTNET_EXECUTABLE NAMES dotnet REQUIRED)

if(NOT DOTNET_RID)
    # 1. Map vcpkg target architecture to .NET architecture string
    if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
        set(DOTNET_ARCH "x64")
    elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
        set(DOTNET_ARCH "arm64")
    elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
        set(DOTNET_ARCH "x86")
    elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm")
        set(DOTNET_ARCH "arm")
    else()
        set(DOTNET_ARCH ${VCPKG_TARGET_ARCHITECTURE})
    endif()

    # 2. Map vcpkg target platform / triplet name to .NET OS string
    if(NOT DEFINED VCPKG_CMAKE_SYSTEM_NAME OR VCPKG_CMAKE_SYSTEM_NAME STREQUAL "" OR VCPKG_CMAKE_SYSTEM_NAME STREQUAL "MinGW")
        # Windows host/target (Empty system name in vcpkg implies Windows Desktop)
        set(DOTNET_OS "win")

    elseif(VCPKG_CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Determine if we are targeting a musl system based on the vcpkg triplet name
        if(TARGET_TRIPLET MATCHES "musl")
            set(DOTNET_OS "linux-musl")
        else()
            set(DOTNET_OS "linux")
        endif()

    elseif(VCPKG_CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(DOTNET_OS "osx")
    else()
        string(TOLOWER ${VCPKG_CMAKE_SYSTEM_NAME} DOTNET_OS)
    endif()

    # 3. Combine into the DOTNET_RID variable
    set(DOTNET_RID "${DOTNET_OS}-${DOTNET_ARCH}")

    message(STATUS "Detected .NET RID: ${DOTNET_RID}")
endif()

vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL https://github.com/Preagonal/Preagonal.Scripting.GS2Compiler.git
    REF 8420fa2769e2e5c7ff56c3a28a5159537a41b390
    HEAD_REF dev
)

set(DOTNET_PROJECT_PATH "${SOURCE_PATH}")
set(DOTNET_PROJECT
    "${DOTNET_PROJECT_PATH}/Preagonal.Scripting.GS2Compiler.Native/Preagonal.Scripting.GS2Compiler.Native.csproj")

if(NOT EXISTS "${DOTNET_PROJECT}")
    message(FATAL_ERROR "Project not found: ${DOTNET_PROJECT}")
endif()

# The default AssemblyName is the project filename. If the .csproj overrides
# AssemblyName, set GS2COMPILER_NATIVE_NAME to that value here or in the triplet.
if(NOT GS2COMPILER_NATIVE_NAME)
    get_filename_component(GS2COMPILER_NATIVE_NAME "gs2compiler" NAME_WLE)
endif()

set(_publish_dir "${CURRENT_BUILDTREES_DIR}/publish-${TARGET_TRIPLET}")
set(_native_dir "${CURRENT_BUILDTREES_DIR}/native-${TARGET_TRIPLET}-release")
file(REMOVE_RECURSE "${_publish_dir}" "${_native_dir}")
file(MAKE_DIRECTORY "${_publish_dir}" "${_native_dir}")

message(STATUS "Publishing GS2 compiler for ${DOTNET_RID}")
vcpkg_execute_required_process(
    COMMAND "${DOTNET_EXECUTABLE}" publish "${DOTNET_PROJECT}"
        --configuration Release
        --runtime "${DOTNET_RID}"
        --output "${_publish_dir}"
        -p:PublishAot=true
        -p:NativeLib=Shared
        -p:UseNativeLibPrefix=false
        "-p:NativeOutputPath=${_native_dir}/"
    WORKING_DIRECTORY "${DOTNET_PROJECT_PATH}"
    LOGNAME "dotnet-publish-${TARGET_TRIPLET}"
)

# Package a shared NativeAOT component, even when a triplet prefers static libs.
# .NET's Release build is reused by both Debug and Release C++ consumers.
set(VCPKG_POLICY_DLLS_IN_STATIC_LIBRARY enabled)
set(VCPKG_POLICY_ONLY_RELEASE_CRT enabled)
set(GS2COMPILER_IMPLIB_RELATIVE_PATH "")
set(GS2COMPILER_SONAME "")

if(VCPKG_TARGET_IS_WINDOWS AND NOT VCPKG_TARGET_IS_UWP)
    set(_library_filename "${GS2COMPILER_NATIVE_NAME}.dll")
    set(_library_subdir bin)
elseif(VCPKG_TARGET_IS_LINUX)
    set(_library_filename "${GS2COMPILER_NATIVE_NAME}.so")
    set(_library_subdir lib)
elseif(VCPKG_TARGET_IS_OSX)
    set(_library_filename "${GS2COMPILER_NATIVE_NAME}.dylib")
    set(_library_subdir lib)
else()
    message(FATAL_ERROR "Unsupported GS2 compiler package platform: ${TARGET_TRIPLET}")
endif()


set(_published_library "${_publish_dir}/${_library_filename}")
if(NOT EXISTS "${_published_library}")
    message(FATAL_ERROR
        "Native library not found: ${_published_library}\n"
        "If the project overrides AssemblyName, set GS2COMPILER_NATIVE_NAME "
        "to that name. The project must publish a NativeAOT shared library.")
endif()

file(INSTALL "${_published_library}"
    DESTINATION "${CURRENT_PACKAGES_DIR}/${_library_subdir}")
set(GS2COMPILER_LIBRARY_RELATIVE_PATH "${_library_subdir}/${_library_filename}")

if(VCPKG_TARGET_IS_WINDOWS)
    # NativeAOT writes the import library to NativeOutputPath. The publish
    # directory does not necessarily contain it.
    set(_import_library "${_native_dir}/${GS2COMPILER_NATIVE_NAME}.lib")
    if(NOT EXISTS "${_import_library}")
        message(FATAL_ERROR "Native import library not found: ${_import_library}")
    endif()
    file(INSTALL "${_import_library}" DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
    set(GS2COMPILER_IMPLIB_RELATIVE_PATH "lib/${GS2COMPILER_NATIVE_NAME}.lib")

    # vcpkg's Debug app-local DLL deployment searches debug/bin.
    file(INSTALL "${_published_library}"
        DESTINATION "${CURRENT_PACKAGES_DIR}/debug/bin")
    file(INSTALL "${_import_library}"
        DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib")

    if(EXISTS "${_publish_dir}/${GS2COMPILER_NATIVE_NAME}.pdb")
        file(INSTALL "${_publish_dir}/${GS2COMPILER_NATIVE_NAME}.pdb"
            DESTINATION "${CURRENT_PACKAGES_DIR}/bin")
        file(INSTALL "${_publish_dir}/${GS2COMPILER_NATIVE_NAME}.pdb"
            DESTINATION "${CURRENT_PACKAGES_DIR}/debug/bin")
    endif()
else()
    # Unix imports use the same Release library for every build configuration.
    set(VCPKG_POLICY_MISMATCHED_NUMBER_OF_BINARIES enabled)
    if(VCPKG_TARGET_IS_LINUX)
        # A SONAME prevents consumers from recording an absolute package path
        # in DT_NEEDED when they link this .so by its full path.
        vcpkg_find_acquire_program(PATCHELF)
        vcpkg_execute_required_process(
            COMMAND "${PATCHELF}" --set-soname "${_library_filename}"
                "${CURRENT_PACKAGES_DIR}/${GS2COMPILER_LIBRARY_RELATIVE_PATH}"
            WORKING_DIRECTORY "${CURRENT_BUILDTREES_DIR}"
            LOGNAME "gs2compiler-soname-${TARGET_TRIPLET}"
        )
        set(GS2COMPILER_SONAME "${_library_filename}")
        if(EXISTS "${_published_library}.dbg")
            file(INSTALL "${_published_library}.dbg"
                DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
        endif()
    endif()
endif()

# Install existing public C/C++ declarations when the .NET repository supplies
# an include directory. Otherwise the consumer can use its existing header.
if(NOT DEFINED GS2COMPILER_INCLUDE_DIR)
    set(GS2COMPILER_INCLUDE_DIR "${DOTNET_PROJECT_PATH}/include")
endif()
set(GS2COMPILER_INCLUDE_RELATIVE_PATH "")
set(_gs2compiler_public_files "")
if(IS_DIRECTORY "${GS2COMPILER_INCLUDE_DIR}")
    file(GLOB_RECURSE _gs2compiler_public_files LIST_DIRECTORIES FALSE
        "${GS2COMPILER_INCLUDE_DIR}/*")
endif()
if(_gs2compiler_public_files)
    file(INSTALL "${GS2COMPILER_INCLUDE_DIR}/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/include")
    set(GS2COMPILER_INCLUDE_RELATIVE_PATH include)
else()
    set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
endif()

file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/share/${PORT}")
configure_file("${CURRENT_PORT_DIR}/gs2compiler-config.cmake.in"
    "${CURRENT_PACKAGES_DIR}/share/${PORT}/gs2compiler-config.cmake" @ONLY)
file(INSTALL "${CURRENT_PORT_DIR}/usage"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

# Install real license text if available. Do not fabricate licensing terms or
# silently suppress the copyright warning when the repository has no license.
if(NOT DEFINED GS2COMPILER_LICENSE_FILE)
    foreach(_license_name IN ITEMS LICENSE LICENSE.txt LICENSE.md COPYING COPYING.txt)
        if(EXISTS "${DOTNET_PROJECT_PATH}/${_license_name}")
            set(GS2COMPILER_LICENSE_FILE "${DOTNET_PROJECT_PATH}/${_license_name}")
            break()
        endif()
    endforeach()
endif()
if(GS2COMPILER_LICENSE_FILE)
    if(NOT EXISTS "${GS2COMPILER_LICENSE_FILE}")
        message(FATAL_ERROR "License file not found: ${GS2COMPILER_LICENSE_FILE}")
    endif()
    vcpkg_install_copyright(FILE_LIST "${GS2COMPILER_LICENSE_FILE}")
endif()
