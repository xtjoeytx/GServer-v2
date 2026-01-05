#ifndef TWEAPON_H
#define TWEAPON_H

#include <memory>
#include <time.h>
#include <vector>
#include <string>

#include <CString.h>
#include "BabyDI.h"

#include "level/LevelItem.h"
#include "scripting/SourceCode.h"

#ifdef V8NPCSERVER
	#include "scripting/interface/ScriptBindings.h"
	#include "scripting/ScriptExecutionContext.h"

class Player;
#endif

class Server;
class Weapon
{
public:
	// -- Constructor | Destructor -- //
	Weapon(LevelItemType itemType);
	Weapon(std::string pName, std::string pImage, std::string pScript, const time_t pModTime = 0, bool pSaveWeapon = false);
	~Weapon();

	// -- Functions -- //
	bool saveWeapon();
	void updateWeapon(std::string pImage, std::string pScript, const time_t pModTime = 0, bool pSaveWeapon = true);

	static std::shared_ptr<Weapon> loadWeapon(const CString& pWeapon);

	// Functions -> Inline Get-Functions
	CString getWeaponPacket(int clientVersion) const;
	bool isDefault() const { return (m_weaponDefault != LevelItemType::INVALID); }
	bool hasBytecode() const { return (!m_bytecode.isEmpty()); }
	LevelItemType getWeaponId() const { return m_weaponDefault; }
	const SourceCode& getSource() const { return m_source; }
	const CString& getByteCode() const { return m_bytecode; }
	const std::string& getByteCodeFile() const { return m_bytecodeFile; }
	const std::string& getImage() const { return m_weaponImage; }
	const std::string& getName() const { return m_weaponName; }
	const std::string& getFullScript() const { return m_source.getSource(); }
	std::string_view getServerScript() const { return m_source.getServerSide(); }
	time_t getModTime() const { return m_modTime; }

	// Functions -> Set Variables
	void setModTime(time_t pModTime) { m_modTime = pModTime; }

#ifdef V8NPCSERVER
	ScriptExecutionContext& getExecutionContext();
	IScriptObject<Weapon>* getScriptObject() const;

	void freeScriptResources();
	void queueWeaponAction(Player* player, const std::string& args);
	void runScriptEvents();
	void setScriptObject(std::unique_ptr<IScriptObject<Weapon>> object);
#endif
protected:
	BabyDI_INJECT(Server, m_server);

	void setClientScript(const CString& pScript);

	// Varaibles -> Weapon Data
	LevelItemType m_weaponDefault;
	time_t m_modTime;

	SourceCode m_source;
	CString m_formattedClientGS1;

	CString m_bytecode;
	std::string m_bytecodeFile;

	std::string m_weaponImage;
	std::string m_weaponName;
	std::vector<std::string> m_joinedClasses;

private:
#ifdef V8NPCSERVER
	std::unique_ptr<IScriptObject<Weapon>> m_scriptObject;
	ScriptExecutionContext m_scriptExecutionContext;
#endif
};
using TWeaponPtr = std::shared_ptr<Weapon>;

#ifdef V8NPCSERVER

inline ScriptExecutionContext& Weapon::getExecutionContext()
{
	return m_scriptExecutionContext;
}

inline IScriptObject<Weapon>* Weapon::getScriptObject() const
{
	return m_scriptObject.get();
}

inline void Weapon::runScriptEvents()
{
	m_scriptExecutionContext.runExecution();
}

inline void Weapon::setScriptObject(std::unique_ptr<IScriptObject<Weapon>> object)
{
	m_scriptObject = std::move(object);
}

#endif

#endif
