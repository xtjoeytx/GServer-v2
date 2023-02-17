#ifndef TWEAPON_H
#define TWEAPON_H

#include <vector>
#include <time.h>
#include "CString.h"
#include "TLevelItem.h"
#include "TPacket.h"
#include "SourceCode.h"
#include "IEnums.h"

#ifdef V8NPCSERVER
#include <memory>
#include <string>
#include "ScriptBindings.h"
#include "ScriptExecutionContext.h"
#include "TPacket.h"

class TPlayer;
#endif

class TServer;
class TWeapon
{
	public:
		// -- Constructor | Destructor -- //
		TWeapon(TServer *pServer, LevelItemType itemType);
		TWeapon(TServer *pServer, std::string pName, std::string pImage, std::string pScript, const time_t pModTime = 0, bool pSaveWeapon = false);
		~TWeapon();

		// -- Functions -- //
		bool saveWeapon();
		void updateWeapon(std::string pImage, std::string pScript, const time_t pModTime = 0, bool pSaveWeapon = true);

		static TWeapon* loadWeapon(const CString& pWeapon, TServer* server);

		// Functions -> Inline Get-Functions
		std::vector<TPacket<PlayerOutPacket>> getWeaponPackets(int clientVersion) const;
		bool isDefault() const						{ return (mWeaponDefault != LevelItemType::INVALID); }
		bool hasBytecode() const					{ return (!_bytecode.isEmpty()); }
		LevelItemType getWeaponId()					{ return mWeaponDefault; }
		const SourceCode& getSource() const			{ return _source; }
		const CString& getByteCode() const			{ return _bytecode; }
		const std::string& getByteCodeFile() const	{ return _bytecodeFile; }
		const std::string& getImage() const			{ return _weaponImage; }
		const std::string& getName() const			{ return _weaponName; }
		const std::string& getFullScript() const	{ return _source.getSource(); }
		std::string_view getServerScript() const	{ return _source.getServerSide(); }
		time_t getModTime() const					{ return mModTime; }

		// Functions -> Set Variables
		void setModTime(time_t pModTime)				{ mModTime = pModTime; }

#ifdef V8NPCSERVER
		ScriptExecutionContext& getExecutionContext();
		IScriptObject<TWeapon> * getScriptObject() const;

		void freeScriptResources();
		void queueWeaponAction(TPlayer *player, const std::string& args);
		void runScriptEvents();
		void setScriptObject(std::unique_ptr<IScriptObject<TWeapon>> object);
#endif
	protected:
		void setClientScript(const CString& pScript);

		// Varaibles -> Weapon Data
		LevelItemType mWeaponDefault;
		time_t mModTime;
		TServer *server;

		SourceCode _source;
		CString _formattedClientGS1;

		CString _bytecode;
		std::string _bytecodeFile;

		std::string _weaponImage;
		std::string _weaponName;
		std::vector<std::string> _joinedClasses;

	private:
#ifdef V8NPCSERVER
		std::unique_ptr<IScriptObject<TWeapon>> _scriptObject;
		ScriptExecutionContext _scriptExecutionContext;
#endif
};

#ifdef V8NPCSERVER

inline ScriptExecutionContext& TWeapon::getExecutionContext() {
	return _scriptExecutionContext;
}

inline IScriptObject<TWeapon> * TWeapon::getScriptObject() const {
	return _scriptObject.get();
}

inline void TWeapon::runScriptEvents() {
	_scriptExecutionContext.runExecution();
}

inline void TWeapon::setScriptObject(std::unique_ptr<IScriptObject<TWeapon>> object) {
	_scriptObject = std::move(object);
}

#endif

#endif
