#include <any>
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <IEnums.h>

#include <object/NPC.h>
#include <player/PlayerClient.h>
#include <scripting/gs1/GS1Flags.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////

void setEventFlags(ScriptEventType event, GameVariableStore& variableStore)
{
	static const std::unordered_map<ScriptEventType, std::string_view> eventFlagMap =
	{
		{ ScriptEventType::CREATED, "created"},
		{ ScriptEventType::INITIALIZED, "initialized" },
		{ ScriptEventType::PLAYERLOGIN, "playerlogin"},
		{ ScriptEventType::PLAYERLOGOUT, "playerlogout" },
		{ ScriptEventType::PLAYERENTERS, "playerenters" },
		{ ScriptEventType::PLAYERLEAVES, "playerleaves" },
		{ ScriptEventType::PLAYERTOUCHSME, "playertouchsme" },
		{ ScriptEventType::PLAYERTOUCHSOTHER, "playertouchsother" },
		{ ScriptEventType::PLAYERLAYSITEM, "playerlaysitem" },
		{ ScriptEventType::PLAYERCHATS, "playerchats" },
		{ ScriptEventType::PLAYERDIES, "playerdies" },
		{ ScriptEventType::PLAYERENDREADING, "playerendreading" },
		{ ScriptEventType::WEAPONFIRED, "weaponfired" },
		{ ScriptEventType::FIREDONHORSE, "firedonhorse" },
		{ ScriptEventType::COMPUSDIED, "compusdied" },
		{ ScriptEventType::WARPED, "warped" },
		{ ScriptEventType::NPCWARPED, "npcwarped" },
		{ ScriptEventType::EXPLODED, "exploded" },
		{ ScriptEventType::WASHIT, "washit" },
		{ ScriptEventType::WASSHOT, "wasshot" },
		{ ScriptEventType::WASPELT, "waspelt" },
		{ ScriptEventType::TIMEOUT, "timeout" },
		//
		{ ScriptEventType::SERVERLISTCONNECT, "serverlistconnect" }
	};

	// Set all our built-in event flags.
	for (auto& [eventType, flagName] : eventFlagMap)
		variableStore.add(flagName, event == eventType);

	// TODO: Put extensions under a server option?
	variableStore.add("playertouchesme", event == ScriptEventType::PLAYERTOUCHSME);
	variableStore.add("playertouchesother", event == ScriptEventType::PLAYERTOUCHSOTHER);
}

void setCustomEventFlags(ScriptEvent& event, GameVariableStore& variableStore)
{
	if (event.args.empty())
		return;

	std::string action;
	if (auto* actionStr = std::any_cast<std::string>(&event.args[0]); actionStr != nullptr)
		action = *actionStr;
	else if (auto* actionStr = std::any_cast<const char*>(&event.args[0]); actionStr != nullptr)
		action = *actionStr;
	else if (auto* actionStr = std::any_cast<std::string_view>(&event.args[0]); actionStr != nullptr)
		action = std::string(*actionStr);

	if (!action.empty())
	{
		// Set the action flag.
		// Set both the original action and a lowercased version.
		variableStore.add(GameVariable{ set_temporary, std::format("action{}", action), true });
		variableStore.add(GameVariable{ set_temporary, string::toLower(std::format("action{}", action)), true });

		// If there are just two arguments, try to unpack the second argument.
		if (event.args.size() == 2)
		{
			if (auto* params = std::any_cast<std::string>(&event.args[1]); params != nullptr)
			{
				// Split the parameters by commas.
				auto tokens = string::fromCSV(*params);
				if (tokens.size() > 1)
				{
					event.args.erase(event.args.begin() + 1);
					event.args.append_range(tokens);
				}
			}
		}
	}
}

void setPlayerFlags(GameVariableStore& variableStore, NPCPtr npc, PlayerClientPtr player)
{
	if (player == nullptr)
		return;

	variableStore.add("canspin", (player->account.status & PLSTATUS_HASSPIN) != 0);

	/* TODO(Nalin): Carry sprite flags. PLPROP_CARRYSPRITE
		carrying           the player carries something
		carriesblackstone  the player carries a blackstone
		carriesbush        the player carries a bush
		carriessign        the player carries a sign
		carriesstone       the player carries a stone
		carriesvase        the player carries a vase
	*/

	variableStore.add("weaponsenabled", (player->account.status & PLSTATUS_ALLOWWEAPONS) != 0);
	variableStore.add("playerpaused", (player->account.status & PLSTATUS_PAUSED) != 0);
	variableStore.add("playerismale", (player->account.status & PLSTATUS_MALE) != 0);
	variableStore.add("playerisfemale", (player->account.status & PLSTATUS_MALE) == 0);
	variableStore.add("playeronhorse", !player->account.character.horseImage.empty());

	// TODO(Nalin): playerswimming - How does this work?  Does it check for the swim gani, or does it do a tile type check?
	/*
		playerlaysitem
		playertrial
	*/

	variableStore.add("playerattached", npc != nullptr && player->getAttachedNPC() == npc->id);

	auto level = player->getLevel();
	variableStore.add("isleader", level != nullptr && level->isPlayerLeader(player->getId()));
}

void setNPCFlags(GameVariableStore& variableStore, NPCPtr npc)
{
	if (npc == nullptr)
		return;

	variableStore.add("visible", npc->visFlags != PROPID(NPCVisFlags::HIDDEN));

	// followsplayer - Client side only, unless we go sicko mode in the future.

	/* TODO(Nalin): Hit events.
		washit              the npc was slayed with a sword or axe
		waspelt             the npc was pelt
		wasshot             the npc was shot with arrows
		wasthrown           the npc was carried and then thrown
		exploded            the npc was exploded by a bomb
		peltwithblackstone  the npc was pelt with a blackstone
		peltwithbush        the npc was pelt with a bush
		peltwithnpc         the npc was pelt with another npc
		peltwithsign        the npc was pelt with a sign
		peltwithstone       the npc was pelt with a stone
		peltwithvase        the npc was pelt with a vase
		shotbybaddy         the npc was shot by a computer opponent
		shotbyplayer        the npc was shot by the player
	*/
}

void setLevelFlags(GameVariableStore& variableStore, NPCPtr npc, LevelPtr level)
{
	variableStore.add("issparringzone", level != nullptr && level->isSparringZone());
	variableStore.add("nopkzone", level != nullptr && level->isNoPkZone());
	variableStore.add("isonmap", level != nullptr && level->getMap() != nullptr);
	variableStore.add("compsdead", level != nullptr && !level->hasLivingBaddies());

	/*
		levelorgx     level origin(x), can be different to 0, 0 if the player is attached to an npc
		levelorgy     level origin(y)
		gravity       the rate at which shot projectiles fall (default Z loss of 2 tiles per second)
	*/
}

void setOtherFlags(GameVariableStore& variableStore, NPCPtr npc, PlayerClientPtr player, LevelPtr level)
{
	// Character based flags.

	// Weapon based flags.
	// isweapon       this npc is a weapon

	// Actions
	// actionprojectile		(serverside - projectile lands)
	// actionsprojectile	(serverside - projectile lands) (how does this differ from actionprojectile?)
	// actionprojectile2	(clientside - projectile lands)
	// actionpushed
	// actionpulled
	// pm  #p(0) is account, #p(1) is the message

	// Others
	// actionplayer

	/* Older flags:
	* 'gotbow' and 'gotsword' are older pre-1.3 flags.
	*/
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1
