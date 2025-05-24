#ifndef SCRIPTCONTAINERS_H
#define SCRIPTCONTAINERS_H

#include <queue>
#include <any>
#include <map>
#include <variant>
#include <functional>
#include <utility>

#include <common.h>

#include <scripting/ScriptTypes.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

enum class ScriptEventSourceType
{
	SERVER,
	PLAYER,
	NPC
};
using ScriptEventSource = std::pair<uint32_t, ScriptEventSourceType>;

//---------------------------

namespace source
{
constexpr ScriptEventSource FromPlayer(PlayerID id)
{
	return std::make_pair(id, ScriptEventSourceType::PLAYER);
}

constexpr ScriptEventSource FromNPC(NPCID id)
{
	return std::make_pair(id, ScriptEventSourceType::NPC);
}

constexpr ScriptEventSource FromServer()
{
	return std::make_pair(static_cast<uint32_t>(0), ScriptEventSourceType::SERVER);
}
} // end namespace source

////////////////////////////////////////////////////////////
// ScriptEvent
////////////////////////////////////////////////////////////

struct ScriptEvent
{
	ScriptEventType type;
	ScriptEventSource initiator;
	std::vector<std::any> args;
};

//---------------------------

// Queued script events.
class ScriptEventQueue
{
public:
	[[inline]] std::queue<ScriptEvent>& queue();

public:
	void addEvent(ScriptEventType type, ScriptEventSource initiator);

	[[inline]] void addEvent(ScriptEventType type, ScriptEventSource initiator, auto&&... args);

private:
	void addEvent(const ScriptEvent& event);
	void addEvent(ScriptEvent&& event);

private:
	std::queue<ScriptEvent> m_eventQueue;
};

//---------------------------

inline std::queue<ScriptEvent>& ScriptEventQueue::queue()
{
	return m_eventQueue;
}

inline void ScriptEventQueue::addEvent(ScriptEventType type, ScriptEventSource initiator, auto&&... args)
{
	ScriptEvent event{ .type = type, .initiator = initiator, .args = { std::forward<decltype(args)>(args)... } };
	addEvent(std::move(event));
}

////////////////////////////////////////////////////////////
// ScriptVariable / ScriptVariableContainer
////////////////////////////////////////////////////////////

// An identifier (normal and array variant).
using ScriptIdentifier = std::variant<std::string, std::pair<std::string, size_t>>;

// A script variable.
using ScriptVariable = std::variant<double, std::string, std::vector<double>>;
using ScriptVariablePair = std::pair<ScriptVariable, ScriptVariable>;

// A container for a script variable.
// This is used to store a variable and its getter/setter functions.
// Scripting language engines should use this intead of raw ScriptVariable values.
class ScriptVariableContainer
{
	using func_get = std::function<ScriptVariable(const ScriptIdentifier&)>;
	using func_set = std::function<void(const ScriptIdentifier&, const ScriptVariable&)>;

public:
	// Don't allow a default constructor.  Store with std::optional instead.
	ScriptVariableContainer() = delete;

	// Construct via a ScriptIdentifier.
	ScriptVariableContainer(const ScriptIdentifier& identifier, func_get getter = {}, func_set setter = {})
		: m_getter(getter), m_setter(setter), m_identifier(identifier)
	{
		retrieveFromGetter();
	}
	ScriptVariableContainer(ScriptIdentifier&& identifier, func_get getter = {}, func_set setter = {}) noexcept
		: m_getter(getter), m_setter(setter), m_identifier(std::move(identifier))
	{
		retrieveFromGetter();
	}

	// Construct via a ScriptVariable.
	// Doesn't have getter functions because those require a ScriptIdentifier.
	ScriptVariableContainer(const ScriptVariable& value, func_set setter = {})
		: m_setter(setter), m_value(value) {}
	ScriptVariableContainer(ScriptVariable&& value, func_set setter = {}) noexcept
		: m_setter(setter), m_value(std::move(value)) {}

	// Construct via a ScriptIdentifier and a ScriptVariable.
	ScriptVariableContainer(const ScriptIdentifier& identifier, const ScriptVariable& value, func_set setter = {})
		: m_setter(setter), m_identifier(identifier), m_value(value) {}
	ScriptVariableContainer(const ScriptIdentifier& identifier, ScriptVariable&& value, func_set setter = {}) noexcept
		: m_setter(setter), m_identifier(identifier), m_value(std::move(value)) {}
	ScriptVariableContainer(ScriptIdentifier&& identifier, const ScriptVariable& value, func_set setter = {}) noexcept
		: m_setter(setter), m_identifier(std::move(identifier)), m_value(value) {}
	ScriptVariableContainer(ScriptIdentifier&& identifier, ScriptVariable&& value, func_set setter = {}) noexcept
		: m_setter(setter), m_identifier(std::move(identifier)), m_value(std::move(value)) {}

public:
	// Copy and move constructors.
	// TODO(Nalin): std::any is just going to copy-construct anything.  Consider optimizing by wrapping with a std::shared_ptr.
	// [[deprecated("Prefer moving with std::move()")]]
	ScriptVariableContainer(const ScriptVariableContainer& other)
		: m_getter(other.m_getter), m_setter(other.m_setter), m_identifier(other.m_identifier), m_value(other.m_value)
	{}
	ScriptVariableContainer(ScriptVariableContainer&& other) noexcept
		: m_getter(other.m_getter), m_setter(other.m_setter), m_identifier(std::move(other.m_identifier)), m_value(std::move(other.m_value))
	{}

public:
	// Assignment and move assignment operators from a ScriptVariable.
	[[inline]] ScriptVariableContainer& operator=(const ScriptVariable& value) noexcept;
	[[inline]] ScriptVariableContainer& operator=(ScriptVariable&& value) noexcept;

public:
	[[inline]] bool hasIdentifier() const noexcept;
	[[inline]] const std::optional<ScriptIdentifier>& getIdentifier() const noexcept;
	[[inline]] void setIdentifier(const ScriptIdentifier& identifier) noexcept;
	[[inline]] void setIdentifier(ScriptIdentifier&& identifier) noexcept;
	[[inline]] std::optional<ScriptIdentifier>& getMutableIdentifier() noexcept;

public:
	// Get the underlying ScriptVariable.
	[[inline]] ScriptVariable& get() noexcept;

	// Get the underlying ScriptVariable.
	[[inline]] const ScriptVariable& get() const noexcept;

	// Get the ScriptVariable as a specific data type.
	template<typename T> requires(VariantContainsType<ScriptVariable, T>)
	[[inline]] T get() const noexcept;

	// Get the ScriptVariable as a boolean data type.
	template<>
	[[inline]] bool get<bool>() const noexcept;

	// Sets the ScriptVariable to a new value.
	ScriptVariableContainer& set(const ScriptVariable& value) noexcept;
	ScriptVariableContainer& set(ScriptVariable&& value) noexcept;
	ScriptVariableContainer& set(const ScriptVariableContainer& value) noexcept;
	ScriptVariableContainer& set(ScriptVariableContainer&& value) noexcept;

public:
	ScriptVariableContainer& setGetter(func_get getter) noexcept
	{
		m_getter = getter;
		return *this;
	}
	ScriptVariableContainer& setSetter(func_set setter) noexcept
	{
		m_setter = setter;
		return *this;
	}
	bool hasGetter() const noexcept
	{
		return m_getter != nullptr;
	}
	bool hasSetter() const noexcept
	{
		return m_setter != nullptr;
	}

public:
	[[inline]] void retrieveFromGetter() noexcept;

private:
	func_get m_getter;
	func_set m_setter;
	std::optional<ScriptIdentifier> m_identifier;
	ScriptVariable m_value;
};

//---------------------------

inline ScriptVariableContainer& ScriptVariableContainer::operator=(const ScriptVariable& value) noexcept
{
	set(value);
}

inline ScriptVariableContainer& ScriptVariableContainer::operator=(ScriptVariable&& value) noexcept
{
	set(std::move(value));
}

inline bool ScriptVariableContainer::hasIdentifier() const noexcept
{
	return m_identifier.has_value();
}

inline const std::optional<ScriptIdentifier>& ScriptVariableContainer::getIdentifier() const noexcept
{
	return m_identifier;
}

inline void ScriptVariableContainer::setIdentifier(const ScriptIdentifier& identifier) noexcept
{
	m_identifier = identifier;
}

inline void ScriptVariableContainer::setIdentifier(ScriptIdentifier&& identifier) noexcept
{
	m_identifier = identifier;
}

inline std::optional<ScriptIdentifier>& ScriptVariableContainer::getMutableIdentifier() noexcept
{
	return m_identifier;
}

inline ScriptVariable& ScriptVariableContainer::get() noexcept
{
	return m_value;
}

inline const ScriptVariable& ScriptVariableContainer::get() const noexcept
{
	return m_value;
}

template<typename T> requires(VariantContainsType<ScriptVariable, T>)
inline T ScriptVariableContainer::get() const noexcept
{
	if (std::holds_alternative<T>(m_value))
		return std::get<T>(m_value);
	if constexpr (std::same_as<T, double>)
		return 0.0;
	return T{};
}

template<>
inline bool ScriptVariableContainer::get<bool>() const noexcept
{
	if (std::holds_alternative<double>(m_value))
		return std::get<double>(m_value) != 0.0;
	return false;
}

inline void ScriptVariableContainer::retrieveFromGetter() noexcept
{
	if (m_getter && m_identifier.has_value())
		m_value = std::move(m_getter(m_identifier.value()));
}

////////////////////////////////////////////////////////////
// ScriptVariableStore
////////////////////////////////////////////////////////////

struct ScriptVariableStore
{
public:
	virtual ~ScriptVariableStore() {}

public:
	virtual ScriptVariableContainer add(std::string name, ScriptVariable value) noexcept;
	virtual bool remove(std::string_view name) noexcept;
	virtual bool contains(std::string_view name) const noexcept;
	virtual std::optional<ScriptVariableContainer> get(std::string name) noexcept;
	virtual std::optional<const ScriptVariableContainer> get(std::string name) const noexcept;
	virtual ScriptVariableContainer get_or_add(std::string name) noexcept;
	virtual ScriptVariableContainer& try_link(ScriptVariableContainer& container) noexcept;

public:
	std::map<std::string, ScriptVariable, std::less<>> store;

public:
	void update(const ScriptIdentifier& identifier, const ScriptVariable& value);
	constexpr auto bindSetter()
	{
		return std::bind(&ScriptVariableStore::update, this, std::placeholders::_1, std::placeholders::_2);
	}
};

//---------------------------

using ScriptVariableFromServer = std::function<ScriptVariableContainer(const ScriptIdentifier&)>;
using ScriptVariableStorePicker = std::variant<ScriptVariableStore*, ScriptVariableFromServer>;
using ScriptVariableStoreMap = std::map<std::string, ScriptVariableStorePicker>;

/////////////////////////////
// ScriptContainer
/////////////////////////////

struct ScriptContainer
{
	ScriptEventQueue events;
	ScriptVariableStore variables;
};

////////////////////////////////////////////////////////////
// ScriptVariable Operators
////////////////////////////////////////////////////////////

inline ScriptVariable operator+(const ScriptVariable& left, const ScriptVariable& right)
{
	if (!std::holds_alternative<double>(left) || !std::holds_alternative<double>(right)) return left;
	return std::get<double>(left) + std::get<double>(right);
}

inline ScriptVariable operator-(const ScriptVariable& left, const ScriptVariable& right)
{
	if (!std::holds_alternative<double>(left) || !std::holds_alternative<double>(right)) return left;
	return std::get<double>(left) - std::get<double>(right);
}

inline ScriptVariable operator*(const ScriptVariable& left, const ScriptVariable& right)
{
	if (!std::holds_alternative<double>(left) || !std::holds_alternative<double>(right)) return left;
	return std::get<double>(left) * std::get<double>(right);
}

inline ScriptVariable operator/(const ScriptVariable& left, const ScriptVariable& right)
{
	if (!std::holds_alternative<double>(left) || !std::holds_alternative<double>(right)) return left;
	return std::get<double>(left) / std::get<double>(right);
}

inline ScriptVariable operator%(const ScriptVariable& left, const ScriptVariable& right)
{
	if (!std::holds_alternative<double>(left) || !std::holds_alternative<double>(right)) return left;
	return static_cast<double>(static_cast<int64_t>(std::get<double>(left)) % static_cast<int64_t>(std::get<double>(right)));
}

inline ScriptVariable operator-(const ScriptVariable& left)
{
	if (!std::holds_alternative<double>(left)) return left;
	return -std::get<double>(left);
}

inline ScriptVariable operator!(const ScriptVariable& left)
{
	if (!std::holds_alternative<double>(left)) return left;
	return (!std::get<double>(left)) ? 0.0 : 1.0;
}

inline std::partial_ordering operator<=>(const ScriptVariable& left, const ScriptVariable& right)
{
	if (!std::holds_alternative<double>(left)) return std::partial_ordering::unordered;
	if (!std::holds_alternative<double>(right)) return std::partial_ordering::unordered;
	return std::get<double>(left) <=> std::get<double>(right);
}

inline std::partial_ordering operator<=>(double left, const ScriptVariable& right)
{
	if (!std::holds_alternative<double>(right)) return std::partial_ordering::unordered;
	return left <=> std::get<double>(right);
}

inline std::partial_ordering operator<=>(const ScriptVariable& left, double right)
{
	if (!std::holds_alternative<double>(left)) return std::partial_ordering::unordered;
	return std::get<double>(left) <=> right;
}

inline bool operator!=(const ScriptVariable& left, const ScriptVariable& right)
{
	if (!std::holds_alternative<double>(left)) return false;
	if (!std::holds_alternative<double>(right)) return false;
	return std::get<double>(left) != std::get<double>(right);
}

////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////

std::optional<ScriptVariableContainer> retrieveVariableFromStore(const ScriptIdentifier& identifier, ScriptVariableStore* defaultStore, ScriptVariableStoreMap* variableStores = nullptr);
ScriptVariableContainer* getScriptVariableContainerUnsafe(std::any& anyval);
std::optional<ScriptVariableContainer> getScriptVariableContainer(const std::any& anyval);

std::string getIdentifierName(const ScriptIdentifier& identifier);

inline auto tryGetScriptVariableValueFromAny(const std::any& anyval, auto defaultValue) -> decltype(defaultValue)
{
	const auto* container = std::any_cast<const ScriptVariableContainer>(&anyval);
	if (container == nullptr) return defaultValue;
	return container->get<decltype(defaultValue)>();
}

inline auto getScriptVariableValue(const ScriptVariable& variable, auto defaultValue) -> decltype(defaultValue)
{
	using value_type = decltype(defaultValue);
	if (std::holds_alternative<value_type>(variable))
		return std::get<value_type>(variable);
	return defaultValue;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // SCRIPTCONTAINERS_H
