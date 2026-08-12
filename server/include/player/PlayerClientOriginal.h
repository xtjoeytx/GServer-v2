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
	~PlayerClientOriginal() override = default;

public:
	// Forcibly move a player (the client doesn't know it is transitioning levels).
	bool warp(const std::shared_ptr<Level>& level, const PixelPosition& position, std::optional<clock::time_point> clientCachedTime) override;
	using Player::warp;

	// Place the player in a new level (the client knows it is transitioning levels).
	bool enterLevel(const std::shared_ptr<Level>& level, std::optional<clock::time_point> clientCachedTime) override;
	using Player::enterLevel;

	using Player::leaveLevel;

	bool sendStaticLevelData(const std::shared_ptr<StaticLevelData>& staticLevelData, const std::shared_ptr<SubLevel>& subLevel, std::optional<clock::time_point> clientCachedTime) override;
	bool sendDynamicLevelData(const std::shared_ptr<Level>& level, std::optional<clock::time_point> clientCachedTime) override;

protected:
	bool m_firstLevel = false;
};

using PlayerClientOriginalPtr = std::shared_ptr<PlayerClientOriginal>;

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // PLAYERCLIENTORIGINAL_H
