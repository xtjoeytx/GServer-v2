#ifndef LOG_H
#define LOG_H

#include <cstdint>
#include <string>
#include <string_view>
#include <format>
#include <chrono>
#include <memory>
#include <mutex>

#include "CString.h"

// Don't change this order.
// For some reason the compile will fail in Windows for some versions of MSVC.
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
//

///////////////////////////////////////////////////////////////////////////////

using namespace std::literals::string_view_literals;

///////////////////////////////////////////////////////////////////////////////

namespace graal::utilities::log
{

// The timestamp mode for the log.
enum class TimestampMode
{
	None,

	// [HH:MM AM] (%I:%M %p)
	Short,

	// [yyyy-MM-dd HH:MM:SS] (%F %T)
	Long
};

inline constexpr std::string_view TimestampShort = "[{0:%I}:{0:%M} {0:%p}]"sv;
inline constexpr std::string_view TimestampLong = "[{0:%F} {0:%T}]"sv;

///////////////////////////////////////////////////////////////////////////////

struct IndentAbsolute_t { explicit IndentAbsolute_t() = default; };
inline constexpr IndentAbsolute_t IndentAbsolute{};

// Indentation helper for logging.
class Log;
struct Indent
{
	Indent(Log* log, uint8_t level);
	Indent(IndentAbsolute_t is_absolute, Log* log, uint8_t level);
	Indent(Indent&& other) noexcept;
	~Indent() noexcept;

private:
	Log* m_log = nullptr;
	uint8_t m_old_level = 0;
};

///////////////////////////////////////////////////////////////////////////////

// Instance of a log file.
struct Log
{
	std::filesystem::path filename;
	uint8_t indentSpaces = 2;
	uint8_t indentLevel = 0;
	uint8_t indentAdditionalSpaces = 3;
	TimestampMode timestampFile = TimestampMode::Long;
	TimestampMode timestampCli = TimestampMode::Short;
	bool mirrorToCli = true;
	bool atLineStart = true;
	std::unique_ptr<std::ofstream> file;
	std::recursive_mutex mutex;

	// Reloads the log file (after a filename change).
	Log& reload();

	// Closes the log file.
	Log& close();

	// Clears the log file.
	Log& clear();

	// Gets the output file stream.
	std::ofstream* getFile();

	// Creates an RAII indentation object.
	Indent indent(uint8_t levels = 1)
	{
		return Indent(this, levels);
	}

	// Creates an RAII indentation object on an absolute indentation level.
	Indent indent_absolute(uint8_t level)
	{
		return Indent(IndentAbsolute, this, level);
	}

	// Creates a unique pointer to an RAII indentation object.
	std::unique_ptr<Indent> indent_ptr(uint8_t levels = 1)
	{
		return std::make_unique<Indent>(this, levels);
	}
};

// The serverlog.txt file.
inline static Log server{ .filename = std::filesystem::path{ "logs" } / "serverlog.txt" };

// The rclog.txt file.
inline static Log rc{ .filename = std::filesystem::path{ "logs" } / "rclog.txt" };

// The npclog.txt file.
inline static Log npc{ .filename = std::filesystem::path{ "logs" } / "npclog.txt" };

// The scriptlog.txt file.
inline static Log script{ .filename = std::filesystem::path{ "logs" } / "scriptlog.txt" };

///////////////////////////////////////////////////////////////////////////////

// Prints a message to the log file and console.
template <typename ...Args>
void print(Log& log, std::string_view fmt, const Args&... args)
{
	std::lock_guard lock(log.mutex);
	std::ostringstream text;

	// Add the indentation whitespace.
	uint8_t spaces = 0;
	if (log.atLineStart && log.indentLevel != 0)
	{
		spaces = log.indentAdditionalSpaces + (log.indentSpaces * log.indentLevel);
		text << std::string(spaces, ' ');
	}

	// Output the message.
	text << std::vformat(fmt, std::make_format_args(args...));

	// Get the resultant string.
	// If empty, don't log anything.
	auto s = text.str();
	if (s.size() <= spaces)
		return;

	// Get the current time, floored to seconds.
	auto localtime = std::chrono::floor<std::chrono::seconds>(std::chrono::zoned_time{ std::chrono::current_zone(), std::chrono::system_clock::now() }.get_local_time());

	// Output to file.
	if (auto* logFile = log.getFile(); logFile && logFile->is_open())
	{
		if (log.atLineStart && log.timestampFile == TimestampMode::Short)
			*logFile << std::format(TimestampShort, localtime) << ' ';
		else if (log.atLineStart && log.timestampFile == TimestampMode::Long)
			*logFile << std::format(TimestampLong, localtime) << ' ';

		*logFile << s;
		logFile->flush();
	}

	// Output to console.
	if (log.mirrorToCli)
	{
		if (log.atLineStart && log.timestampCli == TimestampMode::Short)
			std::cout << std::format(TimestampShort, localtime) << ' ';
		else if (log.atLineStart && log.timestampCli == TimestampMode::Long)
			std::cout << std::format(TimestampLong, localtime) << ' ';
		std::cout << s;
	}

	log.atLineStart = s.back() == '\n';
}

// Prints a message to the log file and console and terminates the line.
template <typename ...Args>
void printLine(Log& log, std::string_view fmt, const Args&... args)
{
	std::lock_guard lock(log.mutex);
	print(log, fmt, args...);
	print(log, "\n"sv);
}

} // end namespace graal::utilities::log

///////////////////////////////////////////////////////////////////////////////

template <>
struct std::formatter<CString> : std::formatter<std::string>
{
	auto format(const CString& str, std::format_context& ctx) const
	{
		return std::format_to(ctx.out(), "{}", std::string_view{ str.text(), static_cast<size_t>(str.length()) });
	}
};

#endif // LOG_H
