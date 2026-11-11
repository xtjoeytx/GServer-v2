#include <algorithm>
#include <cstdint>
#include <ranges>
#include <string>
#include <vector>

#include <CString.h>
#include <IEnums.h>

#include <BabyDI.h>
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
#include <utilities/Log.h>
#include <utilities/manager/GuildManager.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
// A text as CSV fields without commas inside: the v6 scripts join the fields again with ",".
static void appendTextFields(std::vector<std::string>& fields, const std::string& text)
{
	for (size_t from = 0, comma = 0; comma != std::string::npos; from = comma + 1)
	{
		comma = text.find(',', from);
		fields.push_back(text.substr(from, comma == std::string::npos ? std::string::npos : comma - from));
	}
}

///////////////////////////////////////////////////////////////////////////////

void Server::createTriggerCommands(TriggerDispatcher::Builder builder)
{
	builder.registerCommand("serverside", [&](const Player* player, std::vector<std::string>& triggerData)
	{
		if (!hasNPCServer())
			return false;

		if (triggerData.size() >= 2)
		{
			// triggeraction 0,0,serverside,weaponname,params...;
			// Triggers on a player's weapons.
			if (const auto weapon = getWeapon(triggerData[1]); weapon != nullptr)
				weapon->scripting.events.addEvent(ScriptEventType::TRIGGERACTION, source::FromPlayer(player->getId()), "serverside"s, string::toCSV(triggerData | std::views::drop(2)));
		}
		return true;
	});

	builder.registerCommand("servernpc", [&](const Player* player, std::vector<std::string>& triggerData)
	{
		if (!hasNPCServer())
			return false;

		if (triggerData.size() >= 2)
		{
			// triggeraction 0,0,servernpc,npcname,params...;
			if (const auto npcServer = getNPCServer(); npcServer != nullptr)
			{
				if (const auto npc = npcServer->getNPCByName(triggerData[1]); npc != nullptr)
					npc->scripting.events.addEvent(ScriptEventType::TRIGGERACTION, source::FromPlayer(player->getId()), "serverside"s, string::toCSV(triggerData | std::views::drop(2)));
			}
		}
		return true;
	});

	builder.registerCommand("gr.serverlist", [&](Player* player, std::vector<std::string>& triggerData)
	{
		auto& listServer = getServerList();
		const auto& serverList = listServer.getServerList();

		CString actionData("clientside,-Serverlist_v4,updateservers,");
		for (const auto& [serverName, serverID] : serverList)
			actionData << CString(serverName).gtokenize() << "," << CString(serverID) << ",";

		player->sendPacket(CString() >> (char)PLO_TRIGGERACTION >> (short)0 >> (int)0 >> (char)0 >> (char)0 << actionData);
		return true;
	});

	// Weapon management
	builder.registerCommand("gr.addweapon", [&](Player* player, const std::vector<std::string>& triggerData)
	{
		if (!cached.enableTriggerhackWeapons.getValue())
			return false;

		for (size_t i = 1; i < triggerData.size(); ++i)
			player->addWeapon(string::trim(triggerData[i]));

		return true;
	});

	builder.registerCommand("gr.deleteweapon", [&](Player* player, const std::vector<std::string>& triggerData)
	{
		if (!cached.enableTriggerhackWeapons.getValue())
			return false;

		for (size_t i = 1; i < triggerData.size(); ++i)
			player->deleteWeapon(string::trim(triggerData[i]));

		return true;
	});

	// Guild management
	builder.registerCommand("gr.addguildmember", [&](Player* player, const std::vector<std::string>& triggerData)
	{
		if (!cached.enableTriggerhackGuilds.getValue())
			return false;

		CString guild, account, nick;
		if (triggerData.size() > 1) guild = triggerData[1];
		if (triggerData.size() > 2) account = triggerData[2];
		if (triggerData.size() > 3) nick = triggerData[3];

		if (!guild.isEmpty() && !account.isEmpty())
		{
			if (const auto guildManager = BabyDI::Get<GuildManager>(); guildManager != nullptr)
				guildManager->addPlayerToGuild(guild, account, nick);
		}

		return true;
	});

	builder.registerCommand("gr.removeguildmember", [&](Player* player, const std::vector<std::string>& triggerData)
	{
		if (!cached.enableTriggerhackGuilds.getValue())
			return false;

		CString guild, account, nickName;
		if (triggerData.size() > 1) guild = triggerData[1];
		if (triggerData.size() > 2) account = triggerData[2];
		if (triggerData.size() > 3) nickName = triggerData[3];

		if (!guild.isEmpty() && !account.isEmpty())
		{
			if (const auto guildManager = BabyDI::Get<GuildManager>(); guildManager != nullptr)
			{
				if (nickName.isEmpty())
					guildManager->removePlayerEntirelyFromGuild(guild, account);
				else guildManager->removePlayerFromGuild(guild, account, nickName);
			}
		}

		return true;
	});

	builder.registerCommand("gr.removeguild", [&](Player* player, const std::vector<std::string>& triggerData)
	{
		if (!cached.enableTriggerhackGuilds.getValue())
			return false;

		CString guild;
		if (triggerData.size() > 1) guild = triggerData[1];

		if (!guild.isEmpty())
		{
			if (const auto guildManager = BabyDI::Get<GuildManager>(); guildManager != nullptr)
				guildManager->deleteGuild(guild);

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

	builder.registerCommand("gr.setguild", [&](Player* player, const std::vector<std::string>& triggerData)
	{
		if (!cached.enableTriggerhackGuilds.getValue())
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

	// The in-game guild window (Kixt: guild ranks). Unlike the gr.*guild* triggerhacks above, these are always on and the server checks every
	// change itself: an officer can add and remove plain members, a leader can also change ranks; a guild always keeps at least one leader.
	//   gr.guild,info                          -> clientside,-Guilds,info,<guild count>,{name,myrank,count,{account,rank,nickname}...}...
	//   gr.guild,add,<guild>,<account>[,<nickname>]   gr.guild,remove,<guild>,<account>   gr.guild,leave,<guild>
	//   gr.guild,nick,<guild>,<account>,<nickname>   the one name the account may wear with the tag ("" = any name); officers for plain members, leaders for all
	//   gr.guild,rank,<guild>,<account>,<member|officer|leader>
	//   gr.guild,create,<guild>[,<leader account>]   staff only: a new guild with a leader
	// Errors and confirmations come back as clientside,-Guilds,message,<text>.
	builder.registerCommand("gr.guild", [&](Player* player, const std::vector<std::string>& triggerData)
	{
		const auto guildManager = BabyDI::Get<GuildManager>();
		if (guildManager == nullptr || player == nullptr) return true;
		const std::string me = player->account.name;
		const std::string sub = triggerData.size() > 1 ? string::toLower(std::string{string::trim(triggerData[1])}) : "info"s;
		const std::string guildName = triggerData.size() > 2 ? std::string{string::trim(triggerData[2])} : ""s;
		std::string target = triggerData.size() > 3 ? std::string{string::trim(triggerData[3])} : ""s;

		auto send = [&](const std::vector<std::string>& fields)
		{
			std::vector<std::string> all{"clientside", "-Guilds"};
			all.insert(all.end(), fields.begin(), fields.end());
			player->sendPacket(CString() >> (char)PLO_TRIGGERACTION >> (short)0 >> (int)0 >> (char)0 >> (char)0 << string::toCSV(all));
		};
		auto message = [&](const std::string& text) { send({"message", text}); };
		auto sendInfo = [&]()
		{
			std::vector<std::string> fields{"info"};
			const auto guilds = guildManager->guildsOfAccount(me);
			fields.push_back(std::to_string(guilds.size()));
			for (const auto* g : guilds)
			{
				std::vector<std::string> accounts;
				for (const auto& account : g->members | std::views::keys)
					if (std::ranges::find(accounts, account) == accounts.end()) accounts.push_back(account);
				fields.push_back(g->name);
				fields.push_back(std::to_string(guildManager->getRank(g->name, me)));
				fields.push_back(std::to_string(accounts.size()));
				for (const auto& account : accounts)
				{
					fields.push_back(account);
					fields.push_back(std::to_string(guildManager->getRank(g->name, account)));
					fields.push_back(guildManager->getMemberNick(g->name, account));
				}
			}
			send(fields);
		};
		auto leaders = [&](const std::string& g)
		{
			int n = 0;
			if (const auto* guild = guildManager->getGuild(g); guild != nullptr)
				for (const auto& rank : guild->ranks | std::views::values) n += rank == GUILD_LEADER;
			return n;
		};
		// A removed member must not keep wearing the tag.
		auto stripTag = [&](const std::string& account, const std::string& g)
		{
			for (auto& [pid, p] : getPlayerList())
			{
				if (p->account.name == account && p->getGuild() == g)
				{
					CString nick = p->account.character.nickName;
					p->setNick(nick.readString("(").trimI(), true);
					p->sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PlayerProp::NICKNAME >> (char)p->account.character.nickName.length() << p->account.character.nickName);
					sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)p->getId() >> (char)PlayerProp::NICKNAME >> (char)p->account.character.nickName.length() << p->account.character.nickName, { p->getId() });
				}
			}
		};

		if (sub == "info") { sendInfo(); return true; }

		// Staff create guilds: gr.guild,create,<guild>[,<leader account>]. The leader is the staff member unless an account is given.
		if (sub == "create")
		{
			if (!player->isStaff()) { message("Only staff can create guilds."); return true; }
			if (guildName.empty() || guildName.size() > 40 || guildName.find_first_of("()/\\:,") != std::string::npos)
			{
				message("A guild name cannot be empty or have ( ) / \\ : or , in it.");
				return true;
			}
			if (guildManager->guildExists(guildName)) { message("There is already a guild named " + guildName + "."); return true; }
			std::string leader = me;
			if (!target.empty())
			{
				const auto path = getFileSystemServer().findi(fs::FileCategory::ACCOUNT, target + ".txt");
				if (path.empty()) { message("There is no account named " + target + "."); return true; }
				leader = fs::getANSIFileName(path.stem());
			}
			if (!guildManager->createGuild(guildName)) { message("The guild " + guildName + " could not be created."); return true; }
			guildManager->addPlayerToGuild(guildName, leader, ""s);
			guildManager->setRank(guildName, leader, GUILD_LEADER);
			guildManager->saveGuild(guildName);
			log::printLine(log::server, "[Guild] {} created {} with {} as leader", me, guildName, leader);
			message("The guild " + guildName + " was created, " + leader + " is its leader."); sendInfo(); return true;
		}

		const int myRank = guildManager->getRank(guildName, me);
		if (myRank < 0) { message("You are not in the guild " + guildName + "."); return true; }

		// The guild tag behind the name: gr.guild,wear,<guild> puts "(<guild>)" behind the nick (the name the guild file allows for this account,
		// else the current one), gr.guild,unwear takes it away.
		if (sub == "wear")
		{
			std::string base = guildManager->getMemberNick(guildName, me);
			if (base.empty())
			{
				CString nick = player->account.character.nickName;
				base = nick.readString("(").trimI().toString();
			}
			if (base.empty()) { message("You have no name to put the tag behind."); return true; }
			player->setNick(CString() << base << " (" << guildName << ")", true);
			player->sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PlayerProp::NICKNAME >> (char)player->account.character.nickName.length() << player->account.character.nickName);
			sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)player->getId() >> (char)PlayerProp::NICKNAME >> (char)player->account.character.nickName.length() << player->account.character.nickName, { player->getId() });
			log::printLine(log::server, "[Guild] {} wears the tag of {}", me, guildName);
			message("You now wear the tag of " + guildName + "."); sendInfo(); return true;
		}
		if (sub == "unwear")
		{
			stripTag(me, guildName);
			message("The tag of " + guildName + " is off."); sendInfo(); return true;
		}

		if (sub == "leave") target = me;
		if (target.empty()) { message("No account given."); return true; }

		if (sub == "add")
		{
			if (myRank < GUILD_OFFICER) { message("Only officers and leaders can add members."); return true; }
			const auto path = getFileSystemServer().findi(fs::FileCategory::ACCOUNT, target + ".txt");
			if (path.empty()) { message("There is no account named " + target + "."); return true; }
			const std::string account = fs::getANSIFileName(path.stem());
			if (guildManager->getRank(guildName, account) >= 0) { message(account + " is already in " + guildName + "."); return true; }
			const std::string nick = triggerData.size() > 4 ? std::string{string::trim(triggerData[4])} : ""s;
			if (nick.find_first_of("()") != std::string::npos) { message("A name cannot have ( or ) in it."); return true; }
			guildManager->addPlayerToGuild(guildName, account, nick);
			guildManager->saveGuild(guildName);
			log::printLine(log::server, "[Guild] {} added {} ({}) to {}", me, account, nick, guildName);
			message(account + " is now a member of " + guildName + (nick.empty() ? "" : " as " + nick) + "."); sendInfo(); return true;
		}

		const int theirRank = guildManager->getRank(guildName, target);
		if (theirRank < 0) { message(target + " is not in " + guildName + "."); return true; }

		if (sub == "remove" || sub == "leave")
		{
			const bool self = target == me;
			if (!self && myRank < GUILD_OFFICER) { message("Only officers and leaders can remove members."); return true; }
			if (!self && myRank == GUILD_OFFICER && theirRank != GUILD_MEMBER) { message("Officers can only remove plain members."); return true; }
			if (theirRank == GUILD_LEADER && leaders(guildName) <= 1) { message("A guild needs a leader: make someone else leader first."); return true; }
			guildManager->removePlayerEntirelyFromGuild(guildName, target);
			guildManager->saveGuild(guildName);
			stripTag(target, guildName);
			log::printLine(log::server, "[Guild] {} removed {} from {}", me, target, guildName);
			message(self ? "You left " + guildName + "." : target + " was removed from " + guildName + "."); sendInfo(); return true;
		}

		if (sub == "nick")
		{
			const std::string nick = triggerData.size() > 4 ? std::string{string::trim(triggerData[4])} : ""s;
			if (nick.find_first_of("()") != std::string::npos) { message("A name cannot have ( or ) in it."); return true; }
			if (myRank < GUILD_OFFICER) { message("Only officers and leaders can set names."); return true; }
			if (myRank == GUILD_OFFICER && theirRank != GUILD_MEMBER && target != me) { message("Officers can only name plain members."); return true; }
			guildManager->setMemberNick(guildName, target, nick);
			guildManager->saveGuild(guildName);
			// Someone wearing the tag right now gets the new name with it (the old one would no longer pass the guild check).
			for (auto& [pid, p] : getPlayerList())
			{
				if (p->account.name == target && p->getGuild() == guildName && !nick.empty())
				{
					p->setNick(CString() << nick << " (" << guildName << ")", true);
					p->sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PlayerProp::NICKNAME >> (char)p->account.character.nickName.length() << p->account.character.nickName);
					sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)p->getId() >> (char)PlayerProp::NICKNAME >> (char)p->account.character.nickName.length() << p->account.character.nickName, { p->getId() });
				}
			}
			log::printLine(log::server, "[Guild] {} named {} '{}' in {}", me, target, nick, guildName);
			message(nick.empty() ? target + " may wear the tag with any name." : target + " is now " + nick + " (" + guildName + ")."); sendInfo(); return true;
		}

		if (sub == "rank")
		{
			const std::string word = triggerData.size() > 4 ? string::toLower(std::string{string::trim(triggerData[4])}) : ""s;
			const int rank = word == "leader" ? GUILD_LEADER : word == "officer" ? GUILD_OFFICER : word == "member" ? GUILD_MEMBER : -1;
			if (rank < 0) { message("Unknown rank: " + word + "."); return true; }
			if (myRank != GUILD_LEADER) { message("Only leaders can change ranks."); return true; }
			if (theirRank == GUILD_LEADER && rank != GUILD_LEADER && leaders(guildName) <= 1) { message("A guild needs a leader: make someone else leader first."); return true; }
			guildManager->setRank(guildName, target, rank);
			guildManager->saveGuild(guildName);
			log::printLine(log::server, "[Guild] {} made {} {} of {}", me, target, word, guildName);
			message(target + " is now " + word + " of " + guildName + "."); sendInfo(); return true;
		}

		message("Unknown guild command: " + sub + ".");
		return true;
	});

	// The player list with everybody who is logged in, RC accounts included, and private messages between them. The answers go to the weapon -Players:
	//   gr.players                      -> clientside,-Players,list,<count>,{account,nick,guild,kind,level}...      kind: player | staff | rc | npc
	//   gr.pm,<account>,<text>          -> the account gets the message (as a normal PM for RC, as clientside,-Players,pm,<from account>,<from nick>,<kind>,<text> for clients)
	builder.registerCommand("gr.players", [&](Player* player, const std::vector<std::string>& triggerData)
	{
		if (player == nullptr) return true;
		std::vector<std::string> fields{"clientside", "-Players", "list"};
		std::vector<std::string> rows;
		for (auto& [pid, p] : getPlayerList())
		{
			if (p->isNC()) continue;
			const bool npcServer = p->account.name.starts_with('(');             // "(npcserver)"
			rows.push_back(p->account.name);
			rows.push_back(p->account.character.nickName);
			rows.push_back(p->isClient() ? std::string{p->getGuild()} : std::string{});
			// Staff is only listed as staff while wearing one of the staff tags of the server (staffguilds); without the tag they are players like the rest.
			bool staffTag = false;
			const std::string wornTag{p->isClient() ? std::string{p->getGuild()} : std::string{}};
			if (p->isClient() && p->isStaff() && !wornTag.empty())
			{
				const std::string tags = getSettings().get<std::string>("staffguilds").value_or("");
				for (size_t from = 0, comma = 0; comma != std::string::npos && !staffTag; from = comma + 1)
				{
					comma = tags.find(',', from);
					const auto tag = string::trim(std::string_view{tags}.substr(from, comma == std::string::npos ? std::string::npos : comma - from));
					staffTag = string::equalsi(tag, std::string_view{wornTag});
				}
			}
			rows.push_back(npcServer ? "npc" : p->isRC() ? "rc" : staffTag ? "staff" : "player");
			rows.push_back(p->isClient() && p->getLevel() != nullptr ? std::string{p->getLevel()->levelName} : std::string{});
		}
		fields.push_back(std::to_string(rows.size() / 5));
		fields.insert(fields.end(), rows.begin(), rows.end());
		player->sendPacket(CString() >> (char)PLO_TRIGGERACTION >> (short)0 >> (int)0 >> (char)0 >> (char)0 << string::toCSV(fields));
		return true;
	});

	builder.registerCommand("gr.pm", [&](Player* player, const std::vector<std::string>& triggerData)
	{
		if (player == nullptr) return true;
		auto answer = [&](const std::string& text)
		{
			std::vector<std::string> fields{"clientside", "-Players", "message"};
			for (size_t from = 0, comma = 0; comma != std::string::npos; from = comma + 1)
			{
				comma = text.find(',', from);
				fields.push_back(text.substr(from, comma == std::string::npos ? std::string::npos : comma - from));
			}
			player->sendPacket(CString() >> (char)PLO_TRIGGERACTION >> (short)0 >> (int)0 >> (char)0 >> (char)0 << string::toCSV(fields));
		};
		const std::string account = triggerData.size() > 1 ? std::string{string::trim(triggerData[1])} : ""s;
		std::string text;
		for (size_t i = 2; i < triggerData.size(); ++i) text += (i > 2 ? "," : "") + triggerData[i];
		if (account.empty() || text.empty()) { answer("Pick somebody and type a message first."); return true; }
		if (text.size() > 1024) { answer("A message can have 1024 characters at most."); return true; }
		static std::unordered_map<std::string, std::chrono::steady_clock::time_point> lastPm;
		const auto now = std::chrono::steady_clock::now();
		if (const auto it = lastPm.find(player->account.name); it != lastPm.end() && now - it->second < std::chrono::seconds{1}) { answer("Wait a moment before the next message."); return true; }
		lastPm[player->account.name] = now;
		std::shared_ptr<Player> target;
		for (auto& [pid, p] : getPlayerList())
			if (!p->isNC() && string::equalsi(std::string_view{p->account.name}, std::string_view{account})) { target = p; break; }
		if (target == nullptr || target.get() == player) { answer(target == nullptr ? account + " is not online." : "You cannot message yourself."); return true; }
		const std::string constructed = std::format("#bPrivate message:#b{}", text);
		target->sendPrivateMessage(player->getId(), constructed);
		answer("To " + target->account.name + ": " + text);
		log::printLine(log::server, "[PM] {} -> {}: {}", player->account.name, target->account.name, text);
		return true;
	});

	//   gr.mass,<text>                  -> everybody online gets the message as a mass message (30 seconds between two, staff any time)
	//   gr.guildmsg,<guild>,<text>      -> the online members of the guild (all your guilds when no guild is given): clientside,-Players,gm,<guild>,<account>,<nick>,<text>
	builder.registerCommand("gr.mass", [&](Player* player, const std::vector<std::string>& triggerData)
	{
		if (player == nullptr) return true;
		auto answer = [&](const std::string& text)
		{
			std::vector<std::string> fields{"clientside", "-Players", "message"};
			appendTextFields(fields, text);
			player->sendPacket(CString() >> (char)PLO_TRIGGERACTION >> (short)0 >> (int)0 >> (char)0 >> (char)0 << string::toCSV(fields));
		};
		std::string text;
		for (size_t i = 1; i < triggerData.size(); ++i) text += (i > 1 ? "," : "") + triggerData[i];
		if (text.empty()) { answer("Type a message first."); return true; }
		if (text.size() > 300) { answer("A mass message can have 300 characters at most."); return true; }
		static std::unordered_map<std::string, std::chrono::steady_clock::time_point> lastMass;
		const auto now = std::chrono::steady_clock::now();
		if (!player->isStaff())
		{
			if (const auto it = lastMass.find(player->account.name); it != lastMass.end() && now - it->second < std::chrono::seconds{30}) { answer("Wait 30 seconds between two mass messages."); return true; }
			lastMass[player->account.name] = now;
		}
		const std::string constructed = std::format("#bMass message:#b{}", text);
		int n = 0;
		for (auto& [pid, p] : getPlayerList())
		{
			if (p.get() == player || p->isNC() || p->account.name.starts_with('(')) continue;
			if (p->getProp<PlayerProp::ADDITFLAGS>().value & PLFLAG_NOMASSMESSAGE) continue;
			p->sendPrivateMessage(player->getId(), constructed);
			++n;
		}
		log::printLine(log::server, "[PM] {} -> everybody ({}): {}", player->account.name, n, text);
		answer("Mass message to " + std::to_string(n) + " players: " + text);
		return true;
	});

	builder.registerCommand("gr.guildmsg", [&](Player* player, const std::vector<std::string>& triggerData)
	{
		const auto guildManager = BabyDI::Get<GuildManager>();
		if (player == nullptr || guildManager == nullptr) return true;
		auto answer = [&](const std::string& text)
		{
			std::vector<std::string> fields{"clientside", "-Players", "message"};
			appendTextFields(fields, text);
			player->sendPacket(CString() >> (char)PLO_TRIGGERACTION >> (short)0 >> (int)0 >> (char)0 >> (char)0 << string::toCSV(fields));
		};
		const std::string me = player->account.name;
		std::string guildName = triggerData.size() > 1 ? std::string{string::trim(triggerData[1])} : ""s;
		std::string text;
		for (size_t i = 2; i < triggerData.size(); ++i) text += (i > 2 ? "," : "") + triggerData[i];
		if (text.empty()) { answer("Type a message first."); return true; }
		if (text.size() > 300) { answer("A guild message can have 300 characters at most."); return true; }
		std::vector<std::string> guilds;
		if (!guildName.empty())
		{
			if (guildManager->getRank(guildName, me) < 0) { answer("You are not in the guild " + guildName + "."); return true; }
			guilds.push_back(guildName);
		}
		else for (const auto* g : guildManager->guildsOfAccount(me)) guilds.push_back(g->name);
		if (guilds.empty()) { answer("You are not in a guild."); return true; }
		int n = 0;
		for (const auto& g : guilds)
		{
			for (auto& [pid, p] : getPlayerList())
			{
				if (p.get() == player || !p->isClient() || guildManager->getRank(g, p->account.name) < 0) continue;
				std::vector<std::string> fields{"clientside", "-Players", "gm", g, me, player->account.character.nickName};
				appendTextFields(fields, text);
				p->sendPacket(CString() >> (char)PLO_TRIGGERACTION >> (short)0 >> (int)0 >> (char)0 >> (char)0 << string::toCSV(fields));
				++n;
			}
		}
		log::printLine(log::server, "[PM] {} -> guild {} ({}): {}", me, guilds.front(), n, text);
		answer("Guild message to " + std::to_string(n) + " members of " + guilds.front() + ": " + text);
		return true;
	});

	//   gr.warp,<level>,<x>,<y>         -> warps the player, but only when an NPC within 20 tiles has a setlevel2 for that level in its script (the "sail to ..." NPCs)
	builder.registerCommand("gr.warp", [&](Player* player, const std::vector<std::string>& triggerData)
	{
		if (player == nullptr || triggerData.size() < 4) return true;
		const std::string target{string::trim(triggerData[1])};
		if (target.empty()) return true;
		const auto level = player->getLevel();
		if (level == nullptr) return true;
		const auto lower = [](std::string t) { std::ranges::transform(t, t.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); }); return t; };
		const std::string wanted = lower(target);
		bool allowed = false;
		for (const auto& npcId : level->findNPCsInRangeByDistance(player->getGlobalPosition(), 20))
		{
			const auto npc = getNPC(npcId);
			if (npc == nullptr) continue;
			const std::string source = lower(npc->getScript().getOriginalSource());
			if (source.find("setlevel2") != std::string::npos && source.find(wanted) != std::string::npos) { allowed = true; break; }
		}
		if (!allowed)
		{
			log::printLine(log::server, "[Warp] {} asked for {} without an NPC that sends there", player->account.name, target);
			return true;
		}
		const double x = std::strtod(triggerData[2].c_str(), nullptr), y = std::strtod(triggerData[3].c_str(), nullptr);
		log::printLine(log::server, "[Warp] {} -> {} {} {}", player->account.name, target, x, y);
		player->warp(target, PixelPosition{static_cast<int32_t>(x * 16), static_cast<int32_t>(y * 16)});
		return true;
	});

	// Group levels
	builder.registerCommand("gr.setgroup", [&](Player* player, const std::vector<std::string>& triggerData)
	{
		if (const auto client = dynamic_cast<PlayerClient*>(player); cached.enableTriggerhackGroups.getValue() && client != nullptr)
		{
			client->setGroup(triggerData.size() >= 2 ? triggerData[1] : ""s);
			return true;
		}
		return false;
	});

	builder.registerCommand("gr.setlevelgroup", [&](Player* player, const std::vector<std::string>& triggerData)
	{
		if (const auto client = dynamic_cast<PlayerClient*>(player); cached.enableTriggerhackGroups.getValue() && client != nullptr)
		{
			for (const auto& id : client->getLevel()->getPlayers())
			{
				if (auto pl = getPlayer<PlayerClient>(id); pl != nullptr)
					pl->setGroup(triggerData.size() >= 2 ? triggerData[1] : ""s);
			}
			return true;
		}
		return false;
	});

	builder.registerCommand("gr.setplayergroup", [&](Player* player, const std::vector<std::string>& triggerData)
	{
		if (cached.enableTriggerhackGroups.getValue() && triggerData.size() >= 2)
		{
			if (const auto client = getPlayer<PlayerClient>(triggerData[1], PLTYPE_ANYCLIENT); client != nullptr)
				client->setGroup(triggerData.size() >= 3 ? triggerData[2] : ""s);
			return true;
		}
		return false;
	});

	// RC triggers
	builder.registerCommand("gr.rcchat", [&](const Player* player, const std::vector<std::string>& triggerData)
	{
		if (!cached.enableTriggerhackRC.getValue())
			return false;

		const auto p = getPlayer(player->getId());
		if (p == nullptr)
			return false;

		CString msg;
		for (size_t i = 1; i < triggerData.size(); ++i)
			msg << triggerData[i] << ",";

		sendToRC(msg, p);
		return true;
	});

	// Level triggers
	builder.registerCommand("gr.npc.move", [&](Player* player, const std::vector<std::string>& triggerData)
	{
		if (!cached.enableTriggerhackLevels.getValue() || triggerData.size() != 6)
			return false;

		const auto id = string::toNumber<NPCID>(triggerData[1]);
		const int dx = string::toNumber(triggerData[2]);
		const int dy = string::toNumber(triggerData[3]);
		const float duration = string::toFloat(triggerData[4]);
		const int options = string::toNumber(triggerData[5]);

		if (const auto npc = getNPC(id); npc)
		{
			CString packet;
			packet >> (char)(static_cast<float>(npc->character.localPixelX) / 8.0f) >> (char)(static_cast<float>(npc->character.localPixelY) / 8.0f);
			packet >> (char)((dx * 2) + 100) >> (char)((dy * 2) + 100);
			packet >> (short)(duration / 0.05f);
			packet >> (char)options;
			sendPacketToNearby(CString() >> (char)PLO_MOVE >> (int)id << packet, npc->character.getGlobalPosition(), npc->getLevel());

			npc->character.localPixelX = static_cast<int16_t>(npc->character.localPixelX + dx * 16);
			npc->character.localPixelY = static_cast<int16_t>(npc->character.localPixelY + dy * 16);
		}
		return true;
	});

	builder.registerCommand("gr.npc.setpos", [&](Player* player, const std::vector<std::string>& triggerData)
	{
		if (!cached.enableTriggerhackLevels.getValue() || triggerData.size() != 4)
			return false;

		const auto id = string::toNumber<NPCID>(triggerData[1]);
		const float x = string::toFloat(triggerData[2]);
		const float y = string::toFloat(triggerData[3]);

		if (const auto npc = getNPC(id);npc)
		{
			npc->character.localPixelX = static_cast<int16_t>(x * 16.0);
			npc->character.localPixelY = static_cast<int16_t>(y * 16.0);

			// Send the prop packet to the level.
			CString packet;
			packet >> (char)NPCProp::X >> (char)(x * 2.0f);
			packet >> (char)NPCProp::Y >> (char)(y * 2.0f);
			sendPacketToNearby(CString() >> (char)PLO_NPCPROPS >> (int)id << packet, npc->character.getGlobalPosition(), npc->getLevel());
		}
		return true;
	});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
