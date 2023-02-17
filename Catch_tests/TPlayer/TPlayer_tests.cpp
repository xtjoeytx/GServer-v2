#define CATCH_CONFIG_MAIN
#include "catch2/catch_all.hpp"
#include <TPlayer.h>
#include <TServer.h>

SCENARIO( "TPlayer", "[object]" ) {

	GIVEN( "TPlayer" ) {
		int id = 123;
		auto* server = new TServer("test");
		auto* socket = new CSocket();
		auto* player = new TPlayer(server, (CSocket*)socket, id);

		WHEN( "getting player id" ) {
			THEN( "id should be " << id ) {
				REQUIRE( player->getId() == id );
			}
		}

		WHEN( "getting player servername" ) {
			THEN( "name should be test" ) {
				REQUIRE( player->getServerName() == "test" );
			}
		}
	}
}