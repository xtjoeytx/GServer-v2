#ifndef UTILITIES_STRINGUTILS_H
#define UTILITIES_STRINGUTILS_H

#include "CString.h"
#include <string>
#include <vector>

namespace utilities
{
	std::string retokenizeArray(const std::vector<CString>& triggerData, int start_idx = 0);
	CString retokenizeCStringArray(const std::vector<CString>& triggerData, int start_idx = 0);
} // namespace utilities

#endif
