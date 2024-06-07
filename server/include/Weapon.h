#ifndef TWEAPON_H
#define TWEAPON_H

#include "CString.h"
#include "LevelItem.h"
#include "SourceCode.h"
#include <memory>
#include <time.h>
#include <vector>

#ifdef V8NPCSERVER
	#include "ScriptBindings.h"
	#include "ScriptExecutionContext.h"
	#include <string>

class Player;
#endif

class Server;
class Weapon
{
public:
	// -- Constructor | Destructor -- //
	Weapon(Server* pServer, LevelItemType itemType);
	Weapon(Server* pServer, std::string pName, std::string pImage, std::string pScript, const time_t pModTime = 0, bool pSaveWeapon = false);
	~Weapon();

	// -- Functions -- //
	bool saveWeapon();
	void updateWeapon(std::string pImage, std::string pScript, const time_t pModTime = 0, bool pSaveWeapon = true);

	static std::shared_ptr<Weapon> loadWeapon(const CString& pWeapon, Server* server);

	// Functions -> Inline Get-Functions
	CString getWeaponPacket(int clientVersion) const;
	bool isDefault() const { return (mWeaponDefault != LevelItemType::INVALID); }
	bool hasBytecode() const { return (!_bytecode.isEmpty()); }
	LevelItemType getWeaponId() { return mWeaponDefault; }
	const SourceCode& getSource() const { return _source; }
	const CString& getByteCode() const { return _bytecode; }
	const std::string& getByteCodeFile() const { return _bytecodeFile; }
	const std::string& getImage() const { return _weaponImage; }
	const std::string& getName() const { return _weaponName; }
	const std::string& getFullScript() const { return _source.getSource(); }
	std::string_view getServerScript() const { return _source.getServerSide(); }
	time_t getModTime() const { return mModTime; }

	// Functions -> Set Variables
	void setModTime(time_t pModTime) { mModTime = pModTime; }

#ifdef V8NPCSERVER
	ScriptExecutionContext& getExecutionContext();
	IScriptObject<Weapon>* getScriptObject() const;

	void freeScriptResources();
	void queueWeaponAction(Player* player, const std::string& args);
	void runScriptEvents();
	void setScriptObject(std::unique_ptr<IScriptObject<Weapon>> object);
#endif
protected:
	void setClientScript(const CString& pScript);

	// Varaibles -> Weapon Data
	LevelItemType mWeaponDefault;
	time_t mModTime;
	Server* server;

	SourceCode _source;
	CString _formattedClientGS1;

	CString _bytecode;
	std::string _bytecodeFile;

	std::string _weaponImage;
	std::string _weaponName;
	std::vector<std::string> _joinedClasses;

private:
#ifdef V8NPCSERVER
	std::unique_ptr<IScriptObject<Weapon>> _scriptObject;
	ScriptExecutionContext _scriptExecutionContext;
#endif
};
using TWeaponPtr = std::shared_ptr<Weapon>;

#ifdef V8NPCSERVER

inline ScriptExecutionContext& Weapon::getExecutionContext()
{
	return _scriptExecutionContext;
}

inline IScriptObject<Weapon>* Weapon::getScriptObject() const
{
	return _scriptObject.get();
}

inline void Weapon::runScriptEvents()
{
	_scriptExecutionContext.runExecution();
}

inline void Weapon::setScriptObject(std::unique_ptr<IScriptObject<Weapon>> object)
{
	_scriptObject = std::move(object);
}

#endif

#endif
