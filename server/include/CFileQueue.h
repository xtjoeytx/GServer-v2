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

		void addPacket(CString pPacket);
		void setSocket(CSocket* pSock)		{ sock = pSock; }
		void setCodec(unsigned int gen, unsigned char key)
		{
			out_codec.setGen(gen);
			out_codec.reset(key);
		}

		void sendCompress();

	private:
		CSocket* sock;
		std::queue<CString> normalBuffer;
		std::queue<CString> fileBuffer;
		bool prev100;
		unsigned int size100;
		CString pack100;
		CString oBuffer;

		codec out_codec;

		int bytesSentWithoutFile;
};

#endif
