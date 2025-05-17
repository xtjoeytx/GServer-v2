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

constexpr ScriptEventSource FromPlayer(PlayerID id)
{
	return std::make_pair(id, ScriptEventSourceType::PLAYER);
}

constexpr ScriptEventSource FromNPC(NPCID id)
{
	return std::make_pair(id, ScriptEventSourceType::NPC);
}

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
	[[inline]] void add(std::string_view name, ScriptVariable value) noexcept;
	[[inline]] bool remove(std::string_view name) noexcept;
	[[inline]] bool contains(std::string_view name) const noexcept;
	[[inline]] ScriptVariable* get(std::string_view name) noexcept;
	[[inline]] const ScriptVariable* get(std::string_view name) const noexcept;

public:
	std::unordered_map<std::string, ScriptVariable, string::string_hash, std::equal_to<>> store;
};

inline void ScriptVariableStore::add(std::string_view name, ScriptVariable value) noexcept
{
	store.insert_or_assign(std::string{ name }, value);
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

struct ScriptContainer
{
	ScriptEventQueue events;
	ScriptVariableStore variables;
};

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal

#endif // SCRIPTCONTAINERS_H
