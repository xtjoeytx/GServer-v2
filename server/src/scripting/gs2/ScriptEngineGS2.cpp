#include <any>
#include <array>
#include <cstdint>
#include <format>
#include <memory>
#include <ranges>
#include <string_view>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <GS2Context.h>

#include <Server.h>
#include <exceptions/GS2CompilerError.h>
#include <scripting/gs2/ScriptEngineGS2.h>
#include <scripting/ScriptSystem.h>
#include <scripting/ScriptTypes.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs2
{
///////////////////////////////////////////////////////////////////////////////

CompiledScriptResult ScriptEngineGS2::compileScript(std::string_view who, std::string_view script)
{
	// Compile the script.
	auto result = m_scriptManager.compileScript(std::string{ script });
	auto response = result.get();

	// Error.
	if (!response.success)
		return CompiledScriptResult{ string::join(response.errors | std::views::transform([this](const GS2CompilerError& error) -> std::string { return handleGS2Error(error); }), "\n") };

	// Construct the compilation result.
	ScriptExecutionContext scriptContext{ .engine = this };
	for (const auto& joinedClass : response.joinedClasses)
	{
		// TODO: Get class.
		using p = decltype(scriptContext.joinedClasses)::value_type;
		scriptContext.joinedClasses.insert(p(joinedClass, {}));
	}

	// Generate the bytecode.
	std::vector<uint8_t> bytecode;
	bytecode.insert(bytecode.end(), response.bytecode.buffer(), response.bytecode.buffer() + response.bytecode.length());

	// Wrap the bytecode.
	auto wrapper = std::make_any<std::vector<uint8_t>>(std::move(bytecode));
	scriptContext.script = std::make_shared<std::any>(std::move(wrapper));

	// Return the context.
	return CompiledScriptResult{ std::move(scriptContext) };
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
} // end namespace preagonal::gs2
