#include <algorithm>
#include <any>
#include <cstdint>
#include <format>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <BabyDI.h>

#include <Server.h>
#include <level/Level.h>
#include <level/LevelBaddy.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <scripting/gs1/GS1Variables.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////

void setNPCVariables(GameVariableStore& variableStore, std::weak_ptr<NPC> npc)
{
	auto npcPtr = npc.lock();
	if (npcPtr == nullptr)
		return;

	// board[]
	// This variable only checks the sub-level board data, so we need to know the NPC's level and position.
	GameVariable board{.name = "board"};
	board.registerGetter<double>([npc](std::optional<int64_t> index) -> GameValueVariantForGetter
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
	});
	variableStore.add(std::move(board));
}

void setPlayerVariables(GameVariableStore& variableStore, std::weak_ptr<Player> player)
{
	auto playerPtr = player.lock();
	if (playerPtr == nullptr)
		return;

	auto* server = BabyDI::Get<Server>();

	// weaponscount
	GameVariable weaponscount{.name = "weaponscount"};
	weaponscount.registerGetter<double>([player](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		return player.expired() ? 0.0 : static_cast<double>(player.lock()->account.weapons.size());
	});
	variableStore.add(std::move(weaponscount));

	// levelorgx / levelorgy
	GameVariable levelorgx{.name = "levelorgx"};
	GameVariable levelorgy{.name = "levelorgy"};
	levelorgx.registerGetter<double>([server, player](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		if (player.expired()) return 0.0;
		if (auto npc = server->getNPC(player.lock()->getAttachedNPC()); npc != nullptr)
			return -npc->character.getGlobalPosition().x() / 16.0;
		return 0.0;
	});
	levelorgy.registerGetter<double>([server, player](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		if (player.expired()) return 0.0;
		if (auto npc = server->getNPC(player.lock()->getAttachedNPC()); npc != nullptr)
			return -npc->character.getGlobalPosition().y() / 16.0;
		return 0.0;
	});
	variableStore.add(std::move(levelorgx));
	variableStore.add(std::move(levelorgy));

	// all the player property shortcuts
	playerPtr->constructScriptParameters();
	for (const auto& [name, variable] : playerPtr->scriptParameters)
	{
		// Ignore the message code properties.
		if (name.starts_with('#'))
			continue;

		GameVariable v{.name = std::format("player{}", name), .lifetime = variables::Lifetime::NORMAL, .getters = variable.getters, .setters = variable.setters};
		variableStore.add(std::move(v));
	}
}

void setLevelVariables(GameVariableStore& variableStore, std::weak_ptr<Level> level, std::weak_ptr<NPC> npc, std::weak_ptr<Player> player)
{
	// TODO: These variables should be stored on the level so they don't get remade for every single script.  Like the object parameters stuff.

	if (level.expired())
		return;

	// playerscount
	GameVariable playerscount{.name = "playerscount"};
	playerscount.registerGetter<double>([level](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		return level.expired() ? 0.0 : static_cast<double>(level.lock()->getPlayers().size());
	});
	variableStore.add(std::move(playerscount));

	// players
	GameVariable players{.name = "players"};
	players.registerGetter<std::vector<ScriptObject>>([level, player](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		auto levelPtr = level.lock();
		if (levelPtr == nullptr)
			return std::vector<ScriptObject>{};

		std::vector<ScriptObject> players;
		if (index.value_or(0) < 0)
		{
			if (auto playerClient = player.lock(); playerClient != nullptr && index.value() == -1)
				players.push_back(ScriptObject{std::make_pair((size_t)playerClient->getId(), ScriptObjectType::PLAYER)});
			return players;
		}

		// clang-format off
		auto playerObjects = levelPtr->getPlayers()
			| std::views::drop(index.value_or(0))
			| std::views::take(index.has_value() ? 1 : std::numeric_limits<size_t>::max())
			| std::views::transform([](const PlayerID& id) { return ScriptObject{std::make_pair((size_t)id, ScriptObjectType::PLAYER)}; });
		// clang-format on

		std::ranges::copy(playerObjects, std::back_inserter(players));
		return players;
	});
	variableStore.add(std::move(players));

	// npcscount
	GameVariable npcscount{.name = "npcscount"};
	npcscount.registerGetter<double>([level](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		return level.expired() ? 0.0 : static_cast<double>(level.lock()->getNPCs().size());
	});
	variableStore.add(std::move(npcscount));

	// npcs
	GameVariable npcs{.name = "npcs"};
	npcs.registerGetter<std::vector<ScriptObject>>([level, npc](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		auto levelPtr = level.lock();
		if (levelPtr == nullptr)
			return std::vector<ScriptObject>{};

		std::vector<ScriptObject> npcs;
		if (index.value_or(0) < 0)
		{
			if (auto npcPtr = npc.lock(); npcPtr != nullptr && index.value() == -1)
				npcs.push_back(ScriptObject{std::make_pair((size_t)npcPtr->id, ScriptObjectType::NPC)});
			return npcs;
		}

		// clang-format off
		auto npcObjects = levelPtr->getNPCs()
			| std::views::drop(index.value_or(0))
			| std::views::take(index.has_value() ? 1 : std::numeric_limits<size_t>::max())
			| std::views::transform([](const NPCID& id) { return ScriptObject{std::make_pair((size_t)id, ScriptObjectType::NPC)}; });
		// clang-format on

		std::ranges::copy(npcObjects, std::back_inserter(npcs));
		return npcs;
	});
	variableStore.add(std::move(npcs));

	// compuscount
	GameVariable compuscount{.name = "compuscount"};
	compuscount.registerGetter<double>([level](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		return level.expired() ? 0.0 : static_cast<double>(level.lock()->getBaddyCount());
	});
	variableStore.add(std::move(compuscount));

	// compus
	GameVariable compus{.name = "compus"};
	compus.registerGetter<std::vector<ScriptObject>>([level](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		auto levelPtr = level.lock();
		if (levelPtr == nullptr || index.value_or(0) < 0)
			return std::vector<ScriptObject>{};

		// clang-format off
		std::vector<ScriptObject> compus;
		auto objects = levelPtr->getBaddies()
			| std::views::filter([](const LevelBaddy& baddy) { return baddy.mode != BaddyMode::DEAD; })
			| std::views::drop(index.value_or(0))
			| std::views::take(index.has_value() ? 1 : std::numeric_limits<size_t>::max())
			| std::views::transform([](const LevelBaddy& baddy) { return ScriptObject{std::make_pair((size_t)baddy.id, ScriptObjectType::BADDY)}; });
		// clang-format on

		std::ranges::copy(objects, std::back_inserter(compus));
		return compus;
	});
	variableStore.add(std::move(compus));

	// bombscount
	GameVariable bombscount{.name = "bombscount"};
	bombscount.registerGetter<double>([level](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		return level.expired() ? 0.0 : static_cast<double>(level.lock()->getBombs().size());
	});
	variableStore.add(std::move(bombscount));

	// bombs
	GameVariable bombs{.name = "bombs"};
	bombs.registerGetter<std::vector<ScriptObject>>([level](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		auto levelPtr = level.lock();
		if (levelPtr == nullptr || index.value_or(0) < 0)
			return std::vector<ScriptObject>{};

		if (!index.has_value())
		{
			std::vector<ScriptObject> objectList{};
			for (size_t i = 0; levelPtr && i < levelPtr->getBombs().size(); ++i)
				objectList.emplace_back(std::make_pair(i, ScriptObjectType::BOMB));
			return objectList;
		}

		return std::vector<ScriptObject>{std::make_pair(index.value(), ScriptObjectType::BOMB)};
	});
	variableStore.add(std::move(bombs));

	// arrowscount
	GameVariable arrowscount{.name = "arrowscount"};
	arrowscount.registerGetter<double>([level](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		return level.expired() ? 0.0 : static_cast<double>(level.lock()->getArrows().size());
	});
	variableStore.add(std::move(arrowscount));

	// arrows
	GameVariable arrows{.name = "arrows"};
	arrows.registerGetter<std::vector<ScriptObject>>([level](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		auto levelPtr = level.lock();
		if (levelPtr == nullptr || index.value_or(0) < 0)
			return std::vector<ScriptObject>{};

		if (!index.has_value())
		{
			std::vector<ScriptObject> objectList{};
			for (size_t i = 0; levelPtr && i < levelPtr->getArrows().size(); ++i)
				objectList.emplace_back(std::make_pair(i, ScriptObjectType::ARROW));
			return objectList;
		}

		return std::vector<ScriptObject>{std::make_pair(index.value(), ScriptObjectType::ARROW)};
	});
	variableStore.add(std::move(arrows));

	// itemscount
	GameVariable itemscount{.name = "itemscount"};
	itemscount.registerGetter<double>([level](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		return level.expired() ? 0.0 : static_cast<double>(level.lock()->getItems().size());
	});
	variableStore.add(std::move(itemscount));

	// items
	GameVariable items{.name = "items"};
	items.registerGetter<std::vector<ScriptObject>>([level](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		auto levelPtr = level.lock();
		if (levelPtr == nullptr || index.value_or(0) < 0)
			return std::vector<ScriptObject>{};

		if (!index.has_value())
		{
			std::vector<ScriptObject> objectList{};
			for (size_t i = 0; levelPtr && i < levelPtr->getItems().size(); ++i)
				objectList.emplace_back(std::make_pair(i, ScriptObjectType::ITEM));
			return objectList;
		}

		return std::vector<ScriptObject>{std::make_pair(index.value(), ScriptObjectType::ITEM)};
		return std::vector<ScriptObject>{std::make_pair(index.value(), ScriptObjectType::ITEM)};
	});
	variableStore.add(std::move(items));

	// exploscount
	GameVariable exploscount{.name = "exploscount"};
	exploscount.registerGetter<double>([level](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		return level.expired() ? 0.0 : static_cast<double>(level.lock()->getExplosions().size());
	});
	variableStore.add(std::move(exploscount));

	// explos
	GameVariable explos{.name = "explos"};
	explos.registerGetter<std::vector<ScriptObject>>([level](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		auto levelPtr = level.lock();
		if (levelPtr == nullptr || index.value_or(0) < 0)
			return std::vector<ScriptObject>{};

		if (!index.has_value())
		{
			std::vector<ScriptObject> objectList{};
			for (size_t i = 0; levelPtr && i < levelPtr->getExplosions().size(); ++i)
				objectList.emplace_back(std::make_pair(i, ScriptObjectType::EXPLOSION));
			return objectList;
		}

		return std::vector<ScriptObject>{std::make_pair(index.value(), ScriptObjectType::EXPLOSION)};
	});
	variableStore.add(std::move(explos));

	// horsescount
	GameVariable horsescount{.name = "horsescount"};
	horsescount.registerGetter<double>([level](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		return level.expired() ? 0.0 : static_cast<double>(level.lock()->getHorses().size());
	});
	variableStore.add(std::move(horsescount));

	// horses
	GameVariable horses{.name = "horses"};
	horses.registerGetter<std::vector<ScriptObject>>([level](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		auto levelPtr = level.lock();
		if (levelPtr == nullptr || index.value_or(0) < 0)
			return std::vector<ScriptObject>{};

		if (!index.has_value())
		{
			std::vector<ScriptObject> objectList{};
			for (size_t i = 0; levelPtr && i < levelPtr->getHorses().size(); ++i)
				objectList.emplace_back(std::make_pair(i, ScriptObjectType::HORSE));
			return objectList;
		}

		return std::vector<ScriptObject>{std::make_pair(index.value(), ScriptObjectType::HORSE)};
	});
	variableStore.add(std::move(horses));

	// signscount
	GameVariable signscount{.name = "signscount"};
	signscount.registerGetter<double>([level](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		return level.expired() ? 0.0 : static_cast<double>(level.lock()->getSignCount());
	});
	variableStore.add(std::move(signscount));

	// signs
	GameVariable signs{.name = "signs"};
	signs.registerGetter<std::vector<ScriptObject>>([level](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		auto levelPtr = level.lock();
		if (levelPtr == nullptr || index.value_or(0) < 0)
			return std::vector<ScriptObject>{};

		if (!index.has_value())
		{
			std::vector<ScriptObject> objectList{};
			for (size_t i = 0; levelPtr && i < levelPtr->getSignCount(); ++i)
				objectList.emplace_back(std::make_pair(i, ScriptObjectType::SIGN));
			return objectList;
		}

		return std::vector<ScriptObject>{std::make_pair(index.value(), ScriptObjectType::SIGN)};
	});
	variableStore.add(std::move(signs));

	// board[]
	// Needs the map position of the NPC, so moved there.

	// tiles[x,y] -> tiles[]
	GameVariable tiles{.name = "tiles"};
	tiles.registerGetter<double>([level](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		auto levelPtr = level.lock();
		if (levelPtr == nullptr || index.value_or(-1) < 0)
			return 0.0;

		// Get the tile X/Y out of the index.
		uint32_t tileX = static_cast<uint32_t>(index.value() >> 32);
		uint32_t tileY = static_cast<uint32_t>(index.value() & 0xFFFFFFFF);
		TilePosition tilePos{static_cast<float>(tileX), static_cast<float>(tileY)};

		// Get the tile.
		if (auto tile = levelPtr->getMapTileForEditing(tilePos); tile != nullptr)
			return static_cast<double>(*tile);
		return 0.0;
	});
	tiles.registerSetter<double>([level](GameValueVariantForSetter& value, std::optional<int64_t> index)
	{
		auto levelPtr = level.lock();
		if (levelPtr == nullptr || index.value_or(-1) < 0) return;

		// Get the tile X/Y out of the index.
		uint32_t tileX = static_cast<uint32_t>(index.value() >> 32);
		uint32_t tileY = static_cast<uint32_t>(index.value() & 0xFFFFFFFF);
		TilePosition tilePos{static_cast<float>(tileX), static_cast<float>(tileY)};

		// Get and update the tile.
		if (auto wrap = std::get_if<std::reference_wrapper<double>>(&value); wrap != nullptr)
		{
			if (auto tile = levelPtr->getMapTileForEditing(tilePos); tile != nullptr)
				*tile = static_cast<uint16_t>(wrap->get());
		}
	});
	variableStore.add(std::move(tiles));
}

void setOtherVariables(GameVariableStore& variableStore, ScriptEvent& event)
{
	// paramscount
	GameVariable paramscount{.name = "paramscount"};
	paramscount.registerGetter<double>([&event](std::optional<int64_t> index) -> GameValueVariantForGetter
	{
		return static_cast<double>(std::max<std::vector<std::any>::size_type>(1, event.args.size()) - 1);
	});
	variableStore.add(std::move(paramscount));
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1
