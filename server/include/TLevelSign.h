#ifndef TLEVELSIGN_H
#define TLEVELSIGN_H

#include <vector>
#include "ICommon.h"
#include "CString.h"

class TLevelSign
{
	public:
		// constructor - destructor
		TLevelSign() : x(0), y(0) { }
		TLevelSign(const int pX, const int pY, const CString& pSign, bool encoded = false);

		// functions
		CString getSignStr() const;

		// get private variables
		int getX()					{ return x; }
		int getY()					{ return y; }
		CString getText()			{ return text; }
		CString getUText()			{ return unformattedText; }

	private:
		int x, y;
		CString text;
		CString unformattedText;
};

#endif // TLEVELSIGN_H
