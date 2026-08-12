#ifndef LOG_H
#define LOG_H

#include <chrono>
#include <cstdint>
#include <format>
#include <memory>
#include <mutex>
#include <ratio>
#include <source_location>
#include <string_view>
#include <string>
#include <utility>
#include <version>

#include <CString.h>

#include <utilities/CommonTypes.h>

// Don't change this order.
// For some reason the compile will fail in Windows for some versions of MSVC.
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
//

using namespace std::literals::string_view_literals;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::log
{
///////////////////////////////////////////////////////////////////////////////

/// @brief The timestamp mode for the log.
enum class TimestampMode
{
	/// @brief No timestamp.
	None,

	/// @brief [HH:MM AM] (%I:%M %p)
	Short,

	/// @brief [yyyy-MM-dd HH:MM:SS] (%F %T)
	Long
};

/// @brief Timestamp format: short. Example: [12:34 PM]
inline constexpr auto TimestampShort = "[{0:%I}:{0:%M} {0:%p}]"sv;

/// @brief Timestamp format: long. Example: [2024-01-01 12:34:56]
inline constexpr auto TimestampLong = "[{0:%F} {0:%T}]"sv;

///////////////////////////////////////////////////////////////////////////////

struct IndentAbsolute_t { explicit IndentAbsolute_t() = default; };

/// @brief Absolute indentation tag for the Indent class.
inline constexpr IndentAbsolute_t IndentAbsolute{};

/// @brief Indentation helper for logging.
struct Log;
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
	/// @brief The path to the log file.
	std::filesystem::path filename;

	/// @brief A prefix to add at the start of each new line when indent level is 0. Useful for section headers.
	std::string sectionPrefix;

	/// @brief The number of spaces to add for each indentation level.
	uint8_t indentSpaces = 2;

	/// @brief The current indentation level. 0 means no indentation.
	uint8_t indentLevel = 0;

	/// @brief Specifies the timestamp mode for file operations.
	TimestampMode timestampFile = TimestampMode::Long;

	/// @brief Specifies the timestamp mode for console output.
	TimestampMode timestampCli = TimestampMode::Short;

	/// @brief Disables this log instance.  When disabled, no output will be written to the file or console.
	bool disabled = false;

	/// @brief Mirror output to the console as well as the file.
	bool mirrorToCli = true;

	/// @brief Indicates whether the next output is at the start of a new line. Used internally to determine when to add prefixes and indentation.
	bool atLineStart = true;

	/// @brief A unique pointer to an output file stream.
	std::unique_ptr<std::ofstream> file;

	/// @brief A mutex to synchronize access to the log file and related state.
	std::recursive_mutex mutex;

	/// @brief Reloads the log file (after a filename change).
	Log& reload();

	/// @brief Closes the log file.
	Log& close();

	/// @brief Clears the log file.
	Log& clear();

	/// @brief Gets the output file stream.
	/// @return A pointer to the output file stream.
	std::ofstream* getFile();

	/// @brief Creates an RAII indentation object.
	/// @param levels The number of indentation levels to add.
	Indent indent(const uint8_t levels = 1)
	{
		return {this, levels};
	}

	/// @brief Creates an RAII indentation object on an absolute indentation level.
	/// @param level The absolute indentation level.
	Indent indent_absolute(uint8_t level)
	{
		return {IndentAbsolute, this, level};
	}

	/// @brief Creates a unique pointer to an RAII indentation object.
	/// @param levels The number of indentation levels to add.
	std::unique_ptr<Indent> indent_ptr(const uint8_t levels = 1)
	{
		return std::make_unique<Indent>(this, levels);
	}
};

/// @brief The serverlog.txt file.
inline Log server{ .filename = std::filesystem::path{ "logs" } / "serverlog.txt", .sectionPrefix = ":: "s };

/// @brief The rclog.txt file.
inline Log rc{ .filename = std::filesystem::path{ "logs" } / "rclog.txt" };

/// @brief The npclog.txt file.
inline Log npc{ .filename = std::filesystem::path{ "logs" } / "npclog.txt" };

/// @brief The scriptlog.txt file.
inline Log script{ .filename = std::filesystem::path{ "logs" } / "scriptlog.txt" };

/// @brief The networkdump.txt file.
inline Log networkdump{ .filename = std::filesystem::path{ "logs" } / "networkdump.txt", .mirrorToCli = false };

///////////////////////////////////////////////////////////////////////////////

/// @brief Prints a message to the log file and console.
/// @tparam Args The types of the arguments to format.
/// @param log The log instance.
/// @param fmt The format string.
/// @param args The arguments to format.
template <typename ...Args>
void print(Log& log, const std::string_view fmt, const Args&... args)
{
	std::lock_guard lock(log.mutex);

	if (log.disabled)
		return;

	std::ostringstream text;

	// Add the section prefix.
	if (log.atLineStart && log.indentLevel == 0 && !log.sectionPrefix.empty() && !fmt.empty() && fmt != "\n")
		text << log.sectionPrefix;

	// Add the indentation whitespace.
	uint8_t spaces = 0;
	if (log.atLineStart && log.indentLevel != 0)
	{
		spaces = (log.indentSpaces * log.indentLevel);
		if (!log.sectionPrefix.empty())
			spaces += log.sectionPrefix.length();
		text << std::string(spaces, ' ');
	}

	// Output the message.
	if constexpr(sizeof...(args) == 0)
		text << fmt;
	else
		text << std::vformat(fmt, std::make_format_args(args...));

	// Get the resultant string.
	// If empty, don't log anything.
	const auto s = text.str();
	if (s.size() <= spaces)
		return;

#if __cpp_lib_chrono < 201907L
	// Clang doesn't support timezones, so just use system_clock time (UTC) floored to seconds.
	auto localtime = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
#else
	// Get the current time, floored to seconds.
	auto localtime = std::chrono::floor<std::chrono::seconds>(std::chrono::current_zone()->to_local(std::chrono::system_clock::now()));
#endif

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
		std::cout << s << std::flush;
	}

	log.atLineStart = s.back() == '\n';
}

/// @brief Prints a message to the log file and console and terminates the line.
/// @tparam Args The types of the arguments to format.
/// @param log The log instance.
/// @param fmt The format string.
/// @param args The arguments to format.
template <typename ...Args>
void printLine(Log& log, const std::string_view fmt, const Args&... args)
{
	std::lock_guard lock(log.mutex);

	if (log.disabled)
		return;

	print(log, fmt, args...);
	print(log, "\n"sv);
}

/// @brief Prints a message to the log file preventing trailing newlines from starting a new line.
/// @tparam Args The types of the arguments to format.
/// @param log The log instance.
/// @param fmt The format string.
/// @param args The arguments to format.
template <typename ...Args>
void printBlock(Log& log, const std::string_view fmt, const Args&... args)
{
	std::lock_guard lock(log.mutex);

	if (log.disabled)
		return;

	print(log, fmt, args...);
	log.atLineStart = false;
}

/// @brief Batches multiple log entries together to keep entries together when multiple threads are logging at the same time.
/// @param log The log instance.
/// @param range A range of pairs, where each pair consists of an indentation level and a log message. Each message will be printed with the specified indentation level.
void batch(Log& log, RangeOf<std::pair<uint8_t, std::string>> auto const& range)
{
	std::lock_guard lock(log.mutex);

	if (log.disabled)
		return;

	for (auto& [indentation, text] : range)
	{
		auto indent = log.indent(indentation);
		printLine(log, text);
	}
}

/// @brief Batches multiple log entries together to keep entries together when multiple threads are logging at the same time.
/// @param log The log instance.
/// @param range A range of pairs, where each pair consists of an indentation level and a log message. Each message will be printed with the specified indentation level.
void batch(Log& log, RangeOf<std::pair<uint8_t, std::string>> auto&& range)
{
	std::lock_guard lock(log.mutex);

	if (log.disabled)
		return;

	for (auto& [indentation, text] : range)
	{
		auto indent = log.indent(indentation);
		printLine(log, text);
	}
}

/// @brief Writes a message to the log if the condition is false, intended for debugging purposes where a failed assertion is not critical enough to throw an exception or terminate the program, but should still be logged for investigation.
/// @param condition The condition to check.
/// @param log The log instance.
/// @param message The message to log if the assertion fails.
/// @param location The source location of the assertion. Defaults to the current source location.
inline void debug_assert(const bool condition, Log& log, const std::string_view message, const std::source_location location = std::source_location::current())
{
	if (!condition)
		printLine(log, "[WARN][ASSERT] {} ({}:{})", message, location.file_name(), location.line());
}

/// @brief Writes a message to the log if the condition is false, intended for debugging purposes where a failed assertion is not critical enough to throw an exception or terminate the program, but should still be logged for investigation. Writes to log::server.
/// @param condition The condition to check.
/// @param message The message to log if the assertion fails.
/// @param location The source location of the assertion. Defaults to the current source location.
inline void debug_assert(const bool condition, const std::string_view message, const std::source_location location = std::source_location::current())
{
	if (!condition)
		printLine(log::server, "[WARN][ASSERT] {} ({}:{})", message, location.file_name(), location.line());
}

///////////////////////////////////////////////////////////////////////////////

/// @brief Profiles the duration of a scope and logs it when the scope ends.
struct Profile
{
	/// @brief Constructs a Profile object that measures execution time.
	/// @param log A reference to the Log object used for output.
	/// @param message A description of the operation being profiled.
	/// @param format The format string for the profiling output.
	Profile(Log& log, const std::string_view message, const std::string_view format = "[Profile] {} took {:0.6} ms.")
		: m_log(log), m_message(message), m_format(format), m_start(precise_clock::now())
	{}

	~Profile() noexcept
	{
		const auto end = precise_clock::now();
		const auto duration_ns = duration_nano_double(end - m_start);
		const auto duration_ms = std::chrono::duration_cast<duration_milli_double>(duration_ns);
		printLine(m_log, m_format, m_message, duration_ms.count());
	}

	Log& m_log;
	std::string m_message;
	std::string m_format;
	precise_clock::time_point m_start;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::log
///////////////////////////////////////////////////////////////////////////////

template <>
struct std::formatter<CString> : std::formatter<std::string>
{
	static auto format(const CString& str, std::format_context& ctx)
	{
		return std::format_to(ctx.out(), "{}", std::string_view{ str.text(), static_cast<size_t>(str.length()) });
	}
};

#endif // LOG_H
