#ifndef SCRIPTCONTAINERS_H
#define SCRIPTCONTAINERS_H

#include <any>
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
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/StringUtils.h>

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

struct set_temporary_t { explicit set_temporary_t() = default; };
inline constexpr set_temporary_t set_temporary{};

/// @brief Concept to ensure the value passed to `GameValue` is a valid type.
template<class T>
concept ValidGameValue = std::same_as<std::remove_cvref_t<T>, bool>
	|| std::same_as<std::remove_cvref_t<T>, double>
	|| std::same_as<std::remove_cvref_t<T>, std::string>
	|| std::same_as<std::remove_cvref_t<T>, std::vector<double>>
	|| std::same_as<std::remove_cvref_t<T>, ScriptObject>
	|| std::same_as<std::remove_cvref_t<T>, std::vector<ScriptObject>>;

/// @brief A variant of the types that can be stored in a GameValue, used by the getter/setter functions.
using GameValueVariant = std::variant<std::optional<bool>*, std::optional<double>*, std::optional<std::string>*, std::optional<std::vector<double>>*, std::optional<std::vector<ScriptObject>>*>;

/// @brief A container that can hold one or more valid game value types.
///
/// This is used to represent a single value that can be one or many of these types.
/// It provides methods to set and retrieve the value in a type-safe manner.
struct GameValue
{
	using func_get = std::function<void(GameValueVariant, std::optional<int64_t>)>;
	using func_set = std::function<void(GameValueVariant, std::optional<int64_t>)>;

public:
	/// @brief Deserializes a variable.
	/// @tparam T The data type of the variable.
	/// @param identifier The identifier name of the variable.
	/// @param data The data to deserialize.
	/// @return A reference to this.
	template<ValidGameValue T = std::string>
	static GameValue deserialize(std::string identifier, const std::string_view data);

	/// @brief Deserializes a variable.
	/// @param line The data to deserialize (should include the full data line, e.g.: VAR identifier=1,2,3).
	/// @return A reference to this.
	static std::optional<GameValue> deserialize(const std::string_view line);

public:
	GameValue() = default;
	GameValue(ValidGameValue auto&& value)
	{
		insert(std::forward<decltype(value)>(value));
	}
	GameValue(std::string_view identifier, ValidGameValue auto&& value)
		: identifier(identifier)
	{
		insert(std::forward<decltype(value)>(value));
	}
	GameValue(func_get getter, func_set setter)
		: m_getter(getter), m_setter(setter) {}
	GameValue(std::string_view identifier, func_get getter, func_set setter)
		: identifier(identifier), m_getter(getter), m_setter(setter) {}
	GameValue(std::string_view identifier, ValidGameValue auto&& value, func_get getter, func_set setter)
		: identifier(identifier), m_getter(getter), m_setter(setter)
	{
		insert(std::forward<decltype(value)>(value));
	}

public:
	GameValue(set_temporary_t, ValidGameValue auto&& value)
		: temporary(true)
	{
		insert(std::forward<decltype(value)>(value));
	}
	GameValue(set_temporary_t, std::string_view identifier, ValidGameValue auto&& value)
		: identifier(identifier), temporary(true)
	{
		insert(std::forward<decltype(value)>(value));
	}
	GameValue(set_temporary_t, func_get getter, func_set setter)
		: temporary(true), m_getter(getter), m_setter(setter) {}
	GameValue(set_temporary_t, std::string_view identifier, func_get getter, func_set setter)
		: identifier(identifier), temporary(true), m_getter(getter), m_setter(setter) {}
	GameValue(set_temporary_t, std::string_view identifier, ValidGameValue auto&& value, func_get getter, func_set setter)
		: identifier(identifier), temporary(true), m_getter(getter), m_setter(setter)
	{
		insert(std::forward<decltype(value)>(value));
	}

public:
	GameValue(const GameValue& other)
		: identifier(other.identifier), temporary(other.temporary)
		, m_boolean(other.m_boolean), m_number(other.m_number), m_text(other.m_text), m_array(other.m_array), m_source(other.m_source)
		, m_getter(other.m_getter), m_setter(other.m_setter)
	{}
	GameValue(GameValue&& other) noexcept
		: identifier(std::move(other.identifier)), temporary(other.temporary)
		, m_boolean(std::move(other.m_boolean)), m_number(std::move(other.m_number)), m_text(std::move(other.m_text)), m_array(std::move(other.m_array)), m_source(std::move(other.m_source))
		, m_getter(other.m_getter), m_setter(other.m_setter)
	{}

	GameValue& operator=(const GameValue& other) noexcept;
	GameValue& operator=(GameValue&& other) noexcept;
	bool operator==(const GameValue& other) noexcept;
	explicit operator bool() const;

public:
	/// @brief The identifier of the variable.
	std::string identifier;

	/// @brief Marks if the variable is temporary and should be cleared when appropriate.
	bool temporary = false;

public:
	/// @brief Retrieves the stored value of the specified type, if present.
	/// @tparam T The type of value to retrieve. Must satisfy the `ValidGameValue` constraint.
	/// @param index An optional index to specify which element of the array to assign the value to, if applicable.
	/// @return A reference to a 'std::optional{ T }' containing the stored value if it exists; otherwise, throws `std::bad_variant_access` if the type is not supported.
	template<ValidGameValue T>
	[[inline]] const std::optional<T> get(std::optional<int64_t> index = std::nullopt) const;

	/// @brief Retrieves a pointer to the stored value of the specified type, if present.
	/// @tparam T The type of value to retrieve. Must satisfy the `ValidGameValue` constraint.
	/// @param index An optional index to specify which element of the array to assign the value to, if applicable.
	/// @return A pointer to type T containing the stored value if it exists; otherwise, throws `std::bad_variant_access` if the type is not supported.
	template<ValidGameValue T>
	[[inline]] const T* get_unsafe(std::optional<int64_t> index = std::nullopt) const;

	/// @brief Sets the value of the GameValue object to the provided value, resetting any existing number, text, or array state.
	/// @param value The new value to assign to the GameValue object. Must satisfy the ValidGameValue concept.
	/// @param index An optional index to specify which element of the array to assign the value to, if applicable.
	/// @return A reference to the modified GameValue object.
	[[inline]] GameValue& set(ValidGameValue auto&& value, std::optional<int64_t> index = std::nullopt);

	/// @brief Assigns a value to the GameValue, overwriting the value of the passed type. Other types are not affected.
	/// @param value The value to assign to the GameValue. Must satisfy the ValidGameValue concept.
	/// @param index An optional index to specify which element of the array to assign the value to, if applicable.
	/// @return A reference to the modified GameValue object.
	[[inline]] GameValue& assign(ValidGameValue auto&& value, std::optional<int64_t> index = std::nullopt);

	/// @brief Assigns values from another GameValue object to this one, overwriting the values of the specified types.
	/// @tparam ...Types A list of types to assign from the other GameValue.
	/// @param other The other GameValue object from which to assign values.
	/// @param index The index of the array to assign a value, if applicable.
	/// @return A reference to the modified GameValue object.
	template<ValidGameValue... Types>
	GameValue& assign(const GameValue& other, std::optional<int64_t> index = std::nullopt)
	{
		(assign(other.get<Types>().value_or(Types{}), index), ...);
		return *this;
	}

	/// @brief Unassigns a value type from the GameValue.
	/// @tparam Type The value type to unassign.
	/// @return A reference to the modified GameValue object.
	template<ValidGameValue Type>
	GameValue& unassign()
	{
		if constexpr (std::same_as<Type, bool>)
			m_boolean = std::nullopt;
		if constexpr (std::same_as<Type, double>)
			m_number = std::nullopt;
		if constexpr (std::same_as<Type, std::string>)
			m_text = std::nullopt;
		if constexpr (std::same_as<Type, std::vector<double>>)
			m_array = std::nullopt;
		if constexpr (std::same_as<Type, ScriptObject> || std::same_as<Type, std::vector<ScriptObject>>)
			m_source = std::nullopt;
		return *this;
	}

	/// @brief If the variable is an array, flattens it into a single value.
	/// @return A reference to the modified GameValue object.
	GameValue flatten(int64_t index) const noexcept;

	/// @brief Tests the GameValue as a flag check.
	/// @return True if the GameValue has a boolean value or a non-empty string value, false otherwise.
	bool testAsFlag() const;

	/// @brief Checks if the GameValue has a value of the specified type.
	/// @tparam T The type to check for. Must satisfy the `ValidGameValue` constraint.
	/// @return True if the GameValue has a value of the specified type; otherwise, false.
	template<ValidGameValue T>
	[[inline]] bool has() const;

	/// @brief Checks if the GameValue has multiple values.
	/// @return True if the GameValue has multiple values; otherwise, false.
	[[inline]] bool has_many() const;

	/// @brief Returns the stored getter function.
	/// @return The getter function stored in the object.
	func_get getGetter() const { return m_getter; }

	/// @brief Returns the stored setter function.
	/// @return The setter function stored in the object.
	func_set getSetter() const { return m_setter; }

	/// @brief Sets the getter function.
	/// @param getter The function to be used as the getter.
	void setGetter(func_get getter) { m_getter = getter; }

	/// @brief Sets the setter function for the object.
	/// @param setter The function to be used as the setter.
	void setSetter(func_set setter) { m_setter = setter; }

	/// @brief Serializes a variable for distribution.
	/// @param name The name of the game variable to serialize.
	/// @return An optional string that contains the serialized variable.
	std::optional<std::string> serializeModern(std::string_view name) const noexcept;

	/// @brief Serializes the variable for saving.
	/// @return A serialized string for writing to disk.
	template<ValidGameValue T = std::string>
	[[inline]] std::string serialize() const;

protected:
	std::optional<bool> m_boolean;
	std::optional<double> m_number;
	std::optional<std::string> m_text;
	std::optional<std::vector<double>> m_array;
	std::optional<std::vector<ScriptObject>> m_source;
	func_get m_getter;
	func_set m_setter;

	[[inline]] GameValue& insert(const ValidGameValue auto& value, std::optional<int64_t> index = std::nullopt);
	[[inline]] GameValue& insert(ValidGameValue auto&& value, std::optional<int64_t> index = std::nullopt);
};

//----------------------------

template<ValidGameValue T>
inline const std::optional<T> GameValue::get(std::optional<int64_t> index) const
{
	if constexpr (std::same_as<T, double>)
	{
		if (m_getter)
		{
			std::optional<double> val;
			if (m_getter(&val, index); val.has_value())
				return val;
		}
		else if (m_array.has_value() && index.has_value())
		{
			if (index.value() >= 0 && index.value() < (int64_t)m_array.value().size())
				return m_array.value().at(index.value());
			return 0.0;
		}
		if (m_number.has_value())
			return m_number.value();
		if (m_boolean.has_value())
			return m_boolean.value() ? 1.0 : 0.0;
		return std::nullopt;
	}
	if constexpr (std::same_as<T, std::string>)
	{
		if (m_getter)
		{
			std::optional<std::string> val;
			if (m_getter(&val, index); val.has_value())
				return val;
		}
		return m_text;
	}
	if constexpr (std::same_as<T, std::vector<double>>)
	{
		if (m_getter)
		{
			std::optional<std::vector<double>> val;
			if (m_getter(&val, index); val.has_value())
				return val;
		}
		return m_array;
	}
	if constexpr (std::same_as<T, bool>)
	{
		if (m_getter)
		{
			std::optional<bool> val;
			if (m_getter(&val, index); val.has_value())
				return val;
		}
		if (m_boolean.has_value())
			return m_boolean.value();
		if (m_number.has_value())
			return !DoubleIsZero(m_number.value());
		return std::nullopt;
	}
	if constexpr (std::same_as<T, ScriptObject>)
	{
		if (m_getter)
		{
			std::optional<std::vector<ScriptObject>> val;
			if (m_getter(&val, index); val.has_value())
				return val;
		}
		if (!m_source.has_value())
			return std::nullopt;
		if (index.has_value())
		{
			if (index.value() >= 0 && index.value() < (int64_t)m_source.value().size())
				return m_source.value().at(index.value());
			return m_source.value().at(0);
		}
		return m_source.value().at(0);
	}
	if constexpr (std::same_as<T, std::vector<ScriptObject>>)
	{
		if (m_getter)
		{
			std::optional<std::vector<ScriptObject>> val;
			if (m_getter(&val, index); val.has_value())
				return val;
		}
		return m_source;
	}
	else throw std::bad_variant_access();
}

template<ValidGameValue T>
inline const T* GameValue::get_unsafe(std::optional<int64_t> index) const
{
	if constexpr (std::same_as<T, double>)
	{
		if (m_getter) m_getter(const_cast<std::optional<double>*>(&m_number), index);
		if (m_array.has_value() && index.has_value())
		{
			if (index.value() >= 0 && index.value() < (int64_t)m_array.value().size())
				return &m_array.value().at(index.value());
			return nullptr;
		}
		if (!m_number.has_value()) return nullptr;
		return &m_number.value();
	}
	if constexpr (std::same_as<T, std::string>)
	{
		if (m_getter) m_getter(const_cast<std::optional<std::string>*>(&m_text), index);
		if (!m_text.has_value()) return nullptr;
		return &m_text.value();
	}
	if constexpr (std::same_as<T, std::vector<double>>)
	{
		if (m_getter) m_getter(const_cast<std::optional<std::vector<double>>*>(&m_array), index);
		if (!m_array.has_value()) return nullptr;
		return &m_array.value();
	}
	if constexpr (std::same_as<T, bool>)
	{
		if (m_getter) m_getter(const_cast<std::optional<bool>*>(&m_boolean), index);
		if (!m_boolean.has_value()) return nullptr;
		return &m_boolean.value();
	}
	if constexpr (std::same_as<T, ScriptObject>)
	{
		if (m_getter) m_getter(const_cast<std::optional<std::vector<ScriptObject>>*>(&m_source), index);
		if (!m_source.has_value()) return nullptr;
		if (index.has_value())
		{
			if (index.value() >= 0 && index.value() < (int64_t)m_source.value().size())
				return &m_source.value().at(index.value());
			return &m_source.value().at(0);
		}
		return &m_source.value().at(0);
	}
	if constexpr (std::same_as<T, std::vector<ScriptObject>>)
	{
		if (m_getter) m_getter(const_cast<std::optional<std::vector<ScriptObject>>*>(&m_source), index);
		if (!m_source.has_value()) return nullptr;
		return &m_source.value();
	}
	else throw std::bad_variant_access();
}

inline GameValue& GameValue::set(ValidGameValue auto&& value, std::optional<int64_t> index)
{
	m_number = std::nullopt;
	m_text = std::nullopt;
	m_array = std::nullopt;
	m_boolean = std::nullopt;
	m_source = std::nullopt;
	return insert(std::forward<decltype(value)>(value), index);
}

inline GameValue& GameValue::assign(ValidGameValue auto&& value, std::optional<int64_t> index)
{
	return insert(std::forward<decltype(value)>(value), index);
}

inline GameValue& GameValue::insert(const ValidGameValue auto& value, std::optional<int64_t> index)
{
	using V = std::remove_cvref_t<decltype(value)>;
	if constexpr (std::same_as<V, double>)
	{
		if (m_array.has_value() && index.has_value())
		{
			if (index.value() >= 0 && index.value() < (int64_t)m_array.value().size())
				m_array.value().at(index.value()) = value;
			if (m_setter) m_setter(&m_array, index);
		}
		else
		{
			m_number = value;
			if (m_setter) m_setter(&m_number, index);
		}
	}
	else if constexpr (std::same_as<V, std::string>)
	{
		if (value.empty())
			m_text = std::nullopt;
		else m_text = value;
		if (m_setter) m_setter(&m_text, index);
	}
	else if constexpr (std::same_as<V, std::vector<double>>)
	{
		m_array = value;
		if (m_setter) m_setter(&m_array, index);
	}
	else if constexpr (std::same_as<V, bool>)
	{
		m_boolean = value;
		if (m_setter) m_setter(&m_boolean, index);
	}
	else if constexpr (std::same_as<V, ScriptObject>)
	{
		if (m_source.has_value() && index.has_value())
		{
			if (index.value() >= 0 && index.value() < (int64_t)m_source.value().size())
				m_source.value().at(index.value()) = value;
		}
		else
		{
			m_source.value().clear();
			m_source.value().push_back(value);
		}
		if (m_setter) m_setter(&m_source, index);
	}
	else if constexpr (std::same_as<V, std::vector<ScriptObject>>)
	{
		m_source = value;
		if (m_setter) m_setter(&m_source, index);
	}
	else throw std::bad_variant_access();

	return *this;
}

inline GameValue& GameValue::insert(ValidGameValue auto&& value, std::optional<int64_t> index)
{
	using V = std::remove_cvref_t<decltype(value)>;
	if constexpr (std::same_as<V, double>)
	{
		if (m_array.has_value() && index.has_value())
		{
			if (index.value() >= 0 && index.value() < (int64_t)m_array.value().size())
				m_array.value().at(index.value()) = value;
			if (m_setter) m_setter(&m_array, index);
		}
		else
		{
			m_number = value;
			if (m_setter) m_setter(&m_number, index);
		}
	}
	else if constexpr (std::same_as<V, std::string>)
	{
		if (value.empty())
			m_text = std::nullopt;
		m_text = std::move(value);
		if (m_setter) m_setter(&m_text, index);
	}
	else if constexpr (std::same_as<V, std::vector<double>>)
	{
		m_array = std::move(value);
		if (m_setter) m_setter(&m_array, index);
	}
	else if constexpr (std::same_as<V, bool>)
	{
		m_boolean = value;
		if (m_setter) m_setter(&m_boolean, index);
	}
	else if constexpr (std::same_as<V, ScriptObject>)
	{
		if (m_source.has_value() && index.has_value())
		{
			if (index.value() >= 0 && index.value() < (int64_t)m_source.value().size())
				m_source.value().at(index.value()) = value;
		}
		else
		{
			m_source.value().clear();
			m_source.value().push_back(value);
		}
		if (m_setter) m_setter(&m_source, index);
	}
	else if constexpr (std::same_as<V, std::vector<ScriptObject>>)
	{
		m_source = std::move(value);
		if (m_setter) m_setter(&m_source, index);
	}
	else throw std::bad_variant_access();

	return *this;
}

template<ValidGameValue T>
inline bool GameValue::has() const
{
	if constexpr (std::same_as<T, double>)
		return m_number.has_value();
	else if constexpr (std::same_as<T, std::string>)
		return m_text.has_value();
	else if constexpr (std::same_as<T, std::vector<double>>)
		return m_array.has_value();
	else if constexpr (std::same_as<T, bool>)
		return m_boolean.has_value();
	else if constexpr (std::same_as<T, ScriptObject> || std::same_as<T, std::vector<ScriptObject>>)
		return m_source.has_value();
	return false;
}

inline bool GameValue::has_many() const
{
	int count = 0;
	if (m_number.has_value()) ++count;
	if (m_text.has_value()) ++count;
	if (m_array.has_value()) ++count;
	if (m_boolean.has_value()) ++count;
	if (m_source.has_value()) ++count;
	return count > 1;
}

//----------------------------

template<ValidGameValue T>
GameValue GameValue::deserialize(std::string identifier, const std::string_view data)
{
	if constexpr (std::same_as<T, bool>)
		return GameValue{ identifier, true };
	if constexpr (std::same_as<T, double>)
		return GameValue{ identifier, string::toDouble(data) };
	if constexpr (std::same_as<T, std::string>)
		return GameValue{ identifier, std::string{ data } };
	if constexpr (std::same_as<T, std::vector<double>>)
	{
		std::vector<double> array;
		for (auto number : string::split(data, ","sv))
			array.emplace_back(string::toDouble(number));
		return GameValue{ identifier, std::move(array) };
	}
	return GameValue{};
}

template<ValidGameValue T>
inline std::string GameValue::serialize() const
{
	if constexpr (std::same_as<T, bool>)
		return {};
	if constexpr (std::same_as<T, double>)
		return std::format("{}", m_number.value_or(0.0));
	if constexpr (std::same_as<T, std::string>)
		return m_text.value_or(std::string{});
	if constexpr (std::same_as<T, std::vector<double>>)
	{
		std::string array;
		if (m_array.has_value())
		{
			for (size_t i = 0; i < m_array.value().size(); ++i)
			{
				array += std::format("{}", (m_array.value())[i]);
				if (i != m_array.value().size() - 1)
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
	/// @return A weak pointer to the newly added GameValue.
	virtual std::weak_ptr<GameValue> add(std::string_view name, GameValue&& value) noexcept;

	/// @brief Adds a new game variable.
	/// @param variable The variable to add to the store (moved).
	/// @return A weak pointer to the newly added GameValue.
	virtual std::weak_ptr<GameValue> add(GameValue&& variable) noexcept;

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
	/// @return A std::weak_ptr to the GameValue associated with the given name. If the variable does not exist, the returned weak pointer will be empty.
	virtual std::weak_ptr<GameValue> get(std::string_view name) noexcept;

	/// @brief Retrieves a weak pointer to a game variable by its name.
	/// @param name The name of the game variable to retrieve.
	/// @return A weak pointer to the requested GameValue, or an empty weak pointer if not found.
	virtual const std::weak_ptr<GameValue> get(std::string_view name) const noexcept;

	/// @brief Retrieves the value associated with the specified name, if it exists.
	/// @tparam T The type to which the value should be converted.
	/// @param name The name of the value to retrieve.
	/// @return An optional containing the value of type GameValue if found; otherwise, an empty optional.
	template<ValidGameValue T>
	[[inline]] const std::optional<T> getValue(const std::string_view name) const noexcept;

	/// @brief Retrieves a game variable by name, or adds it if it does not exist.
	/// @param name The name of the game variable to retrieve or add.
	/// @return A weak pointer to the retrieved or newly added GameValue.
	virtual std::weak_ptr<GameValue> getOrAdd(std::string_view name) noexcept;

	/// @brief Retrieves a variable stub by name, or adds it if it does not exist.
	/// @param name The name of the variable to retrieve or add.
	/// @return A GameValue representing the variable with getters/setters to update the variable in storage.
	virtual GameValue getOrStub(std::string_view name);

	/// @brief Clears all temporary variables from the store.
	virtual void clearTemporary() noexcept;

	/// @brief Cleras temporary variables with a specific prefix from the store.
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
	bool static_container = false;

	/// @brief The variable store map.
	string_map<std::shared_ptr<GameValue>> store;
};

//----------------------------

template<ValidGameValue T>
inline const std::optional<T> GameVariableStore::getValue(const std::string_view name) const noexcept
{
	if (store.empty()) return std::nullopt;
	auto it = store.find(name);
	if (it == store.end()) return std::nullopt;
	// it->second->update();
	return it->second->get<T>();
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
	ScriptEvent event{ .type = type, .initiator = initiator, .args = { std::forward<decltype(args)>(args)... } };
	addEvent(std::move(event));
}

inline void ScriptEventQueue::addEvent(ScriptEventType type, ScriptObject initiator, string::InputRangeNotString auto&& range)
{
	static_assert(!string::PointerToConstCharString<decltype(range)>,
		"Don't use a const char* in the ranged variant of ScriptEventQueue::addEvent, pass in a std::string_view instead.");

	ScriptEvent event{ .type = type, .initiator = initiator };
	auto transformed = range | std::views::transform([](const auto& arg) -> std::any { return std::any{ arg }; });
	event.args.insert(event.args.end(), std::ranges::begin(transformed), std::ranges::end(transformed));
	addEvent(std::move(event));
}


////////////////////////////////////////////////////////////
// ScriptParameters
////////////////////////////////////////////////////////////

template<class T>
concept HasScriptParameters = requires(T t)
{
	{ T::scriptParameters } -> std::convertible_to<string_map<GameValue>>;
};

template<class T>
concept HasConstructibleScriptParameters = requires(T t)
{
	{ T::scriptParameters } -> std::convertible_to<string_map<GameValue>>;
	{ t.constructScriptParameters() } -> std::same_as<void>;
};

template<HasScriptParameters T>
inline std::optional<GameValue> getScriptParameter(T& source, std::string_view name)
{
	if constexpr (HasConstructibleScriptParameters<T>)
	{
		if (source.scriptParameters.empty())
			source.constructScriptParameters();
	}

	auto it = source.scriptParameters.find(name);
	if (it == source.scriptParameters.end())
		return std::nullopt;
	return it->second;
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
// Ranges
////////////////////////////////////////////////////////////

/// @brief Provides views that manipulate variables.
namespace variables
{

/// @brief A view that filters out temporary variables.
inline constexpr auto no_temporary = std::views::filter([](const decltype(GameVariableStore::store)::value_type& pair) -> bool { return !pair.second->temporary; });

/// @brief A view that filters out optional that don't have a value.
//inline constexpr auto with_value = std::views::filter([](const auto& opt) -> bool { return opt.has_value(); });

/// @brief Only gets variables that identify as flags.
inline constexpr auto only_flags = std::views::filter([](const decltype(GameVariableStore::store)::value_type& pair) -> bool { return pair.second->testAsFlag(); });

} // end namespace preagonal::variables


////////////////////////////////////////////////////////////
// Concepts
////////////////////////////////////////////////////////////

template<typename T>
concept ValidGameValueCallable = requires(T t)
{
	{ t() } -> std::convertible_to<GameValue>;
};

template<typename T>
concept ValidGameValueCallableWithIndex = requires(T t)
{
	{ t(std::declval<std::optional<int64_t>>()) } -> std::convertible_to<GameValue>;
};


////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////

template <typename T>
void copyToArrayAs(const auto& vec, auto& propvalue)
{
	size_t count = std::min(vec.size(), propvalue.size());
	auto it = std::begin(vec);
	for (size_t i = 0; i < count; ++i, ++it)
	{
		propvalue[i] = static_cast<T>(*it);
	}
}

inline void stupid_ide()
{
	auto not_transitive = std::format("");
}


////////////////////////////////////////////////////////////
// Prop helpers
////////////////////////////////////////////////////////////

/// @brief A getter function for a property that gets its results from another getter function.
GameValue::func_get gameValueGetter(ValidGameValueCallable auto getter)
{
	return [getter](GameValueVariant incoming, std::optional<int64_t> index)
	{
		GameValue value{ getter() };
		const auto picker = visit_functions
		{
			[&](std::optional<bool>* in) { *in = value.get<bool>(); },
			[&](std::optional<double>* in) { *in = value.get<double>(); },
			[&](std::optional<std::string>* in) { *in = value.get<std::string>(); },
			[&](std::optional<std::vector<double>>* in) { *in = value.get<std::vector<double>>(); },
			[&](std::optional<std::vector<ScriptObject>>* in) { *in = value.get<std::vector<ScriptObject>>(); }
		};
		std::visit(picker, incoming);
	};
}

/// @brief A getter function for a property that gets its results from another getter function.
inline GameValue::func_get gameValueGetter(ValidGameValueCallableWithIndex auto getter)
{
	return [getter](GameValueVariant incoming, std::optional<int64_t> index)
	{
		GameValue value{ getter(index) };
		const auto picker = visit_functions
		{
			[&](std::optional<bool>* in) { *in = value.get<bool>(); },
			[&](std::optional<double>* in) { *in = value.get<double>(); },
			[&](std::optional<std::string>* in) { *in = value.get<std::string>(); },
			[&](std::optional<std::vector<double>>* in) { *in = value.get<std::vector<double>>(); },
			[&](std::optional<std::vector<ScriptObject>>* in) { *in = value.get<std::vector<ScriptObject>>(); }
		};
		std::visit(picker, incoming);
	};
}

/// @brief A getter function for a property that gets its results from a property directly.
GameValue::func_get gameValueGetter(auto& value)
{
	using V = std::remove_cvref_t<decltype(value)>;
	static_assert(std::integral<V> || std::floating_point<V> || string::StringVariant<V> || std::ranges::forward_range<V> || std::same_as<V, ScriptObject> || std::same_as<V, std::vector<ScriptObject>>,
		"gameValueGetter called with an unsupported type. Supported types are integral, floats, string, ranges, or ScriptObjectSources.");

	// Number.
	if constexpr (std::integral<V> || std::floating_point<V>)
	{
		return [&value](GameValueVariant incoming, std::optional<int64_t> index)
		{
			if (auto var = std::get_if<std::optional<double>*>(&incoming); var != nullptr)
				**var = value;
		};
	}
	// String.
	else if constexpr (string::StringVariant<V>)
	{
		return [&value](GameValueVariant incoming, std::optional<int64_t> index)
		{
			if (auto var = std::get_if<std::optional<std::string>*>(&incoming); var != nullptr)
				**var = value;
		};
	}
	// ScriptObject (and array variant).
	else if constexpr (std::same_as<V, ScriptObject> || std::same_as<V, std::vector<ScriptObject>>)
	{
		return [&value](GameValueVariant incoming, std::optional<int64_t> index)
		{
			if (auto var = std::get_if<std::optional<std::vector<ScriptObject>>*>(&incoming); var != nullptr)
			{
				if constexpr (std::same_as<V, ScriptObject>)
				{
					(*var)->emplace();
					(*var)->value().push_back(value);
				}
				else **var = value;
			}
		};
	}
	// Array.
	else if constexpr (std::ranges::forward_range<V>)
	{
		return [&value](GameValueVariant incoming, std::optional<int64_t> index)
		{
			if (auto var = std::get_if<std::optional<std::vector<double>>*>(&incoming); var != nullptr)
			{
				// Transform the range to a vector of doubles.
				**var = value | std::views::transform([](const auto& v) { return static_cast<double>(v); }) | std::ranges::to<std::vector<double>>();
			}
		};
	}

	throw std::invalid_argument("gameValueGetter called with an unsupported type.");
}

/// @brief A setter function for a property that needs an additional setter function to write the values.
template<class Who, typename Prop>
GameValue::func_set gameValueSetter(Who* who, std::optional<Prop> prop, std::function<void(const GameValue&, std::optional<int64_t>)> setter)
{
	return [who, prop, setter](GameValueVariant incoming, std::optional<int64_t> index)
	{
		GameValue value;
		const auto picker = visit_functions
		{
			[&](std::optional<bool>* in) { if (in->has_value()) value.set(in->value(), index); },
			[&](std::optional<double>* in) { if (in->has_value()) value.set(in->value(), index); },
			[&](std::optional<std::string>* in) { if (in->has_value()) value.set(in->value(), index); },
			[&](std::optional<std::vector<double>>* in) { if (in->has_value()) value.set(in->value(), index); },
			[&](std::optional<std::vector<ScriptObject>>* in) { if (in->has_value()) value.set(in->value(), index); }
		};
		std::visit(picker, incoming);

		// Call the setter function.
		setter(value, index);

		// Record the modification time for the property.
		if (prop.has_value() && who != nullptr)
			who->modTime[PROPID(prop.value())] = currentTime();
	};
}

/// @brief A helper setter function that converts to a GameValue and passes to the next callback function.
inline GameValue::func_set gameValueSetter(std::function<void(const GameValue&, std::optional<int64_t>)> setter)
{
	return [setter](GameValueVariant incoming, std::optional<int64_t> index)
	{
		GameValue value;
		const auto picker = visit_functions
		{
			[&](std::optional<bool>* in) { if (in->has_value()) value.set(in->value(), index); },
			[&](std::optional<double>* in) { if (in->has_value()) value.set(in->value(), index); },
			[&](std::optional<std::string>* in) { if (in->has_value()) value.set(in->value(), index); },
			[&](std::optional<std::vector<double>>* in) { if (in->has_value()) value.set(in->value(), index); },
			[&](std::optional<std::vector<ScriptObject>>* in) { if (in->has_value()) value.set(in->value(), index); }
		};
		std::visit(picker, incoming);

		// Call the setter function.
		setter(value, index);
	};
}

/// @brief A setter function for a property that can directly set to a value.
template<class Who, typename Prop, typename Value>
GameValue::func_set gameValueSetter(Who* who, std::optional<Prop> prop, Value& propvalue)
{
	using V = std::remove_cvref_t<Value>;
	static_assert(std::integral<V> || std::floating_point<V> || string::StringVariant<V> || std::ranges::random_access_range<V>,
		"gameValueSetter called with an unsupported type. Supported types are integral, floats, string, or ranges.");

	// Number.
	if constexpr (std::integral<V> || std::floating_point<V>)
	{
		return [who, prop, &propvalue](GameValueVariant incoming, std::optional<int64_t> index)
		{
			if (auto value = std::get_if<std::optional<double>*>(&incoming); value != nullptr)
				propvalue = static_cast<V>((*value)->value_or(V{}));
			else if (auto value = std::get_if<std::optional<std::vector<double>>*>(&incoming); value != nullptr && (*value)->has_value() && !(*value)->value().empty())
			{
				auto& vec = (*value)->value();
				auto indexValue = index.value_or(0);
				if (indexValue >= 0 && indexValue < (int64_t)vec.size())
					propvalue = static_cast<V>(vec.at(indexValue));
			}
			else if (auto value = std::get_if<std::optional<bool>*>(&incoming); value != nullptr)
				propvalue = static_cast<V>((*value)->value_or(false) ? 1 : 0);
			if (prop.has_value())
				who->modTime[PROPID(prop.value())] = currentTime();
		};
	}
	// String.
	else if constexpr (string::StringVariant<V>)
	{
		return [who, prop, &propvalue](GameValueVariant incoming, std::optional<int64_t> index)
		{
			if (auto value = std::get_if<std::optional<std::string>*>(&incoming); value != nullptr)
				propvalue = static_cast<V>(**value);
			if (prop.has_value())
				who->modTime[PROPID(prop.value())] = currentTime();
		};
	}
	// Array.
	else if constexpr (std::ranges::random_access_range<V>)
	{
		return [who, prop, &propvalue](GameValueVariant incoming, std::optional<int64_t> index)
		{
			size_t propvalue_size = std::ranges::size(propvalue);
			if (propvalue_size > 0)
			{
				using value_type = std::remove_cvref_t<decltype(propvalue[0])>;

				// Setting an individual index in an array.
				if (index.has_value() && index.value() >= 0 && index.value() < (int64_t)propvalue_size)
				{
					if (auto value = std::get_if<std::optional<std::vector<double>>*>(&incoming); value != nullptr && (*value)->has_value())
					{
						std::vector<double>& darray = (**value).value();
						if (index.value() < (int64_t)darray.size())
							propvalue[index.value()] = static_cast<value_type>(darray.at(index.value()));
						else propvalue[index.value()] = value_type{};
					}
					if (prop.has_value())
						who->modTime[PROPID(prop.value()) + index.value()] = currentTime();
				}
				// Setting the whole array.
				else if (!index.has_value())
				{
					if (auto value = std::get_if<std::optional<std::vector<double>>*>(&incoming); value != nullptr)
					{
						const std::vector<double> vec = (*value)->value_or(std::vector<double>{});
						copyToArrayAs<value_type>(vec, propvalue);
					}
					if (prop.has_value())
						who->modTime[PROPID(prop.value())] = currentTime();
				}
			}
		};
	}

	throw std::invalid_argument("gameValueSetter called with an unsupported type.");
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // SCRIPTCONTAINERS_H
