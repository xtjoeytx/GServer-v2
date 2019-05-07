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
    add_library(${TARGET_NAME} ${SOURCES} ${HEADERS})

    # Enable ARC (automatic reference counting) for OS X build
    set_property(
        TARGET ${TARGET_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc"
    )
elseif(WIN32)
    add_library(${TARGET_NAME} ${SOURCES} ${HEADERS})
elseif(AROS)
    add_library(${TARGET_NAME} ${SOURCES} ${HEADERS})
elseif(AMIGA)
    add_library(${TARGET_NAME} ${SOURCES} ${HEADERS})
else()
    add_library(${TARGET_NAME} ${SOURCES} ${HEADERS})
endif()

set(INSTALL_DEST .)

install(TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_DEST})
