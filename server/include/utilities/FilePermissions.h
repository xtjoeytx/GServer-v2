#ifndef GS2EMU_FILEPERMISSIONS_H
#define GS2EMU_FILEPERMISSIONS_H

#pragma once

#include <bitset>
#include <regex>
#include <string>
#include <vector>

class FilePermissions
{
public:
	/**
	 * @enum Type
	 * @brief Defines the types of permissions available.
	 */
	enum Type : uint8_t
	{
		Read,
		Write,
		COUNT
	};

	/**
     * @brief Adds a new permission to the manager.
     *
     * @param permissionString The permission string (e.g., "rw accounts/*").
     */
	void addPermission(const std::string& permissionString);

	/**
	 * @brief Checks if a given file path has the required permissions.
	 *
	 * @param path The path were checking for access
	 * @param Type The type of permission were checking for (e.g. Read or Write)
	 * @return true if the path has the required permission, false otherwise.
	 */
	bool hasPermission(const std::string& path, Type type) const;

	/**
	 * @brief Loads permissions from a string input.
	 *
	 * @param input The string input containing permissions (e.g., "rw accounts/*\n-rw config/settings.php").
	 */
	void loadPermissions(const std::string& permissionString);

private:
	/**
	 * @struct Permission
	 * @brief Represents a single permission rule.
	 */
	struct Permission
	{
		std::bitset<Type::COUNT> flags;
		std::vector<std::regex> segments;
	};

	std::vector<Permission> permissions;
	std::vector<Permission> negativePermissions;

	static bool match(const std::string& path, const Permission& permission);
};

#endif //GS2EMU_FILEPERMISSIONS_H
