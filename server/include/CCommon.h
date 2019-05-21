#ifndef CCOMMON_H
#define CCOMMON_H
#include <stdarg.h>
#include <stdio.h>
#include <mutex>
#include <condition_variable>
#include "CString.h"

//! Logger class for logging information to a file.
class CCommon
{
public:
	//! Sets the name of the file to write to.
	//! \param targetX X-position to call the triggeraction.
	//! \param targetY Y-position to call the triggeraction.
	//! \param action
	//! \param weapon
	//! \param args
	static const CString triggerAction(char targetX, char targetY, CString action, CString weapon, CString args);
};
#endif