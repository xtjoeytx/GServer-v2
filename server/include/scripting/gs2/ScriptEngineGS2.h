#ifndef SCRIPTENGINEGS2_H
#define SCRIPTENGINEGS2_H

#include <string_view>
#include <string>

#include <exceptions/GS2CompilerError.h>

#include <BabyDI.h>
#include <scripting/GS2ScriptManager.h>
#include <scripting/IScriptEngine.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptSystem.h>

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
	virtual CompiledScriptResult compileScript(std::string_view script) override;
	virtual bool reset() override { return false; }

public:
	virtual bool execute(ScriptEvent& event, ScriptObjectSource source, CompiledScriptResultPtr context) override { return false; }

protected:
	BabyDI_INJECT(Server, m_server);

	std::string handleGS2Error(const GS2CompilerError& error);
	void reportScriptException(const std::string& error_message);

	GS2ScriptManager m_scriptManager;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs2

#endif // SCRIPTENGINEGS2_H
