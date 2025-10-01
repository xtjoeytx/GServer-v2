#ifdef PLATFORM_WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
	#define NOMINMAX
#endif

#include <windows.h>
#include <strsafe.h>

/*
#if defined(_MSC_VER)
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ole32.lib")
#pragma warning (disable: 4996)
#endif
*/

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <limits>
#include <utility>

#include <filesystem/FileSystemTypes.h>
#include <filesystem/watch/FileWatch.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::fs::watch
{
///////////////////////////////////////////////////////////////////////////////

struct Watch
{
	static constexpr size_t FileBufferLength = std::numeric_limits<int16_t>::max() + 1;

	Watch()
		: dir_handle(0), notify_filter(0), watch_id(0), recursive(false), callback(nullptr), stop(false)
	{
		memset(&overlapped, 0, sizeof(OVERLAPPED));
		buffer[0] = '\0';
		fileBuffer[0] = L'\0';
	}

	OVERLAPPED overlapped;
	HANDLE dir_handle;
	BYTE buffer[32 * 1024];
	DWORD notify_filter;
	TCHAR fileBuffer[FileBufferLength];

	uint32_t watch_id;
	std::filesystem::path dir;
	bool recursive;
	watch_cb callback;

	std::atomic<bool> stop;

};

struct WatchOS
{
};

/////////////////////////////

static bool refreshWatch(Watch* watch, bool clear = false);
static void deleteWatch(Watch* watch);

static FileEventCollection translateAction(DWORD action)
{
	FileEventCollection event;
	switch (action)
	{
		case FILE_ACTION_RENAMED_NEW_NAME:
		case FILE_ACTION_ADDED:
			event.set(FileEvent::Added);
			break;

		case FILE_ACTION_RENAMED_OLD_NAME:
		case FILE_ACTION_REMOVED:
			event.set(FileEvent::Deleted);
			break;

		case FILE_ACTION_MODIFIED:
			event.set(FileEvent::Modified);
			break;

		default:
			event.set(FileEvent::Invalid);
			break;
	}
	return event;
}

static void CALLBACK watchCallback(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
	// Maximum path length on Windows is 32767.
	PFILE_NOTIFY_INFORMATION pNotify;
	Watch* pWatch = (Watch*)lpOverlapped;
	size_t offset = 0;

	if (pWatch == nullptr || dwNumberOfBytesTransfered == 0)
		return;

	if (dwErrorCode == ERROR_SUCCESS)
	{
		do
		{
			pNotify = (PFILE_NOTIFY_INFORMATION)&pWatch->buffer[offset];
			offset += pNotify->NextEntryOffset;

	#if defined(UNICODE)
			{
				size_t fileNameCharacters = pNotify->FileNameLength / sizeof(WCHAR);
				(void)StringCchCopyNW(pWatch->fileBuffer, Watch::FileBufferLength, pNotify->FileName, fileNameCharacters);
			}
	#else
			{
				int count = WideCharToMultiByte(CP_ACP, 0, pNotify->FileName,
												pNotify->FileNameLength / sizeof(WCHAR),
												pWatch->fileBuffer, Watch::FileBufferLength - 1, NULL, NULL);
				pWatch->fileBuffer[count] = TEXT('\0');
			}
	#endif

			if (pWatch->callback)
				pWatch->callback(pWatch->watch_id, pWatch->dir, std::filesystem::path{ pWatch->fileBuffer }, translateAction(pNotify->Action));
		}
		while (pNotify->NextEntryOffset != 0);
	}

	if (!pWatch->stop)
		refreshWatch(pWatch);
	else
		deleteWatch(pWatch);
}

bool refreshWatch(Watch* watch, bool clear)
{
	return ReadDirectoryChangesW(
			   watch->dir_handle, watch->buffer, sizeof(watch->buffer), watch->recursive,
			   watch->notify_filter, NULL, &watch->overlapped, clear ? 0 : watchCallback) != 0;
}

void deleteWatch(Watch* watch)
{
	CloseHandle(watch->overlapped.hEvent);
	CloseHandle(watch->dir_handle);

	delete watch;
}

/////////////////////////////

FileWatch::FileWatch()
	: m_last_id(0)
{
}

FileWatch::~FileWatch()
{
	for (auto& w : m_watchers)
	{
		Watch* watch = w.second;

		CancelIo(watch->dir_handle);
		refreshWatch(watch, true);

		if (!HasOverlappedIoCompleted(&watch->overlapped))
			SleepEx(5, TRUE);

		deleteWatch(watch);
	}
}

uint32_t FileWatch::add(const std::filesystem::path& directory, watch_cb callback, bool recursive)
{
	Watch* watch = new Watch;

	// Create the directory handle.
	watch->dir_handle = CreateFile(directory.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
	if (watch->dir_handle == INVALID_HANDLE_VALUE)
	{
		delete watch;
		return 0;
	}

	watch->overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	watch->notify_filter = FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_FILE_NAME;
	watch->recursive = recursive;

	uint32_t id = ++m_last_id;

	watch->watch_id = id;
	watch->dir = directory;
	watch->callback = callback;

	if (!refreshWatch(watch))
	{
		deleteWatch(watch);
		return 0;
	}

	m_watchers.insert(std::make_pair(id, watch));
	return id;
}

void FileWatch::remove(const std::filesystem::path& directory)
{
	for (auto& w : m_watchers)
	{
		if (directory == w.second->dir)
		{
			remove(w.first);
			return;
		}
	}
}

void FileWatch::remove(uint32_t watch_id)
{
	auto i = m_watchers.find(watch_id);
	if (i == m_watchers.end())
		return;

	Watch* watch = i->second;
	m_watchers.erase(i);

	CancelIo(watch->dir_handle);
	refreshWatch(watch);

	if (HasOverlappedIoCompleted(&watch->overlapped))
	{
		deleteWatch(watch);
	}
}

void FileWatch::removeAll()
{
	for (auto& w : m_watchers)
	{
		Watch* watch = w.second;

		CancelIo(watch->dir_handle);
		refreshWatch(watch);

		if (HasOverlappedIoCompleted(&watch->overlapped))
		{
			deleteWatch(watch);
		}
	}

	m_watchers.clear();
}

void FileWatch::update()
{
	MsgWaitForMultipleObjectsEx(0, NULL, 0, QS_ALLINPUT, MWMO_ALERTABLE);
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::fs::watch

#endif // PLATFORM_WINDOWS
