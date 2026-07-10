#ifndef SCRIPTCONTAINERS_H
#define SCRIPTCONTAINERS_H

#include <algorithm>
#include <any>
#include <array>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <deque>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/StringUtils.h>
#include <utilities/generator/TimeoutGenerator.h>

////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

class Level;
class Weapon;
using LevelPtr = std::shared_ptr<Level>;
using WeaponPtr = std::shared_ptr<Weapon>;

////////////////////////////////////////////////////////////
// GameValue
////////////////////////////////////////////////////////////

// clang-format off

/// @brief Concept to ensure the value passed to `GameValue` is a valid type.
template<class T>
concept StoresInGameValue = std::same_as<std::remove_cvref_t<T>, bool>
	|| std::same_as<std::remove_cvref_t<T>, double>
	|| std::same_as<std::remove_cvref_t<T>, std::string>
	|| std::same_as<std::remove_cvref_t<T>, ScriptObject>
	|| std::same_as<std::remove_cvref_t<T>, std::vector<double>>
	|| std::same_as<std::remove_cvref_t<T>, std::vector<ScriptObject>>;

// clang-format on

/// @brief A container that can hold one or more valid game value types.
///
/// This is used to represent a single value that can be one or many of these types.
/// It provides methods to set and retrieve the value in a type-safe manner.
struct GameValue
{
public:
	GameValue() = default;
	GameValue(StoresInGameValue auto&& value)
	{
		insert(std::forward<decltype(value)>(value));
	}
	GameValue(std::integral auto const& value)
	{
		insert(static_cast<double>(value));
	}
	GameValue(std::string_view value)
	{
		insert(std::string{value});
	}

public:
	GameValue(const GameValue& other)
		: m_boolean(other.m_boolean), m_number(other.m_number), m_text(other.m_text), m_array(other.m_array)
	{}
	GameValue(GameValue&& other) noexcept
		: m_boolean(std::move(other.m_boolean)), m_number(std::move(other.m_number)), m_text(std::move(other.m_text)), m_array(std::move(other.m_array))
	{}

	GameValue& operator=(const GameValue& other) noexcept;
	GameValue& operator=(GameValue&& other) noexcept;
	bool operator==(const GameValue& other) noexcept;
	explicit operator bool() const;

public:
	/// @brief Retrieves the stored value of the specified type, if present.
	/// @tparam T The type of value to retrieve. Must satisfy the `StoresInGameValue` constraint.
	/// @return A 'std::optional{ T }' containing a reference to the stored value if it exists; otherwise, throws `std::bad_variant_access` if the type is not supported.
	template<StoresInGameValue T>
	[[inline]] std::optional<std::reference_wrapper<T>> get();

	/// @brief Retrieves the stored value of the specified type, if present.
	/// @tparam T The type of value to retrieve. Must satisfy the `StoresInGameValue` constraint.
	/// @return A 'std::optional{ T }' containing a reference to the stored value if it exists; otherwise, throws `std::bad_variant_access` if the type is not supported.
	template<StoresInGameValue T>
	[[inline]] std::optional<std::reference_wrapper<const T>> get() const;

	/// @brief Retrieves the stored value of the specified type, if possible.  Will convert between bool and double if necessary.
	/// @tparam T The type of value to retrieve. Must satisfy the `StoresInGameValue` constraint.
	/// @return A 'std::optional{ T }' containing the stored value if it exists; otherwise, throws `std::bad_variant_access` if the type is not supported.
	template<StoresInGameValue T>
	[[inline]] const std::optional<T> getCopy() const;

	/// @brief Sets the value of the GameValue object to the provided value, resetting any existing number, text, or array state.
	/// @param value The new value to assign to the GameValue object. Must satisfy the StoresInGameValue concept.
	/// @return A reference to the modified GameValue object.
	[[inline]] GameValue& set(StoresInGameValue auto&& value);

	/// @brief Assigns a value to the GameValue, overwriting the value of the passed type. Other types are not affected.
	/// @param value The value to assign to the GameValue. Must satisfy the StoresInGameValue concept.
	/// @return A reference to the modified GameValue object.
	[[inline]] GameValue& assign(StoresInGameValue auto&& value);

	/// @brief Assigns values from another GameValue object to this one, overwriting the values of the specified types.
	/// @tparam ...Types A list of types to assign from the other GameValue.
	/// @param other The other GameValue object from which to assign values.
	/// @return A reference to the modified GameValue object.
	template<StoresInGameValue... Types>
	[[inline]] GameValue& assign(const GameValue& other);

	/// @brief Unassigns a value type from the GameValue.
	/// @tparam Type The value type to unassign.
	/// @return A reference to the modified GameValue object.
	template<StoresInGameValue Type>
	[[inline]] GameValue& unassign();

	/// @brief If the variable is an array, flattens it into a single value.
	/// @return A GameValue object that contains the flattened value.
	GameValue flatten(int64_t index) const noexcept;

	/// @brief Tests the GameValue as a flag check.
	/// @return True if the GameValue has a boolean value or a non-empty string value, false otherwise.
	bool testAsFlag() const;

	/// @brief Checks if the GameValue has a value of the specified type.
	/// @tparam T The type to check for. Must satisfy the `ValidGameValue` constraint.
	/// @return True if the GameValue has a value of the specified type; otherwise, false.
	template<StoresInGameValue T>
	[[inline]] bool has() const;

	/// @brief Checks if the GameValue has multiple values.
	/// @return True if the GameValue has multiple values; otherwise, false.
	[[inline]] bool has_many() const;

protected:
	std::optional<bool> m_boolean;
	std::optional<double> m_number;
	std::optional<std::string> m_text;
	std::optional<std::variant<std::vector<double>, std::vector<ScriptObject>>> m_array;

	[[inline]] GameValue& insert(const StoresInGameValue auto& value);
	[[inline]] GameValue& insert(StoresInGameValue auto&& value);
};

//----------------------------

template<StoresInGameValue T>
inline std::optional<std::reference_wrapper<T>> GameValue::get()
{
	std::optional<std::reference_wrapper<T>> result = std::nullopt;
	if constexpr (std::same_as<T, bool>)
	{
		if (m_boolean.has_value())
			result = std::ref(*m_boolean);
	}
	else if constexpr (std::same_as<T, double>)
	{
		if (m_number.has_value())
			result = std::ref(*m_number);
	}
	else if constexpr (std::same_as<T, std::string>)
	{
		if (m_text.has_value())
			result = std::ref(*m_text);
	}
	else if constexpr (std::same_as<T, ScriptObject>)
	{
		if (!m_array.has_value() || !std::holds_alternative<std::vector<ScriptObject>>(*m_array))
			return std::nullopt;

		const auto& arr = std::get<std::vector<ScriptObject>>(*m_array);
		if (arr.size() == 0)
			return std::nullopt;

		result = std::ref(arr.at(0));
	}
	else if constexpr (std::same_as<T, std::vector<double>>)
	{
		if (!m_array.has_value() || !std::holds_alternative<std::vector<double>>(*m_array))
			return std::nullopt;

		auto& arr = std::get<std::vector<double>>(*m_array);
		result = std::ref(arr);
	}
	else if constexpr (std::same_as<T, std::vector<ScriptObject>>)
	{
		if (!m_array.has_value() || !std::holds_alternative<std::vector<ScriptObject>>(*m_array))
			return std::nullopt;

		const auto& arr = std::get<std::vector<ScriptObject>>(*m_array);
		result = std::ref(arr);
	}
	else static_assert(false, "Invalid type for GameValue::get.");

	return result;
}

template<StoresInGameValue T>
inline std::optional<std::reference_wrapper<const T>> GameValue::get() const
{
	std::optional<std::reference_wrapper<const T>> result = std::nullopt;
	if constexpr (std::same_as<T, bool>)
	{
		if (m_boolean.has_value())
			result = std::ref(*m_boolean);
	}
	else if constexpr (std::same_as<T, double>)
	{
		if (m_number.has_value())
			result = std::ref(*m_number);
	}
	else if constexpr (std::same_as<T, std::string>)
	{
		if (m_text.has_value())
			result = std::ref(*m_text);
	}
	else if constexpr (std::same_as<T, ScriptObject>)
	{
		if (!m_array.has_value() || !std::holds_alternative<std::vector<ScriptObject>>(*m_array))
			return std::nullopt;

		const auto& arr = std::get<std::vector<ScriptObject>>(*m_array);
		if (arr.size() == 0)
			return std::nullopt;

		result = std::ref(arr.at(0));
	}
	else if constexpr (std::same_as<T, std::vector<double>>)
	{
		if (!m_array.has_value() || !std::holds_alternative<std::vector<double>>(*m_array))
			return std::nullopt;

		const auto& arr = std::get<std::vector<double>>(*m_array);
		result = std::ref(arr);
	}
	else if constexpr (std::same_as<T, std::vector<ScriptObject>>)
	{
		if (!m_array.has_value() || !std::holds_alternative<std::vector<ScriptObject>>(*m_array))
			return std::nullopt;

		const auto& arr = std::get<std::vector<ScriptObject>>(*m_array);
		result = std::ref(arr);
	}
	else static_assert(false, "Invalid type for GameValue::get.");

	return result;
}

template<StoresInGameValue T>
inline const std::optional<T> GameValue::getCopy() const
{
	if constexpr (std::same_as<T, bool>)
	{
		if (m_boolean.has_value())
			return m_boolean.value();
		if (m_text.has_value())
			return !m_text.value().empty();
		return std::nullopt;
	}
	else if constexpr (std::same_as<T, double>)
	{
		if (m_number.has_value())
			return m_number.value();
		if (m_boolean.has_value())
			return m_boolean.value() ? 1.0 : 0.0;
		return std::nullopt;
	}
	else if constexpr (std::same_as<T, std::string>)
	{
		return m_text;
	}
	else if constexpr (std::same_as<T, ScriptObject>)
	{
		if (!m_array.has_value() || !std::holds_alternative<std::vector<ScriptObject>>(*m_array))
			return std::nullopt;

		const auto& arr = std::get<std::vector<ScriptObject>>(*m_array);
		if (arr.size() == 0)
			return std::nullopt;

		return arr.at(0);
	}
	else if constexpr (std::same_as<T, std::vector<double>>)
	{
		if (!m_array.has_value() || !std::holds_alternative<std::vector<double>>(*m_array))
			return std::nullopt;

		const auto& arr = std::get<std::vector<double>>(*m_array);
		return arr;
	}
	else if constexpr (std::same_as<T, std::vector<ScriptObject>>)
	{
		if (!m_array.has_value() || !std::holds_alternative<std::vector<ScriptObject>>(*m_array))
			return std::nullopt;

		const auto& arr = std::get<std::vector<ScriptObject>>(*m_array);
		return arr;
	}
	else static_assert(false, "Invalid type for GameValue::get.");
	throw std::bad_variant_access();
}

inline GameValue& GameValue::set(StoresInGameValue auto&& value)
{
	m_boolean = std::nullopt;
	m_number = std::nullopt;
	m_text = std::nullopt;
	m_array = std::nullopt;
	return insert(std::forward<decltype(value)>(value));
}

inline GameValue& GameValue::assign(StoresInGameValue auto&& value)
{
	return insert(std::forward<decltype(value)>(value));
}

template<StoresInGameValue... Types>
inline GameValue& GameValue::assign(const GameValue& other)
{
	(assign(other.getCopy<Types>().value_or(Types{})), ...);
	return *this;
}

template<StoresInGameValue Type>
inline GameValue& GameValue::unassign()
{
	if constexpr (std::same_as<Type, bool>)
		m_boolean = std::nullopt;
	if constexpr (std::same_as<Type, double>)
		m_number = std::nullopt;
	if constexpr (std::same_as<Type, std::string>)
		m_text = std::nullopt;
	if constexpr (std::same_as<Type, std::vector<double>> || std::same_as<Type, ScriptObject> || std::same_as<Type, std::vector<ScriptObject>>)
		m_array = std::nullopt;
	return *this;
}

inline GameValue& GameValue::insert(const StoresInGameValue auto& value)
{
	using V = std::remove_cvref_t<decltype(value)>;
	if constexpr (std::same_as<V, bool>)
	{
		m_boolean = value;
	}
	else if constexpr (std::same_as<V, double>)
	{
		m_number = value;
	}
	else if constexpr (std::same_as<V, std::string>)
	{
		if (value.empty())
			m_text = std::nullopt;
		else m_text = value;
	}
	else if constexpr (std::same_as<V, ScriptObject>)
	{
		if (m_array.has_value() && std::holds_alternative<std::vector<ScriptObject>>(m_array.value()))
		{
			auto& arr = std::get<std::vector<ScriptObject>>(m_array.value());
			arr.clear();
			arr.push_back(value);
		}
	}
	else if constexpr (std::same_as<V, std::vector<double>> || std::same_as<V, std::vector<ScriptObject>>)
	{
		m_array = value;
	}
	else throw std::bad_variant_access();

	return *this;
}

inline GameValue& GameValue::insert(StoresInGameValue auto&& value)
{
	using V = std::remove_cvref_t<decltype(value)>;
	if constexpr (std::same_as<V, bool>)
	{
		m_boolean = value;
	}
	else if constexpr (std::same_as<V, double>)
	{
		m_number = value;
	}
	else if constexpr (std::same_as<V, std::string>)
	{
		if (value.empty())
			m_text = std::nullopt;
		m_text = std::move(value);
	}
	else if constexpr (std::same_as<V, ScriptObject>)
	{
		if (m_array.has_value() && std::holds_alternative<std::vector<ScriptObject>>(m_array.value()))
		{
			auto& arr = std::get<std::vector<ScriptObject>>(m_array.value());
			arr.clear();
			arr.push_back(value);
		}
	}
	else if constexpr (std::same_as<V, std::vector<double>> || std::same_as<V, std::vector<ScriptObject>>)
	{
		m_array = std::move(value);
	}
	else throw std::bad_variant_access();

	return *this;
}

template<StoresInGameValue T>
inline bool GameValue::has() const
{
	if constexpr (std::same_as<T, bool>)
		return m_boolean.has_value();
	else if constexpr (std::same_as<T, double>)
		return m_number.has_value();
	else if constexpr (std::same_as<T, std::string>)
		return m_text.has_value();
	else if constexpr (std::same_as<T, std::vector<double>>)
		return m_array.has_value() && std::holds_alternative<std::vector<double>>(*m_array);
	else if constexpr (std::same_as<T, ScriptObject> || std::same_as<T, std::vector<ScriptObject>>)
		return m_array.has_value() && std::holds_alternative<std::vector<ScriptObject>>(*m_array);
	return false;
}

inline bool GameValue::has_many() const
{
	int count = 0;
	if (m_boolean.has_value()) ++count;
	if (m_number.has_value()) ++count;
	if (m_text.has_value()) ++count;
	if (m_array.has_value()) ++count;
	return count > 1;
}

////////////////////////////////////////////////////////////
// GameVariable
////////////////////////////////////////////////////////////

// clang-format off

/// @brief A variant of the types that can be stored in a GameValue, used by the getter/setter functions.
using GameValueVariantForGetter = std::variant<
	bool, double, std::string, ScriptObject, std::vector<double>, std::vector<ScriptObject>,
	std::reference_wrapper<bool>, std::reference_wrapper<double>, std::reference_wrapper<std::string>,
	std::reference_wrapper<ScriptObject>, std::reference_wrapper<std::vector<double>>, std::reference_wrapper<std::vector<ScriptObject>>
>;
using GameValueVariantForSetter = std::variant<
	std::reference_wrapper<bool>, std::reference_wrapper<double>, std::reference_wrapper<std::string>,
	std::reference_wrapper<ScriptObject>, std::reference_wrapper<std::vector<double>>, std::reference_wrapper<std::vector<ScriptObject>>
>;

// clang-format on

namespace variables
{

/// @brief Lifetime of a game variable, which determines how long the variable exists and when it is removed.
enum class Lifetime
{
	/// @brief Variable only exists for the duration of the current script execution.
	TEMPORARY,

	/// @brief Variable exists for the duration of the current game session, but is removed when the player leaves or the server restarts.
	NORMAL,

	/// @brief Variable exists permanently and is saved to disk. It persists across game sessions and server restarts.
	PERMANENT,
};

} // namespace variables

/// @brief A game variable with a name and a value.
struct GameVariable
{
	using func_get = std::function<GameValueVariantForGetter(std::optional<int64_t>)>;
	using func_set = std::function<void(GameValueVariantForSetter&, std::optional<int64_t>)>;

public:
	/// @brief The name of the game variable.
	std::string name;
	//std::optional<size_t> index;

	/// @brief The value of the game variable.
	GameValue value;

	/// @brief The lifetime of the game variable.
	std::optional<variables::Lifetime> lifetime;

public:
	/// @brief Getters for different data types.
	hash_map<func_get> getters;

	/// @brief Setters for different data types.
	hash_map<func_set> setters;

public:
	/// @brief Assigns a value to the game variable.
	/// @param value The value to assign.
	/// @param index The optional index for array elements.
	/// @return A reference to this game variable.
	[[inline]] GameVariable& assign(StoresInGameValue auto&& value, std::optional<int64_t> index = std::nullopt);

	/// @brief Sets a value to the game variable.
	/// @param value The value to set.
	/// @param index The optional index for array elements.
	/// @return A reference to this game variable.
	[[inline]] GameVariable& set(StoresInGameValue auto& value, std::optional<int64_t> index = std::nullopt);

public:
	/// @brief Gets a reference to the value of the game variable.
	/// @tparam T The type of the value to get.
	/// @param index The optional index for array elements.
	/// @return An optional reference to the value, or std::nullopt if the value does not exist.
	template<StoresInGameValue T>
	[[inline]] std::optional<std::reference_wrapper<T>> get(std::optional<int64_t> index = std::nullopt);

	/// @brief Gets a copy of the value of the game variable.
	/// @tparam T The type of the value to get.
	/// @param index The optional index for array elements.
	/// @return An optional copy of the value, or std::nullopt if the value does not exist.
	template<StoresInGameValue T>
	[[inline]] const std::optional<T> getCopy(std::optional<int64_t> index = std::nullopt) const;

public:
	/// @brief Checks if the game variable has a value of the specified type.
	/// @tparam T The type to check for. Must satisfy the `StoresInGameValue` constraint.
	/// @return True if the game variable has a value of the specified type; otherwise, false.
	template<StoresInGameValue T>
	[[inline]] bool has() const;

public:
	/// @brief Registers a getter function for the game variable.
	/// @tparam T The type of the value to get.
	/// @param getter The getter function to register.
	template<StoresInGameValue T>
	[[inline]] void registerGetter(func_get getter);

	/// @brief Registers a setter function for the game variable.
	/// @tparam T The type of the value to set.
	/// @param setter The setter function to register.
	template<StoresInGameValue T>
	[[inline]] void registerSetter(func_set setter);

public:
	/// @brief Deserializes a variable.
	/// @tparam T The data type of the variable.
	/// @param identifier The identifier name of the variable.
	/// @param data The data to deserialize.
	/// @return A reference to this.
	template<StoresInGameValue T = std::string>
	static GameVariable deserialize(std::string identifier, const std::string_view data);

	/// @brief Deserializes a variable.
	/// @param line The data to deserialize (should include the full data line, e.g.: VAR identifier=1,2,3).
	/// @return A reference to this.
	static std::optional<GameVariable> deserialize(const std::string_view line);

public:
	/// @brief Serializes a variable for distribution.
	/// @param name The name of the game variable to serialize.
	/// @return An optional string that contains the serialized variable.
	std::optional<std::string> serializeModern(std::string_view name) const noexcept;

	/// @brief Serializes the variable for saving.
	/// @return A serialized string for writing to disk.
	template<StoresInGameValue T = std::string>
	[[inline]] std::string serialize() const;
};

//----------------------------

inline GameVariable& GameVariable::assign(StoresInGameValue auto&& value, std::optional<int64_t> index)
{
	using V = std::remove_cvref_t<decltype(value)>;

	if (auto funcIt = setters.find(typeid(V).hash_code()); funcIt != setters.end())
	{
		GameValueVariantForSetter variantValue = std::ref(value);
		funcIt->second(variantValue, index);
		return *this;
	}

	if (!index.has_value())
		this->value.assign(std::forward<decltype(value)>(value));
	else
	{
		if constexpr (std::same_as<V, double> || std::same_as<V, ScriptObject>)
		{
			auto wrap = get<V>(index.value());
			if (wrap.has_value())
				wrap.value().get() = std::forward<decltype(value)>(value);
		}
		else throw std::bad_variant_access();
	}

	return *this;
}

inline GameVariable& GameVariable::set(StoresInGameValue auto& value, std::optional<int64_t> index)
{
	using V = std::remove_cvref_t<decltype(value)>;

	if (auto funcIt = setters.find(typeid(V).hash_code()); funcIt != setters.end())
	{
		GameValueVariantForSetter variantValue = std::ref(value);
		funcIt->second(variantValue, index);
		return *this;
	}

	if (!index.has_value())
		this->value.set(value);
	else
	{
		if constexpr (std::same_as<V, double> || std::same_as<V, ScriptObject>)
		{
			auto wrap = get<V>(index.value());
			if (wrap.has_value())
				wrap.value().get() = value;
		}
		else throw std::bad_variant_access();
	}

	return *this;
}

template<StoresInGameValue T>
inline std::optional<std::reference_wrapper<T>> GameVariable::get(std::optional<int64_t> index)
{
	if (auto funcIt = getters.find(typeid(T).hash_code()); funcIt != getters.end())
	{
		std::optional<std::reference_wrapper<T>> result;
		auto variantValue = funcIt->second(index);
		if (std::holds_alternative<std::reference_wrapper<T>>(variantValue))
		{
			result = std::get<std::reference_wrapper<T>>(variantValue);
		}
		else if (std::holds_alternative<T>(variantValue))
		{
			value.set(std::get<T>(variantValue));
			result = value.get<T>();
		}
		else
		{
			throw std::bad_variant_access();
		}
		return result;
	}

	if (!index.has_value())
		return value.get<T>();
	else
	{
		if constexpr (std::same_as<T, double> || std::same_as<T, ScriptObject>)
		{
			auto wrap = this->value.get<std::vector<T>>();
			if (!wrap.has_value())
				return std::nullopt;
			auto& arr = wrap.value().get();
			if (arr.size() == 0)
				return std::nullopt;

			if (index.value() < 0 || index.value() >= std::ssize(arr))
				index = 0;

			return std::ref(arr[index.value()]);
		}
		else throw std::bad_variant_access();
	}
	return std::nullopt;
}

template<StoresInGameValue T>
inline const std::optional<T> GameVariable::getCopy(std::optional<int64_t> index) const
{
	if (auto funcIt = getters.find(typeid(T).hash_code()); funcIt != getters.end())
	{
		std::optional<T> result;
		auto variantValue = funcIt->second(index);
		if (std::holds_alternative<std::reference_wrapper<T>>(variantValue))
		{
			result = std::get<std::reference_wrapper<T>>(variantValue).get();
		}
		else if (std::holds_alternative<T>(variantValue))
		{
			result = std::get<T>(variantValue);
		}
		else
		{
			throw std::bad_variant_access();
		}
		return result;
	}

	if (!index.has_value())
		return value.getCopy<T>();
	else
	{
		if constexpr (std::same_as<T, double> || std::same_as<T, ScriptObject>)
		{
			auto wrap = this->value.get<std::vector<T>>();
			if (!wrap.has_value())
				return std::nullopt;
			auto& arr = wrap.value().get();
			if (arr.size() == 0)
				return std::nullopt;

			if (index.value() < 0 || index.value() >= std::ssize(arr))
				index = 0;

			return arr[index.value()];
		}
		else throw std::bad_variant_access();
	}
	return std::nullopt;
}

template<StoresInGameValue T>
inline bool GameVariable::has() const
{
	if (getters.find(typeid(T).hash_code()) != getters.end())
		return true;
	return value.has<T>();
}

template<StoresInGameValue T>
inline void GameVariable::registerGetter(func_get getter)
{
	getters[typeid(T).hash_code()] = std::move(getter);
}

template<StoresInGameValue T>
inline void GameVariable::registerSetter(func_set setter)
{
	setters[typeid(T).hash_code()] = std::move(setter);
}

//----------------------------

template<StoresInGameValue T>
GameVariable GameVariable::deserialize(std::string identifier, const std::string_view data)
{
	if constexpr (std::same_as<T, bool>)
		return GameVariable{.name = identifier, .value = true};
	if constexpr (std::same_as<T, double>)
		return GameVariable{.name = identifier, .value = string::toDouble(data)};
	if constexpr (std::same_as<T, std::string>)
		return GameVariable{.name = identifier, .value = std::string{data}};
	if constexpr (std::same_as<T, std::vector<double>>)
	{
		std::vector<double> array;
		for (auto number : string::split(data, ","sv))
			array.emplace_back(string::toDouble(number));
		return GameVariable{.name = identifier, .value = std::move(array)};
	}
	return GameVariable{};
}

template<StoresInGameValue T>
inline std::string GameVariable::serialize() const
{
	if constexpr (std::same_as<T, bool>)
		return {};
	if constexpr (std::same_as<T, double>)
		return std::format("{}", value.getCopy<double>().value_or(0.0));
	if constexpr (std::same_as<T, std::string>)
		return value.getCopy<std::string>().value_or(std::string{});
	if constexpr (std::same_as<T, std::vector<double>>)
	{
		std::string array;
		auto value_array = value.get<std::vector<double>>();
		if (value_array.has_value())
		{
			for (size_t i = 0; i < value_array.value().get().size(); ++i)
			{
				array += std::format("{}", (value_array.value().get())[i]);
				if (i != value_array.value().get().size() - 1)
					array += ",";
			}
		}
		return array;
	}
	return {};
}

////////////////////////////////////////////////////////////
// GameVariableStore
////////////////////////////////////////////////////////////

/// @brief Maintains a collection of game variables.
class GameVariableStore
{
public:
	virtual ~GameVariableStore() {}

public:
	/// @brief Adds a new game variable with the specified name and value.
	/// @param name The name of the game variable to add.
	/// @param value The value to assign to the new game variable (moved).
	/// @return A weak pointer to the newly added GameVariable.
	virtual std::weak_ptr<GameVariable> add(std::string_view name, GameValue&& value) noexcept;

	/// @brief Adds a new game variable.
	/// @param variable The variable to add to the store (moved).
	/// @return A weak pointer to the newly added GameVariable.
	virtual std::weak_ptr<GameVariable> add(GameVariable&& variable) noexcept;

	/// @brief Adds a new game variable with the specified name, getter, and setter functions.
	/// @param name The name of the game variable to add.
	/// @param getter A function that retrieves the value of the game variable.
	/// @param setter A function that sets the value of the game variable.
	/// @return A weak pointer to the newly added GameVariable.
	template<StoresInGameValue T>
	[[inline]] std::weak_ptr<GameVariable> add(std::string_view name, GameVariable::func_get getter, GameVariable::func_set setter) noexcept;

	/// @brief Removes an item identified by the given name.
	/// @param name The name of the item to remove.
	/// @return true if the item was successfully removed; false otherwise.
	virtual bool remove(std::string_view name) noexcept;

	/// @brief Checks if the specified name is contained within the object.
	/// @param name The name to search for.
	/// @return true if the name is found; otherwise, false.
	virtual bool contains(std::string_view name) const noexcept;

	/// @brief Retrieves a weak pointer to a game variable by its name.
	/// @param name The name of the game variable to retrieve.
	/// @return A std::weak_ptr to the GameVariable associated with the given name. If the variable does not exist, the returned weak pointer will be empty.
	virtual std::weak_ptr<GameVariable> get(std::string_view name) noexcept;

	/// @brief Retrieves a weak pointer to a game variable by its name.
	/// @param name The name of the game variable to retrieve.
	/// @return A weak pointer to the requested GameVariable, or an empty weak pointer if not found.
	virtual const std::weak_ptr<GameVariable> get(std::string_view name) const noexcept;

	/// @brief Retrieves a game variable by name, or adds it if it does not exist.
	/// @param name The name of the game variable to retrieve or add.
	/// @return A weak pointer to the retrieved or newly added GameVariable.
	virtual std::weak_ptr<GameVariable> getOrAdd(std::string_view name) noexcept;

	/// @brief Retrieves the value of a game variable by name, if it exists.
	/// @tparam T The type of the value to retrieve. Must satisfy the `StoresInGameValue` constraint.
	/// @param name The name of the game variable to retrieve the value for.
	/// @return A std::optional containing the value of the game variable if it exists and is of the specified type; otherwise, an empty std::optional.
	template<StoresInGameValue T>
	[[inline]] std::optional<T> getValue(std::string_view name) const noexcept;

	/// @brief Clears all temporary variables from the store.
	virtual void clearTemporary() noexcept;

	/// @brief Clears temporary variables with a specific prefix from the store.
	/// @param prefix The prefix to match for temporary variables to clear.
	virtual void clearTemporary(std::string_view prefix) noexcept;

	/// @brief Serializes a variable for distribution.
	/// @param name The name of the game variable to serialize.
	/// @return An optional string that contains the serialized variable.
	virtual std::optional<std::string> serializeModern(std::string_view name) const noexcept;

	/// @brief Fully serializes a variable for writing to the disk.
	/// @param name The name of a game variable to serialize.
	/// @return A list of serialized data.
	virtual std::vector<std::string> serialize(std::string_view name) const noexcept;

public:
	/// @brief Marks the container as not accepting any new variables, nor deleting existing ones.
	bool staticContainer = false;

	/// @brief The default lifetime for new variables added to the store.
	variables::Lifetime defaultLifetime = variables::Lifetime::NORMAL;

	/// @brief The variable store map.
	string_map<std::shared_ptr<GameVariable>> store;
};

//----------------------------

template<StoresInGameValue T>
inline std::weak_ptr<GameVariable> GameVariableStore::add(std::string_view name, GameVariable::func_get getter, GameVariable::func_set setter) noexcept
{
	GameVariable variable{.name{name}};
	if (getter)
		variable.getters[typeid(T).hash_code()] = std::move(getter);
	if (setter)
		variable.setters[typeid(T).hash_code()] = std::move(setter);
	return add(std::move(variable));
}

template<StoresInGameValue T>
inline std::optional<T> GameVariableStore::getValue(std::string_view name) const noexcept
{
	if (auto value = get(name).lock(); value != nullptr)
		return value->getCopy<T>();
	return std::nullopt;
}

////////////////////////////////////////////////////////////
// ScriptEventQueue
////////////////////////////////////////////////////////////

/// @brief Queued script events.
class ScriptEventQueue
{
public:
	/// @brief Gets the underlying queue of script events.
	/// @return The queue of script events.
	[[inline]] std::deque<ScriptEvent>& queue();

public:
	/// @brief Determines whether a specific event exists for a given initiator.
	/// @param type The type of script event to check for.
	/// @param initiator Who initiated the event.
	/// @return True if the event exists for the given initiator; otherwise, false.
	bool hasEvent(ScriptEventType type, ScriptObject initiator);

	/// @brief Adds an event to the queue with the specified type and initiator.
	/// @param type The type of the script event to add.
	/// @param initiator Who initiated the event.
	void addEvent(ScriptEventType type, ScriptObject initiator);

	/// @brief Adds an event to the queue with the specified type, initiator, and additional arguments.
	/// @param type The type of the script event to add.
	/// @param initiator Who initiated the event.
	/// @param ...args A list of additional arguments to be passed with the event.
	[[inline]] void addEvent(ScriptEventType type, ScriptObject initiator, string::NotInputRangeNotString auto&&... args);

	/// @brief Adds an event to the queue with the specified type, initiator, and additional arguments.
	/// @param type The type of the script event to add.
	/// @param initiator Who initiated the event.
	/// @param range A list of additional arguments to be passed with the event.
	[[inline]] void addEvent(ScriptEventType type, ScriptObject initiator, string::InputRangeNotString auto&& range);

private:
	void addEvent(const ScriptEvent& event);
	void addEvent(ScriptEvent&& event);

private:
	std::deque<ScriptEvent> m_eventQueue;
};

//----------------------------

inline std::deque<ScriptEvent>& ScriptEventQueue::queue()
{
	return m_eventQueue;
}

inline void ScriptEventQueue::addEvent(ScriptEventType type, ScriptObject initiator, string::NotInputRangeNotString auto&&... args)
{
	ScriptEvent event{.type = type, .initiator = initiator, .args = {std::forward<decltype(args)>(args)...}};
	addEvent(std::move(event));
}

inline void ScriptEventQueue::addEvent(ScriptEventType type, ScriptObject initiator, string::InputRangeNotString auto&& range)
{
	static_assert(!string::PointerToConstCharString<decltype(range)>, "Don't use a const char* in the ranged variant of ScriptEventQueue::addEvent, pass in a std::string_view instead.");

	ScriptEvent event{.type = type, .initiator = initiator};
	auto transformed = range | std::views::transform([](const auto& arg) -> std::any
	{
		if constexpr (std::same_as<std::remove_cvref_t<decltype(arg)>, std::string_view>)
			return std::any{std::string{arg}};
		else
			return std::any{arg};
	});
	event.args.insert(event.args.end(), std::ranges::begin(transformed), std::ranges::end(transformed));
	addEvent(std::move(event));
}

////////////////////////////////////////////////////////////
// ScriptContainer
////////////////////////////////////////////////////////////

/// @brief A container that holds a script event queue and a game variable store.
struct ScriptContainer
{
	ScriptEventQueue events;
	GameVariableStore variables;
};

////////////////////////////////////////////////////////////
// ScriptParameters
////////////////////////////////////////////////////////////

/// @brief Concept that checks if a type has script parameters.
template<class T>
concept HasScriptParameters = requires(T t) {
	{ T::scriptParameters } -> std::convertible_to<string_map<GameVariable>>;
};

/// @brief Concept that checks if a type has constructible script parameters.
template<class T>
concept HasConstructibleScriptParameters = requires(T t) {
	{ T::scriptParameters } -> std::convertible_to<string_map<GameVariable>>;
	{ t.constructScriptParameters() } -> std::same_as<void>;
};

/// @brief Gets a script parameter from a source object that has script parameters.
/// @tparam T The type of the source object, which must satisfy the HasScriptParameters concept.
/// @param source The source object from which to get the script parameter.
/// @param name The name of the script parameter to get.
/// @return A pointer to the script parameter, or nullptr if it does not exist.
template<HasScriptParameters T>
inline GameVariable* getScriptParameter(T& source, std::string_view name)
{
	if constexpr (HasConstructibleScriptParameters<T>)
	{
		if (source.scriptParameters.empty())
			source.constructScriptParameters();
	}

	auto it = source.scriptParameters.find(name);
	if (it == source.scriptParameters.end())
		return nullptr;

	// Is there no better way than this?
	return const_cast<GameVariable*>(&it->second);
}

////////////////////////////////////////////////////////////
// Helper functions
////////////////////////////////////////////////////////////

namespace helpers
{

/// @brief Gets the server's current frame start time.
/// @return The current frame start time.
clock::time_point currentFrameTime();

/// @brief Wraps a std::reference_wrapper into a GameVariable.
/// @tparam T The type stored in the std::reference_wrapper.
/// @param reference The reference to wrap into a GameVariable.
/// @return The new GameVariable.
template<StoresInGameValue T>
GameVariable wrapReferenceIntoGameVariable(std::reference_wrapper<T> reference)
{
	GameVariable wrap{};
	wrap.registerGetter<double>([value = reference](std::optional<int64_t>) -> GameValueVariantForGetter
	{
		return value;
	});
	wrap.registerSetter<double>([value = reference](GameValueVariantForSetter& incoming, std::optional<int64_t>)
	{
		if (std::holds_alternative<std::reference_wrapper<double>>(incoming))
		{
			auto ref = std::get<std::reference_wrapper<double>>(incoming);
			value.get() = ref.get();
		}
	});
	return wrap;
}

} // end namespace helpers

////////////////////////////////////////////////////////////
// Ranges
////////////////////////////////////////////////////////////

/// @brief Provides views that manipulate variables.
namespace variables
{

/// @brief A view that filters out temporary variables.
inline constexpr auto temporary = std::views::filter([](const decltype(GameVariableStore::store)::value_type& pair) -> bool
{
	return pair.second->lifetime == Lifetime::TEMPORARY;
});

/// @brief A view that filters out variables that cannot be serialized (i.e., those that are not permanent or have an empty name).
inline constexpr auto serializable = std::views::filter([](const decltype(GameVariableStore::store)::value_type& pair) -> bool
{
	return pair.second->lifetime == Lifetime::PERMANENT && !pair.second->name.empty();
});

/// @brief Only gets variables that identify as flags.
inline constexpr auto only_flags = std::views::filter([](const decltype(GameVariableStore::store)::value_type& pair) -> bool
{
	return pair.second->value.testAsFlag();
});

} // namespace variables

////////////////////////////////////////////////////////////
// Prop helpers
////////////////////////////////////////////////////////////

namespace bind
{

/// @brief A concept that checks if a type is a property binder.
template<typename T>
concept IsPropertyBinder = requires(T t) {
	requires StoresInGameValue<typename T::DataType>;
	{ t.name } -> string::StringViewIshVariant;
	{ t.get() } -> std::same_as<GameVariable::func_get>;
	{ t.set() } -> std::same_as<GameVariable::func_set>;
};

/// @brief Binds a string property to a game variable.
template<typename T>
	requires std::same_as<std::remove_cvref_t<T>, std::string>
struct StringProperty
{
	using DataType = std::string;
	std::string_view name;
	std::optional<std::reference_wrapper<std::optional<clock::time_point>>> modTime;
	std::reference_wrapper<T> value;

	GameVariable::func_get get() const
	{
		return [value = value](std::optional<int64_t> index) -> GameValueVariantForGetter
		{
			return value.get();
		};
	}

	GameVariable::func_set set() const
	{
		if constexpr (std::is_const_v<T>)
		{
			return [](GameValueVariantForSetter& incoming, std::optional<int64_t> index)
			{
				// Do nothing, as the value is const and cannot be modified.
			};
		}
		else
		{
			return [modTime = modTime, value = value](GameValueVariantForSetter& incoming, std::optional<int64_t> index)
			{
				if (auto val = std::get_if<std::reference_wrapper<std::string>>(&incoming); val != nullptr)
				{
					value.get() = val->get();
					if (modTime.has_value())
						modTime.value().get() = helpers::currentFrameTime();
				}
			};
		}
	}
};

/// @brief Binds an integral property to a game variable.
template<typename T>
	requires std::integral<T> || std::is_enum_v<T>
struct IntegralProperty
{
	using DataType = double;
	std::string_view name;
	std::optional<std::reference_wrapper<std::optional<clock::time_point>>> modTime;
	std::reference_wrapper<T> value;

	GameVariable::func_get get() const
	{
		return [value = value](std::optional<int64_t> index) -> GameValueVariantForGetter
		{
			return static_cast<double>(value.get());
		};
	}

	GameVariable::func_set set() const
	{
		if constexpr (std::is_const_v<T>)
		{
			return [name = name](GameValueVariantForSetter& incoming, std::optional<int64_t> index)
			{
				// Do nothing, as the value is const and cannot be modified.
				incoming = incoming;
				index = std::nullopt;
			};
		}
		else
		{
			return [name = name, modTime = modTime, value = value](GameValueVariantForSetter& incoming, std::optional<int64_t> index)
			{
				if (auto val = std::get_if<std::reference_wrapper<double>>(&incoming); val != nullptr)
				{
					value.get() = static_cast<T>(val->get());
					if (modTime.has_value())
						modTime.value().get() = helpers::currentFrameTime();
				}
			};
		}
	}
};

/// @brief Binds an integral property that is transformed by a divisor to a game variable.
template<std::integral T>
struct DivideByIntegralProperty
{
	using DataType = double;
	std::string_view name;
	std::optional<std::reference_wrapper<std::optional<clock::time_point>>> modTime;
	std::reference_wrapper<T> value;
	double factor;

	GameVariable::func_get get() const
	{
		return [value = value, factor = factor](std::optional<int64_t> index) -> GameValueVariantForGetter
		{
			return static_cast<double>(value.get()) / factor;
		};
	}

	GameVariable::func_set set() const
	{
		if constexpr (std::is_const_v<T>)
		{
			return [](GameValueVariantForSetter& incoming, std::optional<int64_t> index)
			{
				// Do nothing, as the value is const and cannot be modified.
			};
		}
		else
		{
			return [modTime = modTime, value = value, factor = factor](GameValueVariantForSetter& incoming, std::optional<int64_t> index)
			{
				if (auto val = std::get_if<std::reference_wrapper<double>>(&incoming); val != nullptr)
				{
					value.get() = static_cast<T>(val->get() * factor);
					if (modTime.has_value())
						modTime.value().get() = helpers::currentFrameTime();
				}
			};
		}
	}
};

/// @brief Binds a timeout property to a game variable.
template<typename T>
	requires std::same_as<T, std::chrono::milliseconds> || std::same_as<T, TimeoutGenerator>
struct TimeoutProperty
{
	using DataType = double;
	std::string_view name;
	std::reference_wrapper<T> value;

	GameVariable::func_get get() const
	{
		if constexpr (std::same_as<T, std::chrono::milliseconds>)
		{
			return [value = value](std::optional<int64_t> index) -> GameValueVariantForGetter
			{
				return std::chrono::duration_cast<duration_seconds_double>(value.get()).count();
			};
		}
		else
		{
			return [value = value](std::optional<int64_t> index) -> GameValueVariantForGetter
			{
				return std::chrono::duration_cast<duration_seconds_double>(value.get().getRemainingTime()).count();
			};
		}
	}

	GameVariable::func_set set() const
	{
		if constexpr (std::same_as<T, std::chrono::milliseconds>)
		{
			return [value = value](GameValueVariantForSetter& incoming, std::optional<int64_t> index)
			{
				if (auto val = std::get_if<std::reference_wrapper<double>>(&incoming); val != nullptr)
				{
					value.get() = std::chrono::milliseconds(static_cast<int64_t>(val->get() * 1000.0));
				}
			};
		}
		else
		{
			return [value = value](GameValueVariantForSetter& incoming, std::optional<int64_t> index)
			{
				if (auto val = std::get_if<std::reference_wrapper<double>>(&incoming); val != nullptr)
				{
					value.get().startFor(duration_seconds_double(val->get()));
				}
			};
		}
	}
};

/// @brief Binds a save array property to a game variable.
template<std::integral T, size_t ArraySize, size_t ModTimeSize>
struct IntegralArrayProperty
{
	using DataType = double;
	std::string_view name;
	std::reference_wrapper<std::array<std::optional<clock::time_point>, ModTimeSize>> modTime;
	size_t modTimeIndex0 = 0;
	std::reference_wrapper<std::array<T, ArraySize>> value;

	GameVariable::func_get get() const
	{
		return [value = value](std::optional<int64_t> index) -> GameValueVariantForGetter
		{
			if (index.has_value() && inRange(index.value(), 0, static_cast<int64_t>(ArraySize)))
			{
				return static_cast<double>(value.get()[index.value()]);
			}
			else
			{
				return value.get()
					| std::views::transform([](const auto& v) { return static_cast<double>(v); })
					| std::ranges::to<std::vector<double>>();
			}
		};
	}

	GameVariable::func_set set() const
	{
		return [value = value, modTime = modTime, modTimeIndex0 = modTimeIndex0](GameValueVariantForSetter& incoming, std::optional<int64_t> index)
		{
			if (auto val = std::get_if<std::reference_wrapper<double>>(&incoming); val != nullptr)
			{
				if (index.has_value() && inRange(index.value(), 0, static_cast<int64_t>(ArraySize)))
				{
					value.get()[index.value()] = static_cast<uint8_t>(val->get());
					modTime.get()[modTimeIndex0 + index.value()] = helpers::currentFrameTime();
				}
			}
			else if (auto vec = std::get_if<std::reference_wrapper<std::vector<double>>>(&incoming); vec != nullptr)
			{
				size_t count = std::min(vec->get().size(), value.get().size());
				for (size_t i = 0; i < count; ++i)
				{
					value.get()[i] = static_cast<uint8_t>((vec->get())[i]);
					modTime.get()[modTimeIndex0 + i] = helpers::currentFrameTime();
				}
			}
		};
	}
};

/// @brief Manually defines a property with a name, getter, and setter for binding to a game variable.
/// @tparam ScriptDataType The data type of the property in the script.
template<StoresInGameValue ScriptDataType>
struct ManuallyDefinedProperty
{
	using DataType = ScriptDataType;
	std::string_view name;
	GameVariable::func_get getter;
	GameVariable::func_set setter;

	GameVariable::func_get get() const
	{
		return getter;
	}

	GameVariable::func_set set() const
	{
		return setter;
	}
};

/// @brief Binds a property as read-only to the script parameters.
/// @param scriptParameters The container of script parameters.
/// @param binder The property binder.
void bindPropertyAsReadOnly(MapContainer auto& scriptParameters, bind::IsPropertyBinder auto&& binder)
{
	using BinderType = std::remove_cvref_t<decltype(binder)>;
	GameVariable value{.name{binder.name}};
	value.registerGetter<typename BinderType::DataType>(binder.get());
	scriptParameters.try_emplace(std::string{binder.name}, std::move(value));
}

/// @brief Binds a property as read-write to the script parameters.
/// @param scriptParameters The container of script parameters.
/// @param binder The property binder.
void bindPropertyAsReadWrite(MapContainer auto& scriptParameters, bind::IsPropertyBinder auto&& binder)
{
	using BinderType = std::remove_cvref_t<decltype(binder)>;
	GameVariable value{.name{binder.name}};
	value.registerGetter<typename BinderType::DataType>(binder.get());

	if (auto setter = binder.set(); setter)
		value.registerSetter<typename BinderType::DataType>(std::move(setter));

	scriptParameters.try_emplace(std::string{binder.name}, std::move(value));
}

#define bindGETSIMPLE(func, ...) [__VA_ARGS__](std::optional<int64_t> index) -> GameValueVariantForGetter { return func; }

} // end namespace bind

////////////////////////////////////////////////////////////
// Exceptions
////////////////////////////////////////////////////////////

/// @brief Represents an error that occurs during script execution, such as a runtime error or an invalid operation.
struct script_error : public std::runtime_error
{
	using std::runtime_error::runtime_error;
};

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // SCRIPTCONTAINERS_H
