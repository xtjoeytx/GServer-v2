#define CATCH_CONFIG_MAIN
#include "catch2/catch_all.hpp"
#include <CString.h>

SCENARIO( "CString", "[string]" ) {

	GIVEN( "CString" ) {
		CString test;

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