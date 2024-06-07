#ifndef TNPC_H
#define TNPC_H

#include <algorithm>
#include <memory>
#include <ctime>
#include "CString.h"
#include "IUtil.h"
#include "SourceCode.h"

#ifdef V8NPCSERVER
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include "ScriptAction.h"
#include "ScriptBindings.h"
#include "ScriptExecutionContext.h"
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
	NPCPROP_SCRIPTER		= 49, // My guess is UNKNOWN48 or this is the scripter's name
	NPCPROP_NAME			= 50,
	NPCPROP_TYPE			= 51,
	NPCPROP_CURLEVEL		= 52,

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

//! NPCMOVE_FLAGS values
enum
{
	NPCMOVEFLAG_NOCACHE			= 0x00,
	NPCMOVEFLAG_CACHE			= 0x01,
	NPCMOVEFLAG_APPEND			= 0x02,
	NPCMOVEFLAG_BLOCKCHECK		= 0x04,
	NPCMOVEFLAG_EVENTWHENDONE 	= 0x08,
	NPCMOVEFLAG_APPLYDIR		= 0x10
};

enum class NPCType
{
	LEVELNPC,	// npcs found in a level
	PUTNPC,		// npcs created via script (putnpc)
	DBNPC		// npcs created in RC (Database-NPCs)
};

#ifdef V8NPCSERVER

enum class NPCEventResponse
{
	NoEvents,
	PendingEvents,
	Delete
};

enum class NPCWarpType
{
	None,
	AllLinks,
	OverworldLinks
};

//! NPC Event Flags
enum
{
	NPCEVENTFLAG_CREATED		= (int)(1 << 0),
	NPCEVENTFLAG_TIMEOUT		= (int)(1 << 1),
	NPCEVENTFLAG_PLAYERCHATS	= (int)(1 << 2),
	NPCEVENTFLAG_PLAYERENTERS	= (int)(1 << 3),
	NPCEVENTFLAG_PLAYERLEAVES	= (int)(1 << 4),
	NPCEVENTFLAG_PLAYERTOUCHSME	= (int)(1 << 5),
	NPCEVENTFLAG_PLAYERLOGIN	= (int)(1 << 6),
	NPCEVENTFLAG_PLAYERLOGOUT	= (int)(1 << 7),
	NPCEVENTFLAG_NPCWARPED		= (int)(1 << 8),
};

struct ScriptEventTimer
{
	unsigned int timer;
	ScriptAction action;
};

#endif

class Server;
class Level;
class Player;
class ScriptClass;
class NPC
{
	public:
		NPC(Server* pServer, NPCType type);
		NPC(const CString& pImage, std::string pScript, float pX, float pY, Server* pServer, std::shared_ptr<Level> pLevel, NPCType type);
		~NPC();

		void setScriptCode(std::string pScript);

		// prop functions
		CString getProp(unsigned char pId, int clientVersion = CLVER_2_17) const;
		CString getProps(time_t newTime, int clientVersion = CLVER_2_17) const;
		CString setProps(CString& pProps, int clientVersion = CLVER_2_17, bool pForward = false);
		void setPropModTime(unsigned char pid, time_t time);

		// NPCPROP functions begin

		const std::string& getChat() const;
		void setChat(const std::string& msg);

		const std::string& getGani() const;
		void setGani(const std::string& gani);

		const std::string& getImage() const;
		void setImage(const std::string& image);
		void setImage(const std::string& image, int offsetx, int offsety, int widt, int height);

		const std::string& getNickname() const;
		void setNickname(const std::string& nick);

		unsigned char getSave(unsigned int idx) const;
		void setSave(unsigned int idx, unsigned char val);

		int getRupees() const;
		void setRupees(int val);

		int getDarts() const;
		void setDarts(int val);

		const CString& getBodyImage() const;
		void setBodyImage(const std::string& pBodyImage);

		const CString& getHeadImage() const;
		void setHeadImage(const std::string& pHeadImage);

		const CString& getHorseImage() const;
		void setHorseImage(const std::string& pHeadImage);

		const CString& getShieldImage() const;
		void setShieldImage(const std::string& pShieldImage);

		const CString& getSwordImage() const;
		void setSwordImage(const std::string& pSwordImage);

		// NPCPROP functions end

		// set functions
		void setId(unsigned int pId)			{ id = pId; }
		void setLevel(std::shared_ptr<Level> pLevel) { curlevel = pLevel; }
		void setX(int val)						{ x = val; }
		void setY(int val)						{ y = val; }
		void setHeight(int val)					{ height = val; }
		void setWidth(int val)					{ width = val; }
		void setName(const std::string& name)	{ npcName = name; }
		void setScripter(const CString& name)	{ npcScripter = name; }
		void setScriptType(const CString& type)	{ npcScriptType = type; }
		void setBlockingFlags(int val)			{ blockFlags = val; }
		void setVisibleFlags(int val)			{ visFlags = val; }
		void setColorId(unsigned int idx, unsigned char val);
		void setSprite(int val)					{ sprite = val; }

		// get functions
		unsigned int getId() const				{ return id; }
		NPCType getType() const					{ return npcType; }
		int getX() const						{ return x; }
		int getY() const						{ return y; }
		int getHeight() const 					{ return height; }
		int getWidth() const 					{ return width; }
		unsigned char getSprite() const			{ return sprite; }
		int getBlockFlags() const 				{ return blockFlags; }
		int getVisibleFlags() const 			{ return visFlags; }
		int getTimeout() const 					{ return timeout; }

		const SourceCode& getSource() const		{ return npcScript; }
		const std::string& getName() const		{ return npcName; }
		const CString& getScriptType() const	{ return npcScriptType; }
		const CString& getScripter() const		{ return npcScripter; }
		const CString& getWeaponName() const	{ return weaponName; }
		std::shared_ptr<Level> getLevel() const;
		time_t getPropModTime(unsigned char pId);
		unsigned char getColorId(unsigned int idx) const;

		const CString& getByteCode() const {
			return npcBytecode;
		}

#ifdef V8NPCSERVER
		bool getIsNpcDeleteRequested() const	{ return npcDeleteRequested; }

		bool joinedClass(const std::string& name) {
			auto it = classMap.find(name); // std::find(classMap.begin(), classMap.end(), name);
			return (it != classMap.end());
		}

		ScriptClass * joinClass(const std::string& className);
		void setTimeout(int val);
		void updatePropModTime(unsigned char propId);

		//
		bool hasScriptEvent(int flag) const;
		void setScriptEvents(int mask);

		ScriptExecutionContext& getExecutionContext();
		IScriptObject<NPC> * getScriptObject() const;
		void setScriptObject(std::unique_ptr<IScriptObject<NPC>> object);

		// -- flags
		CString getFlag(const std::string& pFlagName) const;
		void setFlag(const std::string& pFlagName, const CString& pFlagValue);
		void deleteFlag(const std::string& pFlagName);
		std::unordered_map<std::string, CString>& getFlagList() { return flagList; }

		bool deleteNPC();
		void reloadNPC();
		void resetNPC();

		bool isWarpable() const;
		void allowNpcWarping(NPCWarpType canWarp);
		void moveNPC(int dx, int dy, double time, int options);
		void warpNPC(std::shared_ptr<Level> pLevel, int pX, int pY);

		// file
		bool loadNPC(const CString& fileName);
		void saveNPC();

		void queueNpcAction(const std::string& action, Player *player = nullptr, bool registerAction = true);
		void queueNpcTrigger(const std::string& action, Player *player = nullptr, const std::string& data = "");

		template<class... Args>
		void queueNpcEvent(const std::string& action, bool registerAction, Args&&... An);

		void registerNpcUpdates();
		void registerTriggerAction(const std::string& action, IScriptFunction *cbFunc);
		void scheduleEvent(unsigned int timeout, ScriptAction& action);

		bool runScriptTimer();
		NPCEventResponse runScriptEvents();

		CString getVariableDump();

#endif

	private:
		NPCType npcType;
		SourceCode npcScript;

		bool blockPositionUpdates;
		time_t modTime[NPCPROP_COUNT];
		float hurtX, hurtY;
		int x, y;
		unsigned int id;
		int rupees;
		unsigned char darts, bombs, glovePower, bombPower, swordPower, shieldPower;
		unsigned char visFlags, blockFlags, sprite, colors[5], power, ap;
		CString gAttribs[30];
		CString swordImage, shieldImage, headImage, bodyImage, horseImage, bowImage;
		CString imagePart, weaponName;
		unsigned char saves[10];
		std::weak_ptr<Level> curlevel;
		Server* server;

		std::string chatMsg, gani, image;
		std::string nickName;

		CString npcScripter, npcScriptType;
		std::string npcName;
		std::string clientScriptFormatted;
		int timeout;
		int width, height;

		CString npcBytecode;

#ifdef V8NPCSERVER
		bool hasTimerUpdates() const;
		void freeScriptResources();
		void testTouch();
		void testForLinks();
		void updateClientCode();

		std::map<std::string, std::string> classMap;
		std::unordered_set<unsigned char> propModified;
		std::vector<std::string> joinedClasses;

		// Defaults
		CString origImage, origLevel;
		int origX, origY;

		// npc-server
		NPCWarpType canWarp;
		bool npcDeleteRequested;
		std::unordered_map<std::string, CString> flagList;

		unsigned int _scriptEventsMask;
		std::unique_ptr<IScriptObject<NPC>> _scriptObject;
		ScriptExecutionContext _scriptExecutionContext;
		std::unordered_map<std::string, IScriptFunction *> _triggerActions;
		std::vector<ScriptEventTimer> _scriptTimers;
#endif
};

using TNPCPtr = std::shared_ptr<NPC>;
using TNPCWeakPtr = std::weak_ptr<NPC>;

inline
time_t NPC::getPropModTime(unsigned char pId)
{
	if (pId < NPCPROP_COUNT) return modTime[pId];
	return 0;
}

inline
void NPC::setPropModTime(unsigned char pId, time_t time)
{
	if (pId < NPCPROP_COUNT)
		modTime[pId] = time;
}

inline
unsigned char NPC::getColorId(unsigned int idx) const
{
	if (idx < 5) return colors[idx];
	return 0;
}

inline
void NPC::setColorId(unsigned int idx, unsigned char val)
{
	if (idx < 5) colors[idx] = val;
}

inline
unsigned char NPC::getSave(unsigned int idx) const
{
	if (idx < 10) return saves[idx];
	return 0;
}

inline
void NPC::setSave(unsigned int idx, unsigned char val)
{
	if (idx < 10) saves[idx] = val;
}

//////////

inline
const std::string& NPC::getChat() const
{
	return chatMsg;
}

inline
void NPC::setChat(const std::string& msg) {
	chatMsg = msg.substr(0, std::min<size_t>(msg.length(), 223));
}

//////////

inline
const std::string& NPC::getGani() const
{
	return gani;
}

inline
void NPC::setGani(const std::string& gani)
{
	this->gani = gani.substr(0, std::min<size_t>(gani.length(), 223));
}

//////////

inline
int NPC::getRupees() const
{
	return rupees;
}

inline
void NPC::setRupees(int val)
{
	rupees = val;
}

//////////

inline
int NPC::getDarts() const
{
	return darts;
}

inline
void NPC::setDarts(int val)
{
	setProps(CString() >> (char)NPCPROP_ARROWS >> (char)clip(val, 0, 99), CLVER_2_17, true);
}

/////////

inline
const std::string& NPC::getImage() const
{
	return image;
}

inline
void NPC::setImage(const std::string& pImage)
{
	image = pImage.substr(0, std::min<size_t>(pImage.length(), 223));
}

inline
void NPC::setImage(const std::string& pImage, int offsetx, int offsety, int pwidth, int pheight)
{
	setImage(pImage);
	imagePart.clear();
	imagePart.writeGShort(offsetx);
	imagePart.writeGShort(offsety);
	imagePart.writeGChar(pwidth);
	imagePart.writeGChar(pheight);
}

//////////

inline
const std::string& NPC::getNickname() const
{
	return nickName;
}

inline
void NPC::setNickname(const std::string& pNick)
{
	nickName = pNick.substr(0, std::min<size_t>(pNick.length(), 223));
}

//////////

inline
const CString& NPC::getBodyImage() const
{
	return bodyImage;
}

inline
void NPC::setBodyImage(const std::string& pBodyImage)
{
	bodyImage = pBodyImage.substr(0, 200);
}

//////////

inline
const CString& NPC::getHeadImage() const
{
	return headImage;
}

inline
void NPC::setHeadImage(const std::string& pHeadImage)
{
	headImage = pHeadImage.substr(0, 123);
}

//////////

inline
const CString& NPC::getHorseImage() const
{
	return horseImage;
}

inline
void NPC::setHorseImage(const std::string& pHorseImage)
{
	horseImage = pHorseImage.substr(0, 200);
}

//////////

inline
const CString& NPC::getShieldImage() const
{
	return shieldImage;
}

inline
void NPC::setShieldImage(const std::string& pShieldImage)
{
	shieldImage = pShieldImage.substr(0, 200);
}

//////////

inline
const CString& NPC::getSwordImage() const
{
	return swordImage;
}

inline
void NPC::setSwordImage(const std::string& pSwordImage)
{
	swordImage = pSwordImage.substr(0, 120);
}

#ifdef V8NPCSERVER

inline void NPC::updatePropModTime(unsigned char propId)
{
	if (propId < NPCPROP_COUNT)
	{
		propModified.insert(propId);
		registerNpcUpdates();
	}
}

inline bool NPC::isWarpable() const
{
	return canWarp != NPCWarpType::None;
}

inline void NPC::allowNpcWarping(NPCWarpType canWarp)
{
	if (npcType != NPCType::LEVELNPC)
		this->canWarp = canWarp;
}

/**
 * Script Engine
 */
inline bool NPC::hasTimerUpdates() const {
	return (timeout > 0 || !_scriptTimers.empty());
}

inline bool NPC::hasScriptEvent(int flag) const {
	return ((_scriptEventsMask & flag) == flag);
}

inline void NPC::setScriptEvents(int mask) {
	_scriptEventsMask = mask;
}

inline ScriptExecutionContext& NPC::getExecutionContext() {
	return _scriptExecutionContext;
}

inline IScriptObject<NPC> * NPC::getScriptObject() const {
	return _scriptObject.get();
}

inline void NPC::setScriptObject(std::unique_ptr<IScriptObject<NPC>> object) {
	_scriptObject = std::move(object);
}

inline CString NPC::getFlag(const std::string& pFlagName) const
{
	auto it = flagList.find(pFlagName);
	if (it != flagList.end())
		return it->second;
	return "";
}

inline void NPC::setFlag(const std::string & pFlagName, const CString & pFlagValue)
{
	flagList[pFlagName] = pFlagValue;
}

inline void NPC::deleteFlag(const std::string& pFlagName)
{
	flagList.erase(pFlagName);
}

// TODO(joey): hm
#include "Server.h"

template<class... Args>
inline void NPC::queueNpcEvent(const std::string& action, bool registerAction, Args&&... An)
{
	CScriptEngine *scriptEngine = server->getScriptEngine();
	ScriptAction scriptAction = scriptEngine->CreateAction(action, getScriptObject(), std::forward<Args>(An)...);

	_scriptExecutionContext.addAction(scriptAction);
	if (registerAction)
		scriptEngine->RegisterNpcUpdate(this);
}

inline void NPC::registerNpcUpdates()
{
	CScriptEngine *scriptEngine = server->getScriptEngine();
	scriptEngine->RegisterNpcUpdate(this);
}

inline void NPC::scheduleEvent(unsigned int timeout, ScriptAction& action) {
	_scriptTimers.push_back({ timeout, std::move(action) });
}

#endif

#endif
