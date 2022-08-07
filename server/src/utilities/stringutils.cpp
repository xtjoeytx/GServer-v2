#include "stringutils.h"

namespace utilities
{
	std::string retokenizeArray(const std::vector<CString>& triggerData, int start_idx)
	{
		std::string ret;
		for (auto i = start_idx; i < triggerData.size(); i++)
		{
			if (!ret.empty())
				ret.append(",");

			ret.append(triggerData[i].gtokenize().toString());
		}

		return ret;
	}
}
