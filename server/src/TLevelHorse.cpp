#include "TLevelHorse.h"

CString TLevelHorse::getHorseStr() const
{
	char dir_bush = (bushes << 2) | (dir & 0x03);
	return CString() >> (char)x >> (char)y >> (char)dir_bush << image;
}
