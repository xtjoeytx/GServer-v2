#include "TPlayer.h"
#include "TWeapon.h"
#include "TServer.h"
#include "CFileSystem.h"

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