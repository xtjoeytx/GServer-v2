#ifndef GS2EMU_UPDATEPACKAGE_H
#define GS2EMU_UPDATEPACKAGE_H

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

class Server;

class UpdatePackage
{
public:
	struct FileEntry
	{
		uint32_t size;
		uint32_t checksum;
	};

	using FileList = std::unordered_map<std::string, FileEntry>;

public:
	explicit UpdatePackage(std::string packageName);

	// Move operations
	UpdatePackage(UpdatePackage&& o) noexcept;
	UpdatePackage& operator=(UpdatePackage&& o) noexcept;

	// Delete copy operations
	UpdatePackage(const UpdatePackage&) = delete;
	UpdatePackage& operator=(const UpdatePackage&) = delete;

	//! Get the package filename
	//! \return package filename
	const std::string& getPackageName() const;

	//! Get the total package size in bytes across all files referenced in the package
	//! \return total package size in bytes
	uint32_t getPackageSize() const;

	//! Get the list of files referenced by this package
	//! \return hashmap of referenced files, and their size/crc32 checksum
	const FileList& getFileList() const;

	//! Compare a checksum against the packages checksum
	//! \param check crc32 checksum
	//! \return true if the checksums match
	bool compareChecksum(uint32_t check) const;

	//! Load an UpdatePackage from the filesystem
	//! \param fileSystem FileSystem where the package file could be located
	//! \param name filename of the package (ex: base_package.gupd)
	//! \return UpdatePackage if it was successfully loaded, otherwise a nullopt
	static std::optional<UpdatePackage> load(Server* const server, const std::string& name);

private:
	std::string m_packageName;
	std::unordered_map<std::string, FileEntry> m_fileList;
	uint32_t m_checksum;
	uint32_t m_packageSize;
};

inline UpdatePackage::UpdatePackage(std::string packageName)
	: m_packageName(std::move(packageName)),
	  m_checksum(0), m_packageSize(0)
{
}

inline UpdatePackage::UpdatePackage(UpdatePackage&& o) noexcept
	: m_packageName(std::move(o.m_packageName)), m_fileList(std::move(o.m_fileList)),
	  m_checksum(o.m_checksum), m_packageSize(o.m_packageSize)
{
}

inline UpdatePackage& UpdatePackage::operator=(UpdatePackage&& o) noexcept
{
	m_packageName = std::move(o.m_packageName);
	m_fileList = std::move(o.m_fileList);
	m_checksum = o.m_checksum;
	m_packageSize = o.m_packageSize;
	return *this;
}

inline const std::string& UpdatePackage::getPackageName() const
{
	return m_packageName;
}

inline const UpdatePackage::FileList& UpdatePackage::getFileList() const
{
	return m_fileList;
}

inline uint32_t UpdatePackage::getPackageSize() const
{
	return m_packageSize;
}

inline bool UpdatePackage::compareChecksum(uint32_t check) const
{
	return m_checksum == check;
}

#endif //GS2EMU_UPDATEPACKAGE_H
