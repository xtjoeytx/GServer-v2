#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include <catch2/catch_approx.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <CString.h>

#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/generator/TimeoutGenerator.h>

using namespace preagonal;
using namespace std::string_literals;
using namespace std::string_view_literals;

static std::ostream& operator<<(std::ostream& os, CString const& value)
{
	os << value.toString();
	return os;
}

TEST_CASE("GameValue stores and converts primitive values", "[Scripting][ScriptContainers][GameValue]")
{
	GameValue value{};

	SECTION("bool and double conversion")
	{
		value.set(true);
		REQUIRE(value.has<bool>());
		REQUIRE(value.getCopy<bool>().value() == true);
		REQUIRE(value.getCopy<double>().value() == 1.0);
		REQUIRE(static_cast<bool>(value) == true);

		value.set(0.0);
		REQUIRE_FALSE(value.getCopy<bool>().value_or(true) == false);
		REQUIRE(static_cast<bool>(value) == false);
	}

	SECTION("string storage")
	{
		value.set("hello"s);
		REQUIRE(value.has<std::string>());
		REQUIRE(value.getCopy<std::string>().value() == "hello");
		REQUIRE(value.testAsFlag());
	}

	SECTION("multi-value assignment and unassignment")
	{
		value.assign(true);
		value.assign(42.0);
		REQUIRE(value.has_many());

		value.unassign<bool>();
		REQUIRE_FALSE(value.has<bool>());
		REQUIRE(value.has<double>());
	}

	SECTION("single-value assignment overwrites previous values")
	{
		value.assign(true);
		value.assign(42.0);
		REQUIRE(value.has_many());

		value.set("new value"s);
		REQUIRE(value.has<std::string>());
		REQUIRE_FALSE(value.has<bool>());
		REQUIRE_FALSE(value.has<double>());
	}
}

TEST_CASE("GameValue supports flatten and array access", "[Scripting][ScriptContainers][GameValue]")
{
	SECTION("flatten on numeric arrays")
	{
		GameValue value{std::vector<double>{2.5, 3.5}};
		auto flattened = value.flatten(1);
		REQUIRE(flattened.getCopy<double>().value() == 3.5);

		auto fallbackFlattened = value.flatten(99);
		REQUIRE(fallbackFlattened.getCopy<double>().value() == 2.5);
	}

	SECTION("script object vectors exposed as first element")
	{
		std::vector<ScriptObject> objects{source::FromNPC(3), source::FromPlayer(7)};
		GameValue value{objects};
		REQUIRE(value.has<ScriptObject>());
		REQUIRE(value.getCopy<ScriptObject>().value() == source::FromNPC(3));
		REQUIRE(value.getCopy<std::vector<ScriptObject>>().value().size() == 2);
	}
}

TEST_CASE("GameVariable supports assign, set, indexing and custom getter/setter", "[Scripting][ScriptContainers][GameVariable]")
{
	SECTION("indexed access over numeric arrays")
	{
		GameVariable variable{.name = "arr", .value = GameValue{std::vector<double>{1.0, 2.0, 3.0}}};
		REQUIRE(variable.getCopy<double>(1).value() == 2.0);

		double replacement = 9.0;
		variable.set(replacement, 1);
		REQUIRE_FALSE(variable.has<double>());
		REQUIRE(variable.has<std::vector<double>>());
		REQUIRE(variable.getCopy<double>(1).value() == 9.0);
		REQUIRE(variable.getCopy<double>(99).value() == 1.0);
	}

	SECTION("registered getter/setter for bound value")
	{
		double backing = 12.0;
		GameVariable variable{.name = "bound"};
		variable.registerGetter<double>([&](std::optional<int64_t>) -> GameValueVariantForGetter
		{
			return std::ref(backing);
		});
		variable.registerSetter<double>([&](GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (auto val = std::get_if<std::reference_wrapper<double>>(&incoming); val != nullptr)
				backing = val->get();
		});

		REQUIRE(variable.has<double>());
		REQUIRE(variable.getCopy<double>().value() == 12.0);
		double incoming = 30.0;
		variable.assign(incoming);
		REQUIRE(backing == 30.0);
	}
}

TEST_CASE("GameVariable serialize and deserialize behavior", "[Scripting][ScriptContainers][GameVariable]")
{
	SECTION("templated deserialize")
	{
		auto numberVar = GameVariable::deserialize<double>("score", "42.5");
		REQUIRE(numberVar.name == "score");
		REQUIRE(numberVar.getCopy<double>().value() == 42.5);

		auto arrayVar = GameVariable::deserialize<std::vector<double>>("points", "1,2,3");
		REQUIRE(arrayVar.getCopy<std::vector<double>>().value().size() == 3);
	}

	SECTION("line deserialize")
	{
		auto flagOnly = GameVariable::deserialize("FLAG enabled");
		REQUIRE(flagOnly.has_value());
		REQUIRE(flagOnly->name == "enabled");
		REQUIRE(flagOnly->getCopy<bool>().value() == true);

		auto numericVar = GameVariable::deserialize("VAR speed=1.5");
		REQUIRE(numericVar.has_value());
		REQUIRE(numericVar->getCopy<double>().value() == 1.5);
	}

	SECTION("serialize and serializeModern")
	{
		GameVariable textVar{.name = "title", .value = GameValue{"Knight"s}};
		REQUIRE(textVar.serialize<std::string>() == "Knight");
		REQUIRE(textVar.serializeModern("title").value() == "title=Knight");

		GameVariable boolVar{.name = "canjump", .value = GameValue{true}};
		REQUIRE(boolVar.serializeModern("canjump").value() == "canjump");
	}
}

TEST_CASE("GameVariableStore add/get/remove/serialize operations", "[Scripting][ScriptContainers][GameVariableStore]")
{
	GameVariableStore store;
	store.add("flag", GameValue{true});
	store.add("score", GameValue{7.0});

	REQUIRE(store.contains("flag"));
	REQUIRE(store.getValue<bool>("flag").value());
	REQUIRE(store.getValue<double>("score").value() == 7.0);

	auto serialized = store.serialize("flag");
	REQUIRE_FALSE(serialized.empty());
	REQUIRE(serialized.front() == "FLAG flag");

	REQUIRE(store.remove("score"));
	REQUIRE_FALSE(store.contains("score"));
}

TEST_CASE("GameVariableStore temporary clearing and getOrAdd", "[Scripting][ScriptContainers][GameVariableStore]")
{
	GameVariableStore store;
	GameVariable temporary{.name = "tmp_count", .value = GameValue{1.0}, .lifetime = variables::Lifetime::TEMPORARY};
	GameVariable permanent{.name = "perm_count", .value = GameValue{2.0}, .lifetime = variables::Lifetime::PERMANENT};
	store.add(std::move(temporary));
	store.add(std::move(permanent));

	store.clearTemporary("tmp");
	REQUIRE_FALSE(store.contains("tmp_count"));
	REQUIRE(store.contains("perm_count"));

	auto created = store.getOrAdd("new_value").lock();
	REQUIRE(created != nullptr);
	REQUIRE(created->getCopy<double>().value() == 0.0);
}

TEST_CASE("ScriptEventQueue queue visibility and duplicate detection", "[Scripting][ScriptContainers][ScriptEventQueue]")
{
	ScriptEventQueue queue;
	auto initiator = source::FromNPC(123);
	queue.queue().push_back(ScriptEvent{.type = ScriptEventType::TIMEOUT, .initiator = initiator, .args = {}});

	REQUIRE(queue.hasEvent(ScriptEventType::TIMEOUT, initiator));
	REQUIRE_FALSE(queue.hasEvent(ScriptEventType::CREATED, initiator));

	queue.addEvent(ScriptEventType::CREATED, initiator);
	queue.addEvent(ScriptEventType::CREATED, initiator, 1.0, "abc"s);
	REQUIRE(queue.queue().size() == 1);
}

TEST_CASE("ScriptContainer exposes event and variable stores", "[Scripting][ScriptContainers][ScriptContainer]")
{
	ScriptContainer container;
	container.variables.add("hp", GameValue{10.0});
	container.events.queue().push_back(ScriptEvent{.type = ScriptEventType::CUSTOM, .initiator = source::FromServer(), .args = {}});

	REQUIRE(container.variables.contains("hp"));
	REQUIRE(container.events.queue().size() == 1);
}

TEST_CASE("IntegralProperty converts to and from doubles", "[Scripting][ScriptContainers][Bind]")
{
	int32_t hp = 5;
	bind::IntegralProperty<int32_t> binder{.name = "hp", .value = std::ref(hp)};

	auto getter = binder.get();
	auto getterResult = getter(std::nullopt);
	REQUIRE(std::get<double>(getterResult) == 5.0);

	auto setter = binder.set();
	double incomingValue = 17.0;
	GameValueVariantForSetter incoming = std::ref(incomingValue);
	setter(incoming, std::nullopt);
	REQUIRE(hp == 17);
}

TEST_CASE("DivideByIntegralProperty applies factor in both directions", "[Scripting][ScriptContainers][Bind]")
{
	int32_t pixels = 32;
	bind::DivideByIntegralProperty<int32_t> binder{.name = "tiles", .value = std::ref(pixels), .factor = 16.0};

	auto getter = binder.get();
	REQUIRE(std::get<double>(getter(std::nullopt)) == 2.0);

	auto setter = binder.set();
	double incoming = 3.0;
	GameValueVariantForSetter value = std::ref(incoming);
	setter(value, std::nullopt);
	REQUIRE(pixels == 48);
}

TEST_CASE("TimeoutProperty supports duration and generator types", "[Scripting][ScriptContainers][Bind]")
{
	SECTION("milliseconds mapping")
	{
		std::chrono::milliseconds duration{2500};
		bind::TimeoutProperty<std::chrono::milliseconds> binder{.name = "timeout", .value = std::ref(duration)};
		auto getter = binder.get();
		REQUIRE(std::get<double>(getter(std::nullopt)) == Catch::Approx(2.5));

		auto setter = binder.set();
		double incomingSeconds = 1.25;
		GameValueVariantForSetter incoming = std::ref(incomingSeconds);
		setter(incoming, std::nullopt);
		REQUIRE(duration.count() == 1250);
	}

	SECTION("TimeoutGenerator mapping")
	{
		TimeoutGenerator generator;
		generator.startFor(std::chrono::milliseconds(2000));
		bind::TimeoutProperty<TimeoutGenerator> binder{.name = "timeout", .value = std::ref(generator)};

		auto setter = binder.set();
		double incomingSeconds = 0.75;
		GameValueVariantForSetter incoming = std::ref(incomingSeconds);
		setter(incoming, std::nullopt);

		REQUIRE(generator.isRunning());
		REQUIRE(generator.timeout.count() == 750);
	}
}

TEST_CASE("IntegralArrayProperty reads arrays and supports guarded writes", "[Scripting][ScriptContainers][Bind]")
{
	std::array<std::optional<clock::time_point>, 20> modTimes{};
	std::array<uint8_t, 10> values{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	bind::IntegralArrayProperty binder{.name = "save", .modTime = std::ref(modTimes), .modTimeIndex0 = 0, .value = std::ref(values)};

	auto getter = binder.get();
	std::vector<double> allValues = std::get<std::vector<double>>(getter(std::nullopt));
	REQUIRE(allValues.size() == 10);
	REQUIRE(allValues[0] == 1.0);
	REQUIRE(allValues[9] == 10.0);

	auto setter = binder.set();
	double ignored = 99.0;
	GameValueVariantForSetter incoming = std::ref(ignored);
	setter(incoming, 99);
	REQUIRE(values[0] == 1);
	REQUIRE(values[9] == 10);
}

TEST_CASE("ManuallyDefinedProperty returns provided getter and setter", "[Scripting][ScriptContainers][Bind]")
{
	// clang-format off
	double backing = 8.0;
	bind::ManuallyDefinedProperty<double> binder{
		.name = "manual",
		.getter = [&](std::optional<int64_t>) -> GameValueVariantForGetter
		{
			return backing;
		},
		.setter = [&](GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (auto val = std::get_if<std::reference_wrapper<double>>(&incoming); val != nullptr)
				backing = val->get();
		}
	};
	// clang-format on

	auto getter = binder.get();
	REQUIRE(std::get<double>(getter(std::nullopt)) == 8.0);

	auto setter = binder.set();
	double incoming = 13.0;
	GameValueVariantForSetter incomingVariant = std::ref(incoming);
	setter(incomingVariant, std::nullopt);
	REQUIRE(backing == 13.0);
}

namespace
{
struct ScriptParamSource
{
	static inline string_map<GameVariable> scriptParameters{};
};

struct ConstructibleScriptParamSource
{
	static inline string_map<GameVariable> scriptParameters{};

	void constructScriptParameters()
	{
		scriptParameters.try_emplace("constructed", GameVariable{.name = "constructed", .value = GameValue{5.0}});
	}
};
} // namespace

TEST_CASE("Script parameter lookup and property binding helper functions", "[Scripting][ScriptContainers][Bind]")
{
	SECTION("getScriptParameter basic lookup")
	{
		ScriptParamSource::scriptParameters.clear();
		ScriptParamSource::scriptParameters.try_emplace("value", GameVariable{.name = "value", .value = GameValue{3.0}});
		ScriptParamSource source;
		auto* found = getScriptParameter(source, "value");
		REQUIRE(found != nullptr);
		REQUIRE(found->getCopy<double>().value() == 3.0);
	}

	SECTION("getScriptParameter triggers constructScriptParameters")
	{
		ConstructibleScriptParamSource::scriptParameters.clear();
		ConstructibleScriptParamSource source;
		auto* found = getScriptParameter(source, "constructed");
		REQUIRE(found != nullptr);
		REQUIRE(found->name == "constructed");
	}

	SECTION("bindPropertyAsReadOnly and bindPropertyAsReadWrite")
	{
		string_map<GameVariable> scriptParameters;
		int32_t hp = 22;
		bind::IntegralProperty<int32_t> readOnlyBinder{.name = "hp", .value = std::ref(hp)};
		bind::bindPropertyAsReadOnly(scriptParameters, std::move(readOnlyBinder));
		REQUIRE(scriptParameters.contains("hp"));
		REQUIRE(scriptParameters.at("hp").getters.contains(typeid(double).hash_code()));
		REQUIRE_FALSE(scriptParameters.at("hp").setters.contains(typeid(double).hash_code()));

		int32_t rupees = 9;
		bind::IntegralProperty<int32_t> readWriteBinder{.name = "rupees", .value = std::ref(rupees)};
		bind::bindPropertyAsReadWrite(scriptParameters, std::move(readWriteBinder));
		REQUIRE(scriptParameters.contains("rupees"));
		REQUIRE(scriptParameters.at("rupees").setters.contains(typeid(double).hash_code()));
	}
}

TEST_CASE("script_error preserves runtime_error message", "[Scripting][ScriptContainers][Exception]")
{
	try
	{
		throw script_error{"boom"};
	}
	catch (const script_error& error)
	{
		REQUIRE(std::string{error.what()} == "boom");
	}
}
