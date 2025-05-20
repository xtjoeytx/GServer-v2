#ifndef SCRIPTCONTAINERS_H
#define SCRIPTCONTAINERS_H

#include <queue>
#include <any>
#include <variant>

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

/////////////////////////////

struct ScriptEvent
{
	ScriptEventType type;
	ScriptEventSource source;
	std::vector<std::any> args;
};

// Queued script events.
class ScriptEventQueue
{
public:
	[[inline]] std::queue<ScriptEvent>& queue();

public:
	void addEvent(ScriptEventType type, ScriptEventSource source)
	{
		m_eventQueue.push(std::move(ScriptEvent{ .type = type, .source = source }));
	}

	template<class T>
	void addEvent(ScriptEventType type, ScriptEventSource source, T&& arg)
	{
		ScriptEvent event{ .type = type, .source = source, .args = { std::make_any<T>(std::forward<T>(arg)) } };
		addEvent(event);
	}

	template<class T, class... Args>
	void addEvent(ScriptEventType type, ScriptEventSource source, T&& arg, Args&&... args)
	{
		ScriptEvent event{ .type = type, .source = source, .args = { std::make_any<T>(std::forward<T>(arg)) } };
		addEvent(event, std::forward<Args>(args)...);
	}

private:
	void addEvent(ScriptEvent& event)
	{
		m_eventQueue.push(std::move(event));
	}

	template<class T>
	void addEvent(ScriptEvent& event, T&& arg)
	{
		event.args.push_back(std::forward<T>(arg));
		addEvent(event);
	}

	template<class T, class... Args>
	void addEvent(ScriptEvent& event, T&& arg, Args&&... args)
	{
		event.args.push_back(std::forward<T>(arg));
		addEvent(event, std::forward<Args>(args)...);
	}

private:
	std::queue<ScriptEvent> m_eventQueue;
};

inline std::queue<ScriptEvent>& ScriptEventQueue::queue()
{
	return m_eventQueue;
}

/////////////////////////////

using ScriptVariable = std::variant<double, std::string, std::vector<double>>;

struct ScriptVariableStore
{
public:
	[[inline]] ScriptVariable& add(std::string_view name, ScriptVariable value) noexcept;
	[[inline]] bool remove(std::string_view name) noexcept;
	[[inline]] bool contains(std::string_view name) const noexcept;
	[[inline]] ScriptVariable* get(std::string_view name) noexcept;
	[[inline]] const ScriptVariable* get(std::string_view name) const noexcept;

public:
	// Keep as a std::map as iterators/references are not invalidated when the map changes (like on insert or erase).
	// If it must be changed, a refactor of the scripting languages are needed.
	std::map<std::string, ScriptVariable, std::less<>> store;
};

inline ScriptVariable& ScriptVariableStore::add(std::string_view name, ScriptVariable value) noexcept
{
	auto [iter, was_inserted] = store.insert_or_assign(std::string{ name }, value);

	// https://en.cppreference.com/w/cpp/container/map/insert_or_assign
	// No iterators or references are invalidated.
	return *&iter->second;
}

inline bool ScriptVariableStore::remove(std::string_view name) noexcept
{
	return store.erase(name) != 0;
}

inline bool ScriptVariableStore::contains(std::string_view name) const noexcept
{
	return store.contains(name);
}

inline ScriptVariable* ScriptVariableStore::get(std::string_view name) noexcept
{
	auto it = store.find(name);
	if (it == store.end())
		return nullptr;

	return &it->second;
}

inline const ScriptVariable* ScriptVariableStore::get(std::string_view name) const noexcept
{
	auto it = store.find(name);
	if (it == store.end())
		return nullptr;

	return &it->second;
}

/////////////////////////////

using ScriptVariablePair = std::pair<ScriptVariable, ScriptVariable>;

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

/////////////////////////////

inline ScriptVariable* getScriptVariableUnsafe(std::any& anyval)
{
	auto* direct = std::any_cast<ScriptVariable>(&anyval);
	if (direct != nullptr) return direct;
	auto** indirect = std::any_cast<ScriptVariable*>(&anyval);
	if (indirect != nullptr && *indirect != nullptr) return *indirect;
	return nullptr;
}

inline const ScriptVariable* getScriptVariableUnsafe(const std::any& anyval)
{
	const auto* direct = std::any_cast<ScriptVariable>(&anyval);
	if (direct != nullptr) return direct;
	auto* const* indirect = std::any_cast<ScriptVariable*>(&anyval);
	if (indirect != nullptr && *indirect != nullptr) return *indirect;
	return nullptr;
}

inline ScriptVariable& getScriptVariableOr(std::any& anyval, ScriptVariable& defaultValue)
{
	auto* direct = std::any_cast<ScriptVariable>(&anyval);
	if (direct != nullptr) return *direct;
	auto** indirect = std::any_cast<ScriptVariable*>(&anyval);
	if (indirect != nullptr && *indirect != nullptr) return **indirect;
	return defaultValue;
}

///

template<class T = ScriptVariable>
inline std::optional<T> getScriptVariable(std::any& anyval)
{
	auto* variable = getScriptVariableUnsafe(anyval);
	if (variable == nullptr)
		return {};
	if constexpr (std::same_as<T, ScriptVariable>)
	{
		return *variable;
	}
	else
	{
		if (std::holds_alternative<T>(*variable))
			return std::get<T>(*variable);
	}
	return {};
}

template<>
inline std::optional<bool> getScriptVariable<bool>(std::any& anyval)
{
	auto double_value = getScriptVariable<double>(anyval);
	if (!double_value.has_value())
		return {};
	return double_value.value() != 0.0f;
}

///

template<class T = ScriptVariable>
inline const std::optional<T> getScriptVariable(const std::any& anyval)
{
	const auto* variable = getScriptVariableUnsafe(anyval);
	if (variable == nullptr)
		return {};
	if constexpr (std::same_as<T, ScriptVariable>)
	{
		return *variable;
	}
	else
	{
		if (std::holds_alternative<T>(*variable))
			return std::get<T>(*variable);
	}
	return {};
}

template<>
inline const std::optional<bool> getScriptVariable<bool>(const std::any& anyval)
{
	const auto double_value = getScriptVariable<double>(anyval);
	if (!double_value.has_value())
		return {};
	return double_value.value() != 0.0f;
}

/////////////////////////////

struct ScriptContainer
{
	ScriptEventQueue events;
	ScriptVariableStore variables;
};

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal

#endif // SCRIPTCONTAINERS_H
