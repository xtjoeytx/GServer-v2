#include <memory.h>
#include "CFileSystem.h"
#include "TServer.h"
#include "TAccount.h"
#include "ICommon.h"

/*
	TAccount: Constructor - Deconstructor
*/
TAccount::TAccount(TServer* pServer)
: server(pServer),
isBanned(false), isLoadOnly(false),
adminIp("0.0.0.0"),
accountIp(0), adminRights(0),
bodyImg("body.png"), headImg("head0.png"), gani("idle"), language("English"),
nickName("default"), shieldImg("shield1.png"), swordImg("sword1.png"),
deviation(350.0f), oldDeviation(350.0f), power(3.0), rating(1500.0f), x(0), y(0), z(0),
x2(0), y2(0), z2(0), gmaplevelx(0), gmaplevely(0),
additionalFlags(0), ap(50), apCounter(0), arrowc(10), bombc(5), bombPower(1), carrySprite(-1),
deaths(0), glovePower(1), bowPower(1), gralatc(0), horsec(0), kills(0), mp(0), maxPower(3),
onlineTime(0), shieldPower(1), sprite(2), status(20), swordPower(1), udpport(0),
lastSparTime(0),
statusMsg(0)
{
	// Other Defaults
	colors[0] = 2;	// c
	colors[1] = 0;	// a
	colors[2] = 10;	// k
	colors[3] = 4;	// e
	colors[4] = 18;	// s
	bowPower = 1;
}

TAccount::~TAccount()
{
}

/*
	TAccount: Load/Save Account
*/
void TAccount::reset()
{
	if (!accountName.isEmpty())
	{
		CString acc(accountName);
		loadAccount("defaultaccount");
		accountName = acc;
		saveAccount();
	}
}

bool TAccount::loadAccount(const CString& pAccount)
{
	// Just in case this account was loaded offline through RC.
	accountName = pAccount;

	bool loadedFromDefault = false;
	CFileSystem* accfs = server->getAccountsFileSystem();
	std::vector<CString> fileData;

	CString accountText = server->getPluginManager().LoadAccount(pAccount);
	if (!accountText.isEmpty())
		fileData = accountText.tokenize("\n");
	else
	{
		// Find the account in the file system.
		CString accpath(accfs->findi(CString() << pAccount << ".txt"));
		if (accpath.length() == 0)
		{
			accpath = CString() << server->getServerPath() << "accounts/defaultaccount.txt";
			CFileSystem::fixPathSeparators(&accpath);
			loadedFromDefault = true;
		}

		// Load file.
		fileData = CString::loadToken(accpath, "\n");
		if (fileData.size() == 0 || fileData[0].trim() != "GRACC001")
			return false;
	}

	// Clear Lists
	for (int i = 0; i < 30; ++i) attrList[i].clear();
	chestList.clear();
	flagList.clear();
	folderList.clear();
	weaponList.clear();

	// Parse File
	for (unsigned int i = 0; i < fileData.size(); i++)
	{
		// Trim Line
		fileData[i].trimI();

		// Declare Variables;
		CString section, val;
		int sep;

		// Seperate Section & Value
		sep = fileData[i].find(' ');
		section = fileData[i].subString(0, sep);
		if (sep != -1)
			val = fileData[i].subString(sep + 1);

		if (section == "NAME") continue;
		else if (section == "NICK") continue;
		else if (section == "COMMUNITYNAME") communityName = val;
		else if (section == "LEVEL") levelName = val;
		else if (section == "X") { x = (float)strtofloat(val); x2 = (int)(x * 16); }
		else if (section == "Y") { y = (float)strtofloat(val); y2 = (int)(y * 16); }
		else if (section == "Z") { z = (float)strtofloat(val); z2 = (int)(z * 16); }
		else if (section == "MAXHP") maxPower = (int)strtoint(val);
		else if (section == "HP") power = (float)strtofloat(val);
		else if (section == "RUPEES") gralatc = strtoint(val);
		else if (section == "ANI") gani = val;
		else if (section == "ARROWS") arrowc = strtoint(val);
		else if (section == "BOMBS") bombc = strtoint(val);
		else if (section == "GLOVEP") glovePower = strtoint(val);
		else if (section == "SHIELDP") shieldPower = strtoint(val);
		else if (section == "SWORDP") swordPower = strtoint(val);
		else if (section == "BOWP") bowPower = strtoint(val);
		else if (section == "BOW") bowImage = val;
		else if (section == "HEAD") headImg = val;
		else if (section == "BODY") bodyImg = val;
		else if (section == "SWORD") swordImg = val;
		else if (section == "SHIELD") shieldImg = val;
		else if (section == "COLORS") { std::vector<CString> t = val.tokenize(","); for (int i = 0; i < (int)t.size() && i < 5; i++) colors[i] = (unsigned char)strtoint(t[i]); }
		else if (section == "SPRITE") sprite = strtoint(val);
		else if (section == "STATUS") status = strtoint(val);
		else if (section == "MP") mp = strtoint(val);
		else if (section == "AP") ap = strtoint(val);
		else if (section == "APCOUNTER") apCounter = strtoint(val);
		else if (section == "ONSECS") onlineTime = strtoint(val);
		else if (section == "IP") accountIp = strtoint(val);
		else if (section == "LANGUAGE") language = val;
		else if (section == "KILLS") kills = strtoint(val);
		else if (section == "DEATHS") deaths = strtoint(val);
		else if (section == "RATING") rating = (float)strtofloat(val);
		else if (section == "DEVIATION") deviation = (float)strtofloat(val);
		else if (section == "OLDDEVIATION") oldDeviation = (float)strtofloat(val);
		else if (section == "LASTSPARTIME") lastSparTime = strtolong(val);
		else if (section == "FLAG") flagList.push_back(val);
		else if (section == "ATTR1") attrList[0] = val;
		else if (section == "ATTR2") attrList[1] = val;
		else if (section == "ATTR3") attrList[2] = val;
		else if (section == "ATTR4") attrList[3] = val;
		else if (section == "ATTR5") attrList[4] = val;
		else if (section == "ATTR6") attrList[5] = val;
		else if (section == "ATTR7") attrList[6] = val;
		else if (section == "ATTR8") attrList[7] = val;
		else if (section == "ATTR9") attrList[8] = val;
		else if (section == "ATTR10") attrList[9] = val;
		else if (section == "ATTR11") attrList[10] = val;
		else if (section == "ATTR12") attrList[11] = val;
		else if (section == "ATTR13") attrList[12] = val;
		else if (section == "ATTR14") attrList[13] = val;
		else if (section == "ATTR15") attrList[14] = val;
		else if (section == "ATTR16") attrList[15] = val;
		else if (section == "ATTR17") attrList[16] = val;
		else if (section == "ATTR18") attrList[17] = val;
		else if (section == "ATTR19") attrList[18] = val;
		else if (section == "ATTR20") attrList[19] = val;
		else if (section == "ATTR21") attrList[20] = val;
		else if (section == "ATTR22") attrList[21] = val;
		else if (section == "ATTR23") attrList[22] = val;
		else if (section == "ATTR24") attrList[23] = val;
		else if (section == "ATTR25") attrList[24] = val;
		else if (section == "ATTR26") attrList[25] = val;
		else if (section == "ATTR27") attrList[26] = val;
		else if (section == "ATTR28") attrList[27] = val;
		else if (section == "ATTR29") attrList[28] = val;
		else if (section == "ATTR30") attrList[29] = val;
		else if (section == "WEAPON") weaponList.push_back(val);
		else if (section == "CHEST") chestList.push_back(val);
		else if (section == "BANNED") isBanned = (strtoint(val) == 0 ? false : true);
		else if (section == "BANREASON") banReason = val;
		else if (section == "BANLENGTH") banLength = val;
		else if (section == "COMMENTS") accountComments = val;
		else if (section == "EMAIL") email = val;
		else if (section == "LOCALRIGHTS") adminRights = strtoint(val);
		else if (section == "IPRANGE") adminIp = val;
		else if (section == "FOLDERRIGHT") folderList.push_back(val);
		else if (section == "LASTFOLDER") lastFolder = val;
	}

	// Comment out this line if you are actually going to use community names.
	communityName = accountName;

	// If we loaded from the default account, save our account now and add it to the file system.
	if (loadedFromDefault)
	{
		saveAccount();
		accfs->addFile(CString() << "accounts/" << pAccount << ".txt");
	}

	return true;
}

bool TAccount::saveAccount()
{
	// Don't save 'Load Only' or RC Accounts
	if (isLoadOnly)
		return false;

	CString newFile = "GRACC001\r\n";
	newFile << "NAME " << accountName << "\r\n";
	newFile << "NICK " << nickName << "\r\n";
	newFile << "COMMUNITYNAME " << accountName /*communityName*/ << "\r\n";
	newFile << "LEVEL " << levelName << "\r\n";
	newFile << "X " << CString(x) << "\r\n";
	newFile << "Y " << CString(y) << "\r\n";
	newFile << "Z " << CString(z) << "\r\n";
	newFile << "MAXHP " << CString(maxPower) << "\r\n";
	newFile << "HP " << CString(power) << "\r\n";
	newFile << "RUPEES " << CString(gralatc) << "\r\n";
	newFile << "ANI " << gani << "\r\n";
	newFile << "ARROWS " << CString(arrowc) << "\r\n";
	newFile << "BOMBS " << CString(bombc) << "\r\n";
	newFile << "GLOVEP " << CString(glovePower) << "\r\n";
	newFile << "SHIELDP " << CString(shieldPower) << "\r\n";
	newFile << "SWORDP " << CString(swordPower) << "\r\n";
	newFile << "BOWP " << CString(bowPower) << "\r\n";
	newFile << "BOW " << bowImage << "\r\n";
	newFile << "HEAD " << headImg << "\r\n";
	newFile << "BODY " << bodyImg << "\r\n";
	newFile << "SWORD " << swordImg << "\r\n";
	newFile << "SHIELD " << shieldImg << "\r\n";
	newFile << "COLORS " << CString(colors[0]) << "," << CString(colors[1]) << "," << CString(colors[2]) << "," << CString(colors[3]) << "," << CString(colors[4]) << "\r\n";
	newFile << "SPRITE " << CString(sprite) << "\r\n";
	newFile << "STATUS " << CString(status) << "\r\n";
	newFile << "MP " << CString(mp) << "\r\n";
	newFile << "AP " << CString(ap) << "\r\n";
	newFile << "APCOUNTER " << CString(apCounter) << "\r\n";
	newFile << "ONSECS " << CString(onlineTime) << "\r\n";
	newFile << "IP " << CString(accountIp) << "\r\n";
	newFile << "LANGUAGE " << language << "\r\n";
	newFile << "KILLS " << CString(kills) << "\r\n";
	newFile << "DEATHS " << CString(deaths) << "\r\n";
	newFile << "RATING " << CString(rating) << "\r\n";
	newFile << "DEVIATION " << CString(deviation) << "\r\n";
	newFile << "OLDDEVIATION " << CString(oldDeviation) << "\r\n";
	newFile << "LASTSPARTIME " << CString((unsigned long)lastSparTime) << "\r\n";

	// Attributes
	for (unsigned int i = 0; i < 30; i++)
	{
		if (attrList[i].length() > 0)
			newFile << "ATTR" << CString(i+1) << " " << attrList[i] << "\r\n";
	}

	// Chests
	for (unsigned int i = 0; i < chestList.size(); i++)
		newFile << "CHEST " << chestList[i] << "\r\n";

	// Weapons
	for (unsigned int i = 0; i < weaponList.size(); i++)
		newFile << "WEAPON " << weaponList[i] << "\r\n";

	// Flags
	for (unsigned int i = 0; i < flagList.size(); i++)
		newFile << "FLAG " << flagList[i] << "\r\n";

	// Account Settings
	newFile << "\r\n";
	newFile << "BANNED " << CString((int)(isBanned == true ? 1 : 0)) << "\r\n";
	newFile << "BANREASON " << banReason << "\r\n";
	newFile << "BANLENGTH " << banLength << "\r\n";
	newFile << "COMMENTS " << accountComments << "\r\n";
	newFile << "EMAIL " << email << "\r\n";
	newFile << "LOCALRIGHTS " << CString(adminRights) << "\r\n";
	newFile << "IPRANGE " << adminIp << "\r\n";

	// Folder Rights
	for (unsigned int i = 0; i < folderList.size(); i++)
		newFile << "FOLDERRIGHT " << folderList[i] << "\r\n";
	newFile << "LASTFOLDER " << lastFolder << "\r\n";

	// See if a plugin saves the account.
	if (server->getPluginManager().SaveAccount(accountName.text(), newFile.text()))
		return true;

	// Save the account now.
	CString accpath = CString() << server->getServerPath() << "accounts/" << accountName << ".txt";
	CFileSystem::fixPathSeparators(&accpath);
	if (!newFile.save(accpath))
		server->getRCLog().out("** Error saving account: %s\n", accountName.text());

	return true;
}

/*
	TAccount: Attribute-Managing
*/
bool TAccount::hasChest(const TLevelChest *pChest, const CString& pLevel)
{
	// Definitions
	CString chestStr = pChest->getChestStr((pLevel.length() > 1 ? pLevel : levelName));

	// Iterate Chest List
	for (std::vector<CString>::iterator i = chestList.begin(); i != chestList.end(); ++i)
	{
		if (*i == chestStr)
			return true;
	}

	return false;
}

bool TAccount::hasWeapon(const CString& pWeapon)
{
	// Iterate Weapon List
	for (std::vector<CString>::iterator i = weaponList.begin(); i != weaponList.end(); ++i)
	{
		if (*i == pWeapon)
			return true;
	}

	return false;
}

bool TAccount::meetsConditions( CString fileName, CString conditions )
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
	delete [] conditionsMet;
	return true;

condAbort:
	delete [] conditionsMet;
	return false;
}
