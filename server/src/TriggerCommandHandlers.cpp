#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include <CString.h>
#include <IEnums.h>

#include <FileSystem.h>
#include <Server.h>
#include <level/Level.h>
#include <npcserver/NPCServer.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <object/Weapon.h>
#include <player/PlayerClient.h>
#include <player/PlayerProps.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommandDispatcher.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

void Server::createTriggerCommands(TriggerDispatcher::Builder builder)
{
	auto& dispatcher = m_triggerActionDispatcher;

	builder.registerCommand("serverside", [&](Player* player, std::vector<std::string>& triggerData)
	{
		if (!hasNPCServer())
			return false;

		if (triggerData.size() > 2)
		{
			// triggeraction 0,0,serverside,weaponname,params...;
			// Triggers on a player's weapons.
			if (auto weapon = getWeapon(triggerData[1]); weapon != nullptr)
				weapon->scripting.events.addEvent(ScriptEventType::CUSTOM, source::FromPlayer(player->getId()), "serverside"s, string::toCSV(triggerData | std::views::drop(2)));
		}
		return true;
	});

	builder.registerCommand("servernpc", [&](Player* player, std::vector<std::string>& triggerData)
	{
		if (!hasNPCServer())
			return false;

		if (triggerData.size() > 2)
		{
			// triggeraction 0,0,servernpc,npcname,params...;
			if (auto npcServer = getNPCServer(); npcServer != nullptr)
			{
				if (auto npc = npcServer->getNPCByName(triggerData[1]).lock(); npc != nullptr)
					npc->scripting.events.addEvent(ScriptEventType::CUSTOM, source::FromPlayer(player->getId()), "serverside"s, string::toCSV(triggerData | std::views::drop(2)));
			}
		}
		return true;
	});

	builder.registerCommand("gr.serverlist", [&](Player* player, std::vector<std::string>& triggerData)
	{
		auto& listServer = getServerList();
		const auto& serverList = listServer.getServerList();

		CString actionData("clientside,-Serverlist_v4,updateservers,");
		for (auto& serverData : serverList)
			actionData << CString(serverData.first).gtokenize() << "," << CString(serverData.second) << ",";

		player->sendPacket(CString() >> (char)PLO_TRIGGERACTION >> (short)0 >> (int)0 >> (char)0 >> (char)0 << actionData);
		return true;
	});

	// Weapon management
	builder.registerCommand("gr.addweapon", [&](Player* player, std::vector<std::string>& triggerData)
	{
		if (!getSettings().getBool("triggerhack_weapons", false))
			return false;

		for (auto i = 1; i < triggerData.size(); ++i)
			player->addWeapon(string::trim(triggerData[i]));
		return true;
	});

	builder.registerCommand("gr.deleteweapon", [&](Player* player, std::vector<std::string>& triggerData)
	{
		if (!getSettings().getBool("triggerhack_weapons", false))
			return false;

		for (auto i = 1; i < triggerData.size(); ++i)
			player->deleteWeapon(string::trim(triggerData[i]));
		return true;
	});

	// Guild management
	builder.registerCommand("gr.addguildmember", [&](Player* player, std::vector<std::string>& triggerData)
	{
		if (!getSettings().getBool("triggerhack_weapons", false))
			return false;

		CString guild, account, nick;
		if (triggerData.size() > 1) guild = triggerData[1];
		if (triggerData.size() > 2) account = triggerData[2];
		if (triggerData.size() > 3) nick = triggerData[3];

		if (!guild.isEmpty() && !account.isEmpty())
		{
			// Read the guild list.
			FileSystem guildFS;
			guildFS.addDir("guilds");
			CString guildList = guildFS.load(CString() << "guild" << guild << ".txt");

			if (guildList.find(account) == -1)
			{
				if (guildList[guildList.length() - 1] != '\n') guildList << "\n";
				guildList << account;
				if (!nick.isEmpty()) guildList << ":" << nick;

				guildList.save(CString() << "guilds/guild" << guild << ".txt");
			}
		}
		return true;
	});

	builder.registerCommand("gr.removeguildmember", [&](Player* player, std::vector<std::string>& triggerData)
	{
		if (!getSettings().getBool("triggerhack_guilds", false))
			return false;

		CString guild, account;
		if (triggerData.size() > 1) guild = triggerData[1];
		if (triggerData.size() > 2) account = triggerData[2];

		if (!guild.isEmpty() && !account.isEmpty())
		{
			// Read the guild list.
			FileSystem guildFS;
			guildFS.addDir("guilds");
			CString guildList = guildFS.load(CString() << "guild" << guild << ".txt");

			if (guildList.find(account) != -1)
			{
				int pos = guildList.find(account);
				int length = guildList.find("\n", pos) - pos;
				if (length < 0) length = -1;
				else
					++length;

				guildList.removeI(pos, length);
				guildList.save(CString() << "guilds/guild" << guild << ".txt");
			}
		}
		return true;
	});

	builder.registerCommand("gr.removeguild", [&](Player* player, std::vector<std::string>& triggerData)
	{
		if (getSettings().getBool("triggerhack_guilds", false))
			return false;

		CString guild;
		if (triggerData.size() > 1) guild = triggerData[1];

		if (!guild.isEmpty())
		{
			// Read the guild list.
			FileSystem guildFS;
			guildFS.addDir("guilds");
			CString path = guildFS.find(CString() << "guild" << guild << ".txt");

			// Remove the guild.
			remove(path.text());

			// Remove the guild from all players.
			for (auto& [pid, p] : getPlayerList())
			{
				if (p->getGuild() == guild)
				{
					CString nick = p->account.character.nickName;
					p->setNick(nick.readString("(").trimI());
					p->sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PlayerProp::NICKNAME << p->getProp<PlayerProp::NICKNAME>().serialize());
					sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)p->getId() >> (char)PlayerProp::NICKNAME << p->getProp<PlayerProp::NICKNAME>().serialize(), { pid });
				}
			}
		}
		return true;
	});

	builder.registerCommand("gr.setguild", [&](Player* player, std::vector<std::string>& triggerData)
	{
		if (getSettings().getBool("triggerhack_guilds", false))
			return false;

		CString guild, account;
		if (triggerData.size() > 1) guild = triggerData[1];
		if (triggerData.size() > 2) account = triggerData[2];

		if (!guild.isEmpty())
		{
			Player* p = player;
			if (!account.isEmpty()) p = getPlayer(account, PLTYPE_ANYCLIENT).get();
			if (p)
			{
				CString nick = p->account.character.nickName;
				p->setNick(CString() << nick.readString("(").trimI() << " (" << guild << ")", true);
				p->sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PlayerProp::NICKNAME >> (char)p->account.character.nickName.length() << p->account.character.nickName);
				sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)p->getId() >> (char)PlayerProp::NICKNAME >> (char)p->account.character.nickName.length() << p->account.character.nickName, { p->getId() });
			}
		}
		return true;
	});

	// Group levels
	builder.registerCommand("gr.setgroup", [&](Player* player, std::vector<std::string>& triggerData)
	{
		if (auto client = dynamic_cast<PlayerClient*>(player); getSettings().getBool("triggerhack_groups", true) && triggerData.size() == 2 && client != nullptr)
		{
			client->setGroup(triggerData[1]);
			return true;
		}
		return false;
	});

	builder.registerCommand("gr.setlevelgroup", [&](Player* player, std::vector<std::string>& triggerData)
	{
		if (auto client = dynamic_cast<PlayerClient*>(player); getSettings().getBool("triggerhack_groups", true) && triggerData.size() == 2 && client != nullptr)
		{
			const auto& playerList = client->getLevel()->getPlayers();
			for (auto& id : playerList)
			{
				auto pl = getPlayer(id);
				client->setGroup(triggerData[1]);
			}
			return true;
		}
		return false;
	});

	builder.registerCommand("gr.setplayergroup", [&](Player* player, std::vector<std::string>& triggerData)
	{
		if (getSettings().getBool("triggerhack_groups", true) && triggerData.size() == 3)
		{
			if (auto client = getPlayer<PlayerClient>(triggerData[1], PLTYPE_ANYCLIENT); client != nullptr)
				client->setGroup(triggerData[2]);
			return true;
		}
		return false;
	});

	// RC triggers
	builder.registerCommand("gr.rcchat", [&](Player* player, std::vector<std::string>& triggerData)
	{
		if (getSettings().getBool("triggerhack_rc", false))
			return false;

		auto p = getPlayer(player->getId());

		CString msg;
		for (auto i = 1; i < triggerData.size(); ++i)
			msg << triggerData[i] << ",";
		sendToRC(msg, p);
		return true;
	});

	// Level triggers
	builder.registerCommand("gr.npc.move", [&](Player* player, std::vector<std::string>& triggerData)
	{
		if (getSettings().getBool("triggerhack_levels", false) && triggerData.size() == 6)
			return false;

		unsigned int id = string::toNumber(triggerData[1]);
		int dx = string::toNumber(triggerData[2]);
		int dy = string::toNumber(triggerData[3]);
		float duration = string::toFloat(triggerData[4]);
		int options = string::toNumber(triggerData[5]);

		auto npc = getNPC(id);
		if (npc)
		{
			CString packet;
			packet >> (char)(npc->character.pixelX / 8.0f) >> (char)(npc->character.pixelY / 8.0f);
			packet >> (char)((dx * 2) + 100) >> (char)((dy * 2) + 100);
			packet >> (short)(duration / 0.05f);
			packet >> (char)options;
			sendPacketToLevelOnlyGmapArea(CString() >> (char)PLO_MOVE >> (int)id << packet, getPlayer<PlayerClient>(player->getId()));

			npc->character.pixelX += dx * 16;
			npc->character.pixelY += dy * 16;
			//npc->setPropsFromPacket(CString() >> (char)NPCPROP_X >> (char)((npc->getX() + dx) * 2) >> (char)NPCPROP_Y >> (char)((npc->getY() + dy) * 2));
		}
		return true;
	});

	builder.registerCommand("gr.npc.setpos", [&](Player* player, std::vector<std::string>& triggerData)
	{
		if (getSettings().getBool("triggerhack_levels", false) && triggerData.size() == 4)
			return false;

		unsigned int id = string::toNumber(triggerData[1]);
		float x = string::toFloat(triggerData[2]);
		float y = string::toFloat(triggerData[3]);

		auto npc = getNPC(id);
		if (npc)
		{
			npc->character.pixelX = static_cast<int16_t>(x * 16.0);
			npc->character.pixelY = static_cast<int16_t>(y * 16.0);

			// Send the prop packet to the level.
			CString packet;
			packet >> (char)NPCProp::X >> (char)(x * 2.0f);
			packet >> (char)NPCProp::Y >> (char)(y * 2.0f);
			sendPacketToLevelOnlyGmapArea(CString() >> (char)PLO_NPCPROPS >> (int)id << packet, getPlayer<PlayerClient>(player->getId()));
		}
		return true;
	});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
