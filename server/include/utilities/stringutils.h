#ifndef UTILITIES_STRINGUTILS_H
#define UTILITIES_STRINGUTILS_H

#pragma once

#include <string>
#include <vector>
#include "CString.h"

namespace utilities
{
	std::string retokenizeArray(const std::vector<CString>& triggerData, int start_idx = 0);
}

#endif
