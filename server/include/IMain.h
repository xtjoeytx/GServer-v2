#ifndef GONSTRUCT_IMAIN_H
#define GONSTRUCT_IMAIN_H

#include "TLevel.h"
#include "CSettings.h"
class TPlayer;
class TMap;
class TNPC;
class CFileSystem;
class CString;
#include <vector>
#include <memory>

class IMain {
	public:
		virtual TLevel*										getLevel(const CString& pLevel)						= 0;
		virtual std::vector<TLevel *>*						getLevelList()										= 0;
		virtual const std::vector<std::unique_ptr<TMap>>&	getMapList() const									= 0;
		virtual TNPC*										getNPC(unsigned int id) const						= 0;
		virtual TNPC* addNPC(const CString& pImage, const CString& pScript, float pX, float pY, TLevel* pLevel, bool pLevelNPC, bool sendToPlayers = false)	= 0;
		virtual TNPC* addNPC(TNPC* npc, TLevel* pLevel, bool pLevelNPC, bool sendToPlayers = false)				= 0;
		virtual bool										deleteNPC(unsigned int id, bool eraseFromLevel = true)		= 0;
		virtual bool										deleteNPC(TNPC* npc, bool eraseFromLevel = true)	= 0;
		virtual std::vector<TPlayer *>*						getPlayerList()										= 0;
		virtual CSettings*									getSettings()										= 0;
		virtual CFileSystem*								getFileSystem(int c = 0)							= 0;
		virtual CString										getServerPath()										= 0;
		virtual void sendPacketToLevel(CString pPacket, TMap* pMap, TLevel* pLevel, TPlayer* pPlayer = 0, bool onlyGmap = false) const = 0;

};

#endif //GONSTRUCT_IMAIN_H
