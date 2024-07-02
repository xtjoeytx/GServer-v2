#ifndef TNPC_H
#define TNPC_H

#include <algorithm>
#include <ctime>
#include <memory>

#include <CString.h>
#include <IUtil.h>
#include "BabyDI.h"

#include "scripting/SourceCode.h"

#ifdef V8NPCSERVER
	#include <queue>
	#include <unordered_map>
	#include <unordered_set>

    #include "scripting/ScriptAction.h"
	#include "scripting/ScriptExecutionContext.h"
	#include "scripting/interface/ScriptBindings.h"
#endif

enum
{
	NPCPROP_IMAGE = 0,
	NPCPROP_SCRIPT = 1,
	NPCPROP_X = 2,
	NPCPROP_Y = 3,
	NPCPROP_POWER = 4,
	NPCPROP_RUPEES = 5,
	NPCPROP_ARROWS = 6,
	NPCPROP_BOMBS = 7,
	NPCPROP_GLOVEPOWER = 8,
	NPCPROP_BOMBPOWER = 9,
	NPCPROP_SWORDIMAGE = 10,
	NPCPROP_SHIELDIMAGE = 11,
	NPCPROP_GANI = 12, // NPCPROP_BOWGIF in pre-2.x
	NPCPROP_VISFLAGS = 13,
	NPCPROP_BLOCKFLAGS = 14,
	NPCPROP_MESSAGE = 15,
	NPCPROP_HURTDXDY = 16,
	NPCPROP_ID = 17,
	NPCPROP_SPRITE = 18,
	NPCPROP_COLORS = 19,
	NPCPROP_NICKNAME = 20,
	NPCPROP_HORSEIMAGE = 21,
	NPCPROP_HEADIMAGE = 22,
	NPCPROP_SAVE0 = 23,
	NPCPROP_SAVE1 = 24,
	NPCPROP_SAVE2 = 25,
	NPCPROP_SAVE3 = 26,
	NPCPROP_SAVE4 = 27,
	NPCPROP_SAVE5 = 28,
	NPCPROP_SAVE6 = 29,
	NPCPROP_SAVE7 = 30,
	NPCPROP_SAVE8 = 31,
	NPCPROP_SAVE9 = 32,
	NPCPROP_ALIGNMENT = 33,
	NPCPROP_IMAGEPART = 34,
	NPCPROP_BODYIMAGE = 35,
	NPCPROP_GATTRIB1 = 36,
	NPCPROP_GATTRIB2 = 37,
	NPCPROP_GATTRIB3 = 38,
	NPCPROP_GATTRIB4 = 39,
	NPCPROP_GATTRIB5 = 40,
	NPCPROP_GMAPLEVELX = 41,
	NPCPROP_GMAPLEVELY = 42,

	NPCPROP_UNKNOWN43 = 43,

	NPCPROP_GATTRIB6 = 44,
	NPCPROP_GATTRIB7 = 45,
	NPCPROP_GATTRIB8 = 46,
	NPCPROP_GATTRIB9 = 47,

	NPCPROP_UNKNOWN48 = 48,
	NPCPROP_SCRIPTER = 49, // My guess is UNKNOWN48 or this is the scripter's name
	NPCPROP_NAME = 50,
	NPCPROP_TYPE = 51,
	NPCPROP_CURLEVEL = 52,

	NPCPROP_GATTRIB10 = 53,
	NPCPROP_GATTRIB11 = 54,
	NPCPROP_GATTRIB12 = 55,
	NPCPROP_GATTRIB13 = 56,
	NPCPROP_GATTRIB14 = 57,
	NPCPROP_GATTRIB15 = 58,
	NPCPROP_GATTRIB16 = 59,
	NPCPROP_GATTRIB17 = 60,
	NPCPROP_GATTRIB18 = 61,
	NPCPROP_GATTRIB19 = 62,
	NPCPROP_GATTRIB20 = 63,
	NPCPROP_GATTRIB21 = 64,
	NPCPROP_GATTRIB22 = 65,
	NPCPROP_GATTRIB23 = 66,
	NPCPROP_GATTRIB24 = 67,
	NPCPROP_GATTRIB25 = 68,
	NPCPROP_GATTRIB26 = 69,
	NPCPROP_GATTRIB27 = 70,
	NPCPROP_GATTRIB28 = 71,
	NPCPROP_GATTRIB29 = 72,
	NPCPROP_GATTRIB30 = 73,

	NPCPROP_CLASS = 74, // NPC-Server class.  Possibly also join scripts.
	NPCPROP_X2 = 75,
	NPCPROP_Y2 = 76,

	NPCPROP_COUNT
};

//! NPCPROP_VISFLAGS values.
enum
{
	NPCVISFLAG_VISIBLE = 0x01,
	NPCVISFLAG_DRAWOVERPLAYER = 0x02,
	NPCVISFLAG_DRAWUNDERPLAYER = 0x04,
};

//! NPCPROP_BLOCKFLAGS values.
enum
{
	NPCBLOCKFLAG_BLOCK = 0x00,
	NPCBLOCKFLAG_NOBLOCK = 0x01,
};

//! NPCMOVE_FLAGS values
enum
{
	NPCMOVEFLAG_NOCACHE = 0x00,
	NPCMOVEFLAG_CACHE = 0x01,
	NPCMOVEFLAG_APPEND = 0x02,
	NPCMOVEFLAG_BLOCKCHECK = 0x04,
	NPCMOVEFLAG_EVENTWHENDONE = 0x08,
	NPCMOVEFLAG_APPLYDIR = 0x10
};

enum class NPCType
{
	LEVELNPC, // npcs found in a level
	PUTNPC,   // npcs created via script (putnpc)
	DBNPC     // npcs created in RC (Database-NPCs)
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
	NPCEVENTFLAG_CREATED = (int)(1 << 0),
	NPCEVENTFLAG_TIMEOUT = (int)(1 << 1),
	NPCEVENTFLAG_PLAYERCHATS = (int)(1 << 2),
	NPCEVENTFLAG_PLAYERENTERS = (int)(1 << 3),
	NPCEVENTFLAG_PLAYERLEAVES = (int)(1 << 4),
	NPCEVENTFLAG_PLAYERTOUCHSME = (int)(1 << 5),
	NPCEVENTFLAG_PLAYERLOGIN = (int)(1 << 6),
	NPCEVENTFLAG_PLAYERLOGOUT = (int)(1 << 7),
	NPCEVENTFLAG_NPCWARPED = (int)(1 << 8),
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
	NPC(NPCType type);
	NPC(const CString& pImage, std::string pScript, float pX, float pY, std::shared_ptr<Level> pLevel, NPCType type);
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
	void setId(unsigned int pId) { m_id = pId; }
	void setLevel(std::shared_ptr<Level> pLevel) { m_curlevel = pLevel; }
	void setX(int val) { m_x = val; }
	void setY(int val) { m_y = val; }
	void setHeight(int val) { m_height = val; }
	void setWidth(int val) { m_width = val; }
	void setName(const std::string& name) { m_npcName = name; }
	void setScripter(const CString& name) { m_npcScripter = name; }
	void setScriptType(const CString& type) { m_npcScriptType = type; }
	void setBlockingFlags(int val) { m_blockFlags = val; }
	void setVisibleFlags(int val) { m_visFlags = val; }
	void setColorId(unsigned int idx, unsigned char val);
	void setSprite(int val) { m_sprite = val; }

	// get functions
	unsigned int getId() const { return m_id; }
	NPCType getType() const { return m_npcType; }
	int getX() const { return m_x; }
	int getY() const { return m_y; }
	int getHeight() const { return m_height; }
	int getWidth() const { return m_width; }
	unsigned char getSprite() const { return m_sprite; }
	int getBlockFlags() const { return m_blockFlags; }
	int getVisibleFlags() const { return m_visFlags; }
	int getTimeout() const { return m_timeout; }

	const SourceCode& getSource() const { return m_npcScript; }
	const std::string& getName() const { return m_npcName; }
	const CString& getScriptType() const { return m_npcScriptType; }
	const CString& getScripter() const { return m_npcScripter; }
	const CString& getWeaponName() const { return m_weaponName; }
	std::shared_ptr<Level> getLevel() const;
	time_t getPropModTime(unsigned char pId);
	unsigned char getColorId(unsigned int idx) const;

	const CString& getByteCode() const
	{
		return m_npcBytecode;
	}

#ifdef V8NPCSERVER
	bool getIsNpcDeleteRequested() const { return m_npcDeleteRequested; }

	bool joinedClass(const std::string& name)
	{
		auto it = m_classMap.find(name); // std::find(m_classMap.begin(), m_classMap.end(), name);
		return (it != m_classMap.end());
	}

	ScriptClass* joinClass(const std::string& className);
	void setTimeout(int val);
	void updatePropModTime(unsigned char propId);

	//
	bool hasScriptEvent(int flag) const;
	void setScriptEvents(int mask);

	ScriptExecutionContext& getExecutionContext();
	IScriptObject<NPC>* getScriptObject() const;
	void setScriptObject(std::unique_ptr<IScriptObject<NPC>> object);

	// -- flags
	CString getFlag(const std::string& pFlagName) const;
	void setFlag(const std::string& pFlagName, const CString& pFlagValue);
	void deleteFlag(const std::string& pFlagName);
	std::unordered_map<std::string, CString>& getFlagList() { return m_flagList; }

	bool deleteNPC();
	void reloadNPC();
	void resetNPC();

	bool isWarpable() const;
	void allowNpcWarping(NPCWarpType m_canWarp);
	void moveNPC(int dx, int dy, double time, int options);
	void warpNPC(std::shared_ptr<Level> pLevel, int pX, int pY);

	// file
	bool loadNPC(const CString& fileName);
	void saveNPC();

	void queueNpcAction(const std::string& action, Player* player = nullptr, bool registerAction = true);
	void queueNpcTrigger(const std::string& action, Player* player = nullptr, const std::string& data = "");

	template<class... Args>
	void queueNpcEvent(const std::string& action, bool registerAction, Args&&... An);

	void registerNpcUpdates();
	void registerTriggerAction(const std::string& action, IScriptFunction* cbFunc);
	void scheduleEvent(unsigned int timeout, ScriptAction& action);

	bool runScriptTimer();
	NPCEventResponse runScriptEvents();

	CString getVariableDump();

#endif

private:
	BabyDI_INJECT(Server, m_server);

	NPCType m_npcType;
	SourceCode m_npcScript;

	bool m_blockPositionUpdates = false;
	time_t m_modTime[NPCPROP_COUNT];
	float m_hurtX = 32.0f;
	float m_hurtY = 32.0f;
	int m_x = static_cast<int>(30 * 16);
	int m_y = static_cast<int>(30.5 * 16);
	unsigned int m_id = 0;
	int m_rupees = 0;
	unsigned char m_darts = 0;
	unsigned char m_bombs = 0;
	unsigned char m_glovePower = 1;
	unsigned char m_bombPower = 1;
	unsigned char m_swordPower = 1;
	unsigned char m_shieldPower = 1;
	unsigned char m_visFlags = 1;
	unsigned char m_blockFlags = 0;
	unsigned char m_sprite = 2;
	unsigned char m_colors[5] = { 2, 0, 10, 4, 18 };
	unsigned char m_hitpoints = 0;
	unsigned char m_ap = 50;
	CString m_ganiAttribs[30];
	CString m_swordImage, m_shieldImage, m_headImage, m_bodyImage, m_horseImage, m_bowImage;
	CString m_imagePart, m_weaponName;
	unsigned char m_saves[10];
	std::weak_ptr<Level> m_curlevel;

	std::string m_chatMessage, m_gani, m_image;
	std::string m_nickName;

	CString m_npcScripter, m_npcScriptType;
	std::string m_npcName;
	std::string m_clientScriptFormatted;
	int m_timeout = 0;
	int m_width = 32;
	int m_height = 32;

	CString m_npcBytecode;

#ifdef V8NPCSERVER
	bool hasTimerUpdates() const;
	void freeScriptResources();
	void testTouch();
	void testForLinks();
	void updateClientCode();

	std::map<std::string, std::string> m_classMap;
	std::unordered_set<unsigned char> m_propModified;
	std::vector<std::string> m_joinedClasses;

	// Defaults
	CString m_origImage, m_origLevel;
	int m_origX, m_origY;

	// npc-server
	NPCWarpType m_canWarp = NPCWarpType::None;
	bool m_npcDeleteRequested = false;
	std::unordered_map<std::string, CString> m_flagList;

	unsigned int m_scriptEventsMask = 0xFF;
	std::unique_ptr<IScriptObject<NPC>> m_scriptObject;
	ScriptExecutionContext m_scriptExecutionContext;
	std::unordered_map<std::string, IScriptFunction*> m_triggerActions;
	std::vector<ScriptEventTimer> m_scriptTimers;
#endif
};

using NPCPtr = std::shared_ptr<NPC>;
using NPCWeakPtr = std::weak_ptr<NPC>;

inline time_t NPC::getPropModTime(unsigned char pId)
{
	if (pId < NPCPROP_COUNT) return m_modTime[pId];
	return 0;
}

inline void NPC::setPropModTime(unsigned char pId, time_t time)
{
	if (pId < NPCPROP_COUNT)
		m_modTime[pId] = time;
}

inline unsigned char NPC::getColorId(unsigned int idx) const
{
	if (idx < 5) return m_colors[idx];
	return 0;
}

inline void NPC::setColorId(unsigned int idx, unsigned char val)
{
	if (idx < 5) m_colors[idx] = val;
}

inline unsigned char NPC::getSave(unsigned int idx) const
{
	if (idx < 10) return m_saves[idx];
	return 0;
}

inline void NPC::setSave(unsigned int idx, unsigned char val)
{
	if (idx < 10) m_saves[idx] = val;
}

//////////

inline const std::string& NPC::getChat() const
{
	return m_chatMessage;
}

inline void NPC::setChat(const std::string& msg)
{
	m_chatMessage = msg.substr(0, std::min<size_t>(msg.length(), 223));
}

//////////

inline const std::string& NPC::getGani() const
{
	return m_gani;
}

inline void NPC::setGani(const std::string& gani)
{
	this->m_gani = gani.substr(0, std::min<size_t>(gani.length(), 223));
}

//////////

inline int NPC::getRupees() const
{
	return m_rupees;
}

inline void NPC::setRupees(int val)
{
	m_rupees = val;
}

//////////

inline int NPC::getDarts() const
{
	return m_darts;
}

inline void NPC::setDarts(int val)
{
	setProps(CString() >> (char)NPCPROP_ARROWS >> (char)clip(val, 0, 99), CLVER_2_17, true);
}

/////////

inline const std::string& NPC::getImage() const
{
	return m_image;
}

inline void NPC::setImage(const std::string& pImage)
{
	m_image = pImage.substr(0, std::min<size_t>(pImage.length(), 223));
}

inline void NPC::setImage(const std::string& pImage, int offsetx, int offsety, int pwidth, int pheight)
{
	setImage(pImage);
	m_imagePart.clear();
	m_imagePart.writeGShort(offsetx);
	m_imagePart.writeGShort(offsety);
	m_imagePart.writeGChar(pwidth);
	m_imagePart.writeGChar(pheight);
}

//////////

inline const std::string& NPC::getNickname() const
{
	return m_nickName;
}

inline void NPC::setNickname(const std::string& pNick)
{
	m_nickName = pNick.substr(0, std::min<size_t>(pNick.length(), 223));
}

//////////

inline const CString& NPC::getBodyImage() const
{
	return m_bodyImage;
}

inline void NPC::setBodyImage(const std::string& pBodyImage)
{
	m_bodyImage = pBodyImage.substr(0, 200);
}

//////////

inline const CString& NPC::getHeadImage() const
{
	return m_headImage;
}

inline void NPC::setHeadImage(const std::string& pHeadImage)
{
	m_headImage = pHeadImage.substr(0, 123);
}

//////////

inline const CString& NPC::getHorseImage() const
{
	return m_horseImage;
}

inline void NPC::setHorseImage(const std::string& pHorseImage)
{
	m_horseImage = pHorseImage.substr(0, 200);
}

//////////

inline const CString& NPC::getShieldImage() const
{
	return m_shieldImage;
}

inline void NPC::setShieldImage(const std::string& pShieldImage)
{
	m_shieldImage = pShieldImage.substr(0, 200);
}

//////////

inline const CString& NPC::getSwordImage() const
{
	return m_swordImage;
}

inline void NPC::setSwordImage(const std::string& pSwordImage)
{
	m_swordImage = pSwordImage.substr(0, 120);
}

#ifdef V8NPCSERVER

inline void NPC::updatePropModTime(unsigned char propId)
{
	if (propId < NPCPROP_COUNT)
	{
		m_propModified.insert(propId);
		registerNpcUpdates();
	}
}

inline bool NPC::isWarpable() const
{
	return m_canWarp != NPCWarpType::None;
}

inline void NPC::allowNpcWarping(NPCWarpType m_canWarp)
{
	if (m_npcType != NPCType::LEVELNPC)
		this->m_canWarp = m_canWarp;
}

/**
 * Script Engine
 */
inline bool NPC::hasTimerUpdates() const
{
	return (m_timeout > 0 || !m_scriptTimers.empty());
}

inline bool NPC::hasScriptEvent(int flag) const
{
	return ((m_scriptEventsMask & flag) == flag);
}

inline void NPC::setScriptEvents(int mask)
{
	m_scriptEventsMask = mask;
}

inline ScriptExecutionContext& NPC::getExecutionContext()
{
	return m_scriptExecutionContext;
}

inline IScriptObject<NPC>* NPC::getScriptObject() const
{
	return m_scriptObject.get();
}

inline void NPC::setScriptObject(std::unique_ptr<IScriptObject<NPC>> object)
{
	m_scriptObject = std::move(object);
}

inline CString NPC::getFlag(const std::string& pFlagName) const
{
	auto it = m_flagList.find(pFlagName);
	if (it != m_flagList.end())
		return it->second;
	return "";
}

inline void NPC::setFlag(const std::string& pFlagName, const CString& pFlagValue)
{
	m_flagList[pFlagName] = pFlagValue;
}

inline void NPC::deleteFlag(const std::string& pFlagName)
{
	m_flagList.erase(pFlagName);
}

	// TODO(joey): hm
	#include "Server.h"

template<class... Args>
inline void NPC::queueNpcEvent(const std::string& action, bool registerAction, Args&&... An)
{
	ScriptEngine* scriptEngine = m_server->getScriptEngine();
	ScriptAction scriptAction = scriptEngine->createAction(action, getScriptObject(), std::forward<Args>(An)...);

	m_scriptExecutionContext.addAction(scriptAction);
	if (registerAction)
		scriptEngine->registerNpcUpdate(this);
}

inline void NPC::registerNpcUpdates()
{
	ScriptEngine* scriptEngine = m_server->getScriptEngine();
	scriptEngine->registerNpcUpdate(this);
}

inline void NPC::scheduleEvent(unsigned int timeout, ScriptAction& action)
{
	m_scriptTimers.push_back({ timeout, std::move(action) });
}

#endif

#endif
