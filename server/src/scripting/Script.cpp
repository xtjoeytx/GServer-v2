#include <algorithm>
#include <any>
#include <array>
#include <format>
#include <iterator>
#include <numeric>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <tomcrypt.h>
#include <zlib.h>

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
#include <scripting/gs1/ScriptEngineGS1.h>
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
				result += classObject->getScript().getClientSideSource();
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

namespace
{
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
} // namespace

//----------------------------

// NOLINTNEXTLINE(*-no-recursion)
std::generator<decltype(ScriptExecutionContext::joinedClasses)::const_reference> Script::getServerJoinedClasses() const noexcept
{
	if (m_server_compiled == nullptr)
		co_return;

	for (const auto& kvp : m_server_compiled->joinedClasses)
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

const ScriptByteCode& Script::getClientSideByteCode() const noexcept
{
	static ScriptByteCode empty;

	if (m_client_compiled == nullptr || m_client_compiled->script == nullptr || !m_client_compiled->script->has_value())
		return empty;

	if (auto* bytecode = std::any_cast<ScriptByteCode>(m_client_compiled->script.get()); bytecode != nullptr)
		return *bytecode;

	return empty;
}

void Script::calculateClientByteCodeChecksums(std::string& outDesKeyPacket, uint32_t& outPlainChecksum, uint32_t& outEncryptedChecksum) const noexcept
{
	const auto& script = getOriginalSource();
	const auto& bytecode = getClientSideByteCode();
	if (bytecode.empty())
		return;

	// Set the cryptographic key to be the script's hash.
	// This will become the DES encryption key the client will use to encrypt the bytecode.
	constexpr string::string_hash hash{};
	auto scriptHash = static_cast<uint64_t>(hash(script));

	// Package the key into two GYBTE5's.
	const auto hashBytes = reinterpret_cast<uint32_t*>(&scriptHash);
	const CString key = CString() >> (long long)(hashBytes[0]) >> (long long)(hashBytes[1]);
	outDesKeyPacket = key.toString();

	// CRC32 checksum the bytecode.
	outPlainChecksum = crc32(0L, Z_NULL, 0);
	outPlainChecksum = crc32(outPlainChecksum, bytecode.data(), static_cast<uint32_t>(bytecode.size()));

	ScriptByteCode encrypt{bytecode};
	{
		// Convert the hash into individual bytes.
		std::array<uint8_t, 8> keyBytes{};
		for (size_t i = 0; i < 4; ++i)
		{
			keyBytes[i] = static_cast<uint8_t>(hashBytes[0] >> (8 * i));
			keyBytes[i + 4] = static_cast<uint8_t>(hashBytes[1] >> (8 * i));
		}

		// Reverse the bits.
		// The client has a non-standard DES method that reads every byte of the key in a bit-reversed way.
		for (size_t i = 0; i < 8; ++i)
		{
			uint8_t& b = keyBytes[i];
			b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
			b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
			b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
		}

		// Encrypt the bytecode.
		symmetric_key skey;
		if (des_setup(keyBytes.data(), static_cast<int>(keyBytes.size()), 0, &skey) == CRYPT_OK)
		{
			for (size_t i = 0; i + 8 <= encrypt.size(); i += 8)
				des_ecb_encrypt(&encrypt[i], &encrypt[i], &skey);
			des_done(&skey);
		}
	}

	outEncryptedChecksum = crc32(0L, Z_NULL, 0);
	outEncryptedChecksum = crc32(outEncryptedChecksum, encrypt.data(), static_cast<uint32_t>(encrypt.size()));
}

//----------------------------

void Script::executeEvents(ScriptContainer& container, const ScriptObject& source) const
{
	executeEvents(container.events, source);
}

void Script::executeEvents(ScriptEventQueue& events, const ScriptObject& source) const
{
	if (m_server_compiled == nullptr || m_server_compiled->engine == nullptr)
		return;

	auto* engine = m_server_compiled->engine;
	auto& queue = events.queue();

	if (queue.size() == 1)
	{
		ExecutionProfiler profiler{m_server_compiled.get()};
		engine->execute(queue.front(), source, m_server_compiled);
		return;
	}

	std::vector<ScriptEventType> types;
	std::vector<ScriptEventType> processedTypes;

	bool hadTimeout = false;
	ExecutionProfiler profiler{m_server_compiled.get()};
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
			engine->execute(event, source, m_server_compiled);
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

			engine->execute(event, &types, source, m_server_compiled);
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

			engine->execute(event, source, m_server_compiled);
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
	if (m_server_compiled == nullptr || m_server_compiled->engine == nullptr)
		return false;

	ExecutionProfiler profiler{m_server_compiled.get()};
	return m_server_compiled->engine->executeFunction(functionName, event, source, m_server_compiled);
}

//----------------------------

std::string Script::minify(const std::string& src) noexcept
{
	if (src.empty())
		return src;

	std::string minified;
	std::string_view srcView{src};

	bool onlyRemoveCarriageReturn = false;

	// If we have an NPC-Server, then skip the serverside code.
	// We won't minify the code so that script errors have correct line numbers.
	// Also, skip the clientside code if our default script engine isn't GS1.
	if (const auto server = BabyDI::Get<Server>(); server && server->hasNPCServer())
	{
		// No clientside?  Just return the whole script.
		const auto clientsidePos = srcView.find(clientSideTerminator);
		if (clientsidePos == std::string_view::npos)
			onlyRemoveCarriageReturn = true;

		// Default clientside script engine is not GS1?  Return it all.
		if (server->getNPCServer()->scripting.defaultScriptEngineClient != gs1::ScriptEngineGS1::EngineName)
			onlyRemoveCarriageReturn = true;

		// Extract the clientside portion, unless we are only going to remove carriage returns.
		if (!onlyRemoveCarriageReturn)
		{
			minified = srcView.substr(0, clientsidePos);
			srcView.remove_prefix(clientsidePos);
		}
	}

	// If we are only removing the carriage returns, do so now.
	// We are doing this even though the script engines could handle it because we want to special-case sparringzone later.
	// If there are carriage returns in the clientside script, it can cause problems when we mangle.
	if (onlyRemoveCarriageReturn)
		return string::eraseChars(src, "\r"sv);

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
			nextLine = srcView.size();
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
		m_serverside_source = {};
		m_clientside_source = string::trim(source);
		m_clientside_script = m_clientside_source;
		return;
	}

	// Split the code into clientside and serverside.
	if (const auto clientSep = source.find(clientSideTerminator); clientSep != std::string::npos)
	{
		auto endOfLine = source.find('\n', clientSep);
		if (endOfLine == std::string::npos)
			endOfLine = clientSep + clientSideTerminator.size();

		m_serverside_source = string::trim(std::string_view{source}.substr(0, clientSep));
		m_clientside_source = {};

		if (endOfLine + 1 < source.size())
			m_clientside_source = string::trim(std::string_view{source}.substr(endOfLine + 1));
	}
	else
	{
		m_serverside_source = string::trim(source);
		m_clientside_source = {};
	}
}

void Script::compileScript() noexcept
{
	m_client_compiled.reset();
	m_server_compiled.reset();

	const auto server = BabyDI::Get<Server>();
	if (server == nullptr)
		return;

	// If we have no NPC-Server, just mangle the script and return.
	if (!server->hasNPCServer())
	{
		// Mangle the clientside line terminators.
		std::ranges::replace(m_modified_source, '\n', '\xa7');
		m_clientside_script = m_clientside_source;
		return;
	}

	const auto npcServer = server->getNPCServer();
	if (server->Generation == ServerGeneration::NEWMAIN)
	{
		// Mangle the clientside line terminators.
		const auto clientside = determineClientSideLocation(m_modified_source);
		std::replace(clientside, m_modified_source.end(), '\n', '\xa7');
		m_clientside_script = m_clientside_source;

		// Compile serverside scripts.
		m_server_compiled = npcServer->scripting.getCompiledServerScript(m_who, m_serverside_source);
		if (m_server_compiled != nullptr && m_server_compiled->identifier.empty())
			m_server_compiled->identifier = m_who.empty() ? "(server:unnamed)" : m_who;
	}
	else if (server->Generation == ServerGeneration::MODERN)
	{
		// Special case sparringzone.
		// Even though modern servers support clientside GS2, a sparringzone script is still kept as-is and sent with NPCProp::SCRIPT.
		const bool isSparringZone = m_clientside_source == "sparringzone" || m_clientside_source == "//#CLIENTSIDE\nsparringzone";

		if (!isSparringZone) m_client_compiled = npcServer->scripting.getCompiledClientScript(m_who, m_clientside_source);
		m_server_compiled = npcServer->scripting.getCompiledServerScript(m_who, m_serverside_source);
		if (m_client_compiled != nullptr && m_client_compiled->identifier.empty())
			m_client_compiled->identifier = m_who.empty() ? "(client:unnamed)" : m_who;
		if (m_server_compiled != nullptr && m_server_compiled->identifier.empty())
			m_server_compiled->identifier = m_who.empty() ? "(server:unnamed)" : m_who;

		// If we failed to compile the script, remove the clientside code so it doesn't get sent raw.
		if (m_client_compiled == nullptr)
		{
			// Unless we are defaulting to GS1 clientside script, or if it is a sparringzone.
			// If so, mangle the line terminators and set the script to the source.
			if (isSparringZone || npcServer->scripting.defaultScriptEngineClient == gs1::ScriptEngineGS1::EngineName)
			{
				const auto clientside = determineClientSideLocation(m_modified_source);
				std::replace(clientside, m_modified_source.end(), '\n', '\xa7');
				m_clientside_script = m_clientside_source;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
