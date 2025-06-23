#include <algorithm>
#include <array>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <BabyDI.h>

#include <Account.h>
#include <Server.h>
#include <object/Player.h>
#include <player/PlayerProps.h>
#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/StringUtils.h>

////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

/// @brief A getter function for a property that gets its results from another getter function.
static GameVariable::func_get prop_get(ValidGameValueCallable auto getter)
{
	return [getter](std::string_view identifier) -> GameValue
	{
		return GameValue{ getter() };
	};
}

/// @brief A getter function for a property that gets its results from a property directly.
static GameVariable::func_get prop_get(auto& value)
{
	using V = std::remove_cvref_t<decltype(value)>;
	static_assert(std::integral<V> || std::floating_point<V> || string::StringVariant<V> || std::ranges::forward_range<V>,
		"Account prop_get called with an unsupported type. Supported types are integral, floats, string, or ranges.");

	// Number.
	if constexpr (std::integral<V> || std::floating_point<V>)
	{
		return [&value](std::string_view identifier) -> GameValue
		{
			return GameValue{ static_cast<double>(value) };
		};
	}
	// String.
	else if constexpr (string::StringVariant<V>)
	{
		return [&value](std::string_view identifier) -> GameValue
		{
			return GameValue{ std::string{ value } };
		};
	}
	// Array.
	else if constexpr (std::ranges::forward_range<V>)
	{
		return [&value](std::string_view identifier) -> GameValue
		{
			// Transform the range to a vector of doubles.
			return GameValue{ value | std::views::transform([](const auto& v) { return static_cast<double>(v); }) | std::ranges::to<std::vector<double>>() };
		};
	}

	throw std::invalid_argument("Account prop_get called with an unsupported type.");
}

/// @brief A setter function for a property that needs an additional setter function to write the values.
static GameVariable::func_set prop_set(Player* who, std::optional<PlayerProp> prop, std::function<void(const GameValue&, std::optional<size_t>)> setter)
{
	return [who, prop, setter](GameVariable& variable, const GameValue& value, std::optional<size_t> index)
	{
		// Call the setter function.
		setter(value, index);

		// Record the modification time for the property.
		if (prop.has_value() && who != nullptr)
			who->modTime[PROPID(prop.value())] = currentTime();
	};
}

/// @brief A setter function for a property that can directly set to a value.
static GameVariable::func_set prop_set(Player* who, std::optional<PlayerProp> prop, auto& propvalue)
{
	using V = std::remove_cvref_t<decltype(propvalue)>;
	static_assert(std::integral<V> || std::floating_point<V> || string::StringVariant<V> || std::ranges::random_access_range<V>,
		"Account prop_get called with an unsupported type. Supported types are integral, floats, string, or ranges.");

	// Number.
	if constexpr (std::integral<V> || std::floating_point<V>)
	{
		return [who, prop, &propvalue](GameVariable& variable, const GameValue& value, std::optional<size_t> index)
		{
			propvalue = static_cast<V>(value.get<double>().value_or(0.0));
			if (prop.has_value())
				who->modTime[PROPID(prop.value())] = currentTime();
		};
	}
	// String.
	else if constexpr (string::StringVariant<V>)
	{
		return [who, prop, &propvalue](GameVariable& variable, const GameValue& value, std::optional<size_t> index)
		{
			propvalue = value.get<std::string>().value_or({});
			if (prop.has_value())
				who->modTime[PROPID(prop.value())] = currentTime();
		};
	}
	// Array.
	else if constexpr (std::ranges::random_access_range<V>)
	{
		return [who, prop, &propvalue](GameVariable& variable, const GameValue& value, std::optional<size_t> index)
		{
			size_t propvalue_size = std::ranges::size(propvalue);
			if (propvalue_size > 0)
			{
				using value_type = std::remove_cvref_t<decltype(propvalue[0])>;

				// Setting an individual index in an array.
				if (index.has_value() && index.value() < propvalue_size)
				{
					propvalue[index.value()] = static_cast<value_type>(value.get<double>().value_or(0.0));
					if (prop.has_value())
						who->modTime[PROPID(prop.value()) + index.value()] = currentTime();
				}
				// Setting the whole array.
				else if (!index.has_value())
				{
					const std::vector<double> vec = value.get<std::vector<double>>().value();
					copyToArrayAs<value_type>(vec, propvalue);
					if (prop.has_value())
						who->modTime[PROPID(prop.value())] = currentTime();
				}
			}
		};
	}

	throw std::invalid_argument("Account prop_set called with an unsupported type.");
}

//----------------------------

void Account::bindVariablesToPlayer(std::shared_ptr<Player> player)
{
	// TODO(Nalin): I really hate this.  The whole variable assignment system needs to be reworked.
	if (player == nullptr)
		return;

	/* TODO(NPCServer): Add these properties to the player.
	saysnumber
	attachid
	attachtype
	*/

	auto playerPtr = player.get();

	// Create game variables.
	GameVariable id{ set_temporary, "id", prop_get([playerPtr]() { return static_cast<double>(playerPtr->getId()); }), {} };
	GameVariable rupees{ set_temporary, "rupees", prop_get(character.gralats), prop_set(playerPtr, PlayerProp::RUPEESCOUNT, character.gralats) };
	GameVariable gralats{ set_temporary, "gralats", prop_get(character.gralats), prop_set(playerPtr, PlayerProp::RUPEESCOUNT, character.gralats) };
	GameVariable bombs{ set_temporary, "bombs", prop_get(character.bombs), prop_set(playerPtr, PlayerProp::BOMBSCOUNT, character.bombs) };
	GameVariable darts{ set_temporary, "darts", prop_get(character.arrows), prop_set(playerPtr, PlayerProp::ARROWSCOUNT, character.arrows) };
	GameVariable glovepower{ set_temporary, "glovepower", prop_get(character.glovePower), prop_set(playerPtr, PlayerProp::GLOVEPOWER, character.glovePower) };
	GameVariable swordpower{ set_temporary, "swordpower", prop_get(character.swordPower), prop_set(playerPtr, PlayerProp::SWORDPOWER, character.swordPower) };
	GameVariable shieldpower{ set_temporary, "shieldpower", prop_get(character.shieldPower), prop_set(playerPtr, PlayerProp::SHIELDPOWER, character.shieldPower) };
	GameVariable mp{ set_temporary, "mp", prop_get(character.mp), prop_set(playerPtr, PlayerProp::MAGICPOINTS, character.ap) };
	GameVariable ap{ set_temporary, "ap", prop_get(character.ap), prop_set(playerPtr, PlayerProp::ALIGNMENT, character.ap) };
	GameVariable fullhearts{ set_temporary, "fullhearts", prop_get(this->maxHitpoints), prop_set(playerPtr, PlayerProp::MAXPOWER, this->maxHitpoints) };

	GameVariable hearts{ set_temporary, "hearts",
		prop_get([this, playerPtr]() { return character.hitpointsInHalves / 2.0; }),
		prop_set(playerPtr, PlayerProp::CURPOWER, [this, playerPtr](const GameValue& value, std::optional<size_t>) { character.hitpointsInHalves = value.get<double>().value_or(0.0) * 2; }) };
	GameVariable x{ set_temporary, "x",
		prop_get([this, playerPtr]() { return character.pixelX / 16.0; }),
		prop_set(playerPtr, PlayerProp::X2, [this, playerPtr](const GameValue& value, std::optional<size_t>) { character.pixelX = value.get<double>().value_or(0.0) * 16; }) };
	GameVariable y{ set_temporary, "y",
		prop_get([this, playerPtr]() { return character.pixelY / 16.0; }),
		prop_set(playerPtr, PlayerProp::Y2, [this, playerPtr](const GameValue& value, std::optional<size_t>) { character.pixelY = value.get<double>().value_or(0.0) * 16; }) };
	GameVariable z{ set_temporary, "z",
		prop_get([this, playerPtr]() { return character.pixelZ / 16.0; }),
		prop_set(playerPtr, PlayerProp::Z2, [this, playerPtr](const GameValue& value, std::optional<size_t>) { character.pixelZ = value.get<double>().value_or(0.0) * 16; }) };

	GameVariable headset{ set_temporary, "headset",
		prop_get(
			[this, playerPtr]()
			{
				int headSet = -1;
				if (character.headImage.starts_with("head"))
					string::toNumber(character.headImage.substr(4), headSet);
				return static_cast<double>(headSet);
			}),
		prop_set(playerPtr, PlayerProp::HEADGIF,
			[this, playerPtr](const GameValue& value, std::optional<size_t>)
			{
				auto headSet = std::clamp(static_cast<int>(value.get<double>().value_or(-1.0)), -1, 99);
				if (headSet < 0) return;
				character.headImage = std::format("head{}.{}", headSet, (BabyDI::Get<Server>()->Generation == ServerGeneration::ORIGINAL ? "gif" : "png"));
			})
	};
	GameVariable sprite{ set_temporary, "sprite",
		prop_get(character.sprite),
		prop_set(playerPtr, PlayerProp::SPRITE,
			[this, playerPtr](const GameValue& value, std::optional<size_t>)
			{
				character.sprite = static_cast<uint8_t>(value.get<double>().value_or(0.0));
				if (character.sprite >= 4 && BabyDI::Get<Server>()->Generation != ServerGeneration::ORIGINAL)
				{
					character.gani = std::format("def[{}]", character.sprite);
					playerPtr->modTime[PROPID(PlayerProp::GANI)] = currentTime();
				}
			})
	};
	GameVariable dir{ set_temporary, "dir",
		prop_get([this, playerPtr]() { return static_cast<double>(character.direction); }),
		prop_set(playerPtr, PlayerProp::SPRITE,
			[this, playerPtr](const GameValue& value, std::optional<size_t>)
			{
				character.direction = std::clamp(static_cast<uint8_t>(value.get<double>().value_or(0.0)), 0_ui8, 3_ui8);
			})
	};

	std::vector<GameVariable> playerPrefixed =
	{
		id, rupees, gralats, bombs, darts,
		glovepower, swordpower, shieldpower, mp, ap,
		hearts, fullhearts, x, y, z,
		headset, sprite, dir
	};

	// Create variable store links.
	variables.store.clear();
	variables.add(std::move(id));
	variables.add(std::move(rupees));
	variables.add(std::move(gralats));
	variables.add(std::move(bombs));
	variables.add(std::move(darts));
	variables.add(std::move(glovepower));
	variables.add(std::move(swordpower));
	variables.add(std::move(shieldpower));
	variables.add(std::move(mp));
	variables.add(std::move(ap));
	variables.add(std::move(hearts));
	variables.add(std::move(fullhearts));
	variables.add(std::move(x));
	variables.add(std::move(y));
	variables.add(std::move(z));
	variables.add(std::move(headset));
	variables.add(std::move(sprite));
	variables.add(std::move(dir));

	// TODO(NPCServer): This should be done a different way since it would make playerx affect the current source instead of the initiator.
	std::ranges::for_each(playerPrefixed, [this](GameVariable& var)
	{
		var.identifier = std::format("player{}", var.identifier);
		variables.add(std::move(var));
	});
	playerPrefixed.clear();
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
