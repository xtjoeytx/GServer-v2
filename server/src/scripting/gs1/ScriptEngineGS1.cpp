#include <any>
#include <exception>
#include <format>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

#include <ANTLRInputStream.h>
#include <CommonTokenStream.h>
#include <GS1Lexer.h>
#include <GS1Parser.h>

#include <BabyDI.h>
#include <Server.h>
#include <level/Level.h>
#include <object/Character.h>
#include <object/NPC.h>
#include <object/Weapon.h>
#include <player/PlayerClient.h>
#include <scripting/gs1/GS1ErrorListener.h>
#include <scripting/gs1/GS1Flags.h>
#include <scripting/gs1/GS1Variables.h>
#include <scripting/gs1/GS1Visitor.h>
#include <scripting/gs1/ScriptEngineGS1.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptSystem.h>
#include <scripting/ScriptTypes.h>
#include <utilities/Log.h>

using namespace preagonal::gs1::grammar;

////////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
////////////////////////////////////////////////////////////////////////////////

static std::string determineEventName(ScriptEvent& event)
{
	auto knownEventIter = eventFlagMap.find(event.type);
	if (knownEventIter != eventFlagMap.end())
		return std::string{ knownEventIter->second };

	if (event.type == ScriptEventType::CUSTOM || event.type == ScriptEventType::TRIGGERACTION)
	{
		std::string action;
		if (auto* actionStr = std::any_cast<std::string>(&event.args[0]); actionStr != nullptr)
			action = *actionStr;
		else if (auto* actionStr = std::any_cast<const char*>(&event.args[0]); actionStr != nullptr)
			action = *actionStr;
		else if (auto* actionStr = std::any_cast<std::string_view>(&event.args[0]); actionStr != nullptr)
			action = std::string(*actionStr);

		return std::format("{}{}", (event.type == ScriptEventType::TRIGGERACTION ? "action" : ""), action);
	}

	return {};
}

////////////////////////////////////////////////////////////////////////////////

PlayerPtr getPlayerFromSource(const ScriptObjectSource& source, std::optional<size_t> index)
{
	if (source.second != ScriptObjectSourceType::PLAYER)
		return nullptr;

	auto* server = BabyDI::Get<Server>();
	if (auto player = server->getPlayer(source.first); player != nullptr)
	{
		if (index.has_value())
		{
			if (auto level = server->getLevel(player->account.level); level != nullptr && index.value() < level->getPlayers().size())
				player = server->getPlayer(level->getPlayers().at(index.value()));
		}
		return player;
	}

	return nullptr;
}

PlayerClientPtr getPlayerClientFromSource(const ScriptObjectSource& source, std::optional<size_t> index)
{
	auto player = getPlayerFromSource(source, index);
	if (auto client = std::dynamic_pointer_cast<PlayerClient>(player); client != nullptr)
		return client;
	return nullptr;
}

NPCPtr getNPCFromSource(const ScriptObjectSource& source, std::optional<size_t> index)
{
	if (source.second != ScriptObjectSourceType::NPC)
		return nullptr;
	auto* server = BabyDI::Get<Server>();
	if (auto npc = server->getNPC(source.first); npc != nullptr)
	{
		if (index.has_value())
		{
			if (auto level = npc->level.lock(); level != nullptr && index.value() < level->getNPCs().size())
				npc = server->getNPC(level->getNPCs().at(index.value()));
		}
		return npc;
	}
	return nullptr;
}

PlayerOrNPC getPlayerOrNPCFromSource(const ScriptObjectSource& source, std::optional<size_t> index)
{
	if (source.second == ScriptObjectSourceType::SERVER)
		return std::nullopt;

	auto* server = BabyDI::Get<Server>();
	if (source.second == ScriptObjectSourceType::PLAYER)
		return getPlayerFromSource(source, index);
	else if (source.second == ScriptObjectSourceType::NPC)
		return getNPCFromSource(source, index);

	return std::nullopt;
}

Character* getCharacterFromSource(const ScriptObjectSource& source, std::optional<size_t> index)
{
	if (source.second == ScriptObjectSourceType::SERVER)
		return nullptr;

	auto* server = BabyDI::Get<Server>();
	if (source.second == ScriptObjectSourceType::PLAYER)
	{
		if (auto player = getPlayerFromSource(source, index); player != nullptr)
			return &player->account.character;
	}
	else if (source.second == ScriptObjectSourceType::NPC)
	{
		if (auto npc = getNPCFromSource(source, index); npc != nullptr)
			return &npc->character;
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////

GS1ScriptWrapper::GS1ScriptWrapper(std::string_view who, std::string_view script)
{
	errorListener = std::make_shared<GS1ErrorListener>(who);

	input = std::make_shared<antlr4::ANTLRInputStream>(script);
	lexer = std::make_shared<GS1Lexer>(input.get());
	lexer->removeErrorListeners();
	lexer->addErrorListener(errorListener.get());

	tokens = std::make_shared<antlr4::CommonTokenStream>(lexer.get());
	parser = std::make_shared<GS1Parser>(tokens.get());
	parser->removeErrorListeners();
	parser->addErrorListener(errorListener.get());

	visitor = std::make_shared<GS1Visitor>();
	program = parser->program();
	setReadOnlyGlobalVariables(variables);
}

////////////////////////////////////////////////////////////////////////////////

ScriptEngineGS1::ScriptEngineGS1()
{
}

CompiledScriptResult ScriptEngineGS1::compileScript(std::string_view who, std::string_view script)
{
	ScriptExecutionContext result{ .engine = this };
	try
	{
		result.script = std::make_shared<std::any>(std::in_place_type<GS1ScriptWrapper>, who, script);
	}
	catch (const std::exception& ex)
	{
		log::printLine(log::script, "GS1 script compilation SUPER failed: {}", ex.what());
	}

	return result;
}

bool ScriptEngineGS1::execute(ScriptEvent& event, ScriptObjectSource source, CompiledScriptResultPtr context)
{
	auto* wrapper = std::any_cast<GS1ScriptWrapper>(context->script.get());
	if (wrapper == nullptr)
		return false;

	auto* server = BabyDI::Get<Server>();

#ifndef DEBUG
	// If the event is not in the NPC script, don't bother executing it.
	const auto& eventName = determineEventName(event);
	if (!wrapper->parser->identifiers.contains(eventName) &&!server->getSettings().getBool("runallscriptevents", false))
		return false;
#endif

	auto& [source_id, source_type] = source;
	if (source_type != ScriptObjectSourceType::NPC && source_type != ScriptObjectSourceType::WEAPON)
		throw std::invalid_argument("GS1 scripts can only be executed from NPCs and weapons.");

	PlayerClientPtr player = nullptr;
	NPCPtr npc = nullptr;
	WeaponPtr weapon = nullptr;
	LevelPtr level = nullptr;

	// Get whatever links we can.
	if (source_type == ScriptObjectSourceType::PLAYER)
		player = server->getPlayer<PlayerClient>(source_id);
	if (source_type == ScriptObjectSourceType::NPC)
		npc = server->getNPC(source_id);
	if (source_type == ScriptObjectSourceType::WEAPON)
	{
		if (auto it = server->getWeaponList().find(source_id); it != server->getWeaponList().end())
			weapon = it->second;
	}
	if (player != nullptr)
		level = player->getLevel();
	if (npc != nullptr)
		level = npc->level.lock();

	// Try to get variables from the initiator now.
	if (player == nullptr && event.initiator.second == ScriptObjectSourceType::PLAYER)
		player = server->getPlayer<PlayerClient>(event.initiator.first);
	if (npc == nullptr && event.initiator.second == ScriptObjectSourceType::NPC)
		npc = server->getNPC(event.initiator.first);
	if (level == nullptr)
		level = (player != nullptr ? player->getLevel() : npc->level.lock());

	// Determine the "who" for error messages.
	if (npc != nullptr)
		wrapper->visitor->who = npc->name;
	else if (player != nullptr)
		wrapper->visitor->who = player->account.name;
	else if (weapon != nullptr)
		wrapper->visitor->who = weapon->name;
	else
		wrapper->visitor->who = "unknown";

#ifdef DEBUG
	// Log some testing stuff.
	const auto& eventName = determineEventName(event);
	if (!wrapper->parser->identifiers.contains(eventName) && !server->getSettings().getBool("runallscriptevents", false))
	{
		log::printLine(log::script, "GS1 script for event '{}' not found in script '{}'.", eventName, wrapper->visitor->who);
		return false;
	}
#endif

	// Set the built-in store.
	wrapper->variables.clearTemporary();
	wrapper->visitor->builtInStore = &wrapper->variables;

	// Set events.
	setTriggerActionAndCustomEventFlags(event, wrapper->variables);
	setEventFlags(event.type, wrapper->variables);

	// Set flags.
	setPlayerFlags(wrapper->variables, npc, player);
	setNPCFlags(wrapper->variables, npc);
	setLevelFlags(wrapper->variables, npc, level);
	setWeaponFlags(event, source, wrapper->variables);
	setOtherFlags(event, source, wrapper->variables, npc, player, level);

	// Set variables.
	setPlayerVariables(wrapper->variables, player);
	setLevelVariables(wrapper->variables, level);
	setOtherVariables(wrapper->variables, event);

#ifdef DEBUG
	//if (event.args.size() > 0 && event.type == ScriptEventType::CUSTOM)
	if (false)
	{
		log::printLine(log::script, wrapper->program->toStringTree(wrapper->parser.get(), true));
	}
#endif

	try
	{
		// Execute the script.
		wrapper->visitor->execute(event, source, *wrapper->parser.get(), *wrapper->program);
	}
	catch (...)
	{
#ifdef DEBUG
		log::printLine(log::script, wrapper->program->toStringTree(wrapper->parser.get(), true));
		throw;
#endif
		// If we had a terminal error, remove the script from the context so it doesn't get executed again.
		context->script = nullptr;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1
