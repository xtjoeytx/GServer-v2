#define CATCH_CONFIG_MAIN
#include "catch2/catch_all.hpp"
#include <Player.h>
#include <Server.h>

SCENARIO( "Player", "[object]" ) {

	GIVEN( "Player" ) {
		int id = 123;
		auto* server = new Server("test");
		auto* socket = new CSocket();
		auto* player = new Player(server, (CSocket*)socket, id);

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