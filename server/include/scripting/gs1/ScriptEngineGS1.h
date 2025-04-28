#ifndef SCRIPTENGINEGS1_H
#define SCRIPTENGINEGS1_H

#include "scripting/IScriptEngine.h"

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

class ScriptEngineGS1 : public IScriptEngine
{
public:
	~ScriptEngineGS1() {}

public:
	ScriptEngineMode getExecutionMode() override { return ScriptEngineMode::DIRECT; }
	ScriptExecutionType getExecutionType() override { return ScriptExecutionType::INTERPRETED; }

	// executeAction

public:
	ScriptCompilationResultPtr compileScript(ScriptType type, std::string_view name, const std::string& script) override { return {}; }
	bool execute(IScriptedObject& object, const SourceCode& source) override { return false; }
	bool reset() override { return false; }
};

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal

#endif // SCRIPTENGINEGS1_H
