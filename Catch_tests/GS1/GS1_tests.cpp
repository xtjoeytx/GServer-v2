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
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptSystem.h>
#include <scripting/ScriptTypes.h>
#include <scripting/gs1/GS1Variables.h>
#include <scripting/gs1/GS1Visitor.h>
#include <scripting/gs1/ScriptEngineGS1.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/Log.h>
#include <utilities/manager/ITranslationManager.h>
#include <utilities/manager/TranslationManagerClassic.h>

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

		BabyDI_RELEASE(ITranslationManager);
		translationManager = BabyDI_PROVIDE(ITranslationManager, new TranslationManagerClassic());

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
		const auto client = std::make_shared<PlayerClient>(new CSocket(), server->getPlayerIdGenerator().getAvailableId());
		const auto rc = std::make_shared<PlayerRC>(new CSocket(), server->getPlayerIdGenerator().getAvailableId());
		server->addPlayer(client, client->getId());
		server->addPlayer(rc, rc->getId());
		npcServer->playerLogin(client);
		npcServer->playerLogin(rc);
	}

	NPCID testNPC = NPCID_GEN_DATABASE_LOCALN;
	Server* server = nullptr;
	ITranslationManager* translationManager = nullptr;
	std::shared_ptr<NPCServer> npcServer;
	std::shared_ptr<gs1::ScriptEngineGS1> engine;
};

////////////////////////////////////////////////////////////////////////////////

static gs1::GS1ScriptWrapper* get_wrapper(const CompiledScriptResult& result, const std::source_location location = std::source_location::current())
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

static bool execute_script(IScriptEngine& engine, ScriptEvent& event, const ScriptObject& source, CompiledScriptResult& result, const std::source_location location = std::source_location::current())
{
	CAPTURE(location.line());
	CAPTURE(location.function_name());

	REQUIRE(std::holds_alternative<ScriptExecutionContext>(result));

	// Force normal lifetime so we don't lose our script results.
	auto wrapper = get_wrapper(result);
	REQUIRE(wrapper != nullptr);
	wrapper->variables.defaultLifetime = variables::Lifetime::NORMAL;

	auto& context = std::get<ScriptExecutionContext>(result);
	const auto contextPtr = std::shared_ptr<ScriptExecutionContext>(&context, [](ScriptExecutionContext*) {});

	return engine.execute(event, source, contextPtr);
}

////////////////////////////////////////////////////////////////////////////////

TEST_CASE_METHOD(ServerFixture, "ScriptEngineGS1 compiles scripts", "[Scripting][IScriptEngine][GS1]")
{
	SECTION("valid script")
	{
		// Checks a script that should compile successfully.
		// Check also includes sloppy code that should be accepted by the parser, such as missing semi-colons and commands that don't need line breaks.
		constexpr std::string_view validScript = R"(
			// NPC made by
			if (created || playerenters) {
				setimg light2.png;
				dontblock;
			}
			if (created) {x=10}
			if (created) {y=5;say2HEY!}
		)";
		auto result = engine->compileScript("valid_script", validScript);
		REQUIRE(std::holds_alternative<ScriptExecutionContext>(result));
	}

	SECTION("invalid script")
	{
		// Bad script that should fail to compile.
		constexpr std::string_view invalidScript = R"(
			if (created || playerenters) {
				setanimg light2.png;
				doblock;
		)";
		auto result = engine->compileScript("invalid_script", invalidScript);
		REQUIRE(std::holds_alternative<std::string>(result));
	}

	SECTION("compiled script identifiers are found")
	{
		// Checks if every single identifier is found and registered during compilation.
		constexpr std::string_view script = R"(
			if (created || playerenters) {
				this.origx = x;
				this.origy = y;
				setstring mystring,Hello, world!;
			}
		)";
		auto result = engine->compileScript("script_with_identifiers", script);
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
		constexpr std::string_view script = R"(
			function testFunc() {}
		)";
		auto result = engine->compileScript("script_with_functions", script);
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
		constexpr std::string_view script = R"(
			if (created) {
				this.myvar = 42;
				this.myvar2 := 42;
			}
		)";
		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));

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
		constexpr std::string_view script = R"(
			if (created) {
				setstring this.mystring,Hello, world!;
			}
		)";
		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));

		auto wrapper = get_wrapper(result);
		auto store = wrapper->visitor->builtInStore;
		CHECK(store->contains("mystring"));
		CHECK_THAT(store->getValue<std::string>("mystring").value_or(std::string{}), Catch::Matchers::Equals("Hello, world!"s));
	}

	SECTION("array assignment and retrieval")
	{
		// Checks if arrays can be assigned and retrieved, including direct assignment and negative indexing.
		constexpr std::string_view script = R"(
			if (created) {
				setarray this.myarray,5;
				this.myarray[3] = 7;
				this.direct = { 1, 2, 3 };
				this.negativeIndex = { 42 };
				this.negativeIndex[-1] = 99;
			}
		)";
		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));

		auto wrapper = get_wrapper(result);
		auto store = wrapper->visitor->builtInStore;
		CHECK(store->contains("myarray"));
		CHECK(store->contains("direct"));
		CHECK(store->contains("negativeIndex"));

		auto myarray = store->get("myarray").lock();
		REQUIRE(myarray != nullptr);
		auto direct = store->get("direct").lock();
		REQUIRE(direct != nullptr);
		auto negativeIndex = store->get("negativeIndex").lock();
		REQUIRE(negativeIndex != nullptr);

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
		constexpr std::string_view script = R"(
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
		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));

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
		constexpr std::string_view script = R"(
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
		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));

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
		constexpr std::string_view script = R"(
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
		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));

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

	SECTION("string operations")
	{
		// Tests additional string functions.
		constexpr std::string_view script = R"(
			setstring s, thisisatest;
			test1 = indexof(isa, #s(s));
		)";
		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));

		auto wrapper = get_wrapper(result);
		auto store = wrapper->visitor->builtInStore;
		CHECK_THAT(store->getValue<double>("test1").value_or(0.0), Catch::Matchers::WithinRel(4.0));
	}

	SECTION("tokenized text")
	{
		// Tests tokenize and tokenize2.
		constexpr std::string_view script = R"(
			setstring test,This, is "A, test" string;

			// First test.
			tokenize #s(test);
			tokens1count = tokenscount;
			for (i=0; i<tokenscount; i++)
				setstring tokens1_t#v(i),#t(i);

			// Second test.
			tokenize2 i,#s(test);
			tokens2count = tokenscount;
			for (i=0; i<tokenscount; i++)
				setstring tokens2_t#v(i),#t(i);
		)";
		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));

		const auto player = npcServer->getPlayerNPCServer();
		REQUIRE(player != nullptr);

		auto wrapper = get_wrapper(result);
		auto store = wrapper->visitor->builtInStore;
		auto playerstore = &player->account.variables;
		CHECK(playerstore->getValue<std::string>("test").value_or(std::string{}) == "This, is \"A, test\" string");
		CHECK_THAT(store->getValue<double>("tokens1count").value_or(0.0), Catch::Matchers::WithinRel(4.0));
		CHECK(playerstore->getValue<std::string>("tokens1_t0").value_or(std::string{}) == "This");
		CHECK(playerstore->getValue<std::string>("tokens1_t1").value_or(std::string{}) == "is");
		CHECK(playerstore->getValue<std::string>("tokens1_t2").value_or(std::string{}) == "A, test");
		CHECK(playerstore->getValue<std::string>("tokens1_t3").value_or(std::string{}) == "string");

		CHECK_THAT(store->getValue<double>("tokens2count").value_or(0.0), Catch::Matchers::WithinRel(6.0));
		CHECK(playerstore->getValue<std::string>("tokens2_t0").value_or(std::string{}) == "Th");
		CHECK(playerstore->getValue<std::string>("tokens2_t1").value_or(std::string{}) == "s");
		CHECK(playerstore->getValue<std::string>("tokens2_t2").value_or(std::string{}) == "s");
		CHECK(playerstore->getValue<std::string>("tokens2_t3").value_or(std::string{}) == "A, test");
		CHECK(playerstore->getValue<std::string>("tokens2_t4").value_or(std::string{}) == "str");
		CHECK(playerstore->getValue<std::string>("tokens2_t5").value_or(std::string{}) == "ng");
	}

	SECTION("sleep and resume")
	{
		// Tests the sleep command, which pauses execution of the script.
		// Subsequent runs of the script should continue from where it left off in the loop, and the loop variable should not be lost.
		constexpr std::string_view script = R"(
			if (created) {
				for (i = 1; i <= 3; i++) {
					this.myvar = i;
					sleep 1;
				}
			}
		)";
		ScriptEvent timeout{.type = ScriptEventType::TIMEOUT, .initiator = source::FromPlayer(NPCServerPlayerID)};

		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(testNPC), result));

		[[maybe_unused]] auto wrapper = get_wrapper(result);
		auto npcstore = &server->getNPC(testNPC)->scripting.variables;
		CHECK_THAT(npcstore->getValue<double>("myvar").value_or(0.0), Catch::Matchers::WithinRel(1.0));

		CHECK(execute_script(*engine, timeout, source::FromNPC(testNPC), result));
		CHECK_THAT(npcstore->getValue<double>("myvar").value_or(0.0), Catch::Matchers::WithinRel(2.0));

		CHECK(execute_script(*engine, timeout, source::FromNPC(testNPC), result));
		CHECK_THAT(npcstore->getValue<double>("myvar").value_or(0.0), Catch::Matchers::WithinRel(3.0));

		CHECK(execute_script(*engine, timeout, source::FromNPC(testNPC), result));
		auto match = Catch::Matchers::WithinRel(4.0);
		CHECK_FALSE(match.match(npcstore->getValue<double>("myvar").value_or(0.0)));
	}

	SECTION("timeout as a variable and not a flag")
	{
		// GS1 will implicitly convert numbers to boolean flags, but this causes problems with the timeout event, which is both an event flag and an NPC property.
		// Due to this, there is special code in GS1Visitor.cpp that flags that we are expecting timeout as a variable.
		// This lets us set the timeout property to a number while also letting the timeout event be false.
		// This test ensures that this behavior is working as expected by setting timeout to a value and doing relational comparisons on it.
		// Checking the value against a number should work, while checking the value as a flag boolean should not work.
		constexpr std::string_view script = R"(
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
		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(testNPC), result));

		[[maybe_unused]] auto wrapper = get_wrapper(result);
		auto npcstore = &server->getNPC(testNPC)->scripting.variables;
		CHECK_THAT(npcstore->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(42.0));
	}

	SECTION("reserved constants cannot be used as variables")
	{
		constexpr std::string_view script = R"(
            pi = 3.14;
		)";
		auto result = engine->compileScript("test_script", script);
		REQUIRE_THROWS_AS(execute_script(*engine, created, source::FromNPC(testNPC), result), script_error);
	}

	SECTION("reserved constants allowed in scoped variables")
	{
		constexpr std::string_view script = R"(
            this.pi = 3.14;
		)";
		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(testNPC), result));

		[[maybe_unused]] auto wrapper = get_wrapper(result);
		auto npcstore = &server->getNPC(testNPC)->scripting.variables;
		CHECK_THAT(npcstore->getValue<double>("pi").value_or(0.0), Catch::Matchers::WithinRel(3.14));
	}

	SECTION("loop statements")
	{
		constexpr std::string_view script = R"(
			while (i < 10) {
				i++;
				if (i == 5) break;
			}
			for (j = 0; j < 6; j++) {
				if (j == 4) continue;
				q += j;
			}
			testFunc();

			function testFunc() {
				k = 3;
				return;
				k = 5;
			}
		)";
		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(testNPC), result));

		[[maybe_unused]] auto wrapper = get_wrapper(result);
		auto store = wrapper->visitor->builtInStore;
		CHECK_THAT(store->getValue<double>("i").value_or(0.0), Catch::Matchers::WithinRel(5.0));
		CHECK_THAT(store->getValue<double>("q").value_or(0.0), Catch::Matchers::WithinRel(11.0));
		CHECK_THAT(store->getValue<double>("k").value_or(0.0), Catch::Matchers::WithinRel(3.0));
	}
}

////////////////////////////////////////////////////////////////////////////////

TEST_CASE_METHOD(ServerFixture, "ScriptEngineGS1 compiles and executes simple expressions", "[Scripting][IScriptEngine][GS1]")
{
	SECTION("compiled string expression")
	{
		// Tests the compilation and execution of a simple string expression that includes a variable.
		// Used by the system to translate text strings and process them.
		constexpr std::string_view script = R"(
			this#nis a test
		)";
		auto result = engine->processStringExpression(script, source::FromPlayer(NPCServerPlayerID));
		CHECK(result == "thisNPC-Server (Server)is a test");
	}

	SECTION("compiled math expression")
	{
		// Tests the compilation and execution of a simple math expression that includes a variable.
		// Used by the system in various places, such as level links and the playersays command.
		constexpr std::string_view script = R"(
			10 + x
		)";
		const auto result = engine->processMathExpression(script, source::FromPlayer(NPCServerPlayerID));
		CHECK_THAT(result.value_or(0.0), Catch::Matchers::WithinRel(40.5));
	}
}

////////////////////////////////////////////////////////////////////////////////

TEST_CASE_METHOD(ServerFixture, "ScriptEngineGS1 npc and player bindings and cross-npc interactions", "[Scripting][IScriptEngine][GS1]")
{
	ScriptEvent created{.type = ScriptEventType::CREATED, .initiator = source::FromPlayer(NPCServerPlayerID)};
	const auto player = npcServer->getPlayerNPCServer();
	player->account.character = Character{};
	player->account.character.nickName = "NPC-Server (Server)";

	SECTION("variables and flags save to the correct object")
	{
		// Tests that variables and flags are saved to the correct object, whether it be the NPC, the player, or the script's built-in store.
		constexpr std::string_view script = R"(
			if (created) {
				contextVar = 42;

				this.npcVar = 33;
				setstring this.npcFlag,World!;

				setstring playerFlag,Hello!;
				setstring client.playerFlag,World!;
				set clientr.playerFlag;

				set server.Test;
				setstring serverr.Test2,Hello!;

				setstring level.test,LevelHello!;
			}
		)";
		auto npc = server->getNPC(testNPC);

		auto level = std::make_shared<Level>();
		level->levelName = "TestLevel";
		npc->setLevel(level);

		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(testNPC), result));

		auto wrapper = get_wrapper(result);
		auto scriptStore = wrapper->visitor->builtInStore;
		CHECK_THAT(scriptStore->getValue<double>("contextVar").value_or(0.0), Catch::Matchers::WithinRel(42.0));

		auto npcstore = &npc->scripting.variables;
		CHECK_THAT(npcstore->getValue<double>("npcVar").value_or(0.0), Catch::Matchers::WithinRel(33.0));
		CHECK_THAT(npcstore->getValue<std::string>("npcFlag").value_or(std::string{}), Catch::Matchers::Equals("World!"));

		auto playerstore = &player->account.variables;
		CHECK_THAT(playerstore->getValue<std::string>("playerFlag").value_or(std::string{}), Catch::Matchers::Equals("Hello!"));
		CHECK_THAT(playerstore->getValue<std::string>("client.playerFlag").value_or(std::string{}), Catch::Matchers::Equals("World!"));
		CHECK(playerstore->getValue<bool>("clientr.playerFlag").value_or(false) == true);

		auto serverstore = &server->Scripting.variables;
		CHECK(serverstore->getValue<bool>("server.Test").value_or(false) == true);
		CHECK_THAT(serverstore->getValue<std::string>("serverr.Test2").value_or(std::string{}), Catch::Matchers::Equals("Hello!"));

		auto levelstore = &level->scripting.variables;
		CHECK_THAT(levelstore->getValue<std::string>("test").value_or(std::string{}), Catch::Matchers::Equals("LevelHello!"));

		npc->setLevel(nullptr);
	}

	SECTION("variables respect conformance modes")
	{
		// Tests that variables are set and retrieved correctly in both conformance modes.
		constexpr std::string_view script = R"(
			if (created) { client.myvar = 3; }
		)";

		auto matchTest = Catch::Matchers::WithinRel(3.0);

		// Test conforming mode.
		engine->settings.set("strict", true);
		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));

		auto wrapper = get_wrapper(result);
		auto store = wrapper->visitor->builtInStore;
		auto playerstore = &player->account.variables;
		CHECK(store->contains("client.myvar"));
		CHECK_FALSE(matchTest.match(playerstore->getValue<double>("client.myvar").value_or(0.0)));

		// Test non-conforming mode.
		store->store.clear();
		engine->settings.set("strict", false);
		result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));

		wrapper = get_wrapper(result);
		store = wrapper->visitor->builtInStore;
		CHECK_FALSE(store->contains("client.myvar"));
		CHECK(matchTest.match(playerstore->getValue<double>("client.myvar").value_or(0.0)));
	}

	SECTION("setting variables inside and outside of with()")
	{
		// Tests that variables can be set both inside and outside of a with() block, and that the correct variable is set in each case.
		constexpr std::string_view script = R"(
			if (created) {
				this.myvar = 42;
			}
			with (getnpc(Test)) {
				this.myvar = 50;
				thiso.myvar = 3;
			}
		)";
		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));

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
		constexpr std::string_view script = R"(
			if (created) {
				setstring this.message,Hello, #n!;
				setplayerprop #n,Altered;
				setstring this.message2,Hello, #n!;
			}
		)";
		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));

		auto wrapper = get_wrapper(result);
		auto store = wrapper->visitor->builtInStore;
		CHECK(store->getValue<std::string>("message").value_or(std::string{}) == "Hello, NPC-Server (Server)!");
		CHECK(store->getValue<std::string>("message2").value_or(std::string{}) == "Hello, Altered!");

		CHECK(player->account.character.nickName == "Altered");
	}

	SECTION("player properties can be changed")
	{
		// Tests that player properties can be changed and retrieved.
		constexpr std::string_view script = R"(
			if (created) {
				setplayerprop #3,head2.png;
				setplayerprop #C3,cynober;
				setbeltcolor cynober;
				sprite = 14;
				if (sprite == 14) sprite = 16;
			}
		)";
		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));

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
		constexpr std::string_view script = R"(
			this.test = playersaysnumber;
		)";
		auto result = engine->compileScript("test_script", script);

		player->account.character.chatMessage = "10 playerhearts";
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));

		auto wrapper = get_wrapper(result);
		auto store = wrapper->visitor->builtInStore;
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(10.0));

		player->account.character.chatMessage = "playerhearts 10";
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(3.0));

		player->account.character.chatMessage = "true"; // 1
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(1.0));

		player->account.character.chatMessage = "false"; // 0
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(99.0), Catch::Matchers::WithinRel(0.0));

		player->account.character.chatMessage = "5&&2"; // 0
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(0.0));

		player->account.character.chatMessage = "5==5&&false==false"; // 1
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(1.0));

		player->account.character.chatMessage = "5||2"; // 0
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(0.0));

		player->account.character.chatMessage = "5==2"; // 0
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(0.0));

		player->account.character.chatMessage = "5==2||true"; // 1
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(1.0));

		player->account.character.chatMessage = "10 + playerhearts"; // 10
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(10.0));

		player->account.character.chatMessage = "playerhearts + 10"; // 3
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(3.0));

		player->account.character.chatMessage = "10+playerhearts"; // 13
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(13.0));

		player->account.character.chatMessage = "-playerhearts"; // -3
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(-3.0));

		// These tests currently fail.  They are low priority for solving.
		{
			auto matchPlayerHearts = Catch::Matchers::WithinRel(3.0);

			player->account.character.chatMessage = "playerhearts-"; // 3
			REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));
			CHECK_NOFAIL(matchPlayerHearts.match(store->getValue<double>("test").value_or(0.0)));

			player->account.character.chatMessage = "playerhearts--"; // 3
			REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));
			CHECK_NOFAIL(matchPlayerHearts.match(store->getValue<double>("test").value_or(0.0)));
		}

		player->account.character.chatMessage = "(playerhearts)"; // 3
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(3.0));

		player->account.character.chatMessage = "(playerhearts"; // 0
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(0.0));

		player->account.character.chatMessage = "(playerhearts in |1,5|)"; // 0
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(0.0));

		player->account.character.chatMessage = "(playerhearts==3?5:2)"; // 0
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(0.0));
	}
}

////////////////////////////////////////////////////////////////////////////////

TEST_CASE_METHOD(ServerFixture, "ScriptEngineGS1 server bindings", "[Scripting][IScriptEngine][GS1]")
{
	ScriptEvent created{.type = ScriptEventType::CREATED, .initiator = source::FromPlayer(NPCServerPlayerID)};
	const auto player = npcServer->getPlayerNPCServer();
	player->account.character = Character{};
	player->account.character.nickName = "NPC-Server (Server)";

	SECTION("allplayerscount sees all players, including RC")
	{
		// Tests that the allplayerscount command correctly counts all players, including remote clients.
		// The NPC-Server is not counted.
		constexpr std::string_view script = R"(
			this.test = allplayerscount;
		)";
		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));

		const auto wrapper = get_wrapper(result);
		const auto store = wrapper->visitor->builtInStore;
		CHECK_THAT(store->getValue<double>("test").value_or(0.0), Catch::Matchers::WithinRel(2.0));
	}
}

////////////////////////////////////////////////////////////////////////////////

TEST_CASE_METHOD(ServerFixture, "ScriptEngineGS1 functions", "[Scripting][IScriptEngine][GS1]")
{
	ScriptEvent created{.type = ScriptEventType::CREATED, .initiator = source::FromPlayer(NPCServerPlayerID)};
	const auto player = npcServer->getPlayerNPCServer();
	player->account.character = Character{};
	player->account.character.nickName = "NPC-Server (Server)";

	SECTION("passwordmatches() and #E()")
	{
		constexpr std::string_view script = R"(
			setstring this.passwordHash,#E(hunter2);
			this.testSuccess = passwordmatches(#s(this.passwordHash), hunter2);
			this.testFail = passwordmatches(#s(this.passwordHash), hunter3);
		)";
		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(3), result));

		auto wrapper = get_wrapper(result);
		auto store = wrapper->visitor->builtInStore;
		CHECK(store->getValue<std::string>("passwordHash").value_or(std::string{}) == "9S+9MrKzuG/4jvbEkGKChfSCrxXdyylUH5S89Saj9sc="s);
		CHECK_THAT(store->getValue<double>("testSuccess").value_or(0.0), Catch::Matchers::WithinRel(1.0));
		CHECK_THAT(store->getValue<double>("testFail").value_or(0.0), Catch::Matchers::WithinRel(0.0));
	}
}

////////////////////////////////////////////////////////////////////////////////

TEST_CASE_METHOD(ServerFixture, "ScriptEngineGS1 message codes", "[Scripting][IScriptEngine][GS1]")
{
	ScriptEvent created{.type = ScriptEventType::CREATED, .initiator = source::FromPlayer(NPCServerPlayerID)};
	const auto player = npcServer->getPlayerNPCServer();
	player->account.character = Character{};
	player->account.character.nickName = "NPC-Server (Server)";
	engine->settings.set("always-translate-strings", true);

	SECTION("#U - explicit translate")
	{
		constexpr std::string_view script = R"(
			setstring client.animal,cat;
			setstring client.animal.es,dog;
			setstring this.test1,#U(cat);
			setstring this.test2,#U(the animal is #s(client.animal));
			setstring this.test3,#U(the animal is #U(#s(client.animal)));
			setstring this.test4,#U(the animal is #U2(#s(client.animal)));
			setstring this.test5,#U(the animal is #U2(#U(#s(client.animal))));
			setstring this.test6,#U(the animal is #U2(#U2(#U(#s(client.animal)))));
		)";

		auto npc = server->getNPC(testNPC);
		auto npcstore = &npc->scripting.variables;
		auto playerstore = &player->account.variables;

		translationManager->addTranslation(language::originalLanguage, "cat", "cat");
		translationManager->addTranslation(language::originalLanguage, "dog", "dog");
		translationManager->addTranslation(language::originalLanguage, "#s(client.animal)", "#s(client.animal)");
		translationManager->addTranslation(language::originalLanguage, "the animal is #s(client.animal)", "the animal is #s(client.animal)");
		translationManager->addTranslation(language::originalLanguage, "the animal is #U(#s(client.animal))", "the animal is #U(#s(client.animal))");
		translationManager->addTranslation(language::originalLanguage, "the animal is #U2(#s(client.animal))", "the animal is #U2(#s(client.animal))");
		translationManager->addTranslation(language::originalLanguage, "the animal is #U2(#U(#s(client.animal)))", "the animal is #U2(#U(#s(client.animal)))");
		translationManager->addTranslation(language::originalLanguage, "the animal is #U2(#U2(#U(#s(client.animal))))", "the animal is #U2(#U2(#U(#s(client.animal))))");
		translationManager->addTranslation("spanish", "cat", "gato");
		translationManager->addTranslation("spanish", "dog", "perro");
		translationManager->addTranslation("spanish", "#s(client.animal)", "#s(client.animal.es)");
		translationManager->addTranslation("spanish", "the animal is #s(client.animal)", "#s(client.animal): el tipo de animal");
		translationManager->addTranslation("spanish", "the animal is #U(#s(client.animal))", "el animal es #U(#s(client.animal))");
		translationManager->addTranslation("spanish", "the animal is #U2(#s(client.animal))", "el animal es #U2(#s(client.animal))");
		translationManager->addTranslation("spanish", "the animal is #U2(#U(#s(client.animal)))", "el animal es #U2(#U(#s(client.animal)))");
		translationManager->addTranslation("spanish", "the animal is #U2(#U2(#U(#s(client.animal))))", "el animal es #U2(#U2(#U(#s(client.animal))))");
		engine->settings.set("always-translate-strings", false);

		// English.
		auto result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(testNPC), result));

		CHECK(playerstore->getValue<std::string>("client.animal").value_or(std::string{}) == "cat"s);
		CHECK(playerstore->getValue<std::string>("client.animal.es").value_or(std::string{}) == "dog"s);
		CHECK(npcstore->getValue<std::string>("test1").value_or(std::string{}) == "cat"s);
		CHECK(npcstore->getValue<std::string>("test2").value_or(std::string{}) == "the animal is cat"s);
		CHECK(npcstore->getValue<std::string>("test3").value_or(std::string{}) == "the animal is #s(client.animal)"s);
		CHECK(npcstore->getValue<std::string>("test4").value_or(std::string{}) == "the animal is cat"s);
		CHECK(npcstore->getValue<std::string>("test5").value_or(std::string{}) == "the animal is cat"s);
		CHECK(npcstore->getValue<std::string>("test6").value_or(std::string{}) == "the animal is cat"s);

		// Spanish.
		player->account.language = "spanish";
		result = engine->compileScript("test_script", script);
		REQUIRE(execute_script(*engine, created, source::FromNPC(testNPC), result));

		CHECK(playerstore->getValue<std::string>("client.animal").value_or(std::string{}) == "cat"s);
		CHECK(npcstore->getValue<std::string>("test1").value_or(std::string{}) == "gato"s);
		CHECK(npcstore->getValue<std::string>("test2").value_or(std::string{}) == "cat: el tipo de animal"s);
		CHECK(npcstore->getValue<std::string>("test3").value_or(std::string{}) == "el animal es #s(client.animal.es)"s);
		CHECK(npcstore->getValue<std::string>("test4").value_or(std::string{}) == "el animal es gato"s);
		CHECK(npcstore->getValue<std::string>("test5").value_or(std::string{}) == "el animal es dog"s);
		CHECK(npcstore->getValue<std::string>("test6").value_or(std::string{}) == "el animal es perro"s);
	}
}
