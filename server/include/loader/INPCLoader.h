#ifndef INPCLOADER_H
#define INPCLOADER_H

#include <filesystem>
#include <string_view>

#include <object/NPC.h>
#include <filesystem/File.h>
#include <utilities/CommonTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class INPCLoader
{
public:
	virtual ~INPCLoader() = default;

public:
	virtual NPCPtr loadNPC(std::string_view npcName) noexcept = 0;
	virtual NPCPtr loadNPC(const std::filesystem::path& filePath) noexcept = 0;
	virtual void loadNPC(fs::File& file, NPCPtr& npc, const clock::time_point& updateTime) noexcept = 0;
	virtual bool saveNPC(NPCPtr npc) noexcept = 0;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // INPCLOADER_H
