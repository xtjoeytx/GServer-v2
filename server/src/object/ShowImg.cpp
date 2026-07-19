#include <algorithm>
#include <chrono>
#include <cstdint>
#include <format>
#include <optional>
#include <ranges>
#include <string_view>
#include <string>
#include <utility>
#include <vector>

#include <CString.h>

#include <object/ShowImg.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/PropertySerializers.h>

////////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
////////////////////////////////////////////////////////////////////////////////

ShowImg ShowImg::ConstructImage(clock::time_point modTime, const PixelPosition& position, std::string_view image) noexcept
{
	ShowImg showimg{ .image = std::string{ image }, .position = position };
	showimg.modTime.fill(std::nullopt);
	showimg.modTime[PROPID(ShowImgProp::IMAGE)] = modTime;
	showimg.modTime[PROPID(ShowImgProp::X)] = modTime;
	showimg.modTime[PROPID(ShowImgProp::Y)] = modTime;
	if (position.z() != 0)
		showimg.modTime[PROPID(ShowImgProp::Z)] = modTime;

	return showimg;
}

ShowImg ShowImg::ConstructText(clock::time_point modTime, const PixelPosition& position, std::string_view text, std::string_view font, std::string_view style) noexcept
{
	// Construct the formatted text string.
	std::string formattedTextString;
	if (!font.empty())
	{
		formattedTextString += "@";
		formattedTextString += font;
		if (!style.empty())
		{
			formattedTextString += "@";
			formattedTextString += style;
		}
	}
	formattedTextString += "@";
	formattedTextString += text;

	// Create the showimg.
	ShowImg showimg{ .image = std::move(formattedTextString), .position = position };
	showimg.modTime.fill(std::nullopt);
	showimg.modTime[PROPID(ShowImgProp::IMAGE)] = modTime;
	showimg.modTime[PROPID(ShowImgProp::X)] = modTime;
	showimg.modTime[PROPID(ShowImgProp::Y)] = modTime;
	if (position.z() != 0)
		showimg.modTime[PROPID(ShowImgProp::Z)] = modTime;

	return showimg;
}

ShowImg ShowImg::ConstructGani(clock::time_point modTime, const PixelPosition& position, std::string_view animation, uint8_t direction) noexcept
{
	// Create the showimg.
	ShowImg showimg{ .image = std::format("&{},{}", direction, animation), .position = position };
	showimg.modTime.fill(std::nullopt);
	showimg.modTime[PROPID(ShowImgProp::IMAGE)] = modTime;
	showimg.modTime[PROPID(ShowImgProp::X)] = modTime;
	showimg.modTime[PROPID(ShowImgProp::Y)] = modTime;
	if (position.z() != 0)
		showimg.modTime[PROPID(ShowImgProp::Z)] = modTime;

	return showimg;
}

ShowImg ShowImg::ConstructPoly(clock::time_point modTime, const std::vector<double>& points) noexcept
{
	std::string polygon{ "#2" };
	for (const auto& point : points)
	{
		polygon += std::format(",{:.0f}", point);
	}

	// Create the showimg.
	ShowImg showimg{ .image = std::move(polygon), .position = { 0_i32, 0_i32 } };
	showimg.modTime.fill(std::nullopt);
	showimg.modTime[PROPID(ShowImgProp::IMAGE)] = modTime;
	showimg.modTime[PROPID(ShowImgProp::X)] = modTime;
	showimg.modTime[PROPID(ShowImgProp::Y)] = modTime;

	return showimg;
}

//----------------------------

void ShowImg::processProps(CString& props)
{
	while (props.bytesLeft() > 0)
	{
		uint8_t propId = props.readGUChar();
		ShowImgProp prop = static_cast<ShowImgProp>(propId);
		switch (prop)
		{
			case ShowImgProp::IMAGE:
			{
				props::PropertyString prop;
				prop.deserialize(props);

				image = prop.value;
				break;
			}

			case ShowImgProp::X:
			{
				props::PropertyTileCoordinate prop;
				prop.deserialize(props);

				position.x() = prop.pixelCoordinate;
				break;
			}

			case ShowImgProp::Y:
			{
				props::PropertyTileCoordinate prop;
				prop.deserialize(props);

				position.y() = prop.pixelCoordinate;
				break;
			}

			case ShowImgProp::LAYER:
			{
				props::PropertyNumeric<props::GBYTE1> prop;
				prop.deserialize(props);

				layer = prop.value;
				break;
			}

			case ShowImgProp::IMAGEPART:
			{
				props::PropertyImagePart prop;
				prop.deserialize(props);

				imagePart = prop.imagePart;
				break;
			}

			case ShowImgProp::COLORS:
			{
				props::PropertyArray<props::GBYTE1, 4> prop;
				prop.deserialize(props);

				std::ranges::transform(prop.values | std::views::take(4), colors.begin(), [](props::GBYTE1 value)
				{
					return static_cast<float>(value) / 200.0f;
				});
				break;
			}

			case ShowImgProp::ZOOM:
			{
				props::PropertyNumeric<props::GBYTE1> prop;
				prop.deserialize(props);

				zoom = prop.value / 10.0f;
				break;
			}

			case ShowImgProp::Z:
			{
				props::PropertyTileCoordinateZ prop;
				prop.deserialize(props);

				position.z() = prop.pixelCoordinate;
				break;
			}

			case ShowImgProp::DRAWMODE:
			{
				props::PropertyNumeric<props::GBYTE1> prop;
				prop.deserialize(props);

				drawMode = prop.value;
				break;
			}
		}
	}
}

CString ShowImg::getPropPacket(ShowImgProp prop) const
{
	switch (prop)
	{
		case ShowImgProp::IMAGE:
		{
			props::PropertyString prop{ image };
			return prop.serialize();
		}

		case ShowImgProp::X:
		{
			auto localPosition = toLocalPixelPosition(position);
			props::PropertyTileCoordinate prop{ localPosition.x() };
			return prop.serialize();
		}

		case ShowImgProp::Y:
		{
			auto localPosition = toLocalPixelPosition(position);
			props::PropertyTileCoordinate prop{ localPosition.y() };
			return prop.serialize();
		}

		case ShowImgProp::LAYER:
		{
			props::PropertyNumeric<props::GBYTE1> prop{ layer };
			return prop.serialize();
		}

		case ShowImgProp::IMAGEPART:
		{
			if (imagePart.size.width() == 0 && imagePart.size.height() == 0)
				return CString() >> (char)0;

			props::PropertyImagePart prop{ imagePart };
			return CString() >> (char)1 << prop.serialize();
		}

		case ShowImgProp::COLORS:
		{
			auto toByte = [](float value)
			{
				return static_cast<props::GBYTE1>(std::clamp(value, 0.0f, 1.0f) * 200.0f);
			};
			props::PropertyArray<props::GBYTE1, 4> prop{ colors | std::views::transform(toByte) };
			return prop.serialize();
		}

		case ShowImgProp::ZOOM:
		{
			props::PropertyNumeric<props::GBYTE1> prop{ static_cast<props::GBYTE1>(zoom * 10.0f) };
			return prop.serialize();
		}

		case ShowImgProp::Z:
		{
			props::PropertyTileCoordinateZ prop{ static_cast<int16_t>(position.z()) };
			return prop.serialize();
		}

		case ShowImgProp::DRAWMODE:
		{
			props::PropertyNumeric<props::GBYTE1> prop{ drawMode };
			return prop.serialize();
		}
	}

	return CString();
}

CString ShowImg::getAllPropsPacket(std::optional<clock::time_point> newTime) const
{
	CString result;

	for (uint8_t i = 0; i < SHOWIMGPROP_COUNT; ++i)
	{
		if (modTime[i].has_value() && modTime[i] >= newTime)
		{
			auto prop = static_cast<ShowImgProp>(i);
			result >> (char)i << getPropPacket(prop);
		}
	}

	return result;
}

CString ShowImg::getModifiedPropsPacket() const
{
	CString result;

	for (uint8_t i = 0; i < SHOWIMGPROP_COUNT; ++i)
	{
		if (modTime[i] != savedModTime[i])
		{
			auto prop = static_cast<ShowImgProp>(i);
			result >> (char)i << getPropPacket(prop);
		}
	}

	return result;
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
