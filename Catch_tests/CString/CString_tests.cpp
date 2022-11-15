#define CATCH_CONFIG_MAIN
#include "catch2/catch_all.hpp"
#include <CString.h>

std::ostream& operator << ( std::ostream& os, CString const& value ) {
	os << value.toString();
	return os;
}

SCENARIO( "CString", "[string]" ) {
	GIVEN("A constant CString") {
		const CString helloWorldStr = "Hello world";
		
		THEN("Check substring") {
			REQUIRE(helloWorldStr.subString(0) == helloWorldStr);
			REQUIRE(helloWorldStr.subString(0, 5) == "Hello");
			REQUIRE(helloWorldStr.subString(0, 200) == helloWorldStr);
			REQUIRE(helloWorldStr.subString(200) == "");
		}
		
		THEN("check some constants") {
			REQUIRE(helloWorldStr.left(5) == "Hello");
			REQUIRE(helloWorldStr.right(5) == "world");
			REQUIRE(helloWorldStr.trim() == helloWorldStr);
		}
		
		WHEN("remove the first word") {
			CString res = helloWorldStr.remove(0, 5);
			
			THEN("check for removal") {
				REQUIRE(res == " world");
				
				AND_THEN("Trim the result") {
					REQUIRE(res.trim() == "world");
				}
			}
			
			THEN("atempt to remove invalid range") {
				REQUIRE(helloWorldStr.remove(200) == helloWorldStr);
				REQUIRE(helloWorldStr.remove(-1) == helloWorldStr);
				REQUIRE(helloWorldStr.remove(-1, 0) == helloWorldStr);
				
				AND_THEN("Remove entire string") {
					REQUIRE(helloWorldStr.remove(0).isEmpty());
				}
			}
		}
		
		WHEN("remove the first word") {
			CString res = helloWorldStr.remove(0, 5);
			
			THEN("check for removal") {
				REQUIRE(res == " world");
				
				AND_THEN("Trim the result") {
					REQUIRE(res.trim() == "world");
				}
			}
		}
		
		WHEN("escape string") {
			CString originalStr = CString("Test string's, hope it works out \\\\, \\, \", \', \"\" - / lol");
			CString stringEscaped = originalStr.escape();
			
			THEN("check escaped result") {
				printf("Test: %s\n", stringEscaped.toString().c_str());
				REQUIRE(originalStr == stringEscaped.unescape());
			}
		}
		
		WHEN("lowercase string") {
			CString lowerCase = helloWorldStr.toLower();
			THEN("the string should be lowercase") {
				REQUIRE(lowerCase == "hello world");
			}
		}
		
		WHEN("uppercase string") {
			CString upperCase = helloWorldStr.toUpper();
			THEN("the string should be uppercase") {
				REQUIRE(upperCase == "HELLO WORLD");
			}
		}
	}
	
	GIVEN("An empty CString") {
		CString test;
		
		WHEN("we attempt to trim an empty string, we get an empty string") {
			REQUIRE(test.trim().isEmpty());
		}
		
		WHEN("we attempt to set to empty std string") {
			test = std::string("");
			THEN("string is empty") {
				REQUIRE(test.isEmpty());;
			}
		}
		
		WHEN("we attempt to set to empty cstring") {
			CString emptyStr("");
			test = CString(emptyStr);
			THEN("string is empty") {
				REQUIRE(test.isEmpty());;
			}
		}
		
		WHEN("we construct a cstring with another CString 'hello'") {
			test = "hello";
			CString tmpStr(test);
			
			THEN("string == hello") {
				REQUIRE(tmpStr == "hello");
			}
		}
		
		WHEN("we construct the cstring with an integer") {
			test = CString(int(69));
			THEN("test == 69") {
				REQUIRE(test == "69");
			}
		}
		
		WHEN("we construct the cstring with an unsigned integer") {
			test = CString((unsigned int)2150069);
			THEN("check value of conversion to string") {
				REQUIRE(test == "2150069");
			}
		}
		
		WHEN("we construct the cstring with a long") {
			test = CString((long)-2107481369);
			THEN("check value of conversion to string") {
				REQUIRE(test == "-2107481369");
			}
		}
		
		WHEN("we construct the cstring with a unsigned long") {
			test = CString((unsigned long)3107481369);
			THEN("check value of conversion to string") {
				REQUIRE(test == "3107481369");
			}
		}
		
		WHEN("we construct the cstring with a long long") {
			test = CString((long long)348384823);
			THEN("check value of conversion to string") {
				REQUIRE(test == "348384823");
			}
		}
		
		WHEN("we construct the cstring with a long long") {
			test = CString((unsigned long long)15000321179321415669u);
			THEN("check value of conversion to string") {
				REQUIRE(test == "15000321179321415669");
			}
		}
		
		WHEN("we construct the cstring with a float (2-precision only)") {
			test = CString((float)1.250521);
			THEN("check value of conversion to string") {
				REQUIRE(test == "1.25");
			}
		}
		
		WHEN("we construct the cstring with a double") {
			test = CString((double)4.269);
			THEN("same output as std string") {
				REQUIRE(test == std::to_string(4.269));
			}
		}
	}
	GIVEN( "CString" ) {
		CString test;
		
		THEN("the string is empty") {
			REQUIRE(test.isEmpty());
			REQUIRE(test.length() == 0);
		}
		
		WHEN("we assign the string to another cstring") {
			CString other("hello world");
			test = other;
			
			THEN( "length should be 11" ) {
				REQUIRE(test.length() == 11);
			}
			
			THEN("the strings should be the same") {
				REQUIRE(test == other);
			}
			
			THEN("string should not be equal to hello") {
				REQUIRE(test != "hello");
			}
		}
		
		WHEN( "operator << (const char*)\"hello world\"" ) {
			const char* strHelloWorld = "hello world";
			test << strHelloWorld;

			THEN( "length should be 11" ) {
				REQUIRE( test.length() == 11 );
			}

			AND_THEN( "return should be \"hello world\"") {
				REQUIRE_THAT( test.text(), Catch::Matchers::Equals("hello world") );
			}
		}

		WHEN( "operator << (std::string)\"hello world\"" ) {
			std::string strHelloWorld = "hello world";
			test << strHelloWorld;

			THEN( "length should be 11" ) {
				REQUIRE( test.length() == 11 );
			}

			AND_THEN( "return should be \"hello world\"") {
				REQUIRE_THAT( test.text(), Catch::Matchers::Equals("hello world") );
			}
		}

		WHEN( "compared with another CString" ) {
			test << "hello world";
			CString test2 = "HeLlO wOrLd";

			int retVal = test.comparei(test2);

			THEN( "return should be true") {
				REQUIRE( retVal == true );
			}
		}

		WHEN( "compared with another CString" ) {
			test << "hello world";
			CString test2 = "HeLlO wOrZd";

			int retVal = test.comparei(test2);

			THEN( "return should not be true") {
				REQUIRE( retVal != true );
			}
		}
	}
}