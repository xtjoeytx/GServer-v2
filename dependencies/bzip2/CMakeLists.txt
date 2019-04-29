#
#  dependencies/bzip2/CMakeLists.txt
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
    blocksort.c
    bz2compress.c
    randtable.c
    huffman.c
    crctable.c
    decompress.c
    bzlib.c
)

set(
    HEADERS
    bzlib.h
    bzlib_private.h
)

set(TARGET_NAME "bzip2")

if(APPLE)
    add_executable(${TARGET_NAME} MACOSX_BUNDLE ${SOURCES} ${HEADERS})

    # Xcode can deal with Interface Builder xibs automatically - if we are not
    # generating for Xcode, then we must manually compile and install any xibs
    if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
        # Locate ibtool
        find_program(IBTOOL ibtool HINTS /usr/bin ${OSX_DEVELOPER_ROOT}/usr/bin)
        if(IBTOOL STREQUAL "IBTOOL-NOTFOUND")
            message(SEND_ERROR "Unable to find ibtool. Is Xcode installed?")
        endif()

        # The Interface Builder xib file to be compiled
        set(
            XIB_FILE
            ${PROJECT_SOURCE_DIR}/src/tracker/cocoa/resources/Application.xib
        )

        # Destination for compiled xib
        set(RESOURCES_DIR $<TARGET_FILE_DIR:${TARGET_NAME}>/../Resources)

        # Ensure the destination directory of the compiled xib exists
        add_custom_command(
            TARGET ${TARGET_NAME} PRE_BUILD COMMAND mkdir -p ${RESOURCES_DIR}
        )

        # Compile the xib file
        add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD COMMAND ${IBTOOL} --errors
            --warnings --notices --output-format human-readable-text
            --minimum-deployment-target 10.7 --compile
            ${RESOURCES_DIR}/${MACOSX_BUNDLE_NSMAIN_NIB_FILE}.nib ${XIB_FILE}
            COMMENT "Compiling ${XIB_FILE}"
        )
    endif()

    # Pass in the Info.plist template, whose variables are defined in the
    # top-level CMakeLists.txt
    set_target_properties(
        ${TARGET_NAME}
        PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST
        ${PROJECT_SOURCE_DIR}/src/tracker/cocoa/resources/Info.plist.in
    )

    # Enable ARC (automatic reference counting) for OS X build
    set_property(
        TARGET ${TARGET_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc"
    )

    target_link_libraries(${TARGET_NAME} ${ZLIB_LIBRARIES} ${BZIP2_LIBRARIES} ${MINIUPNP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
elseif(WIN32)
    add_library(${TARGET_NAME} ${SOURCES} ${HEADERS})
elseif(AROS)
    add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS})

    target_link_libraries(${TARGET_NAME} ${ZLIB_LIBRARIES} ${BZIP2_LIBRARIES} ${MINIUPNP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
elseif(AMIGA)
    add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS})
    if(MORPHOS OR AMIGAOS3 AND NOT WARPOS)
        target_link_libraries(${TARGET_NAME} ${ZLIB_LIBRARIES} ${BZIP2_LIBRARIES} ${MINIUPNP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
    else()
        target_link_libraries(${TARGET_NAME} ${ZLIB_LIBRARIES} ${BZIP2_LIBRARIES} ${MINIUPNP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
    endif()
else()
    add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS})

    target_link_libraries(${TARGET_NAME} ${ZLIB_LIBRARIES} ${BZIP2_LIBRARIES} ${MINIUPNP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif()

set(INSTALL_DEST .)

install(TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DEST})
