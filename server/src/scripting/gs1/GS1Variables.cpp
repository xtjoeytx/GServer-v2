#include <any>
#include <chrono>
#include <format>
#include <iterator>
#include <optional>
#include <ranges>
#include <utility>
#include <vector>

#include <BabyDI.h>

#include <Server.h>
#include <level/LevelBaddy.h>
#include <npcserver/NPCServer.h>
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
	variableStore.add(GameVariable{ "timevar2", [server](auto) -> GameValue { return static_cast<double>(server->getFrameStartTimeHighPrecision().time_since_epoch().count()); }, {} });

	// allplayers
	variableStore.add(GameVariable{ "allplayerscount",
		[server](auto) -> GameValue
		{
			auto size = std::ranges::distance(server->getNPCServer()->getPlayerList() | std::views::filter([](auto& kvp) { return dynamic_cast<PlayerClient*>(kvp.second.get()) != nullptr; }));
			return static_cast<double>(size);
		}, {}
	});
	variableStore.add(GameVariable{ "allplayers",
		[server](auto) -> GameValue
		{
			auto playerObjects = server->getNPCServer()->getPlayerList()
					| std::views::filter([](auto& kvp) { return dynamic_cast<PlayerClient*>(kvp.second.get()) != nullptr; })
					| std::views::transform([](auto& kvp) { return ScriptObjectSource{ std::make_pair((size_t)kvp.first, ScriptObjectSourceType::PLAYER)}; });
			std::vector<ScriptObjectSource> players{ std::ranges::begin(playerObjects), std::ranges::end(playerObjects) };
			return players;
		}, {}
	});

	// gravity       the rate at which shot projectiles fall (default Z loss of 2 tiles per second)
}

void setNPCVariables(GameVariableStore& variableStore, NPCPtr npc)
{
	if (npc == nullptr)
		return;

	// Explicit timeout variable on the npc to avoid issues with it also being a flag.
	npc->scripting.variables.add(GameVariable{ set_temporary, "timeout",
		gameVariableGetter([npc = std::weak_ptr<NPC>(npc)]() { return npc.expired() ? 0.0 : npc.lock()->timeout.count() / 1000.0; }),
		gameVariableSetter(npc.get(), PROPOPT<NPCProp>(std::nullopt),
			[npc = std::weak_ptr<NPC>(npc)](const GameValue& value, std::optional<size_t>)
			{
				if (auto* doubleValue = value.get_unsafe<double>(); doubleValue != nullptr && !npc.expired())
					npc.lock()->timeout = std::chrono::milliseconds(static_cast<int>(*doubleValue * 1000));
			})
		});
}

void setPlayerVariables(GameVariableStore& variableStore, PlayerClientPtr player)
{
	if (player == nullptr)
		return;

	auto* server = BabyDI::Get<Server>();

	// weaponscount
	variableStore.add(GameVariable{ "weaponscount",
		[player = std::weak_ptr<Player>(player)](auto) -> GameValue
		{
			if (player.expired()) return 0.0;
			return static_cast<double>(player.lock()->account.weapons.size());
		}, {}});

	// playerhurtpower

	// levelorigx / levelorigy
	variableStore.add(GameVariable{ "levelorigx",
		[server, player = std::weak_ptr<Player>(player)](auto) -> GameValue
		{
			if (player.expired()) return 0.0;
			if (auto npc = server->getNPC(player.lock()->getAttachedNPC()); npc != nullptr)
				return -npc->character.pixelX / 16.0;
			return 0.0;
		}, {}
	});
	variableStore.add(GameVariable{ "levelorigy",
		[server, player = std::weak_ptr<Player>(player)](auto) -> GameValue
		{
			if (auto npc = server->getNPC(player.lock()->getAttachedNPC()); npc != nullptr)
				return -npc->character.pixelY / 16.0;
			return 0.0;
		}, {}
	});

	// all the player property shortcuts
	for (const auto& [name, variable] : player->scriptParameters)
		variableStore.add(GameVariable{ set_temporary, std::format("player{}", name), variable.getCallbackGetter(), variable.getCallbackSetter() });
}

void setLevelVariables(GameVariableStore& variableStore, LevelPtr level)
{
	// TODO: These variables should be stored on the level so they don't get remade for every single script.  Like the object parameters stuff.

	if (level == nullptr)
		return;

	auto* server = BabyDI::Get<Server>();

	// players
	variableStore.add(GameVariable{ "playerscount", [level](auto) -> GameValue { return static_cast<double>(level->getPlayers().size()); } , {} });
	variableStore.add(GameVariable{ "players",
		[level](auto) -> GameValue
		{
			auto playerObjects = level->getPlayers()
				| std::views::transform([](const PlayerID& id) { return ScriptObjectSource{ std::make_pair((size_t)id, ScriptObjectSourceType::PLAYER) }; });
			std::vector<ScriptObjectSource> players{ std::ranges::begin(playerObjects), std::ranges::end(playerObjects) };
			return players;
		}, {}
	});

	// npcs
	variableStore.add(GameVariable{ "npcscount", [level](auto) -> GameValue { return static_cast<double>(level->getNPCs().size()); } , {} });
	variableStore.add(GameVariable{ "npcs",
		[level](auto) -> GameValue
		{
			auto npcObjects = level->getNPCs()
				| std::views::transform([](const NPCID& id) { return ScriptObjectSource{ std::make_pair((size_t)id, ScriptObjectSourceType::NPC) }; });
			std::vector<ScriptObjectSource> npcs{ std::ranges::begin(npcObjects), std::ranges::end(npcObjects) };
			return npcs;
		}, {}
	});

	// compus
	variableStore.add(GameVariable{ "compuscount", [level](auto) -> GameValue { return static_cast<double>(level->getBaddies().size()); } , {} });
	variableStore.add(GameVariable{ "compus",
		[level](auto) -> GameValue
		{
			auto objects = level->getBaddies()
				| std::views::filter([](const LevelBaddy& baddy) { return baddy.mode != BaddyMode::DEAD; })
				| std::views::transform([](const LevelBaddy& baddy) { return ScriptObjectSource{ std::make_pair((size_t)baddy.id, ScriptObjectSourceType::BADDY) }; });
			std::vector<ScriptObjectSource> objectList{ std::from_range, objects };
			return objectList;
		}, {}
	});

	// bombs
	variableStore.add(GameVariable{ "bombscount", [level](auto) -> GameValue { return static_cast<double>(level->getBombs().size()); } , {} });
	variableStore.add(GameVariable{ "bombs",
		[level](auto) -> GameValue
		{
			std::vector<ScriptObjectSource> objectList{};
			for (size_t i = 0; i < level->getBombs().size(); ++i)
				objectList.emplace_back(std::make_pair(i, ScriptObjectSourceType::BOMB));
			return objectList;
		}, {}
	});

	// arrows
	/*
	variableStore.add(GameVariable{ "arrowscount", [level](auto) -> GameValue { return static_cast<double>(level->getArrows().size()); } , {} });
	variableStore.add(GameVariable{ "arrows",
		[level](auto) -> GameValue
		{
			std::vector<ScriptObjectSource> objectList{};
			for (size_t i = 0; i < level->getArrows().size(); ++i)
				objectList.emplace_back(std::make_pair(i, ScriptObjectSourceType::ARROW));
			return objectList;
		}, {}
	});
	*/

	// items
	variableStore.add(GameVariable{ "itemscount", [level](auto) -> GameValue { return static_cast<double>(level->getItems().size()); } , {} });
	variableStore.add(GameVariable{ "items",
		[level](auto) -> GameValue
		{
			std::vector<ScriptObjectSource> objectList{};
			for (size_t i = 0; i < level->getItems().size(); ++i)
				objectList.emplace_back(std::make_pair(i, ScriptObjectSourceType::ITEM));
			return objectList;
		}, {}
	});

	// explos
	variableStore.add(GameVariable{ "exploscount", [level](auto) -> GameValue { return static_cast<double>(level->getExplosions().size()); } , {} });
	variableStore.add(GameVariable{ "explos",
		[level](auto) -> GameValue
		{
			std::vector<ScriptObjectSource> objectList{};
			for (size_t i = 0; i < level->getExplosions().size(); ++i)
				objectList.emplace_back(std::make_pair(i, ScriptObjectSourceType::EXPLOSION));
			return objectList;
		}, {}
	});

	// horses
	variableStore.add(GameVariable{ "horsescount", [level](auto) -> GameValue { return static_cast<double>(level->getHorses().size()); } , {} });
	variableStore.add(GameVariable{ "horses",
		[level](auto) -> GameValue
		{
			std::vector<ScriptObjectSource> objectList{};
			for (size_t i = 0; i < level->getHorses().size(); ++i)
				objectList.emplace_back(std::make_pair(i, ScriptObjectSourceType::HORSE));
			return objectList;
		}, {}
	});

	// signs
	variableStore.add(GameVariable{ "signscount", [level](auto) -> GameValue { return static_cast<double>(level->getSigns().size()); } , {} });
	variableStore.add(GameVariable{ "signs",
		[level](auto) -> GameValue
		{
			std::vector<ScriptObjectSource> objectList{};
			for (size_t i = 0; i < level->getSigns().size(); ++i)
				objectList.emplace_back(std::make_pair(i, ScriptObjectSourceType::SIGN));
			return objectList;
		}, {}
	});

	// board[]
	variableStore.add(GameVariable{ "board",
		[level](auto) -> GameValue
		{
			std::vector<double> tiles;
			const auto& levelTiles = level->getTiles(0).tiles();
			tiles.insert(tiles.end(), std::ranges::begin(levelTiles), std::ranges::end(levelTiles));
			return tiles;
		}, {}
	});

	// tiles[x,y]
}

void setOtherVariables(GameVariableStore& variableStore, ScriptEvent& event)
{
	// paramscount
	variableStore.add(GameVariable{ "paramscount",
		[&event](auto) -> GameValue
		{
			return static_cast<double>(std::max<std::vector<std::any>::size_type>(1, event.args.size()) - 1);
		}, {}
	});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1
