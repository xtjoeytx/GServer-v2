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
#include <utilities/CommonTypes.h>
#include <utilities/Log.h>

#define BUFF_SIZE ((sizeof(struct inotify_event) + FILENAME_MAX) * 100)

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::fs::watch
{
///////////////////////////////////////////////////////////////////////////////

struct Watch
{
	int watch_id = 0;
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

uint32_t FileWatch::add(const std::filesystem::path& directory, watch_cb callback, const bool recursive)
{
	auto addWatch = [this, &callback](const std::filesystem::path& file) -> int
	{
		DEBUGPRINT("[FS] Adding watch for directory: {}", file.string());

		// Add the watch for this directory.
		int wd = inotify_add_watch(m_watch_os->fd, file.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_MOVED_FROM | IN_DELETE);

		if (wd < 0)
			return 0;

		auto watch = new Watch;
		watch->watch_id = wd;
		watch->dir = file;
		watch->callback = callback;

		m_watchers.insert(std::make_pair(wd, watch));
		return wd;
	};

	const int wd = addWatch(directory);
	if (wd == 0)
		return 0;

	if (recursive)
	{
		for (const auto& file : std::filesystem::recursive_directory_iterator(directory))
		{
			if (!file.is_directory())
				continue;

			if (addWatch(file.path()) == 0)
				return 0;
		}
	}

	return wd;
}

void FileWatch::remove(const std::filesystem::path& directory)
{
	for (auto& [wd, watch] : m_watchers)
	{
		if (const auto rel = std::filesystem::relative(watch->dir, directory); !rel.empty() && rel.string()[0] != '.')
			remove(wd);
	}
}

void FileWatch::remove(const uint32_t watch_id)
{
	const auto i = m_watchers.find(watch_id);
	if (i == m_watchers.end())
		return;

	const Watch* watch = i->second;
	m_watchers.erase(i);

	inotify_rm_watch(m_watch_os->fd, watch->watch_id);
	delete watch;
}

void FileWatch::removeAll()
{
	for (const auto& watch : m_watchers | std::views::values)
	{
		inotify_rm_watch(m_watch_os->fd, watch->watch_id);
		delete watch;
	}

	m_watchers.clear();
}

void FileWatch::update()
{
	FD_SET(m_watch_os->fd, &m_watch_os->descriptor_set);

	if (const int ret = select(m_watch_os->fd + 1, &m_watch_os->descriptor_set, nullptr, nullptr, &m_watch_os->timeout); ret < 0)
	{
		perror("select");
	}
	else if (FD_ISSET(m_watch_os->fd, &m_watch_os->descriptor_set))
	{
		ssize_t i = 0;
		std::vector<FileEventData> fileEventList;
		std::unordered_map<std::filesystem::path, size_t> fileEvents;
		char notifyBuff[BUFF_SIZE] = {0};
		char fileBuff[FILENAME_MAX + 1] = {0};

		const ssize_t len = read(m_watch_os->fd, notifyBuff, BUFF_SIZE);
		while (i < len)
		{
			const auto pevent = reinterpret_cast<struct inotify_event*>(&notifyBuff[i]);

			if (auto iter = m_watchers.find(pevent->wd); iter != m_watchers.end())
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
						std::filesystem::path fileName{pevent->name};
						auto [eventIter, inserted] = fileEvents.try_emplace(fileName.filename(), fileEventList.size());
						if (inserted)
							fileEventList.emplace_back();

						auto& data = fileEventList[eventIter->second];
						data.fsData = watch;
						data.fileName = fileName;
						if (fileBuff[0] != '\0')
						{
							data.oldFileName = std::filesystem::path{fileBuff};
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

			i += static_cast<ssize_t>(sizeof(struct inotify_event)) + pevent->len;
		}

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
			const auto watch = static_cast<Watch*>(data.fsData);
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
			// If we have an added + renamed event, clear the renamed flag.
			else if (data.events.test(FileEvent::Added) && data.events.test(FileEvent::Renamed))
			{
				data.events.reset(FileEvent::Renamed);
			}
		}

		// Execute callbacks for our queued data.
		for (auto& data : fileEventList)
		{
			if (data.events.test(FileEvent::Invalid))
				continue;

			if (const auto watch = static_cast<Watch*>(data.fsData); watch != nullptr)
				watch->callback(watch->watch_id, watch->dir, data.fileName, data.oldFileName, data.events);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::fs::watch

#endif // PLATFORM_UNIX
