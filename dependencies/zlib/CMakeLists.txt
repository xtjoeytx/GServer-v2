#
#  dependencies/zlib/CMakeLists.txt
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
    compress.c
    deflate.c
    infback.c
    crc32.c
    gzwrite.c
    gzlib.c
    inflate.c
    adler32.c
    uncompr.c
    gzclose.c
    inftrees.c
    trees.c
    inffast.c
    zutil.c
    gzread.c
)

set(
    HEADERS
    trees.h
    zconf.h
    inffixed.h
    gzguts.h
    zutil.h
    inffast.h
    inftrees.h
    crc32.h
    zlib.h
    inflate.h
    deflate.h
)

set(TARGET_NAME "z")

if(APPLE)
    add_library(${TARGET_NAME} ${SOURCES} ${HEADERS})

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
