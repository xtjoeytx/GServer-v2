#include "TWeapon.h"
#include "TLevelItem.h"
#include "TPlayer.h"
#include "TServer.h"

TWeapon::TWeapon(const CString& pName, const CString& pImage, const CString& pScript, const time_t pModTime, bool trimCode)
: name(pName), image(pImage), modTime(pModTime), defaultWeapon(false), defaultWeaponId(-1)
{
	if (pModTime == 0) modTime = time(0);

	// Remove comments.
	std::vector<CString> parsedCode = TNPC::removeComments(pScript, trimCode);
	if (parsedCode.size() == 1) clientScript = parsedCode[0];
	else if (parsedCode.size() > 1)
	{
		serverScript = parsedCode[0];
		for (unsigned int i = 1; i < parsedCode.size(); ++i)
			clientScript << parsedCode[i];
	}
}

TWeapon* TWeapon::loadWeapon(const CString& pWeapon, TServer* server)
{
	CFileSystem* fileSystem = server->getFileSystem();
	CString fileName = server->getServerPath() << "weapons/" << pWeapon << ".txt";
	std::vector<CString> fileData = CString::loadToken(fileName);

	CString name;
	CString image;
	CString script;
	time_t modTime = 0;
	for (std::vector<CString>::iterator i = fileData.begin(); i != fileData.end(); ++i)
	{
		CString line = *i;
		line.removeAllI("\r");

		// See if it is the NEWWEAPON line.
		if (line.find("NEWWEAPON ") != -1)
		{
			std::vector<CString> explode = line.tokenize();
			if (explode.size() != 4) return 0;
			name = explode[1];
			image = explode[2];
			modTime = (time_t)strtolong(explode[3]);
			continue;
		}

		// Don't mess with ENDWEAPON.
		if (line.find("ENDWEAPON") != -1) continue;

		// Anything else weapon script.
		script << line << "\xa7";
	}

	CSettings* settings = server->getSettings();
	return new TWeapon(name, image, script, modTime, settings->getBool("trimnpccode", false));
}

bool TWeapon::saveWeapon(TServer* server)
{
	if (name.length() == 0) return false;
	CString filename = server->getServerPath() << "weapons/" << name << ".txt";
	CString output;

	// Write the header.
	output << "NEWWEAPON " << name << " " << image << " " << CString((unsigned long)modTime) << "\r\n";

	// Write the serverside code.
	std::vector<CString> explode = serverScript.tokenize("\xa7");
	for (std::vector<CString>::iterator i = explode.begin(); i != explode.end(); ++i)
		output << *i << "\r\n";

	// Write the clientside separator.
	output << "\r\n//#CLIENTSIDE\r\n";

	// Write the clientside code.
	explode.clear();
	explode = clientScript.tokenize("\xa7");
	for (std::vector<CString>::iterator i = explode.begin(); i != explode.end(); ++i)
		output << *i << "\r\n";

	// Write the footer.
	output << "ENDWEAPON\r\n";

	// Save it.
	return output.save(filename);
}

CString TWeapon::getWeaponPacket() const
{
	if (defaultWeapon)
		return CString() >> (char)PLO_DEFAULTWEAPON >> (char)defaultWeaponId;

	return CString() >> (char)PLO_NPCWEAPONADD
		>> (char)name.length() << name
		>> (char)0 >> (char)image.length() << image
		>> (char)1 >> (short)clientScript.length() << clientScript;

	// 0x00 - image
	// 0x01 - script
	// 0x4A - npcserver class script whatever.
}

CString TWeapon::getName() const
{
	if (defaultWeapon)
		return TLevelItem::getItemName(defaultWeaponId);
	return name;
}
