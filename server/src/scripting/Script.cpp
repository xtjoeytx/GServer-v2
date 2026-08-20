#include <algorithm>
#include <any>
#include <array>
#include <format>
#include <iterator>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <BabyDI.h>

#include <Server.h>
#include <filesystem/File.h>
#include <filesystem/FileSystem.h>
#include <filesystem/FileSystemTypes.h>
#include <npcserver/NPCServer.h>
#include <scripting/IScriptEngine.h>
#include <scripting/Script.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptSystem.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/StringUtils.h>
#include <utilities/std/generator.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

constexpr auto clientSideTerminator = "//#CLIENTSIDE"sv;

//----------------------------

static std::string performClientSideJoinHack(std::string_view code)
{
	static constexpr std::array<char, 5> blockStarters = {'\n', '\xa7', ')', '{', ';'};
	static constexpr size_t joinKeywordLen = 5; // strlen("join ")

	std::string result;
	std::vector<std::string_view> joins;

	size_t start = 0;
	size_t end = 0;

	while (start < code.length())
	{
		// Find the next "join " token.
		// If we don't find one, copy the rest of the code and break.
		end = code.find("join ", start);
		if (end == std::string_view::npos)
		{
			result += code.substr(start);
			break;
		}

		// Ensure "join " starts a script block context (avoid matching inside strings/text).
		if (end != 0)
		{
			bool joinIsStartOfBlock = true;
			size_t blockStart = end - 1;
			while (blockStart > 0)
			{
				// Skip any whitespace before the join.
				if (code[blockStart] == ' ' || code[blockStart] == '\t')
				{
					--blockStart;
					continue;
				}

				// Look for the start of a block or a newline.
				if (!std::ranges::contains(blockStarters, code[blockStart]))
				{
					joinIsStartOfBlock = false;
					break;
				}

				// We found a new line or a block start.
				break;
			}

			if (!joinIsStartOfBlock)
			{
				result += code.substr(start, end);
				start = end + joinKeywordLen;
				continue;
			}
		}

		// Copy content before join.
		result += code.substr(start, end - start);

		// Parse joined class/file name until semicolon.
		start = end + joinKeywordLen;
		end = code.find(';', start);
		if (end == std::string_view::npos)
		{
			result += "join ";
			result += code.substr(start);
			break;
		}

		// Save the join to the list of joins.
		if (std::string_view join = string::trim(code.substr(start, end - start)); !join.empty())
		{
			bool inSign = false;

			// A simple check to make sure we aren't in a say2 sign.
			// This won't identify when the join keyword is used as the first word in the last line of a sign.
			// TODO: Improve this check.
			if (const auto lineEnd = code.find('\n', start); lineEnd != std::string_view::npos)
			{
				if (const auto signCheck = code.find("#b", start); signCheck < lineEnd)
					inSign = true;
			}

			if (inSign)
			{
				result += "join ";
				result += join;
			}
			// Otherwise, add the join to our joins list.
			else
			{
				joins.push_back(join);
			}

			// Make sure the semi-colon stays.
			result += ';';
		}

		start = end + 1;
	}

	// Append scripts from collected joins.
	if (const auto server = BabyDI::Get<Server>(); server && server->hasNPCServer())
	{
		for (const auto& className : joins)
		{
			if (auto classObject = server->getNPCServer()->getClass(className); classObject != nullptr)
			{
				if (result.back() != '\n')
					result += '\n';
				result += classObject->getScript().getClientSide();
			}
		}
	}
	else
	{
		for (const auto& fileName : joins)
		{
			auto file = server->getFileSystemServer().open(fs::FileCategory::SCRIPTCLASS, std::format("{}.txt", fileName));
			if (file != nullptr)
			{
				std::string classScript = Script::minify(file->readAsString());
				string::replaceMutate(classScript, "\r", "");
				if (result.back() != '\n')
					result += '\n';
				result += classScript;
			}
		}
	}

	return result;
}

//----------------------------

// NOLINTNEXTLINE(*-no-recursion)
std::generator<decltype(ScriptExecutionContext::joinedClasses)::const_reference> Script::getServerJoinedClasses() const noexcept
{
	if (m_server_script == nullptr)
		co_return;

	for (const auto& kvp : m_server_script->joinedClasses)
	{
		co_yield kvp;

		// Get child classes too.
		if (auto class_ = kvp.second.lock(); class_ != nullptr)
		{
			for (const auto& child : class_->getScript().getServerJoinedClasses())
				co_yield child;
		}
	}
}

//----------------------------

const ScriptByteCode& Script::getClientByteCode() const noexcept
{
	static ScriptByteCode empty;

	if (m_client_script == nullptr || !m_client_script->script->has_value())
		return empty;

	if (auto* bytecode = std::any_cast<ScriptByteCode*>(m_client_script->script); bytecode != nullptr)
		return *bytecode;

	return empty;
}

void Script::executeEvents(ScriptContainer& container, const ScriptObject& source) const
{
	executeEvents(container.events, source);
}

void Script::executeEvents(ScriptEventQueue& events, const ScriptObject& source) const
{
	if (m_server_script == nullptr || m_server_script->engine == nullptr)
		return;

	auto* engine = m_server_script->engine;
	auto& queue = events.queue();

	if (queue.size() == 1)
	{
		for (auto& event : queue)
			engine->execute(event, source, m_server_script);
		return;
	}

	std::vector<ScriptEventType> types;
	std::vector<ScriptEventType> processedTypes;

	for (auto& event : queue)
	{
		// Execute events with args (or TIMEOUT) individually.
		if (!event.args.empty() || event.type == ScriptEventType::TIMEOUT)
		{
			engine->execute(event, source, m_server_script);
			processedTypes.emplace_back(event.type);
			continue;
		}

		// Consolidate same-shape events that can run together.
		if (!std::ranges::contains(processedTypes, event.type))
		{
			types.clear();

			std::ranges::for_each(queue, [&event, &types, &processedTypes](const ScriptEvent& e)
			{
				if (e.type != event.type && e.initiator == event.initiator && e.args.empty() && !std::ranges::contains(processedTypes, e.type))
				{
					types.emplace_back(e.type);
					processedTypes.emplace_back(e.type);
				}
			});

			engine->execute(event, &types, source, m_server_script);
			processedTypes.emplace_back(event.type);
		}
	}
}

void Script::executeEvents(clear_container_t, ScriptContainer& container, const ScriptObject& source) const
{
	executeEvents(container, source);
	container.events.queue().clear();
}

void Script::executeEvents(clear_container_t, ScriptEventQueue& events, const ScriptObject& source) const
{
	executeEvents(events, source);
	events.queue().clear();
}

bool Script::runUserDefinedFunction(const std::string_view functionName, ScriptEvent& event, const ScriptObject& source) const
{
	if (m_server_script == nullptr || m_server_script->engine == nullptr)
		return false;

	return m_server_script->engine->executeFunction(functionName, event, source, m_server_script);
}

//----------------------------

std::string Script::minify(const std::string& src) noexcept
{
	if (src.empty())
		return src;

	std::string minified;
	std::string_view srcView{src};

	const auto server = BabyDI::Get<Server>();

	// We don't want to trim serverside code so check if we have any.
	bool hasServerSide = true;
	if (server && !server->hasNPCServer())
		hasServerSide = false;

	bool inServerSide = true;

	// Trim the lines.
	while (!srcView.empty())
	{
		// Find the next newline character.
		auto newline = srcView.find('\n');
		if (newline == std::string_view::npos)
			newline = srcView.size();

		// Save the start of the next line since the carriage return check will mess it up.
		size_t nextLine = std::min(srcView.size(), newline + 1);

		// Search for \r and remove that too.
		if (newline > 0 && srcView[newline - 1] == '\r')
			--newline;

		// Extract the line.
		auto line = srcView.substr(0, newline);

		// Remove single-line comments, preserving //# directives.
		if (const auto comment = line.find("//"); comment != std::string_view::npos)
		{
			if (comment + 2 < line.size() && line[comment + 2] != '#')
			{
				line = line.substr(0, comment);
				newline = comment;
			}
			else if (line.find(clientSideTerminator) != std::string_view::npos)
			{
				// If we have a clientside terminator, we are now in clientside code.
				inServerSide = false;
			}
		}

		// Check if a multi-line comment starts in this line.
		// If it does, we need to skip the entire comment block.
		if (const auto commentStart = line.find("/*"); commentStart != std::string_view::npos)
		{
			line = line.substr(0, commentStart);
			nextLine = std::string_view::npos;
			if (const auto commentEnd = srcView.find("*/", commentStart + 2); commentEnd != std::string_view::npos)
			{
				nextLine = commentEnd + 2;
			}
		}

		// Trim the line.
		if (!hasServerSide || !inServerSide)
			line = string::trim(line);

		// Append the line to minified.
		if (!line.empty())
		{
			minified.append(line);

			// An else without a space after it can break the GS1 parser so manually add a space.
			if (line.ends_with("else"))
				minified.append(" ");

			minified.append("\n");
		}

		// Move to the next line.
		srcView.remove_prefix(nextLine);
	}

	// Final trim.
	string::trimMutate(minified);

	// Return the minified code.
	return minified;
}

void Script::split(std::string& source) noexcept
{
	auto determineClientSideLocation = [](std::string& text) -> std::string::iterator
	{
		auto clientside = text.begin();
		if (const auto pos = text.find(clientSideTerminator); pos != std::string::npos)
			std::advance(clientside, pos);
		else
			clientside = text.end();

		return clientside;
	};

	const auto server = BabyDI::Get<Server>();

	// Check if we have an npc-server or not.
	// If we don't, we don't have serverside code, and thus we will ignore the clientside terminator.
	bool hasServerSide = true;
	if (server && !server->hasNPCServer())
		hasServerSide = false;

	// If we have serverside code, find the start of the clientside terminator.
	// We need to mangle the newlines on just the clientside code.
	// The serverside code will be fed into a compiler so it should have normal line endings.
	auto clientside = source.begin();
	if (hasServerSide)
		clientside = determineClientSideLocation(source);

	// Expand clientside "join" statements when server-side scripts are unavailable.
	if (!hasServerSide && server->getSettings().get<bool>("clientsidejoins").value_or(true) && clientside != source.end())
	{
		const auto joinedScript = performClientSideJoinHack(std::string_view{clientside, source.end()});
		source.replace(clientside, source.end(), joinedScript);

		if (hasServerSide)
			clientside = determineClientSideLocation(source);
		else
			clientside = source.begin();
	}

	// Mangle the line terminators.
	std::replace(clientside, source.end(), '\n', '\xa7');

	// If we don't have an npc-server, we don't support serverside code.
	if (!hasServerSide)
	{
		m_serverside = {};
		m_clientside = string::trim(source);
		return;
	}

	// Split the code into clientside and serverside.
	if (const auto clientSep = source.find(clientSideTerminator); clientSep != std::string::npos)
	{
		auto endOfLine = source.find('\xa7', clientSep);
		if (endOfLine == std::string::npos)
			endOfLine = clientSep + clientSideTerminator.size();

		m_serverside = string::trim(std::string_view{source}.substr(0, clientSep));
		m_clientside = {};

		if (endOfLine + 1 < source.size())
			m_clientside = string::trim(std::string_view{source}.substr(endOfLine + 1));
	}
	else
	{
		m_serverside = string::trim(source);
		m_clientside = {};
	}
}

void Script::compileScript() noexcept
{
	m_client_script.reset();
	m_server_script.reset();

	const auto server = BabyDI::Get<Server>();
	if (!(server && server->hasNPCServer()))
		return;

	const auto npcServer = server->getNPCServer();

	if (server->Generation == ServerGeneration::NEWMAIN)
	{
		m_server_script = npcServer->scripting.getCompiledServerScript(m_who, m_serverside);
	}
	else if (server->Generation == ServerGeneration::MODERN)
	{
		m_client_script = npcServer->scripting.getCompiledClientScript(m_who, m_clientside);
		m_server_script = npcServer->scripting.getCompiledServerScript(m_who, m_serverside);
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
