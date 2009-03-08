#include "ICommon.h"
#include <queue>
#include "CFileQueue.h"
#include "CString.h"
#include "CSocket.h"
#include "TPlayer.h"

void CFileQueue::addPacket(CString pPacket)
{
	while (pPacket.bytesLeft() != 0)
	{
		unsigned char pId = pPacket.readGUChar();
		pPacket.setRead(pPacket.readPos() - 1);
		if (pId == PLO_RAWDATA)
		{
			// Read packet 100.
			pack100 = CString() << pPacket.readString("\n") << "\n";
			pack100.setRead(1);		// Don't read the packet ID.
			prev100 = true;
			size100 = pack100.readGUInt();
		}
		else
		{
			// Read the packet.  If the previous packet was 100, read the specified bytes.
			// If it isn't level data, put it into the file buffer.
			CString packet;
			if (prev100)
			{
				packet = pPacket.readChars(size100);
				if (packet.readGUChar() == PLO_BOARDPACKET)
					normalBuffer.push(CString() << pack100 << packet);
				else
					fileBuffer.push(CString() << pack100 << packet);
				prev100 = false;
			}
			else
			{
				// Else, read to \n.
				packet = CString() << pPacket.readString("\n") << "\n";

				// Certain file related packets go to the file buffer since they must be
				// sent in order.
				switch (pId)
				{
					case PLO_LARGEFILESTART:
					case PLO_LARGEFILEEND:
					case PLO_LARGEFILESIZE:
						fileBuffer.push(packet);
						break;

					// Everything else goes into the normal buffer.
					default:
						normalBuffer.push(packet);
				}
			}
		}
	}
}

bool CFileQueue::canSend()
{
	if (fileBuffer.size() != 0 || normalBuffer.size() != 0) return true;
	return false;
}

void CFileQueue::sendCompress()
{
	CString pSend;

	if (sock == 0 || sock->getState() == SOCKET_STATE_DISCONNECTED)
		return;

	// If we haven't sent a file in a while, forcibly send one now.
	if (bytesSentWithoutFile > 0x7FFF && !fileBuffer.empty())	// 32KB
	{
		bytesSentWithoutFile = 0;
		pSend << fileBuffer.front();
		fileBuffer.pop();
	}

	// Keep adding packets from normalBuffer until we hit 48KB or we run out of packets.
	while (pSend.length() < 0xC000 && !normalBuffer.empty())	// 48KB
	{
		pSend << normalBuffer.front();
		normalBuffer.pop();
	}
	bytesSentWithoutFile += pSend.length();

	// If we have less than 16KB of data, add a file.
	if (pSend.length() < 0x4000 && !fileBuffer.empty())	// 16KB
	{
		bytesSentWithoutFile = 0;
		pSend << fileBuffer.front();
		fileBuffer.pop();
	}

	// Reset this if we have no files to send.
	if (fileBuffer.empty()) bytesSentWithoutFile = 0;

	// If we have no data, just return.
	if (pSend.length() == 0)
		return;

	// compress buffer
	if (out_codec.getGen() >= ENCRYPT_GEN_4)
	{
		// Choose which compression to use and apply it.
		int compressionType = COMPRESS_UNCOMPRESSED;
		if (pSend.length() > 0x2000)	// 8KB
		{
			compressionType = COMPRESS_BZ2;
			pSend.bzcompressI();
		}
		else if (pSend.length() > 40)
		{
			compressionType = COMPRESS_ZLIB;
			pSend.zcompressI();
		}

		// Encrypt the packet and add it to the out buffer.
		out_codec.limitFromType(compressionType);
		pSend = out_codec.encrypt(pSend);
		CString data = CString() << (short)(pSend.length() + 1) << (char)compressionType << pSend;
		oBuffer << data;
		unsigned int dsize = oBuffer.length();
		oBuffer.removeI(0, sock->sendData(oBuffer.text(), &dsize));
	}
	else
	{
		// Compress the packet and add it to the out buffer.
		pSend.zcompressI();
		CString data = CString() << (short)pSend.length() << pSend;
		oBuffer << data;
		unsigned int dsize = oBuffer.length();
		oBuffer.removeI(0, sock->sendData(oBuffer.text(), &dsize));
	}
}
