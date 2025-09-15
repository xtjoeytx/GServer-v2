#include <algorithm>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <BabyDI.h>
#include <Server.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// GameValue
////////////////////////////////////////////////////////////

GameValue& GameValue::operator=(const GameValue& other) noexcept
{
	if (this != &other)
	{
		identifier = other.identifier;
		temporary = other.temporary;
		m_number = other.m_number;
		m_text = other.m_text;
		m_array = other.m_array;
		m_boolean = other.m_boolean;
		m_source = other.m_source;
		m_getter = other.m_getter;
		m_setter = other.m_setter;
	}
	return *this;
}

GameValue& GameValue::operator=(GameValue&& other) noexcept
{
	if (this != &other)
	{
		identifier = std::move(other.identifier);
		temporary = other.temporary;
		m_number = std::move(other.m_number);
		m_text = std::move(other.m_text);
		m_array = std::move(other.m_array);
		m_boolean = std::move(other.m_boolean);
		m_source = std::move(other.m_source);
		m_getter = other.m_getter;
		m_setter = other.m_setter;
	}
	return *this;
}

bool GameValue::operator==(const GameValue& other) noexcept
{
	return (bool)*this == (bool)other;
}

GameValue::operator bool() const
{
	if (m_getter)
	{
		std::optional<bool> boolval;
		if (m_getter(&boolval, std::nullopt); boolval.has_value())
			return *boolval;

		std::optional<double> doubleval;
		if (m_getter(&doubleval, std::nullopt); doubleval.has_value())
			return !DoubleIsZero(*doubleval);
	}
	if (m_boolean.has_value())
		return m_boolean.value();
	if (m_number.has_value())
		return !DoubleIsZero(m_number.value());
	return false;
}

GameValue GameValue::flatten(size_t index) const noexcept
{
	if (m_getter)
	{
		std::optional<double> number;
		m_getter(&number, index);
		if (number.has_value())
			return *number;

		std::optional<std::vector<ScriptObject>> object;
		m_getter(&object, index);
		if (object.has_value())
			return *object;
	}
	if (m_array.has_value() && index < m_array->size())
		return (*m_array)[index];
	if (m_source.has_value() && index < m_source->size())
		return (*m_source)[index];

	return 0.0;
}

bool GameValue::testAsFlag() const
{
	if (m_getter)
	{
		std::optional<bool> boolval;
		m_getter(&boolval, std::nullopt);
		if (boolval.has_value())
			return *boolval;

		std::optional<std::string> stringval;
		m_getter(&stringval, std::nullopt);
		return (stringval.has_value() && !stringval->empty());
	}
	if (m_boolean.has_value())
		return m_boolean.value();
	if (m_text.has_value())
		return !m_text.value().empty();
	return false;
}

//----------------------------

std::optional<GameValue> GameValue::deserialize(const std::string_view line)
{
	if (line.starts_with("FLAG"))
	{
		auto data = string::trim(line.substr(5));
		auto separator = data.find('=');
		if (separator == std::string_view::npos)
			return GameValue{ std::string{ string::trim(data) }, true };
		return GameValue{ std::string{ string::trim(data.substr(0, separator)) }, std::string{ string::trim(data.substr(separator + 1)) } };
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
			return GameValue{ std::string{ identifier }, string::toDouble(std::string{ value }) };

		std::vector<double> array;
		for (auto& number : string::splitHard(value.substr(1, value.length() - 2), ","sv))
			array.emplace_back(string::toDouble(number));
		return GameValue{ std::string{ identifier }, std::move(array) };
	}

	return std::nullopt;
}

std::optional<std::string> GameValue::serializeModern(std::string_view name) const noexcept
{
	if (m_boolean.has_value() && !m_text.has_value() && m_boolean.value_or(false) == true)
		return std::string{ name };
	if (m_text.has_value())
		return std::format("{}={}", name, m_text.value_or(""s));
	return std::nullopt;
}


////////////////////////////////////////////////////////////
// GameVariableStore
////////////////////////////////////////////////////////////

std::weak_ptr<GameValue> GameVariableStore::add(std::string_view name, GameValue&& value) noexcept
{
	auto var = std::make_shared<GameValue>(std::move(value));
	var->identifier = name;
	auto [iter, was_inserted] = store.insert_or_assign(var->identifier, var);
	return iter->second;
}

std::weak_ptr<GameValue> GameVariableStore::add(GameValue&& variable) noexcept
{
	if (variable.identifier.empty())
		return {};
	auto var = std::make_shared<GameValue>(std::move(variable));
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

std::weak_ptr<GameValue> GameVariableStore::get(std::string_view name) noexcept
{
	if (store.empty()) return {};
	auto it = store.find(name);
	if (it == store.end()) return {};
	return it->second;
}

const std::weak_ptr<GameValue> GameVariableStore::get(std::string_view name) const noexcept
{
	if (store.empty()) return {};
	auto it = store.find(name);
	if (it == store.end()) return {};
	return it->second;
}

std::weak_ptr<GameValue> GameVariableStore::getOrAdd(std::string_view name) noexcept
{
	if (!store.empty())
	{
		auto it = store.find(name);
		if (it != store.end())
			return it->second;
	}
	return add(std::move(name), GameValue{ 0.0 });
}

GameValue GameVariableStore::getOrStub(std::string_view name)
{
	if (auto var = getOrAdd(name).lock(); var != nullptr)
	{
		auto getter = [this, variable = var](GameValueVariant incoming, std::optional<size_t> index)
		{
			const auto picker = visit_functions
			{
				[&](std::optional<bool>* ptr) { *ptr = variable->get<bool>(index).value_or(false); },
				[&](std::optional<double>* ptr) { *ptr = variable->get<double>(index).value_or(0.0); },
				[&](std::optional<std::string>* ptr) { *ptr = variable->get<std::string>(index).value_or(""s); },
				[&](std::optional<std::vector<double>>* ptr) { *ptr = variable->get<std::vector<double>>(index).value_or(std::vector<double>{}); },
				[&](std::optional<std::vector<ScriptObject>>* ptr) { *ptr = variable->get<std::vector<ScriptObject>>(index).value_or(std::vector<ScriptObject>{}); }
			};
			std::visit(picker, incoming);
		};

		auto setter = [this, variable = var](GameValueVariant incoming, std::optional<size_t> index)
		{
			const auto picker = visit_functions
			{
				[&](std::optional<bool>* ptr) { variable->assign<bool>(ptr->value_or(false), index); },
				[&](std::optional<double>* ptr) { variable->assign<double>(ptr->value_or(0.0), index); },
				[&](std::optional<std::string>* ptr) { variable->assign<std::string>(ptr->value_or(""s), index); },
				[&](std::optional<std::vector<double>>* ptr) { variable->assign<std::vector<double>>(ptr->value_or(std::vector<double>{}), index); },
				[&](std::optional<std::vector<ScriptObject>>* ptr) { variable->assign<std::vector<ScriptObject>>(ptr->value_or(std::vector<ScriptObject>{}), index); }
			};
			std::visit(picker, incoming);
		};

		return GameValue{ name, getter, setter };
	}
	throw std::runtime_error("Failed to create variable stub.");
}

void GameVariableStore::clearTemporary() noexcept
{
	if (store.empty()) return;
	std::erase_if(store, [](const auto& pair) { return pair.second->temporary; });
}

void GameVariableStore::clearTemporary(std::string_view prefix) noexcept
{
	if (store.empty()) return;
	std::erase_if(store, [prefix](const auto& pair) { return pair.second->temporary && pair.first.starts_with(prefix); });
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

bool ScriptEventQueue::hasEvent(ScriptEventType type, ScriptObject initiator)
{
	return std::ranges::find_if(m_eventQueue,
		[type, initiator](ScriptEvent& event)
		{
			return event.type == type && event.initiator == initiator && event.args.size() == 0;
		}) != m_eventQueue.end();
}

void ScriptEventQueue::addEvent(ScriptEventType type, ScriptObject initiator)
{
	if (hasEvent(type, initiator))
		return;
	
	if (auto* server = BabyDI::Get<Server>(); server != nullptr && server->hasNPCServer())
		m_eventQueue.push_back(std::move(ScriptEvent{ .type = type, .initiator = initiator }));
}

void ScriptEventQueue::addEvent(const ScriptEvent& event)
{
	if (hasEvent(event.type, event.initiator))
		return;

	if (auto* server = BabyDI::Get<Server>(); server != nullptr && server->hasNPCServer())
		m_eventQueue.push_back(event);
}

void ScriptEventQueue::addEvent(ScriptEvent&& event)
{
	if (hasEvent(event.type, event.initiator))
		return;

	if (auto* server = BabyDI::Get<Server>(); server != nullptr && server->hasNPCServer())
		m_eventQueue.push_back(std::move(event));
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
