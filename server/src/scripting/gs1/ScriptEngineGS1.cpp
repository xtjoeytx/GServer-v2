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
	NPCPtr npc = nullptr;
	LevelPtr level = nullptr;

	// Get whatever links we can.
	if (source_type == ScriptEventSourceType::NPC)
		npc = server->getNPC(source_id);
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

	// TODO(Nalin): Link to the server variable handler.

	// Execute the script.
	GS1Visitor visitor;
	visitor.execute(event.initiator, *wrapper->parser.get(), *wrapper->program, defaultVariableStore, &variableStores);

	return false;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1
