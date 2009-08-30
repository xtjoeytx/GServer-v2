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
		void clear();

		void setServer(TServer* pServer) { server = pServer; }

		void addDir(const CString& dir, const CString& wildcard = "*", bool forceRecursive = false);
		void removeDir(const CString& dir);
		void addFile(CString file);
		void removeFile(const CString& file);
		void resync();

		CString find(const CString& file) const;
		CString findi(const CString& file) const;
		CString load(const CString& file) const;
		time_t getModTime(const CString& file) const;
		bool setModTime(const CString& file, time_t modTime) const;
		int getFileSize(const CString& file) const;
		std::map<CString, CString>* getFileList()	{ return &fileList; }
		std::vector<CString>* getDirList()			{ return &dirList; }

		mutable boost::recursive_mutex m_preventChange;

		static void fixPathSeparators(CString* pPath);
		static char getPathSeparator();

	private:
		void loadAllDirectories(const CString& directory, bool recursive = false);

		TServer* server;
		CString basedir;
		std::map<CString, CString> fileList;
		std::vector<CString> dirList;
};

#endif
