#ifndef LEVELHORSE_H
#define LEVELHORSE_H

#include <cstdint>
#include <string>

#include <CString.h>

#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/generator/TimeoutGenerator.h>

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
		auto localPosition = toLocalPixelPosition(position);
		char dir_bush = (bushes << 2) | (direction & 0x03);
		return CString() >> (char)(localPosition.x() / 8) >> (char)(localPosition.y() / 8) >> (char)dir_bush << image;
	}

	PixelPosition position;
	std::string image;
	uint8_t direction;
	uint8_t bushes;
	TimeoutGenerator timeout;

	[[inline]] void constructScriptParameters();
	string_map<GameValue> scriptParameters;
};

//----------------------------

inline void LevelHorse::constructScriptParameters()
{
	scriptParameters.try_emplace("x", set_temporary, "x", gameValueGetter([this]() { return position.x() / 16.0; }), GameValue::func_set{});
	scriptParameters.try_emplace("y", set_temporary, "y", gameValueGetter([this]() { return position.y() / 16.0; }), GameValue::func_set{});
	scriptParameters.try_emplace("dir", set_temporary, "dir", gameValueGetter(direction), GameValue::func_set{});
	scriptParameters.try_emplace("bushes", set_temporary, "bushes", gameValueGetter(bushes), GameValue::func_set{});
	//scriptParameters.try_emplace("bombs", set_temporary, "bombs", gameValueGetter([this]() { return (double)direction; }), GameValue::func_set{});
	//scriptParameters.try_emplace("bombpower", set_temporary, "bombpower", gameValueGetter([this]() { return (double)direction; }), GameValue::func_set{});
	//scriptParameters.try_emplace("type", set_temporary, "type", gameValueGetter([this]() { return (double)direction; }), GameValue::func_set{});
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // LEVELHORSE_H
