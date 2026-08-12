#ifndef LEVELLINK_H
#define LEVELLINK_H

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <CString.h>

#include <object/Character.h>
#include <scripting/ScriptTypes.h>
#include <utilities/Extents.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class Server;

class LevelLink
{
public:
	LevelLink() = default;
	explicit LevelLink(const std::vector<CString>& pLink);
	LevelLink(const Rectangle<uint8_t, uint8_t>& coordinates, std::string_view destinationX, std::string_view destinationY, std::string_view destinationLevel);

public:
	[[nodiscard]] CString getLinkStr() const;
	void parseLinkStr(const std::vector<CString>& pLink);

public:
	[[nodiscard]] [[a::inline]] const Rectangle<uint8_t, uint8_t>& getBoundingBox() const;
	[[a::inline]] void setX(uint8_t posX = 0);
	[[a::inline]] void setY(uint8_t posY = 0);
	[[a::inline]] void setWidth(uint8_t width = 0);
	[[a::inline]] void setHeight(uint8_t height = 0);

public:
	[[nodiscard]] LocalPixelPosition getDestinationForCharacter(const Character& character, const ScriptObject& source) const;
	[[nodiscard]] [[a::inline]] const std::string& getDestinationLevel() const;
	[[nodiscard]] [[a::inline]] const std::string& getDestinationX() const;
	[[nodiscard]] [[a::inline]] const std::string& getDestinationY() const;
	[[a::inline]] void setDestinationLevel(std::string_view level);
	[[a::inline]] void setDestinationX(std::string_view newX);
	[[a::inline]] void setDestinationY(std::string_view newY);

public:
	[[nodiscard]] bool isProbableMapLink() const;

private:
	Server* m_server = nullptr;
	std::string m_destinationLevel, m_destinationX, m_destinationY;
	Rectangle<uint8_t, uint8_t> m_boundingBox;
	bool m_constantX = false;
	bool m_constantY = false;
	std::array<bool, 2> m_complex{false, false};
};

//----------------------------

inline const std::string& LevelLink::getDestinationLevel() const
{
	return m_destinationLevel;
}

inline const std::string& LevelLink::getDestinationX() const
{
	return m_destinationX;
}

inline const std::string& LevelLink::getDestinationY() const
{
	return m_destinationY;
}

inline const Rectangle<uint8_t, uint8_t>& LevelLink::getBoundingBox() const
{
	return m_boundingBox;
}

inline void LevelLink::setDestinationLevel(const std::string_view level)
{
	m_destinationLevel = level;
}

inline void LevelLink::setDestinationX(const std::string_view newX)
{
	m_destinationX = newX;
}

inline void LevelLink::setDestinationY(const std::string_view newY)
{
	m_destinationY = newY;
}

inline void LevelLink::setX(const uint8_t posX)
{
	m_boundingBox.position.x() = posX;
}

inline void LevelLink::setY(const uint8_t posY)
{
	m_boundingBox.position.y() = posY;
}

inline void LevelLink::setWidth(const uint8_t width)
{
	m_boundingBox.size.width() = width;
}

inline void LevelLink::setHeight(const uint8_t height)
{
	m_boundingBox.size.height() = height;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELLINK_H
