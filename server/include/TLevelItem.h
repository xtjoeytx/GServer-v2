#ifndef TLEVELITEM_H
#define TLEVELITEM_H

#include <time.h>
#include "ICommon.h"
#include "CTimeout.h"
#include "CString.h"

class TPlayer;
class TLevelItem
{
	public:
		TLevelItem(float pX, float pY, char pItem) : x(pX), y(pY), item(pItem), modTime(time(0)) { timeout.setTimeout(10); }

		// Return the packet to be sent to the player.
		CString getItemStr() const;

		// Static functions.
		static int getItemId(const CString& pItemName);
		static CString getItemName(const unsigned char id);
		static CString getItemPlayerProp(const char pItemId, TPlayer* player);
		static CString getItemPlayerProp(const CString& pItemName, TPlayer* player);

		// Get functions.
		float getX() const			{ boost::recursive_mutex::scoped_lock lock(m_preventChange); return x; }
		float getY() const			{ boost::recursive_mutex::scoped_lock lock(m_preventChange); return y; }
		char getItem() const		{ boost::recursive_mutex::scoped_lock lock(m_preventChange); return item; }
		time_t getModTime() const	{ boost::recursive_mutex::scoped_lock lock(m_preventChange); return modTime; }

		CTimeout timeout;

	private:
		float x;
		float y;
		char item;
		time_t modTime;
		mutable boost::recursive_mutex m_preventChange;
};


#endif // TLEVELITEM_H
