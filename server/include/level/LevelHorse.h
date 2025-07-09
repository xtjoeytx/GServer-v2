#ifndef LEVELHORSE_H
#define LEVELHORSE_H

#include <cstdint>
#include <string>

#include <CString.h>

#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/TimeoutGenerator.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

struct LevelHorse
{
	float getTileX() const { return position.x() / 16.0f; }
	float getTileY() const { return position.y() / 16.0f; }

	CString getPacket() const
	{
		char dir_bush = (bushes << 2) | (direction & 0x03);
		return  CString() << (char)(position.x() / 8) >> (char)(position.y() / 8) >> (char)dir_bush << image;
	}

	PixelPosition position;
	std::string image;
	uint8_t direction;
	uint8_t bushes;
	TimeoutGenerator timeout;

	[[inline]] void constructScriptParameters();
	string_map<GameVariable> scriptParameters;
};

//----------------------------

inline void LevelHorse::constructScriptParameters()
{
	scriptParameters.try_emplace("x", set_temporary, "x", gameVariableGetter([this]() { return position.x() / 16.0; }), GameVariable::func_set{});
	scriptParameters.try_emplace("y", set_temporary, "y", gameVariableGetter([this]() { return position.y() / 16.0; }), GameVariable::func_set{});
	scriptParameters.try_emplace("dir", set_temporary, "dir", gameVariableGetter([this]() { return (double)direction; }), GameVariable::func_set{});
	scriptParameters.try_emplace("bushes", set_temporary, "bushes", gameVariableGetter([this]() { return (double)bushes; }), GameVariable::func_set{});
	//scriptParameters.try_emplace("bombs", set_temporary, "bombs", gameVariableGetter([this]() { return (double)direction; }), GameVariable::func_set{});
	//scriptParameters.try_emplace("bombpower", set_temporary, "bombpower", gameVariableGetter([this]() { return (double)direction; }), GameVariable::func_set{});
	//scriptParameters.try_emplace("type", set_temporary, "type", gameVariableGetter([this]() { return (double)direction; }), GameVariable::func_set{});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELHORSE_H
