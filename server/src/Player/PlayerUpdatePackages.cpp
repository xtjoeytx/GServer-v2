#include "FileSystem.h"
#include "Player.h"
#include "Server.h"
#include "UpdatePackage.h"

bool TPlayer::msgPLI_VERIFYWANTSEND(CString& pPacket)
{
	unsigned long fileChecksum = pPacket.readGUInt5();
	CString fileName           = pPacket.readString("");

	// There is a USECHECKSUM flag in the config, and im pretty
	// certain it works similar to this: By always sending the
	// update package the client will respond with another request
	// including the crc32 hashes of all the files in the package
	bool ignoreChecksum = false;
	if (getExtension(fileName) == ".gupd")
		ignoreChecksum = true;

	if (!ignoreChecksum)
	{
		CString fileData = server->getFileSystem()->load(fileName);
		if (!fileData.isEmpty())
		{
			if (calculateCrc32Checksum(fileData) == fileChecksum)
			{
				sendPacket(CString() >> (char)PLO_FILEUPTODATE << fileName);
				return true;
			}
		}
	}

	// Send the file to the client
	this->sendFile(fileName);
	return true;
}

bool TPlayer::msgPLI_UPDATEPACKAGEREQUESTFILE(CString& pPacket)
{
	CString packageName = pPacket.readChars(pPacket.readGUChar());

	// 1 -> Install, 2 -> Reinstall
	unsigned char installType = pPacket.readGUChar();
	CString fileChecksums     = pPacket.readString("");

	// If this is a reinstall, we need to download everything so clear the checksum data
	if (installType == 2)
		fileChecksums.clear();

	auto totalDownloadSize = 0;
	std::vector<std::string> missingFiles;

	{
		auto updatePackage = server->getPackageManager().findOrAddResource(packageName.toString());
		if (updatePackage)
		{
			for (const auto& [fileName, entry]: updatePackage->getFileList())
			{
				// Compare the checksum for each file entry if the checksum is provided
				bool needsFile = true;
				if (fileChecksums.bytesLeft() >= 5)
				{
					uint32_t userFileChecksum = fileChecksums.readGUInt5();
					if (entry.checksum == userFileChecksum)
						needsFile = false;
				}

				if (needsFile)
				{
					totalDownloadSize += entry.size;
					missingFiles.push_back(fileName);
				}
			}
		}
	}

	sendPacket(CString() >> (char)PLO_UPDATEPACKAGESIZE >> (char)packageName.length() << packageName >> (long long)totalDownloadSize);

	for (const auto& wantFile: missingFiles)
		this->sendFile(wantFile);

	sendPacket(CString() >> (char)PLO_UPDATEPACKAGEDONE << packageName);
	return true;
}
