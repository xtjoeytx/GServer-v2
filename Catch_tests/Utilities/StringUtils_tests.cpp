#include "catch2/catch_all.hpp"
#include <utilities/StringUtils.h>

using namespace preagonal;
using namespace std::literals::string_view_literals;

SCENARIO("string::trim and trimMutate remove whitespace and section signs")
{
	GIVEN("a string with leading and trailing trim characters")
	{
		const std::string input = " \t\r\n\xa7hello\xa7 \n";
		std::string mutableInput = input;

		WHEN("using non-mutating and mutating trim helpers")
		{
			const auto trimmedView = string::trim(input);
			string::trimMutate(mutableInput);

			THEN("both return the same trimmed content")
			{
				REQUIRE(trimmedView == "hello");
				REQUIRE(mutableInput == "hello");
			}
		}
	}
}

SCENARIO("string::replace and replaceMutate replace all occurrences")
{
	GIVEN("a sentence with repeated words")
	{
		const std::string original = "cat bobcat caterpillar cat";
		std::string mutableInput = original;

		WHEN("replacing cat with dog")
		{
			const auto replaced = string::replace(original, "cat", "dog");
			string::replaceMutate(mutableInput, "cat", "dog");

			THEN("both helpers apply every replacement")
			{
				REQUIRE(replaced == "dog bobdog dogerpillar dog");
				REQUIRE(mutableInput == replaced);
			}
		}

		WHEN("the from token is empty")
		{
			const auto replaced = string::replace(original, "", "x");
			string::replaceMutate(mutableInput, "", "x");

			THEN("the input is unchanged")
			{
				REQUIRE(replaced == original);
				REQUIRE(mutableInput == original);
			}
		}
	}
}

SCENARIO("string::eraseChars and eraseCharsMutate remove specified characters")
{
	GIVEN("a string with various characters")
	{
		const std::string input = "abc123!@#def456";
		std::string mutableInput = input;

		WHEN("erasing digits")
		{
			const auto erased = string::eraseChars(input, "0123456789");
			string::eraseCharsMutate(mutableInput, "0123456789");

			THEN("all digits are removed")
			{
				REQUIRE(erased == "abc!@#def");
				REQUIRE(mutableInput == erased);
			}
		}
	}
}

SCENARIO("string::splitToVector handles empty tokens and CRLF delimiters")
{
	GIVEN("a comma separated string with empty fields")
	{
		const std::string input = ",a,,b,";

		WHEN("keeping empty tokens")
		{
			const auto tokens = string::splitToVector(input, ","sv, false);

			THEN("empty fields are preserved")
			{
				REQUIRE(tokens == std::vector<std::string>{ "", "a", "", "b", "" });
			}
		}

		WHEN("ignoring empty tokens")
		{
			const auto tokens = string::splitToVector(input, ","sv, true);

			THEN("only non-empty fields are returned")
			{
				REQUIRE(tokens == std::vector<std::string>{ "a", "b" });
			}
		}
	}

	GIVEN("a CRLF-delimited string")
	{
		const std::string input = "one\r\ntwo\r\nthree";

		WHEN("splitting with carriage return delimiter and preserving empties")
		{
			const auto tokens = string::splitToVector(input, "\r"sv, false);

			THEN("CRLF pairs are treated as a single delimiter")
			{
				REQUIRE(tokens == std::vector<std::string>{ "one", "two", "three" });
			}
		}
	}
}

SCENARIO("string::splitToVectorByString keeps trailing empty token")
{
	GIVEN("a string ending with a string delimiter")
	{
		const std::string input = "alpha||beta||";

		WHEN("ignoreEmpty is false")
		{
			const auto tokens = string::splitToVectorByString(input, "||"sv, false);

			THEN("a trailing empty token is emitted")
			{
				REQUIRE(tokens == std::vector<std::string>{ "alpha", "beta", "" });
			}
		}
	}
}

SCENARIO("string::join concatenates strings together")
{
	GIVEN("a vector of strings")
	{
		const std::vector<std::string> input{ "one", "two", "three" };

		WHEN("joining with a comma delimiter")
		{
			const auto result = string::join(input, ","sv);

			THEN("the result is a single concatenated string")
			{
				REQUIRE(result == "one,two,three");
			}
		}

		WHEN("joining with an empty delimiter")
		{
			const auto result = string::join(input, ""sv);

			THEN("the result is a single concatenated string without separators")
			{
				REQUIRE(result == "onetwothree");
			}
		}
	}

	GIVEN("a vector of strings with one element")
	{
		const std::vector<std::string> input{ "single" };

		WHEN("joining with any delimiter")
		{
			const auto result = string::join(input, ","sv);

			THEN("the result is the single element")
			{
				REQUIRE(result == "single");
			}
		}
	}

	GIVEN("an empty vector of strings")
	{
		constexpr std::vector<std::string> input{};

		WHEN("joining with any delimiter")
		{
			const auto result = string::join(input, ","sv);

			THEN("the result is an empty string")
			{
				REQUIRE(result.empty());
			}
		}
	}
}

SCENARIO("string::tokenize respects quoted text")
{
	GIVEN("a command-like string with quoted tokens")
	{
		const std::string input = R"(say "hello world",player)";

		WHEN("tokenizing on default delimiters")
		{
			std::vector<std::string> tokens;
			for (const auto token : string::tokenize(std::string_view{ input }))
				tokens.emplace_back(token);

			THEN("quoted text remains a single token")
			{
				REQUIRE(tokens == std::vector<std::string>{ "say", "hello world", "player" });
			}
		}
	}
}

SCENARIO("string::tokenize with nested string arrays")
{
	GIVEN("a string with nested arrays")
	{
		const std::string input = R"("Some string",of,"""this is"",nested,data",data)";

		WHEN("tokenizing on default delimiters")
		{
			std::vector<std::string> tokens;
			for (const auto token : string::tokenize(std::string_view{ input }))
				tokens.emplace_back(token);

			THEN("nested quoted text is handled correctly")
			{
				REQUIRE(tokens == std::vector<std::string>{ "Some string", "of", "", "this is", ",nested,data", "data" });
			}
		}
	}
}

SCENARIO("string::toCSV and fromCSV support escaping")
{
	GIVEN("fields that require CSV escaping")
	{
		const std::vector<std::string> fields = { "simple", "has,comma", R"(quote " and slash \)" };

		WHEN("serializing and parsing")
		{
			const auto csv = string::toCSV(fields);
			const auto parsed = string::fromCSV(csv);

			THEN("the parsed data matches the original fields")
			{
				REQUIRE(parsed == fields);
			}
		}
	}
}

SCENARIO("string::fromCSV with nested string arrays")
{
	GIVEN("a string with nested arrays")
	{
		const std::string input = R"("Some string",of,"""this is"",nested,data",data)";

		WHEN("deserializing on default delimiters")
		{
			std::vector<std::string> tokens;
			for (const auto& token : string::fromCSV(std::string_view{ input }))
				tokens.emplace_back(token);

			THEN("nested quoted text is handled correctly")
			{
				REQUIRE(tokens == std::vector<std::string>{ "Some string", "of", "\"this is\",nested,data", "data" });
			}
		}
	}
}

SCENARIO("string case-insensitive helpers find expected matches")
{
	GIVEN("mixed case source text")
	{
		const std::string text = "HeLLo Wonderful WoRLD";

		THEN("comparison and search helpers ignore case")
		{
			REQUIRE(string::equalsi("hello"sv, "HELLO"sv));
			REQUIRE(string::comparei("abc"sv, "ABC"sv) == 0);
			REQUIRE(string::findi(text, "wonderful"sv) == 6);
			REQUIRE(string::starts_withi(text, "hello"sv));
			REQUIRE(string::ends_withi(text, "world"sv));
		}
	}
}

SCENARIO("string::isIntegral and string::isFloat detect a number")
{
	GIVEN("a string representing an integer")
	{
		const std::string intStr = "+42";

		THEN("isIntegral returns true and isFloat returns true")
		{
			REQUIRE(string::isIntegral(intStr));
			REQUIRE(string::isFloat(intStr));
		}
	}

	GIVEN("a string representing a floating-point number")
	{
		const std::string floatStr = "-3.14";

		THEN("isFloat returns true and isIntegral returns false")
		{
			REQUIRE(string::isFloat(floatStr));
			REQUIRE_FALSE(string::isIntegral(floatStr));
		}
	}

	GIVEN("a string that is a bad floating-point number")
	{
		const std::string floatStr = "-3.14.14";

		THEN("isFloat returns false and isIntegral returns false")
		{
			REQUIRE_FALSE(string::isFloat(floatStr));
			REQUIRE_FALSE(string::isIntegral(floatStr));
		}
	}

	GIVEN("a string that is not a number")
	{
		const std::string nonNumStr = "hello";

		THEN("both isIntegral and isFloat return false")
		{
			REQUIRE_FALSE(string::isIntegral(nonNumStr));
			REQUIRE_FALSE(string::isFloat(nonNumStr));
		}
	}
}

SCENARIO("string::toBase64 and fromBase64 encodes and decodes binary data")
{
	GIVEN("a binary data array")
	{
		const std::vector<uint8_t> data = { 0x48, 0x65, 0x6C, 0x6C, 0x6F }; // "Hello"
		const auto base64 = "SGVsbG8="sv;

		WHEN("encoding to Base64 and decoding back")
		{
			const auto encoded = string::toBase64(data);
			const auto decoded = string::fromBase64(encoded);

			THEN("the encoded data is a valid base64 encoding")
			{
				REQUIRE(encoded == base64);
			}

			THEN("the decoded data matches the original")
			{
				REQUIRE(decoded == data);
			}
		}
	}
}

SCENARIO("string::match supports wildcards and optional case-insensitivity")
{
	GIVEN("a wildcard mask")
	{
		THEN("the matcher handles *, ?, and case behavior")
		{
			REQUIRE(string::match("level01.txt"sv, "level??.*"sv));
			REQUIRE_FALSE(string::match("level01.txt"sv, "LEVEL??.*"sv));
			REQUIRE(string::match<true>("level01.txt"sv, "LEVEL??.*"sv));
		}
	}
}
