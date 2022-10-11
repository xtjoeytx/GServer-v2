#define CATCH_CONFIG_MAIN
#include "catch2/catch_all.hpp"
#include <TPlayer.h>
#include <TServer.h>

SCENARIO( "TPlayer", "[object]" ) {

	GIVEN( "TPlayer" ) {
		int id = 123;
		auto* server = new TServer("test");
		auto* player = new TPlayer(server, nullptr, id);

		WHEN( "getting player id" ) {
			THEN( "id should be " << id ) {
				REQUIRE( player->getId() == id );
			}
		}
	}
}