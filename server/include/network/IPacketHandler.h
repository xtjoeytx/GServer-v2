#ifndef IPACKETHANDLER_H
#define IPACKETHANDLER_H

#include <array>
#include <cstdint>
#include <optional>
#include <string>

#include <CEncryption.h>
#include <CString.h>
#include <IEnums.h>

#ifdef PACKETLOGGING
#include <utilities/Log.h>
#endif

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

#ifdef PACKETLOGGING
#define FOR_INPUT_PACKETS(DO) \
	DO(PLI_LEVELWARP) \
	DO(PLI_BOARDMODIFY) \
	DO(PLI_PLAYERPROPS) \
	DO(PLI_NPCPROPS) \
	DO(PLI_BOMBADD) \
	DO(PLI_BOMBDEL) \
	DO(PLI_TOALL) \
	DO(PLI_HORSEADD) \
	DO(PLI_HORSEDEL) \
	DO(PLI_ARROWADD) \
	DO(PLI_FIRESPY) \
	DO(PLI_THROWCARRIED) \
	DO(PLI_ITEMADD) \
	DO(PLI_ITEMDEL) \
	DO(PLI_CLAIMPKER) \
	DO(PLI_BADDYPROPS) \
	DO(PLI_BADDYHURT) \
	DO(PLI_BADDYADD) \
	DO(PLI_FLAGSET) \
	DO(PLI_FLAGDEL) \
	DO(PLI_OPENCHEST) \
	DO(PLI_PUTNPC) \
	DO(PLI_NPCDEL) \
	DO(PLI_WANTFILE) \
	DO(PLI_SHOWIMG) \
	DO(PLI_UNKNOWN25) \
	DO(PLI_HURTPLAYER) \
	DO(PLI_EXPLOSION) \
	DO(PLI_PRIVATEMESSAGE) \
	DO(PLI_NPCWEAPONDEL) \
	DO(PLI_LEVELWARPMOD) \
	DO(PLI_PACKETCOUNT) \
	DO(PLI_ITEMTAKE) \
	DO(PLI_WEAPONADD) \
	DO(PLI_UPDATEFILE) \
	DO(PLI_ADJACENTLEVEL) \
	DO(PLI_HITOBJECTS) \
	DO(PLI_LANGUAGE) \
	DO(PLI_TRIGGERACTION) \
	DO(PLI_MAPINFO) \
	DO(PLI_SHOOT) \
	DO(PLI_SERVERWARP) \
	DO(PLI_MUTEPLAYER) \
	DO(PLI_PROCESSLIST) \
	DO(PLI_UNKNOWN46) \
	DO(PLI_VERIFYWANTSEND) \
	DO(PLI_SHOOT2) \
	DO(PLI_RAWDATA) \
	DO(PLI_RC_SERVEROPTIONSGET) \
	DO(PLI_RC_SERVEROPTIONSSET) \
	DO(PLI_RC_FOLDERCONFIGGET) \
	DO(PLI_RC_FOLDERCONFIGSET) \
	DO(PLI_RC_RESPAWNSET) \
	DO(PLI_RC_HORSELIFESET) \
	DO(PLI_RC_APINCREMENTSET) \
	DO(PLI_RC_BADDYRESPAWNSET) \
	DO(PLI_RC_PLAYERPROPSGET) \
	DO(PLI_RC_PLAYERPROPSSET) \
	DO(PLI_RC_DISCONNECTPLAYER) \
	DO(PLI_RC_UPDATELEVELS) \
	DO(PLI_RC_ADMINMESSAGE) \
	DO(PLI_RC_PRIVADMINMESSAGE) \
	DO(PLI_RC_LISTRCS) \
	DO(PLI_RC_DISCONNECTRC) \
	DO(PLI_RC_APPLYREASON) \
	DO(PLI_RC_SERVERFLAGSGET) \
	DO(PLI_RC_SERVERFLAGSSET) \
	DO(PLI_RC_ACCOUNTADD) \
	DO(PLI_RC_ACCOUNTDEL) \
	DO(PLI_RC_ACCOUNTLISTGET) \
	DO(PLI_RC_PLAYERPROPSGET2) \
	DO(PLI_RC_PLAYERPROPSGET3) \
	DO(PLI_RC_PLAYERPROPSRESET) \
	DO(PLI_RC_PLAYERPROPSSET2) \
	DO(PLI_RC_ACCOUNTGET) \
	DO(PLI_RC_ACCOUNTSET) \
	DO(PLI_RC_CHAT) \
	DO(PLI_PROFILEGET) \
	DO(PLI_PROFILESET) \
	DO(PLI_RC_WARPPLAYER) \
	DO(PLI_RC_PLAYERRIGHTSGET) \
	DO(PLI_RC_PLAYERRIGHTSSET) \
	DO(PLI_RC_PLAYERCOMMENTSGET) \
	DO(PLI_RC_PLAYERCOMMENTSSET) \
	DO(PLI_RC_PLAYERBANGET) \
	DO(PLI_RC_PLAYERBANSET) \
	DO(PLI_RC_FILEBROWSER_START) \
	DO(PLI_RC_FILEBROWSER_CD) \
	DO(PLI_RC_FILEBROWSER_END) \
	DO(PLI_RC_FILEBROWSER_DOWN) \
	DO(PLI_RC_FILEBROWSER_UP) \
	DO(PLI_NPCSERVERQUERY) \
	DO(PLI_RC_FILEBROWSER_MOVE) \
	DO(PLI_RC_FILEBROWSER_DELETE) \
	DO(PLI_RC_FILEBROWSER_RENAME) \
	DO(PLI_NC_NPCGET) \
	DO(PLI_NC_NPCDELETE) \
	DO(PLI_NC_NPCRESET) \
	DO(PLI_NC_NPCSCRIPTGET) \
	DO(PLI_NC_NPCWARP) \
	DO(PLI_NC_NPCFLAGSGET) \
	DO(PLI_NC_NPCSCRIPTSET) \
	DO(PLI_NC_NPCFLAGSSET) \
	DO(PLI_NC_NPCADD) \
	DO(PLI_NC_CLASSEDIT) \
	DO(PLI_NC_CLASSADD) \
	DO(PLI_NC_LOCALNPCSGET) \
	DO(PLI_NC_WEAPONLISTGET) \
	DO(PLI_NC_WEAPONGET) \
	DO(PLI_NC_WEAPONADD) \
	DO(PLI_NC_WEAPONDELETE) \
	DO(PLI_NC_CLASSDELETE) \
	DO(PLI_REQUESTUPDATEBOARD) \
	DO(PLI_NC_LEVELLISTGET) \
	DO(PLI_NC_LEVELLISTSET) \
	DO(PLI_REQUESTTEXT) \
	DO(PLI_SENDTEXT) \
	DO(PLI_RC_LARGEFILESTART) \
	DO(PLI_RC_LARGEFILEEND) \
	DO(PLI_UPDATEGANI) \
	DO(PLI_UPDATESCRIPT) \
	DO(PLI_UPDATEPACKAGEREQUESTFILE) \
	DO(PLI_RC_FOLDERDELETE) \
	DO(PLI_UPDATECLASS) \
	DO(PLI_RC_UNKNOWN162) \
	DO(PLI_SET_ENC_KEY) \
	DO(PLI_BUNDLE)
#define FILL_INPUT_ARRAY(name) names[(uint8_t)name] = #name;

constexpr std::array<std::string, 255> FillInputPacketNamesArray()
{
	std::array<std::string, 255> names;
	names.fill("(unknown packet)");
	FOR_INPUT_PACKETS(FILL_INPUT_ARRAY)
	return names;
}

inline std::array<std::string, 255> InputPacketNamesArray = FillInputPacketNamesArray();
#endif

///////////////////////////////////////////////////////////////////////////////

enum class HandlePacketResult
{
	Handled,
	Bubble,
	Failed,
};

enum class PacketHandleMode
{
	OLDPROTOCOL,
	NEWPROTOCOL
};

class IPacketHandler
{
public:
	virtual ~IPacketHandler() = default;

public:
	void processBuffer(CString& buffer);

protected:
	std::optional<CString> retrievePacketBundle(CString& buffer) const;
	void processPacketBundle(CString& packet);
	void parsePacketsFromBundle(CString& packet);
	void parseLoginPacket(CString& buffer);
	virtual HandlePacketResult handlePacket(std::optional<uint8_t> id, CString& packet) = 0;

public:
	CEncryption Encryption;
	uint32_t PacketCount = 0;
	uint32_t InvalidPackets = 0;

public:
	PacketHandleMode HandleMode = PacketHandleMode::OLDPROTOCOL;
	bool RemoveNewlinesFromRawPacket = false;
	bool RemoveNewlineFromFileUpload = false;

protected:
	bool m_nextIsRaw = false;
	size_t m_rawPacketSize = 0;
};

inline void IPacketHandler::processBuffer(CString& buffer)
{
	buffer.setRead(0);
	while (buffer.length() > 2)
	{
		auto result = retrievePacketBundle(buffer);
		if (result.has_value() == false)
			break;
		auto& bundle = result.value();
		if (bundle.isEmpty())
			break;

		// Process the packet bundle.
		processPacketBundle(bundle);

		// Parse the packets.
		if (PacketCount != 0) [[likely]]
			parsePacketsFromBundle(bundle);
		else
		{
			// Login packet should parse differently.
			// We also break immediately after parsing it since we are going to create a new player.
			parseLoginPacket(bundle);
			break;
		}
	}
}

inline std::optional<CString> IPacketHandler::retrievePacketBundle(CString& buffer) const
{
	uint16_t packetSize = static_cast<uint16_t>(buffer.readShort());
	if (packetSize > buffer.length() - 2)
		return std::nullopt;

	CString packet = buffer.readChars(packetSize);
	buffer.removeI(0, packetSize + 2);
	return std::make_optional<CString>(packet);
}

inline void IPacketHandler::processPacketBundle(CString& bundle)
{
	// No encryption or compression.
	if (Encryption.getGen() == ENCRYPT_GEN_1)
		return;

	// Version 1.41 - 2.18 non-client.
	// Not encrypted, but zlib compressed.
	if (Encryption.getGen() == ENCRYPT_GEN_2)
	{
		bundle.zuncompressI();
	}
	// Version 1.41 - 2.18 client encryption
	// Compressed with zlib, individual packets in bundle encrypted.
	else if (Encryption.getGen() == ENCRYPT_GEN_3)
	{
		bundle.zuncompressI();
	}
	// Version 2.19+ encryption.
	// Bundle compressed and then encrypted.  Always BZ2 compressed.
	else if (Encryption.getGen() == ENCRYPT_GEN_4)
	{
		// Decrypt the bundle.
		Encryption.limitFromType(COMPRESS_BZ2);
		Encryption.decrypt(bundle);

		// Uncompress bundle.
		bundle.bzuncompressI();
	}
	// Compressed and then encrypted.  Encryption depends on the compression type.
	else if (Encryption.getGen() >= ENCRYPT_GEN_5)
	{
		// Find the compression type and remove it.
		int pType = bundle.readChar();
		bundle.removeI(0, 1);

		// Decrypt the bundle.
		Encryption.limitFromType(pType); // Encryption is partially related to compression.
		Encryption.decrypt(bundle);

		// Uncompress bundle
		if (pType == COMPRESS_ZLIB)
			bundle.zuncompressI();
		else if (pType == COMPRESS_BZ2)
			bundle.bzuncompressI();
		else if (pType != COMPRESS_UNCOMPRESSED)
			; // log::printLine(log::server, "** [ERROR] Client gave incorrect packet compression type! [{}]", pType);
	}
}

inline void IPacketHandler::parsePacketsFromBundle(CString& bundle)
{
	while (bundle.bytesLeft() > 0)
	{
		// Grab a packet out of the input stream.
		CString curPacket;
		if (m_nextIsRaw)
		{
			m_nextIsRaw = false;
			curPacket = bundle.readChars(m_rawPacketSize);

			// The client and RC versions above 1.1 append a \n to the end of the packet.
			// Remove it now.
			//if (isClient() || (isRC() && m_versionId > RCVER_1_1))
			if (RemoveNewlinesFromRawPacket)
			{
				if (curPacket[curPacket.length() - 1] == '\n')
					curPacket.removeI(curPacket.length() - 1);
			}
		}
		else
			curPacket = bundle.readString("\n");

		// Generation 3 encrypts individual packets so decrypt it now.
		if (Encryption.getGen() == ENCRYPT_GEN_3)
			Encryption.decrypt(curPacket);

		// Get the packet id.
		unsigned char id = curPacket.readGUChar();
		++PacketCount;

		// RC version 1.1 adds a "\n" string to the end of file uploads instead of a newline character.
		// This causes issues because it messes with the packet order.
		//if (isRC() && m_versionId == RCVER_1_1 && id == PLI_RC_FILEBROWSER_UP)
		if (RemoveNewlineFromFileUpload && id == PLI_RC_FILEBROWSER_UP)
		{
			curPacket.removeI(curPacket.length() - 1);
			curPacket.setRead(1);
			bundle.readChar(); // Read out the \n that got left behind.
		}

#ifdef PACKETLOGGING
		log::printLine(log::networkdump, "> In Packet: [{}] {} ({} bytes)", (uint32_t)id, InputPacketNamesArray[id], curPacket.length());
		log::print(log::networkdump, "{}", curPacket.text());
		if (curPacket[curPacket.length() - 1] != '\n')
			log::print(log::networkdump, "\n");
		for (int i = 0; i < curPacket.length(); ++i)
			log::print(log::networkdump, "{:02x} ", (unsigned char)((curPacket.text())[i]));
		log::print(log::networkdump, "\n\n");
#endif

		// Raw packet handling.
		if (id == PLI_RAWDATA)
		{
			m_nextIsRaw = true;
			m_rawPacketSize = curPacket.readGUInt();
			continue;
		}

		// Call the function assigned to the packet id.
		handlePacket(id, curPacket);
	}
}

inline void IPacketHandler::parseLoginPacket(CString& buffer)
{
	++PacketCount;

	// Call the login packet handler function.
	auto packet = buffer.readString("\n");
	handlePacket(std::nullopt, packet);
}

///////////////////////////////////////////////////////////////////////////////

} // end namespace preagonal

#endif // IPACKETHANDLER_H
