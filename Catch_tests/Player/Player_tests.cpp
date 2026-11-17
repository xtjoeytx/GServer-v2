#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include <atomic>

#include <CSocket.h>

#include <BabyDI.h>
#include <Server.h>
#include <object/Player.h>

using namespace preagonal;
using preagonal::Player;

std::atomic_bool shutdownProgram{false};

SCENARIO("Player", "[object]")
{

	GIVEN("Player")
	{
		int id = 123;
		auto* server = BabyDI_PROVIDE(Server, new Server("test"));

		auto* socket = new CSocket();
		const auto* player = new Player(socket, id);

		WHEN("getting player id")
		{
			THEN("id should be " << id)
			{
				REQUIRE(player->getId() == id);
			}
		}

		WHEN("getting player servername")
		{
			THEN("name should be test")
			{
				REQUIRE(player->getServerName() == "test");
			}
		}
	}
}

SCENARIO("Player packet handling after login", "[object]")
{
	BabyDI_PROVIDE(Server, new Server("test"));

	GIVEN("a player that was handed over from the login handler")
	{
		Player player(new CSocket(), 124);

		CString packet;
		packet << (char)(PLI_LANGUAGE + 32) << "English\n";
		CString buffer;
		buffer << (short)packet.length() << packet;

		WHEN("the login packet was already processed")
		{
			// Stands in for the hand-over in PlayerLogin::msgLoginPacket, which needs a socket and a full login line.
			player.setLoginProcessed();
			player.processBuffer(buffer);

			THEN("the first packet is handled normally")
			{
				REQUIRE(player.InvalidPackets == 0);
				REQUIRE(player.account.language == "English");
			}
		}
	}
}
