#
#  bin/CMakeLists.txt
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

if(V8NPCSERVER)
	file(GLOB DOCS
		 ../docs/npcserver.txt
		 )

	install(FILES ${DOCS} DESTINATION "./docs")

	file(GLOB SERVERS_DEFAULT_BOOTSTRAP
		 servers/default/bootstrap.js
		 )

	install(FILES ${SERVERS_DEFAULT_BOOTSTRAP} DESTINATION "./servers/default/")

	file(GLOB SERVERS_DEFAULT_NPCS
		 servers/default/npcs/npcControl-NPC.txt
		 )

	install(FILES ${SERVERS_DEFAULT_NPCS} DESTINATION "./servers/default/npcs")

	file(GLOB SERVERS_DEFAULT_NPCLOGS
			servers/default/logs/npclog.txt
			servers/default/logs/scriptlog.txt
		 )

	install(FILES ${SERVERS_DEFAULT_NPCLOGS} DESTINATION "./servers/default/logs")
	
	install(DIRECTORY DESTINATION "./servers/default/scripts")
endif()

file(GLOB TEXT
	 changelog.txt
	 miniupnp_licence.txt
	 servers.txt
	 readme.txt
	 )

install(FILES ${TEXT} DESTINATION ".")

file(GLOB SERVERS_DEFAULT
	 servers/default/serverflags.txt
	 )

install(FILES ${SERVERS_DEFAULT} DESTINATION "./servers/default")

file(GLOB SERVERS_DEFAULT_ACCOUNTS
	 servers/default/accounts/defaultaccount.txt
	 servers/default/accounts/YOURACCOUNT.txt
	 )

install(FILES ${SERVERS_DEFAULT_ACCOUNTS} DESTINATION "./servers/default/accounts")

file(GLOB SERVERS_DEFAULT_CONFIG
	 servers/default/config/rcmessage.txt
	 servers/default/config/adminconfig.txt
	 servers/default/config/serveroptions.txt
	 servers/default/config/foldersconfig.txt
	 servers/default/config/allowedversions.txt
	 servers/default/config/rules.txt
	 servers/default/config/ipbans.txt
	 servers/default/config/rchelp.txt
	 servers/default/config/rules.example.txt
	 servers/default/config/servermessage.html
	 )

install(FILES ${SERVERS_DEFAULT_CONFIG} DESTINATION "./servers/default/config")

file(GLOB SERVERS_DEFAULT_DOCUMENTS
	 servers/default/documents/docu_wordfilter.txt
	 servers/default/documents/rules.txt
	 )

install(FILES ${SERVERS_DEFAULT_DOCUMENTS} DESTINATION "./servers/default/documents")

file(GLOB SERVERS_DEFAULT_EXECSCRIPTS
	 servers/default/execscripts/readme.txt
	 )

install(FILES ${SERVERS_DEFAULT_EXECSCRIPTS} DESTINATION "./servers/default/execscripts")

file(GLOB SERVERS_DEFAULT_GUILDS
	 servers/default/guilds/guildExample.txt
	 )

install(FILES ${SERVERS_DEFAULT_GUILDS} DESTINATION "./servers/default/guilds")

file(GLOB SERVERS_DEFAULT_LOGS
	 servers/default/logs/rclog.txt
	 servers/default/logs/serverlog.txt
	 )

install(FILES ${SERVERS_DEFAULT_LOGS} DESTINATION "./servers/default/logs")


file(GLOB SERVERS_DEFAULT_TRANSLATIONS
	 servers/default/translations/english.po
	 servers/default/translations/deutsch.po
	 servers/default/translations/italiano.po
	 servers/default/translations/nederlands.po
	 servers/default/translations/norsk.po
	 servers/default/translations/svenska.po
	 )

install(FILES ${SERVERS_DEFAULT_TRANSLATIONS} DESTINATION "./servers/default/translations")

file(GLOB SERVERS_DEFAULT_WEAPONS
	 servers/default/weapons/weapon-gr_movement.txt
	 )

install(FILES ${SERVERS_DEFAULT_WEAPONS} DESTINATION "./servers/default/weapons")

file(GLOB SERVERS_DEFAULT_WORLD
	 servers/default/world/readme.txt
	 servers/default/world/onlinestartlocal.nw
	 )

install(FILES ${SERVERS_DEFAULT_WORLD} DESTINATION "./servers/default/world")

install(DIRECTORY DESTINATION "./servers/default/world/bodies")
install(DIRECTORY DESTINATION "./servers/default/world/ganis")
install(DIRECTORY DESTINATION "./servers/default/world/hats")
install(DIRECTORY DESTINATION "./servers/default/world/heads")
install(DIRECTORY DESTINATION "./servers/default/world/images")
install(DIRECTORY DESTINATION "./servers/default/world/shields")
install(DIRECTORY DESTINATION "./servers/default/world/sounds")
install(DIRECTORY DESTINATION "./servers/default/world/swords")
install(DIRECTORY DESTINATION "./servers/default/world/levels")

install(DIRECTORY DESTINATION "./servers/default/world/global")
install(DIRECTORY DESTINATION "./servers/default/world/global/bodies")
install(DIRECTORY DESTINATION "./servers/default/world/global/heads")
install(DIRECTORY DESTINATION "./servers/default/world/global/shields")
install(DIRECTORY DESTINATION "./servers/default/world/global/swords")