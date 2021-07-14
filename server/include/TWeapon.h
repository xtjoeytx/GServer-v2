#ifndef TWEAPON_H
#define TWEAPON_H

#include <vector>
#include <time.h>
#include "CString.h"
#include "TLevelItem.h"

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
		TWeapon(TServer *pServer, const CString& pName, const CString& pImage, const CString& pScript, const time_t pModTime = 0, bool pSaveWeapon = false);
		~TWeapon();

		// -- Functions -- //
		bool saveWeapon();
		void updateWeapon(const CString& pImage, const CString& pCode, const time_t pModTime = 0, bool pSaveWeapon = true);

		static TWeapon* loadWeapon(const CString& pWeapon, TServer* server);

		// Functions -> Inline Get-Functions
		CString getWeaponPacket(bool forceGS1 = false) const;
		inline bool isDefault() const					{ return (mWeaponDefault != LevelItemType::INVALID); }
		inline bool hasBytecode() const					{ return (!mByteCode.empty()); }
		inline LevelItemType getWeaponId()				{ return mWeaponDefault; }
		inline const CString& getByteCodeFile() const	{ return mByteCodeFile; }
		inline const CString& getImage() const			{ return mWeaponImage; }
		inline const CString& getName() const			{ return mWeaponName; }
		inline const CString& getClientScript() const	{ return mScriptClient; }
		inline const CString& getServerScript() const	{ return mScriptServer; }
		inline const CString& getFullScript() const		{ return mWeaponScript; }
		inline time_t getModTime() const				{ return mModTime; }

		// Functions -> Set Variables
		void setImage(const CString& pImage)			{ mWeaponImage = pImage; }
		void setFullScript(const CString& pScript)		{ mWeaponScript = pScript; }
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
		void setServerScript(const CString& pScript) { mScriptServer = pScript; }

		// Varaibles -> Weapon Data
		LevelItemType mWeaponDefault;
		CString mWeaponImage, mWeaponName, mWeaponScript, mByteCodeFile;
		CString mScriptClient, mScriptServer;
		std::vector<std::pair<CString, CString> > mByteCode;
		time_t mModTime;
		TServer *server;

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
