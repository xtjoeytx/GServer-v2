#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <BabyDI.h>
#include <CString.h>
#include <IEnums.h>
#include <IUtil.h>

#include <Server.h>
#include <level/Level.h>
#include <level/Map.h>
#include <npcserver/NPCServer.h>
#include <object/Character.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <scripting/Script.h>
#include <scripting/ScriptClass.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/Log.h>
#include <utilities/PropertySerializers.h>
#include <utilities/StringUtils.h>
#include <utilities/std/inplace_vector.h>

////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

// clang-format off
static constexpr std::array<std::string_view, 8> colorPropertyNames = {"#C0"sv, "#C1"sv, "#C2"sv, "#C3"sv, "#C4"sv, "#C5"sv, "#C6"sv, "#C7"sv};
static constexpr std::array<std::string_view, 30> ganiAttributePropertyNames = {
	"#P1"sv, "#P2"sv, "#P3"sv, "#P4"sv, "#P5"sv, "#P6"sv, "#P7"sv, "#P8"sv, "#P9"sv, "#P10"sv,
	"#P11"sv, "#P12"sv, "#P13"sv, "#P14"sv, "#P15"sv, "#P16"sv, "#P17"sv, "#P18"sv, "#P19"sv, "#P20"sv,
	"#P21"sv, "#P22"sv, "#P23"sv, "#P24"sv, "#P25"sv, "#P26"sv, "#P27"sv, "#P28"sv, "#P29"sv, "#P30"sv,
};
static constexpr std::array<uint8_t, 10> savePackets = {23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
// clang-format on

static std::string_view toWeaponName(std::string_view code);

static bool canSendProp(NPCProp prop)
{
	static Server* server = nullptr;
	if (server == nullptr)
		server = BabyDI::Get<Server>();

	if (server->Generation == ServerGeneration::ORIGINAL && PROPID(prop) > PROPID(NPCProp::BODYIMAGE))
		return false;
	if (prop == NPCProp::SCRIPTER || prop == NPCProp::NAME || prop == NPCProp::TYPE)
		return false;
	if (prop == NPCProp::CLASS && (server->Generation == ServerGeneration::ORIGINAL || server->Generation == ServerGeneration::CLASSIC))
		return false;

	return true;
}

//----------------------------

#ifdef PACKETLOGGING
	#define DO_PACKETLOG(LOG) LOG
#else
	#define DO_PACKETLOG(LOG)
#endif

#define PRINT_NPCPROP(prop, ...) #prop##sv,
constexpr std::array<std::string_view, NPCPROP_COUNT> npcPropNames =
{
	FOR_LIST_OF_NPC_PROPS(PRINT_NPCPROP)
};

//----------------------------

NPC::NPC(NPCID id, NPCStorageType storageType)
	: id(id), storageType(storageType), m_savedModTime()
{
	m_server = BabyDI::Get<Server>();
	assert(m_server != nullptr);

	scripting.variables.defaultLifetime = variables::Lifetime::PERMANENT;

	resetToInitialState();
}

NPC::~NPC()
{
#ifdef DEBUG
	log::printLine(log::server, "Destroying NPC [{}] '{}' in level '{}'.", id, name, level);
#endif
}

//----------------------------

void NPC::resetToInitialState()
{
	groupName.clear();
	image.clear();
	shape = {};
	imagePart = {};
	visFlags = PROPID(NPCVisFlags::VISIBLE);
	blockFlags = 0;
	hurtX = 0.0f;
	hurtY = 0.0f;
	noPlayerOnWall = false;
	timeout = 0ms;
	m_initialCharacter.nickName.clear();
	character = m_initialCharacter;
	saves.fill(0);

	warpRestrictions = m_server->hasNPCServer() ? NPCWarpRestrictions::NOTALLOWED : NPCWarpRestrictions::ALLOWED;

	auto now = m_server->getFrameStartTime();

	// We need to alter the modTime of the following props as they should be always sent.
	// If we don't, they won't be sent until the prop gets modified.
	auto props = std::to_array({NPCProp::IMAGE, NPCProp::SCRIPT, NPCProp::X, NPCProp::Y, NPCProp::Z, NPCProp::VISFLAGS, NPCProp::ID, NPCProp::SPRITE, NPCProp::MESSAGE, NPCProp::X2, NPCProp::Y2, NPCProp::Z2});
	std::ranges::for_each(props, [this, now](const NPCProp& prop)
	{
		modTime[PROPID(prop)] = now;
	});

	if (character.mapX != 0 || character.mapY != 0)
	{
		modTime[PROPID(NPCProp::GMAPLEVELX)] = now;
		modTime[PROPID(NPCProp::GMAPLEVELY)] = now;
	}

	m_savedModTime = modTime;
	lastUpdateTime = now;

	// Clear the variables and queues.
	scripting.variables.store.clear();
	moveQueue.clear();

	// Warp.
	if (auto initialLevel = m_server->getStubbedLevel(m_initialLevel); initialLevel != nullptr)
		warp(initialLevel, character.getGlobalPosition());
}

//----------------------------

bool NPC::warp(LevelPtr level, const PixelPosition& position)
{
	if (level == nullptr)
		return false;

	std::inplace_vector<SetResults, 5> warpResults;
	auto localPosition = toLocalPixelPosition(position);
	auto mapPosition = toMapPosition(position);

	// Clear the move queue since we are being forcibly moved.
	moveQueue.clear();

	// Set our new position.
	warpResults.push_back(setPropWith<NPCProp::X2>(SetBy::SERVER, localPosition.x()));
	warpResults.push_back(setPropWith<NPCProp::Y2>(SetBy::SERVER, localPosition.y()));

	// If the level is a gmap, include the map position.
	if (level->isGmap())
	{
		warpResults.push_back(setPropWith<NPCProp::GMAPLEVELX>(SetBy::SERVER, mapPosition.x()));
		warpResults.push_back(setPropWith<NPCProp::GMAPLEVELY>(SetBy::SERVER, mapPosition.y()));
	}

	// If we are moving levels, change the current level.
	// Do this last so our current position is passed on the warp.
	if (level != m_currentLevel.lock())
		warpResults.push_back(setPropWith<NPCProp::CURLEVEL>(SetBy::SERVER, level->levelName));

	sendPropsFromResults(warpResults);

	// If our initial level is not set, set it now.
	if (m_initialLevel.empty())
	{
		m_initialLevel = level->levelName;
		m_initialCharacter = character;
	}

	return true;
}

void NPC::setLevel(LevelPtr level)
{
	if (level == nullptr)
		return;

	// Refresh our mod times.
	refreshModTimes(m_server->getFrameStartTime());

	this->level = level->levelName;
	m_currentLevel = level;
}

//----------------------------

CString NPC::getShowImagesPacket(std::optional<clock::time_point> modTime) const noexcept
{
	// Construct the packet.
	// Index 9 will cause all of the showimgs to be erased on the client.
	CString packet;
	packet >> (char)PLO_SHOWIMGNPC >> (int)id >> (char)9;

	// Send all the showimgs.
	for (const auto& [id, showimg] : showImgList)
		packet >> (char)(id + 10) << showimg.getAllPropsPacket(modTime);

	return packet;
}

void NPC::sendShowImagesToPlayer(PlayerPtr player, std::optional<clock::time_point> modTime) const noexcept
{
	// Only start sending showimg packets when the NPC gains showimgs.
	if (!m_hadShowImgs && showImgList.size() == 0)
		return;

	m_hadShowImgs = true;

	player->sendPacket(getShowImagesPacket(modTime));
}

void NPC::sendAllShowImagesToLevel(std::optional<clock::time_point> modTime) const noexcept
{
	// Only start sending showimg packets when the NPC gains showimgs.
	if (!m_hadShowImgs && showImgList.size() == 0)
		return;

	m_server->sendPacketToNearby(getShowImagesPacket(modTime), getGlobalPosition(), getLevel());
}

//----------------------------

void NPC::addMoveToQueue(const LocalPixelPosition& moveDelta, float durationInSeconds, uint8_t options)
{
	NPCMove move{.duration = std::chrono::duration_cast<std::chrono::milliseconds>(duration_seconds_double{durationInSeconds}), .modTime = m_server->getFrameStartTime()};

	if (options & (1 << NPCMove::cacheNearbyMovement))
		move.options.set(NPCMove::cacheNearbyMovement);
	if (options & (1 << NPCMove::appendMovement))
		move.options.set(NPCMove::appendMovement);
	if (options & (1 << NPCMove::blockCheck))
		move.options.set(NPCMove::blockCheck);
	if (options & (1 << NPCMove::informWhenDone))
		move.options.set(NPCMove::informWhenDone);
	if (options & (1 << NPCMove::applyDirection))
		move.options.set(NPCMove::applyDirection);

	// Determine our start and stop positions.
	move.origin = moveQueue.empty() ? getGlobalPosition() : moveQueue.back().destination;
	move.destination = translatePosition(move.origin, moveDelta);

	bool finishAllMovements = false;

	// If we are not caching or appending movement, and we have some in the queue, finish the queue.
	if (!move.options.test(NPCMove::cacheNearbyMovement) && !move.options.test(NPCMove::appendMovement) && !moveQueue.empty())
		finishAllMovements = true;
	else if (move.options.test(NPCMove::cacheNearbyMovement) && !moveQueue.empty())
	{
		// If the distance to go from the current position to the end of our new movement is over 5,
		// finish all the movements.
		auto currentTilePosition = getTilePosition();
		auto destinationTilePosition = toTilePosition(move.destination);
		auto distance = std::hypot(destinationTilePosition.x() - currentTilePosition.x(), destinationTilePosition.y() - currentTilePosition.y());
		finishAllMovements = distance > 5.0f;
	}

	// If we are clearing the movement queue, pop all but the last movement in the queue
	// and execute the last movement to the end (so any events get called).
	if (finishAllMovements)
	{
		while (moveQueue.size() > 1)
		{
			auto& queue = moveQueue.front();
			if (queue.onComplete)
				queue.onComplete();
			moveQueue.pop_front();
		}
		if (!moveQueue.empty())
			processMoveQueue(moveQueue.front().duration);
	}

	moveQueue.push_back(std::move(move));
}

void NPC::processMoveQueue(std::chrono::milliseconds deltaTime)
{
	if (moveQueue.empty())
		return;

	while (deltaTime != 0ms && !moveQueue.empty())
	{
		NPCMove& move = moveQueue.front();

		// If the move hasn't started yet, do the starting events.
		if (move.elapsed == 0ms)
		{
			// Set the direction when moving.
			if (move.options.test(NPCMove::applyDirection) && isCharacter())
			{
				uint8_t dir = 0;
				if (move.destination.x() > move.origin.x())
					dir = 3;
				if (move.destination.y() > move.origin.y())
					dir = 2;
				if (move.destination.x() < move.origin.x())
					dir = 1;

				setPropWith<NPCProp::SPRITE>(SetBy::SERVER, character.sprite, dir);
			}
		}

		// Calculate our times.
		auto timeRemaining = 0ms;
		move.elapsed += deltaTime;
		if (move.elapsed < move.duration)
		{
			// The duration was fully used up.
			deltaTime = 0ms;
			timeRemaining = move.duration - move.elapsed;
		}
		else
		{
			// We reached the end, so some duration is still remaining.
			deltaTime = move.elapsed - move.duration;
			move.elapsed = move.duration;
		}

		// Determine where we will end up this frame.
		PixelPosition currentPosition{move.getCurrentPosition()};

		// If the map position changed, set that now.
		const auto& [mapX, mapY, _] = toMapPosition(currentPosition);
		if (mapX != character.mapX)
			setPropWith<NPCProp::GMAPLEVELX>(SetBy::SERVER, mapX);
		if (mapY != character.mapY)
			setPropWith<NPCProp::GMAPLEVELY>(SetBy::SERVER, mapY);

		// Set the new position.
		auto localPosition = toLocalPixelPosition(currentPosition);
		setPropWith<NPCProp::X2>(SetBy::SERVER, localPosition.x());
		setPropWith<NPCProp::Y2>(SetBy::SERVER, localPosition.y());

		// Adjust our saved mod times, just in case.
		// We don't want the position to be accidentally sent.
		m_savedModTime[PROPID(NPCProp::X)] = modTime[PROPID(NPCProp::X)];
		m_savedModTime[PROPID(NPCProp::Y)] = modTime[PROPID(NPCProp::Y)];
		m_savedModTime[PROPID(NPCProp::X2)] = modTime[PROPID(NPCProp::X2)];
		m_savedModTime[PROPID(NPCProp::Y2)] = modTime[PROPID(NPCProp::Y2)];

		bool movementFinished = false;

		// If we are testing for walls, do that now.
		if (move.options.test(NPCMove::blockCheck))
		{
			if (auto levelPtr = getLevel(); levelPtr != nullptr)
			{
				// Do an onwall check at the destination.
				// If we collide, then stop the movement.
				auto boundingBox = getCollisionBoundingBox();
				boundingBox.position = currentPosition;

				// Fix offsets for characters.
				if (isCharacter() && (shape.width() == 0 || shape.height() == 0))
					boundingBox.position.translate(8, 16);

				// Check for wall collision.
				bool isOnWall = levelPtr->isOnWall2(boundingBox);
				if (isOnWall)
					movementFinished = true;
			}
		}

		// Check if our movement is done.
		if (timeRemaining <= 0ms)
			movementFinished = true;

		// If the movement is finished, terminate!
		if (movementFinished)
		{
			// Queue the movement finished event.
			if (move.options.test(NPCMove::informWhenDone))
				scripting.events.addEvent(ScriptEventType::MOVEMENTFINISHED, source::FromNPC(id));

			// Finish callback.
			if (move.onComplete)
				move.onComplete();

			// Pop the front movement.
			moveQueue.pop_front();
		}
	}
}

std::pair<CString, CString> NPC::getMoveQueuePacketData(std::optional<clock::time_point> modTime) const noexcept
{
	if (moveQueue.empty())
		return {};

	std::pair<CString, CString> result;

	// Append the whole move queue to the move packet.
	for (const auto& move : moveQueue)
	{
		// Only send newer movements.
		if (modTime.has_value() && move.modTime < modTime.value())
			continue;

		auto durationLeftInSeconds = std::chrono::duration_cast<duration_seconds_double>(move.duration - move.elapsed);
		auto timeIn50msIncrements = static_cast<uint16_t>(durationLeftInSeconds.count() / 0.05f);

		auto currentPosition = move.getCurrentPosition();
		auto dx = static_cast<int16_t>(move.destination.x() - currentPosition.x());
		auto dy = static_cast<int16_t>(move.destination.y() - currentPosition.y());
		auto localPosition = toLocalPixelPosition(currentPosition);

		// Client versions 2.3+ support the new move packet.
		{
			PropertyPixelCoordinate posX{localPosition.x()};
			PropertyPixelCoordinate posY{localPosition.y()};
			PropertyPixelCoordinate moveDX{dx};
			PropertyPixelCoordinate moveDY{dy};

			result.second << posX.serialize() << posY.serialize();
			result.second << moveDX.serialize() << moveDY.serialize();
			result.second >> (short)timeIn50msIncrements;
			result.second >> (char)move.options.to_ulong();
		}
		{
			uint8_t posX = static_cast<uint8_t>(localPosition.x() / 8.0f);
			uint8_t posY = static_cast<uint8_t>(localPosition.y() / 8.0f);
			auto moveDX = static_cast<int8_t>((dx / 8) + 100);
			auto moveDY = static_cast<int8_t>((dy / 8) + 100);

			result.first >> (char)posX >> (char)posY;
			result.first >> (char)moveDX >> (char)moveDY;
			result.first >> (short)timeIn50msIncrements;
			result.first >> (char)move.options.to_ulong();
		}
	}

	return result;
}

void NPC::sendMoveQueueToPlayer(PlayerPtr player, std::optional<clock::time_point> modTime) const noexcept
{
	if (moveQueue.empty())
		return;

	auto [move1, move2] = getMoveQueuePacketData(modTime);
	if (move1.isEmpty())
		return;

	if (player->getVersion() < CLVER_2_3)
		player->sendPacket(CString() >> (char)PLO_MOVE >> (int)id << move1);
	else
		player->sendPacket(CString() >> (char)PLO_MOVE2 >> (int)id << move2);
}

void NPC::sendMoveQueueToLevel(LevelPtr level, std::optional<clock::time_point> modTime) const noexcept
{
	if (moveQueue.empty())
		return;

	sendMoveQueueToLevel(level, getMoveQueuePacketData(modTime));
}

void NPC::sendMoveQueueToLevel(LevelPtr level, const std::pair<CString, CString>& queue) const noexcept
{
	if (queue.first.isEmpty())
		return;

	// Send them out.
	m_server->sendPacketToNearby(CString() >> (char)PLO_MOVE2 >> (int)id << queue.second, character.getGlobalPosition(), level, {}, [](const Player* player)
	{
		return player->getVersion() >= CLVER_2_3;
	});
	m_server->sendPacketToNearby(CString() >> (char)PLO_MOVE >> (int)id << queue.first, character.getGlobalPosition(), level, {}, [](const Player* player)
	{
		return player->getVersion() < CLVER_2_3;
	});
}

void NPC::sendMoveQueueUpdatesToLevel(LevelPtr level) noexcept
{
	auto result = getMoveQueuePacketData(lastMoveQueueSentTime);
	lastMoveQueueSentTime = m_server->getFrameStartTime();
	sendMoveQueueToLevel(level, result);
}

void NPC::refreshModTimes(clock::time_point modTime) noexcept
{
	for (auto& time : this->modTime)
	{
		if (time.has_value())
			time = modTime;
	}
}

//----------------------------

double NPC::getCalculatedTileZ() const noexcept
{
	auto level = getLevel();
	if (level == nullptr || !level->hasTerrain())
		return character.localPixelZ / 16.0;

	PixelPosition testPosition = character.getGlobalPosition();
	if (isCharacter())
		testPosition.translate(24, 48);

	auto terrainHeight = level->getHeightAt(testPosition);
	auto currentZ = character.localPixelZ / 16.0;
	return std::max(terrainHeight, currentZ);
}

//----------------------------

std::string NPC::getLevelName() const
{
	// If we are a control-NPC, our level constantly changes, so don't rely on our pointer.
	if (scriptType == NPCTYPE_CONTROL)
		return level;

	if (auto levelPtr = getLevel(); levelPtr != nullptr)
		return levelPtr->levelName;

	return level;
}

std::shared_ptr<Level> NPC::getLevel() const
{
	// If we are a control-NPC, our level constantly changes, so don't rely on our pointer.
	if (scriptType == NPCTYPE_CONTROL)
		return m_server->getLoadedLevelNoHint(level);

	return m_currentLevel.lock();
}

//----------------------------

void NPC::hurt(int8_t damageInHalves, std::optional<ScriptEventType> damageEventType, std::optional<ScriptObject> source)
{
	// Adjust the NPC's HP.
	if (allowServerDamageReactions && isCharacter())
	{
		sendPropsFromResults(
			setPropWith<NPCProp::POWER>(SetBy::SERVER, static_cast<uint8_t>(std::max(0, character.hitpointsInHalves - damageInHalves)))
		);
	}

	// Queue the hurt event.
	if (damageEventType.has_value())
		scripting.events.addEvent(damageEventType.value(), source.value_or(source::FromServer()));
}

void NPC::hurtAndPush(int8_t damageInHalves, const PixelPosition& pushOrigin, std::optional<ScriptEventType> damageEventType, std::optional<ScriptObject> source)
{
	if (allowServerDamageReactions && isCharacter())
	{
		// Become invulerable for 1.6 seconds.
		if (timeDifference<std::chrono::milliseconds>(character.lastHurtTime, m_server->getFrameStartTime()) < 1600ms)
			return;

		// Push the character away from the source of damage.
		auto tileOrigin = toTilePosition(pushOrigin);
		TilePosition pushVector{character.getTilePosition().x() + 1.5f - tileOrigin.x(), character.getTilePosition().y() + 2.0f - tileOrigin.y()};
		pushVector.normalize2D(pushVector.length2D());
		pushVector = pushVector * 5.0f;

		// Set the hurt animation and force an X/Y prop update (to cancel any current movements), then clear the move queue.
		// This will let us abort any movements in progress.
		std::inplace_vector<SetResults, 3> results;
		results.push_back(setPropWith<NPCProp::X2>(SetBy::SERVER, character.localPixelX));
		results.push_back(setPropWith<NPCProp::Y2>(SetBy::SERVER, character.localPixelY));
		results.push_back(setPropWith<NPCProp::GANI>(SetBy::SERVER, "hurt"));
		moveQueue.clear();

		// Add our new movement to the queue and send it out.
		addMoveToQueue(toLocalPixelPosition(pushVector), 0.5, ENUM(NPCMoveFlags::BLOCKCHECK));
		sendMoveQueueUpdatesToLevel(getLevel());

		// Set up a task to fix the animation.
		m_server->scheduleTask(500ms, [self = m_server->getNPC(id)]()
		{
			if (self != nullptr && self->character.gani == "hurt")
				self->sendPropsFromResults(self->setPropWith<NPCProp::GANI>(SetBy::SERVER, "idle"));
		});

		// Send prop changes.
		sendPropsFromResults(results);

		// Set the last hurt time.
		character.lastHurtTime = m_server->getFrameStartTime();
	}

	// Do the damage.
	hurt(damageInHalves, damageEventType, source);
}

//----------------------------

void NPC::executeEvents(ScriptEventQueue& events, ScriptObject source) const
{
	if (events.queue().empty())
		return;

	m_script.executeEvents(events, source);

	// Execute classes.
	for (auto& [handle, scriptClassPtr] : m_joinedClasses)
	{
		if (auto scriptClass = scriptClassPtr.lock(); scriptClass != nullptr)
			scriptClass->getScript().executeEvents(events, source);
	}

	events.queue().clear();
}

void NPC::setScript(const Script& script)
{
	m_script = script;

	// TODO: Optimize this.  We need a better way to track joined classes and to assign them to the NPC.
	auto classes = string::join(m_script.getServerJoinedClasses() | std::views::keys);
	setJoinedClasses(classes);

	auto clientside = m_script.getClientSide();

	// Check for position update blocking.
	if (m_server->hasNPCServer() || clientside.contains("//#BLOCKPOSITIONUPDATES"))
		m_blockPositionUpdates = true;

	// If we have no npc-server, we support toweapons, so extract the weapon name.
	if (!m_server->hasNPCServer())
		m_weaponName = toWeaponName(clientside);

	// Just a little warning for people who don't know.
	if (m_script.getClientByteCode().empty() && m_script.getClientSide().length() > 0x705F)
		log::printLine(log::server, "WARNING: Clientside script of NPC ({}) exceeds the limit of 28767 bytes.", (image.length() != 0 ? image : std::to_string(id)));
}

void NPC::setScript(std::string_view script)
{
	//auto profile = log::Profile(log::server, "NPC::setScript");

	// Set the script.
	setJoinedClasses("");
	m_script = std::move(Script{name, script});
	modTime[PROPID(NPCProp::SCRIPT)] = m_server->getFrameStartTime();

	// Check if we have joined classes already (due to a cached script).
	for (const auto& [name, classPtr] : m_script.getServerJoinedClasses())
	{
		if (auto scriptClass = classPtr.lock(); scriptClass != nullptr)
		{
			auto it = std::ranges::find_if(m_joinedClasses, [&scriptClass](const decltype(m_joinedClasses)::value_type& kvp)
			{
				return kvp.second.lock()->name == scriptClass->name;
			});
			if (it != m_joinedClasses.end())
				continue;

			auto handle = scriptClass->onScriptModified.subscribe(std::bind(&NPC::updateScriptClass, this, std::placeholders::_1));
#ifdef DEBUG
			log::printLine(log::server, "[DEBUG] NPC '{}' auto-joining class '{}' due to cached script.", name, scriptClass->name);
#endif
			m_joinedClasses.emplace_back(handle, scriptClass);
		}
	}

	auto clientside = m_script.getClientSide();

	// Check for position update blocking.
	if (m_server->hasNPCServer() || clientside.contains("//#BLOCKPOSITIONUPDATES"))
		m_blockPositionUpdates = true;

	// If we have no npc-server, we support toweapons, so extract the weapon name.
	if (!m_server->hasNPCServer())
		m_weaponName = toWeaponName(clientside);

	// Just a little warning for people who don't know.
	if (m_script.getClientByteCode().empty() && m_script.getClientSide().length() > 0x705F)
		log::printLine(log::server, "WARNING: Clientside script of NPC ({}) exceeds the limit of 28767 bytes.", (image.length() != 0 ? image : std::to_string(id)));
}

std::string NPC::getClientSideScript() const
{
	std::string result{m_script.getClientSide()};
	for (const auto& [handle, classPtr] : m_joinedClasses)
	{
		if (auto scriptClass = classPtr.lock(); scriptClass != nullptr)
		{
			const auto& clientSide = scriptClass->getScript().getClientSide();
			if (!clientSide.empty())
			{
				result += "\xa7";
				result += clientSide;
			}
		}
	}
	return result;
}

std::string NPC::getJoinedClassesList() const
{
	bool hasExpired = false;
	std::string result;
	for (const auto& [handle, classPtr] : m_joinedClasses)
	{
		if (auto scriptClass = classPtr.lock(); scriptClass != nullptr)
		{
			result += scriptClass->name;
			result += ",";
		}
		else hasExpired = true;
	}
	if (!result.empty())
		result.pop_back();

	// If we have expired, clear them out.
	if (hasExpired)
	{
		std::erase_if(m_joinedClasses, [this](const decltype(m_joinedClasses)::value_type& pair)
		{
			return pair.second.expired();
		});
	}

	return result;
}

bool NPC::hasJoinedClass(std::string_view className) const
{
	for (const auto& [handle, classPtr] : m_joinedClasses)
	{
		if (auto scriptClass = classPtr.lock(); scriptClass != nullptr && scriptClass->name == className)
			return true;
	}
	return false;
}

void NPC::setJoinedClasses(std::string_view classes)
{
	if (!m_server->hasNPCServer()) return;

	for (const auto& [handle, classPtr] : m_joinedClasses)
	{
		if (auto scriptClass = classPtr.lock(); scriptClass != nullptr)
			scriptClass->onScriptModified.unsubscribe(handle);
	}

	m_joinedClasses.clear();

	bool sendToLevel = false;
	while (!classes.empty())
	{
		auto className = string::extractLine(classes, ',');
		if (className.empty())
			continue;

		className = string::trim(className);
		if (auto scriptClass = m_server->getNPCServer()->getClass(className).lock(); scriptClass != nullptr)
		{
			auto handle = scriptClass->onScriptModified.subscribe(std::bind(&NPC::updateScriptClass, this, std::placeholders::_1));
			m_joinedClasses.emplace_back(handle, scriptClass);
			modTime[PROPID(NPCProp::CLASS)] = m_server->getFrameStartTime();
			lastUpdateTime = m_server->getFrameStartTime();

			// If the joined script has clientside code, delete the NPC and resend the new code.
			if (!scriptClass->getScript().getClientSide().empty())
				sendToLevel = true;
		}
	}

	if (sendToLevel)
		sendScriptUpdatesToLevel(lastUpdateTime);
}

void NPC::joinClass(std::string_view className)
{
	auto it = std::ranges::find_if(m_joinedClasses, [&className](const decltype(m_joinedClasses)::value_type& kvp)
	{
		return kvp.second.lock()->name == className;
	});
	if (it != m_joinedClasses.end())
		return;

	if (!m_server->hasNPCServer())
		return;

	if (auto scriptClass = m_server->getNPCServer()->getClass(className).lock(); scriptClass != nullptr)
	{
		auto handle = scriptClass->onScriptModified.subscribe(std::bind(&NPC::updateScriptClass, this, std::placeholders::_1));
		m_joinedClasses.emplace_back(handle, scriptClass);
		modTime[PROPID(NPCProp::CLASS)] = m_server->getFrameStartTime();
		lastUpdateTime = m_server->getFrameStartTime();

		// If the joined script has clientside code, delete the NPC and resend the new code.
		if (!scriptClass->getScript().getClientSide().empty())
			sendScriptUpdatesToLevel(lastUpdateTime);
	}
	else
	{
		log::printLine(log::npc, "Error: NPC [{}] '{}' tried to join class '{}', but it does not exist.", id, name, className);
	}
}

void NPC::leaveClass(std::string_view className)
{
	auto it = std::ranges::find_if(m_joinedClasses, [&className](const decltype(m_joinedClasses)::value_type& kvp)
	{
		return kvp.second.lock()->name == className;
	});
	if (it == m_joinedClasses.end())
		return;

	if (!m_server->hasNPCServer())
		return;

	bool sendToLevel = false;
	if (auto scriptClass = it->second.lock(); scriptClass != nullptr)
	{
		scriptClass->onScriptModified.unsubscribe(it->first);
		modTime[PROPID(NPCProp::CLASS)] = m_server->getFrameStartTime();
		lastUpdateTime = m_server->getFrameStartTime();

		// If the joined script has clientside code, delete the NPC and resend the new code.
		if (!scriptClass->getScript().getClientSide().empty())
			sendToLevel = true;
	}

	if (sendToLevel)
		sendScriptUpdatesToLevel(lastUpdateTime);

	m_joinedClasses.erase(it);
}

void NPC::sendScriptUpdatesToLevel(clock::time_point when) const
{
	if (auto npclevel = getLevel(); npclevel != nullptr)
	{
		if (auto levelData = npclevel->getStaticLevelDataAtPosition(character.getMapPosition()); levelData != nullptr)
		{
			const auto& levelName = npclevel->levelName;

			CString packet = CString() >> (char)PLO_NPCDEL2 >> (char)levelName.length() << levelName >> (int)id;
			m_server->sendPacketToLevelAndPastVisitorsAfter(levelData.get(), when, packet);
			m_server->sendPacketToNearby(CString() >> (char)PLO_NPCPROPS >> (int)id << getAllPropsPacket(), character.getGlobalPosition(), npclevel);
		}
	}
}

void NPC::updateScriptClass(ScriptClass* scriptClass)
{
	if (scriptClass == nullptr || !m_server->hasNPCServer())
		return;
	if (scriptClass->getScript().getClientSide().empty())
		return;

	sendScriptUpdatesToLevel(lastUpdateTime);
	modTime[PROPID(NPCProp::SCRIPT)] = m_server->getFrameStartTime();
	lastUpdateTime = m_server->getFrameStartTime();
}

//----------------------------

std::shared_ptr<PropertyBase> NPC::constructPropFor(NPCProp prop) const
{
	switch (prop)
	{
#define GENERATE_CONSTRUCTPROPFOR_CASE(prop, type, ...) \
	case prop: return std::make_shared<type>();
		FOR_LIST_OF_NPC_PROPS(GENERATE_CONSTRUCTPROPFOR_CASE);
	}
	throw std::invalid_argument("Invalid NPCProp type in constructPropFor");
}

//----------------------------

std::shared_ptr<PropertyBase> NPC::getProp(NPCProp prop) const
{
	switch (prop)
	{
#define GENERATE_GETPROP_CASE(prop, type, ...) \
	case prop: return std::make_shared<type>(__VA_ARGS__);
		FOR_LIST_OF_NPC_PROPS(GENERATE_GETPROP_CASE);
	}

	throw std::invalid_argument("Invalid NPCProp type in getProp");
}

//----------------------------

SetResults NPC::setProp(NPCProp prop, SetBy setBy, std::shared_ptr<PropertyBase> base)
{
	PropertyBase* basePtr = base.get();
	if (basePtr != nullptr)
		return setProp(prop, setBy, basePtr);
	throw std::invalid_argument("setProp called with nullptr base pointer.");
}

SetResults NPC::setProp(NPCProp prop, SetBy setBy, PropertyBase* base)
{
	auto levelPtr = getLevel();
	bool canUpdatePosition = !m_blockPositionUpdates || setBy == props::SetBy::SERVER;

	props::SetResults result{.propId = {PROPID(prop)}};
	result.resultFlags.set(props::SetResults::sendToLevel, true);
	result.resultFlags.set(props::SetResults::sendToSource, false);

	const auto& curTime = m_server->getFrameStartTime();
	auto oldTime = modTime[PROPID(prop)];
	auto oldLastUpdateTime = lastUpdateTime;

	modTime[PROPID(prop)] = curTime;
	lastUpdateTime = curTime;

#define SETPROP_RETURN_ERROR                            \
	do                                                  \
	{                                                   \
		result.resultFlags.set(SetResults::wasInvalid); \
		modTime[PROPID(prop)] = oldTime;                \
		lastUpdateTime = oldLastUpdateTime;             \
		return result;                                  \
	}                                                   \
	while (false)

	switch (prop)
	{
		case NPCProp::IMAGE:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr || strProp->value == image)
				SETPROP_RETURN_ERROR;

			// If we are changing to a character, set the gani to idle.
			if (strProp->value == "#c#" && image != "#c")
			{
				visFlags |= PROPID(NPCVisFlags::MALE);
				if (m_server->Generation != ServerGeneration::ORIGINAL)
				{
					character.gani = "idle";
					result.resultPropIds.push_back(PROPID(NPCProp::GANI));
				}
			}

			image = strProp->value;
			auto oldVisFlags = visFlags;

			// If the image is being set and it is empty or "-", and we don't have a shape, make us invisible.
			// This will prevent the NPC from being seen as an obstacle in serverside checks.
			if (!hasImage() && !hasShape())
				visFlags &= ~(uint8_t)NPCVisFlags::VISIBLE;
			else
				visFlags |= (uint8_t)NPCVisFlags::VISIBLE;

			// If we had a visibility change, send it.
			if (visFlags != oldVisFlags)
				result.resultPropIds.push_back(PROPID(NPCProp::VISFLAGS));
			break;
		}

		case NPCProp::SCRIPT:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr || setBy != SetBy::SERVER)
				SETPROP_RETURN_ERROR;

			setScript(strProp->value);
			break;
		}

		case NPCProp::X:
		{
			PropertyTileCoordinate* coordProp = dynamic_cast<PropertyTileCoordinate*>(base);
			if (coordProp == nullptr || !canUpdatePosition)
				SETPROP_RETURN_ERROR;

			character.localPixelX = coordProp->pixelCoordinate;
			result.resultPropIds.push_back(PROPID(NPCProp::X2));

			// Do collision testing.
			testForTouch(result);
			break;
		}

		case NPCProp::Y:
		{
			PropertyTileCoordinate* coordProp = dynamic_cast<PropertyTileCoordinate*>(base);
			if (coordProp == nullptr || !canUpdatePosition)
				SETPROP_RETURN_ERROR;

			character.localPixelY = coordProp->pixelCoordinate;
			result.resultPropIds.push_back(PROPID(NPCProp::Y2));

			// Do collision testing.
			testForTouch(result);
			break;
		}

		case NPCProp::Z:
		{
			PropertyTileCoordinateZ* zProp = dynamic_cast<PropertyTileCoordinateZ*>(base);
			if (zProp == nullptr || !canUpdatePosition)
				SETPROP_RETURN_ERROR;

			character.localPixelZ = zProp->pixelCoordinate;
			result.resultPropIds.push_back(PROPID(NPCProp::Z2));

			// No collision testing for Z movement.
			break;
		}

		case NPCProp::POWER:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.hurtDeltaInHalves = character.hitpointsInHalves - numProp->value;
			character.hitpointsInHalves = numProp->value;

			if (character.hurtDeltaInHalves != 0)
				character.lastHurtTime = curTime;
			break;
		}

		case NPCProp::RUPEES:
		{
			PropertyNumeric<GBYTE3>* numProp = dynamic_cast<PropertyNumeric<GBYTE3>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.gralats = numProp->value;
			break;
		}

		case NPCProp::ARROWS:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.arrows = numProp->value;
			break;
		}

		case NPCProp::BOMBS:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.bombs = numProp->value;
			break;
		}

		case NPCProp::GLOVEPOWER:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.glovePower = numProp->value;
			break;
		}

		case NPCProp::BOMBPOWER:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.bombPower = numProp->value;
			break;
		}

		case NPCProp::SWORDIMAGE:
		{
			PropertySwordPower* swordProp = dynamic_cast<PropertySwordPower*>(base);
			if (swordProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (swordProp->power.has_value())
				character.swordPower = props::Limits::applySwordPower(swordProp->power.value_or(1));

			character.swordImage = props::Limits::apply(swordProp->image, props::Limits::SwordImageLength);
			break;
		}

		case NPCProp::SHIELDIMAGE:
		{
			PropertyShieldPower* shieldProp = dynamic_cast<PropertyShieldPower*>(base);
			if (shieldProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (shieldProp->power.has_value())
				character.shieldPower = props::Limits::applyShieldPower(shieldProp->power.value_or(1));

			character.shieldImage = props::Limits::apply(shieldProp->image, props::Limits::ShieldImageLength);
			break;
		}

		case NPCProp::GANI:
		{
			PropertyGaniOrBowGif* ganiProp = dynamic_cast<PropertyGaniOrBowGif*>(base);
			if (ganiProp == nullptr)
				SETPROP_RETURN_ERROR;

			// 1.x servers didn't have ganis.  This prop was used for the bow instead.
			if (m_server->Generation == ServerGeneration::ORIGINAL)
			{
				if (!ganiProp->bowGif.has_value())
					SETPROP_RETURN_ERROR;

				auto& [image, power] = ganiProp->bowGif.value();
				character.bowPower = props::Limits::apply(power, props::Limits::MaxBowPower);
				character.bowImage = image;
				if (!character.bowImage.empty() && !character.bowImage.contains('.'))
					character.bowImage += ".gif";
				break;
			}

			// Set the gani.
			std::string gani = ganiProp->gani.value_or("idle");
			character.gani = props::Limits::apply(gani, props::Limits::GaniLength);
			result.resultFlags.set(SetResults::getLatestOnSend);

			// If we aren't a character, do that now.
			if (!isCharacter())
			{
				image = "#c#";
				result.resultPropIds.push_back(PROPID(NPCProp::IMAGE));
			}

			// If we are not in a legacy sprite gani and our sprite is not 0, reset the sprite.
			if (!character.gani.starts_with("def[") && character.sprite != 0)
			{
				character.sprite = 0;
				result.resultPropIds.push_back(PROPID(NPCProp::SPRITE));
			}

			// If we are hurting, and didn't get hurt this frame, unset the hurt time.
			if (character.lastHurtTime != curTime)
				character.lastHurtTime = clock::time_point::min();

			// Allow spin to hurt things.
			if (character.gani == "spin")
			{
				auto self = m_server->getNPC(id);
				float tX = static_cast<float>(character.localPixelX / 16.0f) + 1.5f;
				float tY = static_cast<float>(character.localPixelY / 16.0f) + 2.0f;
				m_server->hitObjectsAtPoint({tX, tY + 2.0f}, character.swordPower, m_currentLevel, self);
				m_server->hitObjectsAtPoint({tX, tY - 2.0f}, character.swordPower, m_currentLevel, self);
				m_server->hitObjectsAtPoint({tX + 2.0f, tY}, character.swordPower, m_currentLevel, self);
				m_server->hitObjectsAtPoint({tX - 2.0f, tY}, character.swordPower, m_currentLevel, self);
			}
			break;
		}

		case NPCProp::VISFLAGS:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			visFlags = numProp->value;
			break;
		}

		case NPCProp::BLOCKFLAGS:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			blockFlags = numProp->value;
			break;
		}

		case NPCProp::MESSAGE:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.chatMessage = strProp->value;
			break;
		}

		case NPCProp::HURTDXDY:
		{
			PropertyHurtDxDy* hurtProp = dynamic_cast<PropertyHurtDxDy*>(base);
			if (hurtProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.hurtPushDeltaInHalfPixels[0] = hurtProp->hurtDX;
			character.hurtPushDeltaInHalfPixels[1] = hurtProp->hurtDY;
			break;
		}

		case NPCProp::ID:
			break;

		case NPCProp::SPRITE:
		{
			PropertySprite* spriteProp = dynamic_cast<PropertySprite*>(base);
			if (spriteProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.direction = spriteProp->direction;
			character.sprite = spriteProp->sprite;
			result.resultFlags.set(SetResults::getLatestOnSend);

			// If we manually set a sprite, change the gani.
			if (m_server->Generation != ServerGeneration::ORIGINAL && character.sprite != 0)
			{
				auto gani = std::format("def[{}]", character.sprite);
				//visFlags |= static_cast<uint8_t>(NPCVisFlags::UNKNOWNBIT5);
				result.resultPropIds.push_back(PROPID(NPCProp::GANI));
				//result.resultPropIds.push_back(PROPID(NPCProp::VISFLAGS));
			}
			break;
		}

		case NPCProp::COLORS:
		{
			PropertyColors* colorProp = dynamic_cast<PropertyColors*>(base);
			if (colorProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.colors = colorProp->values;
			break;
		}

		case NPCProp::NICKNAME:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.nickName = strProp->value;
			break;
		}

		case NPCProp::HORSEIMAGE:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.horseImage = strProp->value;

			if (m_server->Generation == ServerGeneration::ORIGINAL && !character.horseImage.empty() && !character.horseImage.contains('.'))
				character.horseImage += ".gif";
			break;
		}

		case NPCProp::HEADIMAGE:
		{
			PropertyHeadGif* headProp = dynamic_cast<PropertyHeadGif*>(base);
			if (headProp == nullptr)
				SETPROP_RETURN_ERROR;

			std::string img;
			if (std::holds_alternative<uint8_t>(headProp->image))
				img = std::format("head{}.{}", std::get<uint8_t>(headProp->image), (m_server->Generation != ServerGeneration::ORIGINAL ? "png" : "gif"));
			else
				img = std::get<std::string>(headProp->image);

			if (m_server->Generation == ServerGeneration::ORIGINAL && !img.empty() && !img.contains('.'))
				img += ".gif";

			character.headImage = props::Limits::apply(img, props::Limits::HeadImageLength);
			result.resultFlags.set(SetResults::getLatestOnSend);
			break;
		}

		case NPCProp::SAVE0:
		case NPCProp::SAVE1:
		case NPCProp::SAVE2:
		case NPCProp::SAVE3:
		case NPCProp::SAVE4:
		case NPCProp::SAVE5:
		case NPCProp::SAVE6:
		case NPCProp::SAVE7:
		case NPCProp::SAVE8:
		case NPCProp::SAVE9:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			auto index = PROPID(prop) - PROPID(NPCProp::SAVE0);
			saves[index] = numProp->value;
			break;
		}

		case NPCProp::ALIGNMENT:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.ap = numProp->value;
			break;
		}

		case NPCProp::IMAGEPART:
		{
			PropertyImagePart* imgPartProp = dynamic_cast<PropertyImagePart*>(base);
			if (imgPartProp == nullptr)
				SETPROP_RETURN_ERROR;

			imagePart = imgPartProp->imagePart;
			break;
		}

		case NPCProp::BODYIMAGE:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.bodyImage = strProp->value;
			break;
		}

		case NPCProp::GMAPLEVELX:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.mapX = numProp->value;
			break;
		}

		case NPCProp::GMAPLEVELY:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			character.mapY = numProp->value;
			break;
		}

		case NPCProp::UNKNOWN48:
			break;

		case NPCProp::SCRIPTER:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			scripter = strProp->value;
			break;
		}

		case NPCProp::NAME:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			name = strProp->value;
			break;
		}

		case NPCProp::TYPE:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			scriptType = strProp->value;
			break;
		}

		case NPCProp::CURLEVEL:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr || !canUpdatePosition)
				SETPROP_RETURN_ERROR;

			// No change?  Don't do anything.
			if (level == strProp->value)
				SETPROP_RETURN_ERROR;

			// See if the level exists.
			auto newLevel = m_server->getLoadedLevel(strProp->value, levelPtr);
			if (newLevel == nullptr)
				SETPROP_RETURN_ERROR;

			// Tell everybody we are moving.
			// This should technically only be sent to players in the level or those who had been in the level.
			auto localPosition = getLocalPosition();
			m_server->sendPacketToType(PLTYPE_ANYCLIENT, CString() >> (char)PLO_NPCMOVED >> (int)id >> (char)(localPosition.x() / 8) >> (char)(localPosition.y() / 8) << strProp->value);

			// Remove ourself from the old level.
			if (auto oldLevel = getLevel(); oldLevel != nullptr)
				oldLevel->removeNPC(id);

			// Add us to the new level.
			newLevel->addNPC(id);

			// Send our props to people in the new level.
			m_server->sendPacketToNearby(CString() >> (char)PLO_NPCPROPS >> (int)id << getAllPropsPacket(), character.getGlobalPosition(), newLevel);

			// Tell NCs about our new position.
			CString ncPacket = CString() >> (char)PLO_NC_NPCADD >> (int)id >> (char)NPCProp::CURLEVEL << getProp<NPCProp::CURLEVEL>().serialize();
			m_server->sendPacketToType(PLTYPE_ANYNC, ncPacket);

			// Send the NPCWARPED event to the NPC.
			scripting.events.addEvent(ScriptEventType::NPCWARPED, source::FromNPC(id));
			break;
		}

		case NPCProp::GATTRIB1:
		case NPCProp::GATTRIB2:
		case NPCProp::GATTRIB3:
		case NPCProp::GATTRIB4:
		case NPCProp::GATTRIB5:
		case NPCProp::GATTRIB6:
		case NPCProp::GATTRIB7:
		case NPCProp::GATTRIB8:
		case NPCProp::GATTRIB9:
		case NPCProp::GATTRIB10:
		case NPCProp::GATTRIB11:
		case NPCProp::GATTRIB12:
		case NPCProp::GATTRIB13:
		case NPCProp::GATTRIB14:
		case NPCProp::GATTRIB15:
		case NPCProp::GATTRIB16:
		case NPCProp::GATTRIB17:
		case NPCProp::GATTRIB18:
		case NPCProp::GATTRIB19:
		case NPCProp::GATTRIB20:
		case NPCProp::GATTRIB21:
		case NPCProp::GATTRIB22:
		case NPCProp::GATTRIB23:
		case NPCProp::GATTRIB24:
		case NPCProp::GATTRIB25:
		case NPCProp::GATTRIB26:
		case NPCProp::GATTRIB27:
		case NPCProp::GATTRIB28:
		case NPCProp::GATTRIB29:
		case NPCProp::GATTRIB30:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			auto index = std::ranges::distance(NPCGaniAttrPackets.begin(), std::ranges::find(NPCGaniAttrPackets, PROPID(prop)));
			character.ganiAttributes[index] = strProp->value;
			break;
		}

		case NPCProp::CLASS:
		{
			PropertyLongString* strProp = dynamic_cast<PropertyLongString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			setJoinedClasses(strProp->value);
			break;
		}

		case NPCProp::X2:
		{
			PropertyPixelCoordinate* pixelProp = dynamic_cast<PropertyPixelCoordinate*>(base);
			if (pixelProp == nullptr || !canUpdatePosition)
				SETPROP_RETURN_ERROR;

			character.localPixelX = pixelProp->pixelCoordinate;
			result.resultPropIds.push_back(PROPID(NPCProp::X));

			// Do collision testing.
			testForTouch(result);
			break;
		}

		case NPCProp::Y2:
		{
			PropertyPixelCoordinate* pixelProp = dynamic_cast<PropertyPixelCoordinate*>(base);
			if (pixelProp == nullptr || !canUpdatePosition)
				SETPROP_RETURN_ERROR;

			character.localPixelY = pixelProp->pixelCoordinate;
			result.resultPropIds.push_back(PROPID(NPCProp::Y));

			// Do collision testing.
			testForTouch(result);
			break;
		}

		case NPCProp::Z2:
		{
			PropertyPixelCoordinate* pixelProp = dynamic_cast<PropertyPixelCoordinate*>(base);
			if (pixelProp == nullptr || !canUpdatePosition)
				SETPROP_RETURN_ERROR;

			character.localPixelZ = pixelProp->pixelCoordinate;
			result.resultPropIds.push_back(PROPID(NPCProp::Z));

			// Do collision testing.
			testForTouch(result);
			break;
		}
	}

	// If we are sending other ids, we need to update the mod time for them too.
	if (!result.resultPropIds.empty() && !result.resultFlags.test(SetResults::wasInvalid))
	{
		for (const auto& id : result.resultPropIds)
			modTime[id] = curTime;
	}

	return result;
}

//----------------------------

void NPC::sendPropsFromSendResults(PropertySendResults& results, PlayerPtr source) const
{
	CString sendAll, sendLevel, sendSource;

	std::erase_if(results, [](const PropertySendResults::value_type& res)
	{
		return !canSendProp((NPCProp)res.first.propId);
	});

	collectPacketsFromResults(results, sendAll, sendLevel, sendSource, [this](uint8_t propId, SetResults::ResultFlagType& destinations)
	{
		return this->getProp((NPCProp)propId);
	});

	// Send the buffers out.
	if (sendAll.length() > 0)
		m_server->sendPacketToAll(CString() >> (char)PLO_NPCPROPS >> (int)id << sendAll);

	PlayerID exclude = 0;
	if (source != nullptr)
		exclude = source->getId();

	if (sendLevel.length() > 0 && !m_currentLevel.expired())
		m_server->sendPacketToNearby(CString() >> (char)PLO_NPCPROPS >> (int)id << sendLevel, character.getGlobalPosition(), getLevel(), {exclude});

	if (sendSource.length() > 0 && source != nullptr)
		source->sendPacket(CString() >> (char)PLO_NPCPROPS >> (int)id << sendSource);
}

//----------------------------

void NPC::setPropsFromPacket(CString& packet, PlayerPtr source)
{
	DO_PACKETLOG(log::printBlock(log::networkdump, "NPC::setPropsFromPacket:\n"));

	PropertySendResults results;
	auto setBy = (source != nullptr ? SetBy::CLIENT : SetBy::SERVER);

	while (packet.bytesLeft() > 0)
	{
		NPCProp propId = (NPCProp)packet.readGUChar();

		DO_PACKETLOG(size_t oldPos = packet.readPos());

		auto prop = constructPropFor(propId);
		prop->deserialize(packet);

#ifdef PACKETLOGGING
		size_t currentPos = packet.readPos();
		CString rawData = packet.subString(oldPos, currentPos - oldPos);

		log::printBlock(log::networkdump, "  {}: {} |", npcPropNames[PROPID(propId)], prop);
		for (size_t i = 0; i < rawData.length(); ++i)
		{
			log::printBlock(log::networkdump, " {:02x}", (unsigned char)rawData[i]);
		}
		log::printBlock(log::networkdump, "\n");
#endif

		results.emplace_back(setProp(propId, setBy, prop), prop);
	}
	DO_PACKETLOG(log::print(log::networkdump, "\n"));

	sendPropsFromSendResults(results, source);
}

//----------------------------

CString NPC::getModifiedPropsPacket() const
{
	DO_PACKETLOG(bool printedHeader = false);

	CString result;
	for (auto i = 0; i < NPCPROP_COUNT; ++i)
	{
		if (!canSendProp((NPCProp)i))
			continue;

		if (modTime[i].has_value() && modTime[i] != m_savedModTime[i])
		{
			DO_PACKETLOG(if (!printedHeader) { printedHeader = true; log::printBlock(log::networkdump, "NPC::getModifiedPropsPacket:\n"); log::printBlock(log::networkdump, "  NPCProp::ID: value: {}\n", id); });

			if (i == PROPID(NPCProp::GANI) && !isCharacter())
			{
				DO_PACKETLOG(log::printBlock(log::networkdump, "  NPCProp::GANI: (empty)\n"));
				result >> (char)i >> (char)0;
			}
			else
			{
#ifdef PACKETLOGGING
				auto prop = getProp((NPCProp)i);
				CString data = prop->serialize();

				log::printBlock(log::networkdump, "  {}: {}", npcPropNames[i], prop);
				if ((NPCProp)i != NPCProp::SCRIPT)
				{
					log::printBlock(log::networkdump, " |");
					for (size_t i = 0; i < data.length(); ++i)
						log::printBlock(log::networkdump, " {:02x}", (unsigned char)data[i]);
				}
				log::printBlock(log::networkdump, "\n");

				result >> (char)i << data;
#else
				result >> (char)i << getProp((NPCProp)i)->serialize();
#endif
			}
		}
	}

	DO_PACKETLOG(if (printedHeader) log::print(log::networkdump, "\n"));
	return result;
}

CString NPC::getAllPropsPacket(std::optional<clock::time_point> newTime) const
{
	DO_PACKETLOG(log::printBlock(log::networkdump, "NPC::getAllPropsPacket:\n"));

	CString retVal;
	int pmax = NPCPROP_COUNT;

	for (int i = 0; i < pmax; i++)
	{
		if (!canSendProp((NPCProp)i))
			continue;

		if (modTime[i].has_value() && modTime[i].value() >= newTime.value_or(clock::time_point::min()))
		{
			if (i == PROPID(NPCProp::GANI) && !isCharacter())
			{
				DO_PACKETLOG(log::printBlock(log::networkdump, "  NPCProp::GANI: (empty)\n"));
				retVal >> (char)i >> (char)0;
			}
			else
			{
#ifdef PACKETLOGGING
				auto prop = getProp((NPCProp)i);
				CString data = prop->serialize();

				log::printBlock(log::networkdump, "  {}: {}", npcPropNames[i], prop);
				if ((NPCProp)i != NPCProp::SCRIPT)
				{
					log::printBlock(log::networkdump, " |");
					for (size_t i = 0; i < data.length(); ++i)
						log::printBlock(log::networkdump, " {:02x}", (unsigned char)data[i]);
				}
				log::printBlock(log::networkdump, "\n");

				retVal >> (char)i << data;
#else
				retVal >> (char)i << getProp((NPCProp)i)->serialize();
#endif
			}
		}
	}

	DO_PACKETLOG(log::print(log::networkdump, "\n"));
	return retVal;
}

//----------------------------

void NPC::constructScriptParameters()
{
	// clang-format off
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"id"sv, std::ref(modTime[PROPID(NPCProp::ID)]), std::ref(id)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{"hurtdpower"sv, std::nullopt, std::ref(character.hurtDeltaInHalves)});

	bind::bindPropertyAsReadOnly(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"width"sv, [this](std::optional<size_t>) -> GameValueVariantForGetter { return getComputedShape().width() / 16.0; }
	});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"height"sv, [this](std::optional<size_t>) -> GameValueVariantForGetter { return getComputedShape().height() / 16.0; }
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::DivideByIntegralProperty{"z"sv, std::ref(modTime[PROPID(NPCProp::Z2)]), std::ref(character.localPixelZ), 16});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::DivideByIntegralProperty{"hearts"sv, std::ref(modTime[PROPID(NPCProp::POWER)]), std::ref(character.hitpointsInHalves), 2});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::DivideByIntegralProperty{"hp"sv, std::ref(modTime[PROPID(NPCProp::POWER)]), std::ref(character.hitpointsInHalves), 2});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"ap"sv, std::ref(modTime[PROPID(NPCProp::ALIGNMENT)]), std::ref(character.ap)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"rupees"sv, std::ref(modTime[PROPID(NPCProp::RUPEES)]), std::ref(character.gralats)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"gralats"sv, std::ref(modTime[PROPID(NPCProp::RUPEES)]), std::ref(character.gralats)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"bombs"sv, std::ref(modTime[PROPID(NPCProp::BOMBS)]), std::ref(character.bombs)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"darts"sv, std::ref(modTime[PROPID(NPCProp::ARROWS)]), std::ref(character.arrows)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"glovepower"sv, std::ref(modTime[PROPID(NPCProp::GLOVEPOWER)]), std::ref(character.glovePower)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"swordpower"sv, std::ref(modTime[PROPID(NPCProp::SWORDIMAGE)]), std::ref(character.swordPower)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{"shieldpower"sv, std::ref(modTime[PROPID(NPCProp::SHIELDIMAGE)]), std::ref(character.shieldPower)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::TimeoutProperty{"timeout"sv, std::ref(timeout)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralArrayProperty{"save"sv, std::ref(modTime), PROPID(NPCProp::SAVE0), std::ref(saves)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{"#1"sv, std::ref(modTime[PROPID(NPCProp::SWORDIMAGE)]), std::ref(character.swordImage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{"#2"sv, std::ref(modTime[PROPID(NPCProp::SHIELDIMAGE)]), std::ref(character.shieldImage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{"#3"sv, std::ref(modTime[PROPID(NPCProp::HEADIMAGE)]), std::ref(character.headImage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{"#5"sv, std::ref(modTime[PROPID(NPCProp::HORSEIMAGE)]), std::ref(character.horseImage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{"#7"sv, std::ref(modTime[PROPID(NPCProp::GANI)]), std::ref(character.bowImage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{"#8"sv, std::ref(modTime[PROPID(NPCProp::BODYIMAGE)]), std::ref(character.bodyImage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{"#c"sv, std::ref(modTime[PROPID(NPCProp::MESSAGE)]), std::ref(character.chatMessage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{"#m"sv, std::ref(modTime[PROPID(NPCProp::GANI)]), std::ref(character.gani)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{"#n"sv, std::ref(modTime[PROPID(NPCProp::NICKNAME)]), std::ref(character.nickName)});

	// colors
	for (size_t i = 0; i < character.colors.size(); ++i)
		bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{colorPropertyNames[i], std::ref(modTime[PROPID(NPCProp::COLORS)]), std::ref(character.colors[i])});

	// gani attributes
	for (size_t i = 0; i < 30; ++i)
		bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{ganiAttributePropertyNames[i], std::ref(modTime[NPCGaniAttrPackets[i]]), std::ref(character.ganiAttributes[i])});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"x"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter { return (double)character.getTilePosition().x(); },
		[this](GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
			{
				auto globalPosition = character.getGlobalPosition();
				globalPosition.x() = value->get() * 16;
				character.localPixelX = toLocalPixelPosition(globalPosition).x();
				moveQueue.clear();

				// Update the location props.
				auto now = m_server->getFrameStartTime();
				modTime[PROPID(NPCProp::X)] = now;
				modTime[PROPID(NPCProp::X2)] = now;

				// Fix the map position if applicable.
				if (auto levelPtr = getLevel(); levelPtr != nullptr && levelPtr->isGmap())
				{
					if (auto mapX = toMapPosition(globalPosition).x(); mapX != character.mapX)
					{
						character.mapX = mapX;
						modTime[PROPID(NPCProp::GMAPLEVELX)] = now;
					}
				}
			}
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"y"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter { return character.getTilePosition().y(); },
		[this](GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
			{
				auto globalPosition = character.getGlobalPosition();
				globalPosition.y() = value->get() * 16;
				character.localPixelY = toLocalPixelPosition(globalPosition).y();
				moveQueue.clear();

				// Update the location props.
				auto now = m_server->getFrameStartTime();
				modTime[PROPID(NPCProp::Y)] = now;
				modTime[PROPID(NPCProp::Y2)] = now;

				// Fix the map position if applicable.
				if (auto levelPtr = getLevel(); levelPtr != nullptr && levelPtr->isGmap())
				{
					if (auto mapY = toMapPosition(globalPosition).y(); mapY != character.mapY)
					{
						character.mapY = mapY;
						modTime[PROPID(NPCProp::GMAPLEVELY)] = now;
					}
				}
			}
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"headset"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter
		{
			int headSet = -1;
			if (character.headImage.starts_with("head"))
				string::toNumber(character.headImage.substr(4), headSet);
			return static_cast<double>(headSet);
		},
		[this](GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			static double noHeadSet = -1.0;
			static auto noHeadRef = std::ref(noHeadSet);
			auto value = std::get_if<std::reference_wrapper<double>>(&incoming);
			if (value == nullptr)
				value = &noHeadRef;

			auto headSet = std::clamp(static_cast<int>(value->get()), -1, 99);
			if (headSet != -1)
			{
				character.headImage = std::format("head{}.{}", headSet, (m_server->Generation == ServerGeneration::ORIGINAL ? "gif" : "png"));
				modTime[PROPID(NPCProp::HEADIMAGE)] = m_server->getFrameStartTime();
			}
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"sprite"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter {
			return static_cast<double>(character.sprite); },
		[this](GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
			{
				character.sprite = static_cast<uint8_t>(value->get());
				if (character.sprite >= 4 && m_server->Generation != ServerGeneration::ORIGINAL)
				{
					character.gani = std::format("def[{}]", character.sprite);
					modTime[PROPID(NPCProp::GANI)] = m_server->getFrameStartTime();
				}
			}
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"dir"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter { return static_cast<double>(character.direction); },
		[this](GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
			{
				character.direction = std::clamp(static_cast<uint8_t>(value->get()), 0_ui8, 3_ui8);
				modTime[PROPID(NPCProp::SPRITE)] = m_server->getFrameStartTime();
			}
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"hurtdx"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter { return character.hurtPushDeltaInHalfPixels[0] / 32.0; },
		[this](GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
			{
				auto clampedValue = std::clamp(value->get(), -1.0, 1.0);
				character.hurtPushDeltaInHalfPixels[0] = static_cast<int8_t>(clampedValue * 32);
				modTime[PROPID(NPCProp::HURTDXDY)] = m_server->getFrameStartTime();
			}
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		"hurtdy"sv,
		[this](std::optional<size_t>) -> GameValueVariantForGetter { return character.hurtPushDeltaInHalfPixels[1] / 32.0; },
		[this](GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
			{
				auto clampedValue = std::clamp(value->get(), -1.0, 1.0);
				character.hurtPushDeltaInHalfPixels[1] = static_cast<int8_t>(clampedValue * 32);
				modTime[PROPID(NPCProp::HURTDXDY)] = m_server->getFrameStartTime();
			}
		}
	});
	// clang-format on
}

//----------------------------

void NPC::testForLinks(SetResults& result)
{
	auto levelPtr = getLevel();
	if (levelPtr == nullptr) return;

	// The NPC changed their level and position.
	auto informNPCWarped = [&]()
	{
		// Tell NCs about our new position.
		CString ncPacket = CString() >> (char)PLO_NC_NPCADD >> (int)id >> (char)NPCProp::CURLEVEL << getProp<NPCProp::CURLEVEL>().serialize();
		m_server->sendPacketToType(PLTYPE_ANYNC, ncPacket);

		// Tell players that we changed level.
		auto localPosition = getLocalPosition();
		m_server->sendPacketToType(PLTYPE_ANYPLAYER, CString() >> (char)PLO_NPCMOVED >> (int)id >> (char)(localPosition.x() / 8) >> (char)(localPosition.y() / 8) << getLevelName());
		m_server->sendPacketToNearby(CString() >> (char)PLO_NPCPROPS >> (int)id << getAllPropsPacket(), character.getGlobalPosition(), levelPtr);

		// Add a scripting event for the warp.
		scripting.events.addEvent(ScriptEventType::NPCWARPED, source::FromNPC(id));
	};

	// The NPC only changed their position, not their level.
	auto informNPCOnlyMoved = [&result, this]()
	{
		result.resultPropIds.push_back(PROPID(NPCProp::X));
		result.resultPropIds.push_back(PROPID(NPCProp::Y));
		result.resultPropIds.push_back(PROPID(NPCProp::X2));
		result.resultPropIds.push_back(PROPID(NPCProp::Y2));
	};

	// Gmaps are treated as one large map, and so level npcs can freely walk across maps (source: post=1193766)
	uint8_t computedMapX = character.localPixelX / 1024;
	uint8_t computedMapY = character.localPixelY / 1024;
	uint8_t computedLocalX = character.localPixelX % 1024;
	uint8_t computedLocalY = character.localPixelY % 1024;

	// Overworld links.
	// We test the NPC's x/y position to see if they walked out of the bounds of the current level.
	// If they did, we warp them to the new level, if allowed.
	const auto& map = levelPtr->getMap();
	if (map != nullptr && (computedMapX != character.mapX || computedMapY != character.mapY))
	{
		auto newLevelName = map->getLevelNameAt(computedMapX, computedMapY);
		if (warpRestrictions != NPCWarpRestrictions::NOTALLOWED)
		{
			character.mapX = map->isGmap() ? computedMapX : 0;
			character.mapY = map->isGmap() ? computedMapY : 0;
			result.resultPropIds.push_back(PROPID(NPCProp::GMAPLEVELX));
			result.resultPropIds.push_back(PROPID(NPCProp::GMAPLEVELY));

			character.localPixelX = computedLocalX;
			character.localPixelY = computedLocalY;

			if (levelPtr->isOnBigMap())
			{
				if (auto newLevel = m_server->getLoadedLevel(newLevelName, levelPtr); newLevel != nullptr)
				{
					setLevel(newLevel);
					result.resultPropIds.push_back(PROPID(NPCProp::CURLEVEL));
					informNPCWarped();
				}
			}
			else informNPCOnlyMoved();
			return;
		}

		// They aren't allowed to leave the level, so clamp them to the borders.
		character.localPixelX = std::clamp(character.localPixelX, static_cast<int16_t>(0), static_cast<int16_t>(61 * 16));
		character.localPixelY = std::clamp(character.localPixelY, static_cast<int16_t>(0), static_cast<int16_t>(61 * 16));
		informNPCOnlyMoved();
		return;
	}

	if (warpRestrictions == NPCWarpRestrictions::ALLOWED)
	{
		static Position<int> touchTest[] = {{2, 1}, {0, 2}, {2, 4}, {3, 2}};
		TilePosition testPos = character.getTilePosition().translate(touchTest[character.direction].x(), touchTest[character.direction].y());
		if (auto linkTouched = levelPtr->getLink(testPos, map != nullptr); linkTouched.has_value())
		{
			auto& destLevelName = linkTouched.value()->getDestinationLevel();
			SubLevelPtr destSubLevel = levelPtr->getSubLevelByName(destLevelName);
			LevelPtr newLevel = nullptr;

			// Destination level was not found on the map, so check the server for the level.
			if (destSubLevel == nullptr)
			{
				if (auto newLevel = m_server->getLoadedLevel(destLevelName, levelPtr); newLevel != nullptr)
				{
					destSubLevel = newLevel->getSubLevelByName(destLevelName);
					setLevel(newLevel);
				}
			}

			// If we have a destination level, move us to it.
			if (destSubLevel != nullptr)
			{
				auto mapPosition = destSubLevel->mapPosition.value_or(MapPosition{0, 0});
				character.mapX = mapPosition.x();
				character.mapY = mapPosition.y();
				result.resultPropIds.push_back(PROPID(NPCProp::GMAPLEVELX));
				result.resultPropIds.push_back(PROPID(NPCProp::GMAPLEVELY));

				auto pos = linkTouched.value()->getDestinationForCharacter(character, source::FromNPC(id));
				character.localPixelX = pos.x();
				character.localPixelY = pos.y();

				// If we are changing levels, do that now.
				if (newLevel != nullptr)
				{
					setLevel(newLevel);
					result.resultPropIds.push_back(PROPID(NPCProp::CURLEVEL));
					informNPCWarped();
				}
				else informNPCOnlyMoved();
			}
		}
	}
}

void NPC::testForTouch(SetResults& result)
{
	if (m_currentLevel.expired() || !m_server->hasNPCServer())
		return;

	testForLinks(result);
}

//----------------------------

std::string_view toWeaponName(std::string_view code)
{
	constexpr size_t notFound = std::string_view::npos;

	size_t name_start = code.find("toweapons");
	if (name_start == notFound)
		return {};

	name_start += 9; // 9 = strlen("toweapons")

	size_t name_end[2] = {code.find(";", name_start), code.find("}", name_start)};
	if (name_end[0] == notFound && name_end[1] == notFound)
		return {};

	size_t name_pos = name_end[0];
	if (name_end[1] != notFound && name_end[1] < name_end[0])
		name_pos = name_end[1];

	if (name_pos == notFound)
		return {};

	return string::trim(code.substr(name_start, name_pos - name_start));
}

std::vector<std::string> NPC::getVariableDump() const
{
	constexpr std::array<std::string_view, NPCPROP_COUNT> propNames =
	{
		"image", "script", "x", "y", "power",
		"rupees", "arrows", "bombs", "glovepower", "bombpower",
		"sword", "shield", "animation", "visibility flags", "blocking flags",
		"message", "hurtdxdy", "id", "sprite", "colors",

		"nickname", "horse", "head", "save[0]", "save[1]",
		"save[2]", "save[3]", "save[4]", "save[5]", "save[6]",
		"save[7]", "save[8]", "save[9]", "alignment", "imagepart",
		"body", "ganiattr1", "ganiattr2", "ganiattr3", "ganiattr4",

		"ganiattr5", "mapx", "mapy", "z", "ganiattr6",
		"ganiattr7", "ganiattr8", "ganiattr9", "UNKNOWN48", "scripter",
		"name", "type", "level", "ganiattr10", "ganiattr11",
		"ganiattr12", "ganiattr13", "ganiattr14", "ganiattr15",

		"ganiattr16", "ganiattr17", "ganiattr18", "ganiattr19", "ganiattr20",
		"ganiattr21", "ganiattr22", "ganiattr23", "ganiattr24", "ganiattr25",
		"ganiattr26", "ganiattr27", "ganiattr28", "ganiattr29", "ganiattr30",
		"joinedclasses", "xprecise", "yprecise", "zprecise"
	};

	constexpr std::array<NPCProp, 58> propSendOrder =
	{
		NPCProp::ID, NPCProp::IMAGE, NPCProp::SCRIPT, NPCProp::CLASS,
		NPCProp::VISFLAGS, NPCProp::BLOCKFLAGS,
		NPCProp::HEADIMAGE, NPCProp::BODYIMAGE, NPCProp::SWORDIMAGE, NPCProp::SHIELDIMAGE,
		NPCProp::NICKNAME, NPCProp::SPRITE, NPCProp::GANI,
		NPCProp::GATTRIB1, NPCProp::GATTRIB2, NPCProp::GATTRIB3, NPCProp::GATTRIB4, NPCProp::GATTRIB5,
		NPCProp::GATTRIB6, NPCProp::GATTRIB7, NPCProp::GATTRIB8, NPCProp::GATTRIB9, NPCProp::GATTRIB10,
		NPCProp::GATTRIB11, NPCProp::GATTRIB12, NPCProp::GATTRIB13, NPCProp::GATTRIB14, NPCProp::GATTRIB15,
		NPCProp::GATTRIB16, NPCProp::GATTRIB17, NPCProp::GATTRIB18, NPCProp::GATTRIB19, NPCProp::GATTRIB20,
		NPCProp::GATTRIB21, NPCProp::GATTRIB22, NPCProp::GATTRIB23, NPCProp::GATTRIB24, NPCProp::GATTRIB25,
		NPCProp::GATTRIB26, NPCProp::GATTRIB27, NPCProp::GATTRIB28, NPCProp::GATTRIB29, NPCProp::GATTRIB30,
		NPCProp::SAVE0, NPCProp::SAVE1, NPCProp::SAVE2, NPCProp::SAVE3, NPCProp::SAVE4,
		NPCProp::SAVE5, NPCProp::SAVE6, NPCProp::SAVE7, NPCProp::SAVE8, NPCProp::SAVE9,
		NPCProp::GMAPLEVELX, NPCProp::GMAPLEVELY, NPCProp::X2, NPCProp::Y2, NPCProp::Z2
	};

	constexpr std::array<NPCProp, 37> characterProps =
	{
		NPCProp::HEADIMAGE, NPCProp::BODYIMAGE, NPCProp::SWORDIMAGE, NPCProp::SHIELDIMAGE,
		NPCProp::NICKNAME, NPCProp::SPRITE, NPCProp::GANI,
		NPCProp::GATTRIB1, NPCProp::GATTRIB2, NPCProp::GATTRIB3, NPCProp::GATTRIB4, NPCProp::GATTRIB5,
		NPCProp::GATTRIB6, NPCProp::GATTRIB7, NPCProp::GATTRIB8, NPCProp::GATTRIB9, NPCProp::GATTRIB10,
		NPCProp::GATTRIB11, NPCProp::GATTRIB12, NPCProp::GATTRIB13, NPCProp::GATTRIB14, NPCProp::GATTRIB15,
		NPCProp::GATTRIB16, NPCProp::GATTRIB17, NPCProp::GATTRIB18, NPCProp::GATTRIB19, NPCProp::GATTRIB20,
		NPCProp::GATTRIB21, NPCProp::GATTRIB22, NPCProp::GATTRIB23, NPCProp::GATTRIB24, NPCProp::GATTRIB25,
		NPCProp::GATTRIB26, NPCProp::GATTRIB27, NPCProp::GATTRIB28, NPCProp::GATTRIB29, NPCProp::GATTRIB30
	};

	std::vector<std::string> result;
	std::string npcname = (!name.empty() ? name : std::format("npcs[{}]", id));

	result.emplace_back(std::format("Variables dump from npc {}", npcname));
	result.emplace_back();
	if (!scriptType.empty())
		result.emplace_back(std::format("{}.type: {}", npcname, scriptType));
	if (!scripter.empty())
		result.emplace_back(std::format("{}.scripter: {}", npcname, scripter));
	result.emplace_back(std::format("{}.level: {}", npcname, level));
	result.emplace_back();
	result.emplace_back("Attributes:");

	std::string nameprop;
	for (const auto& prop : propSendOrder)
	{
		auto propId = PROPID(prop);

		// Don't show character props if the NPC is not a character.
		if (!isCharacter() && std::ranges::contains(characterProps, prop))
			continue;

		// Don't show props that haven't changed.
		if (!modTime[propId].has_value())
			continue;

		nameprop.assign(std::format("{}.{}", npcname, propNames[propId]));
		switch (prop)
		{
			case NPCProp::SCRIPT:
				result.emplace_back(std::format("{}: size: {}", nameprop, m_script.getOriginalSource().length()));
				break;

			case NPCProp::CLASS:
				result.emplace_back(std::format("{}: {}", nameprop, getJoinedClassesList()));
				break;

			case NPCProp::SWORDIMAGE:
			{
				std::string swordImage = character.swordImage;
				if (swordImage.empty() && character.swordPower > 0 && character.swordPower <= 4)
					swordImage = std::format("sword{}.png", character.swordPower);

				result.emplace_back(std::format("{}: {} ({})", nameprop, swordImage, character.swordPower));
				break;
			}

			case NPCProp::SHIELDIMAGE:
			{
				std::string shieldImage = character.shieldImage;
				if (shieldImage.empty() && character.shieldPower > 0 && character.shieldPower <= 3)
					shieldImage = std::format("shield{}.png", character.shieldPower);

				result.emplace_back(std::format("{}: {} ({})", nameprop, shieldImage, character.shieldPower));
				break;
			}

			case NPCProp::VISFLAGS:
			{
				std::string activeVisFlags{(visFlags & PROPID(NPCVisFlags::VISIBLE) ? "visible" : "hidden")};
				if (visFlags & PROPID(NPCVisFlags::DRAWOVERPLAYER))
					activeVisFlags += ", drawoverplayer";
				if (visFlags & PROPID(NPCVisFlags::DRAWUNDERPLAYER))
					activeVisFlags += ", drawunderplayer";
				if (visFlags & PROPID(NPCVisFlags::TIMERSHOW))
					activeVisFlags += ", timershow";
				if (visFlags & PROPID(NPCVisFlags::CREATED))
					activeVisFlags += ", created";
				if (visFlags & PROPID(NPCVisFlags::UNKNOWNBIT6))
					activeVisFlags += ", unknownbit6";
				if (isCharacter())
					activeVisFlags += (visFlags & PROPID(NPCVisFlags::MALE) ? ", male" : ", female");

				result.emplace_back(std::format("{}: {}", nameprop, activeVisFlags));
				break;
			}

			case NPCProp::BLOCKFLAGS:
			{
				std::string activeBlockFlags{(blockFlags & PROPID(NPCBlockFlags::NOBLOCK) ? "noblock" : "block")};
				if (blockFlags & PROPID(NPCBlockFlags::CANBECARRIED))
					activeBlockFlags += ", canbecarried";
				if (blockFlags & PROPID(NPCBlockFlags::CANBEPULLED))
					activeBlockFlags += ", canbepulled";
				if (blockFlags & PROPID(NPCBlockFlags::CANBEPUSHED))
					activeBlockFlags += ", canbepushed";

				result.emplace_back(std::format("{}: {}", nameprop, activeBlockFlags));
				break;
			}

			default:
				result.emplace_back(std::format("{}: {}", nameprop, getProp(prop)));
				break;
		}
	}

	if (timeout != 0ms)
		result.emplace_back(std::format("{}.timeout: {}ms", npcname, timeout.count()));
	//npcDump << npcNameStr << ".scripttime (in the last min): " << CString(executionData.second) << "\n";
	//npcDump << npcNameStr << ".scriptcalls: " << CString(executionData.first) << "\n";

	result.emplace_back();
	result.emplace_back("npc.Flags:");

	for (const auto& [flag, variable] : scripting.variables.store | variables::only_flags)
	{
		if (variable->value.has<bool>() && !variable->value.has<std::string>() && variable->getCopy<bool>().value_or(false))
			result.emplace_back(std::format("{}.flags[{}]: true", npcname, flag));
		else result.emplace_back(std::format("{}.flags[{}]: {}", npcname, flag, variable->getCopy<std::string>().value_or(std::string{})));
	}

	result.emplace_back();
	result.emplace_back("npc.Vars:");

	for (const auto& [flag, variable] : scripting.variables.store | variables::serializable)
	{
		if (variable->value.has<double>())
			result.emplace_back(std::format("{}.vars[{}]: {}", npcname, flag, variable->getCopy<double>().value_or(0.0)));
		if (variable->value.has<std::vector<double>>())
		{
			auto values = variable->get<std::vector<double>>();
			if (values.has_value() && !values.value().get().empty())
			{
				auto valuesAsStrings = values.value().get() | std::views::transform([](const double& val)
				{
					return std::format("{}", val);
				});
				auto valueString = string::join(valuesAsStrings, ", ");
				result.emplace_back(std::format("{}.vars[{}]: {{{}}}", npcname, flag, valueString));
			}
		}
	}

	return result;
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
