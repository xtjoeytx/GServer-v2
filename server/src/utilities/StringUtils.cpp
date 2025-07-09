#include <cstddef>
#include <string>
#include <vector>

#include <CString.h>

#include <utilities/StringUtils.h>

////////////////////////////////////////////////////////////////////////////////
namespace utilities
{
////////////////////////////////////////////////////////////////////////////////

std::string retokenizeArray(const std::vector<CString>& triggerData, int start_idx)
{
	std::string ret;
	for (size_t i = start_idx; i < triggerData.size(); i++)
	{
		if (!ret.empty())
			ret.append(",");

		ret.append(triggerData[i].gtokenize().toString());
	}

	return ret;
}

CString retokenizeCStringArray(const std::vector<CString>& triggerData, int start_idx)
{
	CString ret;
	for (size_t i = start_idx; i < triggerData.size(); i++)
	{
		if (!ret.isEmpty())
			ret << ",";

		ret << triggerData[i].gtokenize();
	}

	return ret;
}

////////////////////////////////////////////////////////////////////////////////
} // end namespace utilities
