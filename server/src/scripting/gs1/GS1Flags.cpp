#include <any>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include <IEnums.h>

#include <BabyDI.h>
#include <Server.h>
#include <level/LevelTileTypes.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <player/PlayerClient.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <scripting/gs1/GS1Flags.h>
#include <scripting/gs1/ScriptEngineGS1.h>
#include <utilities/CommonTypes.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////

static bool hasEvent(const ScriptEventType check, const ScriptEventType& eventType, std::vector<ScriptEventType>* additionalEventTypes)
{
	return check == eventType || (additionalEventTypes != nullptr && std::ranges::contains(*additionalEventTypes, check));
}

void setEventFlags(const ScriptEventType event, std::vector<ScriptEventType>* additionalEventTypes, GameVariableStore& variableStore)
{
	// Set all our built-in event flags.
	for (auto& [eventType, flagName] : eventFlagMap)
		variableStore.add(flagName, hasEvent(eventType, event, additionalEventTypes));

	// Valid alternates.
	variableStore.add("playerhurted", hasEvent(ScriptEventType::PLAYERHURT, event, additionalEventTypes));
	variableStore.add("wasshooted", hasEvent(ScriptEventType::WASSHOT, event, additionalEventTypes));

	// TODO: Put extensions under a server option?
	variableStore.add("playertouchesme", hasEvent(ScriptEventType::PLAYERTOUCHSME, event, additionalEventTypes));
	variableStore.add("playertouchesother", hasEvent(ScriptEventType::PLAYERTOUCHSOTHER, event, additionalEventTypes));
}

void setTriggerActionAndCustomEventFlags(ScriptEvent& event, GameVariableStore& variableStore)
{
	if (event.args.empty() || (event.type != ScriptEventType::TRIGGERACTION && event.type != ScriptEventType::CUSTOM))
		return;

	std::string action;
	if (const auto* actionStr = std::any_cast<std::string>(&event.args[0]); actionStr != nullptr)
		action = *actionStr;
	else if (const auto* actionChar = std::any_cast<const char*>(&event.args[0]); actionChar != nullptr)
		action = *actionChar;
	else if (const auto* actionStrView = std::any_cast<std::string_view>(&event.args[0]); actionStrView != nullptr)
		action = std::string(*actionStrView);

	if (!action.empty())
	{
		if (event.type == ScriptEventType::TRIGGERACTION)
			action.insert(0, "action");

		// Set the action flag.
		// Set both the original action and a lowercased version.
		variableStore.add(GameVariable{.name = action, .value{true}, .lifetime = variables::Lifetime::TEMPORARY});
		variableStore.add(GameVariable{.name = string::toLower(action), .value{true}, .lifetime = variables::Lifetime::TEMPORARY});

		// If there are just two arguments, try to unpack the second argument.
		if (event.args.size() == 2)
		{
			if (const auto* params = std::any_cast<std::string>(&event.args[1]); params != nullptr)
			{
				// Split the parameters by commas.
				if (auto tokens = string::fromCSV(*params); tokens.size() > 1)
				{
					event.args.erase(event.args.begin() + 1);
					event.args.insert(event.args.end(), std::ranges::begin(tokens), std::ranges::end(tokens));
				}
			}
		}
	}
}

void setPlayerFlags(GameVariableStore& variableStore, const NPCPtr& npc, const PlayerPtr& player)
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

	const auto level = player->getLevel();
	variableStore.add("isleader", level != nullptr && level->isPlayerLeader(player->getId()));
}

void setNPCFlags(const ScriptEvent& event, GameVariableStore& variableStore, const NPCPtr& npc)
{
	if (npc == nullptr)
		return;

	variableStore.add("visible", npc->visFlags != PROPID(NPCVisFlags::HIDDEN));

	if (const auto server = BabyDI::Get<Server>(); server != nullptr && server->Generation == ServerGeneration::CLASSIC)
	{
		variableStore.add("gotsword", npc->character.swordPower == 1);
		variableStore.add("gotaxe", npc->character.swordPower == 2);
		variableStore.add("gotlizardsword", npc->character.swordPower == 3);
		variableStore.add("gotgoldensword", npc->character.swordPower == 4);
		//
		variableStore.add("gotshield", npc->character.shieldPower == 1);
		variableStore.add("gotmirrorshield", npc->character.shieldPower == 2);
		variableStore.add("gotlizardshield", npc->character.shieldPower == 3);
		//
		variableStore.add("gotbomb", npc->character.bombPower == 1);
		variableStore.add("gotsuperbomb", npc->character.bombPower == 2);
		//
		variableStore.add("gotbow", npc->character.bowPower == 1);
		//
		variableStore.add("gotglove1", npc->character.glovePower == 1);
		variableStore.add("gotglove2", npc->character.glovePower == 2);
	}

	variableStore.add("shotbyplayer", event.type == ScriptEventType::WASSHOT && event.initiator.second == ScriptObjectType::PLAYER);
	variableStore.add("shotbybaddy", event.type == ScriptEventType::WASSHOT && event.initiator.second == ScriptObjectType::SERVER);
	variableStore.add("shotbynpc", event.type == ScriptEventType::WASSHOT && event.initiator.second == ScriptObjectType::NPC); // GR extension

	// The WASPELT event has the item in the event args so pull it out.
	auto carryType = CarryObjectType::NONE;
	if (event.type == ScriptEventType::WASPELT && !event.args.empty())
	{
		if (const auto type = std::any_cast<CarryObjectType>(&event.args.front()); type != nullptr)
			carryType = *type;
	}

	variableStore.add("peltwithblackstone", carryType == CarryObjectType::BLACKSTONE);
	variableStore.add("peltwithbush", carryType == CarryObjectType::BUSH);
	variableStore.add("peltwithnpc", carryType == CarryObjectType::NPC);
	variableStore.add("peltwithperson", carryType == CarryObjectType::NPC);
	variableStore.add("peltwithsign", carryType == CarryObjectType::SIGN);
	variableStore.add("peltwithstone", carryType == CarryObjectType::STONE);
	variableStore.add("peltwithvase", carryType == CarryObjectType::VASE);
	variableStore.add("peltwithplayer", carryType == CarryObjectType::PLAYER);
}

void setLevelFlags(GameVariableStore& variableStore, const NPCPtr& npc, const LevelPtr& level)
{
	variableStore.add("issparringzone", level != nullptr && npc != nullptr && level->isSparringZone(npc->character.getMapPosition()));
	variableStore.add("nopkzone", level != nullptr && npc != nullptr && level->isNoPkZone(npc->character.getMapPosition()));
	variableStore.add("isonmap", level != nullptr && level->getMap() != nullptr);
	variableStore.add("compsdead", level != nullptr && !level->hasLivingBaddies());
}

void setWeaponFlags(ScriptEvent& event, const ScriptObject& source, GameVariableStore& variableStore)
{
	variableStore.add("isweapon", source.second == ScriptObjectType::WEAPON);
}

void setOtherFlags(const ScriptEvent& event, const ScriptObject& source, GameVariableStore& variableStore, const PlayerPtr& player, const LevelPtr& level)
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
		variableStore.add("actionplayer", GameValue{static_cast<double>(found ? index : -1)});
	}

	// playerswimming
	variableStore.add("playerswimming", player != nullptr && level != nullptr && inList(level->getTileTypeAt(player->getGlobalPosition().translate(24, 32)), tileset::TileType::WATER, tileset::TileType::LAVA));

	/* Older flags:
	* 'gotbow' and 'gotsword' are older pre-1.3 flags.
	*/
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1
