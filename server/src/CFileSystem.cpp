#include "IDebug.h"
#include <sys/stat.h>
#if !defined(_WIN32) && !defined(_WIN64)
	#include <dirent.h>
	#include <utime.h>
#else
	#include <sys/utime.h>
	#define _utime utime
	#define _utimbuf utimbuf;
#endif
#include <map>
#include "ICommon.h"
#include "CString.h"
#include "CSettings.h"
#include "CFileSystem.h"
#include "TServer.h"
//nofoldersconfig 

#if defined(_WIN32) || defined(_WIN64)
	const char fSep = '\\';
	const char fSep_O = '/';
#else
	const char fSep = '/';
	const char fSep_O = '\\';
#endif

CFileSystem::CFileSystem(TServer* pServer)
: server(pServer)
{
}

void CFileSystem::clear()
{
	fileList.clear();
	dirList.clear();
}

void CFileSystem::addDir(const CString& dir, const CString& wildcard)
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	if (server == 0) return;

	// Format the directory.
	CString newDir(dir);
	if (newDir[newDir.length() - 1] == '/' || newDir[newDir.length() - 1] == '\\')
		CFileSystem::fixPathSeparators(&newDir);
	else
	{
		newDir << fSep;
		CFileSystem::fixPathSeparators(&newDir);
	}

	// Add the directory to the directory list.
	CString ndir = CString() << server->getServerPath() << newDir << wildcard;
	if (vecSearch<CString>(dirList, ndir) != -1)	// Already exists?  Resync.
		resync();
	else
	{
		dirList.push_back(ndir);

		// Load up the files in the directory.
		loadAllDirectories(ndir, server->getSettings()->getBool("nofoldersconfig", false));
	}
}

void CFileSystem::addFile(CString file)
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);

	// Grab the file name and directory.
	CFileSystem::fixPathSeparators(&file);
	CString filename(file.subString(file.findl(fSep) + 1));
	CString directory(file.subString(0, file.find(filename)));

	// Fix directory path separators.
	if (directory.find(server->getServerPath()) != -1)
		directory.removeI(0, server->getServerPath().length());

	// Add to the map.
	fileList[filename] = CString() << server->getServerPath() << directory << filename;
}

void CFileSystem::removeFile(const CString& file)
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);

	// Grab the file name and directory.
	CString filename(file.subString(file.findl(fSep) + 1));
	CString directory(file.subString(0, file.find(filename)));

	// Fix directory path separators.
	CFileSystem::fixPathSeparators(&directory);

	// Remove it from the map.
	fileList.erase(filename);
}

void CFileSystem::resync()
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);

	// Clear the file list.
	fileList.clear();

	// Iterate through all the directories, reloading their file list.
	for (std::vector<CString>::const_iterator i = dirList.begin(); i != dirList.end(); ++i)
		loadAllDirectories(*i, server->getSettings()->getBool("nofoldersconfig", false));
}

CString CFileSystem::find(const CString& file) const
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	std::map<CString, CString>::const_iterator i = fileList.find(file);
	if (i == fileList.end()) return CString();
	return CString(i->second);
}

CString CFileSystem::findi(const CString& file) const
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	for (std::map<CString, CString>::const_iterator i = fileList.begin(); i != fileList.end(); ++i)
		if (i->first.comparei(file)) return CString(i->second);
	return CString();
}

#if defined(_WIN32) || defined(_WIN64)
void CFileSystem::loadAllDirectories(const CString& directory, bool recursive)
{
	CString dir = CString() << directory.remove(directory.findl(fSep)) << fSep;
	WIN32_FIND_DATAA filedata;
	HANDLE hFind = FindFirstFileA(directory.text(), &filedata);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (filedata.cFileName[0] != '.' && recursive)
				{
					// We need to add the directory to the directory list.
					CString newDir = CString() << dir << filedata.cFileName << fSep;
					newDir.removeI(0, server->getServerPath().length());
					addDir(newDir);
				}
			}
			else
			{
				// Grab the file name.
				CString file((char *)filedata.cFileName);
				fileList[file] = CString(dir) << filedata.cFileName;
			}
		} while (FindNextFileA(hFind, &filedata));
	}
	FindClose(hFind);
}
#else
void CFileSystem::loadAllDirectories(const CString& directory, bool recursive)
{
	CString path = CString() << directory.remove(directory.findl(fSep)) << fSep;
	CString wildcard = directory.subString(directory.findl(fSep) + 1);
	DIR *dir;
	struct stat statx;
	struct dirent *ent;

	// Try to open the directory.
	if ((dir = opendir(path.text())) == 0)
		return;

	// Read everything in it now.
	while ((ent = readdir(dir)) != 0)
	{
		if (ent->d_name[0] != '.')
		{
			CString dircheck = CString() << path << ent->d_name;
			stat(dircheck.text(), &statx);
			if ((statx.st_mode & S_IFDIR))
			{
				if (recursive)
				{
					// We need to add the directory to the directory list.
					CString newDir = CString() << path << ent->d_name << fSep;
					newDir.removeI(0, server->getServerPath().length());
					addDir(newDir);
				}
				continue;
			}
		}
		else continue;

		// Grab the file name.
		CString file(ent->d_name);
		if (file.match(wildcard))
			fileList[file] = CString(path) << file;
	}
	closedir(dir);
}
#endif

CString CFileSystem::load(const CString& file) const
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);

	// Get the full path to the file.
	CString fileName = find(file);
	if (fileName.length() == 0) return CString();

	// Load the file.
	CString fileData;
	fileData.load(fileName);

	return fileData;
}

time_t CFileSystem::getModTime(const CString& file) const
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);

	// Get the full path to the file.
	CString fileName = find(file);
	if (fileName.length() == 0) return 0;

	struct stat fileStat;
	if (stat(fileName.text(), &fileStat) != -1)
		return (time_t)fileStat.st_mtime;
	return 0;
}

bool CFileSystem::setModTime(const CString& file, time_t modTime) const
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);

	// Get the full path to the file.
	CString fileName = find(file);
	if (fileName.length() == 0) return false;

	// Set the times.
	struct utimbuf ut;
	ut.actime = modTime;
	ut.modtime = modTime;

	// Change the file.
	if (utime(fileName.text(), &ut) == 0) return true;
	return false;
}

int CFileSystem::getFileSize(const CString& file) const
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);

	// Get the full path to the file.
	CString fileName = find(file);
	if (fileName.length() == 0) return 0;

	struct stat fileStat;
	if (stat(fileName.text(), &fileStat) != -1)
		return fileStat.st_size;
	return 0;
}

void CFileSystem::fixPathSeparators(CString* pPath)
{
	pPath->replaceAllI(fSep_O, fSep);
}

char CFileSystem::getPathSeparator()
{
	return fSep;
}
