#include "utilities/FilePermissions.h"
#include <iostream>
#include <sstream>

const char FOLDER_SEPARATOR = '/';
const std::regex WILDCARD_REGEX(R"(\*)");

std::vector<std::string> splitInput(const std::string& input, const char delimiter = FOLDER_SEPARATOR)
{
	std::istringstream stream(input);
	std::string line;
	std::vector<std::string> lines;
	while (std::getline(stream, line, delimiter))
		lines.push_back(line);

	return lines;
}

bool FilePermissions::hasPermission(const std::string& path, FilePermissions::Type type) const
{
	for (const auto& perm : negativePermissions)
	{
		if (perm.flags.test(type) && match(path, perm))
			return false;
	}

	for (const auto& perm : permissions)
	{
		if (perm.flags.test(type) && match(path, perm))
			return true;
	}

	return false;
}

void FilePermissions::addPermission(const std::string& permissionString)
{
	Permission permission{};
	std::vector<std::string> segments;

	for (size_t idx = 0; idx < permissionString.length(); idx++)
	{
		char ch = permissionString[idx];
		if (ch == 'r')
			permission.flags.set(Type::Read);
		else if (ch == 'w')
			permission.flags.set(Type::Write);
		else if (ch == ' ')
		{
			segments = splitInput(permissionString.substr(idx + 1));
			break;
		}
	}

	if (!segments.empty())
	{
		for (const auto& segment : segments)
		{
			std::string replaced = std::regex_replace(segment, WILDCARD_REGEX, ".*");
			permission.segments.emplace_back(replaced);
		}

		if (permissionString[0] == '-')
			negativePermissions.push_back(permission);
		else
			permissions.push_back(permission);
	}
}

void FilePermissions::loadPermissions(const std::string& permissionStr)
{
	permissions.clear();
	negativePermissions.clear();

	std::vector<std::string> lines = splitInput(permissionStr, '\n');
	for (const auto& str : lines)
		addPermission(str);
}

bool FilePermissions::match(const std::string& path, const FilePermissions::Permission& permission)
{
	const auto& segments = splitInput(path);

	if (segments.empty() || segments.size() != permission.segments.size())
		return false;

	for (size_t i = 0; i < segments.size(); i++)
	{
		if (!std::regex_match(segments[i], permission.segments[i]))
			return false;
	}

	return true;
}
