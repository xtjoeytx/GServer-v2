#include <Server.h>
#include <scripting/gs1/GS1Flags.h>
#include <scripting/gs1/ScriptEngineGS1.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////

void setEventFlags(ScriptEventType event, ScriptVariableStore& variableStore)
{
	static const std::unordered_map<ScriptEventType, std::string_view> eventFlagMap =
	{
		{ ScriptEventType::CREATED, ScriptEventFlagNames::CREATED },
		{ ScriptEventType::INITIALIZED, ScriptEventFlagNames::INITIALIZED },
		{ ScriptEventType::PLAYERLOGIN, ScriptEventFlagNames::PLAYERLOGIN },
		{ ScriptEventType::PLAYERLOGOUT, ScriptEventFlagNames::PLAYERLOGOUT },
		{ ScriptEventType::PLAYERENTERS, ScriptEventFlagNames::PLAYERENTERS },
		{ ScriptEventType::PLAYERLEAVES, ScriptEventFlagNames::PLAYERLEAVES },
		{ ScriptEventType::PLAYERTOUCHSME, ScriptEventFlagNames::PLAYERTOUCHSME },
		{ ScriptEventType::PLAYERTOUCHSOTHER, ScriptEventFlagNames::PLAYERTOUCHSOTHER },
		{ ScriptEventType::PLAYERLAYSITEM, ScriptEventFlagNames::PLAYERLAYSITEM },
		{ ScriptEventType::PLAYERCHATS, ScriptEventFlagNames::PLAYERCHATS },
		{ ScriptEventType::PLAYERDIES, ScriptEventFlagNames::PLAYERDIES },
		{ ScriptEventType::PLAYERENDREADING, ScriptEventFlagNames::PLAYERENDREADING },
		{ ScriptEventType::WEAPONFIRED, ScriptEventFlagNames::WEAPONFIRED },
		{ ScriptEventType::FIREDONHORSE, ScriptEventFlagNames::FIREDONHORSE },
		{ ScriptEventType::COMPUSDIED, ScriptEventFlagNames::COMPUSDIED },
		{ ScriptEventType::WARPED, ScriptEventFlagNames::WARPED },
		{ ScriptEventType::NPCWARPED, ScriptEventFlagNames::NPCWARPED },
		{ ScriptEventType::EXPLODED, ScriptEventFlagNames::EXPLODED },
		{ ScriptEventType::WASHIT, ScriptEventFlagNames::WASHIT },
		{ ScriptEventType::WASSHOT, ScriptEventFlagNames::WASSHOT },
		{ ScriptEventType::WASPELT, ScriptEventFlagNames::WASPELT },
		{ ScriptEventType::TIMEOUT, ScriptEventFlagNames::TIMEOUT },
		//
		{ ScriptEventType::SERVERLISTCONNECT, ScriptEventFlagNames::SERVERLISTCONNECT }
	};

	auto it = eventFlagMap.find(event);
	if (it != eventFlagMap.end())
	{
		auto flagName = it->second;
		variableStore.add(std::string{ flagName } + "|double", 1.0);

		// TODO: Put extensions under a server option?
		if (event == ScriptEventType::PLAYERTOUCHSME)
			variableStore.add(std::string{ ScriptEventFlagNames::PLAYERTOUCHESME } + "|double", 1.0);
		if (event == ScriptEventType::PLAYERTOUCHSOTHER)
			variableStore.add(std::string{ ScriptEventFlagNames::PLAYERTOUCHESOTHER } + "|double", 1.0);
	}
}

void setPlayerFlags(ScriptVariableStore& variableStore, NPCPtr npc, PlayerClientPtr player)
{
	if (player == nullptr)
		return;

	if ((player->account.status & PLSTATUS_HASSPIN) != 0)
		variableStore.add("canspin|double", 1.0);

	/* TODO(Nalin): Carry sprite flags. PLPROP_CARRYSPRITE
		carrying           the player carries something
		carriesblackstone  the player carries a blackstone
		carriesbush        the player carries a bush
		carriessign        the player carries a sign
		carriesstone       the player carries a stone
		carriesvase        the player carries a vase
	*/

	if ((player->account.status & PLSTATUS_ALLOWWEAPONS) != 0)
		variableStore.add("weaponsenabled|double", 1.0);
	if ((player->account.status & PLSTATUS_PAUSED) != 0)
		variableStore.add("playerpaused|double", 1.0);
	if ((player->account.status & PLSTATUS_MALE) != 0)
		variableStore.add("playerismale|double", 1.0);
	if ((player->account.status & PLSTATUS_MALE) == 0)
		variableStore.add("playerisfemale|double", 1.0);
	if (!player->account.character.horseImage.empty())
		variableStore.add("playeronhorse|double", 1.0);

	// TODO(Nalin): playerswimming - How does this work?  Does it check for the swim gani, or does it do a tile type check?
	/*
		playerlaysitem
		playertrial
	*/

	if (npc != nullptr && player->getAttachedNPC() == npc->id)
		variableStore.add("playerattached|double", 1.0);
	if (auto level = player->getLevel(); level != nullptr && level->isPlayerLeader(player->getId()))
		variableStore.add("isleader|double", 1.0);
}

void setNpcFlags(ScriptVariableStore& variableStore, NPCPtr npc)
{
	if (npc == nullptr)
		return;

	// TODO(Nalin): timeout

	if (npc->visFlags != PROPID(NPCVisFlags::HIDDEN))
		variableStore.add("visible|double", 1.0);

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

void setLevelFlags(ScriptVariableStore& variableStore, NPCPtr npc, LevelPtr level)
{
	if (level == nullptr)
		return;

	if (level->isSparringZone())
		variableStore.add("issparringzone|double", 1.0);
	if (level->isNoPkZone())
		variableStore.add("nopkzone|double", 1.0);
	if (level->getMap() != nullptr)
		variableStore.add("isonmap|double", 1.0);
	if (!level->hasLivingBaddies())
		variableStore.add("compsdead|double", 1.0);

	/*
		levelorgx     level origin(x), can be different to 0, 0 if the player is attached to an npc
		levelorgy     level origin(y)
		gravity       the rate at which shot projectiles fall (default Z loss of 2 tiles per second)
	*/
}

void setOtherFlags(ScriptVariableStore& variableStore, NPCPtr npc, PlayerClientPtr player, LevelPtr level)
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
	// allstats - Should probably be a lexer thing like allfeatures.
	// actionplayer

	/* Older flags:
	* 'gotbow' and 'gotsword' are older pre-1.3 flags.
	*/
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1
