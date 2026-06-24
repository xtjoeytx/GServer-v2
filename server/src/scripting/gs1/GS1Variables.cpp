#include <algorithm>
#include <any>
#include <chrono>
#include <cstdint>
#include <format>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
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
#include <player/PlayerRC.h>
#include <scripting/gs1/GS1Variables.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////

void setGlobalVariables(GameVariableStore& variableStore)
{
	auto* server = BabyDI::Get<Server>();

	auto playerFilter = std::views::filter([](auto& kvp)
	{
		bool isClient = dynamic_cast<PlayerClient*>(kvp.second.get()) != nullptr;
		bool isRC = dynamic_cast<PlayerRC*>(kvp.second.get()) != nullptr;
		return (isClient || isRC) && kvp.second->getId() != 0;
	});

	// timevar
	variableStore.add(GameValue{ "timevar", gameValueGetter([server]() { return static_cast<double>(server->getNWTime()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "timevar2", gameValueGetter([server]() { return static_cast<double>(server->getFrameStartTimeHighPrecision().time_since_epoch().count()); }), GameValue::func_set{} });

	// allplayers
	variableStore.add(GameValue{ "allplayerscount",
		gameValueGetter([server, playerFilter]()
		{
			auto size = std::ranges::distance(server->getNPCServer()->getPlayerList() | playerFilter);
			return static_cast<double>(size);
		}), GameValue::func_set{}
	});
	variableStore.add(GameValue{ "allplayers",
		gameValueGetter([server, playerFilter]()
		{
			auto playerObjects = server->getNPCServer()->getPlayerList()
				| playerFilter
				| std::views::transform([](auto& kvp) { return ScriptObject{ std::make_pair((size_t)kvp.first, ScriptObjectType::PLAYER)}; });
			std::vector<ScriptObject> players{ std::ranges::begin(playerObjects), std::ranges::end(playerObjects) };
			return players;
		}), GameValue::func_set{}
	});

	// gravity
	variableStore.add(GameValue{ "gravity",
		gameValueGetter([server]()
		{
			return server->Scripting.variables.getValue<double>("gravity").value_or(2.0);
		}),
		gameValueSetter([server](const GameValue& value, std::optional<int64_t> index)
		{
			if (auto var = server->Scripting.variables.get("gravity").lock(); var != nullptr)
				var->set(value.get<double>().value_or(2.0));
		})
	});

	// waterheight
	variableStore.add(GameValue{ "waterheight",
		gameValueGetter([server]()
		{
			return server->Scripting.variables.getValue<double>("waterheight").value_or(0.0);
		}),
		gameValueSetter([server](const GameValue& value, std::optional<int64_t> index)
		{
			if (auto var = server->Scripting.variables.get("waterheight").lock(); var != nullptr)
				var->set(value.get<double>().value_or(0.0));
		})
	});

	// nwtime and derivatives.
	variableStore.add(GameValue{ "nwtime",
		gameValueGetter([server]() { return static_cast<double>(server->getNWTime()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "nwmin",	// 60 min in an hour
		gameValueGetter([server]() { return static_cast<double>(server->getNWTime() % 60); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "nwhour",	// 24 hours in a day
		gameValueGetter([server]() { return static_cast<double>((server->getNWTime() / 60) % 24); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "nwday",	// 28 days in a month
		gameValueGetter([server]() { return static_cast<double>((server->getNWTime() / 1440) % 28); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "nwweekday",
		gameValueGetter([server]() { return static_cast<double>((server->getNWTime() / 1440) % 7) + 1; }), GameValue::func_set{} });
	variableStore.add(GameValue{ "nwweek",	// 4 weeks in a month (7 days per week)
		gameValueGetter([server]() { return static_cast<double>((server->getNWTime() / 10080) % 4); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "nwmonth",	// 10 months in a year
		gameValueGetter([server]() { return static_cast<double>((server->getNWTime() / 40320) % 10); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "nwyear",	// Years start at 1000
		gameValueGetter([server]() { return static_cast<double>((server->getNWTime() / 403200) + 1000); }), GameValue::func_set{} });

	// groundheights[]
	variableStore.add(GameValue{ "groundheights",
		gameValueGetter([server](std::optional<int64_t> index)
		{
			if (!index.has_value() || index.value() < 0 || index.value() >= (int64_t)server->groundHeights.size())
				return 0.0;
			return server->groundHeights.at(index.value());
		}),
		gameValueSetter([server](const GameValue& value, std::optional<int64_t> index)
		{
			if (!index.has_value() || index.value() < 0 || index.value() >= (int64_t)server->groundHeights.size())
				return;
			server->groundHeights.at(index.value()) = value.get<double>().value_or(0.0);
		})
	});
}

void setNPCVariables(GameVariableStore& variableStore, std::weak_ptr<NPC> npc)
{
	auto npcPtr = npc.lock();
	if (npcPtr == nullptr)
		return;

	// board[]
	// This variable only checks the sub-level board data, so we need to know the NPC's level and position.
	variableStore.add(GameValue{ "board",
		gameValueGetter([npc](std::optional<int64_t> index) -> GameValue
		{
			auto npcPtr = npc.lock();
			if (npcPtr == nullptr) return 0.0;

			auto levelPtr = npcPtr->getLevel();
			if (levelPtr == nullptr || index.value_or(0) < 0 || index.value_or(0) >= 4096) return 0.0;

			const auto& levelTiles = levelPtr->getTiles(npcPtr->character.getMapPosition());
			if (!levelTiles.has_value())
				return 0.0;

			if (!index.has_value())
			{
				std::vector<double> tiles;
				tiles.insert(tiles.end(), std::ranges::begin(*levelTiles.value()), std::ranges::end(*levelTiles.value()));
				return tiles;
			}

			return static_cast<double>(levelTiles.value()->at(index.value()));
		}), GameValue::func_set{}
	});
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
	playerPtr->constructScriptParameters();
	for (const auto& [name, variable] : playerPtr->scriptParameters)
		variableStore.add(GameValue{ set_temporary, std::format("player{}", name), variable.getGetter(), variable.getSetter() });
}

void setLevelVariables(GameVariableStore& variableStore, std::weak_ptr<Level> level)
{
	// TODO: These variables should be stored on the level so they don't get remade for every single script.  Like the object parameters stuff.

	if (level.expired())
		return;

	// players
	variableStore.add(GameValue{ "playerscount", gameValueGetter([level]() { return level.expired() ? 0.0 : static_cast<double>(level.lock()->getPlayers().size()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "players",
		gameValueGetter([level](std::optional<int64_t> index)
		{
			auto levelPtr = level.lock();
			if (levelPtr == nullptr) return std::vector<ScriptObject>{};
			std::vector<ScriptObject> players;

			if (index.value_or(0) == -1)
			{
				// TODO: Current player.
				return players;
			}

			auto playerObjects = levelPtr->getPlayers()
				| std::views::drop(index.value_or(0))
				| std::views::take(index.has_value() ? 1 : std::numeric_limits<size_t>::max())
				| std::views::transform([](const PlayerID& id) { return ScriptObject{ std::make_pair((size_t)id, ScriptObjectType::PLAYER) }; });

			std::ranges::copy(playerObjects, std::back_inserter(players));
			return players;
		}), GameValue::func_set{}
	});

	// npcs
	variableStore.add(GameValue{ "npcscount", gameValueGetter([level]() { return level.expired() ? 0.0 : static_cast<double>(level.lock()->getNPCs().size()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "npcs",
		gameValueGetter([level](std::optional<int64_t> index)
		{
			auto levelPtr = level.lock();
			if (levelPtr == nullptr || index.value_or(0) < 0) return std::vector<ScriptObject>{};
			std::vector<ScriptObject> npcs;

			auto npcObjects = levelPtr->getNPCs()
				| std::views::drop(index.value_or(0))
				| std::views::take(index.has_value() ? 1 : std::numeric_limits<size_t>::max())
				| std::views::transform([](const NPCID& id) { return ScriptObject{ std::make_pair((size_t)id, ScriptObjectType::NPC) }; });

			std::ranges::copy(npcObjects, std::back_inserter(npcs));
			return npcs;
		}), GameValue::func_set{}
	});

	// compus
	variableStore.add(GameValue{ "compuscount", gameValueGetter([level]() { return level.expired() ? 0.0 : static_cast<double>(level.lock()->getBaddyCount()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "compus",
		gameValueGetter([level](std::optional<int64_t> index)
		{
			auto levelPtr = level.lock();
			if (levelPtr == nullptr || index.value_or(0) < 0) return std::vector<ScriptObject>{};
			std::vector<ScriptObject> compus;

			auto objects = levelPtr->getBaddies()
				| std::views::filter([](const LevelBaddy& baddy) { return baddy.mode != BaddyMode::DEAD; })
				| std::views::drop(index.value_or(0))
				| std::views::take(index.has_value() ? 1 : std::numeric_limits<size_t>::max())
				| std::views::transform([](const LevelBaddy& baddy) { return ScriptObject{ std::make_pair((size_t)baddy.id, ScriptObjectType::BADDY) }; });

			std::ranges::copy(objects, std::back_inserter(compus));
			return compus;
		}), GameValue::func_set{}
	});

	// bombs
	variableStore.add(GameValue{ "bombscount", gameValueGetter([level]() { return level.expired() ? 0.0 : static_cast<double>(level.lock()->getBombs().size()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "bombs",
		gameValueGetter([level](std::optional<int64_t> index)
		{
			auto levelPtr = level.lock();
			if (levelPtr == nullptr || index.value_or(0) < 0) return std::vector<ScriptObject>{};

			if (!index.has_value())
			{
				std::vector<ScriptObject> objectList{};
				for (size_t i = 0; levelPtr && i < levelPtr->getBombs().size(); ++i)
					objectList.emplace_back(std::make_pair(i, ScriptObjectType::BOMB));
				return objectList;
			}

			return std::vector<ScriptObject>{ std::make_pair(index.value(), ScriptObjectType::BOMB) };
		}), GameValue::func_set{}
	});

	// arrows
	variableStore.add(GameValue{ "arrowscount", gameValueGetter([level]() { return level.expired() ? 0.0 : static_cast<double>(level.lock()->getArrows().size()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "arrows",
		gameValueGetter([level](std::optional<int64_t> index)
		{
			auto levelPtr = level.lock();
			if (levelPtr == nullptr || index.value_or(0) < 0) return std::vector<ScriptObject>{};

			if (!index.has_value())
			{
				std::vector<ScriptObject> objectList{};
				for (size_t i = 0; levelPtr && i < levelPtr->getArrows().size(); ++i)
					objectList.emplace_back(std::make_pair(i, ScriptObjectType::ARROW));
				return objectList;
			}

			return std::vector<ScriptObject>{ std::make_pair(index.value(), ScriptObjectType::ARROW) };
		}), GameValue::func_set{}
	});

	// items
	variableStore.add(GameValue{ "itemscount", gameValueGetter([level]() { return level.expired() ? 0.0 : static_cast<double>(level.lock()->getItems().size()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "items",
		gameValueGetter([level](std::optional<int64_t> index)
		{
			auto levelPtr = level.lock();
			if (levelPtr == nullptr || index.value_or(0) < 0) return std::vector<ScriptObject>{};

			if (!index.has_value())
			{
				std::vector<ScriptObject> objectList{};
				for (size_t i = 0; levelPtr && i < levelPtr->getItems().size(); ++i)
					objectList.emplace_back(std::make_pair(i, ScriptObjectType::ITEM));
				return objectList;
			}

			return std::vector<ScriptObject>{ std::make_pair(index.value(), ScriptObjectType::ITEM) };
		}), GameValue::func_set{}
	});

	// explos
	variableStore.add(GameValue{ "exploscount", gameValueGetter([level]() { return level.expired() ? 0.0 : static_cast<double>(level.lock()->getExplosions().size()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "explos",
		gameValueGetter([level](std::optional<int64_t> index)
		{
			auto levelPtr = level.lock();
			if (levelPtr == nullptr || index.value_or(0) < 0) return std::vector<ScriptObject>{};

			if (!index.has_value())
			{
				std::vector<ScriptObject> objectList{};
				for (size_t i = 0; levelPtr && i < levelPtr->getExplosions().size(); ++i)
					objectList.emplace_back(std::make_pair(i, ScriptObjectType::EXPLOSION));
				return objectList;
			}

			return std::vector<ScriptObject>{ std::make_pair(index.value(), ScriptObjectType::EXPLOSION) };
		}), GameValue::func_set{}
	});

	// horses
	variableStore.add(GameValue{ "horsescount", gameValueGetter([level]() { return level.expired() ? 0.0 : static_cast<double>(level.lock()->getHorses().size()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "horses",
		gameValueGetter([level](std::optional<int64_t> index)
		{
			auto levelPtr = level.lock();
			if (levelPtr == nullptr || index.value_or(0) < 0) return std::vector<ScriptObject>{};

			if (!index.has_value())
			{
				std::vector<ScriptObject> objectList{};
				for (size_t i = 0; levelPtr && i < levelPtr->getHorses().size(); ++i)
					objectList.emplace_back(std::make_pair(i, ScriptObjectType::HORSE));
				return objectList;
			}

			return std::vector<ScriptObject>{ std::make_pair(index.value(), ScriptObjectType::HORSE) };
		}), GameValue::func_set{}
	});

	// signs
	variableStore.add(GameValue{ "signscount", gameValueGetter([level]() { return level.expired() ? 0.0 : static_cast<double>(level.lock()->getSignCount()); }), GameValue::func_set{} });
	variableStore.add(GameValue{ "signs",
		gameValueGetter([level](std::optional<int64_t> index)
		{
			auto levelPtr = level.lock();
			if (levelPtr == nullptr || index.value_or(0) < 0) return std::vector<ScriptObject>{};

			if (!index.has_value())
			{
				std::vector<ScriptObject> objectList{};
				for (size_t i = 0; levelPtr && i < levelPtr->getSignCount(); ++i)
					objectList.emplace_back(std::make_pair(i, ScriptObjectType::SIGN));
				return objectList;
			}

			return std::vector<ScriptObject>{ std::make_pair(index.value(), ScriptObjectType::SIGN) };
		}), GameValue::func_set{}
	});

	// board[]
	// Needs the map position of the NPC, so moved there.

	// tiles[x,y] -> tiles[]
	variableStore.add(GameValue{ "tiles",
		gameValueGetter([level](std::optional<int64_t> index)
		{
			auto levelPtr = level.lock();
			if (levelPtr == nullptr || index.value_or(-1) < 0) return 0.0;

			// Get the tile X/Y out of the index.
			uint32_t tileX = static_cast<uint32_t>(index.value() >> 32);
			uint32_t tileY = static_cast<uint32_t>(index.value() & 0xFFFFFFFF);
			TilePosition tilePos{ static_cast<float>(tileX), static_cast<float>(tileY) };

			// Get the tile.
			if (auto tile = levelPtr->getMapTileForEditing(tilePos); tile != nullptr)
				return static_cast<double>(*tile);
			return 0.0;
		}),
		gameValueSetter([level](const GameValue& value, std::optional<int64_t> index)
		{
			auto levelPtr = level.lock();
			if (levelPtr == nullptr || index.value_or(-1) < 0) return;

			// Get the tile X/Y out of the index.
			uint32_t tileX = static_cast<uint32_t>(index.value() >> 32);
			uint32_t tileY = static_cast<uint32_t>(index.value() & 0xFFFFFFFF);
			TilePosition tilePos{ static_cast<float>(tileX), static_cast<float>(tileY) };

			// Get and update the tile.
			if (auto tile = levelPtr->getMapTileForEditing(tilePos); tile != nullptr)
				*tile = static_cast<uint16_t>(value.get<double>().value_or(0.0));
		})
	});
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
