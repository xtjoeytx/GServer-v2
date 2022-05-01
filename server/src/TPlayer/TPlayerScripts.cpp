#include "TPlayer.h"
#include "TWeapon.h"
#include "TServer.h"
#include "CFileSystem.h"

// packet 157
bool TPlayer::msgPLI_UPDATEGANI(CString& pPacket)
{
	// Read packet data
	uint32_t checksum = pPacket.readGUInt5();
	std::string gani = pPacket.readString("").toString();
	const std::string ganiFile = gani + ".gani";
	
	// Try to find the animation in memory or on disk
	auto findAni = server->getAnimationManager().findOrAddResource(ganiFile);
	if (!findAni)
	{
		//printf("Client requested gani %s, but was not found\n", ganiFile.c_str());
		return true;
	}
	
	// Compare the bytecode checksum from the client with the one for the
	// current script, if it doesn't match send the updated bytecode
	if (calculateCrc32Checksum(findAni->getByteCode()) != checksum)
		sendPacket(findAni->getBytecodePacket());
	
	// v4 and up needs this for some reason.
	sendPacket(CString() >> (char)PLO_UNKNOWN195 >> (char)gani.length() << gani << "\"SETBACKTO " << findAni->getSetBackTo() << "\"");
	return true;
}

bool TPlayer::msgPLI_UPDATESCRIPT(CString& pPacket)
{
	CString weaponName = pPacket.readString("");

	server->getServerLog().out("PLI_UPDATESCRIPT: \"%s\"\n", weaponName.text());

	CString out;

	TWeapon * weaponObj = server->getWeapon(weaponName);

	if (weaponObj != nullptr)
	{
		CString b = weaponObj->getByteCode();
		out >> (char)PLO_RAWDATA >> (int)b.length() << "\n";
		out >> (char)PLO_NPCWEAPONSCRIPT << b;

		sendPacket(out);
	}

	return true;
}

bool TPlayer::msgPLI_UPDATECLASS(CString& pPacket)
{
	// Get the packet data and file mod time.
	time_t modTime = pPacket.readGInt5();
	std::string className = pPacket.readString("").toString();

	server->getServerLog().out("PLI_UPDATECLASS: \"%s\"\n", className.c_str());

	CString out;

	TScriptClass* classObj = server->getClass(className);

	if (classObj != nullptr)
	{
		CString b = classObj->getByteCode();
		out >> (char)PLO_RAWDATA >> (int)b.length() << "\n";
		out >> (char)PLO_NPCWEAPONSCRIPT << b;

		sendPacket(out);
	}

	return true;
}