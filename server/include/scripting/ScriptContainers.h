#ifndef SCRIPTCONTAINERS_H
#define SCRIPTCONTAINERS_H

#include <queue>
#include <any>
#include <map>
#include <variant>
#include <functional>
#include <optional>
#include <vector>
#include <utility>

#include <common.h>

#include <scripting/ScriptTypes.h>
#include <utilities/StringUtils.h>

////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

class Level;
using LevelPtr = std::shared_ptr<Level>;

////////////////////////////////////////////////////////////
// GameValue
////////////////////////////////////////////////////////////

/// @brief Concept to ensure the value passed to `GameValue` is a valid type.
template<class T>
concept ValidGameValue = std::same_as<std::remove_cvref_t<T>, double>
	|| std::same_as<std::remove_cvref_t<T>, std::string>
	|| std::same_as<std::remove_cvref_t<T>, std::vector<double>>;

/// @brief A container that can hold one of three types: double, std::string, or std::vector<double>.
///
/// This is used to represent a single value that can be one or many of these types.
/// It provides methods to set and retrieve the value in a type-safe manner.
struct GameValue
{
	GameValue() = default;
	//GameValue(const ValidGameValue auto& value)
	//{
	//	insert(std::forward<decltype(value)>(value));
	//}
	GameValue(ValidGameValue auto&& value)
	{
		insert(std::forward<decltype(value)>(value));
	}
	GameValue(const GameValue& other)
		: m_number(other.m_number), m_text(other.m_text), m_array(other.m_array)
	{}
	GameValue(GameValue&& other) noexcept
		: m_number(std::move(other.m_number)), m_text(std::move(other.m_text)), m_array(std::move(other.m_array))
	{}

	GameValue& operator=(const GameValue& other) noexcept;
	GameValue& operator=(GameValue&& other) noexcept;
	bool operator==(const GameValue& other) noexcept;
	explicit operator bool() const;

public:
	/// @brief Retrieves the stored value of the specified type, if present.
	/// @tparam T The type of value to retrieve. Must satisfy the `ValidGameValue` constraint.
	/// @param index An optional index to specify which element of the array to assign the value to, if applicable.
	/// @return A reference to a 'std::optional{ T }' containing the stored value if it exists; otherwise, throws `std::bad_variant_access` if the type is not supported.
	template<ValidGameValue T>
	[[inline]] const std::optional<T> get(std::optional<size_t> index = std::nullopt) const;

	/// @brief Retrieves a pointer to the stored value of the specified type, if present.
	/// @tparam T The type of value to retrieve. Must satisfy the `ValidGameValue` constraint.
	/// @param index An optional index to specify which element of the array to assign the value to, if applicable.
	/// @return A pointer to type T containing the stored value if it exists; otherwise, throws `std::bad_variant_access` if the type is not supported.
	template<ValidGameValue T>
	[[inline]] const T* get_unsafe(std::optional<size_t> index = std::nullopt) const;

	/// @brief Sets the value of the GameValue object to the provided value, resetting any existing number, text, or array state.
	/// @param value The new value to assign to the GameValue object. Must satisfy the ValidGameValue concept.
	/// @param index An optional index to specify which element of the array to assign the value to, if applicable.
	/// @return A reference to the modified GameValue object.
	[[inline]] GameValue& set(ValidGameValue auto&& value, std::optional<size_t> index = std::nullopt);

	/// @brief Assigns a value to the GameValue, overwriting the value of the passed type. Other types are not affected.
	/// @param value The value to assign to the GameValue. Must satisfy the ValidGameValue concept.
	/// @param index An optional index to specify which element of the array to assign the value to, if applicable.
	/// @return A reference to the modified GameValue object.
	[[inline]] GameValue& assign(ValidGameValue auto&& value, std::optional<size_t> index = std::nullopt);

	/// @brief Assigns values from another GameValue object to this one, overwriting the values of the specified types.
	/// @tparam ...Types A list of types to assign from the other GameValue.
	/// @param other The other GameValue object from which to assign values.
	/// @return A reference to the modified GameValue object.
	template<ValidGameValue... Types>
	GameValue& assign(GameValue& other)
	{
		(insert(other.get<Types>().value_or(Types{})), ...);
		return *this;
	}

private:
	std::optional<double> m_number;
	std::optional<std::string> m_text;
	std::optional<std::vector<double>> m_array;

	[[inline]] GameValue& insert(const ValidGameValue auto& value, std::optional<size_t> index = std::nullopt);
	[[inline]] GameValue& insert(ValidGameValue auto&& value, std::optional<size_t> index = std::nullopt);
};

//----------------------------

template<ValidGameValue T>
inline const std::optional<T> GameValue::get(std::optional<size_t> index) const
{
	if constexpr (std::same_as<T, double>)
	{
		if (m_array.has_value() && index.has_value())
		{
			if (index.value() < m_array.value().size())
				return m_array.value().at(index.value());
			return 0.0;
		}
		return m_number;
	}
	if constexpr (std::same_as<T, std::string>)
		return m_text;
	if constexpr (std::same_as<T, std::vector<double>>)
		return m_array;
	else [[unlikely]]
		throw std::bad_variant_access();
}

template<ValidGameValue T>
inline const T* GameValue::get_unsafe(std::optional<size_t> index) const
{
	static const double empty_number = 0.0;
	static const std::string empty_string{};
	static const std::vector<double> empty_array{};

	if constexpr (std::same_as<T, double>)
	{
		if (m_array.has_value() && index.has_value())
		{
			if (index.value() < m_array.value().size())
				return &m_array.value().at(index.value());
			return &empty_number;
		}
		const auto* ptr = &m_number.value();
		if (!ptr) ptr = &empty_number;
		return ptr;
	}
	if constexpr (std::same_as<T, std::string>)
	{
		const auto* ptr = &m_text.value();
		if (!ptr) ptr = &empty_string;
		return ptr;
	}
	if constexpr (std::same_as<T, std::vector<double>>)
	{
		const auto* ptr = &m_array.value();
		if (!ptr) ptr = &empty_array;
		return ptr;
	}
	else [[unlikely]]
		throw std::bad_variant_access();
}

inline GameValue& GameValue::set(ValidGameValue auto&& value, std::optional<size_t> index)
{
	m_number = std::nullopt;
	m_text = std::nullopt;
	m_array = std::nullopt;
	return insert(std::forward<decltype(value)>(value), index);
}

inline GameValue& GameValue::assign(ValidGameValue auto&& value, std::optional<size_t> index)
{
	return insert(std::forward<decltype(value)>(value), index);
}

inline GameValue& GameValue::insert(const ValidGameValue auto& value, std::optional<size_t> index)
{
	using V = std::remove_cvref_t<decltype(value)>;
	if constexpr (std::same_as<V, double>)
	{
		if (m_array.has_value() && index.has_value())
		{
			if (index.value() < m_array.value().size())
				m_array.value().at(index.value()) = value;
			return *this;
		}
		m_number = value;
	}
	else if constexpr (std::same_as<V, std::string>)
		m_text = value;
	else if constexpr (std::same_as<V, std::vector<double>>)
		m_array = value;
	else [[unlikely]]
		throw std::bad_variant_access();
	return *this;
}

inline GameValue& GameValue::insert(ValidGameValue auto&& value, std::optional<size_t> index)
{
	using V = std::remove_cvref_t<decltype(value)>;
	if constexpr (std::same_as<V, double>)
	{
		if (m_array.has_value() && index.has_value())
		{
			if (index.value() < m_array.value().size())
				m_array.value().at(index.value()) = value;
			return *this;
		}
		m_number = value;
	}
	else if constexpr (std::same_as<V, std::string>)
		m_text = std::move(value);
	else if constexpr (std::same_as<V, std::vector<double>>)
		m_array = std::move(value);
	else [[unlikely]]
		throw std::bad_variant_access();
	return *this;
}

////////////////////////////////////////////////////////////
// GameVariable
////////////////////////////////////////////////////////////

/// @brief Represents a variable with an identifier and an associated value.
struct GameVariable
{
	using func_get = std::function<GameValue(std::string_view)>;
	using func_set = std::function<void(GameVariable&, const GameValue&, std::optional<size_t>)>;

	GameVariable() = default;
	GameVariable(const std::string& name, GameValue&& value)
		: identifier(name), m_value(std::move(value)) {}
	GameVariable(const std::string& name, GameValue&& value, func_get getter, func_set setter)
		: identifier(name), m_getter(getter), m_setter(setter) {}
	GameVariable(const std::string& name, func_get getter, func_set setter)
		: identifier(name), m_getter(getter), m_setter(setter) {}
	GameVariable(const GameVariable& other)
		: identifier(other.identifier), m_value(other.m_value),
		  m_getter(other.m_getter), m_setter(other.m_setter) {}
	GameVariable(GameVariable&& other) noexcept
		: identifier(std::move(other.identifier)), m_value(std::move(other.m_value)),
		  m_getter(std::move(other.m_getter)), m_setter(std::move(other.m_setter)) {}

	GameVariable& operator=(const GameVariable& other);
	GameVariable& operator=(GameVariable&& other) noexcept;
	[[inline]] auto operator=(ValidGameValue auto&& value) -> GameVariable&;
	[[inline]] auto operator=(const ValidGameValue auto& value) -> GameVariable&;
	operator double() const;
	operator std::string() const;

	// Keeping this in the header since the IDE really hates it when in a separate file.
	operator std::vector<double>() const
	{
		auto* value = game_value().get_unsafe<std::vector<double>>();
		return (value != nullptr) ? *value : std::vector<double>{};
	}

public:
	/// @brief The identifier of the variable.
	std::string identifier;

public:
	/// @brief Sets the callbacks for getting and setting the variable's value.
	/// @param getter The function to get the value of the variable.
	/// @param setter The function to set the value of the variable.
	void setCallbacks(func_get getter, func_set setter);

	/// @brief Gets the getter callback function.
	/// @return The getter callback function.
	func_get getCallbackGetter() const { return m_getter; }

	/// @brief Gets the setter callback function.
	/// @return The setter callback function.
	func_set getCallbackSetter() const { return m_setter; }

	/// @brief Retrieves the stored value of the specified type, if present.
	/// @tparam T The type of value to retrieve. Must satisfy the `ValidGameValue` constraint.
	/// @param index An optional index to specify which element of the array to assign the value to, if applicable.
	/// @return A reference to a 'std::optional{ T }' containing the stored value if it exists; otherwise, throws `std::bad_variant_access` if the type is not supported.
	template<ValidGameValue T>
	[[inline]] const std::optional<T> get(std::optional<size_t> index = std::nullopt) const;

	/// @brief Retrieves a pointer to the stored value of the specified type, if present.
	/// @tparam T The type of value to retrieve. Must satisfy the `ValidGameValue` constraint.
	/// @param index An optional index to specify which element of the array to assign the value to, if applicable.
	/// @return A pointer to type T containing the stored value if it exists; otherwise, throws `std::bad_variant_access` if the type is not supported.
	template<ValidGameValue T>
	[[inline]] const T* get_unsafe(std::optional<size_t> index = std::nullopt) const;

	/// @brief Gets the underlying GameValue object.
	/// @return A reference to the underlying GameValue object.
	GameValue& get_underlying();

	/// @brief Gets the underlying GameValue object (const version).
	/// @return A const reference to the underlying GameValue object.
	const GameValue& get_underlying() const;

	/// @brief Sets the value of the GameValue object to the provided value, resetting any existing number, text, or array state.
	/// @param value The new value to assign to the GameValue object. Must satisfy the ValidGameValue concept.
	/// @param index An optional index to specify which element of the array to assign the value to, if applicable.
	/// @return A reference to the modified GameValue object.
	[[inline]] GameVariable& set(ValidGameValue auto&& value, std::optional<size_t> index = std::nullopt);

	/// @brief Assigns a value to the GameValue, overwriting the value of the passed type. Other types are not affected.
	/// @param value The value to assign to the GameValue. Must satisfy the ValidGameValue concept.
	/// @param index An optional index to specify which element of the array to assign the value to, if applicable.
	/// @return A reference to the modified GameValue object.
	[[inline]] GameVariable& assign(ValidGameValue auto&& value, std::optional<size_t> index = std::nullopt);

	/// @brief Assigns values from another GameVariable object to this one, overwriting the values of the specified types.
	/// @tparam ...Types A list of types to assign from the other GameVariable.
	/// @param other The other GameVariable object from which to assign values.
	/// @return A reference to the modified GameVariable object.
	template<ValidGameValue... Types>
	GameVariable& assign(const GameVariable& other, std::optional<size_t> index = std::nullopt)
	{
		(assign(other.get<Types>().value_or(Types{}), index), ...);
		return *this;
	}

	/// @brief Assigns values from another GameValue object to this one, overwriting the values of the specified types.
	/// @tparam ...Types A list of types to assign from the other GameValue.
	/// @param other The other GameValue object from which to assign values.
	/// @return A reference to the modified GameVariable object.
	template<ValidGameValue... Types>
	GameVariable& assign(const GameValue& other, std::optional<size_t> index = std::nullopt)
	{
		(assign(other.get<Types>().value_or(Types{}), index), ...);
		return *this;
	}

private:
	GameValue& game_value();
	const GameValue& game_value() const;

	mutable GameValue m_value;
	func_get m_getter;
	func_set m_setter;
};

/// @brief Defines a variant type that can hold either a weak pointer to a GameVariable or a GameVariable object.
///
/// Normally, you would receive a weak pointer to a GameVariable from the store,
/// but when a variable is not assigned to a store yet, a normal GameVariable object is used.
using GameVariableVariant = std::variant<std::weak_ptr<GameVariable>, GameVariable>;

//----------------------------

inline auto GameVariable::operator=(ValidGameValue auto&& value) -> GameVariable&
{
	set(std::forward<decltype(value)>(value));
	return *this;
}

inline auto GameVariable::operator=(const ValidGameValue auto& value) -> GameVariable&
{
	set(value);
	return *this;
}

template<ValidGameValue T>
inline const std::optional<T> GameVariable::get(std::optional<size_t> index) const
{
	auto& value = game_value();
	return value.get<T>(index);
}

template<ValidGameValue T>
inline const T* GameVariable::get_unsafe(std::optional<size_t> index) const
{
	auto& value = game_value();
	return value.get_unsafe<T>(index);
}

inline GameVariable& GameVariable::set(ValidGameValue auto&& value, std::optional<size_t> index)
{
	m_value.set(std::forward<decltype(value)>(value), index);
	if (m_setter) [[unlikely]]
		m_setter(*this, m_value, index);
	return *this;
}

inline GameVariable& GameVariable::assign(ValidGameValue auto&& value, std::optional<size_t> index)
{
	m_value.assign(std::forward<decltype(value)>(value), index);
	if (m_setter) [[unlikely]]
		m_setter(*this, m_value, index);
	return *this;
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
	virtual std::weak_ptr<GameVariable> get_or_add(std::string_view name) noexcept;

	/// @brief Retrieves the value of a game variable by name, or, if it does not exist, returns a stub with a setter that adds the variable to the store when assigned a value.
	/// @param name The name of the game variable to retrieve.
	/// @return A GameVariableVariant containing the value of the variable if found, with a stub setter that adds the variable to the store if not found.
	virtual GameVariableVariant get_or_stub(std::string_view name) noexcept;

public:
	/// @brief Marks the container as not accepting any new variables, nor deleting existing ones.
	bool static_container = false;

	/// @brief The variable store map.
	std::map<std::string, std::shared_ptr<GameVariable>, std::less<>> store;

private:
	void stub_new(GameVariable& variable, const GameValue& value);
};

////////////////////////////////////////////////////////////
// ScriptObjectSource
////////////////////////////////////////////////////////////

/// @brief Identifies an object type that may be used by a scripting language.
enum class ScriptObjectSourceType
{
	PLAYER,
	NPC,
	LEVEL,
	SERVER
};

/// @brief Binds a source object type with an identifier.
/// 
/// The first element is the identifier, which may be an id or a hash.
using ScriptObjectSource = std::pair<size_t, ScriptObjectSourceType>;

namespace source
{
/// @brief Creates a ScriptObjectSource for a player with the given id.
constexpr ScriptObjectSource FromPlayer(size_t id)
{
	return std::make_pair(id, ScriptObjectSourceType::PLAYER);
}
/// @brief Creates a ScriptObjectSource for an NPC with the given id.
constexpr ScriptObjectSource FromNPC(size_t id)
{
	return std::make_pair(id, ScriptObjectSourceType::NPC);
}
/// @brief Creates a ScriptObjectSource from a LevelPtr by hashing the level's name.
ScriptObjectSource FromLevel(LevelPtr level);

/// @brief Creates a ScriptObjectSource for the server.
constexpr ScriptObjectSource FromServer()
{
	return std::make_pair(static_cast<size_t>(0), ScriptObjectSourceType::SERVER);
}
} // end namespace source

////////////////////////////////////////////////////////////
// ScriptEvent
////////////////////////////////////////////////////////////

/// @brief Represents an event in a scripting system, including its type, the source that initiated it, and any associated arguments.
struct ScriptEvent
{
	ScriptEventType type;
	ScriptObjectSource initiator;
	std::vector<std::any> args;
};

////////////////////////////////////////////////////////////
// ScriptEventQueue
////////////////////////////////////////////////////////////

/// @brief Queued script events.
class ScriptEventQueue
{
public:
	/// @brief Gets the underlying queue of script events.
	/// @return The queue of script events.
	[[inline]] std::queue<ScriptEvent>& queue();

public:
	/// @brief Adds an event to the queue with the specified type and initiator.
	/// @param type The type of the script event to add.
	/// @param initiator Who initiated the event.
	void addEvent(ScriptEventType type, ScriptObjectSource initiator);

	/// @brief Adds an event to the queue with the specified type, initiator, and additional arguments.
	/// @param type The type of the script event to add.
	/// @param initiator Who initiated the event.
	/// @param ...args A list of additional arguments to be passed with the event.
	[[inline]] void addEvent(ScriptEventType type, ScriptObjectSource initiator, auto&&... args);

private:
	void addEvent(const ScriptEvent& event);
	void addEvent(ScriptEvent&& event);

private:
	std::queue<ScriptEvent> m_eventQueue;
};

//----------------------------

inline std::queue<ScriptEvent>& ScriptEventQueue::queue()
{
	return m_eventQueue;
}

inline void ScriptEventQueue::addEvent(ScriptEventType type, ScriptObjectSource initiator, auto&&... args)
{
	ScriptEvent event{ .type = type, .initiator = initiator, .args = { std::forward<decltype(args)>(args)... } };
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
// Functions
////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // SCRIPTCONTAINERS_H
