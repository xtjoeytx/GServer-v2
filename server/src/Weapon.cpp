#include <IDebug.h>

#include <IEnums.h>
#include <IUtil.h>

// GS2 Compiler includes
#include <GS2Context.h>

#include "NPC.h"
#include "Server.h"
#include "Weapon.h"
#include "level/LevelItem.h"

#ifdef V8NPCSERVER
	#include "Player.h"
#endif

// -- Constructor: Default Weapons -- //
Weapon::Weapon(LevelItemType pId)
	: m_modTime(0), m_weaponDefault(pId)
#ifdef V8NPCSERVER
	  ,
	  m_scriptExecutionContext(m_server->getScriptEngine())
#endif
{
	m_weaponName = LevelItem::getItemName(m_weaponDefault);
}

// -- Constructor: Weapon Script -- //
Weapon::Weapon(std::string pName, std::string pImage, std::string pScript, const time_t pModTime, bool pSaveWeapon)
	: m_weaponName(std::move(pName)), m_modTime(pModTime), m_weaponDefault(LevelItemType::INVALID)
#ifdef V8NPCSERVER
	  ,
	  m_scriptExecutionContext(m_server->getScriptEngine())
#endif
{
	// Update Weapon
	this->updateWeapon(std::move(pImage), std::move(pScript), pModTime, pSaveWeapon);
}

Weapon::~Weapon()
{
#ifdef V8NPCSERVER
	freeScriptResources();
#endif
}

// -- Function: Load Weapon -- //
std::shared_ptr<Weapon> Weapon::loadWeapon(const CString& pWeapon)
{
	// File Path
	CString fileName = m_server->getServerPath() << "weapons" << FileSystem::getPathSeparator() << pWeapon;

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

			byteCodeData.load(m_server->getServerPath() << "weapon_bytecode/" << fileName);
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
			}
			while (fileData.bytesLeft());
		}
	}

	// Valid Weapon Name?
	if (weaponName.empty())
		return nullptr;

	// Give a warning if our weapon was malformed.
	if (has_scriptend && !found_scriptend)
	{
		m_server->getServerLog().out("[%s] WARNING: Weapon %s is malformed.\n", m_server->getName().text(), weaponName.c_str());
		m_server->getServerLog().out("[%s] SCRIPTEND needs to be on its own line.\n", m_server->getName().text());
	}

	// Give a warning if both a script and a bytecode was found.
	if (!weaponScript.empty() && !byteCodeData.isEmpty())
	{
		m_server->getServerLog().out("[%s] WARNING: Weapon %s includes both script and bytecode.  Using bytecode.\n", m_server->getName().text(), weaponName.c_str());
		weaponScript.clear();
	}

	auto weapon = std::make_shared<Weapon>(weaponName, weaponImage, weaponScript, 0);
	if (!byteCodeData.isEmpty())
	{
		weapon->m_bytecode = CString(std::move(byteCodeData));
		weapon->m_bytecodeFile = std::move(byteCodeFile);
	}

	return weapon;
}

// -- Function: Save Weapon -- //
bool Weapon::saveWeapon()
{
	// Don't save default weapons / empty weapons
	if (this->isDefault() || m_weaponName.empty())
		return false;

	// If the bytecode filename is set, the weapon is treated as read-only so it can't be saved
	if (!m_bytecodeFile.empty())
		return false;

	// Prevent the loading/saving of filenames with illegal characters.
	CString name = m_weaponName;
	name.replaceAllI("\\", "_");
	name.replaceAllI("/", "_");
	name.replaceAllI("*", "@");
	name.replaceAllI(":", ";");
	name.replaceAllI("?", "!");
	CString filename = m_server->getServerPath() << "weapons" << FileSystem::getPathSeparator() << "weapon" << name << ".txt";

	// Write the File.
	CString output = "GRAWP001\r\n";
	output << "REALNAME " << m_weaponName << "\r\n";
	output << "IMAGE " << m_weaponImage << "\r\n";

	if (m_source)
	{
		output << "SCRIPT\r\n";
		output << CString(m_source.getSource()).replaceAll("\n", "\r\n");

		// Append a new line to the end of the script if one doesn't exist.
		if (m_source.getSource().back() != '\n')
			output << "\r\n";

		output << "SCRIPTEND\r\n";
	}

	// Save it.
	return output.save(filename);
}

// -- Function: Get Player Packet -- //
CString Weapon::getWeaponPacket(int clientVersion) const
{
	if (this->isDefault())
		return CString() >> (char)PLO_DEFAULTWEAPON >> (char)m_weaponDefault;

	CString weaponPacket;
	weaponPacket >> (char)PLO_NPCWEAPONADD >> (char)m_weaponName.length() << m_weaponName >> (char)NPCPROP_IMAGE >> (char)m_weaponImage.length() << m_weaponImage;

	// GS2 is available for v4+
	if (clientVersion >= CLVER_4_0211)
	{
		if (!m_bytecode.isEmpty())
		{
			weaponPacket >> (char)NPCPROP_CLASS >> (short)0 << "\n";

			CString b = m_bytecode;
			CString header = b.readChars(b.readGUShort());

			// Get the mod time and send packet 197.
			weaponPacket >> (char)PLO_UNKNOWN197 << header << "," >> (long long)time(0) << "\n";
			return weaponPacket;
		}

		// GS1 is disabled for > 5.0.0.7
		if (clientVersion > CLVER_5_07)
			return weaponPacket;
	}

	weaponPacket >> (char)NPCPROP_SCRIPT >> (short)m_formattedClientGS1.length() << m_formattedClientGS1;
	return weaponPacket;
}

// -- Function: Update Weapon Image/Script -- //
void Weapon::updateWeapon(std::string pImage, std::string pCode, const time_t pModTime, bool pSaveWeapon)
{
#ifdef V8NPCSERVER
	// Clear script function
	if (m_source || m_scriptExecutionContext.hasActions())
		freeScriptResources();
#endif

	bool gs2default = m_server->getSettings().getBool("gs2default", false);

	m_source = SourceCode{ std::move(pCode), gs2default };
	m_weaponImage = std::move(pImage);
	setModTime(pModTime == 0 ? time(0) : pModTime);

#ifdef V8NPCSERVER
	// Compile and execute the script.
	ScriptEngine* scriptEngine = m_server->getScriptEngine();
	bool executed = scriptEngine->executeWeapon(this);
	if (executed)
	{
		SCRIPTENV_D("WEAPON SCRIPT COMPILED\n");

		if (!m_source.getServerSide().empty())
		{
			m_scriptExecutionContext.addAction(scriptEngine->createAction("weapon.created", getScriptObject()));
			scriptEngine->registerWeaponUpdate(this);
		}
	}
	else
		SCRIPTENV_D("Could not compile weapon script\n");
#endif

	// Clear any GS1 scripts/GS2 bytecode
	m_bytecode.clear();
	m_formattedClientGS1.clear();

	// Compile GS2 code
	if (!m_source.getClientGS2().empty())
	{
		// Compile gs2 code
		m_server->compileGS2Script(this, [this](const CompilerResponse& response)
								   {
									   if (response.success)
									   {
										   // these should be sent for compilation right after
										   m_joinedClasses = { response.joinedClasses.begin(), response.joinedClasses.end() };

										   auto bytecodeWithHeader = GS2Context::CreateHeader(response.bytecode, "weapon", m_weaponName, true);
										   m_bytecode.clear(bytecodeWithHeader.length());
										   m_bytecode.write((const char*)bytecodeWithHeader.buffer(), static_cast<int>(bytecodeWithHeader.length()));
									   }
								   });
	}

	auto gs1Script = m_source.getClientGS1();
	if (!gs1Script.empty())
		setClientScript(std::string{ gs1Script });

	// Save Weapon
	if (pSaveWeapon)
		saveWeapon();
}

void Weapon::setClientScript(const CString& pScript)
{
	// Remove any comments in the code
	CString formattedScript = removeComments(pScript);

	// Extra padding incase we need to add //#CLIENTSIDE to the script
	m_formattedClientGS1.clear(static_cast<size_t>(formattedScript.length()) + 14);

	if (formattedScript.find("//#CLIENTSIDE") != 0)
	{
		m_formattedClientGS1 << "//#CLIENTSIDE"
							 << "\xa7";
	}

	// Split code into tokens, trim each line, and use the clientside line ending '\xa7'
	std::vector<CString> code = formattedScript.tokenize("\n");
	for (auto& it: code)
		m_formattedClientGS1 << it.trim() << "\xa7";
}

#ifdef V8NPCSERVER

void Weapon::freeScriptResources()
{
	ScriptEngine* scriptEngine = m_server->getScriptEngine();

	scriptEngine->clearCache<Weapon>(m_source.getServerSide());

	// Clear any queued actions
	if (m_scriptExecutionContext.hasActions())
	{
		// Unregister npc from any queued event calls
		scriptEngine->unregisterWeaponUpdate(this);

		// Reset execution
		m_scriptExecutionContext.resetExecution();
	}

	// Delete script object
	if (m_scriptObject)
	{
		m_scriptObject.reset();
	}
}

void Weapon::queueWeaponAction(Player* player, const std::string& args)
{
	ScriptEngine* scriptEngine = m_server->getScriptEngine();

	ScriptAction scriptAction = scriptEngine->createAction("weapon.serverside", getScriptObject(), player->getScriptObject(), args);
	m_scriptExecutionContext.addAction(scriptAction);
	scriptEngine->registerWeaponUpdate(this);
}

#endif
