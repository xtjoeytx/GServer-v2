#include <cstdint>
#include <functional>
#include <optional>
#include <vector>
#include <string>
#include <string_view>

#include <Server.h>
#include <level/Level.h>
#include <scripting/ScriptContainers.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

ScriptObjectSource FromLevel(LevelPtr level)
{
	return std::make_pair(string::string_hash{}(level->getLevelName()), ScriptObjectSourceType::LEVEL);
}

////////////////////////////////////////////////////////////
// GameValue
////////////////////////////////////////////////////////////

GameValue& GameValue::operator=(const GameValue& other) noexcept
{
	if (this != &other)
	{
		m_number = other.m_number;
		m_text = other.m_text;
		m_array = other.m_array;
	}
	return *this;
}

GameValue& GameValue::operator=(GameValue&& other) noexcept
{
	if (this != &other)
	{
		m_number = std::move(other.m_number);
		m_text = std::move(other.m_text);
		m_array = std::move(other.m_array);
	}
	return *this;
}

bool GameValue::operator==(const GameValue& other) noexcept
{
	return (bool)*this == (bool)other;
}

GameValue::operator bool() const
{
	if (m_number.has_value())
		return m_number.value() != 0.0;
	return false;
}

////////////////////////////////////////////////////////////
// GameVariable
////////////////////////////////////////////////////////////

GameVariable::operator double() const
{
	auto* value = game_value().get_unsafe<double>();
	return (value != nullptr) ? *value : 0.0;
}

GameVariable::operator std::string() const
{
	auto* value = game_value().get_unsafe<std::string>();
	return (value != nullptr) ? *value : std::string{};
}

GameVariable& GameVariable::operator=(const GameVariable& other)
{
	if (this != &other)
	{
		identifier = other.identifier;
		m_value = other.m_value;
		m_getter = other.m_getter;
		m_setter = other.m_setter;
	}
	return *this;
}

GameVariable& GameVariable::operator=(GameVariable&& other) noexcept
{
	if (this != &other)
	{
		identifier = std::move(other.identifier);
		m_value = std::move(other.m_value);
		m_getter = std::move(other.m_getter);
		m_setter = std::move(other.m_setter);
	}
	return *this;
}

//----------------------------

void GameVariable::setCallbacks(func_get getter, func_set setter)
{
	m_getter = std::move(getter);
	m_setter = std::move(setter);
}

GameValue& GameVariable::get_underlying()
{
	return game_value();
}

const GameValue& GameVariable::get_underlying() const
{
	return game_value();
}

//----------------------------

GameValue& GameVariable::game_value()
{
	if (!m_getter) [[likely]]
		return m_value;
	m_value = std::move(m_getter(identifier));
	return m_value;
}

const GameValue& GameVariable::game_value() const
{
	if (!m_getter) [[likely]]
		return m_value;
	m_value = std::move(m_getter(identifier));
	return m_value;
}

////////////////////////////////////////////////////////////
// GameVariableStore
////////////////////////////////////////////////////////////

std::weak_ptr<GameVariable> GameVariableStore::add(std::string_view name, GameValue&& value) noexcept
{
	auto var = std::make_shared<GameVariable>(std::string{ name }, GameValue{ std::move(value) });
	auto [iter, was_inserted] = store.insert_or_assign(var->identifier, var);
	return iter->second;
}

std::weak_ptr<GameVariable> GameVariableStore::add(GameVariable&& variable) noexcept
{
	auto var = std::make_shared<GameVariable>(std::move(variable));
	auto [iter, was_inserted] = store.insert_or_assign(var->identifier, var);
	return iter->second;
}

bool GameVariableStore::remove(std::string_view name) noexcept
{
	if (store.empty()) return false;
	auto it = store.find(name);
	if (it == store.end()) return false;
	store.erase(it);
	return true;
}

bool GameVariableStore::contains(std::string_view name) const noexcept
{
	if (store.empty()) return false;
	return store.contains(name);
}

std::weak_ptr<GameVariable> GameVariableStore::get(std::string_view name) noexcept
{
	if (store.empty()) return {};
	auto it = store.find(name);
	if (it == store.end()) return {};
	return it->second;
}

const std::weak_ptr<GameVariable> GameVariableStore::get(std::string_view name) const noexcept
{
	if (store.empty()) return {};
	auto it = store.find(name);
	if (it == store.end()) return {};
	return it->second;
}

std::weak_ptr<GameVariable> GameVariableStore::get_or_add(std::string_view name) noexcept
{
	if (!store.empty())
	{
		auto it = store.find(name);
		if (it != store.end()) return it->second;
	}
	return add(std::move(name), GameValue{ 0.0 });
}

GameVariableVariant GameVariableStore::get_or_stub(std::string_view name) noexcept
{
	if (!store.empty())
	{
		auto it = store.find(name);
		if (it != store.end())
			return std::weak_ptr<GameVariable>(it->second);
	}
	return GameVariable(std::string{ name }, GameValue{ 0.0 }, nullptr, std::bind(&GameVariableStore::stub_new, this, std::placeholders::_1, std::placeholders::_2));
}

void GameVariableStore::stub_new(GameVariable& variable, const GameValue& value)
{
	auto new_variable = add(variable.identifier, GameValue{ value });
	variable.setCallbacks(variable.getCallbackGetter(), {});
}

////////////////////////////////////////////////////////////
// ScriptEventQueue
////////////////////////////////////////////////////////////

void ScriptEventQueue::addEvent(ScriptEventType type, ScriptObjectSource initiator)
{
	m_eventQueue.push(std::move(ScriptEvent{ .type = type, .initiator = initiator }));
}

void ScriptEventQueue::addEvent(const ScriptEvent& event)
{
	m_eventQueue.push(event);
}

void ScriptEventQueue::addEvent(ScriptEvent&& event)
{
	m_eventQueue.push(std::move(event));
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
