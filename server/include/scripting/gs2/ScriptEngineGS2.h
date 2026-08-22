#ifndef SCRIPTENGINEGS2_H
#define SCRIPTENGINEGS2_H

#include <optional>
#include <string>
#include <string_view>

#include <exceptions/GS2CompilerError.h>

#include <BabyDI.h>
#include <scripting/GS2ScriptManager.h>
#include <scripting/IScriptEngine.h>
#include <scripting/ScriptSystem.h>
#include <scripting/ScriptTypes.h>

namespace preagonal
{
class Server;
}

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs2
{
///////////////////////////////////////////////////////////////////////////////

class ScriptEngineGS2 : public IScriptEngine
{
public:
	~ScriptEngineGS2() override = default;

public:
	inline static auto EngineName = "gs2"sv;
	std::string_view getEngineName() override { return EngineName; }
	ScriptEngineMode getExecutionMode() override { return ScriptEngineMode::CALLBACK; }
	ScriptExecutionType getExecutionType() override { return ScriptExecutionType::COMPILED; }

public:
	CompiledScriptResult compileScript(std::string_view who, std::string_view script) override;
	bool reset() override { return false; }

public:
	bool execute(ScriptEvent& event, ScriptObject source, CompiledScriptResultPtr context) override { return false; }
	bool execute(ScriptEvent& event, std::vector<ScriptEventType>* additionalEventTypes, ScriptObject source, CompiledScriptResultPtr context) override { return false; }
	bool executeFunction(std::string_view function, ScriptEvent& event, ScriptObject source, CompiledScriptResultPtr context) override { return false; }
	bool executeFunction(std::string_view function, ScriptEvent& event, std::vector<ScriptEventType>* additionalEventTypes, ScriptObject source, CompiledScriptResultPtr context) override { return false; }

public:
	std::optional<double> processMathExpression(std::string_view expression, ScriptObject source) override { return std::nullopt; }
	std::optional<std::string> processStringExpression(std::string_view expression, ScriptObject source) override { return std::nullopt; }

protected:
	BabyDI_INJECT(Server, m_server);

	static std::string handleGS2Error(const GS2CompilerError& error);
	static void reportScriptException(const std::string& error_message);

	GS2ScriptManager m_scriptManager;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs2

#endif // SCRIPTENGINEGS2_H
