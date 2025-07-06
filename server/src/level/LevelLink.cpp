#include <cstdint>
#include <format>
#include <vector>

#include <CString.h>

#include <level/LevelLink.h>
#include <object/Character.h>
#include <utilities/CommonTypes.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

LevelLink::LevelLink(const std::vector<CString>& pLink)
{
	parseLinkStr(pLink);
}

CString LevelLink::getLinkStr() const
{
	return std::format("{} {} {} {} {} {} {}", m_destinationLevel,
		m_boundingBox.position.x(), m_boundingBox.position.y(),
		m_boundingBox.size.width(), m_boundingBox.size.height(),
		m_destinationX, m_destinationY
	);
}

void LevelLink::parseLinkStr(const std::vector<CString>& pLink)
{
	size_t offset = 0;

	// Find the whole level name.
	m_destinationLevel = pLink[0];
	if (pLink.size() > 7)
	{
		offset = pLink.size() - 7;
		for (size_t i = 0; i < offset; ++i)
		{
			m_destinationLevel += " ";
			m_destinationLevel += pLink[1 + i];
		}
	}

	m_boundingBox =
	{
		{ static_cast<uint8_t>(string::toNumber(pLink[1 + offset].toString())), static_cast<uint8_t>(string::toNumber(pLink[2 + offset].toString())) },
		{ static_cast<uint8_t>(string::toNumber(pLink[3 + offset].toString())), static_cast<uint8_t>(string::toNumber(pLink[4 + offset].toString())) }
	};
	m_destinationX = pLink[5 + offset].toString();
	m_destinationY = pLink[6 + offset].toString();

	// TODO: We need better handling of ancient level links that don't use math.

	if (m_destinationX == "-1")
	{
		m_constantX = true;
		m_destinationX = "playerx";
	}

	if (m_destinationY == "-1")
	{
		m_constantY = true;
		m_destinationY = "playery";
	}
}

//----------------------------

Position<int16_t> LevelLink::getDestinationForCharacter(Character& character) const
{
	// TODO: Level links can use math, so we need to eventually throw this into the script engine.  Yikes.
	Position<int16_t> result;
	auto& [x, y] = result.data;

	x = (m_destinationX == "playerx" ? character.pixelX : static_cast<int16_t>(string::toFloat(m_destinationX) * 16));
	y = (m_destinationY == "playery" ? character.pixelY : static_cast<int16_t>(string::toFloat(m_destinationY) * 16));

	return result;
}

//----------------------------

bool LevelLink::isProbableMapLink() const
{
	if ((m_boundingBox.position.x() == 0 || m_boundingBox.position.x() == 63) && m_boundingBox.size.width() == 1 && m_destinationY == "playery")
		return true;
	if ((m_boundingBox.position.y() == 0 || m_boundingBox.position.y() == 63) && m_boundingBox.size.height() == 1 && m_destinationX == "playerx")
		return true;
	return false;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
