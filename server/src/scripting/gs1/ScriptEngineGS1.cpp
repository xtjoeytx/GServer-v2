#include <any>
#include <cstdint>
#include <exception>
#include <format>
#include <iterator>
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
#include <loader/INPCLoader.h>
#include <npcserver/NPCServer.h>
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
#include <utilities/CommonTypes.h>
#include <utilities/Log.h>
#include <utilities/PropertySerializers.h>

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

PlayerPtr getPlayerFromSource(const ScriptObject& source, std::optional<int64_t> index)
{
	if (source.second != ScriptObjectType::PLAYER)
		return nullptr;

	// TODO: Current player.
	if (index.value_or(0) == -1)
		return nullptr;

	auto server = BabyDI::Get<Server>();
	auto npcserver = server->getNPCServer();
	if (auto player = npcserver->getPlayer(source.first); player != nullptr)
	{
		if (index.has_value() && index.value() >= 0)
		{
			if (auto level = server->getLoadedLevel(player->account.level, player); level != nullptr && index.value() < (int64_t)level->getPlayers().size())
			{
				auto& mapPlayers = level->getPlayers();
				player = server->getPlayer(mapPlayers[index.value()]);
			}
		}
		return player;
	}

	return nullptr;
}

PlayerClientPtr getPlayerClientFromSource(const ScriptObject& source, std::optional<int64_t> index)
{
	auto player = getPlayerFromSource(source, index);
	if (auto client = std::dynamic_pointer_cast<PlayerClient>(player); client != nullptr)
		return client;
	return nullptr;
}

NPCPtr getNPCFromSource(const ScriptObject& source, std::optional<int64_t> index)
{
	if (source.second != ScriptObjectType::NPC)
		return nullptr;

	auto server = BabyDI::Get<Server>();
	auto npcserver = server->getNPCServer();
	if (auto npc = npcserver->getNPC(source.first); npc != nullptr)
	{
		if (index.has_value() && index.value() >= 0)
		{
			if (auto level = npc->getLevel(); level != nullptr && index.value() < (int64_t)level->getNPCs().size())
			{
				auto& mapNPCs = level->getNPCs();
				auto iter = mapNPCs.begin();
				std::ranges::advance(iter, index.value(), mapNPCs.end());
				if (iter != mapNPCs.end())
					npc = npcserver->getNPC(*iter);
			}
		}
		return npc;
	}
	return nullptr;
}

PlayerOrNPC getPlayerOrNPCFromSource(const ScriptObject& source, std::optional<int64_t> index)
{
	if (source.second == ScriptObjectType::SERVER)
		return std::nullopt;

	if (source.second == ScriptObjectType::PLAYER)
		return getPlayerFromSource(source, index);
	else if (source.second == ScriptObjectType::NPC)
		return getNPCFromSource(source, index);

	return std::nullopt;
}

Character* getCharacterFromSource(const ScriptObject& source, std::optional<int64_t> index)
{
	if (source.second == ScriptObjectType::SERVER)
		return nullptr;

	if (source.second == ScriptObjectType::PLAYER)
	{
		if (auto player = getPlayerFromSource(source, index); player != nullptr)
			return &player->account.character;
	}
	else if (source.second == ScriptObjectType::NPC)
	{
		if (auto npc = getNPCFromSource(source, index); npc != nullptr)
			return &npc->character;
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////

GS1ScriptWrapper::GS1ScriptWrapper(std::string_view who, std::string_view script)
{
	errorListenerLexer = std::make_shared<GS1ErrorListener>("lexing", who);
	errorListenerParser = std::make_shared<GS1ErrorListener>("parsing", who);

	// Load the script (lenient UTF-8 parsing).
	input = std::make_shared<antlr4::ANTLRInputStream>();
	input->load(script.data(), script.length(), true);

	// Create the lexer.
	// We don't need to keep this around.
	GS1Lexer lexer{ input.get() };
	lexer.removeErrorListeners();
	lexer.addErrorListener(errorListenerLexer.get());

	// Fill the tokens from the lexer.
	tokens = std::make_shared<antlr4::CommonTokenStream>(&lexer);
	tokens->fill();

	// Create the parser.
	parser = std::make_shared<GS1Parser>(tokens.get());
	parser->removeErrorListeners();
	parser->addErrorListener(errorListenerParser.get());

	// Run the parser and create our AST.
	visitor = std::make_shared<GS1Visitor>();
	program = parser->program();

#ifdef DEBUG
	//if (who == "MoveTester")
	if (false)
	{
		log::printLine(log::script, program->toStringTree(parser.get(), true));
	}
#endif

	setGlobalVariables(variables);
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

//----------------------------

bool ScriptEngineGS1::prepare(GS1ScriptWrapper& wrapper, ScriptEvent& event, ScriptObject source, CompiledScriptResultPtr context, NPCPtr& npc, LevelPtr& level)
{
	auto& [source_id, source_type] = source;
	if (source_type != ScriptObjectType::NPC && source_type != ScriptObjectType::WEAPON)
		throw std::invalid_argument("GS1 scripts can only be executed from NPCs and weapons.");

	auto server = BabyDI::Get<Server>();
	auto npcserver = server->getNPCServer();
	PlayerClientPtr player = nullptr;
	WeaponPtr weapon = nullptr;

	// Get whatever links we can.
	if (source_type == ScriptObjectType::PLAYER)
		player = npcserver->getPlayer<PlayerClient>(source_id);
	if (source_type == ScriptObjectType::NPC)
		npc = npcserver->getNPC(source_id);
	if (source_type == ScriptObjectType::WEAPON)
	{
		if (auto it = server->getWeaponList().find(source_id); it != server->getWeaponList().end())
			weapon = it->second;
	}
	if (player != nullptr)
		level = player->getLevel();
	if (npc != nullptr)
		level = npc->getLevel();

	// Try to get variables from the initiator now.
	if (player == nullptr && event.initiator.second == ScriptObjectType::PLAYER)
		player = npcserver->getPlayer<PlayerClient>(event.initiator.first);
	if (npc == nullptr && event.initiator.second == ScriptObjectType::NPC)
		npc = npcserver->getNPC(event.initiator.first);
	if (level == nullptr)
		level = (player != nullptr ? player->getLevel() : (npc != nullptr ? npc->getLevel() : nullptr));

	// Determine the "who" for error messages.
	if (npc != nullptr)
		wrapper.visitor->who = npc->name;
	else if (weapon != nullptr)
		wrapper.visitor->who = weapon->name;
	else if (player != nullptr)
		wrapper.visitor->who = player->account.name;
	else
		wrapper.visitor->who = "unknown";

	// Set the built-in store.
	wrapper.visitor->builtInStore = &wrapper.variables;

	// Set events.
	setTriggerActionAndCustomEventFlags(event, wrapper.visitor->flagStore);
	setEventFlags(event.type, wrapper.visitor->flagStore);

	// Set flags.
	setPlayerFlags(wrapper.variables, npc, player);
	setNPCFlags(event, wrapper.variables, npc);
	setLevelFlags(wrapper.variables, npc, level);
	setWeaponFlags(event, source, wrapper.variables);
	setOtherFlags(event, source, wrapper.variables, npc, player, level);

	// Set variables.
	setNPCVariables(wrapper.variables, npc);
	setPlayerVariables(wrapper.variables, player);
	setLevelVariables(wrapper.variables, level);
	setOtherVariables(wrapper.variables, event);

	return true;
}

//----------------------------

bool ScriptEngineGS1::execute(ScriptEvent& event, ScriptObject source, CompiledScriptResultPtr context)
{
	auto* wrapper = std::any_cast<GS1ScriptWrapper>(context->script.get());
	if (wrapper == nullptr)
		return false;

	auto server = BabyDI::Get<Server>();
	auto npcserver = server->getNPCServer();
	NPCPtr npc = nullptr;
	LevelPtr level = nullptr;

#if !defined(DEBUG) || 1
	// If the event is not in the NPC script, don't bother executing it.
	if (event.type != ScriptEventType::CREATED && event.type != ScriptEventType::INITIALIZED)
	{
		const auto& eventName = determineEventName(event);
		if (!wrapper->parser->identifiers.contains(eventName) && !server->cached.runAllScriptEvents.getValue())
			return false;
	}
#endif

	if (!prepare(*wrapper, event, source, context, npc, level))
		return false;

#if defined(DEBUG) && 0
	// Log some testing stuff.
	if (event.type != ScriptEventType::CREATED && event.type != ScriptEventType::INITIALIZED)
	{
		const auto& eventName = determineEventName(event);
		if (!wrapper->parser->identifiers.contains(eventName) && !server->cached.runAllScriptEvents.getValue())
		{
			log::printLine(log::script, "GS1 script for event '{}' not found in script '{}'.", eventName, wrapper->visitor->who);
			return false;
		}
	}
#endif

	// If this is a control-NPC, temporarily adjust the level it lives in.
	bool isControlNPC = npc && npc->scriptType == NPCTYPE_CONTROL;
	if (isControlNPC)
	{
		if (level == nullptr)
		{
			if (event.initiator.second == ScriptObjectType::NPC)
			{
				if (auto initiatingNPC = npcserver->getNPC(event.initiator.first); initiatingNPC != nullptr)
					level = initiatingNPC->getLevel();
			}
			else if (event.initiator.second == ScriptObjectType::PLAYER)
			{
				if (auto initiatingPlayer = npcserver->getPlayer<PlayerClient>(event.initiator.first); initiatingPlayer != nullptr)
					level = initiatingPlayer->getLevel();
			}
		}
		if (level != nullptr)
			npc->level = level->levelName;
	}

	try
	{
		// Execute the script.
		wrapper->visitor->execute(event, source, *wrapper->parser.get(), *context, wrapper->program);
	}
	catch (std::exception& e)
	{
#ifdef DEBUG
		log::printLine(log::script, "Script execution failure: {}", e.what());
		log::printLine(log::script, wrapper->program->toStringTree(wrapper->parser.get(), true));
		throw;
#endif
		// If we had a terminal error, remove the script from the context so it doesn't get executed again.
		context->script = nullptr;
	}

	// Fix the control-NPC level.
	if (isControlNPC)
		npc->level.clear();

	// Special case to handle "created" events for the NPC.
	if (npc != nullptr && event.type == ScriptEventType::CREATED)
	{
		npc->setPropWith<NPCProp::VISFLAGS>(SetBy::SERVER, static_cast<uint8_t>(npc->visFlags | PROPID(NPCVisFlags::CREATED)));
		if (npc->storageType == NPCStorageType::DATABASE)
			server->getNPCLoader().saveNPC(npc);
	}

	cleanup(*wrapper);
	return true;
}

bool ScriptEngineGS1::executeFunction(std::string_view function, ScriptEvent& event, ScriptObject source, CompiledScriptResultPtr context)
{
	if (context == nullptr)
		return false;

	auto* wrapper = std::any_cast<GS1ScriptWrapper>(context->script.get());
	if (wrapper == nullptr)
		return false;

	// Check if we have the function.
	auto userFunction = wrapper->parser->userFunctions.find(std::string{ function });
	if (userFunction == wrapper->parser->userFunctions.end())
		return false;

	NPCPtr npc = nullptr;
	LevelPtr level = nullptr;

	if (!prepare(*wrapper, event, source, context, npc, level))
		return false;

	try
	{
		// Execute the script.
		wrapper->visitor->execute(event, source, *wrapper->parser.get(), *context, userFunction->second);
	}
	catch (std::exception& e)
	{
#ifdef DEBUG
		log::printLine(log::script, "Script execution failure: {}", e.what());
		log::printLine(log::script, wrapper->program->toStringTree(wrapper->parser.get(), true));
		throw;
#endif
		// If we had a terminal error, remove the script from the context so it doesn't get executed again.
		context->script = nullptr;
	}

	cleanup(*wrapper);
	return true;
}

//----------------------------

double ScriptEngineGS1::processMathExpression(std::string_view expression, ScriptObject source)
{
	static GS1Visitor visitor{};
	static GameVariableStore variableStore{};
	visitor.builtInStore = &variableStore;

	visitor.pushSource(source);
	visitor.flagStore.store.clear();
	variableStore.store.clear();

	ScriptEvent created{.type = ScriptEventType::CREATED, .initiator = source};

	auto server = BabyDI::Get<Server>();
	auto npcserver = server->getNPCServer();
	if (source.second == ScriptObjectType::NPC)
	{
		if (auto npc = npcserver->getNPC(source.first); npc != nullptr)
		{
			setNPCFlags(created, visitor.flagStore, npc);
			setNPCVariables(variableStore, npc);
		}
	}
	else if (source.second == ScriptObjectType::PLAYER)
	{
		if (auto player = npcserver->getPlayer<PlayerClient>(source.first); player != nullptr)
		{
			setPlayerFlags(visitor.flagStore, nullptr, player);
			setPlayerVariables(variableStore, player);
		}
	}

	try
	{
		auto result = visitor.processMathExpression(expression);
		visitor.popSource();
		return visitor.getGameValueAs<double>(result);
	}
	catch (std::exception& e)
	{
		visitor.popSource();
	}

	return 0.0;
}

std::string ScriptEngineGS1::processStringExpression(std::string_view expression, ScriptObject source)
{
	static GS1Visitor visitor{};
	static GameVariableStore variableStore{};
	visitor.builtInStore = &variableStore;

	visitor.pushSource(source);
	visitor.flagStore.store.clear();

	ScriptEvent created{.type = ScriptEventType::CREATED, .initiator = source};

	auto server = BabyDI::Get<Server>();
	auto npcserver = server->getNPCServer();
	if (source.second == ScriptObjectType::NPC)
	{
		if (auto npc = npcserver->getNPC(source.first); npc != nullptr)
		{
			setNPCFlags(created, visitor.flagStore, npc);
			setNPCVariables(variableStore, npc);
		}
	}
	else if (source.second == ScriptObjectType::PLAYER)
	{
		if (auto player = npcserver->getPlayer<PlayerClient>(source.first); player != nullptr)
		{
			setPlayerFlags(visitor.flagStore, nullptr, player);
			setPlayerVariables(variableStore, player);
		}
	}

	try
	{
		auto result = visitor.processStringExpression(expression);
		visitor.popSource();
		return visitor.getGameValueAs<std::string>(result);
	}
	catch (std::exception& e)
	{
		visitor.popSource();
	}

	return std::string{};
}

//----------------------------

void ScriptEngineGS1::cleanup(GS1ScriptWrapper& wrapper)
{
	// Clear the variables (to clear reference counted pointers, just in case).
	wrapper.variables.clearTemporary();
	wrapper.visitor->flagStore.clearTemporary();
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1
