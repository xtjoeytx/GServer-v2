#include "IDebug.h"

#include "TWeapon.h"
#include "TServer.h"
#include "TLevelItem.h"
#include "TNPC.h"
#include "IEnums.h"
#include "IUtil.h"

// GS2 Compiler includes
#include "GS2Context.h"

#ifdef V8NPCSERVER
#include "TPlayer.h"
#endif

// -- Constructor: Default Weapons -- //
TWeapon::TWeapon(TServer *pServer, LevelItemType pId)
: server(pServer), mModTime(0), mWeaponDefault(pId)
#ifdef V8NPCSERVER
, _scriptExecutionContext(pServer->getScriptEngine())
#endif
{
	_weaponName = TLevelItem::getItemName(mWeaponDefault);
}

// -- Constructor: Weapon Script -- //
TWeapon::TWeapon(TServer *pServer, std::string pName, std::string pImage, std::string pScript, const time_t pModTime, bool pSaveWeapon)
: server(pServer), _weaponName(std::move(pName)), mModTime(pModTime), mWeaponDefault(LevelItemType::INVALID)
#ifdef V8NPCSERVER
, _scriptExecutionContext(pServer->getScriptEngine())
#endif
{
	// Update Weapon
	this->updateWeapon(std::move(pImage), std::move(pScript), pModTime, pSaveWeapon);
}

TWeapon::~TWeapon()
{
#ifdef V8NPCSERVER
	freeScriptResources();
#endif
}

// -- Function: Load Weapon -- //
std::shared_ptr<TWeapon> TWeapon::loadWeapon(const CString& pWeapon, TServer *server)
{
	// File Path
	CString fileName = server->getServerPath() << "weapons" << CFileSystem::getPathSeparator() << pWeapon;

	// Load File
	CString fileData;
	if (!fileData.load(fileName))
		return nullptr;

	fileData.removeAllI("\r");

	// Grab some information.
	bool has_scriptend = fileData.find("SCRIPTEND") != -1;
	bool found_scriptend = false;

	// Parse header
	CString headerLine = fileData.readString("\n");
	if (headerLine != "GRAWP001")
		return nullptr;

	// Definitions
	CString byteCodeData;
	std::string byteCodeFile, weaponImage, weaponName, weaponScript;

	// Parse File
	while (fileData.bytesLeft())
	{
		CString curLine = fileData.readString("\n");

		// Find Command
		CString curCommand = curLine.readString();

		// Parse Line
		if (curCommand == "REALNAME")
			weaponName = curLine.readString("").toString();
		else if (curCommand == "IMAGE")
			weaponImage = curLine.readString("").toString();
		else if (curCommand == "BYTECODE")
		{
			CString fileName = curLine.readString("");

			byteCodeData.load(server->getServerPath() << "weapon_bytecode/" << fileName);
			if (!byteCodeData.isEmpty())
				byteCodeFile = fileName.toString();
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

				weaponScript.append(curLine.text()).append("\n");
			} while (fileData.bytesLeft());
		}
	}

	// Valid Weapon Name?
	if (weaponName.empty())
		return nullptr;

	// Give a warning if our weapon was malformed.
	if (has_scriptend && !found_scriptend)
	{
		server->getServerLog().out("[%s] WARNING: Weapon %s is malformed.\n", server->getName().text(), weaponName.c_str());
		server->getServerLog().out("[%s] SCRIPTEND needs to be on its own line.\n", server->getName().text());
	}

	// Give a warning if both a script and a bytecode was found.
	if (!weaponScript.empty() && !byteCodeData.isEmpty())
	{
		server->getServerLog().out("[%s] WARNING: Weapon %s includes both script and bytecode.  Using bytecode.\n", server->getName().text(), weaponName.c_str());
		weaponScript.clear();
	}

	auto weapon = std::make_shared<TWeapon>(server, weaponName, weaponImage, weaponScript, 0);
	if (!byteCodeData.isEmpty())
	{
		weapon->_bytecode = CString(std::move(byteCodeData));
		weapon->_bytecodeFile = std::move(byteCodeFile);
	}

	return weapon;
}

// -- Function: Save Weapon -- //
bool TWeapon::saveWeapon()
{
	// Don't save default weapons / empty weapons
	if (this->isDefault() || _weaponName.empty())
		return false;

	// If the bytecode filename is set, the weapon is treated as read-only so it can't be saved
	if (!_bytecodeFile.empty())
		return false;

	// Prevent the loading/saving of filenames with illegal characters.
	CString name = _weaponName;
	name.replaceAllI("\\", "_");
	name.replaceAllI("/", "_");
	name.replaceAllI("*", "@");
	name.replaceAllI(":", ";");
	name.replaceAllI("?", "!");
	CString filename = server->getServerPath() << "weapons" << CFileSystem::getPathSeparator() << "weapon" << name << ".txt";

	// Write the File.
	CString output = "GRAWP001\r\n";
	output << "REALNAME " << _weaponName << "\r\n";
	output << "IMAGE " << _weaponImage << "\r\n";

	if (_source)
	{
		output << "SCRIPT\r\n";
		output << CString(_source.getSource()).replaceAll("\n", "\r\n");

		// Append a new line to the end of the script if one doesn't exist.
		if (_source.getSource().back() != '\n')
			output << "\r\n";

		output << "SCRIPTEND\r\n";
	}

	// Save it.
	return output.save(filename);
}

// -- Function: Get Player Packet -- //
CString TWeapon::getWeaponPacket(int clientVersion) const
{
	if (this->isDefault())
		return CString() >> (char)PLO_DEFAULTWEAPON >> (char)mWeaponDefault;
	
	CString weaponPacket;
	weaponPacket >> (char)PLO_NPCWEAPONADD >> (char)_weaponName.length() << _weaponName
				 >> (char)NPCPROP_IMAGE >> (char)_weaponImage.length() << _weaponImage;

	// GS2 is available for v4+
	if (clientVersion >= CLVER_4_0211)
	{
		if (!_bytecode.isEmpty())
		{
			weaponPacket >> (char)NPCPROP_CLASS >> (short)0 << "\n";

			CString b = _bytecode;
			CString header = b.readChars(b.readGUShort());

			// Get the mod time and send packet 197.
			weaponPacket >> (char)PLO_UNKNOWN197 << header << "," >> (long long)time(0) << "\n";
			return weaponPacket;
		}

		// GS1 is disabled for > 5.0.0.7
		if (clientVersion > CLVER_5_07)
			return weaponPacket;
	}

	weaponPacket >> (char)NPCPROP_SCRIPT >> (short)_formattedClientGS1.length() << _formattedClientGS1;
	return weaponPacket;
}

// -- Function: Update Weapon Image/Script -- //
void TWeapon::updateWeapon(std::string pImage, std::string pCode, const time_t pModTime, bool pSaveWeapon)
{
#ifdef V8NPCSERVER
	// Clear script function
	if (_source || _scriptExecutionContext.hasActions())
		freeScriptResources();
#endif

	bool gs2default = server->getSettings().getBool("gs2default", false);

	_source = SourceCode{ std::move(pCode), gs2default };
	_weaponImage = std::move(pImage);
	setModTime(pModTime == 0 ? time(0) : pModTime);

#ifdef V8NPCSERVER
	// Compile and execute the script.
	CScriptEngine *scriptEngine = server->getScriptEngine();
	bool executed = scriptEngine->ExecuteWeapon(this);
	if (executed)
	{
		SCRIPTENV_D("WEAPON SCRIPT COMPILED\n");

		if (!_source.getServerSide().empty()) {
			_scriptExecutionContext.addAction(scriptEngine->CreateAction("weapon.created", getScriptObject()));
			scriptEngine->RegisterWeaponUpdate(this);
		}
	}
	else
		SCRIPTENV_D("Could not compile weapon script\n");
#endif

	// Clear any GS1 scripts/GS2 bytecode
	_bytecode.clear();
	_formattedClientGS1.clear();

	// Compile GS2 code
	if (!_source.getClientGS2().empty())
	{
		// Compile gs2 code
		server->compileGS2Script(this, [this](const CompilerResponse &response) {
			if (response.success)
			{
				// these should be sent for compilation right after
				_joinedClasses = { response.joinedClasses.begin(), response.joinedClasses.end() };

				auto bytecodeWithHeader = GS2Context::CreateHeader(response.bytecode, "weapon", _weaponName, true);
				_bytecode.clear(bytecodeWithHeader.length());
				_bytecode.write((const char*)bytecodeWithHeader.buffer(), bytecodeWithHeader.length());
			}
		});
	}
	
	auto gs1Script = _source.getClientGS1();
	if (!gs1Script.empty())
		setClientScript(std::string{ gs1Script });
	

	// Save Weapon
	if (pSaveWeapon)
		saveWeapon();
}

void TWeapon::setClientScript(const CString& pScript)
{
	// Remove any comments in the code
	CString formattedScript = removeComments(pScript);

	// Extra padding incase we need to add //#CLIENTSIDE to the script
	_formattedClientGS1.clear(formattedScript.length() + 14);

	if (formattedScript.find("//#CLIENTSIDE") != 0) {
		_formattedClientGS1 << "//#CLIENTSIDE" << "\xa7";
	}

	// Split code into tokens, trim each line, and use the clientside line ending '\xa7'
	std::vector<CString> code = formattedScript.tokenize("\n");
	for (auto & it : code)
		_formattedClientGS1 << it.trim() << "\xa7";
}

#ifdef V8NPCSERVER

void TWeapon::freeScriptResources()
{
	CScriptEngine *scriptEngine = server->getScriptEngine();

	scriptEngine->ClearCache<TWeapon>(_source.getServerSide());

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
		_scriptObject.reset();
	}
}

void TWeapon::queueWeaponAction(TPlayer *player, const std::string& args)
{
	CScriptEngine *scriptEngine = server->getScriptEngine();

	ScriptAction scriptAction = scriptEngine->CreateAction("weapon.serverside", getScriptObject(), player->getScriptObject(), args);
	_scriptExecutionContext.addAction(scriptAction);
	scriptEngine->RegisterWeaponUpdate(this);
}

#endif
