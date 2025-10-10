#ifndef FILE_H
#define FILE_H

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <generator>
#include <ios>
#include <istream>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

using namespace std::literals;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

/// @brief Creates a string of the same native type as std::filesystem::path from a string literal using a user-defined literal operator.
/// @param chars Pointer to the character array representing the string literal.
/// @param length The length of the string literal.
/// @return A std::filesystem::path::string_type constructed from the given character array.
constexpr std::filesystem::path::string_type operator ""_pv(const char* chars, size_t length)
{
	return std::filesystem::path::string_type{ chars, chars + length };
}

///////////////////////////////////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::fs
{
///////////////////////////////////////////////////////////////////////////////

/// @brief Returns the filename component of a filesystem path as an ANSI encoded std::string.
/// @param file The filesystem path from which to extract the filename.
/// @return A std::string containing the filename part of the path, encoded in ANSI.
std::string getANSIFileName(const std::filesystem::path& file);

/// @brief Returns an HTML escaped version of the specified file name.
/// @param file The file to escape.
/// @return The escaped file name.
std::filesystem::path getHTMLEscapedFileName(const std::filesystem::path& file);

//----------------------------

class File
{
protected:
	File() {}

public:
	File(const std::filesystem::path& file, std::unique_ptr<std::ifstream>&& stream)
		: m_file(file), m_inputStreamHandle(std::move(stream))
	{
	}

	File(const std::filesystem::path& file) : m_file(file)
	{
		open();
	}

	File(File&& other) noexcept
	{
		std::swap(m_file, other.m_file);
		std::swap(m_inputStreamHandle, other.m_inputStreamHandle);
		std::swap(m_inputStream, other.m_inputStream);
	}

	virtual ~File()
	{
		close();
	}

public:
	File(const File& other) = delete;
	File& operator=(const File& other) = delete;
	bool operator==(const File& other) = delete;

public:
	virtual File& operator=(File&& other) noexcept
	{
		std::swap(m_file, other.m_file);
		std::swap(m_inputStreamHandle, other.m_inputStreamHandle);
		std::swap(m_inputStream, other.m_inputStream);
		return *this;
	}

public:
	/// @brief Converts directly into an istream.
	virtual operator std::istream& ()
	{
		return *m_inputStream;
	}

	/// @brief Converts directly into a shared pointer to the istream.
	virtual operator std::istream* ()
	{
		return m_inputStream;
	}

	/// @brief Returns if this is a valid file.
	virtual operator bool() const
	{
		return opened();
	}

public:
	/// @brief Opens the file.
	/// @return If the file was successfully opened.
	virtual bool open();

	/// @brief Closes the file.
	virtual void close();

	/// @brief Tells us if the file is opened.
	/// @return If the file is opened or not.
	virtual bool opened() const;

public:
	/// @brief Reads the position indicator of the file.
	/// @return The position indicator.
	virtual std::streampos getStreamPosition() const;

	/// @brief Sets the position indicator of the file.
	/// @param position The position in the file.
	virtual File& setStreamPosition(const std::streampos& position);

	/// @brief Sets the position indicator of the file.
	/// @param offset The offset for our new read position.
	/// @param origin Where we calculate the offset from.
	virtual File& setStreamPosition(const std::streamoff& offset, const std::ios_base::seekdir origin = std::ios_base::beg);

	/// @brief Gets the file size.
	/// @return The file size.
	virtual uintmax_t size() const
	{
		return std::filesystem::file_size(m_file);
	}

	/// @brief Gets the path to the file.
	/// @return The path to the file.
	const std::filesystem::path& filePath() const
	{
		return m_file;
	}

	/// @brief Gets the file modified time.
	/// @return The file modified time.
	virtual std::filesystem::file_time_type modifiedTime() const
	{
		return std::filesystem::last_write_time(m_file);
	}

public:
	/// @brief Reads the full file.
	/// @return The file contents.
	virtual std::vector<char> read();

	/// @brief Reads part of a file.
	/// @param count How many bytes to read.
	/// @return The file contents.
	virtual std::vector<char> read(std::size_t count);

	/// @brief Reads from the file until it encounters the token.
	virtual std::vector<char> readUntil(std::string_view delimiter);

	/// @brief Reads the file into a string.
	/// @return The file contents.
	virtual std::string readAsString();

	/// @brief Reads a line from the file.
	/// @return A string containing a single line, excluding the line ending.
	virtual std::string readLine();

	/// @brief Reads the value of a configuration entry for a given key and seeks back to the beginning of the file.
	/// @param key The key identifying the configuration entry to read.
	/// @return The value associated with the specified key as a string, or a std::nullopt if it doesn't exist.
	virtual std::optional<std::string> readConfigLine(std::string_view key, std::string_view separator = "="sv);

	/// @brief Reads a configuration section between the specified start and end keys and seeks back to the beginning of the file.
	/// @param startKey The key indicating the start of the configuration section to read.
	/// @param endKey The key indicating the end of the configuration section to read.
	/// @return An optional string containing the configuration section if found; std::nullopt if the section does not exist.
	virtual std::optional<std::string> readConfigSection(std::string_view startKey, std::string_view endKey);

	/// @brief Returns a generator that yields all lines as strings.
	/// @return A generator that produces each line as a std::string.
	std::generator<std::string> readAllLines();

	/// @brief Reads data into a buffer.
	/// @param buffer The buffer to fill.
	/// @param count How many bytes to read.
	/// @return How much bytes were actually read.
	virtual size_t readIntoBuffer(uint8_t* buffer, size_t count);

	/// @brief Tells us if we finished reading the file.
	/// @return If we finished reading the file or not.
	virtual bool finishedReading() const;

protected:
	std::filesystem::path m_file;
	std::istream* m_inputStream{ nullptr };
	std::unique_ptr<std::ifstream> m_inputStreamHandle;
};

using FilePtr = std::shared_ptr<File>;

//----------------------------

class FileIO : public File
{
public:
	FileIO(const std::filesystem::path& file, std::unique_ptr<std::fstream>&& stream)
		: m_outputStreamHandle(std::move(stream))
	{
		m_file = file;
	}

	FileIO(const std::filesystem::path& file)
	{
		m_file = file;
		open();
	}

	FileIO(FileIO&& other) noexcept
	{
		std::swap(m_file, other.m_file);
		std::swap(m_inputStream, other.m_inputStream);
		std::swap(m_outputStreamHandle, other.m_outputStreamHandle);
	}

	virtual ~FileIO()
	{
		close();
	}

public:
	FileIO() = delete;
	FileIO(const FileIO& other) = delete;
	FileIO& operator=(const FileIO& other) = delete;
	bool operator==(const FileIO& other) = delete;

public:
	virtual File& operator=(File&& other) noexcept override
	{
		if (auto fileIO = dynamic_cast<FileIO*>(&other))
			std::swap(m_outputStreamHandle, fileIO->m_outputStreamHandle);
		return File::operator=(std::move(other));
	}

	FileIO& operator=(FileIO&& other) noexcept
	{
		std::swap(m_file, other.m_file);
		std::swap(m_inputStream, other.m_inputStream);
		std::swap(m_outputStreamHandle, other.m_outputStreamHandle);
		return *this;
	}

public:
	/// @brief Converts directly into an fstream.
	operator std::fstream& () const
	{
		return *(m_outputStreamHandle.get());
	}

	/// @brief Converts directly into a shared pointer to the istream.
	operator std::fstream* () const
	{
		return m_outputStreamHandle.get();
	}

public:
	/// @brief Opens the file.
	/// @return If the file was successfully opened.
	virtual bool open() override;

	/// @brief Closes the file.
	virtual void close() override;

	/// @brief Tells us if the file is opened.
	/// @return If the file is opened or not.
	virtual bool opened() const override;

public:
	/// @brief Clears the contents of the file.
	/// @return A reference to the FileIO object after clearing.
	FileIO& clear();

	/// @brief Writes the contents of the buffer to the file.
	/// @param buffer A span containing the data to write to the file.
	/// @return A reference to the FileIO object, allowing for method chaining.
	FileIO& write(std::span<const uint8_t> buffer);

	/// @brief Writes the contents of the buffer to the file.
	/// @param buffer A span containing the data to write to the file.
	/// @return A reference to the FileIO object after writing.
	FileIO& write(std::span<const char> buffer);

	/// @brief Writes a blank line to the file.
	/// @return A reference to the FileIO object after writing the line.
	FileIO& writeLine();

	/// @brief Writes a line of text to the file.
	/// @param line A span containing the characters to write as a line.
	/// @return A reference to the FileIO object, allowing for method chaining.
	FileIO& writeLine(std::span<const char> line);

	/// @brief Writes multiple lines to the file output stream.
	/// @param lines A range of lines to write to the file.
	/// @return A reference to the FileIO object after writing the lines.
	FileIO& writeLines(std::ranges::input_range auto&& lines)
	{
		for (const auto& line : lines)
			writeLine(line);
		return *this;
	}

	/// @brief Writes a configuration line consisting of a key, a separator, and a value to the file.
	/// @param key A span containing the key to write.
	/// @param value A span containing the value to write.
	/// @param separator A span containing the separator to use between the key and value. Defaults to a single space.
	/// @return A reference to the FileIO object after writing the configuration line.
	FileIO& writeConfigLine(std::span<const char> key, std::span<const char> value, std::span<const char> separator = { " "sv });

	/// @brief Writes a configuration section between specified start and end keys to a file.
	/// @param startKey A span representing the starting key of the configuration section.
	/// @param endKey A span representing the ending key of the configuration section.
	/// @param section A span containing the configuration section data to write.
	/// @return A reference to the FileIO object after writing the configuration section.
	FileIO& writeConfigSection(std::span<const char> startKey, std::span<const char> section, std::span<const char> endKey);

	/// @brief Flushes the output buffer, ensuring all pending data is written to the file.
	/// @return A reference to the FileIO object after flushing.
	FileIO& flush();

protected:
	bool open(bool truncate);

protected:
	std::unique_ptr<std::fstream> m_outputStreamHandle;
};

using FileIOPtr = std::shared_ptr<FileIO>;

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::fs

#endif // FILE_H
