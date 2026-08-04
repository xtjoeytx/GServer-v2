#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <CString.h>
#include <IEnums.h>

#include <Account.h>
#include <BabyDI.h>
#include <Server.h>
#include <level/Level.h>
#include <misc/WordFilter.h>
#include <object/NPC.h>
#include <object/Player.h>
#include <player/PlayerClient.h>
#include <player/PlayerProps.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/Log.h>
#include <utilities/PropertySerializers.h>
#include <utilities/std/inplace_vector.h>

using namespace preagonal::props;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

#ifdef PACKETLOGGING
#define DO_PACKETLOG(LOG) LOG
#else
#define DO_PACKETLOG(LOG)
#endif

#define PRINT_PLAYERPROP(prop, ...) #prop ##sv,
constexpr std::array<std::string_view, PLAYERPROP_COUNT> playerPropNames =
{
	FOR_LIST_OF_PLAYER_PROPS(PRINT_PLAYERPROP)
};

#ifdef PACKETLOGGING
static void printHeader(const Player* player, std::string_view header)
{
	log::printBlock(log::networkdump, "{}:\n", header);
	log::printBlock(log::networkdump, "  PlayerProp::ID: value: {}\n", player->getId());
}

static void printProp(const Player* player, PlayerProp playerProp, PropertyBase* base)
{
	auto prop = player->getProp(playerProp);
	CString data = prop->serialize();

	log::printBlock(log::networkdump, "  {}: {}", playerPropNames[PROPID(playerProp)], prop);
	log::printBlock(log::networkdump, " |");
	for (size_t i = 0; i < data.length(); ++i)
		log::printBlock(log::networkdump, " {:02x}", (unsigned char)data[i]);
	log::printBlock(log::networkdump, "\n");
}
#endif

///////////////////////////////////////////////////////////////////////////////

static bool canSendProp(PlayerProp prop)
{
	static Server* server = nullptr;
	if (server == nullptr)
		server = BabyDI::Get<Server>();

	if (server->Generation == ServerGeneration::CLASSIC && PROPID(prop) > PROPID(PlayerProp::RATING))
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<PropertyBase> Player::constructPropFor(PlayerProp prop) const
{
	switch (prop)
	{
#define GENERATE_CONSTRUCTPROPFOR_CASE(prop, type, ...) case prop: return std::make_shared<type>();
		FOR_LIST_OF_PLAYER_PROPS(GENERATE_CONSTRUCTPROPFOR_CASE);
	}
	throw std::invalid_argument("Invalid PlayerProp type in constructPropFor");
}

///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<PropertyBase> Player::getProp(PlayerProp prop) const
{
	switch (prop)
	{
#define GENERATE_GETPROP_CASE(prop, type, ...) case prop: return std::make_shared<type>( __VA_ARGS__ );
		FOR_LIST_OF_PLAYER_PROPS(GENERATE_GETPROP_CASE);
	}

	throw std::invalid_argument("Invalid PlayerProp type in getProp");
}

///////////////////////////////////////////////////////////////////////////////

SetResults Player::setProp(PlayerProp prop, SetBy setBy, std::shared_ptr<PropertyBase> base)
{
	PropertyBase* basePtr = base.get();
	if (basePtr != nullptr)
		return setProp(prop, setBy, basePtr);
	throw std::invalid_argument("setProp called with nullptr base pointer.");
}

SetResults Player::setProp(PlayerProp prop, SetBy setBy, PropertyBase* base)
{
	auto player = std::dynamic_pointer_cast<PlayerClient>(shared_from_this());
	auto level = player ? player->getLevel() : nullptr;
	bool restrictedPropAllowed = !m_server->hasNPCServer() || setBy == props::SetBy::SERVER;

	props::SetResults result{ .propId = PROPID(prop) };
	result.resultFlags.set(props::SetResults::sendToLevel, clientPropsSharedLocal[PROPID(prop)]);
	result.resultFlags.set(props::SetResults::sendToSource, setBy == props::SetBy::SERVER);

	const auto& curTime = m_server->getFrameStartTime();
	auto oldTime = modTime[PROPID(prop)];
	modTime[PROPID(prop)] = curTime;

#define SETPROP_RETURN_ERROR do { result.resultFlags.set(SetResults::wasInvalid); modTime[PROPID(prop)] = oldTime; return result; } while(false)

	switch (prop)
	{
		case PlayerProp::NICKNAME:
		{
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			// Word filter.
			CString nick{ strProp->value };
			int filter = m_server->getWordFilter().apply(this, nick, FILTER_CHECK_NICK);
			if (filter & FILTER_ACTION_WARN)
			{
				if (account.character.nickName.empty())
					setNick("unknown");
			}
			else
			{
				setNick(nick, setBy == props::SetBy::SERVER);
			}

			// If the nickname was changed due to restrictions, send the new nick back to the source.
			if (account.character.nickName != nick)
			{
				result.resultFlags.set(props::SetResults::sendToSource);
				result.resultFlags.set(props::SetResults::getLatestOnSend);
			}

			result.resultFlags.set(props::SetResults::sendToAll);
			break;
		}

		case PlayerProp::FULLHEARTS:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			uint8_t newMaxHitpoints = numProp->value;

			if (restrictedPropAllowed)
			{
				account.maxHitpoints = props::Limits::applyMaxHitpoints(newMaxHitpoints);
				account.character.hitpointsInHalves = newMaxHitpoints * 2;

				result.resultPropIds.push_back(PROPID(PlayerProp::HALFHEARTS));
				result.resultFlags.set(props::SetResults::sendToSource);
			}
			break;
		}

		case PlayerProp::HALFHEARTS:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			uint8_t power = numProp->value;
			auto powerDelta = static_cast<int8_t>(power - account.character.hitpointsInHalves);

			if (account.character.ap < 40 && powerDelta > 0)
				break;

			account.character.hurtDeltaInHalves = -powerDelta;
			account.character.hitpointsInHalves = props::Limits::apply(power, 0, account.maxHitpoints * 2);

			if (m_server->hasNPCServer() && powerDelta < 0)
			{
				m_server->queueNPCEvent(level, getGlobalPosition(), ScriptEventType::PLAYERHURT, source::FromPlayer(m_id));
			}
			break;
		}

		case PlayerProp::GRALATS:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE3>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			uint32_t newGralatCount = std::min(numProp->value, 9999999u);
			if (restrictedPropAllowed)
				account.character.gralats = newGralatCount;
			break;
		}

		case PlayerProp::ARROWS:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.character.arrows = props::Limits::apply(numProp->value, props::Limits::MaxArrows);
			break;
		}

		case PlayerProp::BOMBS:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.character.bombs = props::Limits::apply(numProp->value, props::Limits::MaxBombs);
			break;
		}

		case PlayerProp::GLOVEPOWER:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			uint8_t newGlovePower = numProp->value;
			if (restrictedPropAllowed)
				account.character.glovePower = props::Limits::apply(newGlovePower, props::Limits::MaxGlovePower);
			break;
		}

		case PlayerProp::BOMBPOWER:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.character.bombPower = props::Limits::apply(numProp->value, props::Limits::MaxBombPower);
			break;
		}

		case PlayerProp::SWORDIMAGE:
		{
			auto swordProp = dynamic_cast<PropertySwordPower*>(base);
			if (swordProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (restrictedPropAllowed && swordProp->power.has_value())
				account.character.swordPower = props::Limits::applySwordPower(swordProp->power.value_or(1));

			account.character.swordImage = props::Limits::apply(swordProp->image, props::Limits::SwordImageLength);
			break;
		}

		case PlayerProp::SHIELDIMAGE:
		{
			auto shieldProp = dynamic_cast<PropertyShieldPower*>(base);
			if (shieldProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (restrictedPropAllowed && shieldProp->power.has_value())
				account.character.shieldPower = props::Limits::applyShieldPower(shieldProp->power.value_or(1));

			account.character.shieldImage = props::Limits::apply(shieldProp->image, props::Limits::ShieldImageLength);
			break;
		}

		case PlayerProp::GANI:
		{
			auto ganiProp = dynamic_cast<PropertyGaniOrBowGif*>(base);
			if (ganiProp == nullptr)
				SETPROP_RETURN_ERROR;

			// 1.x servers didn't have ganis.  This prop was used for the bow instead.
			if (m_server->Generation == ServerGeneration::CLASSIC)
			{
				if (!ganiProp->bowGif.has_value())
					break;

				auto& [image, power] = ganiProp->bowGif.value();
				account.character.bowPower = props::Limits::apply(power, props::Limits::MaxBowPower);
				account.character.bowImage = image;
				if (!account.character.bowImage.empty() && !account.character.bowImage.contains('.'))
					account.character.bowImage += ".gif";
				break;
			}

			std::string gani = ganiProp->gani.value_or("idle");
			account.character.gani = props::Limits::apply(gani, props::Limits::GaniLength);

			// Allow spin to hurt things.
			if (account.character.gani == "spin")
			{
				float tX = static_cast<float>(account.character.localPixelX) / 16.0f + 1.5f;
				float tY = static_cast<float>(account.character.localPixelY) / 16.0f + 2.0f;
				m_server->hitObjectsAtPoint({ tX, tY + 2.0f }, account.character.swordPower, level, shared_from_this());
				m_server->hitObjectsAtPoint({ tX, tY - 2.0f }, account.character.swordPower, level, shared_from_this());
				m_server->hitObjectsAtPoint({ tX + 2.0f, tY }, account.character.swordPower, level, shared_from_this());
				m_server->hitObjectsAtPoint({ tX - 2.0f, tY }, account.character.swordPower, level, shared_from_this());
			}
			break;
		}

		case PlayerProp::HEADIMAGE:
		{
			auto headProp = dynamic_cast<PropertyHeadGif*>(base);
			if (headProp == nullptr)
				SETPROP_RETURN_ERROR;

			std::string img;
			if (std::holds_alternative<uint8_t>(headProp->image))
				img = std::format("head{}.{}", std::get<uint8_t>(headProp->image), (m_server->Generation != ServerGeneration::CLASSIC ? "png" : "gif"));
			else
				img = std::get<std::string>(headProp->image);

			if (m_server->Generation == ServerGeneration::CLASSIC && !img.empty() && !img.contains('.'))
				img += ".gif";

			account.character.headImage = props::Limits::apply(img, props::Limits::HeadImageLength);
			result.resultFlags.set(props::SetResults::sendToAll);
			break;
		}

		case PlayerProp::MESSAGE:
		{
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (bool chatChanged = (account.character.chatMessage != strProp->value); !chatChanged)
				break;

			account.character.chatMessage = props::Limits::apply(strProp->value, props::Limits::ChatMessageLength);

			if (player != nullptr)
			{
				player->setLastChatTime(m_server->getFrameStartTime());

				// Try to process the chat.  If it wasn't processed, apply the word filter to it.
				if (!player->processChat(account.character.chatMessage))
				{
					m_server->queueNPCEvent(level, getGlobalPosition(), ScriptEventType::PLAYERCHATS, source::FromPlayer(m_id));

					CString chat = account.character.chatMessage;
					int found = m_server->getWordFilter().apply(this, chat, FILTER_CHECK_CHAT);
					account.character.chatMessage = chat.toString();

					if ((found & FILTER_ACTION_REPLACE) || (found & FILTER_ACTION_WARN))
						result.resultFlags.set(props::SetResults::sendToSource);
				}
			}
			break;
		}

		case PlayerProp::COLORS:
		{
			auto colorProp = dynamic_cast<PropertyColors*>(base);
			if (colorProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.character.colors = colorProp->values;
			break;
		}

		case PlayerProp::ID:
			break;

		case PlayerProp::X:
		{
			auto coordProp = dynamic_cast<PropertyTileCoordinate*>(base);
			if (coordProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (account.character.localPixelX == coordProp->pixelCoordinate)
				break;

			auto movementDirection = static_cast<uint8_t>(2 + std::clamp(coordProp->pixelCoordinate - account.character.localPixelX, -1, 1));
			account.character.localPixelX = coordProp->pixelCoordinate;
			account.status &= (~PLSTATUS_PAUSED);
			result.resultPropIds.push_back(PROPID(PlayerProp::X2));

			if (player != nullptr)
			{
				player->setLastMovementTime(m_server->getFrameStartTime());
				player->testForTouch(result, movementDirection);
			}
			break;
		}

		case PlayerProp::Y:
		{
			auto coordProp = dynamic_cast<PropertyTileCoordinate*>(base);
			if (coordProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (account.character.localPixelY == coordProp->pixelCoordinate)
				break;

			auto movementDirection = static_cast<uint8_t>(1 + std::clamp(coordProp->pixelCoordinate - account.character.localPixelY, -1, 1));
			account.character.localPixelY = coordProp->pixelCoordinate;
			account.status &= (~PLSTATUS_PAUSED);
			result.resultPropIds.push_back(PROPID(PlayerProp::Y2));

			if (player != nullptr)
			{
				player->setLastMovementTime(m_server->getFrameStartTime());
				player->testForTouch(result, movementDirection);
			}
			break;
		}

		case PlayerProp::Z:
		{
			auto zProp = dynamic_cast<PropertyTileCoordinateZ*>(base);
			if (zProp == nullptr)
				SETPROP_RETURN_ERROR;

			// If Z is disabled, don't allow changing it.
			if (m_server->cached.lockPlayerZ.getValue())
			{
				result.resultFlags.reset();
				result.resultFlags.set(SetResults::sendToSource);
				result.resultFlags.set(SetResults::getLatestOnSend);
				break;
			}

			account.character.localPixelZ = zProp->pixelCoordinate;
			account.status &= (~PLSTATUS_PAUSED);
			result.resultPropIds.push_back(PROPID(PlayerProp::Z2));

			if (player != nullptr)
				player->setLastMovementTime(m_server->getFrameStartTime());
			break;
		}

		case PlayerProp::SPRITE:
		{
			auto spriteProp = dynamic_cast<PropertySprite*>(base);
			if (spriteProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (account.character.sprite == spriteProp->sprite && account.character.direction == spriteProp->direction)
				break;

			bool directionChanged = (account.character.direction != spriteProp->direction);
			account.character.direction = spriteProp->direction;
			account.character.sprite = spriteProp->sprite;
			result.resultFlags.set(SetResults::getLatestOnSend);

			// If we manually set a sprite, change the gani.
			if (m_server->Generation != ServerGeneration::CLASSIC && account.character.sprite != 0 && (!account.character.gani.starts_with("def[") || modTime[PROPID(PlayerProp::GANI)] < curTime))
			{
				auto gani = std::format("def[{}]", account.character.sprite);
				result.resultPropIds.push_back(PROPID(PlayerProp::GANI));
				result.resultFlags.set(SetResults::sendToSource);
			}

			// Do collision testing.
			if (player != nullptr && directionChanged)
				player->testForTouch(result, account.character.direction);
			break;
		}

		case PlayerProp::STATUS:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			int oldStatus = account.status;
			account.status = numProp->value;
			//printf("%s: status: %d, oldStatus: %d\n", m_accountName.text(), status, oldStatus );

			if (m_id == 0) break;

			// When they come back to life, give them hearts.
			if ((oldStatus & PLSTATUS_DEAD) > 0 && (account.status & PLSTATUS_DEAD) == 0)
			{
				// Give them full hearts.  If they have less than 20 AP, give them 3 hearts.  If they have less than 40 AP, give them 5 hearts.
				auto newPower = props::Limits::applyMaxHitpoints(account.character.ap < 20 ? 3 : (account.character.ap < 40 ? 5 : account.maxHitpoints)) * 2;
				account.character.hitpointsInHalves = newPower;

				result.resultPropIds.push_back(PROPID(PlayerProp::HALFHEARTS));
				result.resultFlags.set(props::SetResults::sendToSource);
			}

			// When they die, increase deaths and make somebody else level leader.
			if ((oldStatus & PLSTATUS_DEAD) == 0 && (account.status & PLSTATUS_DEAD) > 0 && level != nullptr)
			{
				if (level->isSparringZone(getMapPosition()) == false)
				{
					++account.deaths;
					player->dropItemsOnDeath();
				}

				// If we are the leader and there are more players on the level, we want to remove
				// ourself from the leader position and tell the new leader that they are the leader.
				if (level->isPlayerLeader(m_id) && level->getPlayers().size() > 1 && !level->isGmap())
				{
					level->removePlayer(m_id);
					level->addPlayer(m_id);

					if (auto leader = m_server->getPlayer(level->getPlayers().front()); leader != nullptr)
						leader->sendPacket(CString() >> (char)PLO_ISLEADER);
				}

				// Update our last dead time.
				lastDeadTime = m_server->getNWTime();

				// Queue up the playerdies event.
				m_server->queueNPCEvent(level, getGlobalPosition(), ScriptEventType::PLAYERDIES, source::FromPlayer(m_id));
			}
			break;
		}

		case PlayerProp::CARRYSPRITE:
		{
			auto numProp = dynamic_cast<PropertyUnsafeByte*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			m_carrySprite = numProp->value;
			break;
		}

		case PlayerProp::LEVEL:
		{
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (restrictedPropAllowed && account.level != strProp->value)
				warp(strProp->value, getGlobalPosition());
			break;
		}

		case PlayerProp::HORSEIMAGE:
		{
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.character.horseImage = strProp->value;
			if (m_server->Generation == ServerGeneration::CLASSIC && !account.character.horseImage.empty() && !account.character.horseImage.contains('.'))
				account.character.horseImage += ".gif";
			break;
		}

		case PlayerProp::HORSEBUSHES:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			m_horseBombCount = numProp->value;
			break;
		}

		case PlayerProp::EFFECTCOLORS:
		{
			auto effectColorsProp = dynamic_cast<PropertyEffectColors*>(base);
			if (effectColorsProp == nullptr)
				SETPROP_RETURN_ERROR;

			m_effectColors = effectColorsProp->values;
			break;
		}

		case PlayerProp::CARRYNPCID:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE3>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			NPCID newNPCID = numProp->value;

			if (player == nullptr)
				break;

			// Not supported on gmaps.
			if (level && level->isGmap())
			{
				// The client seems to freak out when picking up a character on the gmap.
				// Normally, the client would "delete" the NPC when picking it up and respawn it when thrown, but on the gmap the
				// NPC doesn't get deleted, causing a lot of weird problems.
				break;
			}

			// Picked up.
			if (player->getCarryNPC() == 0 && newNPCID != 0)
			{
				// Make the NPC invisible while its being carried so other players don't see a ghost NPC sitting on the ground.
				// Do not send this to the player who picked up the NPC.  It will cause a duplicate NPC to generate in their client, breaking future interactions with the NPC.
				// Except, DO send it to the player if the NPC is on a gmap, since, for some reason, the client does the opposite there.
				if (auto npc = m_server->getNPC(newNPCID); npc != nullptr)
				{
					std::inplace_vector<SetResults, 2> results;
					results.push_back(npc->setPropWith<NPCProp::VISFLAGS>(props::SetBy::SERVER, static_cast<uint8_t>(npc->visFlags & ~ENUM(NPCVisFlags::VISIBLE))));
					npc->sendPropsFromResults(level && level->isGmap() ? nullptr : player, results);
				}

				// TODO: Remove when an npcserver is created.
				if (m_server->getSettings().get<bool>("duplicatecanbecarried").value_or(false) == false)
				{
					[[maybe_unused]] bool isOwner = true;
					{
						auto& playerList = m_server->getPlayerList();
						for (auto& other : playerList | std::views::values)
						{
							if (other.get() == this) continue;
							if (other->getProp<PlayerProp::CARRYNPCID>().value == newNPCID)
							{
								// Somebody else got this NPC first.  Force the player to throw his down
								// and tell the player to remove the NPC from memory.
								sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PlayerProp::CARRYNPCID >> (int)0);
								sendPacket(CString() >> (char)PLO_NPCDEL2 >> (char)level->levelName.length() << level->levelName >> (int)newNPCID);
								m_server->sendPacketToNearby(CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id >> (char)PlayerProp::CARRYNPCID >> (int)0, player->getGlobalPosition(), level, { m_id });
								isOwner = false;
								newNPCID = 0;
								break;
							}
						}
					}
				}
			}
			// Thrown.
			else if (player->getCarryNPC() != 0 && newNPCID == 0)
			{
				if (auto npc = m_server->getNPC(player->getCarryNPC()); npc != nullptr)
				{
					// Player carries NPC above their head.  The bottom center of the NPC is carried at x + 1.5, y + 1.
					// When thrown, the NPC travels 9 tiles in the direction the player is facing and lands on the ground after 0.5 seconds.

					// Determine the starting position of the NPC.
					// Clients handle the rendering, so there is no point to simulate the Z movement, so just place it at the feet of the player.
					constexpr int16_t moveDistance = 16 * 9;
					auto dir = player->account.character.direction;
					auto bbox = npc->getBoundingBox();
					auto pos = player->getLocalPosition();
					pos.translate(static_cast<int16_t>(24 - bbox.size.width() / 2), static_cast<int16_t>(48 - bbox.size.height()));

					// Send the position of the NPC to other players in the level so the it will appear in the right spot after the move completes.
					// Do not send to the player who threw it.
					// The client is still "throwing" the NPC and sending props will break things!
					npc->sendPropsFromResults(
						player,
						SetResults{ .propId = PROPID(NPCProp::VISFLAGS), .resultFlags = (1 << SetResults::sendToLevel) },
						npc->setPropWith<NPCProp::X2>(props::SetBy::SERVER, pos.x()),
						npc->setPropWith<NPCProp::Y2>(props::SetBy::SERVER, pos.y())
					);

					// Queue up the movement of the NPC so clients position it properly.
					LocalPixelPosition moveDelta{ dir == 1_i16 ? -moveDistance : dir == 3 ? moveDistance : 0_i16, dir == 0 ? -moveDistance : dir == 2_i16 ? moveDistance : 0_i16 };
					npc->addMoveToQueue(moveDelta, 0.5, ENUM(NPCMoveFlags::NOCACHE));
					if (!npc->moveQueue.empty())
					{
						// Set up the WASTHROWN event to trigger after 0.5 seconds.
						// Also, make the NPC visible again.
						auto& move = npc->moveQueue.back();
						move.onComplete = [snpc = npc, pid = player->getId(), dir]()
						{
							std::inplace_vector<SetResults, 3> results;

							// Make the NPC visible for everybody again.
							results.push_back(snpc->setPropWith<NPCProp::VISFLAGS>(props::SetBy::SERVER, static_cast<uint8_t>(snpc->visFlags | ENUM(NPCVisFlags::VISIBLE))));

							// If the NPC is a character, set their gani to idle and fix their direction.
							if (snpc->isCharacter())
							{
								auto dirprop = snpc->getProp<NPCProp::SPRITE>();
								dirprop.direction = dir;
								results.push_back(snpc->setProp<NPCProp::SPRITE>(props::SetBy::SERVER, dirprop));
								results.push_back(snpc->setPropWith<NPCProp::GANI>(props::SetBy::SERVER, "idle"));
							}

							// Send the results.
							snpc->sendPropsFromResults(results);

							// Trigger the WASTHROWN event.
							snpc->scripting.events.addEvent(ScriptEventType::WASTHROWN, source::FromPlayer(pid));
						};
					}
				}
			}

			player->setCarryNPC(newNPCID);
			break;
		}

		case PlayerProp::APCOUNTER:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE2>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.apCounter = numProp->value + 1;
			break;
		}

		case PlayerProp::MAGICPOINTS:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (restrictedPropAllowed)
				account.character.mp = props::Limits::apply(numProp->value, props::Limits::MaxMP);
			break;
		}

		case PlayerProp::KILLS:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE3>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (restrictedPropAllowed)
				account.kills = numProp->value;
			break;
		}

		case PlayerProp::DEATHS:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE3>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (restrictedPropAllowed)
				account.deaths = numProp->value;
			break;
		}

		case PlayerProp::ONLINESECONDS:
			break;

		case PlayerProp::IPADDR:
			break;

		case PlayerProp::UDPPORT:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE3>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			m_udpport = numProp->value;
			break;
		}

		case PlayerProp::ALIGNMENT:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			uint8_t newAlignment = numProp->value;
			if (restrictedPropAllowed)
				account.character.ap = std::min<uint8_t>(newAlignment, 100);
			break;
		}

		case PlayerProp::ADDITFLAGS:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			m_additionalFlags = numProp->value;
			break;
		}

		case PlayerProp::ACCOUNTNAME:
			break;

		case PlayerProp::BODYIMAGE:
		{
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.character.bodyImage = props::Limits::apply(strProp->value, props::Limits::BodyImageLength);
			break;
		}

		case PlayerProp::RATING:
		{
			auto eloProp = dynamic_cast<PropertyEloRating*>(base);
			if (eloProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (restrictedPropAllowed)
			{
				account.eloRating = eloProp->rating;
				account.eloDeviation = eloProp->deviation;
			}
			break;
		}

		case PlayerProp::ATTACHNPCID:
		{
			auto attachProp = dynamic_cast<PropertyAttachNPC*>(base);
			if (attachProp == nullptr)
				SETPROP_RETURN_ERROR;

			// Only supports object_type 0 (NPC).
			m_attachNPC = attachProp->npcId;
			break;
		}

		case PlayerProp::GMAPLEVELX:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr || level == nullptr || !level->isGmap())
				SETPROP_RETURN_ERROR;

			if (account.character.mapX == numProp->value)
				break;

			if (auto subLevel = level->getSubLevelAtPosition(getMapPosition()); subLevel != nullptr)
				leaveSubLevel(subLevel);

			account.character.mapX = numProp->value;

			if (auto levelData = level->getStaticLevelDataAtPosition(getMapPosition()); levelData != nullptr)
			{
				auto lastEnteredTime = player->getLevelLastEnteredTime(levelData.get());
				sendDynamicLevelData(level, lastEnteredTime);
			}
			break;
		}

		case PlayerProp::GMAPLEVELY:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr || level == nullptr || !level->isGmap())
				SETPROP_RETURN_ERROR;

			if (account.character.mapY == numProp->value)
				break;

			if (auto subLevel = level->getSubLevelAtPosition(getMapPosition()); subLevel != nullptr)
				leaveSubLevel(subLevel);

			account.character.mapY = numProp->value;

			if (auto levelData = level->getStaticLevelDataAtPosition(getMapPosition()); levelData != nullptr)
			{
				auto lastEnteredTime = player->getLevelLastEnteredTime(levelData.get());
				sendDynamicLevelData(level, lastEnteredTime);
			}
			break;
		}

		case PlayerProp::JOINLEAVELVL:
			break;

		case PlayerProp::DISCONNECT:
			break;

		case PlayerProp::LANGUAGE:
		{
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.language = strProp->value;
			break;
		}

		case PlayerProp::PLAYERLISTSTATUS:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			m_statusMsg = numProp->value;
			if (m_id == 0 || !m_loaded)
				break;

			result.resultFlags.set(props::SetResults::sendToAll);
			break;
		}

		case PlayerProp::GATTRIB1:
		case PlayerProp::GATTRIB2:
		case PlayerProp::GATTRIB3:
		case PlayerProp::GATTRIB4:
		case PlayerProp::GATTRIB5:
		case PlayerProp::GATTRIB6:
		case PlayerProp::GATTRIB7:
		case PlayerProp::GATTRIB8:
		case PlayerProp::GATTRIB9:
		case PlayerProp::GATTRIB10:
		case PlayerProp::GATTRIB11:
		case PlayerProp::GATTRIB12:
		case PlayerProp::GATTRIB13:
		case PlayerProp::GATTRIB14:
		case PlayerProp::GATTRIB15:
		case PlayerProp::GATTRIB16:
		case PlayerProp::GATTRIB17:
		case PlayerProp::GATTRIB18:
		case PlayerProp::GATTRIB19:
		case PlayerProp::GATTRIB20:
		case PlayerProp::GATTRIB21:
		case PlayerProp::GATTRIB22:
		case PlayerProp::GATTRIB23:
		case PlayerProp::GATTRIB24:
		case PlayerProp::GATTRIB25:
		case PlayerProp::GATTRIB26:
		case PlayerProp::GATTRIB27:
		case PlayerProp::GATTRIB28:
		case PlayerProp::GATTRIB29:
		case PlayerProp::GATTRIB30:
		{
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			auto index = std::ranges::distance(GaniAttributePropList.begin(), std::ranges::find(GaniAttributePropList, PROPID(prop)));
			account.character.ganiAttributes[index] = strProp->value;
			break;
		}

		// OS type.
		// Windows: wind
		case PlayerProp::OSTYPE:
		{
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.platform = strProp->value;
			break;
		}

		// Text codepage.
		// Example: 1252
		case PlayerProp::TEXTCODEPAGE:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE3>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.codePage = static_cast<uint16_t>(numProp->value);
			break;
		}

		// TODO(Nalin): Does this need to be read?
		case PlayerProp::ONLINESECONDS2:
			break;

		// Location, in pixels, of the player on the level in 2.30+ clients.
		// Bit 0x0001 controls if it is negative or not.
		// Bits 0xFFFE are the actual value.
		case PlayerProp::X2:
		{
			auto pixelProp = dynamic_cast<PropertyPixelCoordinate*>(base);
			if (pixelProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (account.character.localPixelX == pixelProp->pixelCoordinate)
				break;

			auto movementDirection = static_cast<uint8_t>(2 + std::clamp(pixelProp->pixelCoordinate - account.character.localPixelX, -1, 1));
			account.character.localPixelX = pixelProp->pixelCoordinate;
			account.status &= (~PLSTATUS_PAUSED);
			result.resultPropIds.push_back(PROPID(PlayerProp::X));

			if (player != nullptr)
			{
				player->setLastMovementTime(m_server->getFrameStartTime());
				player->testForTouch(result, movementDirection);
			}
			break;
		}

		case PlayerProp::Y2:
		{
			auto pixelProp = dynamic_cast<PropertyPixelCoordinate*>(base);
			if (pixelProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (account.character.localPixelY == pixelProp->pixelCoordinate)
				break;

			auto movementDirection = static_cast<uint8_t>(1 + std::clamp(pixelProp->pixelCoordinate - account.character.localPixelY, -1, 1));
			account.character.localPixelY = pixelProp->pixelCoordinate;
			account.status &= (~PLSTATUS_PAUSED);
			result.resultPropIds.push_back(PROPID(PlayerProp::Y));

			if (player != nullptr)
			{
				player->setLastMovementTime(m_server->getFrameStartTime());
				player->testForTouch(result, movementDirection);
			}
			break;
		}

		case PlayerProp::Z2:
		{
			auto pixelProp = dynamic_cast<PropertyPixelCoordinate*>(base);
			if (pixelProp == nullptr)
				SETPROP_RETURN_ERROR;

			// If Z is disabled, don't allow changing it.
			if (m_server->cached.lockPlayerZ.getValue())
			{
				result.resultFlags.reset();
				result.resultFlags.set(SetResults::sendToSource);
				result.resultFlags.set(SetResults::getLatestOnSend);
				break;
			}

			account.character.localPixelZ = pixelProp->pixelCoordinate;
			account.status &= (~PLSTATUS_PAUSED);
			result.resultPropIds.push_back(PROPID(PlayerProp::Z));

			if (player != nullptr)
				player->setLastMovementTime(m_server->getFrameStartTime());
			break;
		}

		case PlayerProp::PLAYERLISTCATEGORY:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			m_playerListCategory = (PlayerListCategory)numProp->value;
			break;
		}

		case PlayerProp::COMMUNITYNAME:
		{
			auto strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.communityName = strProp->value;
			break;
		}

		case PlayerProp::UNKNOWN83:
		{
			auto numProp = dynamic_cast<PropertyNumeric<GBYTE5>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			log::printLine(log::server, "Player {} set prop 83 to value {}.  This prop is currently unknown.", account.name, numProp->value);
			break;
		}

		default:
		{
			log::printLine(log::server, "Player {} sent an unidentified prop: {}.", account.name, PROPID(prop));
			result.resultFlags.set(props::SetResults::wasInvalid);
			break;
		}
	}

	// If we are sending other ids, we need to update the mod time for them too.
	if (!result.resultPropIds.empty())
	{
		for (const auto& id : result.resultPropIds)
			modTime[id] = curTime;
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////

void Player::setPropsFromPacket(CString& packet, props::SetBy setBy, Player* originator)
{
	PropertySendResults results;

	while (packet.bytesLeft() > 0)
	{
		const auto propId = static_cast<PlayerProp>(packet.readGUChar());

		auto prop = constructPropFor(propId);
		prop->deserialize(packet);

		if (!checkPropSetAccess(propId, setBy, originator))
			continue;

		results.emplace_back(setProp(propId, setBy, prop), prop);
	}

	if (isLoggedIn() && isLoaded())
		sendPropsFromResults(results);
}

bool Player::checkPropSetAccess(PlayerProp prop, SetBy setBy, Player* originator) const
{
	// Admin check on changing gralats.
	if (prop == PlayerProp::GRALATS && originator != nullptr)
	{
		bool canSet = m_server->cached.normalAdminsCanChangeGralats.getValue();
		canSet = canSet || (originator->isStaff() && originator->account.hasRight(PLPERM_SETRIGHTS));
		return canSet;
	}

	return true;
}

void Player::sendPropsFromResults(PropertySendResults& results)
{
	CString sendAll, sendLevel, sendSource;

	std::erase_if(results, [](const PropertySendResults::value_type& res)
	{
		return !canSendProp(static_cast<PlayerProp>(res.first.propId));
	});

	collectPacketsFromResults(results, sendAll, sendLevel, sendSource, [this](uint8_t propId, SetResults::ResultFlagType& destinations)
	{
		return this->getProp(static_cast<PlayerProp>(propId));
	});

	// Send the buffers out.
	if (sendAll.length() > 0)
		m_server->sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)this->m_id << sendAll, { m_id });

	if (auto player = std::dynamic_pointer_cast<PlayerClient>(shared_from_this()); player != nullptr && sendLevel.length() > 0)
		m_server->sendPacketToNearby(CString() >> (char)PLO_OTHERPLPROPS >> (short)this->m_id << sendLevel, player->getGlobalPosition(), player->getLevel(), { m_id });

	if (sendSource.length() > 0)
		sendPacket(CString() >> (char)PLO_PLAYERPROPS << sendSource);
}

void Player::setPropsFromRCPacket(CString& packet, Player* rc)
{
	[[maybe_unused]] bool hadBomb = false, hadBow = false;
	CString outPacket;

	// Skip playerworld
	packet.readChars(packet.readGUChar());

	// Read props from the packet.
	CString props = packet.readChars(packet.readGUChar());

	// Send props out.
	setPropsFromPacket(props, props::SetBy::SERVER, rc);

	// Clear flags
	for (const auto& [flag, value] : account.variables.store | variables::serializable)
		outPacket >> (char)PLO_FLAGDEL << flag << "\n";
	account.variables.store.clear();

	// Clear Weapons
	for (const auto& weapon : account.weapons)
	{
		outPacket >> (char)PLO_NPCWEAPONDEL << weapon << "\n";

		// Attempt to fix the funky client bomb capitalization issue.
		// Also fix the bomb coming back when you set the player props through RC.
		if (weapon == "bomb")
		{
			outPacket >> (char)PLO_NPCWEAPONDEL << "Bomb\n";
			hadBomb = true;
		}
		if (weapon == "Bomb")
			hadBomb = true;

		// Do the same thing with the bow.
		if (weapon == "bow")
		{
			outPacket >> (char)PLO_NPCWEAPONDEL << "Bow\n";
			hadBow = true;
		}
		if (weapon == "Bow")
			hadBow = true;
	}
	account.weapons.clear();

	// Send the packet to clear the flags and weapons from the client.
	if (isLoaded())
		sendPacket(outPacket);

	// Re-populate the flag list.
	auto flagCount = packet.readGUShort();
	while (flagCount-- > 0)
	{
		CString flag = packet.readChars(packet.readGUChar());
		std::string name = flag.readString("=").toString();
		std::string val = flag.readString("").toString();

		if (val.empty())
			setFlag(name, std::nullopt, SetBy::SERVER);
		else setFlag(name, val, SetBy::SERVER);
	}

	// Clear the chests and re-populate the chest list.
	account.savedChests.clear();
	auto chestCount = packet.readGUShort();
	while (chestCount > 0)
	{
		const unsigned char len = packet.readGUChar();
		const uint8_t loc[2] = { packet.readGUChar(), packet.readGUChar() };
		std::string level = packet.readChars(len - 2).toString();

		account.savedChests.insert(std::make_pair(level, LocalWholeTilePosition{ loc[0], loc[1] }));
		--chestCount;
	}

	// Re-populate the weapons list.
	auto weaponCount = packet.readGUChar();
	while (weaponCount > 0)
	{
		const unsigned char len = packet.readGUChar();
		if (len == 0) continue;
		CString wpn = packet.readChars(len);

		// Allow the bomb through if we are actually adding it.
		if (wpn == "bomb" || wpn == "Bomb")
			hadBomb = true;

		// Allow the bow through if we are actually adding it.
		if (wpn == "bow" || wpn == "Bow")
			hadBow = true;

		// Send the weapon to the player.
		this->addWeapon(wpn.toString());
		--weaponCount;
	}

	// KILL THE BOMB DEAD
	if (isLoaded())
	{
		if (!hadBomb)
			sendPacket(CString() >> (char)PLO_NPCWEAPONDEL << "Bomb");
	}

	// Warp the player to his new location now.
	if (isLoaded() && isClient())
	{
		if (const auto player = std::dynamic_pointer_cast<PlayerClient>(shared_from_this()); player != nullptr)
			player->warp(account.level, account.character.getLocalPosition(), std::nullopt);
	}
}

CString Player::getPropsPacketFromList(const PropList& props) const
{
	DO_PACKETLOG(bool printedHeader = false);

	CString propPacket;

	// Create Props
	for (int i = 0; i < PLAYERPROP_COUNT; ++i)
	{
		if (!canSendProp(static_cast<PlayerProp>(i)))
			continue;

		if (props[i])
		{
			DO_PACKETLOG(if (!printedHeader) { printedHeader = true; printHeader(this, "PlayerProps::getPropsPacketFromList"sv); });
			const auto prop = getProp(static_cast<PlayerProp>(i));
			DO_PACKETLOG(printProp(this, (PlayerProp)i, prop.get()));
			propPacket >> (char)i << prop->serialize();
		}
	}

	if (m_isExternal)
		propPacket >> (char)PlayerProp::PLAYERLISTCATEGORY >> (char)PlayerListCategory::EXTERNAL;

	DO_PACKETLOG(if (printedHeader) log::print(log::networkdump, "\n"));
	return propPacket;
}

CString Player::getPropsForRCPacket()
{
	CString ret;
	ret >> (char)account.name.length() << account.name;
	ret >> (char)4 << "main"; // worldName

	// Add the props.
	const CString props = getPropsPacketFromList(clientPropsForRCView);
	ret >> (char)props.length() << props;

	// Add the player's flags.
	CString flags;
	size_t flagCount = 0;
	for (const auto& [flag, value] : account.variables.store | variables::serializable)
	{
		if (auto computedFlag = account.variables.serializeModern(flag); computedFlag.has_value())
		{
			++flagCount;
			flags >> (char)(std::min((size_t)223, computedFlag.value().length())) << computedFlag.value().substr(0, 223);
		}
	}
	ret >> (short)flagCount << flags;

	// Add the player's chests.
	ret >> (short)account.savedChests.size();
	for (const auto& [level, loc] : account.savedChests)
	{
		ret >> (char)(level.length() + 2) >> (char)loc.x() >> (char)loc.y() << level;
	}

	// Add the player's weapons.
	ret >> (char)account.weapons.size();
	for (const auto& weapon : account.weapons)
		ret >> (char)weapon.length() << weapon;

	return ret;
}

CString Player::getModifiedPropsPacket() const
{
	DO_PACKETLOG(bool printedHeader = false);

	CString result;
	for (auto i = 0; i < PLAYERPROP_COUNT; ++i)
	{
		if (!canSendProp(static_cast<PlayerProp>(i)))
			continue;

		if (modTime[i].has_value() && modTime[i] != m_savedModTime[i])
		{
			DO_PACKETLOG(if (!printedHeader) { printedHeader = true; printHeader(this, "PlayerProps::getPropsPacketFromList"sv); });
			const auto prop = getProp(static_cast<PlayerProp>(i));
			DO_PACKETLOG(printProp(this, (PlayerProp)i, prop.get()));
			CString data = prop->serialize();
			result >> (char)i << data;
		}
	}

	DO_PACKETLOG(if (printedHeader) log::print(log::networkdump, "\n"));
	return result;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
