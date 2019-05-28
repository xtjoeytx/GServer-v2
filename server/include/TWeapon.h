#ifndef TWEAPON_H
#define TWEAPON_H

#include <vector>
#include <time.h>
#include "CString.h"

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
		TWeapon(TServer *pServer, const signed char pId);
		TWeapon(TServer *pServer, const CString& pName, const CString& pImage, const CString& pScript, const time_t pModTime = 0, bool pSaveWeapon = false);
		~TWeapon();

		// -- Functions -- //
		bool saveWeapon();
		void updateWeapon(const CString& pImage, const CString& pCode, const time_t pModTime = 0, bool pSaveWeapon = true);

		static TWeapon* loadWeapon(const CString& pWeapon, TServer* server);

		// Functions -> Inline Get-Functions
		CString getWeaponPacket() const;
		inline bool isDefault() const					{ return (mWeaponDefault != -1); }
		inline signed char getWeaponId()				{ return mWeaponDefault; }
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
		ScriptExecutionContext * getExecutionContext();
		IScriptWrapped<TWeapon> * getScriptObject() const;
		
		void freeScriptResources();
		void queueWeaponAction(TPlayer *player, const std::string& args);
		void runScriptEvents();
		void setScriptObject(IScriptWrapped<TWeapon> *object);
#endif
	protected:
		void setClientScript(const CString& pScript);
		void setServerScript(const CString& pScript) { mScriptServer = pScript; }

		// Varaibles -> Weapon Data
		signed char mWeaponDefault;
		CString mWeaponImage, mWeaponName, mWeaponScript;
		CString mScriptClient, mScriptServer;
		std::vector<std::pair<CString, CString> > mByteCode;
		time_t mModTime;
		TServer *server;

	private:
#ifdef V8NPCSERVER
		IScriptWrapped<TWeapon> *_scriptObject;
		ScriptExecutionContext _scriptExecutionContext;
#endif
};

#ifdef V8NPCSERVER

inline ScriptExecutionContext * TWeapon::getExecutionContext() {
	return &_scriptExecutionContext;
}

inline IScriptWrapped<TWeapon> * TWeapon::getScriptObject() const {
	return _scriptObject;
}

inline void TWeapon::runScriptEvents() {
	_scriptExecutionContext.runExecution();
}

inline void TWeapon::setScriptObject(IScriptWrapped<TWeapon> *object) {
	_scriptObject = object;
}

#endif

#endif
