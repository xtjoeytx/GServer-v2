#ifndef TWEAPON_H
#define TWEAPON_H

#include <vector>
#include <time.h>
#include "CString.h"
#include "TLevelItem.h"
#include "SourceCode.h"

#ifdef V8NPCSERVER
#include <string>
#include "ScriptBindings.h"
#include "ScriptExecutionContext.h"

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
		CString getWeaponPacket(bool forceGS1 = false) const;
		inline bool isDefault() const					{ return (mWeaponDefault != LevelItemType::INVALID); }
		inline bool hasBytecode() const					{ return (!_bytecode.isEmpty()); }
		inline LevelItemType getWeaponId()				{ return mWeaponDefault; }
		inline const CString& getByteCodeFile() const	{ return _bytecodeFile; }
		inline const std::string& getImage() const		{ return _weaponImage; }
		inline const std::string& getName() const		{ return _weaponName; }
		inline const std::string& getFullScript() const	{ return _source.getSource(); }
		inline std::string_view getServerScript() const { return _source.getServerSide(); }
		inline time_t getModTime() const				{ return mModTime; }

		// Functions -> Set Variables
		void setModTime(time_t pModTime)				{ mModTime = pModTime; }

#ifdef V8NPCSERVER
		ScriptExecutionContext& getExecutionContext();
		IScriptObject<TWeapon> * getScriptObject() const;

		void freeScriptResources();
		void queueWeaponAction(TPlayer *player, const std::string& args);
		void runScriptEvents();
		void setScriptObject(IScriptObject<TWeapon> *object);
#endif
	protected:
		void setClientScript(const CString& pScript);
		
		// Varaibles -> Weapon Data
		LevelItemType mWeaponDefault;
		//CString mByteCodeFile;
		//CString mByteCodeData;
		time_t mModTime;
		TServer *server;

		SourceCode _source;
		CString clientScriptFormatted;

		CString _bytecode;
		CString _bytecodeFile;

		std::string _weaponImage;
		std::string _weaponName;
		std::string _clientFormattedScript;

	private:
#ifdef V8NPCSERVER
		IScriptObject<TWeapon> *_scriptObject;
		ScriptExecutionContext _scriptExecutionContext;
#endif
};

#ifdef V8NPCSERVER

inline ScriptExecutionContext& TWeapon::getExecutionContext() {
	return _scriptExecutionContext;
}

inline IScriptObject<TWeapon> * TWeapon::getScriptObject() const {
	return _scriptObject;
}

inline void TWeapon::runScriptEvents() {
	_scriptExecutionContext.runExecution();
}

inline void TWeapon::setScriptObject(IScriptObject<TWeapon> *object) {
	_scriptObject = object;
}

#endif

#endif
