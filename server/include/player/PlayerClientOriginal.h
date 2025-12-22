#ifndef PLAYERCLIENTORIGINAL_H
#define PLAYERCLIENTORIGINAL_H

#include <chrono>
#include <memory>
#include <optional>

#include <CSocket.h>

#include <level/Level.h>
#include <object/Player.h>
#include <player/PlayerClient.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>

using namespace std::literals::string_view_literals;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class PlayerClientOriginal : public PlayerClient
{
public:
	PlayerClientOriginal(CSocket* pSocket, PlayerID pId);
	virtual ~PlayerClientOriginal();

public:
	// Forcibly move a player (the client doesn't know it is transitioning levels).
	virtual bool warp(std::shared_ptr<Level> level, const PixelPosition& position, std::optional<clock::time_point> clientCachedTime = std::nullopt) override;

	// Place the player in a new level (the client knows it is transitioning levels).
	virtual bool enterLevel(std::shared_ptr<Level> level, std::optional<clock::time_point> clientCachedTime = std::nullopt) override;
	using Player::enterLevel;

	virtual bool sendStaticLevelData(std::shared_ptr<StaticLevelData> staticLevelData, std::shared_ptr<SubLevel> subLevel, std::optional<clock::time_point> clientCachedTime = std::nullopt) override;
	virtual bool sendDynamicLevelData(std::shared_ptr<Level> level, std::optional<clock::time_point> clientCachedTime = std::nullopt) override;

protected:
	bool m_firstLevel = false;
};

using PlayerClientOriginalPtr = std::shared_ptr<PlayerClientOriginal>;

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // PLAYERCLIENTORIGINAL_H
