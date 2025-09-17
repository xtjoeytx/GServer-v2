#ifndef ISCRIPTENGINE_H
#define ISCRIPTENGINE_H

#include <string_view>

#include <scripting/ScriptSystem.h>
#include <scripting/ScriptTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

enum class ScriptEngineMode
{
	CALLBACK,
	DIRECT
};

enum class ScriptExecutionType
{
	INTERPRETED,
	COMPILED
};

class IScriptEngine
{
public:
	virtual ~IScriptEngine() {};

public:
	virtual ScriptEngineMode getExecutionMode() = 0;
	virtual ScriptExecutionType getExecutionType() = 0;

public:
	virtual CompiledScriptResult compileScript(std::string_view who, std::string_view script) = 0;
	virtual bool reset() = 0;

public:
	virtual bool execute(ScriptEvent& event, ScriptObject source, CompiledScriptResultPtr context) = 0;
	virtual bool executeFunction(std::string_view function, ScriptEvent& event, ScriptObject source, CompiledScriptResultPtr context) = 0;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // ISCRIPTENGINE_H
