#include <any>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <IEnums.h>

#include <object/NPC.h>
#include <player/PlayerClient.h>
#include <scripting/gs1/GS1Flags.h>
#include <scripting/gs1/ScriptEngineGS1.h>
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
	// Set all our built-in event flags.
	for (auto& [eventType, flagName] : eventFlagMap)
		variableStore.add(flagName, event == eventType);

	// TODO: Put extensions under a server option?
	variableStore.add("playertouchesme", event == ScriptEventType::PLAYERTOUCHSME);
	variableStore.add("playertouchesother", event == ScriptEventType::PLAYERTOUCHSOTHER);

	/*
		playerhurt
		exploded            the npc was exploded by a bomb
		washit              the npc was slayed with a sword or axe
		waspelt             the npc was pelt
		wasshot             the npc was shot with arrows
		wasthrown           the npc was carried and then thrown
	*/
}

void setTriggerActionAndCustomEventFlags(ScriptEvent& event, GameVariableStore& variableStore)
{
	if (event.args.empty() || (event.type != ScriptEventType::TRIGGERACTION && event.type != ScriptEventType::CUSTOM))
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
		if (event.type == ScriptEventType::TRIGGERACTION)
			action.insert(0, "action");

		// Set the action flag.
		// Set both the original action and a lowercased version.
		variableStore.add(GameVariable{ set_temporary, action, true });
		variableStore.add(GameVariable{ set_temporary, string::toLower(action), true });

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
					event.args.insert(event.args.end(), std::ranges::begin(tokens), std::ranges::end(tokens));
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
		carriesnpc         the player carries an npc
		carriessign        the player carries a sign
		carriesstone       the player carries a stone
		carriesvase        the player carries a vase
	*/

	variableStore.add("weaponsenabled", (player->account.status & PLSTATUS_ALLOWWEAPONS) != 0);
	variableStore.add("playerpaused", (player->account.status & PLSTATUS_PAUSED) != 0);
	variableStore.add("playerismale", (player->account.status & PLSTATUS_MALE) != 0);
	variableStore.add("playerisfemale", (player->account.status & PLSTATUS_MALE) == 0);
	variableStore.add("playeronhorse", !player->account.character.horseImage.empty());

	/* TODO(Nalin): Player flags.
		playerswimming - How does this work?  Does it check for the swim gani, or does it do a tile type check?
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

	/* TODO(Nalin): NPC flags.
		peltwithblackstone  the npc was pelt with a blackstone
		peltwithbush        the npc was pelt with a bush
		peltwithnpc         the npc was pelt with another npc
		peltwithsign        the npc was pelt with a sign
		peltwithstone       the npc was pelt with a stone
		peltwithvase        the npc was pelt with a vase
		shotbybaddy         the npc was shot by a computer opponent
		shotbyplayer        the npc was shot by the player
		followsplayer - Client side only, unless we go sicko mode in the future.
	*/
}

void setLevelFlags(GameVariableStore& variableStore, NPCPtr npc, LevelPtr level)
{
	variableStore.add("issparringzone", level != nullptr && level->isSparringZone);
	variableStore.add("nopkzone", level != nullptr && level->isNoPkZone);
	variableStore.add("isonmap", level != nullptr && level->getMap() != nullptr);
	variableStore.add("compsdead", level != nullptr && !level->hasLivingBaddies());
}

void setWeaponFlags(ScriptEvent& event, ScriptObjectSource source, GameVariableStore& variableStore)
{
	variableStore.add("isweapon", source.second == ScriptObjectSourceType::WEAPON);
}

void setOtherFlags(ScriptEvent& event, ScriptObjectSource source, GameVariableStore& variableStore, NPCPtr npc, PlayerClientPtr player, LevelPtr level)
{
	// Others
	// actionplayer

	/* Older flags:
	* 'gotbow' and 'gotsword' are older pre-1.3 flags.
	*/
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1
