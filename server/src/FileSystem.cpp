#include "IDebug.h"
#include <sys/stat.h>
#if (defined(_WIN32) || defined(_WIN64)) && !defined(__GNUC__)
	#include <sys/utime.h>
	#define _utime utime
	#define _utimbuf utimbuf;
#else
	#include <dirent.h>
	#include <utime.h>
#endif
#include "FileSystem.h"
#include "IDebug.h"
#include "IUtil.h"
#include "Server.h"
#include <map>

#if defined(_WIN32) || defined(_WIN64)
	#ifndef __GNUC__ // rain
		#include <mutex>
		#include <condition_variable>
	#endif
#endif

FileSystem::FileSystem()
	: server(nullptr)
{
	m_preventChange = new std::recursive_mutex();
}

FileSystem::FileSystem(Server* pServer)
	: server(pServer)
{
	m_preventChange = new std::recursive_mutex();
}

FileSystem::~FileSystem()
{
	clear();
	delete m_preventChange;
}

void FileSystem::clear()
{
	fileList.clear();
	directoryList.clear();
}

void FileSystem::addDir(const CString& dir, const CString& wildcard, bool forceRecursive)
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

	if (server == nullptr) return;

	// Format the directory.
	CString newDir(dir);
	if (newDir[newDir.length() - 1] == '/' || newDir[newDir.length() - 1] == '\\')
		FileSystem::fixPathSeparators(newDir);
	else
	{
		newDir << fSep;
		FileSystem::fixPathSeparators(newDir);
	}

	// Add the directory to the directory list.
	CString ndir = server->getServerPath() << newDir << wildcard;
	if (vecSearch<CString>(directoryList, ndir) != -1) // Already exists?  Resync.
		resync();
	else
	{
		directoryList.push_back(ndir);

		// Load up the files in the directory.
		loadAllDirectories(ndir, forceRecursive || server->getSettings().getBool("nofoldersconfig", false));
	}
}

void FileSystem::addFile(CString file)
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

	// Grab the file name and directory.
	FileSystem::fixPathSeparators(file);
	CString filename(getFilename(file, fSep));
	CString directory(getPath(file, fSep));

	// Fix directory path separators.
	if (directory.find(server->getServerPath()) != -1)
		directory.removeI(0, server->getServerPath().length());

	// Add to the map.
	fileList[filename] = server->getServerPath() << directory << filename;
}

void FileSystem::removeFile(const CString& file)
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

	// Grab the file name and directory.
	CString filename(file.subString(file.findl(fSep) + 1));
	CString directory(file.subString(0, file.find(filename)));

	// Fix directory path separators.
	FileSystem::fixPathSeparators(directory);

	// Remove it from the map.
	fileList.erase(filename);
}

void FileSystem::resync()
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

	// Clear the file list.
	fileList.clear();

	// Iterate through all the directories, reloading their file list.
	for (const auto& directory: directoryList)
		loadAllDirectories(directory, server->getSettings().getBool("nofoldersconfig", false));
}

CString FileSystem::find(const CString& file) const
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

	auto fileIter = fileList.find(file);
	if (fileIter == fileList.end()) return {};
	return { fileIter->second };
}

CString FileSystem::findi(const CString& file) const
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

	for (const auto& fileIter: fileList)
		if (fileIter.first.comparei(file)) return { fileIter.second };
	return {};
}

CString FileSystem::fileExistsAs(const CString& file) const
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

	for (const auto& fileIter: fileList)
		if (fileIter.first.comparei(file)) return { fileIter.first };
	return {};
}

#if (defined(_WIN32) || defined(_WIN64)) && !defined(__GNUC__)
void FileSystem::loadAllDirectories(const CString& directory, bool recursive)
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
					addDir(newDir, "*", true);
				}
			}
			else
			{
				// Grab the file name.
				CString file((char*)filedata.cFileName);
				fileList[file] = CString(dir) << filedata.cFileName;
			}
		}
		while (FindNextFileA(hFind, &filedata));
	}
	FindClose(hFind);
}
#else
void FileSystem::loadAllDirectories(const CString& directory, bool recursive)
{
	CString path     = CString() << directory.remove(directory.findl(fSep)) << fSep;
	CString wildcard = directory.subString(directory.findl(fSep) + 1);
	DIR* dir;
	struct stat statx
	{
	};
	struct dirent* ent;

	// Try to open the directory.
	if ((dir = opendir(path.text())) == nullptr)
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
					addDir(newDir, "*", true);
				}
				continue;
			}
		}
		else
			continue;

		// Grab the file name.
		CString file(ent->d_name);
		if (file.match(wildcard))
			fileList[file] = CString(path) << file;
	}
	closedir(dir);
}
#endif

CString FileSystem::load(const CString& file) const
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

	// Get the full path to the file.
	CString fileName = find(file);
	if (fileName.length() == 0) return CString();

	// Load the file.
	CString fileData;
	fileData.load(fileName);

	return fileData;
}

time_t FileSystem::getModTime(const CString& file) const
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

	// Get the full path to the file.
	CString fileName = find(file);
	if (fileName.length() == 0) return 0;

	struct stat fileStat
	{
	};
	if (stat(fileName.text(), &fileStat) != -1)
		return (time_t)fileStat.st_mtime;
	return 0;
}

bool FileSystem::setModTime(const CString& file, time_t modTime) const
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

	// Get the full path to the file.
	CString fileName = find(file);
	if (fileName.length() == 0) return false;

	// Set the times.
	struct utimbuf ut
	{
	};
	ut.actime  = modTime;
	ut.modtime = modTime;

	// Change the file.
	return utime(fileName.text(), &ut) == 0;
}

int FileSystem::getFileSize(const CString& file) const
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

	// Get the full path to the file.
	CString fileName = find(file);
	if (fileName.length() == 0) return 0;

	struct stat fileStat
	{
	};
	if (stat(fileName.text(), &fileStat) != -1)
		return fileStat.st_size;
	return 0;
}

CString FileSystem::getDirByExtension(const std::string& extension) const
{
	std::lock_guard<std::recursive_mutex> lock(*m_preventChange);

	for (const auto& directory: directoryList)
	{
		if (getExtension(directory) == extension)
		{
			return { getPath(directory) };
		}
	}

	return {};
}
