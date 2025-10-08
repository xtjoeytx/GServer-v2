#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <generator>
#include <ios>
#include <istream>
#include <memory>
#include <optional>
#include <sstream>
#include <string_view>
#include <string>
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

std::string getFileNameAsANSI(const std::filesystem::path& file)
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
	std::locale loc{};
	using wcvt = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>;
	auto wstr = wcvt{}.from_bytes(file.filename().string());
	std::string result(wstr.size(), '0');
	std::use_facet<std::ctype<wchar_t>>(loc).narrow(wstr.data(), wstr.data() + wstr.size(), '?', &result[0]);
	return result;
#endif
}

//----------------------------

bool File::open() const
{
	if (m_stream)
		dynamic_cast<std::ifstream&>(*m_stream).close();

	auto fstream = std::make_unique<std::ifstream>();
	fstream->open(m_file, std::ios::binary);

	// Sometimes there can be very weird OS issues where the first attempt to open fails.
	// No idea why.  If the permission was denied, briefly sleep and try one more time.
	if (!fstream->is_open() && errno == EACCES)
	{
		std::this_thread::sleep_for(1ms);
		fstream->open(m_file, std::ios::binary);
	}

	// We failed to open the file.
	if (!fstream->is_open())
	{
		std::string error{ strerror(errno) };
		log::printLine(log::server, "** File '{}' read error: {}", m_file.string(), error);
	}

	m_stream = std::move(fstream);
	return true;
}

void File::close() const
{
	dynamic_cast<std::ifstream&>(*m_stream).close();
}

std::vector<char> File::read() const
{
	if (!opened() || finished())
		return std::vector<char>();

	// Seek to the end and get the file size.
	m_stream->seekg(0, std::ios::end);
	auto size = m_stream->tellg();

	// Seek to the start and read into the vector.
	std::vector<char> result(static_cast<size_t>(size));
	m_stream->seekg(0);
	m_stream->read(result.data(), size);
	return result;
}

std::vector<char> File::read(std::size_t count) const
{
	if (!opened() || finished())
		return std::vector<char>();

	std::vector<char> result(count);
	m_stream->read(result.data(), count);
	auto amount = m_stream->gcount();
	result.resize(static_cast<size_t>(amount));
	return result;
}

std::vector<char> File::readUntil(std::string_view delimiter) const
{
	if (delimiter.empty())
		return read();

	std::vector<char> result;

	if (opened() && !finished())
	{
		constexpr size_t bufferSize = 4096;
		char buffer[bufferSize];
		char delim = delimiter[0];

		do
		{
			// Read up to the start of the delimiter, or the full count of the buffer.
			m_stream->get(buffer, bufferSize, delim);

			// Append what we read to the result.
			auto count = m_stream->gcount();
			result.insert(result.end(), buffer, buffer + count);

			// If the next character is the start of our delimiter, check if the full delimiter is there.
			if (!finished() && m_stream->peek() == delim)
			{
				for (size_t i = 0; i < delimiter.length(); ++i)
				{
					buffer[i] = m_stream->get();
					if (finished() || m_stream->peek() != delimiter[i + 1])
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
		while (!finished());
	}

	return result;
}

std::string File::readAsString() const
{
	if (!opened() || finished())
		return std::string();

	std::stringstream s;
	s << m_stream->rdbuf();
	return s.str();
}

std::string File::readLine() const
{
	if (!opened() || finished())
		return std::string();

	std::string result;
	std::getline(*m_stream, result);
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

	setReadPosition(0);
	std::string line;

	while (!finished())
	{
		std::getline(*m_stream, line);
		if (string::trimLeft(line).starts_with(key))
		{
			auto sep = line.find(separator);
			setReadPosition(0);

			if (sep == std::string::npos)
				return std::string{};

			std::string value{ string::trim(line.substr(sep + separator.length())) };
			return value;
		}
	}

	setReadPosition(0);
	return std::nullopt;
}

std::optional<std::string> File::readConfigSection(std::string_view startKey, std::string_view endKey)
{
	if (!opened())
		return std::nullopt;

	setReadPosition(0);
	std::string section;
	std::string line;
	bool inSection = false;

	while (!finished())
	{
		std::getline(*m_stream, line);
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

	setReadPosition(0);

	if (section.empty())
		return std::nullopt;
	return std::string{ section };
}

std::generator<std::string> File::readAllLines() const
{
	while (!finished())
	{
		co_yield readLine();
	}
}

size_t File::readIntoBuffer(uint8_t* buffer, size_t count)
{
	if (!opened() || finished())
		return 0;

	auto as_char = reinterpret_cast<char*>(buffer);
	m_stream->read(as_char, count);
	auto amount = m_stream->gcount();
	return static_cast<size_t>(amount);
}

std::streampos File::getReadPosition() const
{
	return m_stream->tellg();
}

File& File::setReadPosition(const std::streampos& position)
{
	if (opened())
		m_stream->seekg(position);
	return *this;
}

File& File::setReadPosition(const std::streamoff& offset, const std::ios_base::seekdir origin)
{
	if (opened())
		m_stream->seekg(offset, origin);
	return *this;
}

bool File::opened() const
{
	return dynamic_cast<std::ifstream&>(*m_stream).is_open();
}

bool File::finished() const
{
	if (m_stream == nullptr || !opened())
		return true;
	return m_stream->eof();
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::fs
