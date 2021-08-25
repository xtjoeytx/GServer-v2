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
		int getX() const			{ return x; }
		int getY() const			{ return y; }
		CString getText() const		{ return text; }
		CString getUText() const	{ return unformattedText; }

	private:
		int x, y;
		CString text;
		CString unformattedText;
};

#endif // TLEVELSIGN_H
