#ifndef ISCRIPTENGINE_H
#define ISCRIPTENGINE_H

#include <optional>
#include <string>
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
	virtual ~IScriptEngine() = default;

public:
	virtual std::string_view getEngineName() = 0;
	virtual ScriptEngineMode getExecutionMode() = 0;
	virtual ScriptExecutionType getExecutionType() = 0;

public:
	virtual CompiledScriptResult compileScript(std::string_view who, std::string_view script) = 0;
	virtual bool reset() = 0;

public:
	virtual bool execute(ScriptEvent& event, ScriptObject source, CompiledScriptResultPtr context) = 0;
	virtual bool execute(ScriptEvent& event, std::vector<ScriptEventType>* additionalEventTypes, ScriptObject source, CompiledScriptResultPtr context) = 0;
	virtual bool executeFunction(std::string_view function, ScriptEvent& event, ScriptObject source, CompiledScriptResultPtr context) = 0;
	virtual bool executeFunction(std::string_view function, ScriptEvent& event, std::vector<ScriptEventType>* additionalEventTypes, ScriptObject source, CompiledScriptResultPtr context) = 0;

public:
	virtual std::optional<double> processMathExpression(std::string_view expression, ScriptObject source) = 0;
	virtual std::optional<std::string> processStringExpression(std::string_view expression, ScriptObject source) = 0;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // ISCRIPTENGINE_H
