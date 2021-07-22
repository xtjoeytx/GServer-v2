#include "IDebug.h"

#include "TWeapon.h"
#include "TServer.h"
#include "TLevelItem.h"
#include "TNPC.h"
#include "IEnums.h"
#include "IUtil.h"

// GS2 Compiler includes
#include "Parser.h"
#include "ast.h"
#include "GS2SourceVisitor.h"
#include "GS2CompilerVisitor.h"


#ifdef V8NPCSERVER
#include "TPlayer.h"
#endif

// -- Constructor: Default Weapons -- //
TWeapon::TWeapon(TServer *pServer, LevelItemType pId)
: server(pServer), mModTime(0), mWeaponDefault(pId)
#ifdef V8NPCSERVER
, _scriptObject(0), _scriptExecutionContext(pServer->getScriptEngine())
#endif
{
	mWeaponName = TLevelItem::getItemName(mWeaponDefault);
}

// -- Constructor: Weapon Script -- //
TWeapon::TWeapon(TServer *pServer, const CString& pName, const CString& pImage, const CString& pScript, const time_t pModTime, bool pSaveWeapon)
: server(pServer), mWeaponName(pName), mWeaponImage(pImage), mModTime(pModTime), mWeaponDefault(LevelItemType::INVALID)
#ifdef V8NPCSERVER
, _scriptObject(0), _scriptExecutionContext(pServer->getScriptEngine())
#endif
{
	// Update Weapon
	this->updateWeapon(pImage, pScript, pModTime, pSaveWeapon);
}

TWeapon::~TWeapon()
{
#ifdef V8NPCSERVER
	freeScriptResources();
#endif
}

// -- Function: Load Weapon -- //
TWeapon * TWeapon::loadWeapon(const CString& pWeapon, TServer *server)
{
	// File Path
	CString fileName = server->getServerPath() << "weapons" << CFileSystem::getPathSeparator() << pWeapon;

	// Load File
	CString fileData;
	if (!fileData.load(fileName))
		return nullptr;

	fileData.removeAllI("\r");

	// Grab some information.
	bool has_script = (fileData.find("SCRIPT") != -1 ? true : false);
	bool has_scriptend = (fileData.find("SCRIPTEND") != -1 ? true : false);
	bool found_scriptend = false;

	// Parse header
	CString headerLine = fileData.readString("\n");
	if (headerLine != "GRAWP001")
		return nullptr;

	// Definitions
	CString weaponImage, weaponName, weaponScript, byteCodeFile;
	std::vector<std::pair<CString, CString> > byteCode;

	// Parse File
	while (fileData.bytesLeft())
	{
		CString curLine = fileData.readString("\n");

		// Find Command
		CString curCommand = curLine.readString();

		// Parse Line
		if (curCommand == "REALNAME")
			weaponName = curLine.readString("");
		else if (curCommand == "IMAGE")
			weaponImage = curLine.readString("");
		else if (curCommand == "BYTECODE")
		{
			CString fname = curLine.readString("");
			CString bytecode;
			bytecode.load(server->getServerPath() << "weapon_bytecode/" << fname);

			if (!bytecode.isEmpty()) {
				byteCode.emplace_back(fname, bytecode);
				byteCodeFile = fname;
			}
		}
		else if (curCommand == "SCRIPT")
		{
			do {
				curLine = fileData.readString("\n");
				if (curLine == "SCRIPTEND")
				{
					found_scriptend = true;
					break;
				}

				weaponScript << curLine << "\n";
			} while (fileData.bytesLeft());
		}
	}

	// Valid Weapon Name?
	if (weaponName.isEmpty())
		return nullptr;

	// Give a warning if our weapon was malformed.
	if (has_scriptend && !found_scriptend)
	{
		server->getServerLog().out("[%s] WARNING: Weapon %s is malformed.\n", server->getName().text(), weaponName.text());
		server->getServerLog().out("[%s] SCRIPTEND needs to be on its own line.\n", server->getName().text());
	}

	// Give a warning if both a script and a bytecode was found.
	if (!weaponScript.isEmpty() && !byteCode.empty())
		server->getServerLog().out("[%s] WARNING: Weapon %s includes both script and bytecode.  Using bytecode.\n", server->getName().text(), weaponName.text());

	TWeapon* ret = new TWeapon(server, weaponName, weaponImage, weaponScript, 0);
	if (!byteCode.empty() && ret->mByteCode.empty())
		ret->mByteCode = byteCode;

	if (!byteCodeFile.isEmpty())
		ret->mByteCodeFile = byteCodeFile;

	return ret;
}

// -- Function: Save Weapon -- //
bool TWeapon::saveWeapon()
{
	// Don't save default weapons / empty weapons
	if (this->isDefault() || mWeaponName.isEmpty())
		return false;

	// Prevent the loading/saving of filenames with illegal characters.
	CString name = mWeaponName;
	name.replaceAllI("\\", "_");
	name.replaceAllI("/", "_");
	name.replaceAllI("*", "@");
	name.replaceAllI(":", ";");
	name.replaceAllI("?", "!");
	CString filename = server->getServerPath() << "weapons" << CFileSystem::getPathSeparator() << "weapon" << name << ".txt";

	// Write the File.
	CString output = "GRAWP001\r\n";
	output << "REALNAME " << mWeaponName << "\r\n";
	output << "IMAGE " << mWeaponImage << "\r\n";
	for (unsigned int i = 0; i < mByteCode.size(); ++i)
		output << "BYTECODE " << mByteCode[i].first << "\r\n";

	if (!mWeaponScript.isEmpty())
	{
		output << "SCRIPT\r\n";
		output << mWeaponScript.replaceAll("\n", "\r\n");

		// Append a new line to the end of the script if one doesn't exist.
		if (mWeaponScript[mWeaponScript.length() - 1] != '\n')
			output << "\r\n";

		output << "SCRIPTEND\r\n";
	}

	// Save it.
	return output.save(filename);
}

// -- Function: Get Player Packet -- //
CString TWeapon::getWeaponPacket(bool forceGS1) const
{
	if (this->isDefault())
		return CString() >> (char)PLO_DEFAULTWEAPON >> (char)mWeaponDefault;

	if (mByteCode.empty() || forceGS1)
	{
		return CString() >> (char)PLO_NPCWEAPONADD
			>> (char)mWeaponName.length() << mWeaponName
			>> (char)NPCPROP_IMAGE >> (char)mWeaponImage.length() << mWeaponImage
			>> (char)NPCPROP_SCRIPT >> (short)mScriptClient.length() << mScriptClient;
	}
	else
	{
		CString out;
		out >> (char)PLO_NPCWEAPONADD >> (char)mWeaponName.length() << mWeaponName
			>> (char)NPCPROP_IMAGE >> (char)mWeaponImage.length() << mWeaponImage
			>> (char)NPCPROP_CLASS >> (short)0 << "\n";

		for (std::vector<std::pair<CString, CString> >::const_iterator i = mByteCode.begin(); i != mByteCode.end(); ++i)
		{
			CString b = i->second;

			unsigned char id = b.readGUChar();
			CString header = b.readChars(b.readGUShort());
			CString header2 = header.guntokenize();

			CString type = header2.readString("\n");
			CString name = header2.readString("\n");
			CString unknown = header2.readString("\n");
			CString hash = header2.readString("\n");

			// Get the mod time and send packet 197.
			CString smod = CString() >> (long long)time(0);
			smod.gtokenizeI();
			out >> (char)PLO_UNKNOWN197 << header << "," << smod << "\n";

			// Add to the output stream.
			out >> (char)PLO_RAWDATA >> (int)b.length() << "\n";
			out << b;
		}

		return out;
	}
}

// -- Function: Update Weapon Image/Script -- //
void TWeapon::updateWeapon(const CString& pImage, const CString& pCode, const time_t pModTime, bool pSaveWeapon)
{
#ifdef V8NPCSERVER
	// Clear script function
	if (!mScriptServer.isEmpty() || _scriptExecutionContext.hasActions())
		freeScriptResources();
#endif

	// TODO(joey): find out where \xa7 would be used, believe its in an earlier RC-version
	// Replace '\xa7' line endings with "\n"
	CString fixedScript;
	if (pCode.find("\xa7") != -1)
		fixedScript = pCode.replaceAll("\xa7", "\n");
	else
		fixedScript = pCode;

	// Copy Data
	this->setFullScript(fixedScript);
	this->setImage(pImage);
	this->setModTime(pModTime == 0 ? time(0) : pModTime);

#ifdef V8NPCSERVER
	// Separate client and server code
	setServerScript(fixedScript.readString("//#CLIENTSIDE"));

	// Compile and execute the script.
	CScriptEngine *scriptEngine = server->getScriptEngine();
	bool executed = scriptEngine->ExecuteWeapon(this);
	if (executed)
	{
		SCRIPTENV_D("WEAPON SCRIPT COMPILED\n");

		if (!mScriptServer.isEmpty()) {
			_scriptExecutionContext.addAction(scriptEngine->CreateAction("weapon.created", _scriptObject));
			scriptEngine->RegisterWeaponUpdate(this);
		}
	}
	else
		SCRIPTENV_D("Could not compile weapon script\n");
#endif
	if (fixedScript.findi("//#GS2") > -1) {
		setClientScript(fixedScript.readString("//#GS2"));
		ParserData parserStruct;
		parserStruct.parse(fixedScript.readString("").text());

		StatementBlock* stmtBlock = parserStruct.prog;

		if (stmtBlock != nullptr)
		{
			GS2SourceVisitor visit;
			visit.Visit(stmtBlock);

			GS2CompilerVisitor compilerVisitor(&parserStruct);
			compilerVisitor.Visit(stmtBlock);

			auto byteCode = compilerVisitor.getByteCode(mWeaponName.text());

			CString buf;
			buf.write((const char*)byteCode.buffer(), byteCode.length());

			// Compile source file to bytecode
			mByteCode.clear();
			mByteCode.emplace_back(mWeaponName,CString() >> char(PLO_NPCWEAPONSCRIPT) << buf);
			buf.clear();
		}

	} else setClientScript(fixedScript.readString(""));

	// Save Weapon
	if (pSaveWeapon)
		saveWeapon();
}

void TWeapon::setClientScript(const CString& pScript)
{
	// Remove any comments in the code
	CString formattedScript = removeComments(pScript);
	mScriptClient.clear(formattedScript.length());

	// Split code into tokens, trim each line, and use the clientside line ending '\xa7'
	std::vector<CString> code = formattedScript.tokenize("\n");
	for (auto & it : code)
		mScriptClient << it.trim() << "\xa7";
}

#ifdef V8NPCSERVER

void TWeapon::freeScriptResources()
{
	CScriptEngine *scriptEngine = server->getScriptEngine();

	scriptEngine->ClearCache<TWeapon>(mScriptServer.text());

	// Clear any queued actions
	if (_scriptExecutionContext.hasActions())
	{
		// Unregister npc from any queued event calls
		scriptEngine->UnregisterWeaponUpdate(this);

		// Reset execution
		_scriptExecutionContext.resetExecution();
	}

	// Delete script object
	if (_scriptObject)
	{
		delete _scriptObject;
		_scriptObject = nullptr;
	}
}

void TWeapon::queueWeaponAction(TPlayer *player, const std::string& args)
{
	CScriptEngine *scriptEngine = server->getScriptEngine();

	ScriptAction scriptAction = scriptEngine->CreateAction("weapon.serverside", _scriptObject, player->getScriptObject(), args);
	_scriptExecutionContext.addAction(scriptAction);
	scriptEngine->RegisterWeaponUpdate(this);
}

#endif