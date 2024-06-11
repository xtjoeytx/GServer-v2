#include "UpdatePackage.h"
#include "FileSystem.h"
#include "Server.h"
#include <filesystem>

std::optional<UpdatePackage> UpdatePackage::load(Server* const server, const std::string& name)
{
	auto fileSystem = server->getFileSystem();

	// Search for the file in the filesystem, and load the contents
	auto fileContents = fileSystem->load(name);
	if (fileContents.isEmpty())
		return std::nullopt;

	// Calculate the checksum for the gupd file
	UpdatePackage updatePackage(name);
	updatePackage.m_checksum = calculateCrc32Checksum(fileContents);

	// Calculate the checksum and filesize for each file referenced in the package
	auto packageLines = fileContents.tokenize("\n");
	for (const auto& line: packageLines)
	{
		auto startPos = line.findi("FILE");

		// Line should be in the format of FILE levels/body.png
		if (startPos == 0)
		{
			std::string filePath = line.subString(4).trim().toString();
			std::string baseFileName = std::filesystem::path(filePath).filename().string();

			CString updateFileData = fileSystem->load(baseFileName);

			// File was not found in the filesystem
			if (updateFileData.isEmpty())
				continue;

			uint32_t fileLength(updateFileData.length());

			updatePackage.m_fileList.emplace(baseFileName, FileEntry{
															   .size = fileLength,
															   .checksum = calculateCrc32Checksum(updateFileData) });

			updatePackage.m_packageSize += fileLength;
		}
	}

	return updatePackage;
}
