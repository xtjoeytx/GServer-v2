#ifndef TWEAPON_H
#define TWEAPON_H

#include <vector>
#include <time.h>
#include "CString.h"

#ifdef V8NPCSERVER
#include <string>
#include "ScriptWrapped.h"

class TPlayer;
#endif

class TServer;
class TWeapon
{
	public:
		// -- Constructor | Destructor -- //
		TWeapon(const signed char pId);
		TWeapon(TServer *pServer, const CString& pName, const CString& pImage, const CString& pScript, const time_t pModTime = 0, bool pSaveWeapon = false);
		~TWeapon();

		// -- Functions -- //
		bool saveWeapon(TServer* server);
		void updateWeapon(TServer *pServer, const CString& pImage, const CString& pCode, const time_t pModTime = 0, bool pSaveWeapon = true);

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
		void setClientScript(const CString& pScript)	{ mScriptClient = pScript; }
		void setServerScript(const CString& pScript)	{ mScriptServer = pScript; }
		void setFullScript(const CString& pScript)		{ mWeaponScript = pScript; }
		void setModTime(time_t pModTime)				{ mModTime = pModTime; }

#ifdef V8NPCSERVER
		void queueWeaponAction(TServer *server, TPlayer *player, const std::string& args);

		inline IScriptWrapped<TWeapon> * getScriptObject() const {
			return _scriptObject;
		}

		inline void setScriptObject(IScriptWrapped<TWeapon> *object) {
			_scriptObject = object;
		}
#endif
	protected:
		// Varaibles -> Weapon Data
		signed char mWeaponDefault;
		CString mWeaponImage, mWeaponName, mWeaponScript;
		CString mScriptClient, mScriptServer;
		std::vector<std::pair<CString, CString> > mByteCode;
		time_t mModTime;

#ifdef V8NPCSERVER
		IScriptWrapped<TWeapon> *_scriptObject;
#endif
};

#endif
