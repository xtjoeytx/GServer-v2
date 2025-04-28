#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <string>
#include <string_view>
#include <vector>
#include <ranges>
#include <algorithm>
#include <concepts>
#include <cctype>
#include <cstdlib>
#include <sstream>

#include <CString.h>

///////////////////////////////////////////////////////////////////////////////

using namespace std::literals::string_view_literals;

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
concept ForwardRangeNotString = std::ranges::forward_range<T> && !StringVariant<T>;

///////////////////////////////////////////////////////////////////////////////

// A hash function for strings that can be used with heterogenous lookups.
struct string_hash
{
	using hash_type = std::hash<std::string_view>;
	using is_transparent = void;

	size_t operator()(const char* str) const noexcept
	{
		return hash_type{}(str);
	}
	size_t operator()(const std::string_view& str) const noexcept
	{
		return hash_type{}(str);
	}
	size_t operator()(const std::string& str) const noexcept
	{
		return hash_type{}(str);
	}
	size_t operator()(const CString& str) const noexcept
	{
		return hash_type{}(str.toStringView());
	}
};

///////////////////////////////////////////////////////////////////////////////

// Trims whitespace from the start of the string.
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

// Trims whitespace from the end of the string.
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

// Trims whitespace from the start and end of the string.
std::string_view trim(StringViewVariant auto const& str)
{
	return trimLeft(trimRight(str));
}

///////////////////////////////////////////////////////////////////////////////

// Escapes quotes in a string.
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

// Unescapes quotes in a string.
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

// Splits a string on the specified delimiter from a list, returning a vector of string_views.
template <typename T = std::string>
std::vector<T> splitHard(StringViewVariant auto const& str, StringViewVariant auto delims = " \t\n\r"sv)
{
	using Elem = std::remove_cvref_t<decltype(str)>::value_type;
	using Traits = std::remove_cvref_t<decltype(str)>::traits_type;

	//std::vector<std::basic_string_view<Elem, Traits>> tokens{};
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

// Transforms a view to a std::string.
// Useful in combination with split.
const auto as_string = std::views::transform([](std::string_view s) { return std::string(s); });

// Joins a range of strings with the specified delimiter.
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

// Converts a range of strings to a CSV string.
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

// Converts a string to a CSV string, splitting on the specified delimiter.
auto toCSV(StringViewVariant auto const& str, char delim = '\n', bool force_quoted = false)
{
	auto s = splitHard(str, std::string_view(&delim, 1));
	return toCSV(s, force_quoted);
}

// Converts a CSV string to a vector of strings.
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

// Compares two strings, ignoring case.
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

// Finds the first occurrence of a substring in a string, ignoring case.
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

// To uppercase.
std::string toUpper(StringViewVariant auto str)
{
	using Elem = std::remove_cvref_t<decltype(str)>::value_type;
	using Traits = std::remove_cvref_t<decltype(str)>::traits_type;

	std::basic_string<Elem, Traits> ret{};
	ret.reserve(str.size());

	auto r = std::transform([](const Elem& c) { return static_cast<Elem>(std::toupper(static_cast<int>(c))); });
	std::ranges::copy(ret, r, std::back_inserter(ret));
	return ret;
}

// To lowercase.
std::string toLower(StringViewVariant auto str)
{
	using Elem = std::remove_cvref_t<decltype(str)>::value_type;
	using Traits = std::remove_cvref_t<decltype(str)>::traits_type;

	std::basic_string<Elem, Traits> ret{};
	ret.reserve(str.size());

	auto r = std::transform([](const Elem& c) { return static_cast<Elem>(std::tolower(static_cast<int>(c))); });
	std::ranges::copy(ret, r, std::back_inserter(ret));
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

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

template <std::integral T = int32_t>
T toNumber(const std::string& str)
{
	T result{};
	if (toNumber(str, result))
		return result;

	return static_cast<T>(0);
}

inline bool toFloat(const std::string& str, float& result)
{
	char* p_end = nullptr;
	const float num = std::strtof(str.c_str(), &p_end);
	if (p_end == str.c_str())
		return false;

	result = num;
	return true;
}

inline float toFloat(const std::string& str)
{
	float result;
	if (toFloat(str, result))
		return result;

	return 0.0f;
}

///////////////////////////////////////////////////////////////////////////////

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

namespace utilities
{
	std::string retokenizeArray(const std::vector<CString>& triggerData, int start_idx = 0);
	CString retokenizeCStringArray(const std::vector<CString>& triggerData, int start_idx = 0);
} // namespace utilities

#endif // STRINGUTILS_H
