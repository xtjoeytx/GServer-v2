#include "TPlayer.h"
#include "TServer.h"
#include "CFileSystem.h"

bool TPlayer::msgPLI_REQUESTUPDATEPACKAGE(CString& pPacket)
{

	CFileSystem* fileSystem = server->getFileSystem();

	// Get the packet data and file mod time.
	time_t modTime = pPacket.readGInt5();
	CString file = pPacket.readString("");

	time_t fModTime = fileSystem->getModTime(file);

	// If the file on disk is different, send it to the player.
	file.setRead(0);

	// TODO: Fix the modtime stuff
	//if (fModTime > modTime) {

	this->sendFile(file);
	sendPacket(CString() >> (char)PLO_UPDATEPACKAGEDONE << file);
	return true;
	//}

	sendPacket(CString() >> (char)PLO_FILEUPTODATE << file);

	return true;
}

bool TPlayer::msgPLI_UPDATEPACKAGEREQUESTFILE(CString& pPacket)
{
	char num = pPacket.readChar();
	CString file = pPacket.readString(".gupd");
	pPacket.removeI(0,file.length() + 7);
	unsigned char num2 = pPacket.readGUChar();
	CString text = pPacket.readString("");
	std::vector<CString> updatePackage = server->getFileSystem()->load(file << ".gupd").tokenize("\n");
	auto* fileNames = new std::vector<CString>();

	int files = 0;
	int totalFiles = text.length()/5;
	int totalFileSize = 0;
	for (const auto& line : updatePackage) {
		if (line.findi("FILE") > -1) {
			files++;
			CString file2 = line.subString(line.findi("FILE") + 5);
			if (num2 == files) {
				totalFileSize = server->getFileSystem()->getFileSize(file2.trimI());
				int fileSize = text.readGInt5();
#if defined(DEBUG)
				server->getServerLog().out(CString() << "UPDATEPACKAGE - Num: " << CString(num2) << " - Package: " << file << " - File: " << file2 << " - Filesystem size: " << CString(totalFileSize) << " - their size: " << CString(fileSize) << "\n");
#endif
				fileNames->push_back(file2.trimI());
			} else if (totalFiles > 1){
				totalFileSize += server->getFileSystem()->getFileSize(file2.trimI());
				fileNames->push_back(file2.trimI());
			}

		}
	}

	sendPacket(CString() >> (char)PLO_UPDATEPACKAGESIZE << num << file >> (long long)totalFileSize);

	for ( auto wantFile : *fileNames ) { this->sendFile(wantFile); }

#if defined(DEBUG)
	server->getServerLog().out(CString() << "UPDATEPACKAGE - Num: " << CString(num2) << " - Files: " << CString(files) << " - Length left: " << CString(text.length()) << " - Rest: " << text << "\n");
#endif

	sendPacket(CString() >> (char)PLO_UPDATEPACKAGEDONE << file);

	// Stub.
	return true;
}

