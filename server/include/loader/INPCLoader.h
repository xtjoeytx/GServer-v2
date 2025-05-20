#ifndef INPCLOADER_H
#define INPCLOADER_H

#include <string>
#include <string_view>
#include <vector>

#include <object/NPC.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

class INPCLoader
{
public:
	virtual NPCPtr loadNPC(std::string_view npcName) noexcept = 0;
	virtual NPCPtr loadNPC(const std::filesystem::path& filePath) noexcept = 0;
	virtual bool saveNPC(NPCPtr npc) noexcept = 0;
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal

#endif // INPCLOADER_H
