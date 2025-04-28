#include "scripting/gs2/ScriptEngineGS2.h"

#include "common.h"

#include "Server.h"
#include "utilities/StringUtils.h"

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

inline constexpr std::array<std::string_view, SCRIPT_TYPE_COUNT> scriptTypeStrings =
{
	/* ScriptType::CLASS */ "class",
	/* ScriptType::WEAPON */ "weapon"
};

std::shared_ptr<ScriptCompilationResult> ScriptEngineGS2::compileScript(ScriptType type, std::string_view name, const std::string& script)
{
	// Compile the script.
	auto result = m_scriptManager.compileScript(script);
	auto response = result.get();

	// Construct the compilation result.
	ScriptCompilationResult compilationResult;
	compilationResult.success = response.success;
	compilationResult.errorMessage = string::join(response.errors | std::views::transform([this](const GS2CompilerError& error) -> std::string { return handleGS2Error(error); }), "\n");
	if (response.success)
	{
		compilationResult.joinedClasses.insert(response.joinedClasses.begin(), response.joinedClasses.end());
		
		auto bytecodeWithHeader = GS2Context::CreateHeader(response.bytecode, std::string{ scriptTypeStrings.at(static_cast<size_t>(type)) }, std::string{ name }, true);
		auto bytecode = std::make_shared<std::vector<uint8_t>>();
		bytecode->insert(bytecode->end(), bytecodeWithHeader.buffer(), bytecodeWithHeader.buffer() + bytecodeWithHeader.length());
		bytecode->insert(bytecode->end(), response.bytecode.buffer(), response.bytecode.buffer() + response.bytecode.length());
		compilationResult.bytecode = bytecode;
	}

	return std::make_shared<ScriptCompilationResult>(std::move(compilationResult));
}

std::string ScriptEngineGS2::handleGS2Error(const GS2CompilerError& error)
{
	std::string errorMsg;
	switch (error.level())
	{
		case ErrorLevel::E_INFO:
			errorMsg += std::format("info: {}", error.msg());
			break;
		case ErrorLevel::E_WARNING:
			errorMsg += std::format("warning: {}", error.msg());
			break;
		default:
			errorMsg += std::format("error: {}", error.msg());
			break;
	}

	if (!errorMsg.empty())
		reportScriptException(std::format("Script compiler output:\n{}", errorMsg));

	return errorMsg;
}

void ScriptEngineGS2::reportScriptException(const std::string& error_message)
{
	m_server->sendToNC(error_message);
	log::printLine(log::script, error_message);
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal
