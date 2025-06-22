#ifndef LEVELLINK_H
#define LEVELLINK_H

#include <memory>
#include <vector>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

#include <CString.h>

#include <object/Character.h>
#include <utilities/CommonTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class LevelLink : public std::enable_shared_from_this<LevelLink>
{
public:
	// constructor - destructor
	LevelLink() = default;
	LevelLink(const std::vector<CString>& pLink);

	// functions
	CString getLinkStr() const;
	void parseLinkStr(const std::vector<CString>& pLink);

public:
	[[inline]] const Rectangle<uint8_t, uint8_t>& getBoundingBox() const;
	[[inline]] void setX(uint8_t posX = 0);
	[[inline]] void setY(uint8_t posY = 0);
	[[inline]] void setWidth(uint8_t width = 0);
	[[inline]] void setHeight(uint8_t height = 0);

public:
	Position<int16_t> getDestinationForCharacter(Character& character) const;
	[[inline]] const std::string& getDestinationLevel() const;
	[[inline]] const std::string& getDestinationX() const;
	[[inline]] const std::string& getDestinationY() const;
	[[inline]] void setDestinationLevel(std::string_view level);
	[[inline]] void setDestinationX(std::string_view newX);
	[[inline]] void setDestinationY(std::string_view newY);

public:
	bool isProbableMapLink() const;

private:
	std::string m_destinationLevel, m_destinationX, m_destinationY;
	Rectangle<uint8_t, uint8_t> m_boundingBox;
	bool m_constantX = false;
	bool m_constantY = false;
};

using LevelLinkPtr = std::shared_ptr<LevelLink>;

/*
	LevelLink: Get Private Variables
*/
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

inline void LevelLink::setDestinationLevel(std::string_view level)
{
	m_destinationLevel = level;
}

inline void LevelLink::setDestinationX(std::string_view newX)
{
	m_destinationX = newX;
}

inline void LevelLink::setDestinationY(std::string_view newY)
{
	m_destinationY = newY;
}

inline void LevelLink::setX(uint8_t posX)
{
	std::get<0>(m_boundingBox.position.data) = posX;
}

inline void LevelLink::setY(uint8_t posY)
{
	std::get<1>(m_boundingBox.position.data) = posY;
}

inline void LevelLink::setWidth(uint8_t width)
{
	std::get<0>(m_boundingBox.size.data) = width;
}

inline void LevelLink::setHeight(uint8_t height)
{
	std::get<1>(m_boundingBox.size.data) = height;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELLINK_H
