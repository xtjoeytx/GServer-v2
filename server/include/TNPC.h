#ifndef TNPC_H
#define TNPC_H

#include <time.h>
#include "CString.h"
#include "IUtil.h"

enum
{
	NPCPROP_IMAGE			= 0,
	NPCPROP_SCRIPT			= 1,
	NPCPROP_X				= 2,
	NPCPROP_Y				= 3,
	NPCPROP_POWER			= 4,
	NPCPROP_RUPEES			= 5,
	NPCPROP_ARROWS			= 6,
	NPCPROP_BOMBS			= 7,
	NPCPROP_GLOVEPOWER		= 8,
	NPCPROP_BOMBPOWER		= 9,
	NPCPROP_SWORDIMAGE		= 10,
	NPCPROP_SHIELDIMAGE		= 11,
	NPCPROP_GANI			= 12,	// NPCPROP_BOWGIF in pre-2.x
	NPCPROP_VISFLAGS		= 13,
	NPCPROP_BLOCKFLAGS		= 14,
	NPCPROP_MESSAGE			= 15,
	NPCPROP_HURTDXDY		= 16,
	NPCPROP_ID				= 17,
	NPCPROP_SPRITE			= 18,
	NPCPROP_COLORS			= 19,
	NPCPROP_NICKNAME		= 20,
	NPCPROP_HORSEIMAGE		= 21,
	NPCPROP_HEADIMAGE		= 22,
	NPCPROP_SAVE0			= 23,
	NPCPROP_SAVE1			= 24,
	NPCPROP_SAVE2			= 25,
	NPCPROP_SAVE3			= 26,
	NPCPROP_SAVE4			= 27,
	NPCPROP_SAVE5			= 28,
	NPCPROP_SAVE6			= 29,
	NPCPROP_SAVE7			= 30,
	NPCPROP_SAVE8			= 31,
	NPCPROP_SAVE9			= 32,
	NPCPROP_ALIGNMENT		= 33,
	NPCPROP_IMAGEPART		= 34,
	NPCPROP_BODYIMAGE		= 35,
	NPCPROP_GATTRIB1		= 36,
	NPCPROP_GATTRIB2		= 37,
	NPCPROP_GATTRIB3		= 38,
	NPCPROP_GATTRIB4		= 39,
	NPCPROP_GATTRIB5		= 40,
	NPCPROP_GMAPLEVELX		= 41,
	NPCPROP_GMAPLEVELY		= 42,

	NPCPROP_UNKNOWN43		= 43,
	
	NPCPROP_GATTRIB6		= 44,
	NPCPROP_GATTRIB7		= 45,
	NPCPROP_GATTRIB8		= 46,
	NPCPROP_GATTRIB9		= 47,

	NPCPROP_UNKNOWN48		= 48,
	NPCPROP_UNKNOWN49		= 49,
	NPCPROP_UNKNOWN50		= 50,
	NPCPROP_UNKNOWN51		= 51,
	NPCPROP_UNKNOWN52		= 52,

	NPCPROP_GATTRIB10		= 53,
	NPCPROP_GATTRIB11		= 54,
	NPCPROP_GATTRIB12		= 55,
	NPCPROP_GATTRIB13		= 56,
	NPCPROP_GATTRIB14		= 57,
	NPCPROP_GATTRIB15		= 58,
	NPCPROP_GATTRIB16		= 59,
	NPCPROP_GATTRIB17		= 60,
	NPCPROP_GATTRIB18		= 61,
	NPCPROP_GATTRIB19		= 62,
	NPCPROP_GATTRIB20		= 63,
	NPCPROP_GATTRIB21		= 64,
	NPCPROP_GATTRIB22		= 65,
	NPCPROP_GATTRIB23		= 66,
	NPCPROP_GATTRIB24		= 67,
	NPCPROP_GATTRIB25		= 68,
	NPCPROP_GATTRIB26		= 69,
	NPCPROP_GATTRIB27		= 70,
	NPCPROP_GATTRIB28		= 71,
	NPCPROP_GATTRIB29		= 72,
	NPCPROP_GATTRIB30		= 73,

	NPCPROP_CLASS			= 74,	// NPC-Server class.  Possibly also join scripts.
	NPCPROP_X2				= 75,
	NPCPROP_Y2				= 76,

	NPCPROP_COUNT
};

//! NPCPROP_VISFLAGS values.
enum
{
	NPCVISFLAG_VISIBLE			= 0x01,
	NPCVISFLAG_DRAWOVERPLAYER	= 0x02,
	NPCVISFLAG_DRAWUNDERPLAYER	= 0x04,
};

//! NPCPROP_BLOCKFLAGS values.
enum
{
	NPCBLOCKFLAG_BLOCK		= 0x00,
	NPCBLOCKFLAG_NOBLOCK	= 0x01,
};

class TServer;
class TLevel;
class TNPC
{
	public:
		TNPC(const CString& pImage, const CString& pScript, float pX, float pY, TServer* pServer, TLevel* pLevel, bool pLevelNPC = true, bool trimCode = false);
		~TNPC();

		// prop functions
		CString getProp(unsigned char pId, int clientVersion = CLVER_2_17) const;
		CString getProps(time_t newTime, int clientVersion = CLVER_2_17) const;
		CString setProps(CString& pProps, int clientVersion = CLVER_2_17);

		// set functions
		void setId(unsigned int pId)	{ id = pId; }
		void setLevel(TLevel* pLevel)	{ level = pLevel; }
		void setX(float val)			{ x = val; }
		void setY(float val)			{ y = val; }

		// get functions
		unsigned int getId() const		{ return id; }
		TLevel* getLevel()				{ return level; }
		float getX() const				{ return x; }
		float getY() const				{ return y; }
		CString getImage() const		{ return image; }
		CString getWeaponName() const	{ return weaponName; }
		CString getServerScript() const	{ return serverScript; }
		CString getClientScript() const	{ return clientScript; }
		time_t getPropModTime(unsigned char pId);

		bool isLevelNPC()				{ return levelNPC; }

	private:
		bool blockPositionUpdates;
		bool levelNPC;
		time_t modTime[NPCPROP_COUNT];
		float x, y, hurtX, hurtY;
		int x2, y2;
		unsigned char gmaplevelx, gmaplevely;
		unsigned int id;
		int rupees;
		unsigned char darts, bombs, glovePower, bombPower, swordPower, shieldPower;
		unsigned char visFlags, blockFlags, sprite, colors[5], power, ap;
		CString gAttribs[30];
		CString image, swordImage, shieldImage, headImage, bodyImage, horseImage, bowImage, gani;
		CString nickName, imagePart, chatMsg, weaponName;
		CString serverScript, clientScript;
		CString serverScriptFormatted, clientScriptFormatted;
		unsigned char saves[10];
		TLevel* level;
		TServer* server;
	
		// npc-server
		int timeout;
};

inline
time_t TNPC::getPropModTime(unsigned char pId)
{
	if (pId < NPCPROP_COUNT) return modTime[pId];
	return 0;
}

#endif
