#ifndef LEVELSIGN_H
#define LEVELSIGN_H

#include <string>
#include <string_view>

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

	[[inline]] void constructScriptParameters();
	string_map<GameValue> scriptParameters;
};

//----------------------------

inline void LevelSign::constructScriptParameters()
{
	scriptParameters.try_emplace("x", set_temporary, "x", gameValueGetter([this]() { return position.x() / 16.0; }), GameValue::func_set{});
	scriptParameters.try_emplace("y", set_temporary, "y", gameValueGetter([this]() { return position.y() / 16.0; }), GameValue::func_set{});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELSIGN_H
