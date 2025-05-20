#include <any>

#include <scripting/gs1/ScriptEngineGS1.h>

#include <common.h>
#include <Server.h>
#include <scripting/gs1/GS1Visitor.h>
#include <utilities/StringUtils.h>

using namespace preagonal::grammar::gs1;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
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

bool ScriptEngineGS1::execute(const ScriptEvent& event, CompiledScriptResultPtr context, ScriptVariableStore* object_variables, ScriptVariableStore* level_variables)
{
	auto* wrapper = std::any_cast<GS1ScriptWrapper>(context->script.get());
	if (wrapper == nullptr)
		return false;

	GS1Visitor visitor;
	visitor.execute(event.source, wrapper->program, object_variables, level_variables);

	return false;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
