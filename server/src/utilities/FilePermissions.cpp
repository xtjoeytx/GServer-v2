#include <regex>
#include <string>
#include <vector>
#include <string_view>

#include <utilities/FilePermissions.h>
#include <utilities/StringUtils.h>

using namespace std::literals;

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

constexpr auto FOLDER_SEPARATOR = "/"sv;
const std::regex WILDCARD_REGEX(R"(\*)");

bool FilePermissions::hasPermission(const std::string_view path, const FilePermissions::Type type) const
{
	for (const auto& perm : negativePermissions)
	{
		if (perm.flags.test(type) && match(path, perm))
			return false;
	}

	// NOLINTNEXTLINE(*-use-anyofallof)
	for (const auto& perm : permissions)
	{
		if (perm.flags.test(type) && match(path, perm))
			return true;
	}

	return false;
}

void FilePermissions::addPermission(const std::string_view permissionString)
{
	Permission permission{};
	std::vector<std::string> segments;

	for (size_t idx = 0; idx < permissionString.length(); idx++)
	{
		const char ch = permissionString[idx];
		if (ch == 'r')
			permission.flags.set(Type::Read);
		else if (ch == 'w')
			permission.flags.set(Type::Write);
		else if (ch == ' ')
		{
			segments = string::splitToVector(permissionString.substr(idx + 1), FOLDER_SEPARATOR);
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

void FilePermissions::loadPermissions(const std::string_view permissionStr)
{
	permissions.clear();
	negativePermissions.clear();

	for (const auto& str : string::split(permissionStr, "\n"sv))
		addPermission(string::trim(str));
}

bool FilePermissions::match(const std::string_view path, const FilePermissions::Permission& permission)
{
	const auto segments = string::splitToVector(path, FOLDER_SEPARATOR, false);
	if (segments.empty() || segments.size() != permission.segments.size())
		return false;

	for (size_t i = 0; i < segments.size(); i++)
	{
		if (!std::regex_match(segments[i], permission.segments[i]))
			return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
