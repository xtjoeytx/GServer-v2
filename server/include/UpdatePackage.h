#ifndef GS2EMU_UPDATEPACKAGE_H
#define GS2EMU_UPDATEPACKAGE_H

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

class TServer;

class TUpdatePackage
{
public:
	struct FileEntry
	{
		uint32_t size;
		uint32_t checksum;
	};

	using FileList = std::unordered_map<std::string, FileEntry>;

public:
	explicit TUpdatePackage(std::string packageName);

	// Move operations
	TUpdatePackage(TUpdatePackage&& o) noexcept;
	TUpdatePackage& operator=(TUpdatePackage&& o) noexcept;

	// Delete copy operations
	TUpdatePackage(const TUpdatePackage&)            = delete;
	TUpdatePackage& operator=(const TUpdatePackage&) = delete;

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
	//! \param fileSystem CFileSystem where the package file could be located
	//! \param name filename of the package (ex: base_package.gupd)
	//! \return UpdatePackage if it was successfully loaded, otherwise a nullopt
	static std::optional<TUpdatePackage> load(TServer* const server, const std::string& name);

private:
	std::string packageName;
	std::unordered_map<std::string, FileEntry> fileList;
	uint32_t checksum;
	uint32_t packageSize;
};

inline TUpdatePackage::TUpdatePackage(std::string packageName)
	: packageName(std::move(packageName)),
	  checksum(0), packageSize(0)
{
}

inline TUpdatePackage::TUpdatePackage(TUpdatePackage&& o) noexcept
	: packageName(std::move(o.packageName)), fileList(std::move(o.fileList)),
	  checksum(o.checksum), packageSize(o.packageSize)
{
}

inline TUpdatePackage& TUpdatePackage::operator=(TUpdatePackage&& o) noexcept
{
	packageName = std::move(o.packageName);
	fileList    = std::move(o.fileList);
	checksum    = o.checksum;
	packageSize = o.packageSize;
	return *this;
}

inline const std::string& TUpdatePackage::getPackageName() const
{
	return packageName;
}

inline const TUpdatePackage::FileList& TUpdatePackage::getFileList() const
{
	return fileList;
}

inline uint32_t TUpdatePackage::getPackageSize() const
{
	return packageSize;
}

inline bool TUpdatePackage::compareChecksum(uint32_t check) const
{
	return checksum == check;
}

#endif //GS2EMU_UPDATEPACKAGE_H
