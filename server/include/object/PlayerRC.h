#ifndef PLAYERRC_H
#define PLAYERRC_H

#include "object/Player.h"

// Admin-only server options.  They are protected from being changed by people without the
// 'change staff account' right.
constexpr std::array<std::string_view, 22> AdminServerOptions = {
	"name", "description", "url", "serverip", "serverport",
	"localip", "listip", "listport", "maxplayers", "onlystaff",
	"nofoldersconfig", "oldcreated", "serverside", "triggerhack_weapons", "triggerhack_guilds",
	"triggerhack_groups", "triggerhack_files", "triggerhack_rc", "flaghack_movement", "flaghack_ip",
	"sharefolder", "language"
};

// Files that are protected from being downloaded by people without the
// 'change staff account' right.
constexpr std::array<std::string_view, 4> ProtectedFiles = {
	"accounts/defaultaccount.txt",
	"config/adminconfig.txt",
	"config/allowedversions.txt",
	"config/rchelp.txt",
};

// List of important files.
constexpr std::array<std::string_view, 10> ImportantFiles = {
	"accounts/defaultaccount.txt",
	"config/adminconfig.txt",
	"config/allowedversions.txt",
	"config/foldersconfig.txt",
	"config/ipbans.txt",
	"config/rchelp.txt",
	"config/rcmessage.txt",
	"config/rules.txt",
	"config/servermessage.html",
	"config/serveroptions.txt",
};

constexpr std::array<PlayerPermissions, 10> ImportantFileRights = {
	PLPERM_MODIFYSTAFFACCOUNT,
	PLPERM_MODIFYSTAFFACCOUNT,
	PLPERM_MODIFYSTAFFACCOUNT,
	PLPERM_SETFOLDEROPTIONS,
	PLPERM_MODIFYSTAFFACCOUNT,
	PLPERM_MODIFYSTAFFACCOUNT,
	PLPERM_MODIFYSTAFFACCOUNT,
	PLPERM_MODIFYSTAFFACCOUNT,
	PLPERM_SETSERVEROPTIONS,
	PLPERM_SETSERVEROPTIONS,
};

class PlayerRC : public Player
{
public:
	PlayerRC(CSocket* pSocket, uint16_t pId) : Player(pSocket, pId) {}
	virtual ~PlayerRC() {}
	virtual void cleanup() override;

public:
	virtual bool handleLogin(CString& pPacket) override;
	virtual bool sendLogin() override;

public:
	bool isUsingFileBrowser() const { return m_isFtp; }

protected:
	virtual HandlePacketResult handlePacket(std::optional<uint8_t> id, CString& packet) override;

public:
	HandlePacketResult msgPLI_RC_SERVEROPTIONSGET(CString& pPacket);
	HandlePacketResult msgPLI_RC_SERVEROPTIONSSET(CString& pPacket);
	HandlePacketResult msgPLI_RC_FOLDERCONFIGGET(CString& pPacket);
	HandlePacketResult msgPLI_RC_FOLDERCONFIGSET(CString& pPacket);
	HandlePacketResult msgPLI_RC_RESPAWNSET(CString& pPacket);
	HandlePacketResult msgPLI_RC_HORSELIFESET(CString& pPacket);
	HandlePacketResult msgPLI_RC_APINCREMENTSET(CString& pPacket);
	HandlePacketResult msgPLI_RC_BADDYRESPAWNSET(CString& pPacket);
	HandlePacketResult msgPLI_RC_PLAYERPROPSGET(CString& pPacket);
	HandlePacketResult msgPLI_RC_PLAYERPROPSSET(CString& pPacket);
	HandlePacketResult msgPLI_RC_DISCONNECTPLAYER(CString& pPacket);
	HandlePacketResult msgPLI_RC_UPDATELEVELS(CString& pPacket);
	HandlePacketResult msgPLI_RC_ADMINMESSAGE(CString& pPacket);
	HandlePacketResult msgPLI_RC_PRIVADMINMESSAGE(CString& pPacket);
	HandlePacketResult msgPLI_RC_LISTRCS(CString& pPacket);
	HandlePacketResult msgPLI_RC_DISCONNECTRC(CString& pPacket);
	HandlePacketResult msgPLI_RC_APPLYREASON(CString& pPacket);
	HandlePacketResult msgPLI_RC_SERVERFLAGSGET(CString& pPacket);
	HandlePacketResult msgPLI_RC_SERVERFLAGSSET(CString& pPacket);
	HandlePacketResult msgPLI_RC_ACCOUNTADD(CString& pPacket);
	HandlePacketResult msgPLI_RC_ACCOUNTDEL(CString& pPacket);
	HandlePacketResult msgPLI_RC_ACCOUNTLISTGET(CString& pPacket);
	HandlePacketResult msgPLI_RC_PLAYERPROPSGET2(CString& pPacket);
	HandlePacketResult msgPLI_RC_PLAYERPROPSGET3(CString& pPacket);
	HandlePacketResult msgPLI_RC_PLAYERPROPSRESET(CString& pPacket);
	HandlePacketResult msgPLI_RC_PLAYERPROPSSET2(CString& pPacket);
	HandlePacketResult msgPLI_RC_ACCOUNTGET(CString& pPacket);
	HandlePacketResult msgPLI_RC_ACCOUNTSET(CString& pPacket);
	HandlePacketResult msgPLI_RC_CHAT(CString& pPacket);
	HandlePacketResult msgPLI_RC_WARPPLAYER(CString& pPacket);
	HandlePacketResult msgPLI_RC_PLAYERRIGHTSGET(CString& pPacket);
	HandlePacketResult msgPLI_RC_PLAYERRIGHTSSET(CString& pPacket);
	HandlePacketResult msgPLI_RC_PLAYERCOMMENTSGET(CString& pPacket);
	HandlePacketResult msgPLI_RC_PLAYERCOMMENTSSET(CString& pPacket);
	HandlePacketResult msgPLI_RC_PLAYERBANGET(CString& pPacket);
	HandlePacketResult msgPLI_RC_PLAYERBANSET(CString& pPacket);
	HandlePacketResult msgPLI_RC_FILEBROWSER_START(CString& pPacket);
	HandlePacketResult msgPLI_RC_FILEBROWSER_CD(CString& pPacket);
	HandlePacketResult msgPLI_RC_FILEBROWSER_END(CString& pPacket);
	HandlePacketResult msgPLI_RC_FILEBROWSER_DOWN(CString& pPacket);
	HandlePacketResult msgPLI_RC_FILEBROWSER_UP(CString& pPacket);
	HandlePacketResult msgPLI_NPCSERVERQUERY(CString& pPacket);
	HandlePacketResult msgPLI_RC_FILEBROWSER_MOVE(CString& pPacket);
	HandlePacketResult msgPLI_RC_FILEBROWSER_DELETE(CString& pPacket);
	HandlePacketResult msgPLI_RC_FILEBROWSER_RENAME(CString& pPacket);
	HandlePacketResult msgPLI_RC_LARGEFILESTART(CString& pPacket);
	HandlePacketResult msgPLI_RC_LARGEFILEEND(CString& pPacket);
	HandlePacketResult msgPLI_RC_FOLDERDELETE(CString& pPacket);
	HandlePacketResult msgPLI_RC_UNKNOWN162(CString& pPacket);

protected:
	bool m_isFtp = false;
	std::map<CString, CString> m_rcLargeFiles;
};

#endif // PLAYERRC_H
