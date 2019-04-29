#
#  bin/servers/default/CMakeLists.txt
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

add_subdirectory(accounts)
add_subdirectory(config)
add_subdirectory(documents)
add_subdirectory(execscripts)
add_subdirectory(guilds)
add_subdirectory(logs)
add_subdirectory(translations)
add_subdirectory(weapons)
add_subdirectory(world)

file(GLOB FILES
    serverflags.txt
)

set(INSTALL_DEST "./servers/default")

install(FILES ${FILES} DESTINATION ${INSTALL_DEST})
