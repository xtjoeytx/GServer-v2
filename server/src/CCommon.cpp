#include "CCommon.h"
#include "IEnums.h"

const CString CCommon::triggerAction(char targetX, char targetY, CString action, CString weapon, CString args)
{
	return CString() >> char(PLO_TRIGGERACTION) >> short(0) >> int(0) >> char(targetX) >> char(targetY) << action.gtokenize() << "," << weapon.gtokenize() << "," << args;
}