#include "TLevelHorse.h"
#include "TServer.h"

TLevelHorse::TLevelHorse(TServer* server, const CString& pImage, float pX, float pY, char pDir, char pBushes)
: image(pImage), x(pX), y(pY), dir(pDir), bushes(pBushes)
{
	timeout.setTimeout(server->getSettings()->getInt("horselifetime", 30));
}

CString TLevelHorse::getHorseStr() const
{
	char dir_bush = (bushes << 2) | (dir & 0x03);
	return CString() >> (char)(x * 2) >> (char)(y * 2) >> (char)dir_bush << image;
}
