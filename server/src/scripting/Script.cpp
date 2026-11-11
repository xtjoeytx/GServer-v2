#include <cctype>
#include <algorithm>
#include <any>
#include <array>
#include <format>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <numeric>
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

static std::string::iterator determineClientSideLocation(std::string& text)
{
	auto clientside = text.begin();
	if (const auto pos = text.find(clientSideTerminator); pos != std::string::npos)
		std::advance(clientside, pos);
	else
		clientside = text.end();

	return clientside;
}

//----------------------------

static std::string performClientSideJoinHack(std::string_view code)
{
	static constexpr std::array<char, 5> blockStarters = {'\n', '\xa7', ')', '{', ';'};
	static constexpr size_t joinKeywordLen = 5; // strlen("join ")

	std::string result;
	std::vector<std::string_view> joins;

	// Beginning of the actual code portion.
	size_t beginning = 0;
	if (string::trimLeft(code).starts_with("//#CLIENTSIDE"sv))
	{
		beginning = code.find_first_not_of(" \t", clientSideTerminator.length());
		result += code.substr(0, beginning);
	}

	size_t start = beginning;
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
				result += code.substr(start, (end + joinKeywordLen) - start);
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

			// Make sure we aren't in a say2 sign.
			// Work backwards to the last {, }, or ;, then check if this is a say2 command.
			auto blockpos = code.substr(beginning, start - beginning).find_last_of("{};");
			if (blockpos == std::string_view::npos)
				blockpos = beginning;
			else ++blockpos;
			if (const auto say2Command = code.substr(blockpos, start - blockpos); string::trim(say2Command).starts_with("say2"))
				inSign = true;

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

/// @brief Profiler for script execution time.
struct ExecutionProfiler
{
	precise_clock::time_point beginTime;
	ScriptExecutionContext* compiledScript;

	explicit ExecutionProfiler(ScriptExecutionContext* script)
		: beginTime(precise_clock::now()), compiledScript(script)
	{
	}

	ExecutionProfiler(const precise_clock::time_point begin, ScriptExecutionContext* script)
		: beginTime(begin), compiledScript(script)
	{
	}

	~ExecutionProfiler()
	{
		if (compiledScript == nullptr)
			return;

		auto duration = precise_clock::now() - beginTime;
		compiledScript->executionProfiles.emplace_front(beginTime, duration);

		// Add up durations until we reach 1 minute, then remove the rest.
		auto totalDuration = precise_clock::duration(0);
		const auto it = std::ranges::find_if(compiledScript->executionProfiles, [cutoff = beginTime - std::chrono::minutes(1), &totalDuration](const auto& d)
		{
			totalDuration += d.second;
			return totalDuration > std::chrono::minutes(1) || d.first < cutoff;
		});
		compiledScript->executionProfiles.erase(it, compiledScript->executionProfiles.end());
	}
};

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

	if (m_client_script == nullptr || m_client_script->script == nullptr || !m_client_script->script->has_value())
		return empty;

	if (auto* bytecode = std::any_cast<ScriptByteCode>(m_client_script->script.get()); bytecode != nullptr)   // the any holds the vector itself; the pointer form never throws
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
		ExecutionProfiler profiler{m_server_script.get()};
		engine->execute(queue.front(), source, m_server_script);
		return;
	}

	std::vector<ScriptEventType> types;
	std::vector<ScriptEventType> processedTypes;

	bool hadTimeout = false;
	ExecutionProfiler profiler{m_server_script.get()};
	for (auto& event : queue)
	{
		// Save timeouts for the very end.  Many things break unless you do that one last.
		if (event.type == ScriptEventType::TIMEOUT)
		{
			hadTimeout = true;
			continue;
		}

		// Execute events with args individually.
		if (!event.args.empty())
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
				// Don't consolidate:
				// - The same event type.
				// - Timeout events.
				// - Events that have a different initiator.
				// - Events that contain arguments.
				if (e.type != event.type && e.type != ScriptEventType::TIMEOUT && e.initiator == event.initiator && e.args.empty() && !std::ranges::contains(processedTypes, e.type))
				{
					types.emplace_back(e.type);
					processedTypes.emplace_back(e.type);
				}
			});

			engine->execute(event, &types, source, m_server_script);
			processedTypes.emplace_back(event.type);
		}
	}

	// Execute the timeout last.
	if (hadTimeout)
	{
		for (auto& event : queue)
		{
			if (event.type != ScriptEventType::TIMEOUT)
				continue;

			engine->execute(event, source, m_server_script);
			break;
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

	ExecutionProfiler profiler{m_server_script.get()};
	return m_server_script->engine->executeFunction(functionName, event, source, m_server_script);
}

//----------------------------

std::string Script::minify(const std::string& src) noexcept
{
	if (src.empty())
		return src;

	std::string minified;
	std::string_view srcView{src};

	// If we have an NPC-Server, then skip the serverside code.
	// We won't minify the code so that script errors have correct line numbers.
	if (const auto server = BabyDI::Get<Server>(); server && server->hasNPCServer())
	{
		// No clientside?  Just return the whole script.
		const auto clientsidePos = srcView.find(clientSideTerminator);
		if (clientsidePos == std::string_view::npos)
			return src;

		minified = srcView.substr(0, clientsidePos);
		srcView.remove_prefix(clientsidePos);
	}

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
	const auto server = BabyDI::Get<Server>();

	// Check if we have an npc-server or not.
	// If we don't, we don't have serverside code, and thus we will ignore the clientside terminator.
	bool hasServerSide = true;
	if (server && (!server->hasNPCServer() || server->cached.serverside.getValue() == false))
		hasServerSide = false;

	// If we have serverside code, find the start of the clientside terminator.
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
		auto endOfLine = source.find('\n', clientSep);
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

// GS1 client scripts cannot run in v6. tools/gs1_to_gs2.py translates them offline; the result sits in "gs2cache/<key>.gs2" (relative to the
// server folder), where <key> is the FNV-1a 64 hash (16 hex digits) of the client script with its line breaks restored and the ends trimmed.
static std::string gs1CacheKey(std::string_view src)
{
	std::string text{src};
	const auto first = text.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
		return {};
	const auto last = text.find_last_not_of(" \t\r\n");
	std::uint64_t h = 1469598103934665603ull;
	for (size_t i = first; i <= last; ++i)
	{
		h ^= static_cast<unsigned char>(text[i]);
		h *= 1099511628211ull;
	}
	return std::format("{:016x}", h);
}

// The translation of a GS1 client script, newlines mangled like the rest of the client-side code. Empty when there is none.
static const std::string& translatedClientScript(std::string_view original)
{
	// The client part of the ORIGINAL source (m_clientside has been minified, so it cannot be matched with the offline translation).
	std::string_view src;
	if (const auto sep = original.find(clientSideTerminator); sep != std::string_view::npos)
	{
		const auto eol = original.find('\n', sep);
		src = eol == std::string_view::npos ? std::string_view{} : original.substr(eol + 1);
	}
	else if (original.find("//#SERVERSIDE") == std::string_view::npos)
	{
		// A script without any marker (the old doors, lamps ...) was written for a client that also ran it: the whole text is the key.
		src = original;
	}
	static std::unordered_map<std::string, std::string> cache;
	static const std::string none;
	const auto key = gs1CacheKey(src);
	if (key.empty())
		return none;
	if (const auto it = cache.find(key); it != cache.end())
	{
				return it->second;
	}
	std::string out;
	// Not translated yet (a level uploaded after the last offline run): ask the translator once, the result is kept in gs2cache.
	// gs2cache/<key>.none means it was tried and gave nothing.
	if (!std::ifstream{"gs2cache/" + key + ".gs2", std::ios::binary} && !std::ifstream{"gs2cache/" + key + ".none", std::ios::binary} && !std::ifstream{"gs2cache/" + key + ".bad", std::ios::binary}
	    && std::ifstream{"gs2cache/gs1_to_gs2.py", std::ios::binary})
	{
		{
			std::ofstream in{"gs2cache/" + key + ".in", std::ios::binary};
			in << src;
		}
		const std::string cmd = "python3 -I gs2cache/gs1_to_gs2.py --one gs2cache/" + key + ".in --classes scripts > gs2cache/" + key + ".tmp 2>/dev/null";
		const int rc = std::system(cmd.c_str());
		std::error_code ec;
		std::filesystem::remove("gs2cache/" + key + ".in", ec);
		if (rc == 0 && std::filesystem::exists("gs2cache/" + key + ".tmp", ec) && std::filesystem::file_size("gs2cache/" + key + ".tmp", ec) > 0)
			std::filesystem::rename("gs2cache/" + key + ".tmp", "gs2cache/" + key + ".gs2", ec);
		else
		{
			std::filesystem::remove("gs2cache/" + key + ".tmp", ec);
			std::ofstream{"gs2cache/" + key + ".none"};
		}
	}
	if (std::ifstream in{"gs2cache/" + key + ".gs2", std::ios::binary}; in)
	{
		std::ostringstream ss;
		ss << in.rdbuf();
		std::string text = ss.str();
		// leave out the //#CLIENTSIDE line
		if (text.starts_with("//#CLIENTSIDE"))
		{
			const auto eol = text.find('\n');
			text = eol == std::string::npos ? std::string{} : text.substr(eol + 1);
		}
		std::replace(text.begin(), text.end(), '\n', '\xa7');
		out = string::trim(text);
	}
	return cache.emplace(key, std::move(out)).first->second;
}

// With a file gs2cache/CHECK present, every translation in gs2cache is compiled once (the way the server compiles it for a client) and the ones the
// GS2 compiler refuses are renamed to <key>.bad; their keys go to gs2cache/CHECK.out. A script that does not compile is simply missing in the game.
static void checkGs2Cache(ScriptSystem& scripting)
{
	static bool done = false;
	if (done || !std::filesystem::exists("gs2cache/CHECK"))
		return;
	done = true;
	std::ofstream report{"gs2cache/CHECK.out", std::ios::trunc};
	size_t total = 0, bad = 0;
	std::error_code ec;
	std::vector<std::filesystem::path> files;
	for (const auto& entry : std::filesystem::directory_iterator{"gs2cache", ec})
		if (entry.path().extension() == ".gs2")
			files.push_back(entry.path());
	for (const auto& path : files)
	{
		std::ifstream in{path, std::ios::binary};
		std::ostringstream ss;
		ss << in.rdbuf();
		std::string text = ss.str();
		if (text.starts_with("//#CLIENTSIDE"))
		{
			const auto eol = text.find('\n');
			text = eol == std::string::npos ? std::string{} : text.substr(eol + 1);
		}
		std::replace(text.begin(), text.end(), '\n', '\xa7');
		++total;
		if (scripting.getCompiledClientScript(path.stem().string(), string::trim(text)) == nullptr)
		{
			++bad;
			report << path.stem().string() << "\n";
			auto badPath = path;
			badPath.replace_extension(".bad");
			std::filesystem::rename(path, badPath, ec);
		}
	}
	report << "checked " << total << ", refused " << bad << "\n";
	std::filesystem::remove("gs2cache/CHECK", ec);
}

// Does this client-side source look like GS2? v6 clients cannot run GS1, and feeding GS1 to the GS2 compiler only printed one "missing semicolon"
// error per NPC. GS1 has functions of its own ("function clear() { ... }"), so the word "function" alone is no sign. Signs of GS2: a "//#GS2"
// marker, "new Gui...", "public function", an event function ("function onCreated"), or a control handler ("function Name.onAction").
// (Newlines are mangled to \xa7 in client-side code.)
static bool looksLikeGs2(std::string_view src) noexcept
{
	if (src.empty())
		return false;
	if (src.find("//#GS2") != std::string_view::npos || src.find("new Gui") != std::string_view::npos)
		return true;

	auto isSpace = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\xa7'; };
	auto isIdent = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.'; };

	constexpr std::string_view kw = "function";
	for (auto pos = src.find(kw); pos != std::string_view::npos; pos = src.find(kw, pos + 1))
	{
		if (pos > 0 && (std::isalnum(static_cast<unsigned char>(src[pos - 1])) || src[pos - 1] == '_'))
			continue;

		// "public function"
		auto back = pos;
		while (back > 0 && isSpace(src[back - 1]))
			--back;
		if (back >= 6 && src.substr(back - 6, 6) == "public" && back != pos)
			return true;

		// The name after "function".
		auto i = pos + kw.size();
		const auto afterKw = i;
		while (i < src.size() && isSpace(src[i]))
			++i;
		if (i == afterKw)
			continue;
		const auto nameStart = i;
		while (i < src.size() && isIdent(src[i]))
			++i;
		const auto name = src.substr(nameStart, i - nameStart);
		if (name.size() > 2 && ((name[0] == 'o' && name[1] == 'n') || name.find('.') != std::string_view::npos))
			return true;
	}
	return false;
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
		// Mangle the clientside line terminators.
		const auto clientside = determineClientSideLocation(m_modified_source);
		std::replace(clientside, m_modified_source.end(), '\n', '\xa7');

		// Compile serverside scripts.
		m_server_script = npcServer->scripting.getCompiledServerScript(m_who, m_serverside);
		if (m_server_script != nullptr && m_server_script->identifier.empty())
			m_server_script->identifier = m_who.empty() ? "(server:unnamed)" : m_who;
	}
	else if (server->Generation == ServerGeneration::MODERN)
	{
		checkGs2Cache(npcServer->scripting);
		if (looksLikeGs2(m_clientside))
		{
			m_client_script = npcServer->scripting.getCompiledClientScript(m_who, m_clientside);
		}
		else
		{
			if (const auto& translated = translatedClientScript(m_original_source); !translated.empty())
				m_client_script = npcServer->scripting.getCompiledClientScript(m_who, translated);
		}
		// The server script engine is chosen by getCompiledServerScript (GS1 server scripts such as the doors run in the GS1 engine).
		m_server_script = npcServer->scripting.getCompiledServerScript(m_who, m_serverside);
		if (m_client_script != nullptr && m_client_script->identifier.empty())
			m_client_script->identifier = m_who.empty() ? "(client:unnamed)" : m_who;
		if (m_server_script != nullptr && m_server_script->identifier.empty())
			m_server_script->identifier = m_who.empty() ? "(server:unnamed)" : m_who;

		// If we failed to compile the script, remove the clientside code so it doesn't get sent raw.
		if (m_client_script == nullptr)
			m_clientside = {};
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
