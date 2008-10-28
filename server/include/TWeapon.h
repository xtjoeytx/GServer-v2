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

	char getWeaponId() const			{ boost::recursive_mutex::scoped_lock lock(m_preventChange); return defaultWeaponId; }
	time_t getModTime() const			{ boost::recursive_mutex::scoped_lock lock(m_preventChange); return modTime; }
	CString getName() const;
	CString getImage() const			{ boost::recursive_mutex::scoped_lock lock(m_preventChange); return image; }
	CString getServerScript() const		{ boost::recursive_mutex::scoped_lock lock(m_preventChange); return serverScript; }
	CString getClientScript() const		{ boost::recursive_mutex::scoped_lock lock(m_preventChange); return clientScript; }

	void setServerScript(const CString& pScript)	{ boost::recursive_mutex::scoped_lock lock(m_preventChange); serverScript = pScript; }
	void setClientScript(const CString& pScript)	{ boost::recursive_mutex::scoped_lock lock(m_preventChange); clientScript = pScript; }

private:
	CString name;
	CString image;
	CString serverScript;
	CString clientScript;
	time_t modTime;
	bool defaultWeapon;
	char defaultWeaponId;

	mutable boost::recursive_mutex m_preventChange;
};

#endif
