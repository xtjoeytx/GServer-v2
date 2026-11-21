#ifndef GAMEANI_H
#define GAMEANI_H

#include <cstdint>
#include <optional>
#include <string>

#include <scripting/Script.h>
#include <scripting/ScriptSystem.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class Server;

// SPRITE  ID  SHEET  X  Y  W  H  DESC
namespace GaniSheets
{
constexpr auto BODY = "BODY"sv;
constexpr auto HEAD = "HEAD"sv;
constexpr auto SHIELD = "SHIELD"sv;
constexpr auto SWORD = "SWORD"sv;
constexpr auto HORSE = "HORSE"sv;
constexpr auto SPRITES = "SPRITES"sv;
constexpr auto PICS = "PICS"sv;
constexpr auto PARAM = "PARAM"sv;	// setani gani,param1,param2;
constexpr auto ATTR = "ATTR"sv;		// setcharprop #P1,attr1;
} // end namespace GaniSheets

/// @brief Gani animation flags.
enum class GaniFlags
{
	CONTINUOUS = 0,
	LOOP = 1,
	SINGLEDIRECTION = 2,
	COUNT
};
inline constexpr uint8_t GaniFlagsCount = static_cast<uint8_t>(GaniFlags::COUNT);
using GaniFlagsCollection = std::bitset<GaniFlagsCount>;

/// @brief Gani animation.
class GameAni
{
public:
	/// @brief Load a GameAni from the filesystem.
	/// @param server Global server pointer so we can fetch the correct filesystem.
	/// @param name filename of the animation (ex: idle.gani)
	/// @return GameAni if it was successfully loaded, otherwise a nullopt.
	static std::optional<GameAni> load(Server* const server, const std::filesystem::path& name);

public:
	explicit GameAni(std::string_view gani);

	GameAni() = delete;
	GameAni(const GameAni&) = delete;
	GameAni& operator=(const GameAni&) = delete;
	GameAni& operator=(GameAni&& o) = delete;

	GameAni(GameAni&&) noexcept = default;

public:
	GameAni& setScript(std::string_view script);
	void sendByteCodeToPlayer(const std::shared_ptr<Player>& player) const;

public:
	[[nodiscard]] [[a::inline]] bool hasChecksum(uint32_t checksum) const noexcept;
	[[nodiscard]] [[a::inline]] const std::string& getSetBackTo() const noexcept;
	[[nodiscard]] [[a::inline]] bool isContinuous() const noexcept;
	[[nodiscard]] [[a::inline]] bool isLoop() const noexcept;
	[[nodiscard]] [[a::inline]] bool isSingleDir() const noexcept;

public:
	const std::string name;
	clock::time_point modTime;

private:
	std::string m_setBackTo;
	GaniFlagsCollection m_aniFlags;
	Script m_script;
	uint32_t m_plainChecksum = 0;
	uint32_t m_encryptedChecksum = 0;
	CompiledScriptResultPtr m_client_script;
};

//----------------------------

inline bool GameAni::hasChecksum(const uint32_t checksum) const noexcept
{
	return m_plainChecksum == checksum || m_encryptedChecksum == checksum;
}

inline const std::string& GameAni::getSetBackTo() const noexcept
{
	return m_setBackTo;
}

inline bool GameAni::isContinuous() const noexcept
{
	return m_aniFlags.test(ENUM(GaniFlags::CONTINUOUS));
}

inline bool GameAni::isLoop() const noexcept
{
	return m_aniFlags.test(ENUM(GaniFlags::LOOP));
}

inline bool GameAni::isSingleDir() const noexcept
{
	return m_aniFlags.test(ENUM(GaniFlags::SINGLEDIRECTION));
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // GAMEANI_H
