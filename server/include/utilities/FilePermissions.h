#ifndef GS2EMU_FILEPERMISSIONS_H
#define GS2EMU_FILEPERMISSIONS_H

#pragma once

#include <bitset>
#include <regex>
#include <string>
#include <vector>

class FilePermissions {
public:
	enum Type : uint8_t {
		Read,
		Write,
		COUNT
	};

	void addPermission(const std::string& permissionString);
	bool hasPermission(const std::string& path, Type type) const;
	void loadPermissions(const std::string& permissionString);

private:
	struct Permission {
		std::bitset<Type::COUNT> flags;
		std::vector<std::regex> segments;
	};

	std::vector<Permission> permissions;
	std::vector<Permission> negativePermissions;

	bool match(const std::string& path, const Permission& permission) const;
};

#endif //GS2EMU_FILEPERMISSIONS_H
