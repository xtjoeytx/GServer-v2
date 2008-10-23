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
		CFileQueue(CSocket* pSock, bool isPost22) : sock(pSock), prev100(false), PLE_POST22(isPost22), bytesSentWithoutFile(0) {}
		void operator()();

		void setSocket(CSocket* pSock)		{ boost::mutex::scoped_lock lock_preventChange(m_preventChange); sock = pSock; }
		void setCodecKey(unsigned char key)	{ boost::mutex::scoped_lock lock_preventChange(m_preventChange); PLE_POST22 = true; out_codec.reset(key); }
		void addPacket(CString pPacket);

	private:
		void sendCompress();

		boost::mutex m_preventChange;
		CSocket* sock;
		std::queue<CString> normalBuffer;
		std::queue<CString> fileBuffer;
		bool prev100;
		unsigned int size100;
		CString pack100;

		bool PLE_POST22;
		codec out_codec;

		int bytesSentWithoutFile;
};

#endif
