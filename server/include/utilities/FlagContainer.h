#ifndef FLAGCONTAINER_H
#define FLAGCONTAINER_H

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include "utilities/StringUtils.h"

using namespace std::literals;

///////////////////////////////////////////////////////////////////////////////

namespace preagonal
{

///////////////////////////////////////////////////////////////////////////////

using flagPair = std::pair<std::string, std::string>;

struct FlagContainer
{
	std::unordered_map<std::string, std::string, string::string_hash, std::equal_to<>> container;

	bool has(std::string_view flag) const
	{
		return container.contains(flag);
	}

	std::string get(std::string_view flag) const
	{
		if (auto it = container.find(flag); it != container.end())
			return it->second;
		return {};
	}

	void set(std::string_view flag, std::string_view value = {})
	{
		if (auto it = container.find(flag); it != container.end())
			it->second = value;
		else
			container.insert_or_assign(std::string{ flag }, std::string{ value });
	}

	void set(const flagPair& flag)
	{
		if (auto it = container.find(flag.first); it != container.end())
			it->second = flag.second;
		else
			container.insert(flag);
	}

	void set(flagPair&& flag)
	{
		if (auto it = container.find(flag.first); it != container.end())
			it->second = std::move(flag.second);
		else
			container.insert(std::move(flag));
	}

	bool remove(std::string_view flag)
	{
		if (auto it = container.find(flag); it != container.end())
		{
			container.erase(it);
			return true;
		}
		return false;
	}

	void clear()
	{
		container.clear();
	}
};

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal

#endif // FLAGCONTAINER_H
