#include <chrono>
#include <iterator>
#include <ranges>
#include <utility>
#include <vector>

#include <BabyDI.h>
#include <Server.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <player/PlayerClient.h>
#include <scripting/gs1/GS1Variables.h>
#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////

void setReadOnlyGlobalVariables(GameVariableStore& variableStore)
{
	auto* server = BabyDI::Get<Server>();

	// timevar
	variableStore.add(GameVariable{ "timevar", [server](auto) -> GameValue { return static_cast<double>(server->getNWTime()); }, {} });
	variableStore.add(GameVariable{ "timevar2", [server](auto) -> GameValue { return static_cast<double>(chrono::high_resolution_clock::now().time_since_epoch().count()); }, {} });

	// allplayers
	variableStore.add(GameVariable{ "allplayerscount",
		[server](auto) -> GameValue
		{
			auto size = std::ranges::distance(server->getPlayerList() | std::views::filter([](auto& kvp) { return dynamic_cast<PlayerClient*>(kvp.second.get()) != nullptr; }));
			return static_cast<double>(size);
		}, {}
	});
	variableStore.add(GameVariable{ "allplayers",
		[server](auto) -> GameValue
		{
			auto playerObjects = server->getPlayerList()
					| std::views::filter([](auto& kvp) { return dynamic_cast<PlayerClient*>(kvp.second.get()) != nullptr; })
					| std::views::transform([](auto& kvp) { return ScriptObjectSource{ std::make_pair((size_t)kvp.first, ScriptObjectSourceType::PLAYER)}; });
			std::vector<ScriptObjectSource> players{ std::from_range, playerObjects };
			return players;
		}, {}
	});

	/*
		gravity       the rate at which shot projectiles fall (default Z loss of 2 tiles per second)
	*/
}

void setPlayerVariables(GameVariableStore& variableStore, PlayerClientPtr player)
{
	// weaponscount
	variableStore.add(GameVariable{ "weaponscount", [&player](auto) -> GameValue { return static_cast<double>(player->account.weapons.size()); }, {} });

	// playerhurtpower
}

void setLevelVariables(GameVariableStore& variableStore, NPCPtr npc, PlayerClientPtr player, LevelPtr level)
{
	auto* server = BabyDI::Get<Server>();

	// players
	variableStore.add(GameVariable{ "playerscount", [&level](auto) -> GameValue { return static_cast<double>(level->getPlayers().size()); } , {} });
	variableStore.add(GameVariable{ "players",
		[&level](auto) -> GameValue
		{
			auto playerObjects = level->getPlayers()
				| std::views::transform([](const PlayerID& id) { return ScriptObjectSource{ std::make_pair((size_t)id, ScriptObjectSourceType::PLAYER) }; });
			std::vector<ScriptObjectSource> players{ std::from_range, playerObjects };
			return players;
		}, {}
	});

	// npcs
	variableStore.add(GameVariable{ "npcscount", [&level](auto) -> GameValue { return static_cast<double>(level->getNPCs().size()); } , {} });
	variableStore.add(GameVariable{ "npcs",
		[&level](auto) -> GameValue
		{
			auto npcObjects = level->getNPCs()
				| std::views::transform([](const NPCID& id) { return ScriptObjectSource{ std::make_pair((size_t)id, ScriptObjectSourceType::NPC) }; });
			std::vector<ScriptObjectSource> npcs{ std::from_range, npcObjects };
			return npcs;
		}, {}
	});

	// compus

	// bombs

	// arrows

	// items

	// explos

	// horses

	// signs

	// levelorigx / levelorigy
	variableStore.add(GameVariable{ "levelorigx",
		[server, &player](auto) -> GameValue
		{
			if (auto npc = server->getNPC(player->getAttachedNPC()); npc != nullptr)
				return -npc->character.pixelX / 16.0;
			return 0.0;
		}, {}
	});
	variableStore.add(GameVariable{ "levelorigy",
		[server, &player](auto) -> GameValue
		{
			if (auto npc = server->getNPC(player->getAttachedNPC()); npc != nullptr)
				return -npc->character.pixelY / 16.0;
			return 0.0;
		}, {}
	});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1
