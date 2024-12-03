#ifndef UTILITIES_TIMEUNITS_H
#define UTILITIES_TIMEUNITS_H

#include <cstdint>
#include <ctime>

namespace utilities
{
	struct TimeUnits
	{
		std::time_t days;
		uint8_t hours;
		uint8_t minutes;
		uint8_t seconds;

		TimeUnits(std::time_t time)
		{
			days = time / 86400;
			hours = uint8_t((time / 3600) % 24);
			minutes = uint8_t((time / 60) % 60);
			seconds = uint8_t(time % 60);
		}

		static auto calculate(std::time_t time)
		{
			return TimeUnits{ time };
		}
	};
} // namespace utilities

#endif
