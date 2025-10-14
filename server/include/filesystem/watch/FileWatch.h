#ifndef FILEWATCH_H
#define FILEWATCH_H

#include <cstdint>
#include <filesystem>
#include <functional>
#include <unordered_map>
#include <memory>

#include <filesystem/FileSystemTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal::fs::watch
{
///////////////////////////////////////////////////////////////////////////////

// id, dir, file, oldFile, events
using watch_cb = std::function<void(uint32_t, const std::filesystem::path&, const std::filesystem::path&, const std::filesystem::path&, FileEventCollection)>;

struct Watch;
struct WatchOS;
class FileWatch
{
public:
	FileWatch();
	~FileWatch();

	FileWatch(const FileWatch& other) = delete;
	FileWatch(FileWatch&& other) = delete;
	FileWatch& operator=(const FileWatch& other) = delete;
	FileWatch& operator=(FileWatch&& other) = delete;

public:
	uint32_t add(const std::filesystem::path& directory, watch_cb callback, bool recursive = true);
	void remove(const std::filesystem::path& directory);
	void remove(uint32_t watch_id);
	void removeAll();

public:
	void update();

private:
	std::unordered_map<uint32_t, Watch*> m_watchers;
	std::unique_ptr<WatchOS> m_watch_os;
	uint32_t m_last_id;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal::fs::watch

#endif // FILEWATCH_H
