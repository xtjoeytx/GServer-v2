#include <algorithm>
#include <chrono>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
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
// Helper functions
////////////////////////////////////////////////////////////

clock::time_point helpers::currentFrameTime()
{
	static Server* server = nullptr;
	if (server == nullptr)
		server = BabyDI::Get<Server>();
	return server->getFrameStartTime();
}

////////////////////////////////////////////////////////////
// GameValue
////////////////////////////////////////////////////////////

GameValue& GameValue::operator=(const GameValue& other) noexcept
{
	if (this != &other)
	{
		m_boolean = other.m_boolean;
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
		m_boolean = std::move(other.m_boolean);
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
	if (m_boolean.has_value())
		return m_boolean.value();
	if (m_number.has_value())
		return !DoubleIsZero(m_number.value());
	return false;
}

GameValue GameValue::flatten(int64_t index) const noexcept
{
	if (m_array.has_value())
	{
		if (std::holds_alternative<std::vector<double>>(*m_array))
		{
			const auto& arr = std::get<std::vector<double>>(*m_array);
			if (index >= 0 && index < (int64_t)arr.size())
				return GameValue{arr[index]};
			else if (!arr.empty())
				return GameValue{arr[0]};
			else
				return GameValue{0.0};
		}
		else if (std::holds_alternative<std::vector<ScriptObject>>(*m_array))
		{
			const auto& arr = std::get<std::vector<ScriptObject>>(*m_array);
			if (index >= 0 && index < (int64_t)arr.size())
				return GameValue{arr[index]};
			else if (!arr.empty())
				return GameValue{arr[0]};
			else
				return GameValue{ScriptObject{}};
		}
	}

	return 0.0;
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

std::optional<GameVariable> GameVariable::deserialize(const std::string_view line)
{
	if (line.starts_with("FLAG"))
	{
		auto data = string::trim(line.substr(5));
		auto separator = data.find('=');
		if (separator == std::string_view::npos)
			return GameVariable{.name = std::string{string::trim(data)}, .value = true};
		return GameVariable{.name = std::string{string::trim(data.substr(0, separator))}, .value = std::string{string::trim(data.substr(separator + 1))}};
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
			return GameVariable{.name = std::string{identifier}, .value = string::toDouble(std::string{value})};

		std::vector<double> array;
		for (std::string_view number : string::split(value.substr(1, value.length() - 2), ","sv))
			array.emplace_back(string::toDouble(std::string{number}));
		return GameVariable{.name = std::string{identifier}, .value = std::move(array)};
	}

	return std::nullopt;
}

std::optional<std::string> GameVariable::serializeModern(std::string_view name) const noexcept
{
	if (value.has<bool>() && !value.has<std::string>() && value.getCopy<bool>().value_or(false) == true)
		return std::string{name};
	if (value.has<std::string>())
		return std::format("{}={}", name, value.get<std::string>()->get());
	return std::nullopt;
}

////////////////////////////////////////////////////////////
// GameVariableStore
////////////////////////////////////////////////////////////

std::weak_ptr<GameVariable> GameVariableStore::add(std::string_view name, GameValue&& value) noexcept
{
	if (staticContainer)
		return {};

	auto var = std::make_shared<GameVariable>(GameVariable{.name{name}, .value{std::move(value)}, .lifetime{defaultLifetime}});
	auto [iter, was_inserted] = store.insert_or_assign(var->name, var);
	return iter->second;
}

std::weak_ptr<GameVariable> GameVariableStore::add(GameVariable&& variable) noexcept
{
	if (staticContainer || variable.name.empty())
		return {};

	auto var = std::make_shared<GameVariable>(std::move(variable));
	if (!var->lifetime.has_value())
		var->lifetime = defaultLifetime;

	auto [iter, was_inserted] = store.insert_or_assign(var->name, var);
	return iter->second;
}

bool GameVariableStore::remove(std::string_view name) noexcept
{
	if (staticContainer || store.empty()) return false;
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

std::weak_ptr<GameVariable> GameVariableStore::getOrAdd(std::string_view name) noexcept
{
	if (!store.empty())
	{
		auto it = store.find(name);
		if (it != store.end())
			return it->second;
	}
	return add(std::move(name), GameValue{0.0});
}

void GameVariableStore::clearTemporary() noexcept
{
	if (staticContainer || store.empty()) return;
	std::erase_if(store, [](const auto& pair)
	{
		return pair.second->lifetime == variables::Lifetime::TEMPORARY;
	});
}

void GameVariableStore::clearTemporary(std::string_view prefix) noexcept
{
	if (staticContainer || store.empty()) return;
	std::erase_if(store, [prefix](const auto& pair)
	{
		return pair.second->lifetime == variables::Lifetime::TEMPORARY && pair.first.starts_with(prefix);
	});
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
		if (variable->value.has<bool>() && !variable->value.has<std::string>() && variable->getCopy<bool>().value_or(false))
			results.emplace_back(std::format("FLAG {}", name));
		else if (variable->value.has<std::string>())
			results.emplace_back(std::format("FLAG {}={}", name, variable->serialize<std::string>()));

		if (variable->value.has<double>())
			results.emplace_back(std::format("VAR {}={}", name, variable->serialize<double>()));
		if (variable->value.has<std::vector<double>>())
			results.emplace_back(std::format("VAR {}={}", name, variable->serialize<std::vector<double>>()));
	}

	return results;
}

////////////////////////////////////////////////////////////
// ScriptEventQueue
////////////////////////////////////////////////////////////

bool ScriptEventQueue::hasEvent(ScriptEventType type, ScriptObject initiator)
{
	return std::ranges::find_if(m_eventQueue, [type, initiator](ScriptEvent& event)
	{
		return event.type == type && event.initiator == initiator && event.args.size() == 0;
	}) != m_eventQueue.end();
}

void ScriptEventQueue::addEvent(ScriptEventType type, ScriptObject initiator)
{
	if (hasEvent(type, initiator))
		return;

	if (auto* server = BabyDI::Get<Server>(); server != nullptr && server->hasNPCServer())
		m_eventQueue.push_back(std::move(ScriptEvent{.type = type, .initiator = initiator}));
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
