#ifdef PLATFORM_UNIX

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sys/inotify.h>
#include <sys/select.h>
#include <unistd.h>

#include <filesystem/FileSystemTypes.h>
#include <filesystem/watch/FileWatch.h>

#define BUFF_SIZE ((sizeof(struct inotify_event)+FILENAME_MAX)*100)

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::fs::watch
{
///////////////////////////////////////////////////////////////////////////////

struct Watch
{
	uint32_t watch_id;
	std::filesystem::path dir;
	watch_cb callback;
};

struct WatchOS
{
	int fd;
	timeval timeout;
	fd_set descriptor_set;
};

/////////////////////////////

FileWatch::FileWatch()
	: m_last_id(0)
{
	m_watch_os = std::make_unique<WatchOS>();

	m_watch_os->fd = inotify_init();
	if (m_watch_os->fd < 0)
		fprintf(stderr, "Error: %s\n", strerror(errno));

	m_watch_os->timeout.tv_sec = 0;
	m_watch_os->timeout.tv_usec = 0;

	FD_ZERO(&m_watch_os->descriptor_set);
}

FileWatch::~FileWatch()
{
	removeAll();
}

uint32_t FileWatch::add(const std::filesystem::path& directory, watch_cb callback, bool recursive)
{
	int wd = inotify_add_watch(m_watch_os->fd, directory.c_str(),
		IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_MOVED_FROM | IN_DELETE);

	if (wd < 0)
		return 0;

	Watch* watch = new Watch;
	watch->watch_id = wd;
	watch->dir = directory;
	watch->callback = callback;

	m_watchers.insert(std::make_pair(wd, watch));
	return wd;
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

	inotify_rm_watch(m_watch_os->fd, watch->watch_id);
	delete watch;
}

void FileWatch::removeAll()
{
	for (auto& w : m_watchers)
	{
		inotify_rm_watch(m_watch_os->fd, w.second->watch_id);
		delete w.second;
	}

	m_watchers.clear();
}

void FileWatch::update()
{
	FD_SET(m_watch_os->fd, &m_watch_os->descriptor_set);

	int ret = select(m_watch_os->fd + 1, &m_watch_os->descriptor_set, NULL, NULL, &m_watch_os->timeout);
	if (ret < 0)
	{
		perror("select");
	}
	else if (FD_ISSET(m_watch_os->fd, &m_watch_os->descriptor_set))
	{
		ssize_t len, i = 0;
		std::unordered_map<std::filesystem::path, FileEventData> fileEvents;
		char notifyBuff[BUFF_SIZE] = { 0 };
		char fileBuff[FILENAME_MAX + 1] = { 0 };

		len = read(m_watch_os->fd, notifyBuff, BUFF_SIZE);

		while (i < len)
		{
			struct inotify_event* pevent = (struct inotify_event*)&notifyBuff[i];

			auto iter = m_watchers.find(pevent->wd);
			if (iter != m_watchers.end())
			{
				Watch* watch = iter->second;
				if (pevent->name[0] != '.')
				{
					if (IN_MOVED_FROM & pevent->mask)
					{
						std::strncpy(fileBuff, pevent->name, FILENAME_MAX);
						fileBuff[FILENAME_MAX] = '\0';
					}
					else
					{
						std::filesystem::path fileName{ pevent->name };

						auto& data = fileEvents[fileName.filename()];
						data.fsData = watch;
						data.fileName = fileName;
						if (fileBuff[0] != '\0')
						{
							data.oldFileName = std::filesystem::path{ fileBuff };
							fileBuff[0] = '\0';
						}

						if (IN_CLOSE_WRITE & pevent->mask)
							data.events.set(FileEvent::Modified);
						if (IN_CREATE & pevent->mask)
							data.events.set(FileEvent::Added);
						if (IN_DELETE & pevent->mask)
							data.events.set(FileEvent::Deleted);
						if (IN_MOVED_TO & pevent->mask)
							data.events.set(FileEvent::Renamed);
					}
				}
			}

			i += sizeof(struct inotify_event) + pevent->len;
		}

		// Check for rename events on overwritten files.
		// When we save files, a temp file gets created and renamed over top of the original file.
		// The temp file will get an Added event, and the original file will get a Deleted event + a Renamed event.
		// We want the original file to have a single Modified event, while the original file should get a Deleted event.
		for (auto& [file, data] : fileEvents)
		{
			auto* watch = (Watch*)data.fsData;
			if (watch == nullptr) continue;

			// If we have both an added and deleted event, test if the file exists and only set the appropriate one.
			if (data.events.test(FileEvent::Added) && data.events.test(FileEvent::Deleted))
			{
				if (std::filesystem::exists(watch->dir / data.oldFileName))
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
		for (auto& [file, data] : fileEvents)
		{
			if (data.events.test(FileEvent::Invalid))
				continue;

			if (auto* watch = (Watch*)data.fsData; watch != nullptr)
				watch->callback(watch->watch_id, watch->dir, data.fileName, data.oldFileName, data.events);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::fs::watch

#endif // PLATFORM_UNIX
