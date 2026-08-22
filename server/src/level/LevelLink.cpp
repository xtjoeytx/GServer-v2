#include <cstdint>
#include <format>
#include <string_view>
#include <vector>

#include <CString.h>

#include <BabyDI.h>
#include <Server.h>
#include <level/LevelLink.h>
#include <npcserver/NPCServer.h>
#include <object/Character.h>
#include <scripting/IScriptEngine.h>
#include <scripting/ScriptTypes.h>
#include <scripting/gs1/ScriptEngineGS1.h>
#include <utilities/Extents.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

static bool checkIfComplex(const std::string_view destination)
{
	if (destination != "playerx" && destination != "playery" && !string::isFloat(destination))
		return true;
	return false;
}

static int16_t getPixelDestination(const std::string_view destination, const Character& character)
{
	if (destination == "playerx")
		return character.localPixelX;
	if (destination == "playery")
		return character.localPixelY;
	return static_cast<int16_t>(string::toFloat(destination) * 16);
}

///////////////////////////////////////////////////////////////////////////////

LevelLink::LevelLink(const std::vector<CString>& pLink)
{
	parseLinkStr(pLink);

	m_server = BabyDI::Get<Server>();
	m_complex[0] = checkIfComplex(m_destinationX);
	m_complex[1] = checkIfComplex(m_destinationY);
}

LevelLink::LevelLink(const Rectangle<uint8_t, uint8_t>& coordinates, const std::string_view destinationX, const std::string_view destinationY, const std::string_view destinationLevel)
	: m_destinationLevel{destinationLevel}, m_destinationX{destinationX}, m_destinationY{destinationY}, m_boundingBox{coordinates}
{
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

	m_server = BabyDI::Get<Server>();
	m_complex[0] = checkIfComplex(m_destinationX);
	m_complex[1] = checkIfComplex(m_destinationY);
}

CString LevelLink::getLinkStr() const
{
	return std::format("{} {} {} {} {} {} {}", m_destinationLevel, m_boundingBox.position.x(), m_boundingBox.position.y(), m_boundingBox.size.width(), m_boundingBox.size.height(), m_destinationX, m_destinationY);
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
			m_destinationLevel += ' ';
			m_destinationLevel += pLink[1 + i];
		}
	}

	m_boundingBox =
	{
		{static_cast<uint8_t>(string::toNumber(pLink[1 + offset].toString())), static_cast<uint8_t>(string::toNumber(pLink[2 + offset].toString()))},
		{static_cast<uint8_t>(string::toNumber(pLink[3 + offset].toString())), static_cast<uint8_t>(string::toNumber(pLink[4 + offset].toString()))}
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

LocalPixelPosition LevelLink::getDestinationForCharacter(const Character& character, const ScriptObject& source) const
{
	// If the link is complex and we have an NPC server, process the link via the scripting system.
	if ((m_complex[0] || m_complex[1]) && m_server && m_server->hasNPCServer())
	{
		const auto& npcServer = m_server->getNPCServer();
		if (const auto gs1 = npcServer->scripting.getScriptEngine(gs1::ScriptEngineGS1::EngineName); gs1 != nullptr)
		{
			const auto x = m_complex[0] ? static_cast<int16_t>(gs1->processMathExpression(m_destinationX, source).value_or(0.0) * 16) : getPixelDestination(m_destinationX, character);
			const auto y = m_complex[1] ? static_cast<int16_t>(gs1->processMathExpression(m_destinationY, source).value_or(0.0) * 16) : getPixelDestination(m_destinationY, character);
			return LocalPixelPosition{x, y};
		}
	}

	// If not complex, or in classic mode, we can just return the result without doing any math or scripting.
	const LocalPixelPosition result =
	{
		getPixelDestination(m_destinationX, character),
		getPixelDestination(m_destinationY, character)
	};
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
