#include "FileSystem.h"
#include "Player.h"
#include "Server.h"
#include "Weapon.h"
#include "utilities/stringutils.h"

// packet 157
bool Player::msgPLI_UPDATEGANI(CString& pPacket)
{
	// Read packet data
	uint32_t checksum = pPacket.readGUInt5();
	std::string gani = pPacket.readString("").toString();
	const std::string ganiFile = gani + ".gani";

	// Try to find the animation in memory or on disk
	auto findAni = m_server->getAnimationManager().findOrAddResource(ganiFile);
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

bool Player::msgPLI_UPDATESCRIPT(CString& pPacket)
{
	CString weaponName = pPacket.readString("");

	m_server->getServerLog().out("PLI_UPDATESCRIPT: \"%s\"\n", weaponName.text());

	CString out;

	auto weaponObj = m_server->getWeapon(weaponName.toString());
	if (weaponObj != nullptr)
	{
		CString b = weaponObj->getByteCode();
		out >> (char)PLO_RAWDATA >> (int)b.length() << "\n";
		out >> (char)PLO_NPCWEAPONSCRIPT << b;

		sendPacket(out);
	}

	return true;
}

bool Player::msgPLI_UPDATECLASS(CString& pPacket)
{
	// Get the packet data and file mod time.
	time_t modTime = pPacket.readGInt5();
	std::string className = pPacket.readString("").toString();

	m_server->getServerLog().out("PLI_UPDATECLASS: \"%s\"\n", className.c_str());

	ScriptClass* classObj = m_server->getClass(className);

	if (classObj != nullptr)
	{
		CString out;
		out >> (char)PLO_RAWDATA >> (int)classObj->getByteCode().length() << "\n";
		out >> (char)PLO_NPCWEAPONSCRIPT << classObj->getByteCode();
		sendPacket(out);
	}
	else
	{
		std::vector<CString> headerData;
		headerData.push_back("class");
		headerData.push_back(className);
		headerData.push_back('1');
		headerData.push_back(CString() >> (long long)0 >> (long long)0);
		headerData.push_back(CString() >> (long long)0);

		CString gstr = utilities::retokenizeCStringArray(headerData);

		// Should technically be PLO_UNKNOWN197 but for some reason the client breaks player.join() scripts
		// if a weapon decides to request an class that doesnt exist on the server. This seems to fix it by
		// sending an empty bytecode
		sendPacket(CString() >> (char)PLO_NPCWEAPONSCRIPT >> (short)gstr.length() << gstr);
	}

	return true;
}
