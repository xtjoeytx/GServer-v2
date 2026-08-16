#ifdef PLATFORM_WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
	#define NOMINMAX
#endif

#include <strsafe.h>
#include <windows.h>

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
#include <unordered_map>
#include <utility>
#include <vector>

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
	}

	OVERLAPPED overlapped;
	HANDLE dir_handle;
	BYTE buffer[32 * 1024];
	DWORD notify_filter;

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

static FileEventCollection translateAction(FileEventCollection& event, DWORD action)
{
	switch (action)
	{
		case FILE_ACTION_ADDED:
			event.set(FileEvent::Added);
			break;

		case FILE_ACTION_REMOVED:
		case FILE_ACTION_RENAMED_OLD_NAME:
			event.set(FileEvent::Deleted);
			break;

		case FILE_ACTION_MODIFIED:
			event.set(FileEvent::Modified);
			break;

		case FILE_ACTION_RENAMED_NEW_NAME:
			event.set(FileEvent::Renamed);
			break;

		default:
			event.set(FileEvent::Invalid);
			break;
	}
	return event;
}

static void CALLBACK watchCallback(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
	thread_local TCHAR fileBuffer[Watch::FileBufferLength];
	thread_local TCHAR oldFileBuffer[Watch::FileBufferLength];

	// Maximum path length on Windows is 32767.
	PFILE_NOTIFY_INFORMATION pNotify;
	Watch* pWatch = (Watch*)lpOverlapped;
	size_t offset = 0;

	if (pWatch == nullptr || pWatch->callback == nullptr || dwNumberOfBytesTransfered == 0)
		return;

	if (dwErrorCode == ERROR_SUCCESS)
	{
		std::vector<FileEventData> fileEventList;
		std::unordered_map<std::filesystem::path, size_t> fileEvents;
		fileBuffer[0] = TEXT('\0');
		oldFileBuffer[0] = TEXT('\0');

		do
		{
			pNotify = (PFILE_NOTIFY_INFORMATION)&pWatch->buffer[offset];
			offset += pNotify->NextEntryOffset;

			// Set our filename buffer.
			PTCHAR targetBuffer = fileBuffer;
			if (pNotify->Action == FILE_ACTION_RENAMED_OLD_NAME)
				targetBuffer = oldFileBuffer;

			*targetBuffer = TEXT('\0');

#if defined(UNICODE)
			{
				size_t fileNameCharacters = pNotify->FileNameLength / sizeof(WCHAR);
				(void)StringCchCopyNW(targetBuffer, Watch::FileBufferLength, pNotify->FileName, fileNameCharacters);
			}
#else
			{
				int count = WideCharToMultiByte(CP_ACP, 0, pNotify->FileName, pNotify->FileNameLength / sizeof(WCHAR), targetBuffer, Watch::FileBufferLength - 1, NULL, NULL);
				targetBuffer[count] = TEXT('\0');
			}
#endif

			// If this is a rename event, and it is the old file, continue to the next entry.
			// The old file was stored above.
			if (pNotify->Action == FILE_ACTION_RENAMED_OLD_NAME)
				continue;

			std::filesystem::path fileName{fileBuffer};
			auto [eventIter, inserted] = fileEvents.try_emplace(fileName.filename(), fileEventList.size());
			if (inserted)
				fileEventList.emplace_back();

			auto& data = fileEventList[eventIter->second];

			// Save information to the queued data.
			translateAction(data.events, pNotify->Action);
			data.fileName = fileName;
			if (oldFileBuffer[0] != TEXT('\0'))
				data.oldFileName = std::filesystem::path{oldFileBuffer};
		}
		while (pNotify->NextEntryOffset != 0);

		// Fix events on temporary files.
		// Save:
		//   When we save files, a temp file gets created and renamed over top of the original file.
		//   The temp file will get an Added event, and the original file will get a Deleted event + a Renamed event.
		//   We want the original file to have a single Modified event, while the original file should get a Deleted event.
		// Add:
		//   When we add files, a temp file gets created and renamed over top of the original file.
		//   We want just the non-temp file to get the Added event.
		for (auto& data : fileEventList)
		{
			if (data.events.test(FileEvent::Renamed))
			{
				// Make sure we are only getting a rename event.
				data.events.reset();
				data.events.set(FileEvent::Renamed);

				// If the old file exists in our list, ignore it.
				// The filesystem logic will take care of removing it.
				auto oldFileIter = fileEvents.find(data.oldFileName);
				if (oldFileIter != fileEvents.end())
				{
					auto& oldFileData = fileEventList[oldFileIter->second];
					oldFileData.events.set(FileEvent::Invalid);
				}
			}
			// If we have both an added and deleted event, test if the file exists and only set the appropriate one.
			else if (data.events.test(FileEvent::Added) && data.events.test(FileEvent::Deleted))
			{
				if (std::filesystem::exists(pWatch->dir / data.oldFileName))
					data.events.reset(FileEvent::Deleted);
				else
					data.events.reset(FileEvent::Added);
			}
			// If we have an added + modified event, clear the modified.
			else if (data.events.test(FileEvent::Added) && data.events.test(FileEvent::Modified))
			{
				data.events.reset(FileEvent::Modified);
			}
		}

		// Execute callbacks for our queued data.
		for (auto& data : fileEventList)
		{
			if (data.events.test(FileEvent::Invalid))
				continue;

			pWatch->callback(pWatch->watch_id, pWatch->dir, data.fileName, data.oldFileName, data.events);
		}
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
		   watch->notify_filter, NULL, &watch->overlapped, clear ? 0 : watchCallback
		   ) != 0;
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
	// FILE_FLAG_BACKUP_SEMANTICS lets us actually create a directory handle to get file change events.
	// FILE_FLAG_OVERLAPPED lets us do async IO.
	watch->dir_handle = CreateFile(directory.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
	if (watch->dir_handle == INVALID_HANDLE_VALUE)
	{
		delete watch;
		return 0;
	}

	// Create our event.
	// This is the important part that lets us do async IO.
	// The callback was registered with ReadDirectoryChangesW(), and this event is passed in the overlapped data.
	// That means that ReadDirectoryChangesW() will buffer events until the event gets signaled.
	// The signal is kicked off by the MsgWaitForMultipleObjectsEx() function.
	watch->overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	watch->notify_filter = FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_FILE_NAME;
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
	// Count is 0, so it waits only for an input event.
	// No handles.
	// 0ms timeout, which means this function will not block.
	// QS_ALLINPUT means we want to wake up for all input events.
	// Wait only for alertable input events.
	MsgWaitForMultipleObjectsEx(0, NULL, 0, QS_ALLINPUT, MWMO_ALERTABLE);
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::fs::watch

#endif // PLATFORM_WINDOWS
