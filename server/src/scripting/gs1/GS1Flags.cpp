#include <string_view>
#include <unordered_map>

#include <IEnums.h>

#include <Server.h>
#include <object/NPC.h>
#include <player/PlayerClient.h>
#include <scripting/gs1/GS1Flags.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>

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

	auto it = eventFlagMap.find(event);
	if (it != eventFlagMap.end())
	{
		auto flagName = it->second;
		variableStore.add(flagName, true);

		// TODO: Put extensions under a server option?
		if (event == ScriptEventType::PLAYERTOUCHSME)
			variableStore.add("playertouchesme", true);
		if (event == ScriptEventType::PLAYERTOUCHSOTHER)
			variableStore.add("playertouchesother", true);
	}
}

void setPlayerFlags(GameVariableStore& variableStore, NPCPtr npc, PlayerClientPtr player)
{
	if (player == nullptr)
		return;

	if ((player->account.status & PLSTATUS_HASSPIN) != 0)
		variableStore.add("canspin", true);

	/* TODO(Nalin): Carry sprite flags. PLPROP_CARRYSPRITE
		carrying           the player carries something
		carriesblackstone  the player carries a blackstone
		carriesbush        the player carries a bush
		carriessign        the player carries a sign
		carriesstone       the player carries a stone
		carriesvase        the player carries a vase
	*/

	if ((player->account.status & PLSTATUS_ALLOWWEAPONS) != 0)
		variableStore.add("weaponsenabled", true);
	if ((player->account.status & PLSTATUS_PAUSED) != 0)
		variableStore.add("playerpaused", true);
	if ((player->account.status & PLSTATUS_MALE) != 0)
		variableStore.add("playerismale", true);
	if ((player->account.status & PLSTATUS_MALE) == 0)
		variableStore.add("playerisfemale", true);
	if (!player->account.character.horseImage.empty())
		variableStore.add("playeronhorse", true);

	// TODO(Nalin): playerswimming - How does this work?  Does it check for the swim gani, or does it do a tile type check?
	/*
		playerlaysitem
		playertrial
	*/

	if (npc != nullptr && player->getAttachedNPC() == npc->id)
		variableStore.add("playerattached", true);
	if (auto level = player->getLevel(); level != nullptr && level->isPlayerLeader(player->getId()))
		variableStore.add("isleader", true);
}

void setNPCFlags(GameVariableStore& variableStore, NPCPtr npc)
{
	if (npc == nullptr)
		return;

	// TODO(Nalin): timeout

	if (npc->visFlags != PROPID(NPCVisFlags::HIDDEN))
		variableStore.add("visible", true);

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
	if (level == nullptr)
		return;

	if (level->isSparringZone())
		variableStore.add("issparringzone", true);
	if (level->isNoPkZone())
		variableStore.add("nopkzone", true);
	if (level->getMap() != nullptr)
		variableStore.add("isonmap", true);
	if (!level->hasLivingBaddies())
		variableStore.add("compsdead", true);

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
