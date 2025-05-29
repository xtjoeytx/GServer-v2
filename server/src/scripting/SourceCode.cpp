#include <scripting/SourceCode.h>

#include <BabyDI.h>

#include <Server.h>
#include <npcserver/NPCServer.h>
#include <scripting/IScriptEngine.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

const ScriptByteCode& SourceCode::getClientByteCode() const noexcept
{
	static ScriptByteCode empty;

	if (m_client_script == nullptr || !m_client_script->script->has_value())
		return empty;

	if (auto* bytecode = std::any_cast<ScriptByteCode*>(m_client_script->script); bytecode != nullptr)
		return *bytecode;

	return empty;
}

void SourceCode::executeEvents(ScriptContainer& container, ScriptObjectSource source) const
{
	return executeEvents(container.events, source);
}

void SourceCode::executeEvents(ScriptEventQueue& events, ScriptObjectSource source) const
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

std::string SourceCode::minify(const std::string& src) noexcept
{
	if (src.empty())
		return src;

	std::string minified;

	// Trim the lines.
	std::string_view srcView{ src };
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
		}

		// Trim the line.
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

	// Return the minified code.
	return minified;
}

void SourceCode::split(std::string& source) noexcept
{
	static constexpr std::string_view clientSideTerminator = "//#CLIENTSIDE"sv;

	// Check if we have an npc-server or not.
	// If we don't, we don't have serverside code, and thus we will ignore the clientside terminator.
	auto server = BabyDI::Get<Server>();
	bool hasServerSide = true;
	if (server && !server->isNpcServerEnabled())
		hasServerSide = false;

	// If we have serverside code, find the start of the clientside terminator.
	// We need to mangle the newlines on just the clientside code.
	// The serverside code will be fed into a compiler so it should have normal line endings.
	auto clientside = source.begin();
	if (hasServerSide)
	{
		if (auto clientSep = source.find(clientSideTerminator); clientSep != std::string::npos)
			std::advance(clientside, clientSep);
		else clientside = source.end();
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
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
