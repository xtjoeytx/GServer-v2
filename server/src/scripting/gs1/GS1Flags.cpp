#include <any>
#include <iterator>
#include <string_view>
#include <string>
#include <vector>

#include <IEnums.h>

#include <level/LevelTileTypes.h>
#include <object/NPC.h>
#include <object/Player.h>
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

	// Valid alternates.
	variableStore.add("playerhurted", event == ScriptEventType::PLAYERHURT);
	variableStore.add("wasshooted", event == ScriptEventType::WASSHOT);

	// TODO: Put extensions under a server option?
	variableStore.add("playertouchesme", event == ScriptEventType::PLAYERTOUCHSME);
	variableStore.add("playertouchesother", event == ScriptEventType::PLAYERTOUCHSOTHER);

	/*
		washit              the npc was slayed with a sword or axe
		waspelt             the npc was pelt
		wasthrown           the npc was carried and then thrown
		emoticon
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
		variableStore.add(GameVariable{.name = action, .value = true, .lifetime = variables::Lifetime::TEMPORARY});
		variableStore.add(GameVariable{.name = string::toLower(action), .value = true, .lifetime = variables::Lifetime::TEMPORARY});

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

void setPlayerFlags(GameVariableStore& variableStore, NPCPtr npc, PlayerPtr player)
{
	if (player == nullptr)
		return;

	variableStore.add("canspin", (player->account.status & PLSTATUS_HASSPIN) != 0);

	variableStore.add("carrying", player->getCarrySprite() != PROPID(CarryObjectSprite::NONE));
	variableStore.add("carriesblackstone", player->getCarrySprite() == PROPID(CarryObjectSprite::BLACKSTONE));
	variableStore.add("carriesbush", player->getCarrySprite() == PROPID(CarryObjectSprite::BUSH));
	variableStore.add("carriesnpc", player->getCarryNPC() != 0);
	variableStore.add("carriessign", player->getCarrySprite() == PROPID(CarryObjectSprite::SIGN));
	variableStore.add("carriesstone", player->getCarrySprite() == PROPID(CarryObjectSprite::STONE));
	variableStore.add("carriesvase", player->getCarrySprite() == PROPID(CarryObjectSprite::VASE));

	variableStore.add("weaponsenabled", (player->account.status & PLSTATUS_ALLOWWEAPONS) != 0);
	variableStore.add("playerpaused", (player->account.status & PLSTATUS_PAUSED) != 0);
	variableStore.add("playerismale", (player->account.status & PLSTATUS_MALE) != 0);
	variableStore.add("playerisfemale", (player->account.status & PLSTATUS_MALE) == 0);
	variableStore.add("playeronhorse", !player->account.character.horseImage.empty());
	variableStore.add("playeronline", true);
	variableStore.add("playerattached", npc != nullptr && player->getAttachedNPC() == npc->id);

	auto level = player->getLevel();
	variableStore.add("isleader", level != nullptr && level->isPlayerLeader(player->getId()));

	// playertrial
}

void setNPCFlags(ScriptEvent& event, GameVariableStore& variableStore, NPCPtr npc)
{
	if (npc == nullptr)
		return;

	variableStore.add("visible", npc->visFlags != PROPID(NPCVisFlags::HIDDEN));
	variableStore.add("shotbyplayer", event.type == ScriptEventType::WASSHOT && event.initiator.second == ScriptObjectType::PLAYER);
	variableStore.add("shotbybaddy", event.type == ScriptEventType::WASSHOT && event.initiator.second == ScriptObjectType::SERVER);

	// Extension.
	variableStore.add("shotbynpc", event.type == ScriptEventType::WASSHOT && event.initiator.second == ScriptObjectType::NPC);

	variableStore.add("peltwithblackstone", false);
	variableStore.add("peltwithbush", false);
	variableStore.add("peltwithnpc", false);
	variableStore.add("peltwithsign", false);
	variableStore.add("peltwithstone", false);
	variableStore.add("peltwithvase", false);
}

void setLevelFlags(GameVariableStore& variableStore, NPCPtr npc, LevelPtr level)
{
	variableStore.add("issparringzone", level != nullptr && npc != nullptr && level->isSparringZone(npc->character.getMapPosition()));
	variableStore.add("nopkzone", level != nullptr && npc != nullptr && level->isNoPkZone(npc->character.getMapPosition()));
	variableStore.add("isonmap", level != nullptr && level->getMap() != nullptr);
	variableStore.add("compsdead", level != nullptr && !level->hasLivingBaddies());
}

void setWeaponFlags(ScriptEvent& event, ScriptObject source, GameVariableStore& variableStore)
{
	variableStore.add("isweapon", source.second == ScriptObjectType::WEAPON);
}

void setOtherFlags(ScriptEvent& event, ScriptObject source, GameVariableStore& variableStore, NPCPtr npc, PlayerPtr player, LevelPtr level)
{
	// actionplayer
	if (event.type == ScriptEventType::TRIGGERACTION && event.initiator.second == ScriptObjectType::PLAYER && level != nullptr)
	{
		bool found = false;
		size_t index = 0;
		for (const auto& playerId : level->getPlayers())
		{
			if (playerId == static_cast<PlayerID>(source.first))
			{
				found = true;
				break;
			}
			++index;
		}
		variableStore.add("actionplayer", GameValue{ (double)(found ? index : -1) });
	}

	// playerswimming
	variableStore.add("playerswimming", player != nullptr && level != nullptr && inList(level->getTileTypeAt(player->getGlobalPosition().translate(24, 32)), tileset::TileType::WATER, tileset::TileType::LAVA));

	/* Older flags:
	* 'gotbow' and 'gotsword' are older pre-1.3 flags.
	*/
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1
