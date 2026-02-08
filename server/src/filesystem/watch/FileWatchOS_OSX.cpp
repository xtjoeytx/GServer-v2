#ifdef PLATFORM_APPLE

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>

#include <filesystem>
#include <unordered_map>
#include <vector>

#include <filesystem/FileSystemTypes.h>
#include <filesystem/watch/FileWatch.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::fs::watch
{
///////////////////////////////////////////////////////////////////////////////

struct FileSnapshot
{
	std::filesystem::file_time_type mtime;
};

struct Watch
{
	uint32_t watch_id;
	std::filesystem::path dir;
	int fd;
	watch_cb callback;
	std::unordered_map<std::string, FileSnapshot> snapshot;
};

static std::unordered_map<std::string, FileSnapshot> takeSnapshot(const std::filesystem::path& dir)
{
	std::unordered_map<std::string, FileSnapshot> snap;
	std::error_code ec;
	for (auto& entry : std::filesystem::directory_iterator(dir, ec))
	{
		if (!entry.is_regular_file(ec))
			continue;

		auto filename = entry.path().filename().string();
		if (filename.empty() || filename[0] == '.')
			continue;

		auto mtime = entry.last_write_time(ec);
		if (!ec)
			snap[filename] = { mtime };
	}
	return snap;
}

struct WatchOS
{
	int kq;
};

/////////////////////////////

FileWatch::FileWatch()
	: m_last_id(0)
{
	m_watch_os = std::make_unique<WatchOS>();

	m_watch_os->kq = kqueue();
	if (m_watch_os->kq < 0)
		fprintf(stderr, "FileWatch: kqueue() failed: %s\n", strerror(errno));
}

FileWatch::~FileWatch()
{
	removeAll();
	if (m_watch_os->kq >= 0)
		close(m_watch_os->kq);
}

uint32_t FileWatch::add(const std::filesystem::path& directory, watch_cb callback, bool recursive)
{
	int fd = open(directory.c_str(), O_RDONLY | O_EVTONLY);
	if (fd < 0)
		return 0;

	struct kevent change;
	EV_SET(&change, fd, EVFILT_VNODE, EV_ADD | EV_CLEAR,
		NOTE_WRITE | NOTE_DELETE | NOTE_RENAME | NOTE_ATTRIB, 0, nullptr);

	if (kevent(m_watch_os->kq, &change, 1, nullptr, 0, nullptr) < 0)
	{
		close(fd);
		return 0;
	}

	uint32_t id = ++m_last_id;

	Watch* watch = new Watch;
	watch->watch_id = id;
	watch->dir = directory;
	watch->fd = fd;
	watch->callback = callback;

	watch->snapshot = takeSnapshot(directory);

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

	close(watch->fd);
	delete watch;
}

void FileWatch::removeAll()
{
	for (auto& w : m_watchers)
	{
		close(w.second->fd);
		delete w.second;
	}

	m_watchers.clear();
}

void FileWatch::update()
{
	struct timespec timeout = { 0, 0 };
	struct kevent events[16];

	int nev = kevent(m_watch_os->kq, nullptr, 0, events, 16, &timeout);
	if (nev <= 0)
		return;

	for (int i = 0; i < nev; ++i)
	{
		int fd = static_cast<int>(events[i].ident);

		// Find the watch associated with this fd.
		Watch* watch = nullptr;
		for (auto& w : m_watchers)
		{
			if (w.second->fd == fd)
			{
				watch = w.second;
				break;
			}
		}

		if (watch == nullptr)
			continue;

		auto newSnapshot = takeSnapshot(watch->dir);
		auto& oldSnapshot = watch->snapshot;

		std::unordered_map<std::filesystem::path, FileEventData> fileEvents;

		// Check for deleted and modified files.
		for (auto& [name, oldEntry] : oldSnapshot)
		{
			auto it = newSnapshot.find(name);
			if (it == newSnapshot.end())
			{
				auto& data = fileEvents[name];
				data.fileName = name;
				data.events.set(FileEvent::Deleted);
			}
			else if (it->second.mtime != oldEntry.mtime)
			{
				auto& data = fileEvents[name];
				data.fileName = name;
				data.events.set(FileEvent::Modified);
			}
		}

		// Check for added files.
		for (auto& [name, newEntry] : newSnapshot)
		{
			if (oldSnapshot.find(name) == oldSnapshot.end())
			{
				auto& data = fileEvents[name];
				data.fileName = name;
				data.events.set(FileEvent::Added);
			}
		}

		// Detect renames: a deleted + added pair with the same mtime.
		std::vector<std::string> deleted, added;
		for (auto& [file, data] : fileEvents)
		{
			if (data.events.test(FileEvent::Deleted))
				deleted.push_back(file);
			if (data.events.test(FileEvent::Added))
				added.push_back(file);
		}

		for (auto& del : deleted)
		{
			if (added.empty())
				break;

			auto oldIt = oldSnapshot.find(del);
			if (oldIt == oldSnapshot.end())
				continue;

			for (auto addIt = added.begin(); addIt != added.end(); ++addIt)
			{
				auto newIt = newSnapshot.find(*addIt);
				if (newIt == newSnapshot.end())
					continue;

				if (oldIt->second.mtime == newIt->second.mtime)
				{
					auto& data = fileEvents[*addIt];
					data.oldFileName = del;
					data.events.set(FileEvent::Renamed);
					data.events.reset(FileEvent::Added);
					fileEvents.erase(del);
					added.erase(addIt);
					break;
				}
			}
		}

		oldSnapshot = std::move(newSnapshot);

		for (auto& [file, data] : fileEvents)
		{
			if (data.events.test(FileEvent::Invalid))
				continue;
			watch->callback(watch->watch_id, watch->dir, data.fileName, data.oldFileName, data.events);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::fs::watch

#endif // PLATFORM_APPLE
