#ifndef FLATFILENPCLOADER_H
#define FLATFILENPCLOADER_H

#include <string_view>
#include <filesystem>

#include <filesystem/File.h>
#include <loader/INPCLoader.h>
#include <object/NPC.h>
#include <utilities/CommonTypes.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class FlatFileNPCLoader : public INPCLoader
{
public:
	NPCPtr loadNPC(std::string_view npcName) noexcept override;
	NPCPtr loadNPC(const std::filesystem::path& filePath) noexcept override;
	void loadNPC(fs::File& file, NPCPtr& npc, const clock::time_point& updateTime) noexcept override;
	bool saveNPC(NPCPtr npc) noexcept override;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // FLATFILENPCLOADER_H
