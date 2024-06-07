#include "Account.h"
#include "FileSystem.h"
#include "IDebug.h"
#include "Server.h"
#include <algorithm>
#include <memory.h>
#include <time.h>

/*
	Account: Constructor - Deconstructor
*/
Account::Account(Server* pServer)
	: m_server(pServer),
	  m_isBanned(false), m_isLoadOnly(false), m_isGuest(false), m_isExternal(false),
	  m_adminIp("0.0.0.0"),
	  m_accountIp(0), m_adminRights(0),
	  m_bodyImage("body.png"), m_headImage("head0.png"), m_gani("idle"), m_language("English"),
	  m_nickName("default"), m_shieldImage("shield1.png"), m_swordImage("sword1.png"),
	  m_eloDeviation(350.0f), m_hitpoints(3.0), m_eloRating(1500.0f),
	  m_x(0), m_y(0), m_z(0),
	  m_additionalFlags(0), m_ap(50), m_apCounter(0), m_arrowCount(10), m_bombCount(5), m_bombPower(1), m_carrySprite(-1),
	  m_deaths(0), m_glovePower(1), m_bowPower(1), m_gralatCount(0), m_horseBombCount(0), m_kills(0), m_mp(0), m_maxHitpoints(3),
	  m_onlineTime(0), m_shieldPower(1), m_sprite(2), m_status(20), m_swordPower(1), m_udpport(0),
	  m_attachNPC(0),
	  m_lastSparTime(0),
	  m_statusMsg(0), m_deviceId(0)
{
	// Other Defaults
	m_colors[0] = 2;  // c
	m_colors[1] = 0;  // a
	m_colors[2] = 10; // k
	m_colors[3] = 4;  // e
	m_colors[4] = 18; // s
	m_bowPower = 1;
}

Account::~Account() = default;

/*
	Account: Load/Save Account
*/
void Account::reset()
{
	if (!m_accountName.isEmpty())
	{
		CString acc(m_accountName);
		loadAccount("defaultaccount");
		m_accountName = acc;
		saveAccount();
	}
}

bool Account::loadAccount(const CString& pAccount, bool ignoreNickname)
{
	// Just in case this account was loaded offline through RC.
	m_accountName = pAccount;

	bool loadedFromDefault = false;
	FileSystem* accfs = m_server->getAccountsFileSystem();
	std::vector<CString> fileData;

	// Find the account in the file system.
	CString accpath(accfs->findi(CString() << pAccount << ".txt"));
	if (accpath.length() == 0)
	{
		accpath = m_server->getServerPath() << "accounts/defaultaccount.txt";
		FileSystem::fixPathSeparators(accpath);
		loadedFromDefault = true;
	}

	// Load file.
	fileData = CString::loadToken(accpath, "\n");
	if (fileData.empty() || fileData[0].trim() != "GRACC001")
		return false;

	// Clear Lists
	for (auto& i: m_attrList) i.clear();
	m_chestList.clear();
	m_flagList.clear();
	m_folderList.clear();
	m_weaponList.clear();
	m_privateMessageServerList.clear();

	// Parse File
	for (auto& i: fileData)
	{
		// Trim Line
		i.trimI();

		// Declare Variables;
		CString section, val;
		int sep;

		// Seperate Section & Value
		sep = i.find(' ');
		section = i.subString(0, sep);
		if (sep != -1)
			val = i.subString(sep + 1);

		if (section == "NAME") continue;
		else if (section == "NICK")
		{
			if (!ignoreNickname) m_nickName = val.subString(0, 223);
		}
		else if (section == "COMMUNITYNAME")
			m_communityName = val;
		else if (section == "LEVEL")
			m_levelName = val;
		else if (section == "X") { m_x = (float)strtofloat(val); }
		else if (section == "Y") { m_y = (float)strtofloat(val); }
		else if (section == "Z") { m_z = (float)strtofloat(val); }
		else if (section == "MAXHP")
			setMaxPower(strtoint(val));
		else if (section == "HP")
			setPower((float)strtofloat(val));
		else if (section == "RUPEES")
			m_gralatCount = strtoint(val);
		else if (section == "ANI")
			setGani(val);
		else if (section == "ARROWS")
			m_arrowCount = strtoint(val);
		else if (section == "BOMBS")
			m_bombCount = strtoint(val);
		else if (section == "GLOVEP")
			m_glovePower = strtoint(val);
		else if (section == "SHIELDP")
			setShieldPower(strtoint(val));
		else if (section == "SWORDP")
			setSwordPower(strtoint(val));
		else if (section == "BOWP")
			m_bowPower = strtoint(val);
		else if (section == "BOW")
			m_bowImage = val;
		else if (section == "HEAD")
			setHeadImage(val);
		else if (section == "BODY")
			setBodyImage(val);
		else if (section == "SWORD")
			setSwordImage(val);
		else if (section == "SHIELD")
			setShieldImage(val);
		else if (section == "COLORS")
		{
			std::vector<CString> t = val.tokenize(",");
			for (int i = 0; i < (int)t.size() && i < 5; i++) m_colors[i] = (unsigned char)strtoint(t[i]);
		}
		else if (section == "SPRITE")
			m_sprite = strtoint(val);
		else if (section == "STATUS")
			m_status = strtoint(val);
		else if (section == "MP")
			m_mp = strtoint(val);
		else if (section == "AP")
			m_ap = strtoint(val);
		else if (section == "APCOUNTER")
			m_apCounter = strtoint(val);
		else if (section == "ONSECS")
			m_onlineTime = strtoint(val);
		else if (section == "IP")
		{
			if (m_accountIp == 0) m_accountIp = strtolong(val);
		}
		else if (section == "LANGUAGE")
		{
			m_language = val;
			if (m_language.isEmpty()) m_language = "English";
		}
		else if (section == "KILLS")
			m_kills = strtoint(val);
		else if (section == "DEATHS")
			m_deaths = strtoint(val);
		else if (section == "RATING")
			m_eloRating = (float)strtofloat(val);
		else if (section == "DEVIATION")
			m_eloDeviation = (float)strtofloat(val);
		else if (section == "LASTSPARTIME")
			m_lastSparTime = strtolong(val);
		else if (section == "FLAG")
			setFlag(val);
		else if (section == "ATTR1")
			m_attrList[0] = val;
		else if (section == "ATTR2")
			m_attrList[1] = val;
		else if (section == "ATTR3")
			m_attrList[2] = val;
		else if (section == "ATTR4")
			m_attrList[3] = val;
		else if (section == "ATTR5")
			m_attrList[4] = val;
		else if (section == "ATTR6")
			m_attrList[5] = val;
		else if (section == "ATTR7")
			m_attrList[6] = val;
		else if (section == "ATTR8")
			m_attrList[7] = val;
		else if (section == "ATTR9")
			m_attrList[8] = val;
		else if (section == "ATTR10")
			m_attrList[9] = val;
		else if (section == "ATTR11")
			m_attrList[10] = val;
		else if (section == "ATTR12")
			m_attrList[11] = val;
		else if (section == "ATTR13")
			m_attrList[12] = val;
		else if (section == "ATTR14")
			m_attrList[13] = val;
		else if (section == "ATTR15")
			m_attrList[14] = val;
		else if (section == "ATTR16")
			m_attrList[15] = val;
		else if (section == "ATTR17")
			m_attrList[16] = val;
		else if (section == "ATTR18")
			m_attrList[17] = val;
		else if (section == "ATTR19")
			m_attrList[18] = val;
		else if (section == "ATTR20")
			m_attrList[19] = val;
		else if (section == "ATTR21")
			m_attrList[20] = val;
		else if (section == "ATTR22")
			m_attrList[21] = val;
		else if (section == "ATTR23")
			m_attrList[22] = val;
		else if (section == "ATTR24")
			m_attrList[23] = val;
		else if (section == "ATTR25")
			m_attrList[24] = val;
		else if (section == "ATTR26")
			m_attrList[25] = val;
		else if (section == "ATTR27")
			m_attrList[26] = val;
		else if (section == "ATTR28")
			m_attrList[27] = val;
		else if (section == "ATTR29")
			m_attrList[28] = val;
		else if (section == "ATTR30")
			m_attrList[29] = val;
		else if (section == "WEAPON")
			m_weaponList.push_back(val);
		else if (section == "CHEST")
			m_chestList.push_back(val);
		else if (section == "BANNED")
			m_isBanned = (strtoint(val) == 0 ? false : true);
		else if (section == "BANREASON")
			m_banReason = val;
		else if (section == "BANLENGTH")
			m_banLength = val;
		else if (section == "COMMENTS")
			m_accountComments = val;
		else if (section == "EMAIL")
			m_email = val;
		else if (section == "LOCALRIGHTS")
			m_adminRights = strtoint(val);
		else if (section == "IPRANGE")
			m_adminIp = val;
		else if (section == "LOADONLY")
			m_isLoadOnly = (strtoint(val) == 0 ? false : true);
		else if (section == "FOLDERRIGHT")
			m_folderList.push_back(val);
		else if (section == "LASTFOLDER")
			m_lastFolder = val;
	}

	// If this is a guest account, loadonly is set to true.
	if (pAccount.toLower() == "guest")
	{
		m_isLoadOnly = true;
		m_isGuest = true;
		srand((unsigned int)time(0));

		// Try to create a unique account number.
		while (true)
		{
			int v = (rand() * rand()) % 9999999;
			if (m_server->getPlayer("pc:" + CString(v).subString(0, 6), PLTYPE_ANYPLAYER) == 0)
			{
				m_communityName = "pc:" + CString(v).subString(0, 6);
				break;
			}
		}
	}

	// Comment out this line if you are actually going to use community names.
	if (pAccount.toLower() == "guest")
	{
		// The PC:123123123 should only be sent to other players, the logged in player should see it as guest.
		// Setting it back to only show as guest to everyone until that's fixed.
		m_accountName = m_communityName;
		m_communityName = "guest";
	}
	else
		m_communityName = m_accountName;

	// If we loaded from the default account...
	if (loadedFromDefault)
	{
		auto& settings = m_server->getSettings();

		// Check to see if we are overriding our start level and position.
		if (settings.exists("startlevel"))
			m_levelName = settings.getStr("startlevel", "onlinestartlocal.nw");
		if (settings.exists("startx"))
		{
			m_x = settings.getFloat("startx", 30.0f);
		}
		if (settings.exists("starty"))
		{
			m_y = settings.getFloat("starty", 30.5f);
		}

		// Save our account now and add it to the file system.
		if (!m_isLoadOnly)
		{
			saveAccount();
			accfs->addFile(CString() << "accounts/" << pAccount << ".txt");
		}
	}

	return true;
}

bool Account::saveAccount()
{
	// Don't save 'Load Only' or RC Accounts
	if (m_isLoadOnly)
		return false;

	CString newFile = "GRACC001\r\n";
	newFile << "NAME " << m_accountName << "\r\n";
	newFile << "NICK " << m_nickName << "\r\n";
	newFile << "COMMUNITYNAME " << m_accountName /*m_communityName*/ << "\r\n";
	newFile << "LEVEL " << m_levelName << "\r\n";
	newFile << "X " << CString(m_x) << "\r\n";
	newFile << "Y " << CString(m_y) << "\r\n";
	newFile << "Z " << CString(m_z) << "\r\n";
	newFile << "MAXHP " << CString(m_maxHitpoints) << "\r\n";
	newFile << "HP " << CString(m_hitpoints) << "\r\n";
	newFile << "RUPEES " << CString(m_gralatCount) << "\r\n";
	newFile << "ANI " << m_gani << "\r\n";
	newFile << "ARROWS " << CString(m_arrowCount) << "\r\n";
	newFile << "BOMBS " << CString(m_bombCount) << "\r\n";
	newFile << "GLOVEP " << CString(m_glovePower) << "\r\n";
	newFile << "SHIELDP " << CString(m_shieldPower) << "\r\n";
	newFile << "SWORDP " << CString(m_swordPower) << "\r\n";
	newFile << "BOWP " << CString(m_bowPower) << "\r\n";
	newFile << "BOW " << m_bowImage << "\r\n";
	newFile << "HEAD " << m_headImage << "\r\n";
	newFile << "BODY " << m_bodyImage << "\r\n";
	newFile << "SWORD " << m_swordImage << "\r\n";
	newFile << "SHIELD " << m_shieldImage << "\r\n";
	newFile << "COLORS " << CString(m_colors[0]) << "," << CString(m_colors[1]) << "," << CString(m_colors[2]) << "," << CString(m_colors[3]) << "," << CString(m_colors[4]) << "\r\n";
	newFile << "SPRITE " << CString(m_sprite) << "\r\n";
	newFile << "STATUS " << CString(m_status) << "\r\n";
	newFile << "MP " << CString(m_mp) << "\r\n";
	newFile << "AP " << CString(m_ap) << "\r\n";
	newFile << "APCOUNTER " << CString(m_apCounter) << "\r\n";
	newFile << "ONSECS " << CString(m_onlineTime) << "\r\n";
	newFile << "IP " << CString(m_accountIp) << "\r\n";
	newFile << "LANGUAGE " << m_language << "\r\n";
	newFile << "KILLS " << CString(m_kills) << "\r\n";
	newFile << "DEATHS " << CString(m_deaths) << "\r\n";
	newFile << "RATING " << CString(m_eloRating) << "\r\n";
	newFile << "DEVIATION " << CString(m_eloDeviation) << "\r\n";
	newFile << "LASTSPARTIME " << CString((unsigned long)m_lastSparTime) << "\r\n";

	// Attributes
	for (unsigned int i = 0; i < 30; i++)
	{
		if (m_attrList[i].length() > 0)
			newFile << "ATTR" << CString(i + 1) << " " << m_attrList[i] << "\r\n";
	}

	// Chests
	for (unsigned int i = 0; i < m_chestList.size(); i++)
		newFile << "CHEST " << m_chestList[i] << "\r\n";

	// Weapons
	for (unsigned int i = 0; i < m_weaponList.size(); i++)
		newFile << "WEAPON " << m_weaponList[i] << "\r\n";

	// Flags
	for (auto i = m_flagList.begin(); i != m_flagList.end(); ++i)
	{
		newFile << "FLAG " << i->first.c_str();
		if (!i->second.isEmpty()) newFile << "=" << i->second;
		newFile << "\r\n";
	}

	// Account Settings
	newFile << "\r\n";
	newFile << "BANNED " << CString((int)(m_isBanned == true ? 1 : 0)) << "\r\n";
	newFile << "BANREASON " << m_banReason << "\r\n";
	newFile << "BANLENGTH " << m_banLength << "\r\n";
	newFile << "COMMENTS " << m_accountComments << "\r\n";
	newFile << "EMAIL " << m_email << "\r\n";
	newFile << "LOCALRIGHTS " << CString(m_adminRights) << "\r\n";
	newFile << "IPRANGE " << m_adminIp << "\r\n";
	newFile << "LOADONLY " << CString((int)(m_isLoadOnly == true ? 1 : 0)) << "\r\n";

	// Folder Rights
	for (unsigned int i = 0; i < m_folderList.size(); i++)
		newFile << "FOLDERRIGHT " << m_folderList[i] << "\r\n";
	newFile << "LASTFOLDER " << m_lastFolder << "\r\n";

	// Get the file name for the account.
	CString accountFileName = m_server->getAccountsFileSystem()->fileExistsAs(CString() << m_accountName << ".txt");
	if (accountFileName.isEmpty()) accountFileName = CString() << m_accountName << ".txt";

	// Save the account now.
	CString accpath = m_server->getServerPath() << "accounts/" << accountFileName;
	FileSystem::fixPathSeparators(accpath);
	if (!newFile.save(accpath))
		m_server->getRCLog().out("** Error saving account: %s\n", m_accountName.text());

	return true;
}

/*
	Account: Account Management
*/
bool Account::meetsConditions(CString fileName, CString conditions)
{
	const char* conditional[] = { ">=", "<=", "!=", "=", ">", "<" };

	// Load and check if the file is valid.
	std::vector<CString> file;
	file = CString::loadToken(fileName, "\n", true);
	if (file.size() == 0 || (file.size() != 0 && file[0] != "GRACC001"))
		return false;

	// Load the conditions into a string list.
	std::vector<CString> cond;
	conditions.removeAllI("'");
	conditions.replaceAllI("%", "*");
	cond = conditions.tokenize(",");
	bool* conditionsMet = new bool[cond.size()];
	memset((void*)conditionsMet, 0, sizeof(bool) * cond.size());

	// Go through each line of the loaded file.
	for (std::vector<CString>::iterator i = file.begin(); i != file.end(); ++i)
	{
		int sep = (*i).find(' ');
		CString section = (*i).subString(0, sep);
		CString val = (*i).subString(sep + 1).removeAll("\r");
		section.trimI();
		val.trimI();

		// Check each line against the conditions specified.
		for (unsigned int j = 0; j < cond.size(); ++j)
		{
			int cond_num = -1;

			// Read out the name and value.
			cond[j].setRead(0);

			// Find out what conditional we are using.
			for (int k = 0; k < 6; ++k)
			{
				if (cond[j].find(conditional[k]) != -1)
				{
					cond_num = k;
					k = 6;
				}
			}
			if (cond_num == -1) continue;

			CString cname = cond[j].readString(conditional[cond_num]);
			CString cvalue = cond[j].readString("");
			cname.trimI();
			cvalue.trimI();
			cond[j].setRead(0);

			// Now, do a case-insensitive comparison of the section name.
#ifdef WIN32
			if (_stricmp(section.text(), cname.text()) == 0)
#else
			if (strcasecmp(section.text(), cname.text()) == 0)
#endif
			{
				switch (cond_num)
				{
					case 0:
					case 1:
					{
						// 0: >=
						// 1: <=
						// Check if it is a number.  If so, do a number comparison.
						bool condmet = false;
						if (val.isNumber())
						{
							double vNum[2] = { atof(val.text()), atof(cvalue.text()) };
							if (((cond_num == 1) ? (vNum[0] <= vNum[1]) : (vNum[0] >= vNum[1])))
							{
								conditionsMet[j] = true;
								condmet = true;
							}
						}
						else
						{
							// If not a number, do a string comparison.
							int ret = strcmp(val.text(), cvalue.text());
							if (((cond_num == 1) ? (ret <= 0) : (ret >= 0)))
							{
								conditionsMet[j] = true;
								condmet = true;
							}
						}

						// No conditions met means we see if we can fail.
						if (condmet == false)
						{
							CString cnameUp = cname.toUpper();
							if (!(cnameUp == "CHEST" || cnameUp == "WEAPON" ||
								  cnameUp == "FLAG" || cnameUp == "FOLDERRIGHT"))
								goto condAbort;
						}
						break;
					}

					case 4:
					case 5:
					{
						// 4: >
						// 5: <
						bool condmet = false;
						if (val.isNumber())
						{
							double vNum[2] = { atof(val.text()), atof(cvalue.text()) };
							if (((cond_num == 5) ? (vNum[0] < vNum[1]) : (vNum[0] > vNum[1])))
							{
								conditionsMet[j] = true;
								condmet = true;
							}
						}
						else
						{
							int ret = strcmp(val.text(), cvalue.text());
							if (((cond_num == 5) ? (ret < 0) : (ret > 0)))
							{
								conditionsMet[j] = true;
								condmet = true;
							}
						}

						if (condmet == false)
						{
							CString cnameUp = cname.toUpper();
							if (!(cnameUp == "CHEST" || cnameUp == "WEAPON" ||
								  cnameUp == "FLAG" || cnameUp == "FOLDERRIGHT"))
								goto condAbort;
						}
						break;
					}

					case 2:
					{
						// 2: !=
						// If we find a match, return false.
						if (val.isNumber())
						{
							double vNum[2] = { atof(val.text()), atof(cvalue.text()) };
							if (vNum[0] == vNum[1]) goto condAbort;
							conditionsMet[j] = true;
						}
						else
						{
							if (val.match(cvalue.text()) == true) goto condAbort;
							conditionsMet[j] = true;
						}
						break;
					}

					case 3:
					default:
					{
						// 0 - equals
						// If it returns false, don't include this account in the search.
						bool condmet = false;
						if (val.isNumber())
						{
							double vNum[2] = { atof(val.text()), atof(cvalue.text()) };
							if (vNum[0] == vNum[1])
							{
								conditionsMet[j] = true;
								condmet = true;
							}
						}
						else
						{
							if (val.match(cvalue.text()) == true)
							{
								conditionsMet[j] = true;
								condmet = true;
							}
						}

						if (condmet == false)
						{
							CString cnameUp = cname.toUpper();
							if (!(cnameUp == "CHEST" || cnameUp == "WEAPON" ||
								  cnameUp == "FLAG" || cnameUp == "FOLDERRIGHT"))
								goto condAbort;
						}
						break;
					}
				}
			}
		}
	}

	// Check if all the conditions were met.
	for (unsigned int i = 0; i < cond.size(); ++i)
		if (conditionsMet[i] == false) goto condAbort;

	// Clean up.
	delete[] conditionsMet;
	return true;

condAbort:
	delete[] conditionsMet;
	return false;
}

/*
	Account: Attribute-Managing
*/
bool Account::hasChest(const CString& pChest)
{
	auto it = std::find(m_chestList.begin(), m_chestList.end(), pChest);
	return (it != m_chestList.end());
}

bool Account::hasWeapon(const CString& pWeapon)
{
	auto it = std::find(m_weaponList.begin(), m_weaponList.end(), pWeapon);
	return (it != m_weaponList.end());
}

/*
	Account: Flag Management
*/
void Account::setFlag(CString pFlag)
{
	CString flagName = pFlag.readString("=");
	CString flagValue = pFlag.readString("");
	this->setFlag(flagName.text(), flagValue);
}

void Account::setFlag(const std::string& pFlagName, const CString& pFlagValue)
{
	if (m_server->getSettings().getBool("cropflags", true))
	{
		int fixedLength = 223 - 1 - pFlagName.length();
		m_flagList[pFlagName] = pFlagValue.subString(0, fixedLength);
	}
	else
		m_flagList[pFlagName] = pFlagValue;
}

/*
	Translation Functionality
*/
CString Account::translate(const CString& pKey)
{
	return m_server->TS_Translate(m_language, pKey);
}

void Account::setMaxPower(int newMaxPower)
{
	const auto& settings = m_server->getSettings();

	auto heartLimit = std::min(settings.getInt("heartlimit", 3), 20);
	m_maxHitpoints = clip(newMaxPower, 0, heartLimit);
}

void Account::setShieldPower(int newPower)
{
	const auto& settings = m_server->getSettings();

	m_shieldPower = clip(newPower, 0, settings.getInt("shieldlimit", 3));
}

void Account::setSwordPower(int newPower)
{
	const auto& settings = m_server->getSettings();

	m_swordPower = clip(newPower, ((settings.getBool("healswords", false) == true) ? -(settings.getInt("swordlimit", 3)) : 0), settings.getInt("swordlimit", 3));
}
