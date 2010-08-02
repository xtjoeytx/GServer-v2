#include "IDebug.h"

#include "TWeapon.h"
#include "TServer.h"
#include "TLevelItem.h"
#include "IEnums.h"
#include "IUtil.h"

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
	CString fileData;
	fileData.load(fileName);
	fileData.removeAllI("\r");

	// Grab some information.
	bool has_script = (fileData.find("SCRIPT") != -1 ? true : false);
	bool has_scriptend = (fileData.find("SCRIPTEND") != -1 ? true : false);
	bool found_scriptend = false;

	// Parse into lines.
	std::vector<CString> fileLines = fileData.tokenize("\n");
	if (fileLines.size() == 0 || fileLines[0].trim() != "GRAWP001")
		return 0;

	// Definitions
	CString weaponImage, weaponName, weaponScript;

	// Parse File
	std::vector<CString>::iterator i = fileLines.begin();
	while (i != fileLines.end())
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
			while (i != fileLines.end())
			{
				if (*i == "SCRIPTEND")
				{
					found_scriptend = true;
					break;
				}
				weaponScript << *i << "\xa7";
				++i;
			}
		}
		if (i != fileLines.end()) ++i;
	}

	// Valid Weapon Name?
	if (weaponName.isEmpty())
		return 0;

	// Give a warning if our weapon was malformed.
	if (has_scriptend && !found_scriptend)
	{
		server->getServerLog().out("[%s] WARNING: Weapon %s is malformed.\n", server->getName().text(), weaponName.text());
		server->getServerLog().out("[%s] SCRIPTEND needs to be on its own line.\n", server->getName().text());
	}

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

	// Format the weapon script.
	CString script(mWeaponScript);
	if (script[script.length() - 1] != '\xa7')
		script << "\xa7";
	script.replaceAllI("\xa7", "\r\n");

	// Write the File.
	CString output = "GRAWP001\r\n";
	output << "REALNAME " << mWeaponName << "\r\n";
	output << "IMAGE " << mWeaponImage << "\r\n";
	output << "SCRIPT\r\n";
	output << script;
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
	std::vector<CString> code = script.tokenize("\xa7");
	script.clear();
	for (std::vector<CString>::iterator i = code.begin(); i != code.end(); ++i)
		script << (*i).trim() << "\xa7";
	
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
