#ifndef TLEVELSIGN_H
#define TLEVELSIGN_H

#include "CString.h"

class TPlayer;

class TLevelSign
{
	public:
		TLevelSign(const int pX, const int pY, const CString& pSign, bool encoded = false);

		// functions
		CString getSignStr(TPlayer *pPlayer = 0) const;

		// get private variables
		int getX() const					{ return x; }
		int getY() const					{ return y; }
		CString getText() const				{ return text; }
		CString getUText() const			{ return unformattedText; }

		void setX(int value = 0)			{ x = value; }
		void setY(int value = 0)			{ y = value; }
		void setText(const CString& value)	{ text = value; }
		void setUText(const CString& value)	{ unformattedText = value; }

	private:
		int x, y;
		CString text;
		CString unformattedText;
};

#endif // TLEVELSIGN_H
