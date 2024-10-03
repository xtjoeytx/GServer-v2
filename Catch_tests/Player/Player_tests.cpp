#define CATCH_CONFIG_MAIN
#include "catch2/catch_all.hpp"
#include <BabyDI.h>
#include <Server.h>
#include <object/Player.h>

SCENARIO( "Player", "[object]" ) {

	GIVEN( "Player" ) {
		int id = 123;
		auto* server = BabyDI_PROVIDE(Server, new Server("test"));

		auto* socket = new CSocket();
		auto* player = new Player((CSocket*)socket, id);

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
