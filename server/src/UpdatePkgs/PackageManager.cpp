#include "UpdatePkgs/PackageManager.h"
#include "TServer.h"

PackageManager::UpdatePackagePtr PackageManager::addPackage(const std::string& packageName)
{
	// We will store an empty pointer in the hashmap if no package could be loaded
	// so successive calls to findOrAddPackage() do not keep trying to load the file
	UpdatePackagePtr packagePtr;
	
	auto newPackage = UpdatePackage::loadPackage(_server->getFileSystem(), packageName);
	if (newPackage)
		packagePtr = std::make_shared<UpdatePackage>(std::move(newPackage.value()));
	
	_updatePackages.emplace(packageName, packagePtr);
	return packagePtr;
}

bool PackageManager::deletePackage(const std::string& packageName)
{
	auto it = _updatePackages.find(packageName);
	if (it == _updatePackages.end())
		return false;
	
	_updatePackages.erase(it);
	return true;
}
