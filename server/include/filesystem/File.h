#ifndef FILE_H
#define FILE_H

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <istream>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string_view>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <utilities/std/generator.h>

using namespace std::literals;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

/// @brief Creates a string of the same native type as std::filesystem::path from a string literal using a user-defined literal operator.
/// @param chars Pointer to the character array representing the string literal.
/// @param length The length of the string literal.
/// @return A std::filesystem::path::string_type constructed from the given character array.
constexpr std::filesystem::path::string_type operator""_pv(const char* chars, const size_t length)
{
	return std::filesystem::path::string_type{chars, chars + length};
}

///////////////////////////////////////////////////////////////////////////////
} // namespace preagonal

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

/// @brief Returns an HTML unescaped version of the specified file name.
/// @param file The file to unescape.
/// @return The unescaped file name.
std::filesystem::path getHTMLUnescapedFileName(const std::filesystem::path& file);

//----------------------------

class File
{
public:
	File() = default;

	File(std::filesystem::path file, std::unique_ptr<std::ifstream>&& stream)
		: m_file(std::move(file)), m_inputStreamHandle(std::move(stream))
	{
	}

	explicit File(std::filesystem::path file)
		: m_file(std::move(file))
	{
		File::open();
	}

	File(File&& other) noexcept
	{
		std::swap(m_file, other.m_file);
		std::swap(m_inputStreamHandle, other.m_inputStreamHandle);
		std::swap(m_inputStream, other.m_inputStream);
	}

	virtual ~File()
	{
		File::close();
	}

public:
	File(const File& other) = delete;
	File& operator=(const File& other) = delete;
	bool operator==(const File& other) = delete;

public:
	File& operator=(File&& other) noexcept
	{
		std::swap(m_file, other.m_file);
		std::swap(m_inputStreamHandle, other.m_inputStreamHandle);
		std::swap(m_inputStream, other.m_inputStream);
		return *this;
	}

public:
	// ReSharper disable once CppNonExplicitConversionOperator
	/// @brief Converts directly into an istream.
	operator std::istream&() const // NOLINT(*-explicit-constructor)
	{
		return *m_inputStream;
	}

	// ReSharper disable once CppNonExplicitConversionOperator
	/// @brief Converts directly into a shared pointer to the istream.
	operator std::istream*() const // NOLINT(*-explicit-constructor)
	{
		return m_inputStream;
	}

	// ReSharper disable once CppNonExplicitConversionOperator
	/// @brief Returns if this is a valid file.
	operator bool() const // NOLINT(*-explicit-constructor)
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
	[[nodiscard]] virtual bool opened() const;

public:
	/// @brief Reads the position indicator of the file.
	/// @return The position indicator.
	[[nodiscard]] virtual std::streampos getStreamPosition() const;

	/// @brief Sets the position indicator of the file.
	/// @param position The position in the file.
	virtual File& setStreamPosition(const std::streampos& position);

	/// @brief Sets the position indicator of the file.
	/// @param offset The offset for our new read position.
	/// @param origin Where we calculate the offset from.
	virtual File& setStreamPosition(const std::streamoff& offset, const std::ios_base::seekdir origin);

	/// @brief Sets the position indicator of the file.
	/// @param offset The offset for our new read position.
	File& setStreamPosition(const std::streamoff& offset)
	{
		return setStreamPosition(offset, std::ios_base::beg);
	}

	/// @brief Gets the file size.
	/// @return The file size.
	[[nodiscard]] virtual uintmax_t size() const
	{
		return std::filesystem::file_size(m_file);
	}

	/// @brief Gets the path to the file.
	/// @return The path to the file.
	[[nodiscard]] const std::filesystem::path& filePath() const
	{
		return m_file;
	}

	/// @brief Sets the file path for the current object, closing any previously opened file.
	/// @param filePath The new file path to set.
	void setFilePath(const std::filesystem::path& filePath)
	{
		if (opened()) close();
		m_file = filePath;
	}

	/// @brief Gets the file modified time.
	/// @return The file modified time.
	[[nodiscard]] virtual std::filesystem::file_time_type modifiedTime() const
	{
		try
		{
			return std::filesystem::last_write_time(m_file);
		}
		catch (...)
		{
			return std::filesystem::file_time_type::min();
		}
	}

public:
	/// @brief Reads the full file.
	/// @return The file contents.
	virtual std::vector<char> read();

	/// @brief Reads part of a file.
	/// @param count How many bytes to read.
	/// @return The file contents.
	virtual std::vector<char> read(std::size_t count);

	/// @brief Reads part of a file as a string.
	/// @param count How many bytes to read.
	/// @return The file contents.
	virtual std::string readChars(std::size_t count);

	/// @brief Reads a packed string (length + data).
	/// @return The file contents.
	virtual std::string readGString();

	/// @brief Reads a packed integral value.
	/// @tparam C The number of bytes to read. Valid values are 1, 2, 3, 4, 5, or 10.
	/// @return An integral value whose type is deduced and corresponds to the requested size C.
	template<size_t C>
	[[a::inline]] auto readPackedIntegral() const;

	/// @brief Reads an integral value.
	/// @tparam C The number of bytes to read.
	/// @return An integral value whose type is deduced and corresponds to the requested size C.
	template<size_t C>
	[[a::inline]] auto readIntegral() const;

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
	/// @param separator The separator used to split the key and value in the configuration line. Defaults to "=".
	/// @return The value associated with the specified key as a string, or a std::nullopt if it doesn't exist.
	virtual std::optional<std::string> readConfigLine(std::string_view key, std::string_view separator);

	/// @brief Reads the value of a configuration entry for a given key and seeks back to the beginning of the file.
	/// @param key The key identifying the configuration entry to read.
	/// @return The value associated with the specified key as a string, or a std::nullopt if it doesn't exist.
	std::optional<std::string> readConfigLine(const std::string_view key)
	{
		return readConfigLine(key, "="sv);
	}

	/// @brief Reads a configuration section between the specified start and end keys and seeks back to the beginning of the file.
	/// @param startKey The key indicating the start of the configuration section to read.
	/// @param endKey The key indicating the end of the configuration section to read.
	/// @return An optional string containing the configuration section if found; std::nullopt if the section does not exist.
	virtual std::optional<std::string> readConfigSection(std::string_view startKey, std::string_view endKey);

	/// @brief Returns a generator that yields all lines as strings.
	/// @return A generator that produces each line as a std::string.
	std::generator<std::string> readAllLines();

	/// @brief Returns a generator that yields lines until it reaches the end key.
	/// @param endKey A string that, when encountered at the start of a line, ends the read.
	/// @return A generator that produces each line as a std::string.
	std::generator<std::string> readLinesUntilSectionEnd(std::string_view endKey);

	/// @brief Reads data into a buffer.
	/// @param buffer The buffer to fill.
	/// @param count How many bytes to read.
	/// @return How much bytes were actually read.
	virtual size_t readIntoBuffer(uint8_t* buffer, size_t count);

	/// @brief Tells us if we finished reading the file.
	/// @return If we finished reading the file or not.
	[[nodiscard]] virtual bool finishedReading() const;

protected:
	std::filesystem::path m_file;
	std::istream* m_inputStream{nullptr};
	std::unique_ptr<std::ifstream> m_inputStreamHandle;
};

using FilePtr = std::shared_ptr<File>;

//----------------------------

template<size_t C>
inline auto File::readPackedIntegral() const
{
	using Type = std::conditional_t<C == 1, uint8_t,
		std::conditional_t<C == 2, uint16_t,
		std::conditional_t<C == 3, uint32_t,
		std::conditional_t<C == 4, uint32_t,
		std::conditional_t<C == 5, uint64_t, void>>>>>;

	static_assert(C >= 1 && C <= 5, "Unsupported size for readPackedIntegral.");

	Type result = 0;
	char byte = 0;

	if (!opened() || finishedReading())
		return result;

	auto readAndApply = [&](const size_t N)
	{
		m_inputStream->read(&byte, 1);
		result |= (static_cast<Type>(static_cast<uint8_t>(byte - 32)) << ((N - 1) * 7));
	};

	readAndApply(1);

	if constexpr (C <= 2)
	{
		readAndApply(2);
	}
	if constexpr (C <= 3)
	{
		readAndApply(3);
	}
	if constexpr (C <= 4)
	{
		readAndApply(4);
	}
	if constexpr (C <= 5)
	{
		readAndApply(5);
	}

	return result;
}

template<size_t C>
inline auto File::readIntegral() const
{
	static_assert(C == 1 || C == 2 || C == 4 || C == 8, "Unsupported integral size for readIntegral.");

	using Type = std::conditional_t<C == 1, uint8_t,
		std::conditional_t<C == 2, uint16_t,
		std::conditional_t<C == 4, uint32_t,
		std::conditional_t<C == 8, uint64_t, void>>>>;

	if (!opened() || finishedReading())
		return static_cast<Type>(0);

	Type value = 0;
	m_inputStream->read(reinterpret_cast<char*>(&value), sizeof(value));
	return value;
}

//----------------------------

class FileIO : public File
{
public:
	FileIO() = default;

	FileIO(const std::filesystem::path& file, std::unique_ptr<std::ifstream>&& stream)
	{
		throw std::logic_error("This constructor is not intended to be used. Use the File constructor instead.");
	}

	FileIO(const std::filesystem::path& file, std::unique_ptr<std::fstream>&& stream)
		: m_outputStreamHandle(std::move(stream))
	{
		m_file = file;
	}

	explicit FileIO(const std::filesystem::path& file)
	{
		m_file = file;
		FileIO::open();
	}

	FileIO(const std::filesystem::path& file, const bool truncate)
	{
		m_file = file;
		FileIO::open(truncate);
	}

	FileIO(FileIO&& other) noexcept
	{
		std::swap(m_file, other.m_file);
		std::swap(m_tempFile, other.m_tempFile);
		std::swap(m_inputStream, other.m_inputStream);
		std::swap(m_outputStreamHandle, other.m_outputStreamHandle);
	}

	~FileIO() override
	{
		FileIO::close();
	}

public:
	FileIO(const FileIO& other) = delete;
	FileIO& operator=(const FileIO& other) = delete;
	bool operator==(const FileIO& other) = delete;

public:
	FileIO& operator=(FileIO&& other) noexcept
	{
		std::swap(m_file, other.m_file);
		std::swap(m_inputStream, other.m_inputStream);
		std::swap(m_outputStreamHandle, other.m_outputStreamHandle);
		return *this;
	}

public:
	using File::operator std::istream&;
	using File::operator std::istream*;
	using File::operator bool;

	// ReSharper disable once CppNonExplicitConversionOperator
	/// @brief Converts directly into an fstream.
	operator std::fstream&() const // NOLINT(*-explicit-constructor)
	{
		return *(m_outputStreamHandle);
	}

	// ReSharper disable once CppNonExplicitConversionOperator
	/// @brief Converts directly into a shared pointer to the istream.
	operator std::fstream*() const // NOLINT(*-explicit-constructor)
	{
		return m_outputStreamHandle.get();
	}

public:
	/// @brief Opens the file.
	/// @return If the file was successfully opened.
	bool open() override;

	/// @brief Closes the file.
	void close() override;

	/// @brief Tells us if the file is opened.
	/// @return If the file is opened or not.
	[[nodiscard]] bool opened() const override;

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
	FileIO& writeConfigLine(std::span<const char> key, std::span<const char> value, std::span<const char> separator = {" "sv});

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
	virtual bool open(bool truncate);

protected:
	std::filesystem::path m_tempFile;
	std::unique_ptr<std::fstream> m_outputStreamHandle;
};

using FileIOPtr = std::shared_ptr<FileIO>;

//----------------------------

class FileSimpleIO : public FileIO
{
public:
	using FileIO::FileIO;

	explicit FileSimpleIO(const std::filesystem::path& file)
	{
		m_file = file;
		FileSimpleIO::open(false);
	}

	FileSimpleIO(const std::filesystem::path& file, const bool truncate)
	{
		m_file = file;
		FileSimpleIO::open(truncate);
	}

public:
	FileSimpleIO(const FileSimpleIO& other) = delete;
	FileSimpleIO& operator=(const FileSimpleIO& other) = delete;
	bool operator==(const FileSimpleIO& other) = delete;

public:
	FileSimpleIO& operator=(FileSimpleIO&& other) noexcept
	{
		std::swap(m_file, other.m_file);
		std::swap(m_inputStream, other.m_inputStream);
		std::swap(m_outputStreamHandle, other.m_outputStreamHandle);
		return *this;
	}

public:
	using File::operator std::istream&;
	using File::operator std::istream*;
	using File::operator bool;
	using FileIO::operator=;
	using FileIO::operator std::fstream&;
	using FileIO::operator std::fstream*;

public:
	/// @brief Closes the file.
	void close() override;

protected:
	bool open(bool truncate) override;
};

using FileSimpleIOPtr = std::shared_ptr<FileSimpleIO>;

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::fs

#endif // FILE_H
