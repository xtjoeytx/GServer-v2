#ifndef TLEVELITEM_H
#define TLEVELITEM_H

#include <time.h>
#include "CTimeout.h"
#include "CString.h"

class TPlayer;
class TLevelItem
{
	public:
		TLevelItem(float pX, float pY, signed char pItem) : x(pX), y(pY), item(pItem), modTime(time(0)) { timeout.setTimeout(10); }

		// Return the packet to be sent to the player.
		CString getItemStr() const;

		// Static functions.
		static int getItemId(const CString& pItemName);
		static CString getItemName(const unsigned char id);
		static CString getItemPlayerProp(const signed char pItemId, TPlayer* player);
		static CString getItemPlayerProp(const CString& pItemName, TPlayer* player);

		// Get functions.
		float getX() const			{ return x; }
		float getY() const			{ return y; }
		signed char getItem() const	{ return item; }
		time_t getModTime() const	{ return modTime; }

		CTimeout timeout;

	private:
		float x;
		float y;
		signed char item;
		time_t modTime;
};


#endif // TLEVELITEM_H
