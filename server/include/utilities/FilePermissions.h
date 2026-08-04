#ifndef FILEPERMISSIONS_H
#define FILEPERMISSIONS_H

#include <bitset>
#include <cstdint>
#include <regex>
#include <string_view>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class FilePermissions
{
public:
	/// @enum Type
	/// @brief Defines the types of permissions available.
	enum Type : uint8_t
	{
		Read,
		Write,
		//
		COUNT
	};

	/// @brief Adds a new permission to the manager.
	/// @param permissionString The permission string (e.g., "rw accounts/*").
	void addPermission(std::string_view permissionString);

	/// @brief Checks if a given file path has the required permissions.
	/// @param path The path were checking for access
	/// @param Type The type of permission were checking for (e.g. Read or Write)
	/// @return true if the path has the required permission, false otherwise.
	bool hasPermission(std::string_view path, Type type) const;

	/// @brief Checks if a given file path has the required permissions for multiple types.
	/// @param path The path to check for access.
	/// @param ...types The types of permissions to check (e.g., Read, Write).
	/// @return true if the path has all the required permissions, false otherwise.
	[[a::inline]] auto hasPermission(std::string_view path, auto... types) const;

	/// @brief Loads permissions from a string input.
	/// @param input The string input containing permissions (e.g., "rw accounts/*\n-rw config/settings.php").
	void loadPermissions(std::string_view permissionString);

private:
	/// @struct Permission
	/// @brief Represents a single permission rule.	
	struct Permission
	{
		std::bitset<Type::COUNT> flags;
		std::vector<std::regex> segments;
	};

	std::vector<Permission> permissions;
	std::vector<Permission> negativePermissions;

	static bool match(std::string_view path, const Permission& permission);
};

//----------------------------

inline auto FilePermissions::hasPermission(std::string_view path, auto... types) const
{
	return (hasPermission(path, types) && ...);
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // FILEPERMISSIONS_H
