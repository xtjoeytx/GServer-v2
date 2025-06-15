#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <algorithm>
#include <array>
#include <cctype>
#include <concepts>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <ranges>
#include <sstream>
#include <string_view>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <CString.h>

using namespace std::literals::string_view_literals;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::string
{
///////////////////////////////////////////////////////////////////////////////

// A concept that checks if a type is a string.
template <typename T>
concept StringVariant = std::same_as<std::remove_cvref_t<T>, std::string>; // || std::same_as<T, std::u8string>;

// A concept that checks if a type is a string or string_view.
template <typename T>
concept StringViewVariant = StringVariant<T> || std::same_as<std::remove_cvref_t<T>, std::string_view>; // || std::same_as<T, std::u8string_view>;

// A concept that checks if a type is a forward range, but not a string.
template <typename T>
concept ForwardRangeNotString = std::ranges::forward_range<T> && !StringViewVariant<T>;

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
	[[nodiscard]] bool operator()(const char* lhs, const std::string& rhs) const noexcept
	{
		return lhs == rhs;
	}
	[[nodiscard]] bool operator()(const std::string_view& lhs, const std::string& rhs) const noexcept
	{
		return lhs == rhs;
	}
	[[nodiscard]] bool operator()(const std::string& lhs, const std::string& rhs) const noexcept
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
};

///////////////////////////////////////////////////////////////////////////////

/// @brief Trims whitespace from the start of a string.
/// @param str A string or string_view to trim.
/// @return A string_view to the trimmed string.
std::string_view trimLeft(StringViewVariant auto const& str)
{
	std::string_view view{ str };
	auto size = str.size();
	for (size_t i = 0; i < size; ++i)
	{
		if (!std::isspace(static_cast<unsigned char>(view[i])))
			return view.substr(i, size - i);
	}
	return {};
}

/// @brief Trims whitespace from the end of a string.
/// @param str A string or string_view to trim.
/// @return A string_view to the trimmed string.
std::string_view trimRight(StringViewVariant auto const& str)
{
	std::string_view view{ str };
	for (size_t i = view.size(); i > 0; --i)
	{
		if (!std::isspace(static_cast<unsigned char>(view[i - 1])))
			return view.substr(0, i);
	}
	return {};
}

/// @brief Trims whitespace from the start and end of a string.
/// @param str A string or string_view to trim.
/// @return A string_view to the trimmed string.
std::string_view trim(StringViewVariant auto const& str)
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
	while (idx < str.length() && std::isspace(int(p[idx])))
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
	while (idx > 0 && std::isspace(int(p[idx - 1])))
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
	while (front < str.length() && std::isspace(static_cast<int>(p[front])))
		++front;
	while (front < back && std::isspace(static_cast<int>(p[back - 1])))
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

///////////////////////////////////////////////////////////////////////////////

/// @brief Escapes quotes in a string using a CSV-like format.
/// @param str The input string or string_view to escape quotes in.
/// @return A new string with quotes escaped.
auto escapeQuotes(StringViewVariant auto const str)
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
auto unescapeQuotes(StringVariant auto const& str)
{
	using Elem = std::remove_cvref_t<decltype(str)>::value_type;
	using Traits = std::remove_cvref_t<decltype(str)>::traits_type;

	// The shortest an escaped character can be is 2 characters.
	if (str.size() < 2)
		return str;

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

// Splits a string on the specified delimiter, returning a range.
/* GCC hates this.
auto split(std::string_view str, std::string_view delim = "\n"sv)
{
	return str
		| std::views::split(delim)
		| std::views::transform([](auto r) { return std::string_view{ r }; });
}
*/

/// @brief Splits a string into tokens based on a set of delimiter characters, returning the results as a vector.
/// @tparam T The type of each token in the resulting vector. Defaults to std::string.
/// @param str The input string to split. Can be any type compatible with string view semantics.
/// @param delims A set of delimiter characters used to split the string. Defaults to whitespace characters (space, tab, newline, carriage return).
/// @return A vector containing the tokens extracted from the input string, with each token converted to type T.
template <typename T = std::string>
std::vector<T> splitHard(StringViewVariant auto const& str, StringViewVariant auto delims = " \t\n\r"sv)
{
	using Elem = std::remove_cvref_t<decltype(str)>::value_type;
	using Traits = std::remove_cvref_t<decltype(str)>::traits_type;

	std::vector<T> tokens{};
	std::basic_string_view<Elem, Traits> strview{ str };

	// Collect the tokens.
	size_t start = 0, end = 0;
	while (start < str.length())
	{
		// Find the next delimiter.
		end = strview.find_first_of(delims, start);

		// None found, so add the rest of the string.
		if (end == std::string_view::npos)
		{
			tokens.push_back(T{ strview.substr(start) });
			break;
		}

		// Add the token to the vector.
		if (end > start)
			tokens.push_back(T{ strview.substr(start, end - start) });

		start = end + 1;
	}

	return tokens;
}

/// @brief Joins the elements of a range into a single string, separated by a specified delimiter.
/// @param range A forward range containing elements to join. The elements must be streamable to std::ostringstream.
/// @param delim The delimiter string to insert between elements. Defaults to ','.
/// @return A string containing the joined elements of the range, separated by the specified delimiter.
std::string join(std::ranges::forward_range auto&& range, std::string_view delim = ",")
{
	std::ostringstream oss;
	auto it = std::ranges::begin(range);
	if (it != std::ranges::end(range))
	{
		oss << *it;
		++it;
	}
	for (; it != std::end(range); ++it)
		oss << delim << *it;
	return oss.str();
}

///////////////////////////////////////////////////////////////////////////////

/// @brief Converts a range of strings to a single CSV-formatted string, quoting fields as needed.
/// @param range A forward range of string-like elements to be converted to CSV format.
/// @param force_quoted If true, all fields will be quoted regardless of content. Defaults to false.
/// @return A std::string containing the CSV-formatted representation of the input range, with fields separated by commas and quoted as necessary.
auto toCSV(ForwardRangeNotString auto&& range, bool force_quoted = false)
{
	constexpr std::array<char, 3> complexChars = { '"', ',', '\\' };
	std::ostringstream oss;

	for (const auto wordFromRange : range)
	{
		std::string_view word{ wordFromRange };
		if (word.empty())
			continue;

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
auto toCSV(StringViewVariant auto const& str, char delim = '\n', bool force_quoted = false)
{
	auto s = splitHard(str, std::string_view(&delim, 1));
	return toCSV(s, force_quoted);
}

/// @brief Parses a CSV-formatted string into a vector of strings, handling quoted fields and optional leading whitespace.
/// @param str The input string or string view containing CSV data to parse.
/// @param ignoreLeadingWhitespace If true, leading spaces and tabs before each field are ignored. Defaults to false.
/// @return A vector of strings, each representing a parsed field from the CSV input.
std::vector<std::string> fromCSV(StringViewVariant auto const& str, bool ignoreLeadingWhitespace = false)
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
		if (wordQuoted && (c == '\\' || c == '"'))
		{
			if (i + 1 >= str.length())
				break;

			const auto& next = str[i + 1];

			// Escaped backslash.
			if (c == '\\' && next == '\\')
			{
				token += '\\';
				++i;
			}
			// Escaped quote.
			else if (c == '"' && next == '"')
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
			}
		}
	}

	return tokens;
}

///////////////////////////////////////////////////////////////////////////////

/// @brief Performs a case-insensitive comparison of two string-like objects.
/// @param str1 The first string-like object to compare.
/// @param str2 The second string-like object to compare.
/// @return An integer less than, equal to, or greater than zero if str1 is found, respectively, to be less than, to match, or be greater than str2 in a case-insensitive comparison.
int comparei(StringViewVariant auto str1, StringViewVariant auto str2)
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

/// @brief Finds the first occurrence of a substring within a string, ignoring case, starting from a specified position.
/// @param str The string to search within.
/// @param substr The substring to search for.
/// @param pos The position in the string to start the search from. Defaults to 0.
/// @return The index of the first occurrence of the substring (case-insensitive) in the string after the specified position, or std::string::npos if not found.
size_t findi(StringViewVariant auto str, StringViewVariant auto substr, size_t pos = 0)
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

///////////////////////////////////////////////////////////////////////////////

/// @brief Converts all characters in the input string to uppercase, using the current C locale (not locale-aware).
/// @param str The input string or string view to convert to uppercase. Accepts any type compatible with StringViewVariant.
/// @return A new string with all characters converted to uppercase, preserving the original string's character type and traits.
auto toUpper(StringViewVariant auto str)
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
/// @param str The input string or string view to be converted to lowercase. Accepts any type compatible with StringViewVariant.
/// @return A new string with all characters from the input converted to lowercase, preserving the original character and traits types.
auto toLower(StringViewVariant auto str)
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
bool toNumber(const std::string& str, T& result)
{
	char* p_end = nullptr;
	const long num = std::strtol(str.c_str(), &p_end, 10);
	if (p_end == str.c_str())
		return false;

	result = num;
	return true;
}

/// @brief Converts a string to a number of the specified integral type.
/// @tparam T The integral type to convert the string to. Defaults to int32_t.
/// @param str The string to convert to a number.
/// @return The converted number if the conversion succeeds; otherwise, returns 0 of the specified type.
template <std::integral T = int32_t>
T toNumber(const std::string& str)
{
	T result{};
	if (toNumber(str, result))
		return result;

	return static_cast<T>(0);
}

/// @brief Attempts to convert a string to a float value.
/// @param str The input string to convert to a float.
/// @param result Reference to a float variable where the converted value will be stored if the conversion succeeds.
/// @return true if the conversion was successful and the result is stored in 'result'; false otherwise.
inline bool toFloat(const std::string& str, float& result)
{
	char* p_end = nullptr;
	const float num = std::strtof(str.c_str(), &p_end);
	if (p_end == str.c_str())
		return false;

	result = num;
	return true;
}

/// @brief Converts a string to a float value.
/// @param str The string to convert to a float.
/// @return The float value represented by the string, or 0.0f if the conversion fails.
inline float toFloat(const std::string& str)
{
	float result;
	if (toFloat(str, result))
		return result;

	return 0.0f;
}

/// @brief Attempts to convert a string to a double-precision floating-point number.
/// @param str The input string to convert.
/// @param result Reference to a double where the converted value will be stored if the conversion succeeds.
/// @return true if the conversion was successful and the result is stored in 'result'; false otherwise.
inline bool toDouble(const std::string& str, double& result)
{
	char* p_end = nullptr;
	const double num = std::strtod(str.c_str(), &p_end);
	if (p_end == str.c_str())
		return false;

	result = num;
	return true;
}

/// @brief Converts a string to a double-precision floating-point number.
/// @param str The string to convert to a double.
/// @return The converted double value if the conversion is successful; otherwise, returns 0.0.
inline double toDouble(const std::string& str)
{
	double result;
	if (toDouble(str, result))
		return result;

	return 0.0f;
}

///////////////////////////////////////////////////////////////////////////////

/// @brief Extracts the next line or substring from a string view, using a specified delimiter.
/// @param str A reference to the string view to extract from. This will be updated to remove the extracted line.
/// @param delim The delimiter character to use for splitting lines. Defaults to '\n'.
/// @return A string containing the extracted line or substring up to the delimiter. If the delimiter is not found, returns the remainder of the string.
inline std::string extractLine(std::string_view& str, char delim = '\n')
{
	auto pos = str.find(delim);
	if (pos == std::string::npos)
	{
		auto line = str;
		str = {};
		return std::string(line);
	}

	auto line = str.substr(0, pos);
	str.remove_prefix(pos + 1);
	return std::string(line);
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::string

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::range
{
///////////////////////////////////////////////////////////////////////////////

/// Transforms a range of std::string_view to std::string.
const auto as_string = std::views::transform([](std::string_view s) { return std::string(s); });

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::range

namespace utilities
{
	std::string retokenizeArray(const std::vector<CString>& triggerData, int start_idx = 0);
	CString retokenizeCStringArray(const std::vector<CString>& triggerData, int start_idx = 0);
} // namespace utilities

#endif // STRINGUTILS_H
