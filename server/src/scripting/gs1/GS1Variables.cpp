#include <algorithm>
#include <any>
#include <chrono>
#include <format>
#include <iterator>
#include <memory>
#include <ranges>
#include <utility>
#include <vector>

#include <BabyDI.h>

#include <Server.h>
#include <level/Level.h>
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
	variableStore.add(GameValue{ "timevar", gameValueGetter([server]() { return static_cast<double>(server->getNWTime()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "timevar2", gameValueGetter([server]() { return static_cast<double>(server->getFrameStartTimeHighPrecision().time_since_epoch().count()); }), GameValue::func_set{} });

	// allplayers
	variableStore.add(GameValue{ "allplayerscount",
		gameValueGetter([server]()
		{
			auto size = std::ranges::distance(server->getNPCServer()->getPlayerList() | std::views::filter([](auto& kvp) { return dynamic_cast<PlayerClient*>(kvp.second.get()) != nullptr; }));
			return static_cast<double>(size);
		}), GameValue::func_set{}
	});
	variableStore.add(GameValue{ "allplayers",
		gameValueGetter([server]()
		{
			auto playerObjects = server->getNPCServer()->getPlayerList()
					| std::views::filter([](auto& kvp) { return dynamic_cast<PlayerClient*>(kvp.second.get()) != nullptr; })
					| std::views::transform([](auto& kvp) { return ScriptObject{ std::make_pair((size_t)kvp.first, ScriptObjectType::PLAYER)}; });
			std::vector<ScriptObject> players{ std::ranges::begin(playerObjects), std::ranges::end(playerObjects) };
			return players;
		}), GameValue::func_set{}
	});

	/*
		gravity       the rate at which shot projectiles fall (default Z loss of 2 tiles per second)
		waterheight
		nwday
		nwhour
		nwmin
		nwmonth
		nwtime
		nwweek
		nwweekday
		nwyear
	*/
}

void setNPCVariables(GameVariableStore& variableStore, std::weak_ptr<NPC> npc)
{
	auto npcPtr = npc.lock();
	if (npcPtr == nullptr)
		return;

	// Explicit timeout variable on the npc to avoid issues with it also being a flag.
	/*
	npcPtr->scripting.variables.add(GameVariable{ set_temporary, "timeout",
		gameVariableGetter([npc]() { return npc.expired() ? 0.0 : npc.lock()->timeout.count() / 1000.0; }),
		gameVariableSetter(npcPtr.get(), PROPOPT<NPCProp>(std::nullopt),
			[npc](const GameValue& value, std::optional<size_t>)
			{
				if (auto* doubleValue = value.get_unsafe<double>(); doubleValue != nullptr && !npc.expired())
					npc.lock()->timeout = std::chrono::milliseconds(static_cast<int>(*doubleValue * 1000));
			})
		});
	*/
}

void setPlayerVariables(GameVariableStore& variableStore, std::weak_ptr<PlayerClient> player)
{
	auto playerPtr = player.lock();
	if (playerPtr == nullptr)
		return;

	auto* server = BabyDI::Get<Server>();

	// weaponscount
	variableStore.add(GameValue{ "weaponscount",
		gameValueGetter([player]() { return player.expired() ? 0.0 : static_cast<double>(player.lock()->account.weapons.size()); }), GameValue::func_set{}});

	// playerhurtpower

	// levelorgx / levelorgy
	variableStore.add(GameValue{ "levelorgx",
		gameValueGetter([server, player]()
		{
			if (player.expired()) return 0.0;
			if (auto npc = server->getNPC(player.lock()->getAttachedNPC()); npc != nullptr)
				return -npc->character.getGlobalPosition().x() / 16.0;
			return 0.0;
		}), GameValue::func_set{}
	});
	variableStore.add(GameValue{ "levelorgy",
		gameValueGetter([server, player]()
		{
			if (player.expired()) return 0.0;
			if (auto npc = server->getNPC(player.lock()->getAttachedNPC()); npc != nullptr)
				return -npc->character.getGlobalPosition().y() / 16.0;
			return 0.0;
		}), GameValue::func_set{}
	});

	// all the player property shortcuts
	for (const auto& [name, variable] : playerPtr->scriptParameters)
		variableStore.add(GameValue{ set_temporary, std::format("player{}", name), variable.getGetter(), variable.getSetter() });
}

void setLevelVariables(GameVariableStore& variableStore, std::weak_ptr<Level> level)
{
	// TODO: These variables should be stored on the level so they don't get remade for every single script.  Like the object parameters stuff.

	if (level.expired())
		return;

	// players
	variableStore.add(GameValue{ "playerscount", gameValueGetter([level]() { return level.expired() ? 0.0 : static_cast<double>(level.lock()->getMapPlayerCount()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "players",
		gameValueGetter([level]()
		{
			if (level.expired()) return std::vector<ScriptObject>{};
			auto playerObjects = level.lock()->getMapPlayers()
				| std::views::transform([](const PlayerID& id) { return ScriptObject{ std::make_pair((size_t)id, ScriptObjectType::PLAYER) }; });
			std::vector<ScriptObject> players;
			std::ranges::copy(playerObjects, std::back_inserter(players));
			return players;
		}), GameValue::func_set{}
	});

	// npcs
	variableStore.add(GameValue{ "npcscount", gameValueGetter([level]() { return level.expired() ? 0.0 : static_cast<double>(level.lock()->getMapNPCCount()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "npcs",
		gameValueGetter([level]()
		{
			if (level.expired()) return std::vector<ScriptObject>{};
			auto npcObjects = level.lock()->getMapNPCs()
				| std::views::transform([](const NPCID& id) { return ScriptObject{ std::make_pair((size_t)id, ScriptObjectType::NPC) }; });
			std::vector<ScriptObject> npcs;
			std::ranges::copy(npcObjects, std::back_inserter(npcs));
			return npcs;
		}), GameValue::func_set{}
	});

	// compus
	variableStore.add(GameValue{ "compuscount", gameValueGetter([level]() { return level.expired() ? 0.0 : static_cast<double>(level.lock()->getBaddies().size()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "compus",
		gameValueGetter([level]()
		{
			if (level.expired()) return std::vector<ScriptObject>{};
			auto objects = level.lock()->getBaddies()
				| std::views::filter([](const LevelBaddy& baddy) { return baddy.mode != BaddyMode::DEAD; })
				| std::views::transform([](const LevelBaddy& baddy) { return ScriptObject{ std::make_pair((size_t)baddy.id, ScriptObjectType::BADDY) }; });
			std::vector<ScriptObject> objectList{ std::ranges::begin(objects), std::ranges::end(objects) };
			return objectList;
		}), GameValue::func_set{}
	});

	// bombs
	variableStore.add(GameValue{ "bombscount", gameValueGetter([level]() { return level.expired() ? 0.0 : static_cast<double>(level.lock()->getMapBombCount()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "bombs",
		gameValueGetter([level]()
		{
			auto levelPtr = level.lock();
			std::vector<ScriptObject> objectList{};
			for (size_t i = 0; levelPtr && i < levelPtr->getMapBombCount(); ++i)
				objectList.emplace_back(std::make_pair(i, ScriptObjectType::BOMB));
			return objectList;
		}), GameValue::func_set{}
	});

	// arrows
	variableStore.add(GameValue{ "arrowscount", gameValueGetter([level]() { return level.expired() ? 0.0 : static_cast<double>(level.lock()->getMapArrowCount()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "arrows",
		gameValueGetter([level]()
		{
			auto levelPtr = level.lock();
			std::vector<ScriptObject> objectList{};
			for (size_t i = 0; levelPtr && i < levelPtr->getMapArrowCount(); ++i)
				objectList.emplace_back(std::make_pair(i, ScriptObjectType::ARROW));
			return objectList;
		}), GameValue::func_set{}
	});

	// items
	variableStore.add(GameValue{ "itemscount", gameValueGetter([level]() { return level.expired() ? 0.0 : static_cast<double>(level.lock()->getMapItemCount()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "items",
		gameValueGetter([level]()
		{
			auto levelPtr = level.lock();
			std::vector<ScriptObject> objectList{};
			for (size_t i = 0; levelPtr && i < levelPtr->getMapItemCount(); ++i)
				objectList.emplace_back(std::make_pair(i, ScriptObjectType::ITEM));
			return objectList;
		}), GameValue::func_set{}
	});

	// explos
	variableStore.add(GameValue{ "exploscount", gameValueGetter([level]() { return level.expired() ? 0.0 : static_cast<double>(level.lock()->getMapExplosionCount()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "explos",
		gameValueGetter([level]()
		{
			auto levelPtr = level.lock();
			std::vector<ScriptObject> objectList{};
			for (size_t i = 0; levelPtr && i < levelPtr->getMapExplosionCount(); ++i)
				objectList.emplace_back(std::make_pair(i, ScriptObjectType::EXPLOSION));
			return objectList;
		}), GameValue::func_set{}
	});

	// horses
	variableStore.add(GameValue{ "horsescount", gameValueGetter([level]() { return level.expired() ? 0.0 : static_cast<double>(level.lock()->getMapHorseCount()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "horses",
		gameValueGetter([level]()
		{
			auto levelPtr = level.lock();
			std::vector<ScriptObject> objectList{};
			for (size_t i = 0; levelPtr && i < levelPtr->getMapHorseCount(); ++i)
				objectList.emplace_back(std::make_pair(i, ScriptObjectType::HORSE));
			return objectList;
		}), GameValue::func_set{}
	});

	// signs
	variableStore.add(GameValue{ "signscount", gameValueGetter([level]() { return level.expired() ? 0.0 : static_cast<double>(level.lock()->getMapSignCount()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "signs",
		gameValueGetter([level]()
		{
			auto levelPtr = level.lock();
			std::vector<ScriptObject> objectList{};
			for (size_t i = 0; levelPtr && i < levelPtr->getMapSignCount(); ++i)
				objectList.emplace_back(std::make_pair(i, ScriptObjectType::SIGN));
			return objectList;
		}), GameValue::func_set{}
	});

	// board[]
	variableStore.add(GameValue{ "board",
		gameValueGetter([level]()
		{
			std::vector<double> tiles;
			if (auto levelPtr = level.lock(); levelPtr != nullptr)
			{
				const auto& levelTiles = levelPtr->getTiles(0).tiles();
				tiles.insert(tiles.end(), std::ranges::begin(levelTiles), std::ranges::end(levelTiles));
			}
			return tiles;
		}), GameValue::func_set{}
	});

	// tiles[x,y] is directly handled in the GS1Visitor and is aliased to the "board" variable.
}

void setOtherVariables(GameVariableStore& variableStore, ScriptEvent& event)
{
	// paramscount
	variableStore.add(GameValue{ "paramscount",
		gameValueGetter([&event]()
		{
			return static_cast<double>(std::max<std::vector<std::any>::size_type>(1, event.args.size()) - 1);
		}), GameValue::func_set{}
	});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1
