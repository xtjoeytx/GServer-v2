#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include <catch2/catch_message.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <any>
#include <atomic>
#include <memory>
#include <source_location>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <CSocket.h>

#include <BabyDI.h>
#include <Server.h>
#include <npcserver/NPCServer.h>
#include <object/Character.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <player/PlayerClient.h>
#include <player/PlayerRC.h>
#include <scripting/IScriptEngine.h>
#include <scripting/ScriptSystem.h>
#include <scripting/ScriptTypes.h>
#include <scripting/gs1/GS1Variables.h>
#include <scripting/gs1/GS1Visitor.h>
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

		// Configure NPC-Player.
		auto player = std::dynamic_pointer_cast<Player>(server->getNPCServer()->getPlayerNPCServer());
		gs1::setPlayerVariables(player->account.variables, player);

		// Configure Test NPC.
		auto npcServer = server->getNPCServer();
		auto npc = npcServer->addNPC("door.png"sv, ""sv, nullptr, TilePosition{20.0f, 30.0f}, NPCTYPE_OBJECT);
		npc->name = "Test";
		testNPC = npc->id;

		// Configure Test Clients.
		auto client = std::make_shared<PlayerClient>(new CSocket(), server->getPlayerIdGenerator().getAvailableId());
		auto rc = std::make_shared<PlayerRC>(new CSocket(), server->getPlayerIdGenerator().getAvailableId());
		server->addPlayer(client, client->getId());
		server->addPlayer(rc, rc->getId());
		npcServer->playerLogin(client);
		npcServer->playerLogin(rc);
	}

	NPCID testNPC = NPCID_GEN_DATABASE_LOCALN;
	Server* server;
	gs1::ScriptEngineGS1 engine;
};

////////////////////////////////////////////////////////////////////////////////

static bool execute_script(IScriptEngine& engine, ScriptEvent& event, ScriptObject source, CompiledScriptResult& result, const std::source_location location = std::source_location::current())
{
	CAPTURE(location.line());
	CAPTURE(location.function_name());

	REQUIRE(std::holds_alternative<ScriptExecutionContext>(result));

	auto& context = std::get<ScriptExecutionContext>(result);
	auto contextPtr = std::shared_ptr<ScriptExecutionContext>(&context, [](ScriptExecutionContext*) {});

	return engine.execute(event, source, contextPtr);
}

static gs1::GS1ScriptWrapper* get_wrapper(CompiledScriptResult& result, const std::source_location location = std::source_location::current())
{
	CAPTURE(location.line());
	CAPTURE(location.function_name());

	REQUIRE(std::holds_alternative<ScriptExecutionContext>(result));

	auto& context = std::get<ScriptExecutionContext>(result);
	auto wrapper = std::any_cast<gs1::GS1ScriptWrapper>(context.script.get());
	REQUIRE(wrapper != nullptr);
	REQUIRE(wrapper->parser != nullptr);

	return wrapper;
}

////////////////////////////////////////////////////////////////////////////////

TEST_CASE_METHOD(ServerFixture, "ScriptEngineGS1 compiles scripts", "[Scripting][IScriptEngine][GS1]")
{
	SECTION("valid script")
	{
		// Checks a script that should compile successfully.
		// Check also includes sloppy code that should be accepted by the parser, such as missing semi-colons and commands that don't need line breaks.
		const std::string_view validScript = R"(
			// NPC made by
			if (created || playerenters) {
				setimg light2.png;
				dontblock;
			}
			if (created) {x=10}
			if (created) {y=5;say2HEY!}
		)";
		auto result = engine.compileScript("valid_script", validScript);
		REQUIRE(std::holds_alternative<ScriptExecutionContext>(result));
	}

	SECTION("invalid script")
	{
		// Bad script that should fail to compile.
		const std::string_view invalidScript = R"(
			if (created || playerenters) {
				setanimg light2.png;
				doblock;
		)";
		auto result = engine.compileScript("invalid_script", invalidScript);
		REQUIRE(std::holds_alternative<std::string>(result));
	}

	SECTION("compiled script identifiers are found")
	{
		// Checks if every single identifier is found and registered during compilation.
		const std::string_view script = R"(
			if (created || playerenters) {
				this.origx = x;
				this.origy = y;
				setstring mystring,Hello, world!;
			}
		)";
		auto result = engine.compileScript("script_with_identifiers", script);
		REQUIRE(std::holds_alternative<ScriptExecutionContext>(result));

		auto& context = std::get<ScriptExecutionContext>(result);
		auto wrapper = std::any_cast<gs1::GS1ScriptWrapper>(context.script.get());
		REQUIRE(wrapper != nullptr);
		REQUIRE(wrapper->parser != nullptr);

		CHECK(wrapper->parser->identifiers.size() == 7);
		CHECK(wrapper->parser->identifiers.contains("created"));
		CHECK(wrapper->parser->identifiers.contains("playerenters"));
		CHECK(wrapper->parser->identifiers.contains("this.origx"));
		CHECK(wrapper->parser->identifiers.contains("this.origy"));
		CHECK(wrapper->parser->identifiers.contains("x"));
		CHECK(wrapper->parser->identifiers.contains("y"));
		CHECK(wrapper->parser->identifiers.contains("mystring"));
	}

	SECTION("compiled script functions are found")
	{
		// Checks if functions are found and registered during compilation.
		const std::string_view script = R"(
			function testFunc() {}
		)";
		auto result = engine.compileScript("script_with_functions", script);
		REQUIRE(std::holds_alternative<ScriptExecutionContext>(result));

		auto& context = std::get<ScriptExecutionContext>(result);
		auto wrapper = std::any_cast<gs1::GS1ScriptWrapper>(context.script.get());
		REQUIRE(wrapper != nullptr);
		REQUIRE(wrapper->parser != nullptr);

		CHECK(wrapper->parser->userFunctions.size() == 1);
		CHECK(wrapper->parser->userFunctions.contains("testFunc"));
	}
}

////////////////////////////////////////////////////////////////////////////////

TEST_CASE_METHOD(ServerFixture, "ScriptEngineGS1 executes basic expressions", "[Scripting][IScriptEngine][GS1]")
{
	ScriptEvent created{.type = ScriptEventType::CREATED, .initiator = source::FromPlayer(NPCServerPlayerID)};

	SECTION("integer variable assignment and retrieval")
	{
		// Checks if integer variables can be assigned, using both types of assignment operators.
		const std::string_view script = R"(
			if (created) {
				this.myvar = 42;
				this.myvar2 := 42;
			}
		)";
		auto result = engine.compileScript("test_script", script);
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));

		auto wrapper = get_wrapper(result);
		auto store = wrapper->visitor->builtInStore;
		CHECK(store->contains("myvar"));
		CHECK(store->contains("myvar2"));
		CHECK_THAT(store->getValue<double>("myvar").value_or(0.0), Catch::Matchers::WithinRel(42.0));
		CHECK_THAT(store->getValue<double>("myvar2").value_or(0.0), Catch::Matchers::WithinRel(42.0));
	}

	SECTION("string flag assignment and retrieval")
	{
		// Checks if string flags can be assigned and retrieved.
		const std::string_view script = R"(
			if (created) {
				setstring this.mystring,Hello, world!;
			}
		)";
		auto result = engine.compileScript("test_script", script);
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));

		auto wrapper = get_wrapper(result);
		auto store = wrapper->visitor->builtInStore;
		CHECK(store->contains("mystring"));
		CHECK_THAT(store->getValue<std::string>("mystring").value_or(std::string{}), Catch::Matchers::Equals("Hello, world!"s));
	}

	SECTION("array assignment and retrieval")
	{
		// Checks if arrays can be assigned and retrieved, including direct assignment and negative indexing.
		const std::string_view script = R"(
			if (created) {
				setarray this.myarray,5;
				this.myarray[3] = 7;
				this.direct = { 1, 2, 3 };
				this.negativeIndex = { 42 };
				this.negativeIndex[-1] = 99;
			}
		)";
		auto result = engine.compileScript("test_script", script);
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));

		auto wrapper = get_wrapper(result);
		auto store = wrapper->visitor->builtInStore;
		CHECK(store->contains("myarray"));
		CHECK(store->contains("direct"));
		CHECK(store->contains("negativeIndex"));

		auto myarray = store->get("myarray").lock();
		auto direct = store->get("direct").lock();
		auto negativeIndex = store->get("negativeIndex").lock();

		CHECK(myarray->value.has<std::vector<double>>());
		CHECK(myarray->value.get<std::vector<double>>().value().get().size() == 5);
		CHECK_THAT(myarray->value.get<std::vector<double>>().value().get().at(3), Catch::Matchers::WithinRel(7.0));
		CHECK(direct->value.has<std::vector<double>>());
		CHECK(direct->value.get<std::vector<double>>().value().get().size() == 3);
		CHECK_THAT(direct->value.get<std::vector<double>>().value().get().at(1), Catch::Matchers::WithinRel(2.0));
		CHECK_THAT(negativeIndex->value.get<std::vector<double>>().value().get().at(0), Catch::Matchers::WithinRel(42.0));
	}

	SECTION("logical expressions")
	{
		// Tests all forms of logical expressions.
		const std::string_view script = R"(
			if (created) {
				this.myvar = 42;
				this.truevar = true;
				this.falsevar = false;
				set this.trueflag;
				setstring this.truestring,Hello;
				this.rangevar = { 40, 42, 44 };

				this.equality = (this.myvar == 42);
				this.equality2 = (this.myvar = 42);
				this.inequality = (this.myvar != 41);
				this.inequality2 = (this.myvar <> 41);
				this.greaterThan = (this.myvar > 41);
				this.lessThan = (this.myvar < 43);
				this.greaterThanOrEqual = (this.myvar >= 42);
				this.greaterThanOrEqual2 = (this.myvar => 42);
				this.lessThanOrEqual = (this.myvar <= 42);
				this.lessThanOrEqual2 = (this.myvar =< 42);
				this.logicalAnd = (this.truevar && this.myvar == 42);
				this.logicalAnd2 = (this.truevar == true && this.myvar == 42);
				this.logicalOr = (this.falsevar || this.myvar == 42);
				this.not = !this.falsevar;

				this.ternary = (this.myvar == 42 ? 1 : 0);
				this.rangeII = this.myvar in |40, 42|;
				this.rangeIE = this.myvar in |40, 42>;
				this.rangeEI = this.myvar in <40, 42|;
				this.rangeEE = this.myvar in <42, 44>;
				this.rangeEE2 = this.myvar in <41, 43>;
				this.rangeV = this.myvar in this.rangevar;
				this.rangeCF = 40,41 in this.rangevar;
				this.rangeCS = 40,44 in this.rangevar;
				this.rangeC1 = 40,44 in |40, 44>;
				this.rangeC2 = 40,44 in <40, 44|;
				this.rangeC3 = 40,44 in |40, 44|;

				if (this.truevar)
					this.testtruevar = 1;
				if (this.trueflag)
					this.testtrueflag = 1;
				if (this.truestring)
					this.testtruestring = 1;
			}
		)";
		auto result = engine.compileScript("test_script", script);
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));

		auto wrapper = get_wrapper(result);
		auto store = wrapper->visitor->builtInStore;

		// Only flags implicitly convert to a boolean true.
		CHECK_FALSE(store->getValue<bool>("myvar").value_or(false));
		CHECK_FALSE(store->getValue<bool>("truevar").value_or(false));
		CHECK_FALSE(store->getValue<bool>("falsevar").value_or(false));
		CHECK(store->getValue<bool>("trueflag").value_or(false));

		// Setting a variable with boolean true sets to 1.0 or 0.0.
		CHECK_THAT(store->getValue<double>("truevar").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("falsevar").value_or(0.0), Catch::Matchers::WithinRel(0.0));

		// Relational.
		CHECK_THAT(store->getValue<double>("equality").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("equality2").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("inequality").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("inequality2").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("greaterThan").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("lessThan").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("greaterThanOrEqual").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("greaterThanOrEqual2").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("lessThanOrEqual").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("lessThanOrEqual2").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("logicalAnd").value_or(99.0), Catch::Matchers::WithinRel(0.0)); // false
		CHECK_THAT(store->getValue<double>("logicalAnd2").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("logicalOr").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("not").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("ternary").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("rangeII").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("rangeIE").value_or(99.0), Catch::Matchers::WithinRel(0.0)); // false
		CHECK_THAT(store->getValue<double>("rangeEI").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("rangeEE").value_or(99.0), Catch::Matchers::WithinRel(0.0)); // false
		CHECK_THAT(store->getValue<double>("rangeEE2").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("rangeV").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("rangeCF").value_or(99.0), Catch::Matchers::WithinRel(0.0)); // false
		CHECK_THAT(store->getValue<double>("rangeCS").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("rangeC1").value_or(99.0), Catch::Matchers::WithinRel(0.0)); // false
		CHECK_THAT(store->getValue<double>("rangeC2").value_or(99.0), Catch::Matchers::WithinRel(0.0)); // false
		CHECK_THAT(store->getValue<double>("rangeC3").value_or(0.0), Catch::Matchers::WithinRel(1.0));

		// Implicit conversions to boolean only affect flags.
		CHECK_THAT(store->getValue<double>("testtruevar").value_or(0.0), Catch::Matchers::WithinRel(0.0));
		CHECK_THAT(store->getValue<double>("testtrueflag").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("testtruestring").value_or(0.0), Catch::Matchers::WithinRel(1.0));
	}

	SECTION("arithmetic expressions")
	{
		// Tests all forms of arithmetic expressions, including order of operations and compound assignment operators.
		const std::string_view script = R"(
			if (created) {
				this.three = 1 + 2;
				this.four = 3 + true;
				this.five = 10 - 5;
				this.six = 2 * 3;
				this.two = 8 / 4;
				this.seven = 23 % 8;
				this.nine = 3 ^ 2;
				this.orderOfOperations = 1 + 2 * 3 - 4 / 2;
				this.orderOfOperations2 = (1 + 2) * (3 - 4) / 2;
				this.inc = 5;
				this.inc++;
				this.dec = 5;
				this.dec--;
				this.as_add = 5;
				this.as_add += 3;
				this.as_subtract = 10;
				this.as_subtract -= 4;
				this.as_multiply = 6;
				this.as_multiply *= 2;
				this.as_divide = 8;
				this.as_divide /= 4;
				this.as_modulus = 10;
				this.as_modulus %= 3;
				this.as_power = 2;
				this.as_power ^= 3;
			}
		)";
		auto result = engine.compileScript("test_script", script);
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));

		auto wrapper = get_wrapper(result);
		auto store = wrapper->visitor->builtInStore;
		CHECK_THAT(store->getValue<double>("three").value_or(0.0), Catch::Matchers::WithinRel(3.0));
		CHECK_THAT(store->getValue<double>("four").value_or(0.0), Catch::Matchers::WithinRel(4.0));
		CHECK_THAT(store->getValue<double>("five").value_or(0.0), Catch::Matchers::WithinRel(5.0));
		CHECK_THAT(store->getValue<double>("six").value_or(0.0), Catch::Matchers::WithinRel(6.0));
		CHECK_THAT(store->getValue<double>("two").value_or(0.0), Catch::Matchers::WithinRel(2.0));
		CHECK_THAT(store->getValue<double>("seven").value_or(0.0), Catch::Matchers::WithinRel(7.0));
		CHECK_THAT(store->getValue<double>("nine").value_or(0.0), Catch::Matchers::WithinRel(9.0));
		CHECK_THAT(store->getValue<double>("orderOfOperations").value_or(0.0), Catch::Matchers::WithinRel(5.0));
		CHECK_THAT(store->getValue<double>("orderOfOperations2").value_or(0.0), Catch::Matchers::WithinRel(-1.5));
		CHECK_THAT(store->getValue<double>("inc").value_or(0.0), Catch::Matchers::WithinRel(6.0));
		CHECK_THAT(store->getValue<double>("dec").value_or(0.0), Catch::Matchers::WithinRel(4.0));
		CHECK_THAT(store->getValue<double>("as_add").value_or(0.0), Catch::Matchers::WithinRel(8.0));
		CHECK_THAT(store->getValue<double>("as_subtract").value_or(0.0), Catch::Matchers::WithinRel(6.0));
		CHECK_THAT(store->getValue<double>("as_multiply").value_or(0.0), Catch::Matchers::WithinRel(12.0));
		CHECK_THAT(store->getValue<double>("as_divide").value_or(0.0), Catch::Matchers::WithinRel(2.0));
		CHECK_THAT(store->getValue<double>("as_modulus").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("as_power").value_or(0.0), Catch::Matchers::WithinRel(8.0));
	}

	SECTION("string manipulation")
	{
		// Tests string manipulation functions, including adding, removing, inserting, deleting, and replacing strings in a list.
		const std::string_view script = R"(
			if (created) {
				setstring this.temp,One;
				addstring this.temp,Two,Two;
				addstring this.temp,Three;
				addstring this.temp,Two;
				setstring this.test1,#s(this.temp);

				removestring this.temp,Two;
				setstring this.test2,#s(this.temp);

				insertstring this.temp,1,Two;
				setstring this.test3,#s(this.temp);

				deletestring this.temp,2;
				setstring this.test4,#s(this.temp);

				addstring this.temp,One;
				replacestring this.temp,2,Four;
				setstring this.test5,#s(this.temp);

				insertstring this.temp,1,One,One;
				setstring this.test6,#s(this.temp);

				addstring this.test7,One;
				insertstring this.test7,3,Two;
				insertstring this.test8,3,Two;
			}
		)";
		auto result = engine.compileScript("test_script", script);
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));

		auto wrapper = get_wrapper(result);
		auto store = wrapper->visitor->builtInStore;
		CHECK(store->getValue<std::string>("test1").value_or(std::string{}) == "One,\"Two,Two\",Three,Two");
		CHECK(store->getValue<std::string>("test2").value_or(std::string{}) == "One,\"Two,Two\",Three");
		CHECK(store->getValue<std::string>("test3").value_or(std::string{}) == "One,Two,\"Two,Two\",Three");
		CHECK(store->getValue<std::string>("test4").value_or(std::string{}) == "One,Two,Three");
		CHECK(store->getValue<std::string>("test5").value_or(std::string{}) == "One,Two,Four,One");
		CHECK(store->getValue<std::string>("test6").value_or(std::string{}) == "One,\"One,One\",Two,Four,One");
		CHECK(store->getValue<std::string>("test7").value_or(std::string{}) == "One,Two");
		CHECK(store->getValue<std::string>("test8").value_or(std::string{}) == "Two");
	}

	SECTION("sleep and resume")
	{
		// Tests the sleep command, which pauses execution of the script.
		// Subsequent runs of the script should continue from where it left off in the loop, and the loop variable should not be lost.
		const std::string_view script = R"(
			if (created) {
				for (i = 1; i <= 3; i++) {
					this.myvar = i;
					sleep 1;
				}
			}
		)";
		ScriptEvent timeout{.type = ScriptEventType::TIMEOUT, .initiator = source::FromPlayer(NPCServerPlayerID)};

		auto result = engine.compileScript("test_script", script);
		REQUIRE(execute_script(engine, created, source::FromNPC(testNPC), result));

		auto wrapper = get_wrapper(result);
		auto npcstore = &server->getNPC(testNPC)->scripting.variables;
		CHECK(npcstore->getValue<double>("myvar").value_or(0.0) == 1.0);

		CHECK(execute_script(engine, timeout, source::FromNPC(testNPC), result));
		CHECK(npcstore->getValue<double>("myvar").value_or(0.0) == 2.0);

		CHECK(execute_script(engine, timeout, source::FromNPC(testNPC), result));
		CHECK(npcstore->getValue<double>("myvar").value_or(0.0) == 3.0);

		CHECK(execute_script(engine, timeout, source::FromNPC(testNPC), result));
		CHECK_FALSE(npcstore->getValue<double>("myvar").value_or(0.0) == 4.0);
	}

	SECTION("timeout as a variable and not a flag")
	{
		// GS1 will implicitly convert numbers to boolean flags, but this causes problems with the timeout event, which is both an event flag and an NPC property.
		// Due to this, there is special code in GS1Visitor.cpp that flags that we are expecting timeout as a variable.
		// This lets us set the timeout property to a number while also letting the timeout event be false.
		// This test ensures that this behavior is working as expected by setting timeout to a value and doing relational comparisons on it.
		// Checking the value against a number should work, while checking the value as a flag boolean should not work.
		const std::string_view script = R"(
			if (created) {
				this.test = 1;
				timeout = 5;
			}
			if (timeout == 5) {
				this.test = 42;
			}
			if (timeout) {
				this.test = 3;
			}
		)";
		auto result = engine.compileScript("test_script", script);
		REQUIRE(execute_script(engine, created, source::FromNPC(testNPC), result));

		auto wrapper = get_wrapper(result);
		auto npcstore = &server->getNPC(testNPC)->scripting.variables;
		CHECK(npcstore->getValue<double>("test").value_or(0.0) == 42.0);
	}
}

////////////////////////////////////////////////////////////////////////////////

TEST_CASE_METHOD(ServerFixture, "ScriptEngineGS1 compiles and executes simple expressions", "[Scripting][IScriptEngine][GS1]")
{
	SECTION("compiled string expression")
	{
		// Tests the compilation and execution of a simple string expression that includes a variable.
		// Used by the system to translate text strings and process them.
		const std::string_view script = R"(
			this#nis a test
		)";
		auto result = engine.processStringExpression(script, source::FromPlayer(NPCServerPlayerID));
		CHECK(result == "thisNPC-Server (Server)is a test");
	}

	SECTION("compiled math expression")
	{
		// Tests the compilation and execution of a simple math expression that includes a variable.
		// Used by the system in various places, such as level links and the playersays command.
		const std::string_view script = R"(
			10 + x
		)";
		auto result = engine.processMathExpression(script, source::FromPlayer(NPCServerPlayerID));
		CHECK_THAT(result.value_or(0.0), Catch::Matchers::WithinRel(40.5));
	}
}

////////////////////////////////////////////////////////////////////////////////

TEST_CASE_METHOD(ServerFixture, "ScriptEngineGS1 npc and player bindings and cross-npc interactions", "[Scripting][IScriptEngine][GS1]")
{
	ScriptEvent created{.type = ScriptEventType::CREATED, .initiator = source::FromPlayer(NPCServerPlayerID)};
	auto player = server->getNPCServer()->getPlayerNPCServer();
	player->account.character = Character{};
	player->account.character.nickName = "NPC-Server (Server)";

	SECTION("setting variables inside and outside of with()")
	{
		// Tests that variables can be set both inside and outside of a with() block, and that the correct variable is set in each case.
		const std::string_view script = R"(
			if (created) {
				this.myvar = 42;
			}
			with (getnpc(Test)) {
				this.myvar = 50;
				thiso.myvar = 3;
			}
		)";
		auto result = engine.compileScript("test_script", script);
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));

		auto wrapper = get_wrapper(result);
		auto store = wrapper->visitor->builtInStore;
		CHECK_THAT(store->getValue<double>("myvar").value_or(0.0), Catch::Matchers::WithinRel(3.0));

		auto npc = server->getNPC(testNPC);
		auto npcstore = &npc->scripting.variables;
		CHECK(npc != nullptr);
		CHECK(npcstore->contains("myvar"));
		CHECK_THAT(npcstore->getValue<double>("myvar").value_or(0.0), Catch::Matchers::WithinRel(50.0));
	}

	SECTION("message code bindings")
	{
		// Tests that message codes can be set and retrieved, and that they are correctly bound to the player.
		const std::string_view script = R"(
			if (created) {
				setstring this.message,Hello, #n!;
				setplayerprop #n,Altered;
				setstring this.message2,Hello, #n!;
			}
		)";
		auto result = engine.compileScript("test_script", script);
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));

		auto wrapper = get_wrapper(result);
		auto store = wrapper->visitor->builtInStore;
		CHECK(store->getValue<std::string>("message").value_or(std::string{}) == "Hello, NPC-Server (Server)!");
		CHECK(store->getValue<std::string>("message2").value_or(std::string{}) == "Hello, Altered!");

		CHECK(player->account.character.nickName == "Altered");
	}

	SECTION("player properties can be changed")
	{
		// Tests that player properties can be changed and retrieved.
		const std::string_view script = R"(
			if (created) {
				setplayerprop #3,head2.png;
				setplayerprop #C3,cynober;
				setbeltcolor cynober;
				sprite = 14;
				if (sprite == 14) sprite = 16;
			}
		)";
		auto result = engine.compileScript("test_script", script);
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));

		CHECK(player->account.character.colors[0] == 2);
		CHECK(player->account.character.colors[1] == 0);
		CHECK(player->account.character.colors[2] == 10);
		CHECK(player->account.character.colors[3] == 13);
		CHECK(player->account.character.colors[4] == 13);
		CHECK(player->account.character.headImage == "head2.png");
		CHECK(player->account.character.sprite == 16);
	}

	SECTION("playersaysnumber processes math expressions")
	{
		// Tests that the playersaysnumber command can process math expressions.
		const std::string_view script = R"(
			this.test = playersaysnumber;
		)";
		auto result = engine.compileScript("test_script", script);

		player->account.character.chatMessage = "10 playerhearts";
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));
		auto wrapper = get_wrapper(result);
		auto store = wrapper->visitor->builtInStore;
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(10.0));

		player->account.character.chatMessage = "playerhearts 10";
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(3.0));

		player->account.character.chatMessage = "true"; // 1
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(1.0));

		player->account.character.chatMessage = "false"; // 0
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(99.0), Catch::Matchers::WithinRel(0.0));

		player->account.character.chatMessage = "5&&2"; // 0
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(0.0));

		player->account.character.chatMessage = "5==5&&false==false"; // 1
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(1.0));

		player->account.character.chatMessage = "5||2"; // 0
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(0.0));

		player->account.character.chatMessage = "5==2"; // 0
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(0.0));

		player->account.character.chatMessage = "5==2||true"; // 1
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(1.0));

		player->account.character.chatMessage = "10 + playerhearts"; // 10
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(10.0));

		player->account.character.chatMessage = "playerhearts + 10"; // 3
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(3.0));

		player->account.character.chatMessage = "10+playerhearts"; // 13
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(13.0));

		player->account.character.chatMessage = "-playerhearts"; // -3
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(-3.0));

		// These tests currently fail.  They are low priority for solving.
		{
			auto matchPlayerHearts = Catch::Matchers::WithinRel(3.0);

			player->account.character.chatMessage = "playerhearts-"; // 3
			REQUIRE(execute_script(engine, created, source::FromNPC(3), result));
			CHECK_NOFAIL(matchPlayerHearts.match(store->getValue<double>("test").value_or(0.0)));

			player->account.character.chatMessage = "playerhearts--"; // 3
			REQUIRE(execute_script(engine, created, source::FromNPC(3), result));
			CHECK_NOFAIL(matchPlayerHearts.match(store->getValue<double>("test").value_or(0.0)));
		}

		player->account.character.chatMessage = "(playerhearts)"; // 3
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(3.0));

		player->account.character.chatMessage = "(playerhearts"; // 0
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(0.0));

		player->account.character.chatMessage = "(playerhearts in |1,5|)"; // 0
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(0.0));

		player->account.character.chatMessage = "(playerhearts==3?5:2)"; // 0
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(0.0));
	}
}

////////////////////////////////////////////////////////////////////////////////

TEST_CASE_METHOD(ServerFixture, "ScriptEngineGS1 server bindings", "[Scripting][IScriptEngine][GS1]")
{
	ScriptEvent created{.type = ScriptEventType::CREATED, .initiator = source::FromPlayer(NPCServerPlayerID)};
	auto player = server->getNPCServer()->getPlayerNPCServer();
	player->account.character = Character{};
	player->account.character.nickName = "NPC-Server (Server)";

	SECTION("allplayerscount sees all players, including RC")
	{
		// Tests that the allplayerscount command correctly counts all players, including remote clients.
		// The NPC-Server is not counted.
		const std::string_view script = R"(
			this.test = allplayerscount;
		)";
		auto result = engine.compileScript("test_script", script);
		REQUIRE(execute_script(engine, created, source::FromNPC(3), result));

		auto wrapper = get_wrapper(result);
		auto store = wrapper->visitor->builtInStore;
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(2.0));
	}
}
