#ifndef SHOWIMG_H
#define SHOWIMG_H

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string_view>
#include <string>
#include <vector>

#include <CString.h>

#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>

////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

// index 0-199 = visible to everybody

/*
showimg colors,layer, and zoom is sent to other players (even
when the layer is 4, then only values between 0 and 440 allowed)

showing text with showimg
you can do showimg 1,@Hello!,x,y; do show text; setting
font and style is possible by doing showimg index,@font@style@text,x,y;
(style: characters from 'bicus', b-bold, i-italic, c-centered,u-underline,
s-strikeout); for change the view of the text use the commands
changeimgzoom,changeimgcolors
*/

/*
	props: {GCHAR prop}{value}
		- max of 9 props

	prop 0: {GSTRING image}
	prop 1: {GCHAR x}
		- if layer is 10, it is computed as val - 64 instead of val - 32, or something like that.
	prop 2: {GCHAR y}
	prop 3: {GCHAR layer}
		- it is 1-indexed in the packet (val - 33)
		- caps at 8 (9 in the packet)
	prop 4: {GSHORT imagePartX}{GSHORT imagePartY}{GCHAR imagePartWidth}{GCHAR imagePartHeight}
	prop 5: {GCHAR red}{GCHAR green}{GCHAR blue}{GCHAR alpha}
		- value / 200.0, for 0..1
	prop 6: {GCHAR zoom}
		- value / 10.0
	prop 7: {GCHAR z}
	prop 8: {GCHAR drawMode}
		- 0 = add, 1 = replace, 2 = subtract, 3 = daynight

	image prop:
		If it starts with @, it is a showtext:  @Font@Style@Text
		If it starts with &, it is a showani:   &dir,gani  e.g.:  &0,skip
		If it starts with :, it is a showpoly:  #2,x1,y1,...,xn,yn  e.g.:  #2,10,10,15,10,15,15,10,15
			Unknown what the 2 is for.

	layer prop:
		0 - under players
		1 - default, same layer as players
		2 - over players
		4 - GUI level
*/

enum class ShowImgProp : uint8_t
{
	IMAGE = 0,
	X = 1,
	Y = 2,
	LAYER = 3,
	IMAGEPART = 4,
	COLORS = 5,
	ZOOM = 6,
	Z = 7,
	DRAWMODE = 8,

	SHOWIMGPROP_COUNT
};
inline constexpr size_t SHOWIMGPROP_COUNT = static_cast<size_t>(ShowImgProp::SHOWIMGPROP_COUNT);

struct ShowImg
{
	std::string image;
	ImagePartRectangle imagePart;
	PixelPosition position;
	float zoom = 1.0f;
	uint8_t drawMode = 0;
	uint8_t layer = 1;
	std::array<float, 4> colors = { 0.0f, 0.0f, 0.0f, 1.0f };
	std::array<std::optional<clock::time_point>, SHOWIMGPROP_COUNT> modTime;
	std::array<std::optional<clock::time_point>, SHOWIMGPROP_COUNT> savedModTime;

	static ShowImg ConstructImage(clock::time_point modTime, const PixelPosition& position, std::string_view image) noexcept;
	static ShowImg ConstructText(clock::time_point modTime, const PixelPosition& position, std::string_view text, std::string_view font = {}, std::string_view style = {}) noexcept;
	static ShowImg ConstructGani(clock::time_point modTime, const PixelPosition& position, std::string_view animation, uint8_t direction) noexcept;
	static ShowImg ConstructPoly(clock::time_point modTime, const std::vector<double>& points) noexcept;

	void processProps(CString& props);
	CString getPropPacket(ShowImgProp prop) const;
	CString getAllPropsPacket(std::optional<clock::time_point> newTime = std::nullopt) const;
	CString getModifiedPropsPacket() const;

	[[inline]] void recordCurrentPropModTime();
};

// ---------------------------

inline void ShowImg::recordCurrentPropModTime()
{
	savedModTime = modTime;
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // SHOWIMG_H
