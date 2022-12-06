#include <regex>
#include <tuple>
#include <unordered_map>
#include "V8ScriptWrappers.h"

const std::regex word_regex(R"(((public[\s]+)?function[\s]+){1}(\w+)[\s]*\()");

struct FunctionMetaData
{
	bool isPublic = false;
	bool isEvent = false;
	std::string fnName;
};

std::string make_js_hash_code(const std::string& str)
{
	// "onCreated": onCreated
	std::string out;
	out.reserve(4 + str.length() * 2);
	out.append("\"").append(str).append("\": ").append(str);
	return out;
}

std::unordered_map<std::string, FunctionMetaData> getFunctionList(const std::string& code)
{
	std::unordered_map<std::string, FunctionMetaData> fns;
	auto words_begin = std::sregex_iterator(code.begin(), code.end(), word_regex);
	auto words_end = std::sregex_iterator();

	for (std::sregex_iterator i = words_begin; i != words_end; ++i)
	{
		const std::smatch& match = *i;
		if (match.size() == 4)
		{
			auto matchFnName = match[3].str();
			if (!matchFnName.empty())
			{
				bool isPub = (match[2].compare("public") == 1);
				bool isEvent = (matchFnName.starts_with("on"));

				fns[matchFnName] = {
					isPub || fns[matchFnName].isPublic,
					isEvent || fns[matchFnName].isEvent,
					matchFnName
				};
			}
		}
	}

	return fns;
}

std::pair<std::string, std::string> getScriptTables(const std::string& code)
{
	auto fns = getFunctionList(code);

	std::string eventTable;
	std::string publicTable;

	for (const auto& [fnName, fn] : fns)
	{
		if (fn.isPublic)
		{
			if (!publicTable.empty())
				publicTable.append(",\n");
			publicTable.append("\t").append(make_js_hash_code(fnName));
		}

		if (fn.isEvent)
		{
			if (!eventTable.empty())
				eventTable.append(",\n");
			eventTable.append("\t").append(make_js_hash_code(fnName));
		}
	}

	return std::make_pair(eventTable, publicTable);
}

std::string prepareScript(const std::string& code, uint32_t flags)
{
	std::string newCode;
	newCode.reserve(code.length() + 1024);

	if (flags & V8ScriptWrapperFlags::EMIT_SCOPE)
	{
		newCode += R"(
			var player;
			const __setScope = function(value) {
				player = value;
			};
		)";
	}
	else
	{
		// Require something, not doing checks on callback. Can't keep player global
		// as player-class has that variable although no events should really flow that way
		newCode += "const __setScope = function(value) {};";
	}

	// Emit script table
	auto scriptTables = getScriptTables(code);
	newCode += "const __eventFunctions = {\n"
			   + scriptTables.first + "\n};\n" +
			   +"const __publicFunctions = {\n"
			   + scriptTables.second + "\n};\n";

	newCode += R"(
			const __getScriptTable = () => ({
				'events': __eventFunctions,
				'public': __publicFunctions,
				'scope': __setScope
			});
		)";

	// Replace public functions, and emit the code
	newCode += std::regex_replace(std::string(code), word_regex, "function $3(");

	// PLAYER / NPC / WEAPONS:
	// Don't want classes overriding via self
	if (!(flags & V8ScriptWrapperFlags::EMIT_CLASS))
	{
		newCode += "self.__main = __getScriptTable();";
	}

	// Classes require this, an event will be dispatched to the bootstrap file using
	// callback npc.joincls with this script table so we can reconstruct the event-flow
	newCode += "return __getScriptTable();";

	return newCode;
}
