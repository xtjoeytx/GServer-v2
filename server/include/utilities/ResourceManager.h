#ifndef GS2EMU_RESOURCEMANAGER_H
#define GS2EMU_RESOURCEMANAGER_H

#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>

template<class ResourceCls, typename... PassArgs>
class ResourceManager
{
	using ResourcePtr = std::shared_ptr<ResourceCls>;
	using ResourceMap = std::unordered_map<std::string, ResourcePtr>;

public:
	explicit ResourceManager(PassArgs&&... An) : m_tuple(std::forward<PassArgs>(An)...) {}

	// Delete move operations
	ResourceManager(ResourceManager&& o) = delete;

	ResourceManager& operator=(ResourceManager&& o) noexcept = delete;

	// Delete copy operations
	ResourceManager& operator=(const ResourceManager&) = delete;

	ResourceManager(const ResourceManager&) = delete;

	//! Retrieve a resource that has already been loaded
	//! \param resourceName resource filename
	//! \return Shared pointer to a Resource class
	ResourcePtr findResource(const std::string& resourceName)
	{
		auto it = m_resourceMap.find(resourceName);
		if (it != m_resourceMap.end())
			return it->second;

		return {};
	}

	//! Search for a resource in the resource manager, otherwise we will
	//! attempt to load the resource
	//! \param resourceName resource filename
	//! \return Shared pointer to a Resource class
	ResourcePtr findOrAddResource(const std::string& resourceName)
	{
		auto it = m_resourceMap.find(resourceName);
		if (it != m_resourceMap.end())
			return it->second;

		return addResource(resourceName);
	}

	//! Add a resource to the resource manager
	//! \param resourceName resource filename
	//! \return Shared pointer to a Resource class
	ResourcePtr addResource(const std::string& resourceName)
	{
		// We will store an empty pointer in the hashmap if no resource could be loaded
		// so successive calls to findOrAddResource() do not keep trying to load the file
		ResourcePtr resourcePtr;

		auto newResource = std::apply(ResourceCls::load, std::tuple_cat(m_tuple, std::make_tuple(resourceName)));
		if (newResource)
			resourcePtr = std::make_shared<ResourceCls>(std::move(newResource.value()));

		m_resourceMap.emplace(resourceName, resourcePtr);
		return resourcePtr;
	}

	//! Delete a resource from the resource manager
	//! \param resourceName resource filename
	//! \return true if the resource was deleted
	bool deleteResource(const std::string& resourceName)
	{
		auto it = m_resourceMap.find(resourceName);
		if (it == m_resourceMap.end())
			return false;

		m_resourceMap.erase(it);
		return true;
	}

	const ResourceMap& getResources() const
	{
		return m_resourceMap;
	}

private:
	std::tuple<PassArgs...> m_tuple;
	ResourceMap m_resourceMap;
};

#endif
