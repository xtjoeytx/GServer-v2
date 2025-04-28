#ifndef SCRIPTENGINEGS2_H
#define SCRIPTENGINEGS2_H

#include <unordered_map>
#include <string>
#include <string_view>

#include "BabyDI.h"
#include "scripting/IScriptEngine.h"
#include "scripting/GS2ScriptManager.h"

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

class Server;
class ScriptEngineGS2 : public IScriptEngine
{
public:
	~ScriptEngineGS2() {}

public:
	ScriptEngineMode getExecutionMode() override { return ScriptEngineMode::CALLBACK; }
	ScriptExecutionType getExecutionType() override { return ScriptExecutionType::COMPILED; }

	// executeAction

public:
	ScriptCompilationResultPtr compileScript(ScriptType type, std::string_view name, const std::string& script) override;
	bool execute(IScriptedObject& object, const SourceCode& source) override { return false; }
	bool reset() override { return false; }

protected:
	BabyDI_INJECT(Server, m_server);

	std::string handleGS2Error(const GS2CompilerError& error);
	void reportScriptException(const std::string& error_message);

	GS2ScriptManager m_scriptManager;
};

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal

#endif // SCRIPTENGINEGS2_H
