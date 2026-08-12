#ifndef LEVELSIGN_H
#define LEVELSIGN_H

#include <functional>
#include <optional>
#include <string_view>
#include <string>

#include <CString.h>

#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class Player;

struct LevelSign
{
	LevelSign(const LocalWholeTilePosition& position, std::string_view signText, bool signTextIsEncoded = false);
	CString getSignPacket(const Player* player = nullptr) const;
	void setText(std::string_view signText, bool signTextIsEncoded = false);

	[[a::inline]] float getTileX() const;
	[[a::inline]] float getTileY() const;

	LocalWholeTilePosition position;
	std::string text;
	std::string encodedText;

	[[a::inline]] void constructScriptParameters();
	string_map<GameVariable> scriptParameters;
};

//----------------------------

inline float LevelSign::getTileX() const
{
	return position.x();
}

inline float LevelSign::getTileY() const
{
	return position.y();
}

inline void LevelSign::constructScriptParameters()
{
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{.name = "x"sv, .modTime = std::nullopt, .value = std::ref(position.x()), .factor = 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{.name = "y"sv, .modTime = std::nullopt, .value = std::ref(position.y()), .factor = 16});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELSIGN_H
