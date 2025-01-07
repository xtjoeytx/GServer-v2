#ifndef IPACKETHANDLER_H
#define IPACKETHANDLER_H

#include <optional>
#include <cstdint>

#include <CString.h>
#include <CEncryption.h>
#include <IEnums.h>

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
	PacketHandleMode PacketHandleMode = PacketHandleMode::OLDPROTOCOL;
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
			; // serverlog.out("** [ERROR] Client gave incorrect packet compression type! [%d]\n", pType);
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

#endif // IPACKETHANDLER_H
