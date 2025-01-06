#include "TUpdatePackage.h"
#include "CFileSystem.h"
#include "TServer.h"
#include <filesystem>

std::optional<TUpdatePackage> TUpdatePackage::load(TServer *const server, const std::string &name) {
	auto fileSystem = server->getFileSystem();

	// Search for the file in the filesystem, and load the contents
	auto fileContents = fileSystem->load(name);
	if (fileContents.isEmpty())
		return std::nullopt;

	// Calculate the checksum for the gupd file
	TUpdatePackage updatePackage(name);
	updatePackage.reload(server);

	return updatePackage;
}

void TUpdatePackage::reload(TServer *const server) {
	this->checksum = 0;
	this->packageSize = 0;
	this->fileList.clear();

	auto fileSystem = server->getFileSystem();

	// Search for the file in the filesystem, and load the contents
	auto fileContents = fileSystem->load(this->packageName.c_str());
	if (fileContents.isEmpty())
		return;

	// Calculate the checksum for the gupd file
	this->checksum = calculateCrc32Checksum(fileContents);

	// Calculate the checksum and filesize for each file referenced in the package
	for (const auto packageLines = fileContents.tokenize("\n"); const auto &line: packageLines) {
		// Line should be in the format of FILE levels/body.png
		if (const auto startPos = line.findi("FILE"); startPos == 0) {
			std::string filePath = line.subString(4).trim().toString();
			std::string baseFileName = std::filesystem::path(filePath).filename().string();

			CString updateFileData = fileSystem->load(baseFileName);

			// File was not found in the filesystem
			if (updateFileData.isEmpty()) {
				server->sendToRC(CString() << "[Server]: Unable to find file '" << baseFileName << "' in package '" << packageName << "'");
				continue;
			}

			uint32_t fileLength(updateFileData.length());

			this->fileList.emplace(baseFileName, FileEntry{
															 .size = fileLength,
															 .checksum = calculateCrc32Checksum(updateFileData)});

			this->packageSize += fileLength;
		}
	}
}
