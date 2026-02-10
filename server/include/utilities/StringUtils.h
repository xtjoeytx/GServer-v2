#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <algorithm>
#include <array>
#include <cctype>
#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <iterator>
#include <optional>
#include <ranges>
#include <span>
#include <sstream>
#include <string_view>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <CString.h>

#include <utilities/std/generator.h>

using namespace std::literals::string_view_literals;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::string
{
///////////////////////////////////////////////////////////////////////////////

// A concept that checks if a type is a string.
template<typename T>
concept StringVariant = std::same_as<std::remove_cvref_t<T>, std::string> || std::same_as<std::remove_cvref_t<T>, std::wstring>;

// A concept that checks if a type is a string_view.
template<typename T>
concept StringViewVariant = std::same_as<std::remove_cvref_t<T>, std::string_view> || std::same_as<std::remove_cvref_t<T>, std::wstring_view>;

// A concept that checks if a type is a string or string_view.
template<typename T>
concept StringViewIshVariant = StringVariant<T> || StringViewVariant<T>;

/*
// A concept that checks if a type is a string or string_view.
template<typename T>
concept StringViewVariantUnicode = StringViewIshVariant<T> || std::same_as<std::remove_cvref_t<T>, std::u8string> || std::same_as<std::remove_cvref_t<T>, std::u8string_view>;

// A concept that checks if a type is a string or string_view.
template<typename T>
concept StringViewVariantNotUnicode = StringViewIshVariant<T> && !StringViewVariantUnicode<T>;
*/

// A concept that checks if a type is a pointer to a const char string (e.g. const char[], const char[N], const char*).
template<typename T>
concept PointerToConstCharString = std::is_bounded_array_v<std::remove_cvref_t<T>> && std::is_same_v<std::remove_all_extents_t<std::remove_cvref_t<T>>, char>;

// A concept that checks if a type is an input range, but not a string.
template<typename T>
concept InputRangeNotString = std::ranges::input_range<T> && !StringViewIshVariant<T> && !PointerToConstCharString<T>;

template<typename T>
concept NotInputRangeNotString = !InputRangeNotString<T>;

///////////////////////////////////////////////////////////////////////////////

/// A hash function for strings that can be used with heterogeneous lookups.
struct string_hash
{
	using hash_type = std::hash<std::string_view>;
	using is_transparent = void;

	[[nodiscard]] size_t operator()(const char* str) const noexcept
	{
		return hash_type{}(str);
	}
	[[nodiscard]] size_t operator()(const std::string_view& str) const noexcept
	{
		return hash_type{}(str);
	}
	[[nodiscard]] size_t operator()(const std::string& str) const noexcept
	{
		return hash_type{}(str);
	}
	[[nodiscard]] size_t operator()(const std::u8string_view& str) const noexcept
	{
		std::hash<std::u8string_view> hasher{};
		return hasher(str);
	}
	[[nodiscard]] size_t operator()(const std::u8string& str) const noexcept
	{
		std::hash<std::u8string_view> hasher{};
		return hasher(str);
	}
	[[nodiscard]] size_t operator()(const CString& str) const noexcept
	{
		return hash_type{}(str.toStringView());
	}
	[[nodiscard]] size_t operator()(const size_t& hash) const noexcept
	{
		return hash;
	}
};

/// A comparator function for strings that can be used with heterogeneous lookups.
struct string_hash_equal
{
	using is_transparent = void;
	[[nodiscard]] bool operator()(const std::string& lhs, const std::string& rhs) const noexcept
	{
		return lhs == rhs;
	}
	//
	[[nodiscard]] bool operator()(const char* lhs, const std::string& rhs) const noexcept
	{
		return lhs == rhs;
	}
	[[nodiscard]] bool operator()(const std::string_view& lhs, const std::string& rhs) const noexcept
	{
		return lhs == rhs;
	}
	[[nodiscard]] bool operator()(const CString& lhs, const std::string& rhs) const noexcept
	{
		return lhs == rhs;
	}
	[[nodiscard]] bool operator()(const size_t& lhs, const std::string& rhs) const noexcept
	{
		return lhs == string_hash{}(rhs);
	}
	//
	[[nodiscard]] bool operator()(const std::string& lhs, const char* rhs) const noexcept
	{
		return lhs == rhs;
	}
	[[nodiscard]] bool operator()(const std::string& lhs, const std::string_view& rhs) const noexcept
	{
		return lhs == rhs;
	}
	[[nodiscard]] bool operator()(const std::string& lhs, const CString& rhs) const noexcept
	{
		return lhs == rhs;
	}
	[[nodiscard]] bool operator()(const std::string& lhs, const size_t& rhs) const noexcept
	{
		return string_hash{}(lhs) == rhs;
	}
};

/// A comparator function for strings that can be used with heterogeneous lookups.
struct u8string_hash_equal
{
	using is_transparent = void;
	[[nodiscard]] bool operator()(const std::u8string& lhs, const std::u8string& rhs) const noexcept
	{
		return lhs == rhs;
	}
	//
	[[nodiscard]] bool operator()(const char8_t* lhs, const std::u8string& rhs) const noexcept
	{
		return lhs == rhs;
	}
	[[nodiscard]] bool operator()(const std::u8string_view& lhs, const std::u8string& rhs) const noexcept
	{
		return lhs == rhs;
	}
	[[nodiscard]] bool operator()(const CString& lhs, const std::u8string& rhs) const noexcept
	{
		auto sv = lhs.toStringView();
		std::u8string_view view{ reinterpret_cast<const char8_t*>(sv.data()), sv.length() };
		return view == rhs;
	}
	[[nodiscard]] bool operator()(const size_t& lhs, const std::u8string& rhs) const noexcept
	{
		return lhs == string_hash{}(rhs);
	}
	//
	[[nodiscard]] bool operator()(const std::u8string& lhs, const char8_t* rhs) const noexcept
	{
		return lhs == rhs;
	}
	[[nodiscard]] bool operator()(const std::u8string& lhs, const std::u8string_view& rhs) const noexcept
	{
		return lhs == rhs;
	}
	[[nodiscard]] bool operator()(const std::u8string& lhs, const CString& rhs) const noexcept
	{
		auto sv = rhs.toStringView();
		std::u8string_view view{ reinterpret_cast<const char8_t*>(sv.data()), sv.length() };
		return lhs == view;
	}
	[[nodiscard]] bool operator()(const std::u8string& lhs, const size_t& rhs) const noexcept
	{
		return string_hash{}(lhs) == rhs;
	}
};

/// A comparator function for hashes that can be used with heterogeneous lookups.
struct hash_string_equal
{
	using is_transparent = void;
	[[nodiscard]] bool operator()(const size_t& lhs, const size_t& rhs) const noexcept
	{
		return lhs == rhs;
	}
	//
	[[nodiscard]] bool operator()(const char* lhs, const size_t& rhs) const noexcept
	{
		return string_hash{}(lhs) == rhs;
	}
	[[nodiscard]] bool operator()(const std::string_view& lhs, const size_t& rhs) const noexcept
	{
		return string_hash{}(lhs) == rhs;
	}
	[[nodiscard]] bool operator()(const std::string& lhs, const size_t& rhs) const noexcept
	{
		return string_hash{}(lhs) == rhs;
	}
	[[nodiscard]] bool operator()(const CString& lhs, const size_t& rhs) const noexcept
	{
		return string_hash{}(lhs) == rhs;
	}
	//
	[[nodiscard]] bool operator()(const size_t& lhs, const char* rhs) const noexcept
	{
		return lhs == string_hash{}(rhs);
	}
	[[nodiscard]] bool operator()(const size_t& lhs, const std::string_view& rhs) const noexcept
	{
		return lhs == string_hash{}(rhs);
	}
	[[nodiscard]] bool operator()(const size_t& lhs, const std::string& rhs) const noexcept
	{
		return lhs == string_hash{}(rhs);
	}
	[[nodiscard]] bool operator()(const size_t& lhs, const CString& rhs) const noexcept
	{
		return lhs == string_hash{}(rhs);
	}
};

///////////////////////////////////////////////////////////////////////////////

/// @brief Trims whitespace from the start of a string.
/// @param str A string or string_view to trim.
/// @return A string_view to the trimmed string.
std::string_view trimLeft(StringViewIshVariant auto const& str)
{
	std::string_view view{ str };
	auto size = str.size();
	for (size_t i = 0; i < size; ++i)
	{
		auto ch = view[i];
		if (!std::isspace(static_cast<unsigned char>(ch)) && ch != '\xa7')
			return view.substr(i, size - i);
	}
	return {};
}

/// @brief Trims whitespace from the end of a string.
/// @param str A string or string_view to trim.
/// @return A string_view to the trimmed string.
std::string_view trimRight(StringViewIshVariant auto const& str)
{
	std::string_view view{ str };
	for (size_t i = view.size(); i > 0; --i)
	{
		auto ch = view[i - 1];
		if (!std::isspace(static_cast<unsigned char>(ch)) && ch != '\xa7')
			return view.substr(0, i);
	}
	return {};
}

/// @brief Trims newlines (\\n and \\r) from the end of a string.
/// @param str A string or string_view to trim.
/// @return A string_view to the trimmed string.
std::string_view trimNewlines(StringViewIshVariant auto const& str)
{
	std::string_view view{ str };
	for (size_t i = view.size(); i > 0; --i)
	{
		auto ch = view[i - 1];
		if (ch != '\n' && ch != '\r' && ch != '\xa7')
			return view.substr(0, i);
	}
	return {};
}

/// @brief Trims whitespace from the start and end of a string.
/// @param str A string or string_view to trim.
/// @return A string_view to the trimmed string.
std::string_view trim(StringViewIshVariant auto const& str)
{
	return trimLeft(trimRight(str));
}

/// @brief Trims whitespace from the start of a string, mutating it.
/// @param str A string to trim.
/// @return A reference to the trimmed string.
inline std::string& trimLeftMutate(std::string& str)
{
	if (str.empty()) return str;

	// Find first non-space.
	const auto p = str.c_str();
	size_t idx = 0;
	while (idx < str.length() && (std::isspace(int(p[idx])) || p[idx] == '\r' || p[idx] == '\n' || p[idx] == '\xa7'))
		++idx;

	// No whitespace.
	if (idx == 0)
		return str;

	// All whitespace.
	if (idx == str.length())
	{
		str.clear();
		return str;
	}

	str = std::move(std::string{ str.begin() + idx, str.begin() + str.length()});
	return str;
}

/// @brief Trims whitespace from the end of a string, mutating it.
/// @param str A string to trim.
/// @return A reference to the trimmed string.
inline std::string& trimRightMutate(std::string& str)
{
	if (str.empty()) return str;

	// Find last non-space.
	const auto p = str.c_str();
	size_t idx = str.length();
	while (idx > 0 && (std::isspace(int(p[idx - 1])) || p[idx - 1] == '\r' || p[idx - 1] == '\n' || p[idx - 1] == '\xa7'))
		--idx;

	// No whitespace.
	if (idx == str.length())
		return str;

	// All whitespace.
	if (idx < 0)
	{
		str.clear();
		return str;
	}

	str.resize(idx);
	return str;
}

/// @brief Trims newlines (\\n and \\r) from the end of a string, mutating it.
/// @param str A string to trim.
/// @return A reference to the trimmed string.
inline std::string& trimNewlinesMutate(std::string& str)
{
	if (str.empty()) return str;

	// Find last non-space.
	const auto p = str.c_str();
	size_t idx = str.length();
	while (idx > 0 && (p[idx - 1] == '\n' || p[idx - 1] == '\r' || p[idx - 1] == '\xa7'))
		--idx;

	// No whitespace.
	if (idx == str.length())
		return str;

	// All whitespace.
	if (idx < 0)
	{
		str.clear();
		return str;
	}

	str.resize(idx);
	return str;
}

/// @brief Trims whitespace from the start and end of a string, mutating it.
/// @param str A string to trim.
/// @return A reference to the trimmed string.
inline std::string& trimMutate(std::string& str)
{
	if (str.empty()) return str;

	// Find first and last non-space.
	const auto p = str.c_str();
	size_t front = 0, back = str.length();
	while (front < str.length() && (std::isspace(static_cast<int>(static_cast<uint8_t>(p[front]))) || p[front] == '\r' || p[front] == '\n' || p[front] == '\xa7'))
		++front;
	while (front < back && (std::isspace(static_cast<int>(static_cast<uint8_t>(p[back - 1]))) || p[back - 1] == '\r' || p[back - 1] == '\n' || p[back - 1] == '\xa7'))
		--back;

	// No whitespace.
	if (front == 0 && back == str.length())
		return str;

	// All whitespace.
	if (back <= front)
	{
		str.clear();
		return str;
	}

	str = std::move(std::string{ str.begin() + front, str.begin() + back });
	return str;
}

/// @brief Trims whitespace from the start of a string, mutating it.
/// @param str A string to trim.
/// @return A reference to the trimmed string.
inline std::string&& trimLeftMutate(std::string&& str)
{
	return std::move(trimLeftMutate(str));
}

/// @brief Trims whitespace from the end of a string, mutating it.
/// @param str A string to trim.
/// @return A reference to the trimmed string.
inline std::string&& trimRightMutate(std::string&& str)
{
	return std::move(trimRightMutate(str));
}

/// @brief Trims whitespace from the start and end of a string, mutating it.
/// @param str A string to trim.
/// @return A reference to the trimmed string.
inline std::string&& trimMutate(std::string&& str)
{
	return std::move(trimMutate(str));
}

///////////////////////////////////////////////////////////////////////////////

/// @brief Replaces all occurrences of a substring within a string with another substring.
/// @param in The input string to process.
/// @param from The substring to search for and replace.
/// @param to The substring to replace each occurrence of 'from' with.
/// @return A new string with all occurrences of 'from' replaced by 'to'.
inline std::string replace(std::string_view in, std::string_view from, std::string_view to)
{
	if (from.empty())
		return std::string{ in };

	std::string out;
	out.reserve(in.size() + 16); // Reserve some extra space for the new string.
	size_t pos = 0;
	while (true)
	{
		const auto next_pos = in.find(from, pos);
		if (next_pos == std::string::npos)
		{
			out.append(in.substr(pos));
			break;
		}
		out.append(in.substr(pos, next_pos - pos));
		out.append(to);
		pos = next_pos + from.size();
	}
	return out;
}

/// @brief Replaces all occurrences of a substring with another substring in a given string, modifying the original string.
/// @param in The string to perform replacements on. This string will be modified in place.
/// @param from The substring to search for and replace.
/// @param to The substring to replace each occurrence of 'from' with.
/// @return A reference to the modified input string after all replacements have been made.
inline std::string& replaceMutate(std::string& in, std::string_view from, std::string_view to)
{
	if (from.empty())
		return in;

	size_t pos = 0;
	while ((pos = in.find(from, pos)) != std::string::npos)
	{
		in.replace(pos, from.size(), to);
		pos += to.size();
	}
	return in;
}

/// @brief Removes all occurrences of specified characters from a string.
/// @param in The input string to process.
/// @param chars A string containing the characters to remove from the input.
/// @return A new string with all characters from 'chars' removed from the input string.
inline std::string eraseChars(std::string_view in, std::string_view chars)
{
	if (chars.empty())
		return std::string{ in };

	auto filtered = in | std::views::filter([&chars](char c) {
		return chars.find(c) == std::string_view::npos;
	});
	std::string result(std::ranges::begin(filtered), std::ranges::end(filtered));

	return result;
}

/// @brief Removes all occurrences of specified characters from a string, modifying the original string.
/// @param in The string to be modified by removing specified characters.
/// @param chars A string view containing the characters to remove from the input string.
/// @return A reference to the modified input string with the specified characters removed.
inline std::string& eraseCharsMutate(std::string& in, std::string_view chars)
{
	if (chars.empty())
		return in;

	auto erased = std::ranges::remove_if(in, [&chars](char c) { return chars.find(c) != std::string_view::npos; });
	in.erase(erased.begin(), erased.end());

	return in;
}

inline auto removeExtension(StringViewIshVariant auto const& str)
{
	using Elem = std::remove_cvref_t<decltype(str)>::value_type;
	using Traits = std::remove_cvref_t<decltype(str)>::traits_type;

	std::basic_string_view<Elem, Traits> view{ str };
	auto pos = view.rfind('.');
	if (pos == std::string_view::npos)
		return view;
	return view.substr(0, pos);
}

///////////////////////////////////////////////////////////////////////////////

/// @brief Escapes quotes in a string using a CSV-like format.
/// @param str The input string or string_view to escape quotes in.
/// @return A new string with quotes escaped.
auto escapeQuotes(StringViewIshVariant auto const& str)
{
	using Elem = std::remove_cvref_t<decltype(str)>::value_type;
	using Traits = std::remove_cvref_t<decltype(str)>::traits_type;

	std::basic_string<Elem, Traits> ret{};
	ret.reserve(str.size() * 1.5);
	for (const auto& c: str)
	{
		switch (c)
		{
			case '\\':
				ret += "\\\\";
				break;
			case '\"':
				ret += "\"\"";
				break;
			case '\'':
				ret += "\'\'";
				break;
			default:
				ret += c;
				break;
		}
	}
	return ret;
}

/// @brief Unescapes quotes in a string that were escaped using a CSV-like format.
/// @param str The input string or string_view to unescape quotes in.
/// @return A new string with quotes unescaped.
auto unescapeQuotes(StringViewIshVariant auto const& str)
{
	using Elem = std::remove_cvref_t<decltype(str)>::value_type;
	using Traits = std::remove_cvref_t<decltype(str)>::traits_type;

	// The shortest an escaped character can be is 2 characters.
	if (str.size() < 2)
	{
		if constexpr (StringVariant<decltype(str)>)
			return str;
		else
			return std::basic_string<Elem, Traits>{ str };
	}

	std::basic_string<Elem, Traits> ret{};
	ret.reserve(str.size());
	size_t i = 0;
	for (; i < str.size() - 1; ++i)
	{
		// If the current character is not an escape character, add it to the result and go to the next.
		if (str[i] != '\\')
		{
			ret += str[i];
			continue;
		}

		// We had an escape character, so check the next character.
		// If it is a valid escape character, add the unescaped character to the result.
		// Otherwise, keep both the escape character and the next character.
		switch (str[i + 1])
		{
			case '\\':
				ret += '\\';
				break;
			case '\"':
				ret += '\"';
				break;
			case '\'':
				ret += '\'';
				break;
			default:
				ret += '\\';
				ret += str[i + 1];
				break;
		}

		// We skipped the next character, so increment the index.
		++i;
	}

	// Catch the last character.
	if (i == str.size() - 1)
		ret += str[i];

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

/// @brief Splits a string into tokens based on a list of delimiters and returns them as a generator of string views.
/// @param str The input string to split. Can be any type compatible with string view.
/// @param delims A string containing delimiter characters used to split the input.
/// @param ignoreEmpty If true, empty tokens are ignored; if false, empty tokens are included in the output.
/// @return A generator yielding each token as a std::string_view.
auto split(StringViewVariant auto const& str, StringViewVariant auto delims, bool ignoreEmpty) -> std::generator<std::remove_cvref_t<decltype(str)>>
{
	using Elem = std::remove_cvref_t<decltype(str)>::value_type;
	using Traits = std::remove_cvref_t<decltype(str)>::traits_type;
	using StringViewType = std::basic_string_view<Elem, Traits>;
	StringViewType strview{ str };

	size_t start = 0, end = 0;
	while (start < str.length())
	{
		// Find the next delimiter.
		end = strview.find_first_of(delims, start);

		// None found, so add the rest of the string.
		if (end == StringViewType::npos)
		{
			co_yield strview.substr(start);
			break;
		}

		// Add the token to the vector.
		if (end > start)
			co_yield strview.substr(start, end - start);
		else if (!ignoreEmpty)
			co_yield StringViewType{};

		// If the delim was \r and the next character is \n, include it in the delim.
		if (strview[end] == '\r' && end + 1 < strview.length() && strview[end + 1] == '\n')
			++end;

		start = end + 1;
	}
}

/// @brief Splits a string into tokens separated by a delimiting string and returns them as a generator of string views.
/// @param str The input string to split. Can be any type compatible with string view.
/// @param delim A string used to split the input.
/// @param ignoreEmpty If true, empty tokens are ignored; if false, empty tokens are included in the output.
/// @return A generator yielding each token as a std::string_view.
auto splitByString(StringViewVariant auto const& str, StringViewVariant auto delim, bool ignoreEmpty) -> std::generator<std::remove_cvref_t<decltype(str)>>
{
	using Elem = std::remove_cvref_t<decltype(str)>::value_type;
	using Traits = std::remove_cvref_t<decltype(str)>::traits_type;
	using StringViewType = std::basic_string_view<Elem, Traits>;
	StringViewType strview{ str };

	size_t start = 0, end = 0;
	while (start < str.length())
	{
		// Find the next delimiter.
		end = strview.find(delim, start);

		// None found, so add the rest of the string.
		if (end == StringViewType::npos)
		{
			co_yield strview.substr(start);
			break;
		}

		// Add the token to the vector.
		if (end > start)
			co_yield strview.substr(start, end - start);
		else if (!ignoreEmpty)
			co_yield StringViewType{};

		start = end + delim.length();
	}
}

/// @brief Splits a string into a vector of tokens based on specified delimiters.
/// @param str The input string to split.
/// @param delims A string containing delimiter characters used to split the input string.
/// @param ignoreEmpty If true, empty tokens are ignored; if false, empty tokens are included in the result.
/// @return A vector of strings containing the tokens extracted from the input string.
auto splitToVector(StringViewIshVariant auto const& str, StringViewIshVariant auto const& delims, bool ignoreEmpty)
{
	using Elem = std::remove_cvref_t<decltype(str)>::value_type;
	using Traits = std::remove_cvref_t<decltype(str)>::traits_type;
	using StringType = std::basic_string<Elem, Traits>;
	using StringViewType = std::basic_string_view<Elem, Traits>;
	StringViewType strview{ str };
	StringViewType delimview{ delims };

	std::vector<StringType> tokens;
	for (const auto& token : split(strview, delimview, ignoreEmpty))
		tokens.emplace_back(token);

	return tokens;
}

/// @brief Splits a string into a vector of tokens based on specified delimiters.
/// @param str The input string to split.
/// @param delims A string containing delimiter characters used to split the input string.
/// @param ignoreEmpty If true, empty tokens are ignored; if false, empty tokens are included in the result.
/// @return A vector of strings containing the tokens extracted from the input string.
auto splitToVectorView(StringViewIshVariant auto const& str, StringViewIshVariant auto const& delims, bool ignoreEmpty)
{
	using Elem = std::remove_cvref_t<decltype(str)>::value_type;
	using Traits = std::remove_cvref_t<decltype(str)>::traits_type;
	using StringViewType = std::basic_string_view<Elem, Traits>;
	StringViewType strview{ str };
	StringViewType delimview{ delims };

	std::vector<StringViewType> tokens;
	for (const auto& token : split(strview, delimview, ignoreEmpty))
		tokens.emplace_back(token);

	return tokens;
}

/// @brief Splits a string into a vector of substrings using a specified delimiter.
/// @param str The input string to be split.
/// @param delim The delimiter string used to split the input.
/// @param ignoreEmpty If true, empty substrings are ignored; otherwise, they are included in the result.
/// @return A vector containing the substrings resulting from splitting the input string by the delimiter.
auto splitToVectorByString(StringViewIshVariant auto const& str, StringViewIshVariant auto const& delim, bool ignoreEmpty)
{
	using Elem = std::remove_cvref_t<decltype(str)>::value_type;
	using Traits = std::remove_cvref_t<decltype(str)>::traits_type;
	using StringType = std::basic_string<Elem, Traits>;
	using StringViewType = std::basic_string_view<Elem, Traits>;
	StringViewType strview{ str };
	StringViewType delimview{ delim };

	std::vector<StringType> tokens;
	for (const auto& token : splitByString(strview, delimview, ignoreEmpty))
		tokens.emplace_back(token);

	return tokens;
}

//----------------------------

/// @brief Splits a string into tokens based on whitespace and returns them as a generator of string views, ignoring empty tokens.
/// @param str The input string to split. Can be any type compatible with string view.
/// @return A generator yielding each token as a std::string_view.
auto split(StringViewVariant auto str) -> std::generator<decltype(str)>
{
	for (const auto& item : split(str, " \t\n\r"sv, true))
		co_yield item;
}

/// @brief Splits a string into tokens based on a list of delimiters and returns them as a generator of string views, ignoring empty tokens.
/// @param str The input string to split. Can be any type compatible with string view.
/// @param delims A string containing delimiter characters used to split the input. Defaults to whitespace characters.
/// @return A generator yielding each token as a std::string_view.
auto split(StringViewVariant auto str, StringViewVariant auto delims) -> std::generator<decltype(str)>
{
	for (const auto& item : split(str, delims, true))
		co_yield item;
}

/// @brief Splits a string into tokens separated by a delimiting string and returns them as a generator of string views, ignoring empty tokens.
/// @param str The input string to split. Can be any type compatible with string view.
/// @param delim A string used to split the input.
/// @return A generator yielding each token as a std::string_view.
auto splitByString(StringViewVariant auto str, StringViewVariant auto delim) -> std::generator<decltype(str)>
{
	for (const auto& item : splitByString(str, delim, true))
		co_yield item;
}

/// @brief Splits a string into a vector of tokens based on whitespace, ignoring empty tokens.
/// @param str The input string to split.
/// @return A vector of strings containing the tokens extracted from the input string.
auto splitToVector(StringViewIshVariant auto const& str)
{
	return splitToVector(str, " \t\n\r"sv, true);
}

/// @brief Splits a string into a vector of tokens based on specified delimiters, ignoring empty tokens.
/// @param str The input string to split.
/// @param delims A string containing delimiter characters used to split the input string.
/// @return A vector of strings containing the tokens extracted from the input string.
auto splitToVector(StringViewIshVariant auto const& str, StringViewIshVariant auto const& delims)
{
	return splitToVector(str, delims, true);
}

/// @brief Splits a string into a vector of tokens based on whitespace, ignoring empty tokens.
/// @param str The input string to split.
/// @return A vector of strings containing the tokens extracted from the input string.
auto splitToVectorView(StringViewIshVariant auto const& str)
{
	return splitToVectorView(str, " \t\n\r"sv, true);
}

/// @brief Splits a string into a vector of tokens based on specified delimiters, ignoring empty tokens.
/// @param str The input string to split.
/// @param delims A string containing delimiter characters used to split the input string.
/// @return A vector of strings containing the tokens extracted from the input string.
auto splitToVectorView(StringViewIshVariant auto const& str, StringViewIshVariant auto const& delims)
{
	return splitToVectorView(str, delims, true);
}

/// @brief Splits a string into a vector of substrings using a specified delimiter, ignoring empty tokens.
/// @param str The input string to be split.
/// @param delim The delimiter string used to split the input.
/// @return A vector containing the substrings resulting from splitting the input string by the delimiter.
auto splitToVectorByString(StringViewIshVariant auto const& str, StringViewIshVariant auto const& delim)
{
	return splitToVectorByString(str, delim, true);
}

//----------------------------

/// @brief Splits a string into tokens based on a list of delimiters and returns them as a generator of string views.
/// @param str The input string to split. Can be any type compatible with string view.
/// @param delims A string containing delimiter characters used to split the input.
/// @param ignoreEmpty If true, empty tokens are ignored; if false, empty tokens are included in the output.
/// @return A generator yielding each token as a std::string_view.
inline std::generator<std::string_view> split(std::string& str, std::string_view delims, bool ignoreEmpty = true)
{
	for (const auto& item : split(std::string_view{ str }, delims, ignoreEmpty))
		co_yield item;
}
inline std::generator<std::wstring_view> split(std::wstring& str, std::wstring_view delims, bool ignoreEmpty = true)
{
	for (const auto& item : split(std::wstring_view{ str }, delims, ignoreEmpty))
		co_yield item;
}

/// @brief Splits a string into tokens separated by a delimiting string and returns them as a generator of string views, ignoring empty tokens.
/// @param str The input string to split. Can be any type compatible with string view.
/// @param delim A string used to split the input.
/// @param ignoreEmpty If true, empty tokens are ignored; if false, empty tokens are included in the output.
/// @return A generator yielding each token as a std::string_view.
inline std::generator<std::string_view> splitByString(std::string& str, std::string_view delim, bool ignoreEmpty = true)
{
	for (const auto& item : splitByString(std::string_view{ str }, delim, ignoreEmpty))
		co_yield item;
}

//----------------------------

/// @brief Joins the elements of a range into a single string, separated by a specified delimiter.
/// @param range An input range containing elements to join. The elements must be streamable to std::ostringstream.
/// @param delim The delimiter string to insert between elements. Defaults to ','.
/// @return A string containing the joined elements of the range, separated by the specified delimiter.
std::string join(std::ranges::input_range auto&& range, std::string_view delim = ",")
{
	std::ostringstream oss;
	auto it = std::ranges::begin(range);
	if (it != std::ranges::end(range))
	{
		oss << *it;
		++it;
	}
	for (; it != std::ranges::end(range); ++it)
		oss << delim << *it;
	return oss.str();
}

///////////////////////////////////////////////////////////////////////////////

/// @brief Converts a range of strings to a single CSV-formatted string, quoting fields as needed.
/// @param range An input range of string-like elements to be converted to CSV format.
/// @param force_quoted If true, all fields will be quoted regardless of content. Defaults to false.
/// @return A std::string containing the CSV-formatted representation of the input range, with fields separated by commas and quoted as necessary.
auto toCSV(InputRangeNotString auto&& range, bool force_quoted = false)
{
	constexpr std::array<char, 3> complexChars = { '"', ',', '\\' };
	std::ostringstream oss;

	for (const auto& wordFromRange : range)
	{
		std::string_view word{ wordFromRange };

		// Check if the word contains any complex characters.
		bool complex = std::ranges::any_of(word,
			[&complexChars](const auto& c) { return std::ranges::find(complexChars, c) != complexChars.end(); });

		// Output the word.
		if (!complex && !force_quoted)
		{
			oss << word << ',';
			continue;
		}

		// This was a complex word, so we need to certain characters.
		// For some reason we were doubling the backslash.  I can't remember if that was intentional or not.
		oss << '"';
		for (const char& c: word)
		{
			oss << c;
			if (c == '"' || c == '\\')
				oss << c;
		}

		// Add the separator.
		oss << "\",";
	}

	// Remove the last comma.
	auto result = oss.str();
	if (!result.empty())
		result.pop_back();

	return result;
}

/// @brief Converts a string or string-like object into CSV format, splitting it by a specified delimiter.
/// @param str The input string or string-like object to be converted to CSV.
/// @param delim The character used to split the input string into fields. Defaults to newline ('\n').
/// @param force_quoted If true, all fields will be quoted in the resulting CSV. Defaults to false.
/// @return A CSV-formatted string constructed from the split fields of the input.
auto toCSV(StringViewIshVariant auto const& str, std::string_view delim = "\n"sv, bool force_quoted = false)
{
	using Elem = std::remove_cvref_t<decltype(str)>::value_type;
	using Traits = std::remove_cvref_t<decltype(str)>::traits_type;
	using StringViewType = std::basic_string_view<Elem, Traits>;
	StringViewType strview{ str };

	auto s = splitByString(strview, delim);
	return toCSV(s, force_quoted);
}

/// @brief Parses a CSV-formatted string into a vector of strings, handling quoted fields and optional leading whitespace.
/// @param str The input string or string view containing CSV data to parse.
/// @param ignoreLeadingWhitespace If true, leading spaces and tabs before each field are ignored. Defaults to false.
/// @return A vector of strings, each representing a parsed field from the CSV input.
std::vector<std::string> fromCSV(StringViewIshVariant auto const& str, bool ignoreLeadingWhitespace = false)
{
	std::vector<std::string> tokens{};
	auto token = std::string{};

	bool wordStart = true;
	bool wordQuoted = false;
	for (size_t i = 0; i < str.length(); ++i)
	{
		const auto& c = str[i];

		// Ignore whitespace at the start.
		if (ignoreLeadingWhitespace)
		{
			if (wordStart && (c == ' ' || c == '\t'))
				continue;
		}

		// Check for a quoted word.
		if (wordStart == true && c == '"')
		{
			wordStart = false;
			wordQuoted = true;
			continue;
		}

		// Check for an escaped character.
		if (wordQuoted)
		{
			std::optional<char> next;
			if (i + 1 < str.length())
				next = str[i + 1];

			// Escaped backslash.
			if (c == '\\' && next.has_value() && next.value() == '\\')
			{
				token += '\\';
				++i;
			}
			// Escaped quote.
			else if (c == '"' && next.has_value() && next.value() == '"')
			{
				token += '"';
				++i;
			}
			// Quote that isn't escaped.
			else if (c == '"')
			{
				// We reached the end of the quoted word.
				// Store the current word and reset for the next one.
				wordStart = true;
				wordQuoted = false;
				tokens.push_back(token);
				token.clear();

				// Advance to the comma, ignoring anything after the closing quote.
				// Text after an unescaped quote is invalid, so just skip it.
				auto nextcomma = str.find(',', i + 1);
				if (nextcomma == std::string::npos)
					break;
				i = nextcomma;
			}
			else
			{
				// Add the character as is.
				token += c;
			}
		}
		else
		{
			if (c == ',')
			{
				// We reached the end of the quoted word.
				// Store the current word and reset for the next one.
				wordStart = true;
				wordQuoted = false;
				tokens.push_back(token);
				token.clear();
			}
			else
			{
				// Add the current character to the token.
				token += c;
				wordStart = false;
			}
		}
	}

	// Push the last token if it exists.
	if (!token.empty())
		tokens.push_back(token);

	return tokens;
}

///////////////////////////////////////////////////////////////////////////////

/// @brief Performs a case-insensitive comparison of two string-like objects.
/// @param str1 The first string-like object to compare.
/// @param str2 The second string-like object to compare.
/// @return An integer less than, equal to, or greater than zero if str1 is found, respectively, to be less than, to match, or be greater than str2 in a case-insensitive comparison.
int comparei(StringViewIshVariant auto const& str1, StringViewIshVariant auto const& str2)
{
	auto it1 = str1.begin();
	auto it2 = str2.begin();
	while (it1 != str1.end() && it2 != str2.end())
	{
		if (std::tolower(*it1) != std::tolower(*it2))
			return std::tolower(*it1) - std::tolower(*it2);
		++it1;
		++it2;
	}
	return str1.size() - str2.size();
}

/// @brief Performs a case-insensitive equality check of two string-like objects.
/// @param str1 The first string-like object to compare.
/// @param str2 The second string-like object to compare.
/// @return true if the strings are equal (case-insensitive), false otherwise.
bool equalsi(StringViewIshVariant auto const& str1, StringViewIshVariant auto const& str2)
{
	return comparei(std::forward<decltype(str1)>(str1), std::forward<decltype(str2)>(str2)) == 0;
}

/// @brief Finds the first occurrence of a substring within a string, ignoring case, starting from a specified position.
/// @param str The string to search within.
/// @param substr The substring to search for.
/// @param pos The position in the string to start the search from. Defaults to 0.
/// @return The index of the first occurrence of the substring (case-insensitive) in the string after the specified position, or std::string::npos if not found.
size_t findi(StringViewIshVariant auto const& str, StringViewIshVariant auto const& substr, size_t pos = 0)
{
	if (pos >= str.size())
		return std::string::npos;

	auto it = std::search(str.begin() + pos, str.end(), substr.begin(), substr.end(),
		[](auto a, auto b)
		{
			return std::tolower(a) == std::tolower(b);
		});

	if (it == str.end())
		return std::string::npos;

	return std::distance(str.begin(), it);
}

/// @brief Checks whether a string begins with a given prefix using a case-insensitive comparison.
/// @param str The string to check.
/// @param prefix The prefix to test for.
/// @return true if str starts with prefix (case-insensitive), otherwise false.
bool starts_withi(StringViewIshVariant auto const& str, StringViewIshVariant auto const& prefix)
{
	return findi(str, prefix, 0) == 0;
}

/// @brief Returns true if str ends with suffix, performing a case-insensitive comparison.
/// @param str The string to test.
/// @param suffix The suffix to check for. If suffix.size() > str.size(), the function returns false.
/// @return true if str ends with suffix when compared case-insensitively; otherwise false.
bool ends_withi(StringViewIshVariant auto const& str, StringViewIshVariant auto const& suffix)
{
	if (suffix.size() > str.size())
		return false;

	using Elem = std::remove_cvref_t<decltype(str)>::value_type;
	using Traits = std::remove_cvref_t<decltype(str)>::traits_type;
	using StringViewType = std::basic_string_view<Elem, Traits>;
	StringViewType strview{ str };

	strview = strview.substr(strview.size() - suffix.size());
	return equalsi(strview, suffix);
}

///////////////////////////////////////////////////////////////////////////////

/// @brief Converts all characters in the input string to uppercase, using the current C locale (not locale-aware).
/// @param str The input string or string view to convert to uppercase. Accepts any type compatible with StringViewIshVariant.
/// @return A new string with all characters converted to uppercase, preserving the original string's character type and traits.
auto toUpper(StringViewIshVariant auto const& str)
{
	using Elem = std::remove_cvref_t<decltype(str)>::value_type;
	using Traits = std::remove_cvref_t<decltype(str)>::traits_type;

	std::basic_string<Elem, Traits> ret{};
	ret.reserve(str.size());

	auto r = std::transform([](const Elem& c) { return static_cast<Elem>(std::toupper(static_cast<int>(c))); });
	std::ranges::copy(str | r, std::back_inserter(ret));
	return ret;
}

/// @brief Converts all characters in the input string to lowercase, using the current C locale (not locale-aware).
/// @param str The input string or string view to be converted to lowercase. Accepts any type compatible with StringViewIshVariant.
/// @return A new string with all characters from the input converted to lowercase, preserving the original character and traits types.
auto toLower(StringViewIshVariant auto const& str)
{
	using Elem = std::remove_cvref_t<decltype(str)>::value_type;
	using Traits = std::remove_cvref_t<decltype(str)>::traits_type;

	std::basic_string<Elem, Traits> ret{};
	ret.reserve(str.size());

	auto r = std::views::transform([](const Elem& c) { return static_cast<Elem>(std::tolower(static_cast<int>(c))); });
	std::ranges::copy(str | r, std::back_inserter(ret));
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

/// @brief Attempts to convert a string to a number of the specified integral type.
/// @tparam T The integral type to convert the string to. Defaults to int32_t.
/// @param str The string to convert to a number.
/// @param result Reference to a variable where the converted number will be stored if the conversion succeeds.
/// @return true if the conversion was successful; false otherwise.
template <std::integral T = int32_t>
bool toNumber(std::string_view str, T& result)
{
	try
	{
		char* p_end = nullptr;
		const long num = std::strtol(str.data(), &p_end, 10);
		if (p_end == str.data())
			return false;

		result = num;
		return true;
	}
	catch (...)
	{
		result = 0;
		return false;
	}
}

/// @brief Converts a string to a number of the specified integral type.
/// @tparam T The integral type to convert the string to. Defaults to int32_t.
/// @param str The string to convert to a number.
/// @return The converted number if the conversion succeeds; otherwise, returns 0 of the specified type.
template <std::integral T = int32_t>
T toNumber(std::string_view str)
{
	if (T result{}; toNumber(str, result))
		return result;

	return static_cast<T>(0);
}

/// @brief Attempts to convert a string to a float value.
/// @param str The input string to convert to a float.
/// @param result Reference to a float variable where the converted value will be stored if the conversion succeeds.
/// @return true if the conversion was successful and the result is stored in 'result'; false otherwise.
inline bool toFloat(std::string_view str, float& result)
{
	try
	{
		char* p_end = nullptr;
		const float num = std::strtof(str.data(), &p_end);
		if (p_end == str.data())
			return false;

		result = num;
		return true;
	}
	catch (...)
	{
		result = 0.0f;
		return false;
	}
}

/// @brief Converts a string to a float value.
/// @param str The string to convert to a float.
/// @return The float value represented by the string, or 0.0f if the conversion fails.
inline float toFloat(std::string_view str)
{
	if (float result; toFloat(str, result))
		return result;

	return 0.0f;
}

/// @brief Attempts to convert a string to a double-precision floating-point number.
/// @param str The input string to convert.
/// @param result Reference to a double where the converted value will be stored if the conversion succeeds.
/// @return true if the conversion was successful and the result is stored in 'result'; false otherwise.
inline bool toDouble(std::string_view str, double& result)
{
	try
	{
		char* p_end = nullptr;
		const double num = std::strtod(str.data(), &p_end);
		if (p_end == str.data())
			return false;

		result = num;
		return true;
	}
	catch (...)
	{
		result = 0.0;
		return false;
	}
}

/// @brief Converts a string to a double-precision floating-point number.
/// @param str The string to convert to a double.
/// @return The converted double value if the conversion is successful; otherwise, returns 0.0.
inline double toDouble(std::string_view str)
{
	if (double result; toDouble(str, result))
		return result;

	return 0.0f;
}

/// @brief Converts a string to its Base64 encoded representation.
/// @param str The string-like object to be encoded.
/// @return A Base64 encoded string.
inline std::string toBase64(std::span<uint8_t> in)
{
	static constexpr std::array<const char, 64> encodingTable = {
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
		'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
		'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		'w', 'x', 'y', 'z', '0', '1', '2', '3',
		'4', '5', '6', '7', '8', '9', '+', '/'
	};

	// Calculate the lengths of the input and output.
	size_t in_len = in.size();
	size_t out_len = 4 * ((in_len + 2) / 3);

	// Create the output buffer.
	std::string out(out_len, '\0');
	auto p = out.data();

	// Encode the input string.
	size_t i;
	for (i = 0; i < in_len - 2; i += 3)
	{
		*p++ = encodingTable[(in[i] >> 2) & 0x3F];
		*p++ = encodingTable[(static_cast<size_t>(in[i] & 0x3) << 4) | (static_cast<size_t>(in[i + 1] & 0xF0) >> 4)];
		*p++ = encodingTable[(static_cast<size_t>(in[i + 1] & 0xF) << 2) | (static_cast<size_t>(in[i + 2] & 0xC0) >> 6)];
		*p++ = encodingTable[static_cast<size_t>(in[i + 2] & 0x3F)];
	}

	// Handle padding for remaining bytes.
	if (i < in_len)
	{
		*p++ = encodingTable[(in[i] >> 2) & 0x3F];
		if (i == (in_len - 1))
		{
			*p++ = encodingTable[(static_cast<size_t>(in[i] & 0x3) << 4)];
			*p++ = '=';
		}
		else
		{
			*p++ = encodingTable[(static_cast<size_t>(in[i] & 0x3) << 4) | (static_cast<size_t>(in[i + 1] & 0xF0) >> 4)];
			*p++ = encodingTable[(static_cast<size_t>(in[i + 1] & 0xF) << 2)];
		}
		*p++ = '=';
	}

	return out;
}

/// @brief Converts a Base64 encoded string back to its original representation.
/// @param str The Base64 encoded string-like object to be decoded.
/// @return A string containing the decoded data. If the input is not valid Base64, returns the input as is.
inline std::vector<uint8_t> fromBase64(StringViewIshVariant auto const& str)
{
	static constexpr std::array<uint8_t, 256> decodingTable = {
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
		52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
		64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
		15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
		64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	};

	// Check if our input is a valid length.
	size_t in_len = str.length();
	if (in_len % 4 != 0)
		return {};

	// Calculate the output length.
	size_t out_len = in_len / 4 * 3;
	if (str[in_len - 1] == '=') out_len--;
	if (str[in_len - 2] == '=') out_len--;

	// Prepare the output buffer.
	std::vector<uint8_t> out{ out_len, 0 };

	// Decode.
	for (size_t i = 0, j = 0; i < in_len;)
	{
		uint32_t a = str[i] == '=' ? 0 & i++ : decodingTable[static_cast<int>(str[i++])];
		uint32_t b = str[i] == '=' ? 0 & i++ : decodingTable[static_cast<int>(str[i++])];
		uint32_t c = str[i] == '=' ? 0 & i++ : decodingTable[static_cast<int>(str[i++])];
		uint32_t d = str[i] == '=' ? 0 & i++ : decodingTable[static_cast<int>(str[i++])];

		uint32_t triple = (a << 3 * 6) + (b << 2 * 6) + (c << 1 * 6) + (d << 0 * 6);

		if (j < out_len) out[j++] = (triple >> 2 * 8) & 0xFF;
		if (j < out_len) out[j++] = (triple >> 1 * 8) & 0xFF;
		if (j < out_len) out[j++] = (triple >> 0 * 8) & 0xFF;
	}

	return out;
}

/// @brief Bring std::to_string into this namespace so we can overload it.
using std::to_string;

/// @brief Converts a double value to a string with the specified number of decimal places.
/// @param value The double value to convert to a string.
/// @param precision The number of digits to display after the decimal point.
/// @return A string representation of the value with the specified precision.
inline auto to_string(double value, int precision)
{
	return std::format("{:0.{}f}", value, precision);
}

/// @brief Converts a double value to a string with specified width and precision.
/// @param value The double value to convert to a string.
/// @param width The minimum width of the resulting string, including padding if necessary.
/// @param precision The number of digits to display after the decimal point.
/// @return A string representation of the value, formatted with the given width and precision.
inline auto to_string(double value, int width, int precision)
{
	return std::format("{:0{}.{}f}", value, width, precision);
}

///////////////////////////////////////////////////////////////////////////////

/// @brief Extracts the next line or substring from a string view, using a specified delimiter.
/// @param str A reference to the string view to extract from. This will be updated to remove the extracted line.
/// @param delim The delimiter character to use for splitting lines. Defaults to '\\n'.
/// @return A string containing the extracted line or substring up to the delimiter. If the delimiter is not found, returns the remainder of the string.
inline std::string extractLine(std::string_view& str, char delim = '\n')
{
	const auto pos = str.find(delim);
	if (pos == std::string::npos)
	{
		std::string_view line = str;
		str = {};
		return std::string(line);
	}

	const auto line = str.substr(0, pos);
	str.remove_prefix(pos + 1);
	return std::string(line);
}

/// @brief Splits a string into two trimmed parts using a specified delimiter.
/// @param str The input string to split.
/// @param delim The character used as the delimiter to split the string. Defaults to a space (' ').
/// @return A pair of std::string_view objects: the first is the trimmed substring before the delimiter, the second is the trimmed substring after the delimiter (or empty if the delimiter is not found).
inline std::pair<std::string_view, std::string_view> extractConfigParts(StringViewIshVariant auto const& str, char delim = ' ')
{
	using StrType = std::remove_cvref_t<decltype(str)>;

	const auto pos = str.find(delim);
	if (pos == StrType::npos)
		return { string::trim(str), std::string_view{} };
	return { string::trim(str.substr(0, pos)), string::trim(str.substr(pos + 1)) };
}

///////////////////////////////////////////////////////////////////////////////

template<bool ignoreCase = false>
inline bool match(StringViewIshVariant auto const& str, StringViewIshVariant auto const& mask)
{
	using str_value_type = std::remove_cvref_t<decltype(str)>::value_type;
	using mask_value_type = std::remove_cvref_t<decltype(mask)>::value_type;

	static_assert(std::same_as<str_value_type, mask_value_type>, "String and mask must have the same character type");

	const str_value_type* curpos = str.data();
	const mask_value_type* maskpos = mask.data();
	const mask_value_type* laststarpos = nullptr;
	while (*curpos != 0)
	{
		// Star (match any).
		if (*maskpos == static_cast<str_value_type>('*'))
		{
			if (!*++maskpos)
				return true;
			else
			{
				laststarpos = maskpos - 1;

				if constexpr (ignoreCase)
				{
					const auto m = std::tolower(static_cast<int>(*maskpos));
					while (*curpos != 0 && std::tolower(static_cast<int>(*curpos)) != m)
						curpos++;
				}
				else
				{
					while (*curpos != 0 && *curpos != *maskpos)
						curpos++;
				}
			}
		}

		// Check for a character match.
		bool match = false;
		if constexpr (ignoreCase)
		{
			match = (std::tolower(static_cast<int>(*maskpos)) == std::tolower(static_cast<int>(*curpos)));
		}
		else
		{
			match = (*maskpos == *curpos);
		}

		// Exact match or single character (match one).
		if (match || (*maskpos == static_cast<str_value_type>('?')))
		{
			maskpos++;
			curpos++;
		}
		else
		{
			// If we did not have a previous star, abort.
			if (!laststarpos)
				return false;

			// Otherwise, back up to it.
			maskpos = laststarpos;
			continue;
		}

		// Reach the end of the string.
		if (*curpos == 0 && *maskpos == 0)
			return true;

		// Matchstring too short.
		if (!*curpos && *maskpos && *maskpos != static_cast<str_value_type>('*'))
			return false;
	}

	// Still match characters left.
	return false;
}

/////////////////////////////////////////////////////////////////////
} // end namespace preagonal::string

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::range
{
///////////////////////////////////////////////////////////////////////////////

/// Transforms a range of std::string_view to std::string.
constexpr auto as_string = std::views::transform([](std::string_view s) { return std::string(s); });

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::range

namespace utilities
{
	std::string retokenizeArray(const std::vector<CString>& triggerData, int start_idx = 0);
	CString retokenizeCStringArray(const std::vector<CString>& triggerData, int start_idx = 0);
} // namespace utilities

#endif // STRINGUTILS_H
