#include <any>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <ANTLRInputStream.h>
#include <CommonTokenStream.h>
#include <GS1Lexer.h>
#include <GS1Parser.h>

#include <BabyDI.h>
#include <Server.h>
#include <level/Level.h>
#include <object/Character.h>
#include <object/NPC.h>
#include <player/PlayerClient.h>
#include <scripting/gs1/GS1Flags.h>
#include <scripting/gs1/GS1Variables.h>
#include <scripting/gs1/GS1Visitor.h>
#include <scripting/gs1/ScriptEngineGS1.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptSystem.h>

using namespace preagonal::gs1::grammar;

////////////////////////////////////////////////////////////////////////////////
namespace preagonal::gs1
{
////////////////////////////////////////////////////////////////////////////////

PlayerPtr getPlayerFromSource(const ScriptObjectSource& source, std::optional<size_t> index)
{
	if (source.second != ScriptObjectSourceType::PLAYER)
		return nullptr;

	auto* server = BabyDI::Get<Server>();
	if (auto player = server->getPlayer(source.first); player != nullptr)
	{
		if (index.has_value())
		{
			if (auto level = server->getLevel(player->account.level); level != nullptr && index.value() < level->getPlayers().size())
				player = server->getPlayer(level->getPlayers().at(index.value()));
		}
		return player;
	}

	return nullptr;
}

PlayerClientPtr getPlayerClientFromSource(const ScriptObjectSource& source, std::optional<size_t> index)
{
	auto player = getPlayerFromSource(source, index);
	if (auto client = std::dynamic_pointer_cast<PlayerClient>(player); client != nullptr)
		return client;
	return nullptr;
}

NPCPtr getNPCFromSource(const ScriptObjectSource& source, std::optional<size_t> index)
{
	if (source.second != ScriptObjectSourceType::NPC)
		return nullptr;
	auto* server = BabyDI::Get<Server>();
	if (auto npc = server->getNPC(source.first); npc != nullptr)
	{
		if (index.has_value())
		{
			if (auto level = npc->level.lock(); level != nullptr && index.value() < level->getNPCs().size())
				npc = server->getNPC(level->getNPCs().at(index.value()));
		}
		return npc;
	}
	return nullptr;
}

PlayerOrNPC getPlayerOrNPCFromSource(const ScriptObjectSource& source, std::optional<size_t> index)
{
	if (source.second == ScriptObjectSourceType::SERVER)
		return std::nullopt;

	auto* server = BabyDI::Get<Server>();
	if (source.second == ScriptObjectSourceType::PLAYER)
		return getPlayerFromSource(source, index);
	else if (source.second == ScriptObjectSourceType::NPC)
		return getNPCFromSource(source, index);

	return std::nullopt;
}

Character* getCharacterFromSource(const ScriptObjectSource& source, std::optional<size_t> index)
{
	if (source.second == ScriptObjectSourceType::SERVER)
		return nullptr;

	auto* server = BabyDI::Get<Server>();
	if (source.second == ScriptObjectSourceType::PLAYER)
	{
		if (auto player = getPlayerFromSource(source, index); player != nullptr)
			return &player->account.character;
	}
	else if (source.second == ScriptObjectSourceType::NPC)
	{
		if (auto npc = getNPCFromSource(source, index); npc != nullptr)
			return &npc->character;
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////

GS1ScriptWrapper::GS1ScriptWrapper(std::string_view script)
{
	input = std::make_shared<antlr4::ANTLRInputStream>(script);
	lexer = std::make_shared<GS1Lexer>(input.get());
	tokens = std::make_shared<antlr4::CommonTokenStream>(lexer.get());
	parser = std::make_shared<GS1Parser>(tokens.get());
	visitor = std::make_shared<GS1Visitor>();
	program = parser->program();
	setReadOnlyGlobalVariables(variables);
}

////////////////////////////////////////////////////////////////////////////////

ScriptEngineGS1::ScriptEngineGS1()
{
}

CompiledScriptResult ScriptEngineGS1::compileScript(std::string_view script)
{
	ScriptExecutionContext result{ .engine = this };
	result.script = std::make_shared<std::any>(std::make_any<GS1ScriptWrapper>(script));
	return result;
}

bool ScriptEngineGS1::execute(ScriptEvent& event, ScriptObjectSource source, CompiledScriptResultPtr context)
{
	auto* wrapper = std::any_cast<GS1ScriptWrapper>(context->script.get());
	if (wrapper == nullptr)
		return false;

	auto* server = BabyDI::Get<Server>();
	auto& [source_id, source_type] = source;

	if (source_type != ScriptObjectSourceType::NPC && source_type != ScriptObjectSourceType::WEAPON)
		throw std::invalid_argument("GS1 scripts can only be executed from NPCs and weapons.");

	PlayerClientPtr player = nullptr;
	NPCPtr npc = nullptr;
	LevelPtr level = nullptr;

	// Set the built-in store.
	wrapper->variables.clearTemporary();
	wrapper->visitor->builtInStore = &wrapper->variables;

	// Get whatever links we can.
	if (source_type == ScriptObjectSourceType::PLAYER)
		player = server->getPlayer<PlayerClient>(source_id);
	if (source_type == ScriptObjectSourceType::NPC)
		npc = server->getNPC(source_id);
	if (player != nullptr)
		level = player->getLevel();
	if (npc != nullptr)
		level = npc->level.lock();

	// Set flags.
	setCustomEventFlags(event, wrapper->variables);
	setEventFlags(event.type, wrapper->variables);
	setPlayerFlags(wrapper->variables, npc, player);
	setNPCFlags(wrapper->variables, npc);
	setLevelFlags(wrapper->variables, npc, level);
	setOtherFlags(wrapper->variables, npc, player, level);

	// Execute the script.
	wrapper->visitor->execute(event, source, *wrapper->parser.get(), *wrapper->program);

	return false;
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::gs1
