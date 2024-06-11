#ifndef CFILESYSTEM_H
#define CFILESYSTEM_H

#include "CString.h"
#include <map>
#include <mutex>

class Server;

class FileSystem
{
#if defined(_WIN32) || defined(_WIN64)
	static const char fSep = '\\';
	static const char fSep_O = '/';
#else
	static const char fSep = '/';
	static const char fSep_O = '\\';
#endif

public:
	FileSystem();

	FileSystem(Server* pServer);

	~FileSystem();

	void clear();

	void setServer(Server* pServer) { m_server = pServer; }

	void addDir(const CString& dir, const CString& wildcard = "*", bool forceRecursive = false);

	void removeDir(const CString& dir);

	void addFile(CString file);

	void removeFile(const CString& file);

	void resync();

	CString find(const CString& file) const;

	CString findi(const CString& file) const;

	CString fileExistsAs(const CString& file) const;

	CString load(const CString& file) const;

	time_t getModTime(const CString& file) const;

	bool setModTime(const CString& file, time_t modTime) const;

	int getFileSize(const CString& file) const;

	std::map<CString, CString>& getFileList() { return m_fileList; }

	std::vector<CString>* getDirList() { return &m_directoryList; }

	CString getDirByExtension(const std::string& extension) const;

	mutable std::recursive_mutex* m_preventChange;

	static constexpr char getPathSeparator();

	static void fixPathSeparators(CString& pPath);

private:
	void loadAllDirectories(const CString& directory, bool recursive = false);

	Server* m_server;
	CString m_basedir;
	std::map<CString, CString> m_fileList;
	std::vector<CString> m_directoryList;
};

inline void FileSystem::fixPathSeparators(CString& pPath)
{
	pPath.replaceAllI(fSep_O, fSep);
}

constexpr char FileSystem::getPathSeparator()
{
	return fSep;
}

#endif
