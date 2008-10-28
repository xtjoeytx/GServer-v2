#ifndef TLEVELHORSE_H
#define TLEVELHORSE_H

#include "ICommon.h"
#include "CString.h"
#include "CTimeout.h"

class TLevelHorse
{
	public:
		// constructor - destructor
		TLevelHorse(const CString& pImage, float pX, float pY, char pDir = 0, char pBushes = 0) : image(pImage), x(pX), y(pY), dir(pDir), bushes(pBushes) { timeout.setTimeout(180); }

		CString getHorseStr() const;

		// get private variables
		CString getImage() const	{ boost::recursive_mutex::scoped_lock lock(m_preventChange); return image; }
		float getX() const			{ boost::recursive_mutex::scoped_lock lock(m_preventChange); return x; }
		float getY() const			{ boost::recursive_mutex::scoped_lock lock(m_preventChange); return y; }
		char getDir() const			{ boost::recursive_mutex::scoped_lock lock(m_preventChange); return dir; }
		char getBushes() const		{ boost::recursive_mutex::scoped_lock lock(m_preventChange); return bushes; }

		CTimeout timeout;

	private:
		CString image;
		float x, y;
		char dir, bushes;
		mutable boost::recursive_mutex m_preventChange;
};


#endif // TLEVELHORSE_H
