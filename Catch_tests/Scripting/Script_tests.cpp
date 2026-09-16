#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include <atomic>
#include <memory>
#include <source_location>
#include <string>
#include <string_view>

#include <CSocket.h>

#include <BabyDI.h>
#include <Server.h>
#include <npcserver/NPCServer.h>
#include <scripting/IScriptEngine.h>
#include <scripting/Script.h>
#include <scripting/ScriptSystem.h>
#include <scripting/ScriptTypes.h>
#include <scripting/gs1/GS1Variables.h>
#include <scripting/gs1/ScriptEngineGS1.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/Log.h>

using namespace preagonal;
using namespace std::string_literals;
using namespace std::string_view_literals;

std::atomic_bool shutdownProgram{false};

struct ServerFixture
{
	ServerFixture()
	{
		log::networkdump.disabled = true;
		log::npc.disabled = true;
		log::rc.disabled = true;
		log::script.disabled = true;
		log::server.disabled = true;

		BabyDI_RELEASE(Server);
		server = BabyDI_PROVIDE(Server, new Server("test"));
		server->getSettings().set("serverside", true);
		server->loadNPCServer();

		// Link the scripting engine.
		npcServer = server->getNPCServer();
		engine = std::dynamic_pointer_cast<gs1::ScriptEngineGS1>(npcServer->scripting.getScriptEngine(gs1::ScriptEngineGS1::EngineName));

		// Configure NPC-Player.
		const auto player = std::dynamic_pointer_cast<Player>(server->getNPCServer()->getPlayerNPCServer());
		gs1::setPlayerVariables(player->account.variables, player);

		// Configure Test NPC.
		const auto npc = npcServer->addNPC("door.png"sv, ""sv, nullptr, TilePosition{20.0f, 30.0f}, NPCTYPE_OBJECT);
		npc->name = "Test";
		testNPC = npc->id;

		// Configure Test Clients.
		client = std::make_shared<PlayerClient>(new CSocket(), server->getPlayerIdGenerator().getAvailableId());
		client->setType(PLTYPE_CLIENT2);
		server->addPlayer(client, client->getId());
		npcServer->playerLogin(client);
	}

	NPCID testNPC = NPCID_GEN_DATABASE_LOCALN;
	Server* server = nullptr;
	std::shared_ptr<NPCServer> npcServer;
	std::shared_ptr<gs1::ScriptEngineGS1> engine;
	std::shared_ptr<PlayerClient> client;
};

////////////////////////////////////////////////////////////////////////////////

TEST_CASE_METHOD(ServerFixture, "Script joins", "[Scripting][IScriptEngine][GS1]")
{
	ScriptEvent created{.type = ScriptEventType::CREATED, .initiator = source::FromPlayer(NPCServerPlayerID)};
	const auto player = npcServer->getPlayerNPCServer();
	player->account.character = Character{};
	player->account.character.nickName = "NPC-Server (Server)";

	npcServer->addClass("TestClass"sv, "if (created) { this.test = 42; }"sv);
	server->getSettings().set("serverside", false);

	SECTION("clientside join hack")
	{
		constexpr std::string_view script = R"(
			//#CLIENTSIDE
			join TestClass;
		)";

		auto npc = server->getNPC(testNPC);
		REQUIRE(npc != nullptr);

		npc->setScript(script);
		REQUIRE_FALSE(npc->getScript().getClientSide().empty());
		CHECK(npc->getScript().getClientSide() == "//#CLIENTSIDE\xA7;\xA7if (created) { this.test = 42; }"s);
	}

	SECTION("clientside join hack ignores sign")
	{
		constexpr std::string_view script = R"(
			//#CLIENTSIDE
			say2 join TestClass#b
			join TestClass;
			if (created) { say2 join TestClass; }
		)";

		auto npc = server->getNPC(testNPC);
		REQUIRE(npc != nullptr);

		npc->setScript(script);
		REQUIRE_FALSE(npc->getScript().getClientSide().empty());
		CHECK(npc->getScript().getClientSide() == "//#CLIENTSIDE\xA7say2 join TestClass#b\xA7join TestClass;\xA7if (created) { say2 join TestClass; }"s);
	}
}
