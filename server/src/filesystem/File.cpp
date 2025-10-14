#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <generator>
#include <ios>
#include <istream>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string_view>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include <filesystem/File.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

#ifdef PLATFORM_WINDOWS
#include <stdexcept>
#include <windows.h>
#endif

#ifdef PLATFORM_UNIX
#include <codecvt>
#include <locale>
#endif

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::fs
{
///////////////////////////////////////////////////////////////////////////////

std::string getANSIFileName(const std::filesystem::path& file)
{
#ifdef PLATFORM_WINDOWS
	// Graal uses ANSI encoding for filenames, so convert so we don't mangle the filenames in Windows.
	std::filesystem::path::string_type fileName = file.filename().native();

	// Calculate the required buffer size for the conversion.
	int bufferSize = WideCharToMultiByte(1252, 0, fileName.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (bufferSize == 0)
		throw std::runtime_error("Failed to calculate buffer size for CP-1252 conversion.");

	// Allocate the string.
	std::string result(bufferSize - 1, '\0');

	// Convert to CP-1252.
	int bytesWritten = WideCharToMultiByte(1252, 0, fileName.c_str(), -1, &result[0], bufferSize, nullptr, nullptr);
	if (bytesWritten == 0)
		throw std::runtime_error("Failed to convert file name to CP-1252.");

	return result;
#else
	// Hacky version for Linux using deprecated C++.
	// TODO: Link to ICU.
	try
	{
		std::locale loc{};
		using wcvt = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>;
		auto wstr = wcvt{}.from_bytes(file.filename().string());
		std::string result(wstr.size(), '\0');
		std::use_facet<std::ctype<wchar_t>>(loc).narrow(wstr.data(), wstr.data() + wstr.size(), '?', &result[0]);
		return result;
	}
	catch (...)
	{
		return file.filename().string();
	}
#endif
}

std::filesystem::path getHTMLEscapedFileName(const std::filesystem::path& file)
{
	using ST = std::filesystem::path::string_type;
	using VT = std::filesystem::path::value_type;

	std::function<size_t(const ST&, size_t)> findFirstNotOfAlphaNumeric;
	std::function<void(ST&, VT)> writeEncoded;

#ifdef PLATFORM_WINDOWS
	findFirstNotOfAlphaNumeric = [](const ST& native, size_t pos) -> size_t
	{
		return native.find_first_not_of(L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.", pos);
	};
	writeEncoded = [](ST& result, VT ch)
	{
		result += std::format(L"{:03}", (uint32_t)ch);
	};
#else
	findFirstNotOfAlphaNumeric = [](const ST& native, size_t pos) -> size_t
	{
		return native.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.", pos);
	};
	writeEncoded = [](ST& result, VT ch)
	{
		result += std::format("{:03}", (uint32_t)ch);
	};
#endif

	auto& native = file.native();

	ST result;
	size_t oldpos = 0, pos = 0;
	while ((pos = findFirstNotOfAlphaNumeric(native, pos)) != ST::npos)
	{
		result.append(native.c_str() + oldpos, pos - oldpos);
		result.append(1, (VT)'%');
		writeEncoded(result, native[pos]);
		oldpos = ++pos;
	}
	if (oldpos < native.length())
		result.append(native.c_str() + oldpos);

	return result;
}

//----------------------------

bool File::open()
{
	if (m_inputStreamHandle)
		m_inputStreamHandle->close();

	auto fstream = std::make_unique<std::ifstream>();
	fstream->open(m_file, std::ios::binary | std::ios::in);

	// Sometimes there can be very weird OS issues where the first attempt to open fails.
	// No idea why.  If the permission was denied, briefly sleep and try one more time.
	if (!fstream->is_open() && errno == EACCES)
	{
		std::this_thread::sleep_for(1ms);
		fstream->open(m_file, std::ios::binary | std::ios::in);
	}

	// We failed to open the file.
	if (!fstream->is_open())
	{
		std::string error{ strerror(errno) };
		log::printLine(log::server, "** File '{}' read error: {}", m_file.string(), error);
	}

	m_inputStreamHandle = std::move(fstream);
	m_inputStream = dynamic_cast<std::ifstream*>(m_inputStreamHandle.get());

	return true;
}

void File::close()
{
	m_inputStream = nullptr;
	if (m_inputStreamHandle)
		m_inputStreamHandle->close();
}

std::vector<char> File::read()
{
	if (!opened() || finishedReading())
		return std::vector<char>();

	// Seek to the end and get the file size.
	m_inputStream->seekg(0, std::ios::end);
	auto size = m_inputStream->tellg();

	// Seek to the start and read into the vector.
	std::vector<char> result(static_cast<size_t>(size));
	m_inputStream->seekg(0);
	m_inputStream->read(result.data(), size);
	return result;
}

std::vector<char> File::read(std::size_t count)
{
	if (!opened() || finishedReading())
		return std::vector<char>();

	std::vector<char> result(count);
	m_inputStream->read(result.data(), count);
	auto amount = m_inputStream->gcount();
	result.resize(static_cast<size_t>(amount));
	return result;
}

std::vector<char> File::readUntil(std::string_view delimiter)
{
	if (delimiter.empty())
		return read();

	std::vector<char> result;

	if (opened() && !finishedReading())
	{
		constexpr size_t bufferSize = 4096;
		char buffer[bufferSize];
		char delim = delimiter[0];

		do
		{
			// Read up to the start of the delimiter, or the full count of the buffer.
			m_inputStream->get(buffer, bufferSize, delim);

			// Append what we read to the result.
			auto count = m_inputStream->gcount();
			result.insert(result.end(), buffer, buffer + count);

			// If the next character is the start of our delimiter, check if the full delimiter is there.
			if (!finishedReading() && m_inputStream->peek() == delim)
			{
				for (size_t i = 0; i < delimiter.length(); ++i)
				{
					buffer[i] = m_inputStream->get();
					if (finishedReading() || m_inputStream->peek() != delimiter[i + 1])
					{
						// We didn't find the full delimiter, add what we read to the result and continue.
						result.insert(result.end(), buffer, buffer + i + 1);
						break;
					}
				}

				// We found the full delimiter so stop reading.
				if (std::equal(buffer, buffer + delimiter.length(), delimiter.begin()))
					break;
			}
		}
		while (!finishedReading());
	}

	return result;
}

std::string File::readAsString()
{
	if (!opened() || finishedReading())
		return std::string();

	std::stringstream s;
	s << m_inputStream->rdbuf();
	return s.str();
}

std::string File::readLine()
{
	if (!opened() || finishedReading())
		return std::string();

	std::string result;
	std::getline(*m_inputStream, result);
	if (result.empty())
		return result;

	if (*result.crbegin() == '\r')
		result.pop_back();

	return result;
}

std::optional<std::string> File::readConfigLine(std::string_view key, std::string_view separator)
{
	if (!opened())
		return std::nullopt;

	setStreamPosition(0);
	std::string line;

	while (!finishedReading())
	{
		std::getline(*m_inputStream, line);
		if (string::trimLeft(line).starts_with(key))
		{
			auto sep = line.find(separator);
			setStreamPosition(0);

			if (sep == std::string::npos)
				return std::string{};

			std::string value{ string::trim(line.substr(sep + separator.length())) };
			return value;
		}
	}

	setStreamPosition(0);
	return std::nullopt;
}

std::optional<std::string> File::readConfigSection(std::string_view startKey, std::string_view endKey)
{
	if (!opened())
		return std::nullopt;

	setStreamPosition(0);
	std::string section;
	std::string line;
	bool inSection = false;

	while (!finishedReading())
	{
		std::getline(*m_inputStream, line);
		if (string::trimLeft(line).starts_with(startKey))
		{
			inSection = true;
			continue;
		}

		if (inSection && string::trimLeft(line).starts_with(endKey))
			break;

		if (inSection)
			section += line + "\n";
	}

	setStreamPosition(0);

	if (section.empty())
		return std::nullopt;
	return std::string{ section };
}

std::generator<std::string> File::readAllLines()
{
	while (!finishedReading())
	{
		co_yield readLine();
	}
}

size_t File::readIntoBuffer(uint8_t* buffer, size_t count)
{
	if (!opened() || finishedReading())
		return 0;

	auto as_char = reinterpret_cast<char*>(buffer);
	m_inputStream->read(as_char, count);
	auto amount = m_inputStream->gcount();
	return static_cast<size_t>(amount);
}

std::streampos File::getStreamPosition() const
{
	return m_inputStream->tellg();
}

File& File::setStreamPosition(const std::streampos& position)
{
	if (opened())
		m_inputStream->seekg(position);
	return *this;
}

File& File::setStreamPosition(const std::streamoff& offset, const std::ios_base::seekdir origin)
{
	if (opened())
		m_inputStream->seekg(offset, origin);
	return *this;
}

bool File::opened() const
{
	if (auto inputStream = dynamic_cast<std::ifstream*>(m_inputStream); inputStream != nullptr)
		return inputStream->is_open();
	return false;
}

bool File::finishedReading() const
{
	if (m_inputStream == nullptr || !opened())
		return true;
	return m_inputStream->eof();
}

///////////////////////////////////////////////////////////////////////////////

static void removeNullTermination(std::span<const char>& input)
{
	if (input.size() != 0 && input.back() == '\0')
		input = input.subspan(0, input.size() - 1);
}

bool FileIO::open()
{
	return open(false);
}

bool FileIO::open(bool truncate)
{
	// We want to write into a temp file and move it over the original when done, so record the file name of the temp file.
	m_tempFile = m_file;
	m_tempFile.concat(".partial");
	//m_tempFile.replace_extension(m_tempFile.extension().concat(".partial"));

	if (m_outputStreamHandle)
		m_outputStreamHandle->close();

	// Binary read/write mode.
	// binary | in | out = open for read/write and create new if not exists.
	// trunc = destroy contents.
	// app | ate = append to end of file and seek to the end on open.
	std::ios_base::openmode modeFlags = std::ios::binary | std::ios::in | std::ios::out;
	modeFlags |= (truncate ? std::ios::trunc : (std::ios::app | std::ios::ate));

	auto fstream = std::make_unique<std::fstream>();
	fstream->open(m_tempFile, modeFlags);

	// Sometimes there can be very weird OS issues where the first attempt to open fails.
	// No idea why (maybe virus scanners locking the file?)
	// If the permission was denied, briefly sleep and try one more time.
	if (!fstream->is_open() && errno == EACCES)
	{
		std::this_thread::sleep_for(1ms);
		fstream->open(m_tempFile, modeFlags);
	}

	// We failed to open the file.
	if (!fstream->is_open())
	{
		std::string error{ strerror(errno) };
		log::printLine(log::server, "** File '{}' read error: {}", m_tempFile.string(), error);
	}

	m_outputStreamHandle = std::move(fstream);
	auto outputStream = m_outputStreamHandle.get();
	m_inputStream = dynamic_cast<std::istream*>(outputStream);

	return true;
}

void FileIO::close()
{
	if (m_outputStreamHandle)
		m_outputStreamHandle->close();

	File::close();

	// Move the temp file over the destination file.
	if (!m_tempFile.empty() && std::filesystem::exists(m_tempFile))
	{
		std::error_code ec;
		std::filesystem::rename(m_tempFile, m_file, ec);
		if (ec)
			log::printLine(log::server, "** File '{}' write error: {}", m_file.string(), ec.message());

		m_tempFile.clear();
	}
}

bool FileIO::opened() const
{
	if (!m_outputStreamHandle)
		return false;
	return m_outputStreamHandle->is_open();
}

FileIO& FileIO::clear()
{
	open(true);
	return *this;
}

FileIO& FileIO::write(std::span<const uint8_t> buffer)
{
	if (!opened())
		return *this;

	m_outputStreamHandle->write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
	return *this;
}

FileIO& FileIO::write(std::span<const char> buffer)
{
	if (!opened())
		return *this;

	// Deal with char[] literals in the code.
	removeNullTermination(buffer);

	m_outputStreamHandle->write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
	return *this;
}

FileIO& FileIO::writeLine()
{
	if (!opened())
		return *this;

	m_outputStreamHandle->write("\n", 1);
	return *this;
}

FileIO& FileIO::writeLine(std::span<const char> line)
{
	if (!opened())
		return *this;

	// Deal with char[] literals in the code.
	removeNullTermination(line);

	m_outputStreamHandle->write(line.data(), static_cast<std::streamsize>(line.size()));
	m_outputStreamHandle->write("\n", 1);
	return *this;
}

FileIO& FileIO::writeConfigLine(std::span<const char> key, std::span<const char> value, std::span<const char> separator)
{
	if (!opened())
		return *this;

	// Deal with char[] literals in the code.
	removeNullTermination(key);
	removeNullTermination(value);
	removeNullTermination(separator);

	m_outputStreamHandle->write(key.data(), static_cast<std::streamsize>(key.size()));
	m_outputStreamHandle->write(separator.data(), static_cast<std::streamsize>(separator.size()));
	m_outputStreamHandle->write(value.data(), static_cast<std::streamsize>(value.size()));
	m_outputStreamHandle->write("\n", 1);
	return *this;
}

FileIO& FileIO::writeConfigSection(std::span<const char> startKey, std::span<const char> section, std::span<const char> endKey)
{
	if (!opened())
		return *this;

	// Deal with char[] literals in the code.
	removeNullTermination(startKey);
	removeNullTermination(section);
	removeNullTermination(endKey);

	m_outputStreamHandle->write(startKey.data(), static_cast<std::streamsize>(startKey.size()));
	m_outputStreamHandle->write("\n", 1);

	m_outputStreamHandle->write(section.data(), static_cast<std::streamsize>(section.size()));
	if (section.back() != '\n')
		m_outputStreamHandle->write("\n", 1);

	m_outputStreamHandle->write(endKey.data(), static_cast<std::streamsize>(endKey.size()));
	m_outputStreamHandle->write("\n", 1);
	return *this;
}

FileIO& FileIO::flush()
{
	if (!opened())
		return *this;

	m_outputStreamHandle->flush();
	return *this;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::fs
