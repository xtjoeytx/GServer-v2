#include "ICommon.h"
#include <queue>
#include "CFileQueue.h"
#include "CString.h"
#include "CSocket.h"
#include "TPlayer.h"

void CFileQueue::operator()()
{
	while (sock != 0)
	{
		// Check for data to be sent.
		bool canSend = true;
		{
			boost::mutex::scoped_lock lock_preventChange(m_preventChange);
			if (normalBuffer.empty() && fileBuffer.empty()) canSend = false;
		}

		// If we have nothing to send, just yield this thread.
		if (!canSend)
		{
			boost::xtime xt;
			boost::xtime_get(&xt, boost::TIME_UTC);
			xt.nsec += 10000000;		// 10 milliseconds
			boost::thread::sleep(xt); 
			//boost::this_thread::yield();
		}
		else
		{
			boost::mutex::scoped_lock lock_preventChange(m_preventChange);
			sendCompress();
		}
	}
}

void CFileQueue::addPacket(CString pPacket)
{
	boost::mutex::scoped_lock lock_preventChange(m_preventChange);

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

void CFileQueue::sendCompress()
{
	CString pSend;

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

	// compress buffer
	if (PLE_POST22)
	{
		// Choose which compression to use and apply it.
		int compressionType = ENCRYPT22_UNCOMPRESSED;
		if (pSend.length() > 0x2000)	// 8KB
		{
			compressionType = ENCRYPT22_BZ2;
			pSend.bzcompressI();
		}
		else if (pSend.length() > 40)
		{
			compressionType = ENCRYPT22_ZLIB;
			pSend.zcompressI();
		}

		// Encrypt the packet and add it to the out buffer.
		out_codec.limitfromtype(compressionType);
		out_codec.apply(reinterpret_cast<uint8_t*>(pSend.text()), pSend.length());
		sock->sendData(CString() << (short)(pSend.length() + 1) << (char)compressionType << pSend);
	}
	else
	{
		// Compress the packet and add it to the out buffer.
		pSend.zcompressI();
		sock->sendData(CString() << (short)pSend.length() << pSend);
	}
}
