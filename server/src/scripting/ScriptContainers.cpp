#include <format>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

#include <BabyDI.h>
#include <Server.h>
#include <level/Level.h>
#include <object/Weapon.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

namespace source
{
ScriptObjectSource FromWeapon(WeaponPtr weapon)
{
	size_t hash = string::string_hash{}(weapon->name);
	return std::make_pair(hash, ScriptObjectSourceType::WEAPON);
}
ScriptObjectSource FromLevel(LevelPtr level)
{
	size_t hash = string::string_hash{}(level->getLevelName());
	return std::make_pair(hash, ScriptObjectSourceType::LEVEL);
}
} // end namespace source

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
		m_boolean = other.m_boolean;
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
		m_boolean = std::move(other.m_boolean);
	}
	return *this;
}

bool GameValue::operator==(const GameValue& other) noexcept
{
	return (bool)*this == (bool)other;
}

GameValue::operator bool() const
{
	if (m_boolean.has_value())
		return m_boolean.value();
	if (m_number.has_value())
		return !DoubleIsZero(m_number.value());
	return false;
}

bool GameValue::testAsFlag() const
{
	if (m_boolean.has_value())
		return m_boolean.value();
	if (m_text.has_value())
		return !m_text.value().empty();
	return false;
}

////////////////////////////////////////////////////////////
// GameVariable
////////////////////////////////////////////////////////////

GameVariable::operator double() const
{
	auto* value = m_value.get_unsafe<double>();
	return (value != nullptr) ? *value : 0.0;
}

GameVariable::operator std::string() const
{
	auto* value = m_value.get_unsafe<std::string>();
	return (value != nullptr) ? *value : std::string{};
}

GameVariable::operator bool() const
{
	return (bool)m_value;
}

GameVariable& GameVariable::operator=(const GameVariable& other)
{
	if (this != &other)
	{
		identifier = other.identifier;
		temporary = other.temporary;
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
		temporary = other.temporary;
		m_value = std::move(other.m_value);
		m_getter = std::move(other.m_getter);
		m_setter = std::move(other.m_setter);
	}
	return *this;
}

bool GameVariable::testAsFlag() const
{
	return m_value.testAsFlag();
}

GameVariable& GameVariable::update()
{
	m_value = game_value();
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
	return m_value;
}

const GameValue& GameVariable::get_underlying() const
{
	return m_value;
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

//----------------------------

std::optional<GameVariable> GameVariable::deserialize(std::string_view line)
{
	if (line.starts_with("FLAG"))
	{
		auto data = string::trim(line.substr(5));
		auto separator = data.find('=');
		if (separator == std::string_view::npos)
			return GameVariable{ std::string{ string::trim(data) }, true };
		return GameVariable{ std::string{ string::trim(data.substr(0, separator)) }, std::string{ string::trim(data.substr(separator + 1)) } };
	}
	else if (line.starts_with("VAR"))
	{
		auto data = string::trim(line.substr(4));
		auto separator = data.find('=');
		if (separator == std::string_view::npos)
			return std::nullopt;

		auto identifier = string::trim(data.substr(0, separator));
		auto value = string::trim(data.substr(separator + 1));
		if (value.empty())
			return std::nullopt;
		if (value[0] != '{')
			return GameVariable{ std::string{ identifier }, string::toDouble(std::string{ value }) };

		std::vector<double> array;
		for (auto& number : string::splitHard(value.substr(1, value.length() - 2), ","sv))
			array.emplace_back(string::toDouble(number));
		return GameVariable{ std::string{ identifier }, std::move(array) };
	}

	return std::nullopt;
}

std::optional<std::string> GameVariable::serializeModern(std::string_view name) const noexcept
{
	auto& value = game_value();
	auto* boolVal = value.get_unsafe<bool>();
	auto* stringVal = value.get_unsafe<std::string>();
	if (boolVal != nullptr && stringVal == nullptr && *boolVal == true)
		return std::string{ name };
	if (stringVal != nullptr)
		return std::format("{}={}", name, *stringVal);
	return std::nullopt;
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
	it->second->update();
	return it->second;
}

const std::weak_ptr<GameVariable> GameVariableStore::get(std::string_view name) const noexcept
{
	if (store.empty()) return {};
	auto it = store.find(name);
	if (it == store.end()) return {};
	it->second->update();
	return it->second;
}

std::weak_ptr<GameVariable> GameVariableStore::get_or_add(std::string_view name) noexcept
{
	if (!store.empty())
	{
		auto it = store.find(name);
		if (it != store.end())
		{
			it->second->update();
			return it->second;
		}
	}
	return add(std::move(name), GameValue{ 0.0 });
}

GameVariableVariant GameVariableStore::get_or_stub(std::string_view name) noexcept
{
	if (!store.empty())
	{
		auto it = store.find(name);
		if (it != store.end())
		{
			it->second->update();
			return std::weak_ptr<GameVariable>(it->second);
		}
	}
	return GameVariable(std::string{ name }, GameValue{ 0.0 }, nullptr, std::bind(&GameVariableStore::stub_new, this, std::placeholders::_1, std::placeholders::_2));
}

void GameVariableStore::stub_new(GameVariable& variable, const GameValue& value)
{
	auto new_variable = add(variable.identifier, GameValue{ value });
	variable.setCallbacks(variable.getCallbackGetter(), {});
}

void GameVariableStore::clearTemporary() noexcept
{
	if (store.empty()) return;
	std::erase_if(store, [](const auto& pair) { return pair.second->temporary; });
}

std::optional<std::string> GameVariableStore::serializeModern(std::string_view name) const noexcept
{
	auto var = get(name);
	if (auto variable = var.lock(); variable != nullptr)
		return variable->serializeModern(name);

	return std::nullopt;
}

std::vector<std::string> GameVariableStore::serialize(std::string_view name) const noexcept
{
	std::vector<std::string> results;
	auto var = get(name);
	if (auto variable = var.lock(); variable != nullptr)
	{
		if (variable->has<bool>() && !variable->has<std::string>() && variable->get<bool>().value_or(false))
			results.emplace_back(std::format("FLAG {}", name));
		else if (variable->has<std::string>())
			results.emplace_back(std::format("FLAG {}={}", name, variable->serialize<std::string>()));

		if (variable->has<double>())
			results.emplace_back(std::format("VAR {}={}", name, variable->serialize<double>()));
		if (variable->has<std::vector<double>>())
			results.emplace_back(std::format("VAR {}={}", name, variable->serialize<std::vector<double>>()));
	}

	return results;
}

////////////////////////////////////////////////////////////
// ScriptEventQueue
////////////////////////////////////////////////////////////

void ScriptEventQueue::addEvent(ScriptEventType type, ScriptObjectSource initiator)
{
	auto* server = BabyDI::Get<Server>();
	if (server != nullptr && server->hasNPCServer())
		m_eventQueue.push(std::move(ScriptEvent{ .type = type, .initiator = initiator }));
}

void ScriptEventQueue::addEvent(const ScriptEvent& event)
{
	auto* server = BabyDI::Get<Server>();
	if (server != nullptr && server->hasNPCServer())
		m_eventQueue.push(event);
}

void ScriptEventQueue::addEvent(ScriptEvent&& event)
{
	auto* server = BabyDI::Get<Server>();
	if (server != nullptr && server->hasNPCServer())
		m_eventQueue.push(std::move(event));
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
