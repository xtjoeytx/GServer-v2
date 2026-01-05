#define CATCH_CONFIG_MAIN
#include "catch2/catch_all.hpp"
#include <utilities/IdGenerator.h>

SCENARIO("IdGenerator")
{
	GIVEN("A default int generator")
	{
		IdGenerator<int> generator;

		WHEN("getting a new id")
		{
			THEN("the first id should be 0")
			{
				REQUIRE(generator.getAvailableId() == 0);
				AND_THEN("the second id should be 1")
				{
					REQUIRE(generator.getAvailableId() == 1);
				}
			}

			AND_WHEN("releasing id 0")
			{
				generator.freeId(0);
				THEN("peeking the next id should be 0")
				{
					REQUIRE(generator.peekNextId() == 0);
					AND_THEN("getting a new id should be 0")
					{
						REQUIRE(generator.getAvailableId() == 0);
					}
				}
			}

			AND_WHEN("reseting back to defaults")
			{
				generator.resetAndSetNext(0);
				THEN("peeking next id should be 0")
				{
					REQUIRE(generator.peekNextId() == 0);
					AND_THEN("getting a new id should be 0")
					{
						REQUIRE(generator.getAvailableId() == 0);
					}
				}
			}
		}
	}

	GIVEN("A generator that starts at 10001")
	{
		IdGenerator<int> generator{ 10001 };

		WHEN("getting a new id")
		{
			THEN("the first id should be 10001")
			{
				REQUIRE(generator.getAvailableId() == 10001);
				AND_THEN("the second id should be 10002")
				{
					REQUIRE(generator.getAvailableId() == 10002);
				}
			}

			AND_WHEN("releasing id 10001")
			{
				generator.freeId(10001);
				THEN("peeking next id should be 10001")
				{
					REQUIRE(generator.peekNextId() == 10001);
					AND_THEN("getting a new id should be 10001")
					{
						REQUIRE(generator.getAvailableId() == 10001);
					}
				}
			}

			AND_WHEN("reseting back to defaults")
			{
				generator.resetAndSetNext(10001);
				THEN("peeking next id should be 10001")
				{
					REQUIRE(generator.peekNextId() == 10001);
					AND_THEN("getting a new id should be 10001")
					{
						REQUIRE(generator.getAvailableId() == 10001);
					}
				}
			}
		}
	}
}
