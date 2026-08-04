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
	CString getSignPacket(Player* player = nullptr) const;
	void setText(std::string_view signText, bool signTextIsEncoded = false);

	float getTileX() const { return (float)position.x(); }
	float getTileY() const { return (float)position.y(); }

	LocalWholeTilePosition position;
	std::string text;
	std::string encodedText;

	[[a::inline]] void constructScriptParameters();
	string_map<GameVariable> scriptParameters;
};

//----------------------------

inline void LevelSign::constructScriptParameters()
{
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{"x"sv, std::nullopt, std::ref(position.x()), 16});
	bind::bindPropertyAsReadOnly(scriptParameters, bind::DivideByIntegralProperty{"y"sv, std::nullopt, std::ref(position.y()), 16});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELSIGN_H
