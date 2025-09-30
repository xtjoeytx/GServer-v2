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
#include <string_view>
#include <string>
#include <utility>
#include <vector>

using namespace std::literals;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::fs
{
///////////////////////////////////////////////////////////////////////////////

class File
{
public:
	File() = delete;
	File(const std::filesystem::path& file, std::unique_ptr<std::ifstream>&& stream) : m_file(file), m_stream(std::move(stream)) {};

	File(const std::filesystem::path& file) : m_file(file)
	{
		open();
	};

	virtual ~File()
	{
		close();
	}

	File(const File& other) = delete;
	File& operator=(const File& other) = delete;

	File(File&& other) noexcept : m_file(std::move(other.m_file))
	{
		m_stream.swap(other.m_stream);
	}

public:
	virtual File& operator=(File&& other) noexcept
	{
		std::swap(m_file, other.m_file);
		std::swap(m_stream, other.m_stream);
		return *this;
	}

	bool operator==(const File& other) = delete;

public:
	/// @brief Converts directly into an istream.
	virtual operator std::istream&() const
	{
		return *(m_stream.get());
	}

	/// @brief Converts directly into a shared pointer to the istream.
	virtual operator std::istream*() const
	{
		return m_stream.get();
	}

	/// @brief Returns if this is a valid file.
	virtual operator bool() const
	{
		return opened();
	}

	/// @brief Opens the file.
	/// @return If the file was successfully opened.
	virtual bool open() const;

	/// @brief Closes the file.
	virtual void close() const;

	/// @brief Reads the full file.
	/// @return The file contents.
	virtual std::vector<char> read() const;

	/// @brief Reads part of a file.
	/// @param count How many bytes to read.
	/// @return The file contents.
	virtual std::vector<char> read(std::size_t count) const;

	/// @brief Reads from the file until it encounters the token.
	virtual std::vector<char> readUntil(std::string_view delimiter) const;

	/// @brief Reads the file into a string.
	/// @return The file contents.
	virtual std::string readAsString() const;

	/// @brief Reads a line from the file.
	/// @return A string containing a single line, excluding the line ending.
	virtual std::string readLine() const;

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
	std::generator<std::string> readAllLines() const;

	/// @brief Reads data into a buffer.
	/// @param buffer The buffer to fill.
	/// @param count How many bytes to read.
	/// @return How much bytes were actually read.
	virtual size_t readIntoBuffer(uint8_t* buffer, size_t count);

	/// @brief Reads the position indicator of the file.
	/// @return The position indicator.
	virtual std::streampos getReadPosition() const;

	/// @brief Sets the position indicator of the file.
	/// @param position The position in the file.
	virtual File& setReadPosition(const std::streampos& position);

	/// @brief Sets the position indicator of the file.
	/// @param offset The offset for our new read position.
	/// @param origin Where we calculate the offset from.
	virtual File& setReadPosition(const std::streamoff& offset, const std::ios_base::seekdir origin = std::ios_base::beg);

	/// @brief Tells us if the file is opened.
	/// @return If the file is opened or not.
	virtual bool opened() const;

	/// @brief Tells us if we finished reading the file.
	/// @return If we finished reading the file or not.
	virtual bool finished() const;

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

protected:
	std::filesystem::path m_file;
	mutable std::unique_ptr<std::istream> m_stream;
};

using FilePtr = std::shared_ptr<File>;

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::fs

#endif // FILE_H
