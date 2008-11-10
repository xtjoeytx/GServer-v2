#ifndef CFILEQUEUE_H
#define CFILEQUEUE_H

#include <queue>
#include "ICommon.h"
#include "CString.h"
#include "CSocket.h"
#include "codec.h"

class CFileQueue
{
	public:
		CFileQueue(CSocket* pSock) : sock(pSock), prev100(false), bytesSentWithoutFile(0) {}
		void operator()();

		void addPacket(CString pPacket);
		void setSocket(CSocket* pSock)
		{
			boost::mutex::scoped_lock lock_preventChange(m_preventChange);
			sock = pSock;
		}
		void setCodec(unsigned int gen, unsigned char key)
		{
			boost::mutex::scoped_lock lock_preventChange(m_preventChange);
			out_codec.setGen(gen);
			out_codec.reset(key);
		}

		void forceSend()
		{
			boost::mutex::scoped_lock lock_preventChange(m_preventChange);
			if (normalBuffer.empty() && fileBuffer.empty()) return;
			sendCompress();
		}

	private:
		void sendCompress();

		boost::mutex m_preventChange;
		CSocket* sock;
		std::queue<CString> normalBuffer;
		std::queue<CString> fileBuffer;
		bool prev100;
		unsigned int size100;
		CString pack100;

		codec out_codec;

		int bytesSentWithoutFile;
};

#endif
