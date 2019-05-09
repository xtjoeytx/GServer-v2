#ifndef TLEVEL_H
#define TLEVEL_H

#include <vector>
#include <map>
#include "IUtil.h"
#include "CString.h"
#include "TLevelBaddy.h"
#include "TLevelBoardChange.h"
#include "TLevelChest.h"
#include "TLevelHorse.h"
#include "TLevelItem.h"
#include "TLevelLink.h"
#include "TLevelSign.h"

class TServer;
class TPlayer;
class TNPC;
class TMap;

class TLevel
{
	public:
		//! Destructor.
		~TLevel();

		//! Finds a level with the specified level name and returns it.  If not found, it tries to load it from the disk.
		//! \param pLevelName The name of the level to search for.
		//! \param server The server the level belongs to.
		//! \return A pointer to the level found.
		static TLevel* findLevel(const CString& pLevelName, TServer* server);

		//! Re-loads the level.
		//! \return True if it succeeds in re-loading the level.
		bool reload();

		//! Returns a clone of the level.
		TLevel* clone();
		
		// get crafted packets
		CString getBaddyPacket(int clientVersion = CLVER_2_17);
		CString getBoardPacket();
		CString getBoardChangesPacket(time_t time);
		CString getBoardChangesPacket2(time_t time);
		CString getChestPacket(TPlayer *pPlayer);
		CString getHorsePacket();
		CString getLinksPacket();
		CString getNpcsPacket(time_t time, int clientVersion = CLVER_2_17);
		CString getSignsPacket(TPlayer *pPlayer);

		//! Gets the actual level name.
		//! \return The action level name.
		CString getActualLevelName() const				{ return actualLevelName; }

		//! Gets the level name.
		//! \return The level name.
		CString getLevelName() const					{ return levelName; }

		//! Sets the level name.
		//! \param pLevelName The new name of the level.
		void setLevelName(CString pLevelName)			{ levelName = pLevelName; }

		//! Gets the raw level tile data.
		//! \return A pointer to all 4096 raw level tiles.
		short* getTiles()								{ return levelTiles; }

		//! Gets the level mod time.
		//! \return The modified time of the level when it was first loaded from the disk.
		time_t getModTime() const						{ return modTime; }

		//! Gets a vector full of all the level chests.
		//! \return The level chests.
		std::vector<TLevelChest *>* getLevelChests()	{ return &levelChests; }

		//! Gets a vector full of the level signs.
		//! \return The level signs.
		std::vector<TLevelSign *>* getLevelSigns()		{ return &levelSigns; }

		//! Gets a vector full of the players on the level.
		//! \return The players on the level.
		std::vector<TPlayer *>* getPlayerList()			{ return &levelPlayerList; }

		//! Gets the sparring zone status of the level.
		//! \return The sparring zone status.  If true, the level is a sparring zone.
		bool isSparringZone() const						{ return levelSpar; }

		//! Sets the sparring zone status of the level.
		//! \param pLevelSpar If true, the level becomes a sparring zone level.
		void setSparringZone(bool pLevelSpar)			{ levelSpar = pLevelSpar; }

		//! Gets the singleplayer status of the level.
		//! \return The singleplayer status.  If true, the level is singleplayer.
		bool isSingleplayer() const						{ return levelSingleplayer; }

		//! Sets the singleplayer status of the level.
		//! \param pLevelSingleplayer If true, the level becomes a singleplayer level.
		void setSingleplayer(bool pLevelSingleplayer)	{ levelSingleplayer = pLevelSingleplayer; }

		//! Adds a board change to the level.
		//! \param pTileData Linear array of Graal-packed tiles.  Starts with the top-left tile, ends with the bottom-right.
		//! \param pX X location of the top-left tile.
		//! \param pY Y location of the top-left tile.
		//! \param pWidth How many tiles wide we are altering.
		//! \param pHeight How many tiles high we are altering.
		//! \param player The player who initiated this board change.
		//! \return True if it succeeds, false if it doesn't.
		bool alterBoard(CString& pTileData, int pX, int pY, int pWidth, int pHeight, TPlayer* player);

		//! Adds an item to the level.
		//! \param pX X location of the item to add.
		//! \param pY Y location of the item to add.
		//! \param pItem The item we are adding.  Use TLevelItem::getItemId() to get the item type from an item name.
		//! \return True if it succeeds, false if it doesn't.
		bool addItem(float pX, float pY, char pItem);

		//! Removes an item from the level.
		//! \param pX X location of the item to remove.
		//! \param pY Y location of the item to remove.
		//! \return The type of item removed.  Use TLevelItem::getItemId() to get the item type from an item name.
		signed char removeItem(float pX, float pY);

		//! Adds a new horse to the level.
		//! \param pImage The image of the horse.
		//! \param pX X location of the horse.
		//! \param pY Y location of the horse.
		//! \param pDir The direction of the horse.
		//! \param pBushes The bushes the horse has eaten.
		//! \return Returns true if it succeeds.
		bool addHorse(CString& pImage, float pX, float pY, char pDir, char pBushes);

		//! Removes a horse from the level.
		//! \param pX X location of the horse to remove.
		//! \param pY Y location of the horse to remove.
		void removeHorse(float pX, float pY);

		//! Adds a baddy to the level.
		//! \param pX X location of the baddy to add.
		//! \param pY Y location of the baddy to add.
		//! \param pType The type of baddy to add.
		//! \return A pointer to the new TLevelBaddy.
		TLevelBaddy* addBaddy(float pX, float pY, char pType);

		//! Removes a baddy from the level.
		//! \param pId ID of the baddy to remove.
		void removeBaddy(char pId);

		//! Finds a baddy by the specified id number.
		//! \param pId The ID number of the baddy to find.
		//! \return A pointer to the found TLevelBaddy.
		TLevelBaddy* getBaddy(char id);

		//! Adds a player to the level.
		//! \param player The player to add.
		//! \return The id number of the player in the level.
		int addPlayer(TPlayer* player);

		//! Removes a player from the level.
		//! \param player The player to remove.
		void removePlayer(TPlayer* player);

		//! Gets a player from the level with the specified level id.
		//! \param id The level id the player is at.  0 will return the level leader.
		//! \return The player at the id location.
		TPlayer* getPlayer(unsigned int id);

		//! Gets the gmap this level belongs to.
		//! \return The gmap this level belongs to.
		TMap* getMap() const;

		//! Adds an NPC to the level.
		//! \param npc NPC to add to the level.
		//! \return True if the NPC was successfully added or false if it already exists in the level.
		bool addNPC(TNPC* npc);

		//! Removes an NPC from the level.
		//! \param npc The NPC to remove.
		void removeNPC(TNPC* npc);

		//! Does special events that should happen every second.
		//! \return Currently, it always returns true.
		bool doTimedEvents();

#ifdef V8NPCSERVER
		TLevelLink *isOnLink(int pX, int pY);
		TNPC *isOnNPC(int pX, int pY, bool checkEventFlag = false);
		bool isOnWall(double pX, double pY);
		bool isOnWater(double pX, double pY);
		void sendChatToLevel(const TPlayer *player, const CString& message);

		inline IScriptWrapped<TLevel> * getScriptObject() const {
			return _scriptObject;
		}

		inline void setScriptObject(IScriptWrapped<TLevel> *object) {
			_scriptObject = object;
		}
#endif

	private:
		TLevel(TServer* pServer);

		// level-loading functions
		bool loadLevel(const CString& pLevelName);
		bool detectLevelType(const CString& pLevelName);
		bool loadGraal(const CString& pLevelName);
		bool loadZelda(const CString& pLevelName);
		bool loadNW(const CString& pLevelName);

		TServer* server;
		time_t modTime;
		bool levelSpar;
		bool levelSingleplayer;
		short levelTiles[4096];
		CString fileName, fileVersion, actualLevelName, levelName;
		std::vector<TLevelBaddy *> levelBaddies;
		std::vector<TLevelBaddy *> levelBaddyIds;
		std::vector<TLevelBoardChange *> levelBoardChanges;
		std::vector<TLevelChest *> levelChests;
		std::vector<TLevelHorse *> levelHorses;
		std::vector<TLevelItem *> levelItems;
		std::vector<TLevelLink *> levelLinks;
		std::vector<TLevelSign *> levelSigns;
		std::vector<TNPC *> levelNPCs;
		std::vector<TPlayer *> levelPlayerList;

#ifdef V8NPCSERVER
	IScriptWrapped<TLevel> *_scriptObject;
#endif
};

#endif // TLEVEL_H
