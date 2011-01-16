#include <queue>
#include "IDebug.h"
#include "IEnums.h"
#include "CFileQueue.h"

void CFileQueue::addPacket(CString pPacket)
{
	while (pPacket.bytesLeft() != 0)
	{
		// Protect against invalid packet ids.
		if ((unsigned char)pPacket[pPacket.readPos()] < 0x20)
			break;

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
						break;
				}
			}
		}
	}
}

bool CFileQueue::canSend()
{
	if (normalBuffer.size() != 0 || fileBuffer.size() != 0) return true;
	return false;
}

void CFileQueue::sendCompress()
{
	CString pSend;

	if (sock == 0 || sock->getState() == SOCKET_STATE_DISCONNECTED)
		return;

	// If the next normal packet is huge, lets "try" to send it.
	// Everything else should skip because this may throw is way over the limit.
	if (!normalBuffer.empty() && normalBuffer.front().length() > 0xF000)
	{
		pSend << normalBuffer.front();
		normalBuffer.pop();
	}

	// If we haven't sent a file in a while, forcibly send one now.
	if (pSend.length() == 0 && bytesSentWithoutFile > 0x7FFF && !fileBuffer.empty())	// 32KB
	{
		bytesSentWithoutFile = 0;
		pSend << fileBuffer.front();
		fileBuffer.pop();
	}

	// Keep adding packets from normalBuffer until we hit 48KB or we run out of packets.
	while (pSend.length() < 0xC000 && !normalBuffer.empty())	// 48KB
	{
		// If the next packet sticks us over 60KB, don't add it.
		if (pSend.length() + normalBuffer.front().length() > 0xF000) break;

		pSend << normalBuffer.front();
		normalBuffer.pop();
	}
	bytesSentWithoutFile += pSend.length();

	// If we have less than 16KB of data, try to add a file.
	if (pSend.length() < 0x4000 && !fileBuffer.empty())	// 16KB
	{
		// If the next packet sticks us over 60KB, don't add it.
		if (pSend.length() + fileBuffer.front().length() <= 0xF000)
		{
			bytesSentWithoutFile = 0;
			pSend << fileBuffer.front();
			fileBuffer.pop();
		}
	}

	// Reset this if we have no files to send.
	if (fileBuffer.empty()) bytesSentWithoutFile = 0;

	// If we have no data, just return.
	if (pSend.length() == 0)
		return;

	// compress buffer
	switch (out_codec.getGen())
	{
		case ENCRYPT_GEN_1:
		case ENCRYPT_GEN_2:
			printf("** Generations 1 and 2 are not supported!\n");
			break;

		case ENCRYPT_GEN_3:
		{
			// Compress the packet.
			pSend.zcompressI();

			// Sanity check.
			if (pSend.length() > 0xFFFD)
			{
				printf("** [ERROR] Trying to send a GEN_3 packet over 65533 bytes!  Tossing data.\n");
				return;
			}

			// Add the packet to the out buffer.
			CString data = CString() << (short)pSend.length() << pSend;
			oBuffer << data;
			unsigned int dsize = oBuffer.length();
			oBuffer.removeI(0, sock->sendData(oBuffer.text(), &dsize));
			break;
		}

		case ENCRYPT_GEN_4:
		{
			pSend.bzcompressI();

			// Sanity check.
			if (pSend.length() > 0xFFFD)
			{
				printf("** [ERROR] Trying to send a GEN_4 packet over 65533 bytes!  Tossing data.\n");
				return;
			}

			// Encrypt the packet and add it to the out buffer.
			out_codec.limitFromType(COMPRESS_BZ2);
			pSend = out_codec.encrypt(pSend);
			CString data = CString() << (short)pSend.length() << pSend;
			oBuffer << data;
			unsigned int dsize = oBuffer.length();
			oBuffer.removeI(0, sock->sendData(oBuffer.text(), &dsize));
			break;
		}

		case ENCRYPT_GEN_5:
		{
			//unsigned int oldSize = pSend.length();

			// Choose which compression to use and apply it.
			int compressionType = COMPRESS_UNCOMPRESSED;
			if (pSend.length() > 0x2000)	// 8KB
			{
				compressionType = COMPRESS_BZ2;
				pSend.bzcompressI();
			}
			else if (pSend.length() > 55)
			{
				compressionType = COMPRESS_ZLIB;
				pSend.zcompressI();
			}

			//unsigned int newSize = pSend.length();
			//printf("Compression [%s] - old size: %ld, new size: %ld\n", (compressionType == COMPRESS_UNCOMPRESSED ? "uncompressed" : (compressionType == COMPRESS_ZLIB ? "zlib" : "bz2")), oldSize, newSize);

			// Sanity check.
			if (pSend.length() > 0xFFFC)
			{
				printf("** [ERROR] Trying to send a GEN_5 packet over 65532 bytes!  Tossing data.\n");
				return;
			}

			// Encrypt the packet and add it to the out buffer.
			out_codec.limitFromType(compressionType);
			pSend = out_codec.encrypt(pSend);
			CString data = CString() << (short)(pSend.length() + 1) << (char)compressionType << pSend;
			oBuffer << data;
			unsigned int dsize = oBuffer.length();
			oBuffer.removeI(0, sock->sendData(oBuffer.text(), &dsize));
			break;
		}
	}
}
