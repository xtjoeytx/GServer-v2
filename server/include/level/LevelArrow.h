#ifndef LEVELARROW_H
#define LEVELARROW_H

#include <cstdint>

#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

struct LevelArrow
{
	float getTileX() const { return position.x() / 16.0f; }
	float getTileY() const { return position.y() / 16.0f; }

	PixelPosition position;
	PixelPosition speed;
	uint8_t direction;
	int8_t type;
	uint8_t from;

	[[inline]] void constructScriptParameters();
	string_map<GameVariable> scriptParameters;
};

//----------------------------

inline void LevelArrow::constructScriptParameters()
{
	scriptParameters.try_emplace("x", set_temporary, "x", gameVariableGetter([this]() { return position.x() / 16.0; }), GameVariable::func_set{});
	scriptParameters.try_emplace("y", set_temporary, "y", gameVariableGetter([this]() { return position.y() / 16.0; }), GameVariable::func_set{});
	scriptParameters.try_emplace("dx", set_temporary, "dx", gameVariableGetter([this]() { return speed.x() / 16.0; }), GameVariable::func_set{});
	scriptParameters.try_emplace("dy", set_temporary, "dy", gameVariableGetter([this]() { return speed.y() / 16.0; }), GameVariable::func_set{});
	scriptParameters.try_emplace("dir", set_temporary, "dir", gameVariableGetter([this]() { return (double)direction; }), GameVariable::func_set{});
	scriptParameters.try_emplace("type", set_temporary, "type", gameVariableGetter([this]() { return (double)type; }), GameVariable::func_set{});
	scriptParameters.try_emplace("from", set_temporary, "from", gameVariableGetter([this]() { return (double)from; }), GameVariable::func_set{});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELARROW_H
