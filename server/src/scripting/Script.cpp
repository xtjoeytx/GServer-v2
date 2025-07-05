#include <algorithm>
#include <any>
#include <format>
#include <iterator>
#include <string_view>
#include <string>
#include <vector>

#include <BabyDI.h>
#include <CString.h>
#include <IUtil.h>

#include <FileSystem.h>
#include <Server.h>
#include <npcserver/NPCServer.h>
#include <scripting/IScriptEngine.h>
#include <scripting/Script.h>
#include <scripting/ScriptContainers.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

constexpr std::string_view clientSideTerminator = "//#CLIENTSIDE"sv;

//----------------------------

static std::string performClientSideJoinHack(std::string_view code)
{
	std::string result;
	std::vector<std::string_view> joins;

	size_t start = 0, end = 0;
	while (start < code.length())
	{
		// Find the next join.
		// If we don't find one, copy the rest of the code and break.
		end = code.find("join ", start);
		if (end == std::string_view::npos)
		{
			result += code.substr(start);
			break;
		}

		// Look for a newline or the start of a code block so we don't capture the word join in a string.
		bool join_is_start_of_block = true;
		if (end != 0)
		{
			size_t block_start = end - 1;
			while (block_start > 0)
			{
				// Skip any whitespace before the join.
				if (code[block_start] == ' ' || code[block_start] == '\t')
				{
					--block_start;
					continue;
				}
				// Look for the start of a block or a newline.
				else if (!(code[block_start] == '\n' || code[block_start] == '\xa7' || code[block_start] == '{'))
				{
					join_is_start_of_block = false;
					break;
				}

				// We found a new line or a block start.
				break;
			}
			if (!join_is_start_of_block)
			{
				result += code.substr(start, end);
				start = end + 5; // 5 = strlen("join ")
				continue;
			}
		}

		// Copy the code before the join.
		// Then, add a semi-colon.  We are going to remove the join entirely.
		result += code.substr(start, end - start);
		result += ";";

		// Get the name of the join.
		start = end + 5; // 5 = strlen("join ")
		end = code.find(";", start);
		if (end == std::string_view::npos)
			break;

		// Save the join to the list of joins.
		std::string_view join = string::trim(code.substr(start, end - start));
		if (!join.empty())
			joins.push_back(join);

		start = end + 1;
	}

	// Load the files and append them to the result.
	auto server = BabyDI::Get<Server>();
	if (server && server->hasNPCServer())
	{
		for (const auto& className : joins)
		{
			if (auto classObject = server->getNPCServer()->getClass(className).lock(); classObject != nullptr)
			{
				if (result.back() != '\n')
					result += '\n';
				result += classObject->getScript().getClientSide();
			}
		}
	}
	else
	{
		std::string classScript;
		for (const auto& fileName : joins)
		{
			classScript = Script::minify(server->getFileSystem()->load(std::format("{}.txt", fileName)).toString());
			string::replaceMutate(classScript, "\r", "");
			if (result.back() != '\n')
				result += '\n';
			result += classScript;
		}
	}

	return result;
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

void Script::executeEvents(ScriptContainer& container, ScriptObjectSource source) const
{
	return executeEvents(container.events, source);
}

void Script::executeEvents(ScriptEventQueue& events, ScriptObjectSource source) const
{
	if (m_server_script == nullptr || m_server_script->engine == nullptr)
		return;

	for (; !events.queue().empty(); events.queue().pop())
	{
		auto& event = events.queue().front();
		auto* engine = m_server_script->engine;
		engine->execute(event, source, m_server_script);
	}
}

std::string Script::minify(const std::string& src) noexcept
{
	if (src.empty())
		return src;

	std::string minified;
	std::string_view srcView{ src };

	// We don't want to trim serverside code so check if we have any.
	auto server = BabyDI::Get<Server>();
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
		size_t nextline = std::min(srcView.size(), newline + 1);

		// Search for \r and remove that too.
		if (newline > 0 && srcView[newline - 1] == '\r')
			--newline;

		// Extract the line.
		auto line = srcView.substr(0, newline);

		// Remove single-line comments.
		// But don't remove //# comments as those are directives.
		if (auto comment = line.find("//"); comment != std::string_view::npos)
		{
			if (comment + 2 < line.size() && line[comment + 2] != '#')
				line = line.substr(0, comment);
			else if (line.find(clientSideTerminator) != std::string_view::npos)
			{
				// If we have a clientside terminator, we are now in clientside code.
				inServerSide = false;
			}
		}

		// Trim the line.
		if (!hasServerSide || !inServerSide)
			line = string::trim(line);

		// Append the line to minified.
		if (!line.empty())
			minified.append(line).append("\n");

		// Move to the next line.
		srcView.remove_prefix(nextline);
	}

	// Remove multi-line comments from minified.
	std::string::size_type start = 0;
	while ((start = minified.find("/*", start)) != std::string::npos)
	{
		auto end = minified.find("*/", start);
		if (end == std::string::npos)
			break;
		minified.erase(start, end - start + 2);
	}

	// Final trim.
	string::trimMutate(minified);

	// Return the minified code.
	return minified;
}

void Script::split(std::string& source) noexcept
{
	auto determineClientSideLocation = [](std::string& source) -> std::string::iterator
	{
		auto clientside = source.begin();
		if (auto clientSep = source.find(clientSideTerminator); clientSep != std::string::npos)
			std::advance(clientside, clientSep);
		else clientside = source.end();
		return clientside;
	};

	// Check if we have an npc-server or not.
	// If we don't, we don't have serverside code, and thus we will ignore the clientside terminator.
	auto server = BabyDI::Get<Server>();
	bool hasServerSide = true;
	if (server && !server->hasNPCServer())
		hasServerSide = false;

	// If we have serverside code, find the start of the clientside terminator.
	// We need to mangle the newlines on just the clientside code.
	// The serverside code will be fed into a compiler so it should have normal line endings.
	auto clientside = source.begin();
	if (hasServerSide)
		clientside = determineClientSideLocation(source);

	// Do clientside script joins.
	if (server->getSettings().getBool("clientsidejoins", true) && clientside != source.end())
	{
		auto joinedScript = performClientSideJoinHack(std::string_view{ clientside, source.end() });
		source.replace(clientside, source.end(), joinedScript);
		clientside = determineClientSideLocation(source);
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
	if (auto clientSep = source.find(clientSideTerminator); clientSep != std::string::npos)
	{
		m_serverside = string::trim(std::string_view{ source }.substr(0, clientSep));
		m_clientside = string::trim(std::string_view{ source }.substr(clientSep + clientSideTerminator.size()));
	}
	else
	{
		m_serverside = string::trim(source);
		m_clientside = {};
	}

	m_client_script.reset();
	m_server_script.reset();

	if (server && server->hasNPCServer())
	{
		auto npcServer = server->getNPCServer();
		if (server->Generation == ServerGeneration::CLASSIC)
		{
			m_server_script = npcServer->scripting.getCompiledServerScript(m_who, m_serverside);
		}
		else if (server->Generation == ServerGeneration::NEWMAIN || server->Generation == ServerGeneration::MODERN)
		{
			m_client_script = npcServer->scripting.getCompiledClientScript(m_who, m_clientside);
			m_server_script = npcServer->scripting.getCompiledServerScript(m_who, m_serverside);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
