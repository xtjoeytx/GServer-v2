#include "ICommon.h"
#include "TWeapon.h"
#include "TLevelItem.h"
#include "TPlayer.h"
#include "TServer.h"

TWeapon::TWeapon(const CString& pName, const CString& pImage, const CString& pScript, const time_t pModTime, bool trimCode)
: name(pName), image(pImage), fullScript(pScript), modTime(pModTime), defaultWeapon(false), defaultWeaponId(-1)
{
	if (pModTime == 0) modTime = time(0);

	// Remove comments and separate clientside and serverside scripts.
	CString nocomments = removeComments(pScript, "\xa7");
	if (nocomments.find("//#CLIENTSIDE") != -1)
	{
		serverScript = nocomments.readString("//#CLIENTSIDE");
		clientScript = CString("//#CLIENTSIDE\xa7") << nocomments.readString("");
	}
	else clientScript = nocomments;

	// Trim the code if specified.
	if (trimCode)
	{
		if (!serverScript.isEmpty())
		{
			std::vector<CString> code = serverScript.tokenize("\xa7");
			serverScript.clear();
			for (std::vector<CString>::iterator i = code.begin(); i != code.end(); ++i)
				serverScript << (*i).trim() << "\xa7";
		}
		if (!clientScript.isEmpty())
		{
			std::vector<CString> code = clientScript.tokenize("\xa7");
			clientScript.clear();
			for (std::vector<CString>::iterator i = code.begin(); i != code.end(); ++i)
				clientScript << (*i).trim() << "\xa7";
		}
	}
}

TWeapon* TWeapon::loadWeapon(const CString& pWeapon, TServer* server)
{
	// Prevent the loading/saving of filenames with illegal characters.
	CString w(pWeapon);
	w.replaceAllI("/", "_");
	w.replaceAllI("*", "@");

	CString fileName = server->getServerPath() << "weapons/" << w << ".txt";
	std::vector<CString> fileData = CString::loadToken(fileName);
	if (fileData.size() == 0) return 0;

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
			line.readString("NEWWEAPON ");
			CString s = line.readString("");
			std::vector<CString> explode = s.tokenize(",");
			if (explode.size() != 3) return 0;
			name = explode[0];
			image = explode[1];
			modTime = (time_t)strtolong(explode[2]);
			continue;
		}

		// Don't mess with ENDWEAPON.
		if (line.find("ENDWEAPON") != -1) continue;

		// Anything else weapon script.
		script << line << "\xa7";
	}

	// Don't allow weapons with no name.
	if (name.length() == 0)
		return 0;

	CSettings* settings = server->getSettings();
	return new TWeapon(name, image, script, modTime, settings->getBool("trimnpccode", false));
}

bool TWeapon::saveWeapon(TServer* server)
{
	if (name.length() == 0) return false;

	// Prevent the loading/saving of filenames with illegal characters.
	CString w(name);
	w.replaceAllI("/", "_");
	w.replaceAllI("*", "@");

	CString filename = server->getServerPath() << "weapons/" << w << ".txt";
	CString output;

	// Write the header.
	output << "NEWWEAPON " << name << "," << image << "," << CString((unsigned long)modTime) << "\r\n";

	// Write the serverside code.
	std::vector<CString> explode = serverScript.tokenize("\xa7");
	for (std::vector<CString>::iterator i = explode.begin(); i != explode.end(); ++i)
		output << *i << "\r\n";

	// Write the clientside separator if it does not exist.
	if (clientScript.find("//#CLIENTSIDE") == -1)
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

	CString outPacket = CString() >> (char)PLO_NPCWEAPONADD
		>> (char)name.length() << name
		>> (char)0 >> (char)image.length() << image
		>> (char)1 >> (short)clientScript.length() << clientScript;

	return outPacket;

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
