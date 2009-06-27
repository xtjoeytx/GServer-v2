#ifndef TWEAPON_H
#define TWEAPON_H

#include <time.h>
#include "ICommon.h"
#include "TNPC.h"
#include "CString.h"

class TServer;
class TWeapon
{
public:
	TWeapon(const char id) : modTime(0), defaultWeapon(true), defaultWeaponId(id) {}
	TWeapon(const CString& pName, const CString& pImage, const CString& pScript, const time_t pModTime = 0, bool trimCode = false);

	static TWeapon* loadWeapon(const CString& pWeapon, TServer* server);
	bool saveWeapon(TServer* server);

	CString getWeaponPacket() const;
	bool isDefault() const				{ return defaultWeapon; }

	char getWeaponId() const			{ return defaultWeaponId; }
	time_t getModTime() const			{ return modTime; }
	CString getName() const;
	CString getImage() const			{ return image; }
	CString getServerScript() const		{ return serverScript; }
	CString getClientScript() const		{ return clientScript; }
	CString getFullScript() const		{ return fullScript; }

	void setImage(const CString& pImage)			{ image = pImage; }
	void setServerScript(const CString& pScript)	{ serverScript = pScript; }
	void setClientScript(const CString& pScript)	{ clientScript = pScript; }
	void setFullScript(const CString& pScript)		{ fullScript = pScript; }
	void setModTime(time_t mod)						{ modTime = mod; }

private:
	CString name;
	CString image;
	CString serverScript;
	CString clientScript;
	CString fullScript;
	time_t modTime;
	bool defaultWeapon;
	char defaultWeaponId;
};

#endif
