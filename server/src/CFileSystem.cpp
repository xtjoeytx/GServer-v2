#include <sys/stat.h>
#if !defined(_WIN32) && !defined(_WIN64)
	#include <dirent.h>
#endif
#include <map>
#include "ICommon.h"
#include "CString.h"
#include "CSettings.h"
#include "CFileSystem.h"
#include "TServer.h"
//nofoldersconfig 

#if defined(_WIN32) || defined(_WIN64)
	#define fSep "\\"
#else
	#define fSep "/"
#endif

static void loadAllDirectories(std::map<CString, CString>& fileList, const CString& directory);

CFileSystem::CFileSystem(TServer* pServer)
: server(pServer)
{
}

void CFileSystem::init(const CString& dir)
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	if (server == 0) return;
	CSettings* settings = server->getSettings();

	// Load every directory if we aren't using folders config.
	if (settings->getBool("nofoldersconfig", false) == true)
	{
		fileList.clear();
		loadAllDirectories(fileList, CString() << server->getServerPath() << dir << fSep);

		// Load extra folders.
		if (settings->getStr("sharefolder").length() > 0)
		{
			std::vector<CString> folders = settings->getStr("sharefolder").tokenize(",");
			for (std::vector<CString>::iterator i = folders.begin(); i != folders.end(); ++i)
			{
				CString f = CString() << server->getServerPath() << (*i).trim();
				if (f[f.length() - 1] != '/' || f[f.length() - 1] != '\\') f << fSep;
				loadAllDirectories(fileList, f);
			}
		}
	}
	else
	{
		// TODO: folders config
		printf( "TODO: CFileSystem::load, Do folders config.\n" );
	}
}

CString CFileSystem::find(const CString& file) const
{
	boost::recursive_mutex::scoped_lock lock(m_preventChange);
	std::map<CString, CString>::const_iterator i = fileList.find(file);
	if (i == fileList.end()) return CString();
	return CString(i->second);
}

#if defined(_WIN32) || defined(_WIN64)
void loadAllDirectories(std::map<CString, CString>& fileList, const CString& directory)
{
	CString searchdir = CString() << directory << "*";
	WIN32_FIND_DATAA filedata;
	HANDLE hFind = FindFirstFileA(searchdir.text(), &filedata);

	if (hFind != 0)
	{
		do
		{
			if (filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (filedata.cFileName[0] != '.')
					loadAllDirectories(fileList, CString() << directory << CString(filedata.cFileName) << fSep);
			}
			else
			{
				// Grab the file name.
				CString file(filedata.cFileName);
				fileList[file] = CString() << directory << filedata.cFileName;
			}
		} while (FindNextFileA(hFind, &filedata));
	}
	FindClose(hFind);
}
#else
void loadAllDirectories(std::map<CString, CString>& fileList, const CString& directory)
{
	DIR *dir;
	struct stat statx;
	struct dirent *ent;

	// Try to open the directory.
	if ((dir = opendir(directory.text())) == 0)
		return;

	// Read everything in it now.
	while ((ent = readdir(dir)) != 0)
	{
		if (ent->d_name[0] != '.')
		{
			CString dir = CString() << directory << ent->d_name;
			stat(dir.text(), &statx);
			if (statx.st_mode & S_IFDIR)
			{
				loadAllDirectories(fileList, dir);
				continue;
			}
		}
		else continue;

		// Grab the file name.
		CString file(ent->d_name);
		fileList[file] = CString() << directory << ent->d_name;
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
