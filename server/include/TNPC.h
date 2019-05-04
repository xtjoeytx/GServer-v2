#ifndef TNPC_H
#define TNPC_H

#include <time.h>
#include "CString.h"
#include "IUtil.h"

#ifdef V8NPCSERVER
#include <unordered_map>
#include "ScriptWrapped.h"

class ScriptAction;

#endif

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

#ifdef V8NPCSERVER
//! NPC Event Flags
enum
{
	NPCEVENTFLAG_CREATED		= (int)(1 << 0),
	NPCEVENTFLAG_TIMEOUT		= (int)(1 << 1),
	NPCEVENTFLAG_PLAYERCHATS	= (int)(1 << 2),
	NPCEVENTFLAG_PLAYERENTERS	= (int)(1 << 3),
	NPCEVENTFLAG_PLAYERLEAVES	= (int)(1 << 4),
	NPCEVENTFLAG_PLAYERTOUCHSME	= (int)(1 << 5),
};
#endif

class TServer;
class TLevel;
class TPlayer;
class TNPC
{
	public:
		TNPC(const CString& pImage, const CString& pScript, float pX, float pY, TServer* pServer, TLevel* pLevel, bool pLevelNPC = true, bool trimCode = false);
		~TNPC();

		void setScriptCode(const CString& pScript, bool trimCode = false);

		// prop functions
		CString getProp(unsigned char pId, int clientVersion = CLVER_2_17) const;
		CString getProps(time_t newTime, int clientVersion = CLVER_2_17) const;
		CString setProps(CString& pProps, int clientVersion = CLVER_2_17, bool pForward = false);

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
#ifdef V8NPCSERVER
		inline int getPixelX() const	{ return x2; }
		inline int getPixelY() const	{ return y2; }
#endif
		CString getImage() const		{ return image; }
		CString getWeaponName() const	{ return weaponName; }
		CString getServerScript() const	{ return serverScript; }
		CString getClientScript() const	{ return clientScript; }
		time_t getPropModTime(unsigned char pId);

		bool isLevelNPC()				{ return levelNPC; }

#ifdef V8NPCSERVER
		inline int getBlockFlags() const;
		inline int getVisFlags() const;

		//
		inline int getWidth() const;
		inline void setWidth(int newWidth);

		//
		inline int getHeight() const;
		inline void setHeight(int newHeight);

		//
		inline int getTimeout() const;
		void setTimeout(int newTimeout);

		// 
		inline bool hasScriptEvent(int flag) const;
		inline void setScriptEvents(int mask);

		inline IScriptWrapped<TNPC> * getScriptObject() const;
		inline void setScriptObject(IScriptWrapped<TNPC> *object);

		// -- triggeractions
		void registerTriggerAction(const std::string& action, IScriptFunction *cbFunc);
		void queueNpcTrigger(const std::string& action, const std::string& data);

		//
		void freeScriptResources();
		void queueNpcAction(const std::string& action, TPlayer *player = 0, bool registerAction = true);
		bool runScriptTimer();
		void runScriptEvents();

		template<class... Args>
		inline void queueNpcEvent(const std::string& action, bool registerAction, Args&&... An);
#endif
	
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
	
#ifdef V8NPCSERVER
		// npc-server
		int width, height;
		int timeout;

		int _scriptEventsMask;
		IScriptWrapped<TNPC> *_scriptObject;
		std::vector<ScriptAction *> _actions;
		std::unordered_map<std::string, IScriptFunction *> _triggerActions;
#endif
};

inline
time_t TNPC::getPropModTime(unsigned char pId)
{
	if (pId < NPCPROP_COUNT) return modTime[pId];
	return 0;
}

#ifdef V8NPCSERVER

/**
 * NPC Properties Getter/Setters
 */
inline int TNPC::getBlockFlags() const {
	return blockFlags;
}

inline int TNPC::getVisFlags() const {
	return visFlags;
}

//
inline int TNPC::getWidth() const {
	return width;
}

inline void TNPC::setWidth(int newWidth) {
	width = newWidth;
}

//
inline int TNPC::getHeight() const {
	return height;
}

inline void TNPC::setHeight(int newHeight) {
	height = newHeight;
}

inline int TNPC::getTimeout() const {
	return timeout;
}

/**
 * Script Engine 
 */
inline bool TNPC::hasScriptEvent(int flag) const {
	return ((_scriptEventsMask & flag) == flag);
}

inline void TNPC::setScriptEvents(int mask) {
	_scriptEventsMask = mask;
}

inline IScriptWrapped<TNPC> * TNPC::getScriptObject() const {
	return _scriptObject;
}

inline void TNPC::setScriptObject(IScriptWrapped<TNPC> *object) {
	_scriptObject = object;
}

template<class... Args>
inline void TNPC::queueNpcEvent(const std::string& action, bool registerAction, Args&&... An)
{
	CScriptEngine *scriptEngine = server->getScriptEngine();
	ScriptAction *scriptAction = scriptEngine->CreateAction(action, _scriptObject, std::forward<Args>(An)...);

	_actions.push_back(scriptAction);
	if (registerAction)
		scriptEngine->RegisterNpcUpdate(this);
}
#endif

#endif
