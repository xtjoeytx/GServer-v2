#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
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
#include <object/ShowImg.h>
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

	if (server->Generation == ServerGeneration::CLASSIC && PROPID(prop) > PROPID(NPCProp::BODYIMAGE))
		return false;
	if (prop == NPCProp::SCRIPTER || prop == NPCProp::NAME || prop == NPCProp::TYPE)
		return false;
	if (prop == NPCProp::CLASS && (server->Generation == ServerGeneration::CLASSIC || server->Generation == ServerGeneration::NEWMAIN))
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

NPC::NPC(const NPCID id, const NPCStorageType storageType)
	: id(id), storageType(storageType)
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
	if (const auto initialLevel = m_server->getStubbedLevel(m_initialLevel); initialLevel != nullptr)
		warp(initialLevel, character.getGlobalPosition());
}

//----------------------------

bool NPC::warp(const LevelPtr& level, const PixelPosition& position)
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
		warpResults.push_back(setPropWith<NPCProp::LEVEL>(SetBy::SERVER, level->levelName));

	sendPropsFromResults(warpResults);

	// If our initial level is not set, set it now.
	if (m_initialLevel.empty())
	{
		m_initialLevel = level->levelName;
		m_initialCharacter = character;
	}

	return true;
}

void NPC::setLevel(const LevelPtr& level)
{
	if (level == nullptr)
		return;

	// Refresh our mod times.
	refreshModTimes(m_server->getFrameStartTime());

	this->level = level->levelName;
	m_currentLevel = level;
}

//----------------------------

void NPC::addShowImg(const uint8_t index, ShowImg&& showImg)
{
	if (index > 199)
		return;

	m_server->sendPacketToNearby(CString() >> (char)PLO_SHOWIMGNPC >> (int)id >> (char)(index + 10) << showImg.getAllPropsPacket(), getGlobalPosition(), getLevel());

	m_hadShowImgs = true;
	showImgList[index] = std::move(showImg);
}

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

void NPC::sendShowImagesToPlayer(const PlayerPtr& player, std::optional<clock::time_point> modTime) const noexcept
{
	// Only start sending showimg packets when the NPC gains showimgs.
	if (!m_hadShowImgs && showImgList.empty())
		return;

	m_hadShowImgs = true;

	player->sendPacket(getShowImagesPacket(modTime));
}

void NPC::sendAllShowImagesToLevel(std::optional<clock::time_point> modTime) const noexcept
{
	// Only start sending showimg packets when the NPC gains showimgs.
	if (!m_hadShowImgs && showImgList.empty())
		return;

	m_hadShowImgs = true;

	m_server->sendPacketToNearby(getShowImagesPacket(modTime), getGlobalPosition(), getLevel());
}

//----------------------------

void NPC::addMoveToQueue(const LocalPixelPosition& moveDelta, const float durationInSeconds, const uint8_t options)
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
		const auto currentTilePosition = getTilePosition();
		const auto destinationTilePosition = toTilePosition(move.destination);
		const auto distance = std::hypot(destinationTilePosition.x() - currentTilePosition.x(), destinationTilePosition.y() - currentTilePosition.y());
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
				const bool isOnWall = levelPtr->isOnWall2(boundingBox);
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

		const auto durationLeftInSeconds = std::chrono::duration_cast<duration_seconds_double>(move.duration - move.elapsed);
		const auto timeIn50msIncrements = static_cast<uint16_t>(durationLeftInSeconds.count() / 0.05f);

		const auto currentPosition = move.getCurrentPosition();
		const auto dx = static_cast<int16_t>(move.destination.x() - currentPosition.x());
		const auto dy = static_cast<int16_t>(move.destination.y() - currentPosition.y());
		const auto localPosition = toLocalPixelPosition(currentPosition);

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
			const auto posX = static_cast<uint8_t>(localPosition.x() / 8);
			const auto posY = static_cast<uint8_t>(localPosition.y() / 8);
			const auto moveDX = static_cast<int8_t>((dx / 8) + 100);
			const auto moveDY = static_cast<int8_t>((dy / 8) + 100);

			result.first >> (char)posX >> (char)posY;
			result.first >> (char)moveDX >> (char)moveDY;
			result.first >> (short)timeIn50msIncrements;
			result.first >> (char)move.options.to_ulong();
		}
	}

	return result;
}

void NPC::sendMoveQueueToPlayer(const PlayerPtr& player, std::optional<clock::time_point> modTime) const noexcept
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

void NPC::sendMoveQueueToLevel(const LevelPtr& level, std::optional<clock::time_point> modTime) const noexcept
{
	if (moveQueue.empty())
		return;

	sendMoveQueueToLevel(level, getMoveQueuePacketData(modTime));
}

void NPC::sendMoveQueueToLevel(const LevelPtr& level, const std::pair<CString, CString>& queue) const noexcept
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

void NPC::sendMoveQueueUpdatesToLevel(const LevelPtr& level) noexcept
{
	const auto result = getMoveQueuePacketData(lastMoveQueueSentTime);
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
	const auto levelPtr = getLevel();
	if (levelPtr == nullptr || !levelPtr->hasTerrain())
		return character.localPixelZ / 16.0;

	PixelPosition testPosition = character.getGlobalPosition();
	if (isCharacter())
		testPosition.translate(24, 48);

	const auto terrainHeight = levelPtr->getHeightAt(testPosition);
	const auto currentZ = character.localPixelZ / 16.0;
	return std::max(terrainHeight, currentZ);
}

//----------------------------

std::string NPC::getLevelName() const
{
	// If we are a control-NPC, our level constantly changes, so don't rely on our pointer.
	if (scriptType == NPCTYPE_CONTROL)
		return level;

	if (const auto levelPtr = getLevel(); levelPtr != nullptr)
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

void NPC::hurt(const int8_t damageInHalves, const std::optional<ScriptEventType> damageEventType, const std::optional<ScriptObject>& source, const std::optional<CarryObjectType> hitByType)
{
	// Adjust the NPC's HP.
	if (allowServerDamageReactions && isCharacter())
		sendPropsFromResults(setPropWith<NPCProp::HALFHEARTS>(SetBy::SERVER, static_cast<uint8_t>(std::max(0, character.hitpointsInHalves - damageInHalves))));

	// Queue the hurt event.
	if (damageEventType.has_value())
	{
		if (hitByType.has_value())
			scripting.events.addEvent(damageEventType.value(), source.value_or(source::FromServer()), hitByType.value());
		else scripting.events.addEvent(damageEventType.value(), source.value_or(source::FromServer()));
	}
}

void NPC::hurtAndPush(const int8_t damageInHalves, const PixelPosition& pushOrigin, const std::optional<ScriptEventType> damageEventType, const std::optional<ScriptObject>& source, const std::optional<CarryObjectType> hitByType)
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
	hurt(damageInHalves, damageEventType, source, hitByType);
}

//----------------------------

void NPC::executeEvents(ScriptEventQueue& events, const ScriptObject& source) const
{
	if (events.queue().empty())
		return;

	m_script.executeEvents(events, source);

	// Execute classes.
	for (auto& scriptClassPtr : m_joinedClasses | std::views::values)
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
	const auto classes = string::join(m_script.getServerJoinedClasses() | std::views::keys);
	setJoinedClasses(classes);

	const auto clientside = m_script.getClientSide();

	// Check for position update blocking.
	if (m_server->hasNPCServer() || clientside.contains("//#BLOCKPOSITIONUPDATES"))
		m_blockPositionUpdates = true;

	// If we have no npc-server, we support toweapons, so extract the weapon name.
	if (!m_server->hasNPCServer())
		m_weaponName = toWeaponName(clientside);

	// Just a little warning for people who don't know.
	if (m_script.getClientByteCode().empty() && m_script.getClientSide().length() > 0x705F)
		log::printLine(log::server, "WARNING: Clientside script of NPC ({}) exceeds the limit of 28767 bytes.", (!image.empty() ? image : std::to_string(id)));
}

void NPC::setScript(const std::string_view script)
{
	//auto profile = log::Profile(log::server, "NPC::setScript");

	// Set the script.
	setJoinedClasses("");
	m_script = Script{name, script};
	modTime[PROPID(NPCProp::SCRIPT)] = m_server->getFrameStartTime();

	// Check if we have joined classes already (due to a cached script).
	for (const auto& [className, classPtr] : m_script.getServerJoinedClasses())
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
			log::printLine(log::server, "[DEBUG] NPC '{}' auto-joining class '{}' due to cached script.", className, scriptClass->name);
#endif
			m_joinedClasses.emplace_back(handle, scriptClass);
		}
	}

	const auto clientside = m_script.getClientSide();

	// Check for position update blocking.
	if (m_server->hasNPCServer() || clientside.contains("//#BLOCKPOSITIONUPDATES"))
		m_blockPositionUpdates = true;

	// If we have no npc-server, we support toweapons, so extract the weapon name.
	if (!m_server->hasNPCServer())
		m_weaponName = toWeaponName(clientside);

	// Just a little warning for people who don't know.
	if (m_script.getClientByteCode().empty() && m_script.getClientSide().length() > 0x705F)
		log::printLine(log::server, "WARNING: Clientside script of NPC ({}) exceeds the limit of 28767 bytes.", (!image.empty() ? image : std::to_string(id)));
}

std::string NPC::getClientSideScript() const
{
	std::string result{m_script.getClientSide()};
	for (const auto& classPtr : m_joinedClasses | std::views::values)
	{
		if (auto scriptClass = classPtr.lock(); scriptClass != nullptr)
		{
			if (const auto& clientSide = scriptClass->getScript().getClientSide(); !clientSide.empty())
			{
				result += '\xa7';
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
	for (const auto& classPtr : m_joinedClasses | std::views::values)
	{
		if (auto scriptClass = classPtr.lock(); scriptClass != nullptr)
		{
			result += scriptClass->name;
			result += ',';
		}
		else hasExpired = true;
	}
	if (!result.empty())
		result.pop_back();

	// If we have expired, clear them out.
	if (hasExpired)
	{
		std::erase_if(m_joinedClasses, [](const decltype(m_joinedClasses)::value_type& pair)
		{
			return pair.second.expired();
		});
	}

	return result;
}

bool NPC::hasJoinedClass(const std::string_view className) const
{
	// NOLINTNEXTLINE(*-use-anyofallof)
	for (const auto& classPtr : m_joinedClasses | std::views::values)
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
		if (const auto scriptClass = m_server->getNPCServer()->getClass(className); scriptClass != nullptr)
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
	const auto it = std::ranges::find_if(m_joinedClasses, [&className](const decltype(m_joinedClasses)::value_type& kvp)
	{
		return kvp.second.lock()->name == className;
	});
	if (it != m_joinedClasses.end())
		return;

	if (!m_server->hasNPCServer())
		return;

	if (const auto scriptClass = m_server->getNPCServer()->getClass(className); scriptClass != nullptr)
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
	const auto it = std::ranges::find_if(m_joinedClasses, [&className](const decltype(m_joinedClasses)::value_type& kvp)
	{
		return kvp.second.lock()->name == className;
	});
	if (it == m_joinedClasses.end())
		return;

	if (!m_server->hasNPCServer())
		return;

	bool sendToLevel = false;
	if (const auto scriptClass = it->second.lock(); scriptClass != nullptr)
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

void NPC::sendScriptUpdatesToLevel(const clock::time_point when) const
{
	if (const auto npclevel = getLevel(); npclevel != nullptr)
	{
		if (const auto levelData = npclevel->getStaticLevelDataAtPosition(character.getMapPosition()); levelData != nullptr)
		{
			const auto& levelName = npclevel->levelName;

			const CString packet = CString() >> (char)PLO_NPCDEL2 >> (char)levelName.length() << levelName >> (int)id;
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

std::shared_ptr<PropertyBase> NPC::constructPropFor(const NPCProp prop)
{
	switch (prop)
	{
#define GENERATE_CONSTRUCTPROPFOR_CASE(prop, type, ...) \
	case prop: return std::make_shared<type>();
		FOR_LIST_OF_NPC_PROPS(GENERATE_CONSTRUCTPROPFOR_CASE);
		default:;
	}
	throw std::invalid_argument("Invalid NPCProp type in constructPropFor");
}

//----------------------------

std::shared_ptr<PropertyBase> NPC::getProp(const NPCProp prop) const
{
	switch (prop)
	{
#define GENERATE_GETPROP_CASE(prop, type, ...) \
	case prop: return std::make_shared<type>(__VA_ARGS__);
		FOR_LIST_OF_NPC_PROPS(GENERATE_GETPROP_CASE);
		default:;
	}

	throw std::invalid_argument("Invalid NPCProp type in getProp");
}

//----------------------------

SetResults NPC::setProp(const NPCProp prop, const SetBy setBy, const std::shared_ptr<PropertyBase>& base)
{
	if (PropertyBase* basePtr = base.get(); basePtr != nullptr)
		return setProp(prop, setBy, basePtr);
	throw std::invalid_argument("setProp called with nullptr base pointer.");
}

SetResults NPC::setProp(const NPCProp prop, const SetBy setBy, PropertyBase* base)
{
	auto levelPtr = getLevel();
	bool canUpdatePosition = !m_blockPositionUpdates || setBy == props::SetBy::SERVER;

	props::SetResults result{.propId = PROPID(prop)};
	result.resultFlags.set(props::SetResults::sendToLevel, true);
	result.resultFlags.set(props::SetResults::sendToSource, false);

	const auto& curTime = m_server->getFrameStartTime();
	auto oldTime = modTime[PROPID(prop)];
	auto oldLastUpdateTime = lastUpdateTime;

	modTime[PROPID(prop)] = curTime;
	lastUpdateTime = curTime;

	// Ignores a property change.
#define SETPROP_RETURN_IGNORE                           \
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
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr || strProp->value == image)
				SETPROP_RETURN_IGNORE;

			// If we are changing to a character, set the gani to idle.
			if (strProp->value == "#c#" && image != "#c")
			{
				visFlags |= PROPID(NPCVisFlags::MALE);
				if (m_server->Generation != ServerGeneration::CLASSIC)
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
				visFlags &= ~static_cast<uint8_t>(NPCVisFlags::VISIBLE);
			else
				visFlags |= static_cast<uint8_t>(NPCVisFlags::VISIBLE);

			// If we had a visibility change, send it.
			if (visFlags != oldVisFlags)
				result.resultPropIds.push_back(PROPID(NPCProp::VISFLAGS));
			break;
		}

		case NPCProp::SCRIPT:
		{
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr || setBy != SetBy::SERVER)
				SETPROP_RETURN_IGNORE;

			setScript(strProp->value);
			break;
		}

		case NPCProp::X:
		{
			auto coordProp = dynamic_cast<PropertyTileCoordinate*>(base);
			if (coordProp == nullptr || !canUpdatePosition || coordProp->pixelCoordinate == character.localPixelX)
				SETPROP_RETURN_IGNORE;

			character.localPixelX = coordProp->pixelCoordinate;
			result.resultPropIds.push_back(PROPID(NPCProp::X2));

			// Do collision testing.
			testForTouch(result);
			break;
		}

		case NPCProp::Y:
		{
			auto coordProp = dynamic_cast<PropertyTileCoordinate*>(base);
			if (coordProp == nullptr || !canUpdatePosition || coordProp->pixelCoordinate == character.localPixelY)
				SETPROP_RETURN_IGNORE;

			character.localPixelY = coordProp->pixelCoordinate;
			result.resultPropIds.push_back(PROPID(NPCProp::Y2));

			// Do collision testing.
			testForTouch(result);
			break;
		}

		case NPCProp::Z:
		{
			auto zProp = dynamic_cast<PropertyTileCoordinateZ*>(base);
			if (zProp == nullptr || !canUpdatePosition || zProp->pixelCoordinate == character.localPixelZ)
				SETPROP_RETURN_IGNORE;

			character.localPixelZ = zProp->pixelCoordinate;
			result.resultPropIds.push_back(PROPID(NPCProp::Z2));

			// No collision testing for Z movement.
			break;
		}

		case NPCProp::HALFHEARTS:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr || numProp->value == character.hitpointsInHalves)
				SETPROP_RETURN_IGNORE;

			character.hurtDeltaInHalves = character.hitpointsInHalves - numProp->value;
			character.hitpointsInHalves = numProp->value;

			if (character.hurtDeltaInHalves != 0)
				character.lastHurtTime = curTime;
			break;
		}

		case NPCProp::GRALATS:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE3>*>(base);
			if (numProp == nullptr || numProp->value == character.gralats)
				SETPROP_RETURN_IGNORE;

			character.gralats = numProp->value;
			break;
		}

		case NPCProp::ARROWS:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr || numProp->value == character.arrows)
				SETPROP_RETURN_IGNORE;

			character.arrows = props::Limits::apply(numProp->value, props::Limits::MaxArrows);
			break;
		}

		case NPCProp::BOMBS:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr || numProp->value == character.bombs)
				SETPROP_RETURN_IGNORE;

			character.bombs = props::Limits::apply(numProp->value, props::Limits::MaxBombs);
			break;
		}

		case NPCProp::GLOVEPOWER:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr || numProp->value == character.glovePower)
				SETPROP_RETURN_IGNORE;

			character.glovePower = props::Limits::apply(numProp->value, props::Limits::MaxNPCGlovePower);
			break;
		}

		case NPCProp::BOMBPOWER:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr || numProp->value == character.bombPower)
				SETPROP_RETURN_IGNORE;

			character.bombPower = props::Limits::apply(numProp->value, props::Limits::MaxBombPower);
			break;
		}

		case NPCProp::SWORDIMAGE:
		{
			auto swordProp = dynamic_cast<PropertySwordPower*>(base);
			if (swordProp == nullptr || (swordProp->power.value_or(0) == character.swordPower && swordProp->image == character.swordImage))
				SETPROP_RETURN_IGNORE;

			if (swordProp->power.has_value())
				character.swordPower = props::Limits::applySwordPower(swordProp->power.value_or(1));

			character.swordImage = props::Limits::apply(swordProp->image, props::Limits::SwordImageLength);
			break;
		}

		case NPCProp::SHIELDIMAGE:
		{
			auto shieldProp = dynamic_cast<PropertyShieldPower*>(base);
			if (shieldProp == nullptr || (shieldProp->power.value_or(0) == character.shieldPower && shieldProp->image == character.shieldImage))
				SETPROP_RETURN_IGNORE;

			if (shieldProp->power.has_value())
				character.shieldPower = props::Limits::applyShieldPower(shieldProp->power.value_or(1));

			character.shieldImage = props::Limits::apply(shieldProp->image, props::Limits::ShieldImageLength);
			break;
		}

		case NPCProp::GANI:
		{
			auto ganiProp = dynamic_cast<PropertyGaniOrBowGif*>(base);
			if (ganiProp == nullptr)
				SETPROP_RETURN_IGNORE;

			// 1.x servers didn't have ganis.  This prop was used for the bow instead.
			if (m_server->Generation == ServerGeneration::CLASSIC)
			{
				if (!ganiProp->bowGif.has_value())
					SETPROP_RETURN_IGNORE;

				auto& [image, power] = ganiProp->bowGif.value();
				if (image.contains('.'))
					image += ".gif";

				if (image == character.bowImage || power == character.bowPower)
					SETPROP_RETURN_IGNORE;

				character.bowPower = props::Limits::apply(power, props::Limits::MaxBowPower);
				character.bowImage = image;
				break;
			}

			// Set the gani.
			// Ganis can be set to themselves to play it again from the beginning, so don't check for equality here.
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
				float tX = (static_cast<float>(character.localPixelX) / 16.0f) + 1.5f;
				float tY = (static_cast<float>(character.localPixelY) / 16.0f) + 2.0f;
				m_server->hitObjectsAtPoint({tX, tY + 2.0f}, character.swordPower, m_currentLevel, self);
				m_server->hitObjectsAtPoint({tX, tY - 2.0f}, character.swordPower, m_currentLevel, self);
				m_server->hitObjectsAtPoint({tX + 2.0f, tY}, character.swordPower, m_currentLevel, self);
				m_server->hitObjectsAtPoint({tX - 2.0f, tY}, character.swordPower, m_currentLevel, self);
			}
			break;
		}

		case NPCProp::VISFLAGS:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr || numProp->value == visFlags)
				SETPROP_RETURN_IGNORE;

			visFlags = numProp->value;
			break;
		}

		case NPCProp::BLOCKFLAGS:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr || numProp->value == blockFlags)
				SETPROP_RETURN_IGNORE;

			blockFlags = numProp->value;
			break;
		}

		case NPCProp::MESSAGE:
		{
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr || strProp->value == character.chatMessage)
				SETPROP_RETURN_IGNORE;

			character.chatMessage = strProp->value;
			break;
		}

		case NPCProp::HURTDXDY:
		{
			auto hurtProp = dynamic_cast<PropertyHurtDxDy<>*>(base);
			if (hurtProp == nullptr || (hurtProp->hurtDX == character.hurtPushDeltaInHalfPixels[0] && hurtProp->hurtDY == character.hurtPushDeltaInHalfPixels[1]))
				SETPROP_RETURN_IGNORE;

			character.hurtPushDeltaInHalfPixels[0] = hurtProp->hurtDX;
			character.hurtPushDeltaInHalfPixels[1] = hurtProp->hurtDY;
			break;
		}

		case NPCProp::ID:
			break;

		case NPCProp::SPRITE:
		{
			auto spriteProp = dynamic_cast<PropertySprite*>(base);
			if (spriteProp == nullptr || spriteProp->sprite == character.sprite)
				SETPROP_RETURN_IGNORE;

			character.direction = spriteProp->direction;
			character.sprite = spriteProp->sprite;
			result.resultFlags.set(SetResults::getLatestOnSend);

			// If we manually set a sprite, change the gani.
			if (m_server->Generation != ServerGeneration::CLASSIC && character.sprite != 0)
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
			auto colorProp = dynamic_cast<PropertyColors*>(base);
			if (colorProp == nullptr || colorProp->values == character.colors)
				SETPROP_RETURN_IGNORE;

			character.colors = colorProp->values;
			break;
		}

		case NPCProp::NICKNAME:
		{
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr || strProp->value == character.nickName)
				SETPROP_RETURN_IGNORE;

			character.nickName = strProp->value;
			break;
		}

		case NPCProp::HORSEIMAGE:
		{
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_IGNORE;

			std::string horseImage = strProp->value;
			if (m_server->Generation == ServerGeneration::CLASSIC && !horseImage.empty() && !horseImage.contains('.'))
				horseImage += ".gif";

			if (horseImage == character.horseImage)
				SETPROP_RETURN_IGNORE;

			character.horseImage = horseImage;
			break;
		}

		case NPCProp::HEADIMAGE:
		{
			auto headProp = dynamic_cast<PropertyHeadGif*>(base);
			if (headProp == nullptr)
				SETPROP_RETURN_IGNORE;

			std::string img;
			if (std::holds_alternative<uint8_t>(headProp->image))
				img = std::format("head{}.{}", std::get<uint8_t>(headProp->image), (m_server->Generation != ServerGeneration::CLASSIC ? "png" : "gif"));
			else
				img = std::get<std::string>(headProp->image);

			if (m_server->Generation == ServerGeneration::CLASSIC && !img.empty() && !img.contains('.'))
				img += ".gif";

			if (img == character.headImage)
				SETPROP_RETURN_IGNORE;

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
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_IGNORE;

			int index = PROPID(prop) - PROPID(NPCProp::SAVE0);
			if (index < 0 || index >= static_cast<int>(saves.size()))
				SETPROP_RETURN_IGNORE;
			if (numProp->value == saves[index])
				SETPROP_RETURN_IGNORE;

			saves[index] = numProp->value;
			break;
		}

		case NPCProp::ALIGNMENT:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr || numProp->value == character.ap)
				SETPROP_RETURN_IGNORE;

			character.ap = numProp->value;
			break;
		}

		case NPCProp::IMAGEPART:
		{
			auto imgPartProp = dynamic_cast<PropertyImagePart*>(base);
			if (imgPartProp == nullptr || (imgPartProp->imagePart.position == imagePart.position && imgPartProp->imagePart.size == imagePart.size))
				SETPROP_RETURN_IGNORE;

			imagePart = imgPartProp->imagePart;
			break;
		}

		case NPCProp::BODYIMAGE:
		{
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr || strProp->value == character.bodyImage)
				SETPROP_RETURN_IGNORE;

			character.bodyImage = strProp->value;
			break;
		}

		case NPCProp::GMAPLEVELX:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr || numProp->value == character.mapX)
				SETPROP_RETURN_IGNORE;

			character.mapX = numProp->value;
			break;
		}

		case NPCProp::GMAPLEVELY:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr || numProp->value == character.mapY)
				SETPROP_RETURN_IGNORE;

			character.mapY = numProp->value;
			break;
		}

		case NPCProp::UNKNOWN48:
			break;

		case NPCProp::SCRIPTER:
		{
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_IGNORE;

			scripter = strProp->value;
			break;
		}

		case NPCProp::NAME:
		{
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_IGNORE;

			name = strProp->value;
			break;
		}

		case NPCProp::TYPE:
		{
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_IGNORE;

			scriptType = strProp->value;
			break;
		}

		case NPCProp::LEVEL:
		{
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr || !canUpdatePosition)
				SETPROP_RETURN_IGNORE;

			// No change?  Don't do anything.
			if (level == strProp->value)
				SETPROP_RETURN_IGNORE;

			// See if the level exists.
			auto newLevel = m_server->getLoadedLevel(strProp->value, levelPtr);
			if (newLevel == nullptr)
				SETPROP_RETURN_IGNORE;

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
			CString ncPacket = CString() >> (char)PLO_NC_NPCADD >> (int)id >> (char)NPCProp::LEVEL << getProp<NPCProp::LEVEL>().serialize();
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
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_IGNORE;

			auto index = std::ranges::distance(NPCGaniAttrPackets.begin(), std::ranges::find(NPCGaniAttrPackets, PROPID(prop)));
			if (index < 0 || index >= static_cast<std::ptrdiff_t>(character.ganiAttributes.size()))
				SETPROP_RETURN_IGNORE;
			if (strProp->value == character.ganiAttributes[index])
				SETPROP_RETURN_IGNORE;

			character.ganiAttributes[index] = strProp->value;
			break;
		}

		case NPCProp::CLASS:
		{
			auto strProp = dynamic_cast<PropertyLongString*>(base);
			if (strProp == nullptr || strProp->value == getJoinedClassesList())
				SETPROP_RETURN_IGNORE;

			setJoinedClasses(strProp->value);
			break;
		}

		case NPCProp::X2:
		{
			auto pixelProp = dynamic_cast<PropertyPixelCoordinate*>(base);
			if (pixelProp == nullptr || !canUpdatePosition || pixelProp->pixelCoordinate == character.localPixelX)
				SETPROP_RETURN_IGNORE;

			character.localPixelX = pixelProp->pixelCoordinate;
			result.resultPropIds.push_back(PROPID(NPCProp::X));

			// Do collision testing.
			testForTouch(result);
			break;
		}

		case NPCProp::Y2:
		{
			auto pixelProp = dynamic_cast<PropertyPixelCoordinate*>(base);
			if (pixelProp == nullptr || !canUpdatePosition || pixelProp->pixelCoordinate == character.localPixelY)
				SETPROP_RETURN_IGNORE;

			character.localPixelY = pixelProp->pixelCoordinate;
			result.resultPropIds.push_back(PROPID(NPCProp::Y));

			// Do collision testing.
			testForTouch(result);
			break;
		}

		case NPCProp::Z2:
		{
			auto pixelProp = dynamic_cast<PropertyPixelCoordinate*>(base);
			if (pixelProp == nullptr || !canUpdatePosition || pixelProp->pixelCoordinate == character.localPixelZ)
				SETPROP_RETURN_IGNORE;

			character.localPixelZ = pixelProp->pixelCoordinate;
			result.resultPropIds.push_back(PROPID(NPCProp::Z));

			// Do collision testing.
			testForTouch(result);
			break;
		}

		default:;
	}

	// If we are sending other ids, we need to update the mod time for them too.
	if (!result.resultPropIds.empty() && !result.resultFlags.test(SetResults::wasInvalid))
	{
		for (const auto& pid : result.resultPropIds)
			modTime[pid] = curTime;
	}

	return result;
}

//----------------------------

void NPC::sendPropsFromSendResults(PropertySendResults& results, const PlayerPtr& source) const
{
	CString sendAll, sendLevel, sendSource;

	std::erase_if(results, [](const PropertySendResults::value_type& res)
	{
		return !canSendProp(static_cast<NPCProp>(res.first.propId));
	});

	collectPacketsFromResults(results, sendAll, sendLevel, sendSource, [this](uint8_t propId, SetResults::ResultFlagType& destinations)
	{
		return this->getProp(static_cast<NPCProp>(propId));
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

void NPC::setPropsFromPacket(CString& packet, const PlayerPtr& source)
{
	try
	{
		DO_PACKETLOG(log::printBlock(log::networkdump, "NPC::setPropsFromPacket:\n"));

		PropertySendResults results;
		const auto setBy = (source != nullptr ? SetBy::CLIENT : SetBy::SERVER);

		while (packet.bytesLeft() > 0)
		{
			const auto propId = static_cast<NPCProp>(packet.readGUChar());

			DO_PACKETLOG(size_t oldPos = packet.readPos());

			auto prop = constructPropFor(propId);
			prop->deserialize(packet);

#ifdef PACKETLOGGING
			const size_t currentPos = packet.readPos();
			CString rawData = packet.subString(static_cast<int>(oldPos), static_cast<int>(currentPos - oldPos));

			log::printBlock(log::networkdump, "  {}: {} |", npcPropNames[PROPID(propId)], prop);
			for (int i = 0; i < rawData.length(); ++i)
			{
				log::printBlock(log::networkdump, " {:02x}", static_cast<unsigned char>(rawData[i]));
			}
			log::printBlock(log::networkdump, "\n");
#endif

			results.emplace_back(setProp(propId, setBy, prop), prop);
		}
		DO_PACKETLOG(log::print(log::networkdump, "\n"));

		sendPropsFromSendResults(results, source);
	}
	catch (const std::exception& e)
	{
		DO_PACKETLOG(log::printLine(log::networkdump, "\nError in NPC::setPropsFromPacket: {}", e.what()));
		DO_PACKETLOG(log::print(log::networkdump, "\n"));

		log::printLine(log::server, "** Error in NPC::setPropsFromPacket: {}", e.what());
		if (source != nullptr)
			source->disconnect("Corrupted packet received.");
	}
}

//----------------------------

CString NPC::getModifiedPropsPacket() const
{
	DO_PACKETLOG(bool printedHeader = false);

	CString result;
	for (auto i = 0; i < NPCPROP_COUNT; ++i)
	{
		if (!canSendProp(static_cast<NPCProp>(i)))
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
				auto prop = getProp(static_cast<NPCProp>(i));
				CString data = prop->serialize();

				log::printBlock(log::networkdump, "  {}: {}", npcPropNames[i], prop);
				if (static_cast<NPCProp>(i) != NPCProp::SCRIPT)
				{
					log::printBlock(log::networkdump, " |");
					for (int j = 0; j < data.length(); ++j)
						log::printBlock(log::networkdump, " {:02x}", static_cast<unsigned char>(data[j]));
				}
				log::printBlock(log::networkdump, "\n");

				result >> (char)i << data;
#else
				result >> (char)i << getProp(static_cast<NPCProp>(i))->serialize();
#endif
			}
		}
	}

	DO_PACKETLOG(if (printedHeader) log::print(log::networkdump, "\n"));
	return result;
}

CString NPC::getAllPropsPacket(const std::optional<clock::time_point> newTime) const
{
	DO_PACKETLOG(log::printBlock(log::networkdump, "NPC::getAllPropsPacket:\n"));

	CString retVal;
	constexpr auto pmax = NPCPROP_COUNT;

	for (int i = 0; i < pmax; i++)
	{
		if (!canSendProp(static_cast<NPCProp>(i)))
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
				auto prop = getProp(static_cast<NPCProp>(i));
				CString data = prop->serialize();

				log::printBlock(log::networkdump, "  {}: {}", npcPropNames[i], prop);
				if (static_cast<NPCProp>(i) != NPCProp::SCRIPT)
				{
					log::printBlock(log::networkdump, " |");
					for (int j = 0; j < data.length(); ++j)
						log::printBlock(log::networkdump, " {:02x}", static_cast<unsigned char>(data[j]));
				}
				log::printBlock(log::networkdump, "\n");

				retVal >> (char)i << data;
#else
				retVal >> (char)i << getProp(static_cast<NPCProp>(i))->serialize();
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
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{.name = "id"sv, .modTime = std::ref(modTime[PROPID(NPCProp::ID)]), .value = std::ref(id)});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::IntegralProperty{.name = "hurtdpower"sv, .modTime = std::nullopt, .value = std::ref(character.hurtDeltaInHalves)});

	bind::bindPropertyAsReadOnly(scriptParameters, bind::ManuallyDefinedProperty<double>{
		.name = "width"sv, .getter = [this](std::optional<size_t>) -> GameValueVariantForGetter { return getComputedShape().width() / 16.0; }
	});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::ManuallyDefinedProperty<double>{
		.name = "height"sv, .getter = [this](std::optional<size_t>) -> GameValueVariantForGetter { return getComputedShape().height() / 16.0; }
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::DivideByIntegralProperty{.name = "z"sv, .modTime = std::ref(modTime[PROPID(NPCProp::Z2)]), .value = std::ref(character.localPixelZ), .factor = 16});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::DivideByIntegralProperty{.name = "hearts"sv, .modTime = std::ref(modTime[PROPID(NPCProp::HALFHEARTS)]), .value = std::ref(character.hitpointsInHalves), .factor = 2});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::DivideByIntegralProperty{.name = "hp"sv, .modTime = std::ref(modTime[PROPID(NPCProp::HALFHEARTS)]), .value = std::ref(character.hitpointsInHalves), .factor = 2});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{.name = "ap"sv, .modTime = std::ref(modTime[PROPID(NPCProp::ALIGNMENT)]), .value = std::ref(character.ap)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{.name = "rupees"sv, .modTime = std::ref(modTime[PROPID(NPCProp::GRALATS)]), .value = std::ref(character.gralats)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{.name = "gralats"sv, .modTime = std::ref(modTime[PROPID(NPCProp::GRALATS)]), .value = std::ref(character.gralats)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{.name = "bombs"sv, .modTime = std::ref(modTime[PROPID(NPCProp::BOMBS)]), .value = std::ref(character.bombs)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{.name = "darts"sv, .modTime = std::ref(modTime[PROPID(NPCProp::ARROWS)]), .value = std::ref(character.arrows)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{.name = "glovepower"sv, .modTime = std::ref(modTime[PROPID(NPCProp::GLOVEPOWER)]), .value = std::ref(character.glovePower)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{.name = "swordpower"sv, .modTime = std::ref(modTime[PROPID(NPCProp::SWORDIMAGE)]), .value = std::ref(character.swordPower)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{.name = "shieldpower"sv, .modTime = std::ref(modTime[PROPID(NPCProp::SHIELDIMAGE)]), .value = std::ref(character.shieldPower)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::TimeoutProperty{.name = "timeout"sv, .value = std::ref(timeout)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralArrayProperty{.name = "save"sv, .modTime = std::ref(modTime), .modTimeIndex0 = PROPID(NPCProp::SAVE0), .value = std::ref(saves)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{.name = "#1"sv, .modTime = std::ref(modTime[PROPID(NPCProp::SWORDIMAGE)]), .value = std::ref(character.swordImage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{.name = "#2"sv, .modTime = std::ref(modTime[PROPID(NPCProp::SHIELDIMAGE)]), .value = std::ref(character.shieldImage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{.name = "#3"sv, .modTime = std::ref(modTime[PROPID(NPCProp::HEADIMAGE)]), .value = std::ref(character.headImage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{.name = "#5"sv, .modTime = std::ref(modTime[PROPID(NPCProp::HORSEIMAGE)]), .value = std::ref(character.horseImage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{.name = "#7"sv, .modTime = std::ref(modTime[PROPID(NPCProp::GANI)]), .value = std::ref(character.bowImage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{.name = "#8"sv, .modTime = std::ref(modTime[PROPID(NPCProp::BODYIMAGE)]), .value = std::ref(character.bodyImage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{.name = "#c"sv, .modTime = std::ref(modTime[PROPID(NPCProp::MESSAGE)]), .value = std::ref(character.chatMessage)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{.name = "#m"sv, .modTime = std::ref(modTime[PROPID(NPCProp::GANI)]), .value = std::ref(character.gani)});
	bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{.name = "#n"sv, .modTime = std::ref(modTime[PROPID(NPCProp::NICKNAME)]), .value = std::ref(character.nickName)});

	// colors
	const size_t colorCount = m_server->isNewWorldMode() ? 8 : 5;
	for (size_t i = 0; i < colorCount; ++i)
		bind::bindPropertyAsReadWrite(scriptParameters, bind::IntegralProperty{.name = colorPropertyNames[i], .modTime = std::ref(modTime[PROPID(NPCProp::COLORS)]), .value = std::ref(character.colors[i])});

	// gani attributes
	for (size_t i = 0; i < 30; ++i)
		bind::bindPropertyAsReadWrite(scriptParameters, bind::StringProperty{.name = ganiAttributePropertyNames[i], .modTime = std::ref(modTime[NPCGaniAttrPackets[i]]), .value = std::ref(character.ganiAttributes[i])});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		.name = "x"sv,
		.getter = [this](std::optional<size_t>) -> GameValueVariantForGetter { return (double)character.getTilePosition().x(); },
		.setter = [this](const GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (const auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
			{
				auto globalPosition = character.getGlobalPosition();
				globalPosition.x() = static_cast<int32_t>(value->get() * 16);
				character.localPixelX = toLocalPixelPosition(globalPosition).x();
				moveQueue.clear();

				// Update the location props.
				auto now = m_server->getFrameStartTime();
				modTime[PROPID(NPCProp::X)] = now;
				modTime[PROPID(NPCProp::X2)] = now;

				// Fix the map position if applicable.
				if (const auto levelPtr = getLevel(); levelPtr != nullptr && levelPtr->isGmap())
				{
					if (const auto mapX = toMapPosition(globalPosition).x(); mapX != character.mapX)
					{
						character.mapX = mapX;
						modTime[PROPID(NPCProp::GMAPLEVELX)] = now;
					}
				}
			}
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		.name = "y"sv,
		.getter = [this](std::optional<size_t>) -> GameValueVariantForGetter { return character.getTilePosition().y(); },
		.setter = [this](const GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (const auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
			{
				auto globalPosition = character.getGlobalPosition();
				globalPosition.y() = static_cast<int32_t>(value->get() * 16);
				character.localPixelY = toLocalPixelPosition(globalPosition).y();
				moveQueue.clear();

				// Update the location props.
				auto now = m_server->getFrameStartTime();
				modTime[PROPID(NPCProp::Y)] = now;
				modTime[PROPID(NPCProp::Y2)] = now;

				// Fix the map position if applicable.
				if (const auto levelPtr = getLevel(); levelPtr != nullptr && levelPtr->isGmap())
				{
					if (const auto mapY = toMapPosition(globalPosition).y(); mapY != character.mapY)
					{
						character.mapY = mapY;
						modTime[PROPID(NPCProp::GMAPLEVELY)] = now;
					}
				}
			}
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		.name = "headset"sv,
		.getter = [this](std::optional<size_t>) -> GameValueVariantForGetter
		{
			int headSet = -1;
			if (character.headImage.starts_with("head"))
				string::toNumber(character.headImage.substr(4), headSet);
			return static_cast<double>(headSet);
		},
		.setter = [this](const GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			static double noHeadSet = -1.0;
			static auto noHeadRef = std::ref(noHeadSet);
			auto value = std::get_if<std::reference_wrapper<double>>(&incoming);
			if (value == nullptr)
				value = &noHeadRef;

			const auto headSet = std::clamp(static_cast<int>(value->get()), -1, 99);
			if (headSet != -1)
			{
				character.headImage = std::format("head{}.{}", headSet, (m_server->Generation == ServerGeneration::CLASSIC ? "gif" : "png"));
				modTime[PROPID(NPCProp::HEADIMAGE)] = m_server->getFrameStartTime();
			}
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		.name = "sprite"sv,
		.getter = [this](std::optional<size_t>) -> GameValueVariantForGetter {
			return static_cast<double>(character.sprite); },
		.setter = [this](const GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (const auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
			{
				character.sprite = static_cast<uint8_t>(value->get());
				if (character.sprite >= 4 && m_server->Generation != ServerGeneration::CLASSIC)
				{
					character.gani = std::format("def[{}]", character.sprite);
					modTime[PROPID(NPCProp::GANI)] = m_server->getFrameStartTime();
				}
			}
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		.name = "dir"sv,
		.getter = [this](std::optional<size_t>) -> GameValueVariantForGetter { return static_cast<double>(character.direction); },
		.setter = [this](const GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (const auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
			{
				character.direction = std::clamp(static_cast<uint8_t>(value->get()), 0_ui8, 3_ui8);
				modTime[PROPID(NPCProp::SPRITE)] = m_server->getFrameStartTime();
			}
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		.name = "hurtdx"sv,
		.getter = [this](std::optional<size_t>) -> GameValueVariantForGetter { return character.hurtPushDeltaInHalfPixels[0] / 32.0; },
		.setter = [this](const GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (const auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
			{
				const auto clampedValue = std::clamp(value->get(), -1.0, 1.0);
				character.hurtPushDeltaInHalfPixels[0] = static_cast<int8_t>(clampedValue * 32);
				modTime[PROPID(NPCProp::HURTDXDY)] = m_server->getFrameStartTime();
			}
		}
	});

	bind::bindPropertyAsReadWrite(scriptParameters, bind::ManuallyDefinedProperty<double>{
		.name = "hurtdy"sv,
		.getter = [this](std::optional<size_t>) -> GameValueVariantForGetter { return character.hurtPushDeltaInHalfPixels[1] / 32.0; },
		.setter = [this](const GameValueVariantForSetter& incoming, std::optional<int64_t>)
		{
			if (const auto value = std::get_if<std::reference_wrapper<double>>(&incoming); value != nullptr)
			{
				const auto clampedValue = std::clamp(value->get(), -1.0, 1.0);
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
	const auto levelPtr = getLevel();
	if (levelPtr == nullptr) return;

	// The NPC changed their position.
	auto informNPCMoved = [&result]()
	{
		result.resultPropIds.push_back(PROPID(NPCProp::X));
		result.resultPropIds.push_back(PROPID(NPCProp::Y));
		result.resultPropIds.push_back(PROPID(NPCProp::X2));
		result.resultPropIds.push_back(PROPID(NPCProp::Y2));
	};

	// The NPC changed their level and position.
	auto informNPCWarped = [&]()
	{
		// Tell NCs about our new position.
		const CString ncPacket = CString() >> (char)PLO_NC_NPCADD >> (int)id >> (char)NPCProp::LEVEL << getProp<NPCProp::LEVEL>().serialize();
		m_server->sendPacketToType(PLTYPE_ANYNC, ncPacket);

		// Set our level prop.
		result.resultPropIds.clear();
		result.resultPropIds.push_back(PROPID(NPCProp::LEVEL));
		informNPCMoved();

		// Tell players that we changed level.
		auto localPosition = getLocalPosition();
		m_server->sendPacketToType(PLTYPE_ANYPLAYER, CString() >> (char)PLO_NPCMOVED >> (int)id >> (char)(localPosition.x() / 8) >> (char)(localPosition.y() / 8) << getLevelName());
		m_server->sendPacketToNearby(CString() >> (char)PLO_NPCPROPS >> (int)id << getAllPropsPacket(), character.getGlobalPosition(), levelPtr);

		// Add a scripting event for the warp.
		scripting.events.addEvent(ScriptEventType::NPCWARPED, source::FromNPC(id));
	};

	// Clamp NPC to the level.
	auto clampToLevel = [&]()
	{
		const auto clampedX = std::clamp(character.localPixelX, static_cast<int16_t>(0), static_cast<int16_t>(61 * 16));
		const auto clampedY = std::clamp(character.localPixelY, static_cast<int16_t>(0), static_cast<int16_t>(61 * 16));
		if (clampedX != character.localPixelX || clampedY != character.localPixelY)
		{
			character.localPixelX = clampedX;
			character.localPixelY = clampedY;
			informNPCMoved();
		}
	};

	// Gmaps are treated as one large map, and so level npcs can freely walk across maps (source: post=1193766)
	if (levelPtr->isGmap())
	{
		const uint8_t computedMapX = character.localPixelX / 1024;
		const uint8_t computedMapY = character.localPixelY / 1024;
		const auto computedLocalX = static_cast<int16_t>(character.localPixelX % 1024);
		const auto computedLocalY = static_cast<int16_t>(character.localPixelY % 1024);

		// We test the NPC's x/y position to see if they walked out of the bounds of the current level.
		// If they did, alter their map location.
		if (computedMapX != character.mapX || computedMapY != character.mapY)
		{
			character.mapX = computedMapX;
			character.mapY = computedMapY;
			result.resultPropIds.push_back(PROPID(NPCProp::GMAPLEVELX));
			result.resultPropIds.push_back(PROPID(NPCProp::GMAPLEVELY));

			character.localPixelX = computedLocalX;
			character.localPixelY = computedLocalY;
			informNPCMoved();
		}
	}

	// They aren't allowed to leave the level, so clamp them to the borders.
	if (levelPtr->isGmap() || levelPtr->isOnBigMap())
		clampToLevel();

	// If we have warp restrictions, don't process any further.
	if (warpRestrictions == NPCWarpRestrictions::NOTALLOWED)
		return;

	// Test for links.
	static Position<float> touchTest[] = {{2, 1}, {0, 2}, {2, 4}, {3, 2}};
	const TilePosition testPos = character.getTilePosition().translate(touchTest[character.direction].x(), touchTest[character.direction].y());
	if (const auto linkTouched = levelPtr->getLink(testPos); linkTouched.has_value())
	{
		auto& destLevelName = linkTouched.value()->getDestinationLevel();
		const auto& currentMap = levelPtr->getMap();

		// If we only allow overworld links, and the destination level was not found on the current map, then don't do anything.
		if (warpRestrictions == NPCWarpRestrictions::ONLYOVERWORLD && (currentMap == nullptr || !currentMap->hasLevel(destLevelName)))
			return;

		// Check if we have the level.
		const LevelPtr destLevel = m_server->getLoadedLevel(destLevelName, levelPtr);
		if (destLevel == nullptr)
			return;

		// Get the sub-level for the destination level.
		const SubLevelPtr destSubLevel = destLevel->getSubLevelByName(destLevelName);
		if (destSubLevel == nullptr)
			return;

		// If the dest level is a gmap, set our map x/y props.
		if (destSubLevel->isOnGmap)
		{
			auto mapPosition = destSubLevel->mapPosition.value_or(MapPosition{0, 0});
			character.mapX = mapPosition.x();
			character.mapY = mapPosition.y();
			result.resultPropIds.push_back(PROPID(NPCProp::GMAPLEVELX));
			result.resultPropIds.push_back(PROPID(NPCProp::GMAPLEVELY));
		}

		// Set our position.
		auto pos = linkTouched.value()->getDestinationForCharacter(character, source::FromNPC(id));
		character.localPixelX = pos.x();
		character.localPixelY = pos.y();

		// Warp to the new level.
		if (destLevel == levelPtr)
			informNPCMoved();
		else
		{
			setLevel(destLevel);
			informNPCWarped();
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

	const size_t name_end[2] = {code.find(';', name_start), code.find('}', name_start)};
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
		const auto propId = PROPID(prop);

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
			const auto values = variable->get<std::vector<double>>();
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
