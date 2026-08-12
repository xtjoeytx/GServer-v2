#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include <CString.h>

#include <Server.h>
#include <UpdatePackage.h>
#include <filesystem/FileSystem.h>
#include <filesystem/FileSystemTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

std::optional<UpdatePackage> UpdatePackage::load(Server* const server, const std::string& name)
{
	// Search for the file in the filesystem.
	if (const auto& fileSystem = server->getFileSystem(); !fileSystem.hasi(fs::FileCategory::FILE, name))
		return std::nullopt;

	// Calculate the checksum for the gupd file.
	UpdatePackage updatePackage(name);
	updatePackage.reload(server);

	return updatePackage;
}

void UpdatePackage::reload(Server* const server)
{
	this->m_checksum = 0;
	this->m_packageSize = 0;
	this->m_fileList.clear();

	const auto& fileSystem = server->getFileSystem();

	// Search for the file in the filesystem, and load the contents
	auto fileData = fileSystem.infoi(fs::FileCategory::FILE, m_packageName);
	if (fileData == nullptr)
		return;

	CString fileContents;
	fileContents.load(fileData->file.string());

	// Calculate the checksum for the gupd file
	this->m_checksum = calculateCrc32Checksum(fileContents);

	// Calculate the checksum and filesize for each file referenced in the package
	for (const auto packageLines = fileContents.tokenize("\n"); const auto& line : packageLines)
	{
		// Line should be in the format of FILE levels/body.png
		if (const auto startPos = line.findi("FILE"); startPos == 0)
		{
			std::string filePath = line.subString(4).trim().toString();
			std::string baseFileName = std::filesystem::path(filePath).filename().string();

			// File was not found in the filesystem
			fileData = fileSystem.info(fs::FileCategory::FILE, baseFileName);
			if (fileData == nullptr)
			{
				server->sendToRC(CString() << "[Server]: Unable to find file '" << baseFileName << "' in package '" << m_packageName << "'");
				continue;
			}

			CString updateFileData;
			updateFileData.load(fileData->file.string());
			const uint32_t fileLength(updateFileData.length());

			this->m_fileList.emplace(baseFileName, FileEntry{.size = fileLength, .checksum = calculateCrc32Checksum(updateFileData)});
			this->m_packageSize += fileLength;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
