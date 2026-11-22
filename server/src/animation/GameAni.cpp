#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <CString.h>
#include <IEnums.h>

#include <Server.h>
#include <animation/GameAni.h>
#include <filesystem/FileSystemTypes.h>
#include <npcserver/NPCServer.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

std::optional<GameAni> GameAni::load(Server* const server, const std::filesystem::path& name)
{
	const auto& fileSystem = server->getFileSystem();

	// Search for the file in the filesystem
	const auto fileInfo = fileSystem.infoi(fs::FileCategory::FILE, name);
	if (fileInfo == nullptr)
		return std::nullopt;

	const auto fileData = fileInfo->openFile();
	if (fileData == nullptr || !fileData->opened())
		return std::nullopt;

	// Check for the header.
	// GANI0001 = normal
	// GANI0002 = movie
	const auto header = fileData->readLine();
	if (header != "GANI0001" && header != "GANI0002")
		return std::nullopt;

	GameAni gani(fs::getANSIFileName(name));

	std::string curLine;
	while (!fileData->finishedReading())
	{
		// Read the line.
		fileData->readLine(curLine);
		std::string_view line{string::trim(curLine)};
		if (line.empty())
			continue;

		// Get the line data.
		const auto [section, data] = string::extractConfigParts(line);

		// Parse each line.
		if (section == "SPRITE")
		{
			// SPRITE index sheet x y width height description
			// sheet = image or GaniSheets preset
		}
		else if (section == "CONTINUOUS")
		{
			gani.m_aniFlags.set(ENUM(GaniFlags::CONTINUOUS), data.empty() || !DoubleIsZero(string::toFloat(data)));
		}
		else if (section == "LOOP")
		{
			gani.m_aniFlags.set(ENUM(GaniFlags::LOOP), data.empty() || !DoubleIsZero(string::toFloat(data)));
		}
		else if (section == "SINGLEDIRECTION")
		{
			gani.m_aniFlags.set(ENUM(GaniFlags::SINGLEDIRECTION), data.empty() || !DoubleIsZero(string::toFloat(data)));
		}
		else if (section == "SETBACKTO")
		{
			gani.m_setBackTo = string::trim(data);
		}
		else if (section.starts_with("DEFAULT"))
		{
			// DEFAULTHEAD
			// DEFAULTBODY
			// DEFAULTATTR#
			// DEFAULTPARAM#
		}
		else if (section == "SCRIPT")
		{
			gani.setScript(string::join(fileData->readLinesUntilSectionEnd("SCRIPTEND"), "\n"sv));
		}
		else if (section == "ANI")
		{
			// Animation data.
			fileData->skipUntil("ANIEND");

			// WAIT
			// PLAYSOUND
		}
		else if (section == "REDBODY")
		{
			// New World, swimming in lava.
		}
		else if (section == "BLUEBODY")
		{
			// New World, swimming in water.
		}
		else if (section == "COLOREFFECT")
		{
			// COLOREFFECT sprite r g b a
		}
		else if (section == "ZOOMEFFECT")
		{
			// ZOOMEFFECT sprite zoom
		}
		else if (section == "ATTACHSPRITE")
		{
			// ATTACHSPRITE parent-sprite child-sprite dx dy
		}
		else if (section == "EFFECTMODE")
		{
			// EFFECTMODE sprite mode
			// mode: 0 - lights, 1 - transparency, 2 - holes.
		}
		else if (section == "ROTATEPROJECTILE")
		{
			// ROTATEPROJECTILE
			// Make a SINGLEDIRECTION gani with a projectile facing to the right and the game will automatically rotate it when used with shoot.
		}
		else if (section == "ROTATEEFFECT")
		{
			// ROTATEEFFECT sprite angle
		}
		else if (section == "STRETCHXEFFECT")
		{
			// STRETCHXEFFECT sprite factor (default 1)
		}
		else if (section == "STRETCHYEFFECT")
		{
			// STRETCHYEFFECT sprite factor (default 1)
		}
	}

	return gani;
}

///////////////////////////////////////////////////////////////////////////////

GameAni::GameAni(const std::string_view gani)
	: name{gani}
{
}

GameAni& GameAni::setScript(const std::string_view script)
{
	if (const auto server = BabyDI::Get<Server>(); server && server->hasNPCServer())
		m_client_script = server->getNPCServer()->scripting.getCompiledClientScript(name, script);
	return *this;
}

void GameAni::sendByteCodeToPlayer(const std::shared_ptr<Player>& player) const
{
	std::string_view gani = name;
	if (gani.ends_with(".gani"))
		gani = gani.substr(0, gani.length() - 5);

	// Send the bytecode.
	if (const auto& bytecode = m_script.getClientSideByteCode(); !bytecode.empty())
	{
		const auto bytecodePtr = reinterpret_cast<const char*>(bytecode.data());
		const std::string_view bytecodeView(bytecodePtr, bytecode.size());

		player->sendRawPacket(CString() >> (char)PLO_GANIBYTECODE >> (char)gani.length() << gani << bytecodeView);
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
