#include "ICommon.h"
#include "TWeapon.h"
#include "TLevelItem.h"
#include "TPlayer.h"
#include "TServer.h"

// -- Constructor: Default Weapons -- //
TWeapon::TWeapon(const char pId) : mModTime(0), mWeaponDefault(pId)
{
	mWeaponName = TLevelItem::getItemName(mWeaponDefault);
}

// -- Constructor: Weapon Script -- //
TWeapon::TWeapon(TServer *pServer, const CString& pName, const CString& pImage, const CString& pScript, const time_t pModTime, bool pSaveWeapon)
: mWeaponName(pName), mWeaponImage(pImage), mModTime(pModTime), mWeaponDefault(-1)
{
	// Update Weapon
	this->updateWeapon(pServer, pImage, pScript, pModTime, pSaveWeapon);
}

// -- Function: Load Weapon -- //
TWeapon * TWeapon::loadWeapon(const CString& pWeapon, TServer *server)
{
	// File Path
	CString fileName = server->getServerPath() << "weapons/" << pWeapon;

	// Load File
	std::vector<CString> fileData = CString::loadToken(fileName, "\n", true);
	if (fileData.size() == 0 || fileData[0].trim() != "GRAWP001")
		return 0;

	// Definitions
	CString weaponImage, weaponName, weaponScript;

	// Parse File
	for (std::vector<CString>::iterator i = fileData.begin(); i != fileData.end(); ++i)
	{
		// Find Command
		CString curCommand = i->readString();

		// Parse Line
		if (curCommand == "REALNAME")
			weaponName = i->readString("");
		else if (curCommand == "IMAGE")
			weaponImage = i->readString("");
		else if (curCommand == "SCRIPT")
		{
			++i;
			while (i != fileData.end() && *i != "SCRIPTEND")
			{
				weaponScript << *i << "\xa7";
				++i;
			}
		}
	}

	// Valid Weapon Name?
	if (weaponName.isEmpty())
		return 0;

	// Create Weapon
	return new TWeapon(server, weaponName, weaponImage, weaponScript, 0);
}

// -- Function: Save Weapon -- //
bool TWeapon::saveWeapon(TServer* server)
{
	// Don't save default weapons / empty weapons
	if (this->isDefault() || mWeaponName.isEmpty())
		return false;

	// Prevent the loading/saving of filenames with illegal characters.
	CString filename = server->getServerPath() << "weapons/weapon" << mWeaponName.replaceAll("/", "_").replaceAll("*", "@") << ".txt";

	// Write the File.
	CString output = "GRAWP001\r\n";
	output << "REALNAME " << mWeaponName << "\r\n";
	output << "IMAGE " << mWeaponImage << "\r\n";
	output << "SCRIPT\r\n";
	output << mWeaponScript.replaceAll("\xa7", "\r\n") << (mWeaponScript[mWeaponScript.length() - 1] != '\xa7' ? "\r\n" : "");
	output << "SCRIPTEND\r\n";

	// Save it.
	return output.save(filename);
}

// -- Function: Get Player Packet -- //
CString TWeapon::getWeaponPacket() const
{
	if (this->isDefault())
		return CString() >> (char)PLO_DEFAULTWEAPON >> (char)mWeaponDefault;

	return CString() >> (char)PLO_NPCWEAPONADD
		>> (char)mWeaponName.length() << mWeaponName
		>> (char)0 >> (char)mWeaponImage.length() << mWeaponImage
		>> (char)1 >> (short)mScriptClient.length() << mScriptClient;
}

// -- Function: Update Weapon Image/Script -- //
void TWeapon::updateWeapon(TServer *pServer, const CString& pImage, const CString& pCode, const time_t pModTime, bool pSaveWeapon)
{
	// Copy Data
	this->setFullScript(pCode);
	this->setImage(pImage);
	this->setModTime(pModTime == 0 ? time(0) : pModTime);
	
	// Remove Comments
	CString script = removeComments(pCode, "\xa7");

	// Trim Code
	if (pServer->getSettings()->getBool("trimnpccode", false))
	{
		std::vector<CString> code = script.tokenize("\xa7");
		script.clear();
		for (std::vector<CString>::iterator i = code.begin(); i != code.end(); ++i)
			script << (*i).trim() << "\xa7";
	}
	
	// Parse Text
	if (pServer->hasNPCServer())
	{
		setServerScript(script.readString("//#CLIENTSIDE"));
		setClientScript(script.readString(""));
	}
	else setClientScript(script);

	// Save Weapon
	if (pSaveWeapon)
		saveWeapon(pServer);
}
