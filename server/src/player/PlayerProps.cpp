#include <algorithm>
#include <cstdint>
#include <ctime>
#include <format>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <CString.h>
#include <IEnums.h>

#include <Account.h>
#include <Server.h>
#include <level/Level.h>
#include <level/Map.h>
#include <misc/WordFilter.h>
#include <object/Player.h>
#include <player/PlayerClient.h>
#include <player/PlayerProps.h>
#include <scripting/ScriptContainers.h>
#include <scripting/ScriptTypes.h>
#include <utilities/CommonTypes.h>
#include <utilities/Log.h>
#include <utilities/PropsContainer.h>

using namespace preagonal::props;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
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

	props::SetResults result{ .propId = { PROPID(prop) } };
	result.resultFlags.set(props::SetResults::sendToLevel, clientPropsSharedLocal[PROPID(prop)]);
	result.resultFlags.set(props::SetResults::sendToSource, setBy == props::SetBy::SERVER);

	const auto& curTime = m_server->getFrameStartTime();
	clock::time_point oldTime = modTime[PROPID(prop)];
	modTime[PROPID(prop)] = curTime;

#define SETPROP_RETURN_ERROR do { result.resultFlags.set(SetResults::wasInvalid); modTime[PROPID(prop)] = oldTime; return result; } while(false)

	switch (prop)
	{
		case PlayerProp::NICKNAME:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
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

			result.resultFlags.set(props::SetResults::sendToAll);
			break;
		}

		case PlayerProp::MAXPOWER:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			uint8_t newMaxHitpoints = numProp->value;

			if (restrictedPropAllowed)
			{
				account.maxHitpoints = props::Limits::applyMaxHitpoints(newMaxHitpoints);
				account.character.hitpointsInHalves = newMaxHitpoints * 2;

				result.resultPropIds.push_back(PROPID(PlayerProp::CURPOWER));
				result.resultFlags.set(props::SetResults::sendToSource);
			}
			break;
		}

		case PlayerProp::CURPOWER:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			uint8_t power = numProp->value;

			if (account.character.ap < 40 && power > account.character.hitpointsInHalves) break;
			account.character.hitpointsInHalves = props::Limits::apply(power, 0, account.maxHitpoints * 2);
			break;
		}

		case PlayerProp::RUPEESCOUNT:
		{
			PropertyNumeric<GBYTE3>* numProp = dynamic_cast<PropertyNumeric<GBYTE3>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			uint32_t newGralatCount = std::min(numProp->value, 9999999u);
			if (restrictedPropAllowed)
				account.character.gralats = newGralatCount;
			break;
		}

		case PlayerProp::ARROWSCOUNT:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.character.arrows = props::Limits::apply(numProp->value, props::Limits::MaxArrows);
			break;
		}

		case PlayerProp::BOMBSCOUNT:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.character.bombs = props::Limits::apply(numProp->value, props::Limits::MaxBombs);
			break;
		}

		case PlayerProp::GLOVEPOWER:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			uint8_t newGlovePower = numProp->value;
			if (restrictedPropAllowed)
				account.character.glovePower = props::Limits::apply(newGlovePower, props::Limits::MaxGlovePower);
			break;
		}

		case PlayerProp::BOMBPOWER:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.character.bombPower = props::Limits::apply(numProp->value, props::Limits::MaxBombPower);
			break;
		}

		case PlayerProp::SWORDPOWER:
		{
			PropertySwordPower* swordProp = dynamic_cast<PropertySwordPower*>(base);
			if (swordProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (restrictedPropAllowed && swordProp->power.has_value())
				account.character.swordPower = props::Limits::applySwordPower(swordProp->power.value_or(1));

			account.character.swordImage = props::Limits::apply(swordProp->image, props::Limits::SwordImageLength);
			break;
		}

		case PlayerProp::SHIELDPOWER:
		{
			PropertyShieldPower* shieldProp = dynamic_cast<PropertyShieldPower*>(base);
			if (shieldProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (restrictedPropAllowed && shieldProp->power.has_value())
				account.character.shieldPower = props::Limits::applyShieldPower(shieldProp->power.value_or(1));

			account.character.shieldImage = props::Limits::apply(shieldProp->image, props::Limits::ShieldImageLength);
			break;
		}

		case PlayerProp::GANI:
		{
			PropertyGaniOrBowGif* ganiProp = dynamic_cast<PropertyGaniOrBowGif*>(base);
			if (ganiProp == nullptr)
				SETPROP_RETURN_ERROR;

			// 1.x servers didn't have ganis.  This prop was used for the bow instead.
			if (m_server->Generation == ServerGeneration::ORIGINAL)
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

			// Hack to allow spin to hurt things.
			if (account.character.gani == "spin")
			{
				float tX = static_cast<float>(account.character.pixelX / 16.0f) + 1.5f;
				float tY = static_cast<float>(account.character.pixelY / 16.0f) + 2.0f;
				m_server->hitObjectsAtPoint({ tX, tY + 2.0f }, account.character.swordPower, level, shared_from_this());
				m_server->hitObjectsAtPoint({ tX, tY - 2.0f }, account.character.swordPower, level, shared_from_this());
				m_server->hitObjectsAtPoint({ tX + 2.0f, tY }, account.character.swordPower, level, shared_from_this());
				m_server->hitObjectsAtPoint({ tX - 2.0f, tY }, account.character.swordPower, level, shared_from_this());
			}
			break;
		}

		case PlayerProp::HEADGIF:
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

			account.character.headImage = props::Limits::apply(img, props::Limits::HeadImageLength);
			result.resultFlags.set(props::SetResults::sendToAll);
			break;
		}

		case PlayerProp::CURCHAT:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			bool chatChanged = (account.character.chatMessage != strProp->value);
			if (!chatChanged)
				break;

			account.character.chatMessage = props::Limits::apply(strProp->value, props::Limits::ChatMessageLength);

			if (player != nullptr)
			{
				player->setLastChatTime(time(0));

				// Try to process the chat.  If it wasn't processed, apply the word filter to it.
				if (!player->processChat(account.character.chatMessage))
				{
					m_server->queueNPCEvent(level, ScriptEventType::PLAYERCHATS, source::FromPlayer(m_id));

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
			PropertyColors* colorProp = dynamic_cast<PropertyColors*>(base);
			if (colorProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.character.colors = colorProp->values;
			break;
		}

		case PlayerProp::ID:
			break;

		case PlayerProp::X:
		{
			PropertyTileCoordinate* coordProp = dynamic_cast<PropertyTileCoordinate*>(base);
			if (coordProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (account.character.pixelX == coordProp->pixelCoordinate)
				break;

			auto movementDirection = static_cast<uint8_t>(2 + std::clamp(coordProp->pixelCoordinate - account.character.pixelX, -1, 1));
			account.character.pixelX = coordProp->pixelCoordinate;
			account.status &= (~PLSTATUS_PAUSED);
			result.resultPropIds.push_back(PROPID(PlayerProp::X2));

			if (player != nullptr)
			{
				player->setLastMovementTime(time(0));
				player->testForTouch(result, movementDirection);
			}
			break;
		}

		case PlayerProp::Y:
		{
			PropertyTileCoordinate* coordProp = dynamic_cast<PropertyTileCoordinate*>(base);
			if (coordProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (account.character.pixelY == coordProp->pixelCoordinate)
				break;

			auto movementDirection = static_cast<uint8_t>(1 + std::clamp(coordProp->pixelCoordinate - account.character.pixelY, -1, 1));
			account.character.pixelY = coordProp->pixelCoordinate;
			account.status &= (~PLSTATUS_PAUSED);
			result.resultPropIds.push_back(PROPID(PlayerProp::Y2));

			if (player != nullptr)
			{
				player->setLastMovementTime(time(0));
				player->testForTouch(result, movementDirection);
			}
			break;
		}

		case PlayerProp::Z:
		{
			PropertyTileCoordinateZ* zProp = dynamic_cast<PropertyTileCoordinateZ*>(base);
			if (zProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.character.pixelZ = zProp->pixelCoordinate;
			account.status &= (~PLSTATUS_PAUSED);
			result.resultPropIds.push_back(PROPID(PlayerProp::Z2));

			if (player != nullptr)
				player->setLastMovementTime(time(0));
			break;
		}

		case PlayerProp::SPRITE:
		{
			PropertySprite* spriteProp = dynamic_cast<PropertySprite*>(base);
			if (spriteProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (account.character.sprite == spriteProp->sprite && account.character.direction == spriteProp->direction)
				break;

			bool directionChanged = (account.character.direction != spriteProp->direction);
			account.character.direction = spriteProp->direction;
			account.character.sprite = spriteProp->sprite;
			result.resultFlags.set(SetResults::getLatestOnSend);

			// If we manually set a sprite, change the gani.
			if (m_server->Generation != ServerGeneration::ORIGINAL && account.character.sprite != 0 && (!account.character.gani.starts_with("def[") || modTime[PROPID(PlayerProp::GANI)] < curTime))
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
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
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

				result.resultPropIds.push_back(PROPID(PlayerProp::CURPOWER));
				result.resultFlags.set(props::SetResults::sendToSource);

				// TODO(Nalin): There could be a race condition on when this packet is sent.  Do we delay until after props are sent to the client?
				if ((level != nullptr && level->isPlayerLeader(m_id)) && (level->getMap() == nullptr || !level->getMap()->isGmap()))
					sendPacket(CString() >> (char)PLO_ISLEADER);

				/*
				// If we are the leader of the level, call warp().  This will fix NPCs not
				// working again after we respawn.
				if (level != 0 && level->getPlayer(0) == this)
					warp(m_levelName, x, y, time(0));
				*/
			}

			// When they die, increase deaths and make somebody else level leader.
			if ((oldStatus & PLSTATUS_DEAD) == 0 && (account.status & PLSTATUS_DEAD) > 0 && level != nullptr)
			{
				if (level->isSparringZone() == false)
				{
					++account.deaths;
					player->dropItemsOnDeath();
				}

				// If we are the leader and there are more players on the level, we want to remove
				// ourself from the leader position and tell the new leader that they are the leader.
				if (level->isPlayerLeader(m_id) && level->getPlayers().size() > 1)
				{
					level->removePlayer(m_id);
					level->addPlayer(m_id);

					if (auto map = level->getMap(); map == nullptr || !map->isGmap())
					{
						auto leader = m_server->getPlayer(level->getPlayers().front());
						if (leader) leader->sendPacket(CString() >> (char)PLO_ISLEADER);
					}
				}

				// Update our last dead time.
				lastDeadTime = m_server->getNWTime();

				// Queue up the playerdies event.
				m_server->queueNPCEvent(level, ScriptEventType::PLAYERDIES, source::FromPlayer(m_id));
			}
			break;
		}

		case PlayerProp::CARRYSPRITE:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			m_carrySprite = numProp->value;
			break;
		}

		case PlayerProp::CURLEVEL:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (restrictedPropAllowed)
				account.level = strProp->value;
			break;
		}

		case PlayerProp::HORSEGIF:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.character.horseImage = strProp->value;
			if (m_server->Generation == ServerGeneration::ORIGINAL && !account.character.horseImage.empty() && !account.character.horseImage.contains('.'))
				account.character.horseImage += ".gif";
			break;
		}

		case PlayerProp::HORSEBUSHES:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			m_horseBombCount = numProp->value;
			break;
		}

		case PlayerProp::EFFECTCOLORS:
		{
			PropertyEffectColors* effectColorsProp = dynamic_cast<PropertyEffectColors*>(base);
			if (effectColorsProp == nullptr)
				SETPROP_RETURN_ERROR;

			m_effectColors = effectColorsProp->values;
			break;
		}

		case PlayerProp::CARRYNPC:
		{
			PropertyNumeric<GBYTE3>* numProp = dynamic_cast<PropertyNumeric<GBYTE3>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			NPCID newNPCID = numProp->value;

			if (player == nullptr)
				break;

			// Not supported on gmaps.
			// If we want carry npcs to work with database npcs, a lot of work would be required.
			// The throw range is 9 tiles.  We could probably send a move2 command on a throw.
			if (auto map = player->getMap().lock(); map && map->getType() == MapType::GMAP)
			{
				// I tried to throw the NPC and make it visible again, but there seems to be a race condition with the client.
				// The client wasn't throwing the NPC automatically when setting the PlayerProp::CARRYNPC to 0.
				// It would also permanently hide the NPC when thrown and I couldn't bring it back.
				// There are probably race conditions with how long it takes for the pick up and throw animations to fully play out.
				break;
			}

			// Picked up.
			if (player->getCarryNPC() == 0 && newNPCID != 0)
			{
				// TODO: Remove when an npcserver is created.
				if (m_server->getSettings().getBool("duplicatecanbecarried", false) == false)
				{
					bool isOwner = true;
					{
						auto& playerList = m_server->getPlayerList();
						for (auto& [otherId, other] : playerList)
						{
							if (other.get() == this) continue;
							if (other->getProp<PlayerProp::CARRYNPC>().value == newNPCID)
							{
								// Somebody else got this NPC first.  Force the player to throw his down
								// and tell the player to remove the NPC from memory.
								sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PlayerProp::CARRYNPC >> (int)0);
								sendPacket(CString() >> (char)PLO_NPCDEL2 >> (char)level->getLevelName().length() << level->getLevelName() >> (int)newNPCID);
								m_server->sendPacketToLevelArea(CString() >> (char)PLO_OTHERPLPROPS >> (short)m_id >> (char)PlayerProp::CARRYNPC >> (int)0, player, { m_id });
								isOwner = false;
								newNPCID = 0;
								break;
							}
						}
					}
				}
			}
			player->setCarryNPC(newNPCID);
			break;
		}

		case PlayerProp::APCOUNTER:
		{
			PropertyNumeric<GBYTE2>* numProp = dynamic_cast<PropertyNumeric<GBYTE2>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.apCounter = numProp->value + 1;
			break;
		}

		case PlayerProp::MAGICPOINTS:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (restrictedPropAllowed)
				account.character.mp = props::Limits::apply(numProp->value, props::Limits::MaxMP);
			break;
		}

		case PlayerProp::KILLSCOUNT:
		{
			PropertyNumeric<GBYTE3>* numProp = dynamic_cast<PropertyNumeric<GBYTE3>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (restrictedPropAllowed)
				account.kills = numProp->value;
			break;
		}

		case PlayerProp::DEATHSCOUNT:
		{
			PropertyNumeric<GBYTE3>* numProp = dynamic_cast<PropertyNumeric<GBYTE3>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (restrictedPropAllowed)
				account.deaths = numProp->value;
			break;
		}

		case PlayerProp::ONLINESECS:
			break;

		case PlayerProp::IPADDR:
			break;

		case PlayerProp::UDPPORT:
		{
			PropertyNumeric<GBYTE3>* numProp = dynamic_cast<PropertyNumeric<GBYTE3>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			m_udpport = numProp->value;
			break;
		}

		case PlayerProp::ALIGNMENT:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			uint8_t newAlignment = numProp->value;
			if (restrictedPropAllowed)
				account.character.ap = std::min<uint8_t>(newAlignment, 100);
			break;
		}

		case PlayerProp::ADDITFLAGS:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			m_additionalFlags = numProp->value;
			break;
		}

		case PlayerProp::ACCOUNTNAME:
			break;

		case PlayerProp::BODYIMG:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.character.bodyImage = props::Limits::apply(strProp->value, props::Limits::BodyImageLength);
			break;
		}

		case PlayerProp::RATING:
		{
			PropertyEloRating* eloProp = dynamic_cast<PropertyEloRating*>(base);
			if (eloProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (restrictedPropAllowed)
			{
				account.eloRating = eloProp->rating;
				account.eloDeviation = eloProp->deviation;
			}
			break;
		}

		case PlayerProp::ATTACHNPC:
		{
			PropertyAttachNPC* attachProp = dynamic_cast<PropertyAttachNPC*>(base);
			if (attachProp == nullptr)
				SETPROP_RETURN_ERROR;

			// Only supports object_type 0 (NPC).
			m_attachNPC = attachProp->npcId;
			break;
		}

		case PlayerProp::GMAPLEVELX:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (level == nullptr)
				break;

			if (auto cmap = level->getMap(); cmap && cmap->isGmap())
			{
				auto& newLevelName = cmap->getLevelAt(numProp->value, level->getMapY());
				if (auto newLevel = m_server->getLevel(newLevelName); newLevel != nullptr)
					enterLevel(m_server->getLevel(newLevelName), { account.character.pixelX, account.character.pixelY }, player->getCachedLevelModTime(newLevel.get()));
			}
			break;
		}

		case PlayerProp::GMAPLEVELY:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (level == nullptr)
				break;

			if (auto cmap = level->getMap(); cmap && cmap->isGmap())
			{
				auto& newLevelName = cmap->getLevelAt(level->getMapX(), numProp->value);
				if (auto newLevel = m_server->getLevel(newLevelName); newLevel != nullptr)
					enterLevel(m_server->getLevel(newLevelName), { account.character.pixelX, account.character.pixelY }, player->getCachedLevelModTime(newLevel.get()));
			}
			break;
		}

		case PlayerProp::JOINLEAVELVL:
			break;

		case PlayerProp::PCONNECTED:
			break;

		case PlayerProp::PLANGUAGE:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.language = strProp->value;
			break;
		}

		case PlayerProp::PSTATUSMSG:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
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
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
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
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			m_os = strProp->value;
			break;
		}

		// Text codepage.
		// Example: 1252
		case PlayerProp::TEXTCODEPAGE:
		{
			PropertyNumeric<GBYTE3>* numProp = dynamic_cast<PropertyNumeric<GBYTE3>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			m_envCodePage = numProp->value;
			break;
		}

		// TODO(Nalin): Does this need to be read?
		case PlayerProp::ONLINESECS2:
			break;

		// Location, in pixels, of the player on the level in 2.30+ clients.
		// Bit 0x0001 controls if it is negative or not.
		// Bits 0xFFFE are the actual value.
		case PlayerProp::X2:
		{
			PropertyPixelCoordinate* pixelProp = dynamic_cast<PropertyPixelCoordinate*>(base);
			if (pixelProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (account.character.pixelX == pixelProp->pixelCoordinate)
				break;

			auto movementDirection = static_cast<uint8_t>(2 + std::clamp(pixelProp->pixelCoordinate - account.character.pixelX, -1, 1));
			account.character.pixelX = pixelProp->pixelCoordinate;
			account.status &= (~PLSTATUS_PAUSED);
			result.resultPropIds.push_back(PROPID(PlayerProp::X));

			if (player != nullptr)
			{
				player->setLastMovementTime(time(0));
				player->testForTouch(result, movementDirection);
			}
			break;
		}

		case PlayerProp::Y2:
		{
			PropertyPixelCoordinate* pixelProp = dynamic_cast<PropertyPixelCoordinate*>(base);
			if (pixelProp == nullptr)
				SETPROP_RETURN_ERROR;

			if (account.character.pixelY == pixelProp->pixelCoordinate)
				break;

			auto movementDirection = static_cast<uint8_t>(1 + std::clamp(pixelProp->pixelCoordinate - account.character.pixelY, -1, 1));
			account.character.pixelY = pixelProp->pixelCoordinate;
			account.status &= (~PLSTATUS_PAUSED);
			result.resultPropIds.push_back(PROPID(PlayerProp::Y));

			if (player != nullptr)
			{
				player->setLastMovementTime(time(0));
				player->testForTouch(result, movementDirection);
			}
			break;
		}

		case PlayerProp::Z2:
		{
			PropertyPixelCoordinate* pixelProp = dynamic_cast<PropertyPixelCoordinate*>(base);
			if (pixelProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.character.pixelZ = pixelProp->pixelCoordinate;
			account.status &= (~PLSTATUS_PAUSED);
			result.resultPropIds.push_back(PROPID(PlayerProp::Z));

			if (player != nullptr)
				player->setLastMovementTime(time(0));
			break;
		}

		case PlayerProp::PLAYERLISTCATEGORY:
		{
			PropertyNumeric<GBYTE1>* numProp = dynamic_cast<PropertyNumeric<GBYTE1>*>(base);
			if (numProp == nullptr)
				SETPROP_RETURN_ERROR;

			m_playerListCategory = (PlayerListCategory)numProp->value;
			break;
		}

		case PlayerProp::COMMUNITYNAME:
		{
			PropertyString* strProp = dynamic_cast<PropertyString*>(base);
			if (strProp == nullptr)
				SETPROP_RETURN_ERROR;

			account.communityName = strProp->value;
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
		PlayerProp propId = (PlayerProp)packet.readGUChar();

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
	if (prop == PlayerProp::RUPEESCOUNT && originator != nullptr)
	{
		bool canSet = m_server->getSettings().getBool("normaladminscanchangegralats", true);
		canSet = canSet || (originator->isStaff() && originator->account.hasRight(PLPERM_SETRIGHTS));
		return canSet;
	}

	return true;
}

void Player::sendPropsFromResults(PropertySendResults& results)
{
	CString sendAll, sendLevel, sendSource;

	collectPacketsFromResults(results, sendAll, sendLevel, sendSource, [this](uint8_t propId)
	{
		return this->getProp((PlayerProp)propId);
	});

	// Send the buffers out.
	if (sendAll.length() > 0)
		m_server->sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)this->m_id << sendAll, { m_id });

	auto player = std::dynamic_pointer_cast<PlayerClient>(shared_from_this());
	if (player != nullptr && sendLevel.length() > 0)
		m_server->sendPacketToLevelArea(CString() >> (char)PLO_OTHERPLPROPS >> (short)this->m_id << sendLevel, player, { m_id });

	if (sendSource.length() > 0)
		sendPacket(CString() >> (char)PLO_PLAYERPROPS << sendSource);
}

void Player::setPropsFromRCPacket(CString& pPacket, Player* rc)
{
	bool hadBomb = false, hadBow = false;
	CString outPacket;

	// Skip playerworld
	pPacket.readChars(pPacket.readGUChar());

	// Read props from the packet.
	CString props = pPacket.readChars(pPacket.readGUChar());

	// Send props out.
	setPropsFromPacket(props, props::SetBy::SERVER, rc);

	// Clear flags
	for (const auto& [flag, value] : account.variables.store)
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
	auto flagCount = pPacket.readGUShort();
	while (flagCount-- > 0)
	{
		CString flag = pPacket.readChars(pPacket.readGUChar());
		std::string name = flag.readString("=").toString();
		std::string val = flag.readString("").toString();

		if (val.empty())
			setFlag(name, std::nullopt, isLoaded());
		else setFlag(name, val, isLoaded());
	}

	// Clear the chests and re-populate the chest list.
	account.savedChests.clear();
	auto chestCount = pPacket.readGUShort();
	while (chestCount > 0)
	{
		unsigned char len = pPacket.readGUChar();
		char loc[2] = { pPacket.readGChar(), pPacket.readGChar() };
		std::string level = pPacket.readChars(len - 2).toString();

		account.savedChests.insert(std::make_pair(level, std::make_pair(loc[0], loc[1])));
		--chestCount;
	}

	// Re-populate the weapons list.
	auto weaponCount = pPacket.readGUChar();
	while (weaponCount > 0)
	{
		unsigned char len = pPacket.readGUChar();
		if (len == 0) continue;
		CString wpn = pPacket.readChars(len);

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
		if (auto player = std::dynamic_pointer_cast<PlayerClient>(shared_from_this()); player != nullptr)
			player->warp(account.level, { static_cast<int16_t>(getX() * 16), static_cast<int16_t>(getY() * 16) }, 0);
	}
}

CString Player::getPropsPacketFromList(const PropList& props) const
{
	CString propPacket;

	// Create Props
	for (int i = 0; i < PLAYERPROP_COUNT; ++i)
	{
		if (props[i])
		{
			auto prop = getProp(static_cast<PlayerProp>(i));
			propPacket >> (char)i << prop->serialize();
		}
	}

	if (m_isExternal)
		propPacket >> (char)PlayerProp::PLAYERLISTCATEGORY >> (char)PlayerListCategory::EXTERNAL;

	return propPacket;
}

CString Player::getPropsForRCPacket()
{
	CString ret;
	ret >> (char)account.name.length() << account.name;
	ret >> (char)4 << "main"; // worldName

	// Add the props.
	CString props = getPropsPacketFromList(clientPropsForRCView);
	ret >> (char)props.length() << props;

	// Add the player's flags.
	ret >> (short)account.variables.store.size();
	for (const auto& [flag, value] : account.variables.store)
	{
		if (auto computedFlag = account.variables.serializeModern(flag); computedFlag.has_value())
			ret >> (char)(std::min((size_t)223, computedFlag.value().length())) << computedFlag.value().substr(0, 223);
	}

	// Add the player's chests.
	ret >> (short)account.savedChests.size();
	for (const auto& [level, loc] : account.savedChests)
	{
		ret >> (char)(level.length() + 2) >> (char)loc.first >> (char)loc.second << level;
	}

	// Add the player's weapons.
	ret >> (char)account.weapons.size();
	for (const auto& weapon : account.weapons)
		ret >> (char)weapon.length() << weapon;

	return ret;
}

CString Player::getModifiedPropsPacket() const
{
	CString result;
	for (auto i = 0; i < PLAYERPROP_COUNT; ++i)
	{
		if (modTime[i] != m_savedModTime[i])
		{
			auto prop = getProp((PlayerProp)i);
			CString data = prop->serialize();
			result >> (char)i << data;
		}
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
