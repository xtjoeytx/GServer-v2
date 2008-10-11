#ifndef CFILESYSTEM_H
#define CFILESYSTEM_H

#include <map>
#include "ICommon.h"
#include "CString.h"
//#include "TServer.h"

class TServer;
class CFileSystem
{
	public:
		CFileSystem() : server(0) { }
		CFileSystem(TServer* pServer);

		void setServer(TServer* pServer) { server = pServer; }
		void init(const CString& dir);

		CString find(const CString& file) const;
		CString load(const CString& file) const;
		time_t getModTime(const CString& file) const;
		int getFileSize(const CString& file) const;
		std::map<CString, CString>* getFileList()	{ return &fileList; }

	private:
		TServer* server;
		std::map<CString, CString> fileList;
		mutable boost::recursive_mutex m_preventChange;
};

#endif
