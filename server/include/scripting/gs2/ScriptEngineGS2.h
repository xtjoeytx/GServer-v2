#ifndef SCRIPTENGINEGS2_H
#define SCRIPTENGINEGS2_H

#include <unordered_map>
#include <string>
#include <string_view>

#include <BabyDI.h>
#include <scripting/IScriptEngine.h>
#include <scripting/GS2ScriptManager.h>

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
	virtual ~ScriptEngineGS2() override {}

public:
	virtual ScriptEngineMode getExecutionMode() override { return ScriptEngineMode::CALLBACK; }
	virtual ScriptExecutionType getExecutionType() override { return ScriptExecutionType::COMPILED; }

public:
	virtual CompiledScriptResult compileScript(ScriptType type, std::string_view name, const std::string& script) override;
	virtual bool reset() override { return false; }

public:
	virtual bool execute(const ScriptEvent& event, ScriptEventSource source, CompiledScriptResultPtr context) override { return false; }

protected:
	BabyDI_INJECT(Server, m_server);

	std::string handleGS2Error(const GS2CompilerError& error);
	void reportScriptException(const std::string& error_message);

	GS2ScriptManager m_scriptManager;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs2
#endif // SCRIPTENGINEGS2_H
