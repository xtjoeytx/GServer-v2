#include <scripting/ScriptContainers.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// ScriptEventQueue
////////////////////////////////////////////////////////////

void ScriptEventQueue::addEvent(ScriptEventType type, ScriptEventSource initiator)
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

////////////////////////////////////////////////////////////
// ScriptVariableContainer
////////////////////////////////////////////////////////////

// Sets the ScriptVariable to a new value.
ScriptVariableContainer& ScriptVariableContainer::set(const ScriptVariable& value) noexcept
{
	m_value = value;
	if (m_setter && m_identifier.has_value())
		m_setter(m_identifier.value(), m_value);
	return *this;
}

// Sets the ScriptVariable to a new value.
ScriptVariableContainer& ScriptVariableContainer::set(ScriptVariable&& value) noexcept
{
	m_value = std::move(value);
	if (m_setter && m_identifier.has_value())
		m_setter(m_identifier.value(), m_value);
	return *this;
}

// Sets the ScriptVariable to a new value.
ScriptVariableContainer& ScriptVariableContainer::set(const ScriptVariableContainer& value) noexcept
{
	m_value = value.get();
	if (m_setter && m_identifier.has_value())
		m_setter(m_identifier.value(), m_value);
	return *this;
}

// Sets the ScriptVariable to a new value.
ScriptVariableContainer& ScriptVariableContainer::set(ScriptVariableContainer&& value) noexcept
{
	m_value = std::move(value.get());
	if (m_setter && m_identifier.has_value())
		m_setter(m_identifier.value(), m_value);
	return *this;
}

////////////////////////////////////////////////////////////
// ScriptVariableStore
////////////////////////////////////////////////////////////


ScriptVariableContainer ScriptVariableStore::add(std::string name, ScriptVariable value) noexcept
{
	auto [iter, was_inserted] = store.insert_or_assign(name, value);
	return ScriptVariableContainer(ScriptIdentifier{ std::move(name) }, iter->second, bindSetter());
}

bool ScriptVariableStore::remove(std::string_view name) noexcept
{
	return store.erase(name) != 0;
}

bool ScriptVariableStore::contains(std::string_view name) const noexcept
{
	return store.contains(name);
}

std::optional<ScriptVariableContainer> ScriptVariableStore::get(std::string name) noexcept
{
	auto it = store.find(name);
	if (it == store.end())
		return std::nullopt;

	return ScriptVariableContainer(ScriptIdentifier{ std::move(name) }, it->second, bindSetter());
}

std::optional<const ScriptVariableContainer> ScriptVariableStore::get(std::string name) const noexcept
{
	auto it = store.find(name);
	if (it == store.end())
		return std::nullopt;

	return ScriptVariableContainer{ ScriptIdentifier{ std::move(name) }, it->second };
}

ScriptVariableContainer ScriptVariableStore::get_or_add(std::string name) noexcept
{
	auto it = store.find(name);
	if (it != store.end())
		return ScriptVariableContainer(ScriptIdentifier{ std::move(name) }, it->second, bindSetter());

	return add(name, { 0.0 });
}

ScriptVariableContainer& ScriptVariableStore::try_link(ScriptVariableContainer& container) noexcept
{
	if (!container.hasIdentifier())
		return container;

	const auto& identifier = container.getIdentifier().value();
	if (store.contains(getIdentifierName(identifier)))
		container.setSetter(bindSetter());

	return container;
}

void ScriptVariableStore::update(const ScriptIdentifier& identifier, const ScriptVariable& value)
{
	std::string_view identifierName;
	std::optional<size_t> index = std::nullopt;

	if (std::holds_alternative<std::string>(identifier))
		identifierName = std::get<std::string>(identifier);
	else if (std::holds_alternative<std::pair<std::string, size_t>>(identifier))
	{
		auto& pair = std::get<std::pair<std::string, size_t>>(identifier);
		identifierName = pair.first;
		index = pair.second;
	}
	else throw std::exception("ScriptVariableStore::update received an invalid identifier");

	auto it = store.find(identifierName);
	if (it == store.end())
	{
		add(std::string{ identifierName }, value);
		return;
	}

	// If we don't have an index, we need to set the value directly.
	if (!index.has_value())
	{
		it->second = value;
	}
	// Update the record in the array.
	else
	{
		if (!std::holds_alternative<std::vector<double>>(it->second))
			throw std::exception("ScriptVariableStore::update received an indexed identifier, but the value does not contain an array");

		auto& array = std::get<std::vector<double>>(it->second);
		if (const auto* newValue = std::get_if<double>(&value))
		{
			array[index.value()] = *newValue;
		}
		else
		{
			array[index.value()] = 0.0;
		}
	}
}

////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////

std::optional<ScriptVariableContainer> retrieveVariableFromStore(const ScriptIdentifier& identifier, ScriptVariableStore* defaultStore, ScriptVariableStoreMap* variableStores)
{
	// Early out if we aren't saving variables anywhere.
	if (variableStores == nullptr && defaultStore == nullptr)
		return std::nullopt;

	std::string identifierName;
	std::optional<size_t> index = std::nullopt;

	// Get our identifier name and index;
	if (std::holds_alternative<std::string>(identifier))
		identifierName = std::get<std::string>(identifier);
	else if (std::holds_alternative<std::pair<std::string, size_t>>(identifier))
	{
		auto& pair = std::get<std::pair<std::string, size_t>>(identifier);
		identifierName = pair.first;
		index = pair.second;
	}
	else throw std::exception("retrieveVariableFromStore received an invalid identifier");

	// Look through all the variable stores for the variable.
	if (variableStores != nullptr)
	{
		for (auto& [prefix, storePicker] : *variableStores)
		{
			if (prefix.empty() || identifierName.starts_with(prefix))
			{
				if (std::holds_alternative<ScriptVariableStore*>(storePicker))
				{
					auto store = std::get<ScriptVariableStore*>(storePicker);
					return store->get_or_add(identifierName);
				}
				else if (std::holds_alternative<ScriptVariableFromServer>(storePicker))
				{
					auto& picker = std::get<ScriptVariableFromServer>(storePicker);
					return picker(identifier);
				}
			}
		}
	}

	// Check the default store.
	if (defaultStore != nullptr)
		return defaultStore->get_or_add(identifierName);

	// No variable found.
	return std::nullopt;
}

ScriptVariableContainer* getScriptVariableContainerUnsafe(std::any& anyval)
{
	if (!anyval.has_value()) return nullptr;
	auto* direct = std::any_cast<ScriptVariableContainer>(&anyval);
	if (direct != nullptr) return direct;
	return nullptr;
}

std::optional<ScriptVariableContainer> getScriptVariableContainer(const std::any& anyval)
{
	if (!anyval.has_value()) return std::nullopt;
	auto* direct = std::any_cast<ScriptVariableContainer>(&anyval);
	if (direct != nullptr) return *direct;
	return std::nullopt;
}

std::string getIdentifierName(const ScriptIdentifier& identifier)
{
	if (std::holds_alternative<std::string>(identifier))
		return std::get<std::string>(identifier);
	else if (std::holds_alternative<std::pair<std::string, size_t>>(identifier))
	{
		auto& pair = std::get<std::pair<std::string, size_t>>(identifier);
		return pair.first;
	}

	throw std::exception("getIdentifierName received an invalid identifier");
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
