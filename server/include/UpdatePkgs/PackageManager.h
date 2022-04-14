#ifndef GS2EMU_PACKAGEMANAGER_H
#define GS2EMU_PACKAGEMANAGER_H

#include <memory>
#include <optional>
#include <unordered_map>
#include "UpdatePackage.h"

class TServer;

class PackageManager
{
	using UpdatePackagePtr = std::shared_ptr<UpdatePackage>;

public:
	explicit PackageManager(TServer* server);
	
	// Delete move operations
	PackageManager(PackageManager&& o) = delete;
	PackageManager& operator=(PackageManager&& o) noexcept = delete;
	
	// Delete copy operations
	PackageManager& operator=(const PackageManager&) = delete;
	PackageManager(const PackageManager&) = delete;
	
	//! Retrieve a package that has already been loaded
	//! \param packageName package filename
	//! \return Shared pointer to an UpdatePackage class
	UpdatePackagePtr findPackage(const std::string& packageName);
	
	//! Search for a package in the package manager, otherwise we will
	//! attempt to load the package from disk.
	//! \param packageName package filename
	//! \return Shared pointer to an UpdatePackage class
	UpdatePackagePtr findOrAddPackage(const std::string& packageName);
	
	//! Add a package to the package manager
	//! \param packageName package filename
	//! \return Shared pointer to an UpdatePackage class
	UpdatePackagePtr addPackage(const std::string& packageName);
	
	//! Delete a package from the package manager
	//! \param packageName package filename
	//! \return true if the package was deleted
	bool deletePackage(const std::string& packageName);

private:
	TServer* _server;
	std::unordered_map<std::string, UpdatePackagePtr> _updatePackages;
};

inline PackageManager::PackageManager(TServer* server) : _server(server)
{
}

inline PackageManager::UpdatePackagePtr PackageManager::findPackage(const std::string& packageName)
{
	auto it = _updatePackages.find(packageName);
	if (it != _updatePackages.end())
		return it->second;
	
	return {};
}

inline PackageManager::UpdatePackagePtr PackageManager::findOrAddPackage(const std::string& packageName)
{
	auto it = _updatePackages.find(packageName);
	if (it != _updatePackages.end())
		return it->second;
	
	return addPackage(packageName);
}

#endif //GS2EMU_PACKAGEMANAGER_H
