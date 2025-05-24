#include <any>

#include <scripting/gs1/ScriptEngineGS1.h>

#include <common.h>
#include <Server.h>
#include <level/Level.h>
#include <object/NPC.h>
#include <scripting/gs1/GS1Visitor.h>
#include <utilities/StringUtils.h>

using namespace preagonal::grammar::gs1;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
///////////////////////////////////////////////////////////////////////////////

void setEventFlags(ScriptEventType event, ScriptVariableStore& variableStore)
{
	static const std::unordered_map<ScriptEventType, std::string_view> eventFlagMap =
	{
		{ ScriptEventType::CREATED, ScriptEventFlagNames::CREATED },
		{ ScriptEventType::INITIALIZED, ScriptEventFlagNames::INITIALIZED },
		{ ScriptEventType::PLAYERLOGIN, ScriptEventFlagNames::PLAYERLOGIN },
		{ ScriptEventType::PLAYERLOGOUT, ScriptEventFlagNames::PLAYERLOGOUT },
		{ ScriptEventType::PLAYERENTERS, ScriptEventFlagNames::PLAYERENTERS },
		{ ScriptEventType::PLAYERLEAVES, ScriptEventFlagNames::PLAYERLEAVES },
		{ ScriptEventType::PLAYERTOUCHSME, ScriptEventFlagNames::PLAYERTOUCHSME },
		{ ScriptEventType::PLAYERTOUCHSOTHER, ScriptEventFlagNames::PLAYERTOUCHSOTHER },
		{ ScriptEventType::PLAYERLAYSITEM, ScriptEventFlagNames::PLAYERLAYSITEM },
		{ ScriptEventType::PLAYERCHATS, ScriptEventFlagNames::PLAYERCHATS },
		{ ScriptEventType::PLAYERDIES, ScriptEventFlagNames::PLAYERDIES },
		{ ScriptEventType::PLAYERENDREADING, ScriptEventFlagNames::PLAYERENDREADING },
		{ ScriptEventType::WEAPONFIRED, ScriptEventFlagNames::WEAPONFIRED },
		{ ScriptEventType::FIREDONHORSE, ScriptEventFlagNames::FIREDONHORSE },
		{ ScriptEventType::COMPUSDIED, ScriptEventFlagNames::COMPUSDIED },
		{ ScriptEventType::WARPED, ScriptEventFlagNames::WARPED },
		{ ScriptEventType::NPCWARPED, ScriptEventFlagNames::NPCWARPED },
		{ ScriptEventType::EXPLODED, ScriptEventFlagNames::EXPLODED },
		{ ScriptEventType::WASHIT, ScriptEventFlagNames::WASHIT },
		{ ScriptEventType::WASSHOT, ScriptEventFlagNames::WASSHOT },
		{ ScriptEventType::WASPELT, ScriptEventFlagNames::WASPELT },
		{ ScriptEventType::TIMEOUT, ScriptEventFlagNames::TIMEOUT },
		//
		{ ScriptEventType::SERVERLISTCONNECT, ScriptEventFlagNames::SERVERLISTCONNECT }
	};

	auto it = eventFlagMap.find(event);
	if (it != eventFlagMap.end())
	{
		auto flagName = it->second;
		variableStore.add(std::string{ flagName } + "|double", 1.0);

		// TODO: Put extensions under a server option?
		if (event == ScriptEventType::PLAYERTOUCHSME)
			variableStore.add(std::string{ ScriptEventFlagNames::PLAYERTOUCHESME } + "|double", 1.0);
		if (event == ScriptEventType::PLAYERTOUCHSOTHER)
			variableStore.add(std::string{ ScriptEventFlagNames::PLAYERTOUCHESOTHER } + "|double", 1.0);
	}
}

///////////////////////////////////////////////////////////////////////////////

GS1ScriptWrapper::GS1ScriptWrapper(std::string_view script)
{
	input = std::make_shared<antlr4::ANTLRInputStream>(script);
	lexer = std::make_shared<preagonal::grammar::gs1::GS1Lexer>(input.get());
	tokens = std::make_shared<antlr4::CommonTokenStream>(lexer.get());
	parser = std::make_shared<preagonal::grammar::gs1::GS1Parser>(tokens.get());
	program = parser->program();
}

///////////////////////////////////////////////////////////////////////////////

ScriptEngineGS1::ScriptEngineGS1()
{
}

CompiledScriptResult ScriptEngineGS1::compileScript(ScriptType type, std::string_view name, const std::string& script)
{
	ScriptExecutionContext result{ .engine = this };
	result.script = std::make_shared<std::any>(GS1ScriptWrapper{ script });
	return result;
}

bool ScriptEngineGS1::execute(const ScriptEvent& event, ScriptEventSource source, CompiledScriptResultPtr context)
{
	auto* wrapper = std::any_cast<GS1ScriptWrapper>(context->script.get());
	if (wrapper == nullptr)
		return false;

	auto* server = BabyDI::Get<Server>();
	auto& [source_id, source_type] = source;
	PlayerClientPtr player = nullptr;
	NPCPtr npc = nullptr;
	LevelPtr level = nullptr;

	// Get whatever links we can.
	if (source_type == ScriptEventSourceType::PLAYER)
		player = server->getPlayer<PlayerClient>(source_id);
	if (source_type == ScriptEventSourceType::NPC)
		npc = server->getNPC(source_id);
	if (player != nullptr)
		level = player->getLevel();
	if (npc != nullptr)
		level = npc->level.lock();

	ScriptVariableStore* defaultVariableStore = nullptr;
	ScriptVariableStoreMap variableStores{};

	// Link to the default variable store.
	if (level != nullptr)
		defaultVariableStore = &level->variables;

	// Link to the NPC variable store.
	if (npc != nullptr)
		variableStores.insert(std::make_pair("this.", &npc->scripting.variables));

	// Set flags.
	ScriptVariableStore eventFlagStore;
	setEventFlags(event.type, eventFlagStore);

	// Use the flag store as a default container if none is set.
	if (defaultVariableStore == nullptr)
		defaultVariableStore = &eventFlagStore;

	// TODO(Nalin): Link to the server variable handler.

	// Execute the script.
	GS1Visitor visitor;
	visitor.execute(event, source, *wrapper->parser.get(), *wrapper->program, defaultVariableStore, &variableStores);

	return false;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1
