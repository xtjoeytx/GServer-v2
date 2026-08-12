#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <CString.h>
#include <IEnums.h>

#include <Account.h>
#include <Server.h>
#include <filesystem/File.h>
#include <filesystem/FileSystem.h>
#include <filesystem/FileSystemTypes.h>
#include <level/Level.h>
#include <network/IPacketHandler.h>
#include <npcserver/NPCServer.h>
#include <object/Player.h>
#include <player/PlayerClient.h>
#include <player/PlayerRC.h>
#include <scripting/ScriptContainers.h>
#include <utilities/CommonTypes.h>
#include <utilities/Extents.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

HandlePacketResult PlayerRC::msgPLI_RC_SERVEROPTIONSGET(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to view the server options.", account.name);
		return HandlePacketResult::Handled;
	}

	const auto settings = m_server->getFileSystemServer().openi(fs::FileCategory::CONFIG, "serveroptions.txt");
	auto options = string::toCSV(settings->readAllLines());

	// RC will automatically add a newline after the last line, so remove the newline if it exists to prevent an extra blank line from showing up in RC.
	if (options.back() == ',')
		options.pop_back();

	sendPacket(CString() >> (char)PLO_RC_SERVEROPTIONSGET << options);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_SERVEROPTIONSSET(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_SETSERVEROPTIONS))
	{
		if (isClient())
			log::printLine(log::rc, "[Hack] {} attempted to set the server options.", account.name);
		else
			log::printLine(log::rc, "{} attempted to set the server options.", account.name);

		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " is not authorized to change the server options.");
		return HandlePacketResult::Handled;
	}

	const auto& settings = m_server->getSettings();
	CString options = pPacket.readString("");

	// RC will trim the end of the string, so if the last character is not a comma, add it back in.
	if (options[options.length() - 1] != ',')
		options << ",";

	options.guntokenizeI();

	// If they don't have the modify staff account right, prevent them from changing admin-only options.
	if (!account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		std::vector<CString> newOptions = options.tokenize("\n");
		options.clear();
		for (auto& newOption : newOptions)
		{
			CString name = newOption.subString(0, newOption.find("="));
			name.trimI();

			// See if this command is an admin command.
			bool isAdmin = false;
			for (const auto& j : AdminServerOptions)
				if (name == j) isAdmin = true;

			// If it is an admin command, replace it with the current value.
			if (isAdmin)
				newOption = CString() << name << " = " << settings.get(name).value_or("");

			// Add this line back into options.
			options << newOption << "\n";
		}
	}

	// Save settings.
	if (const auto file = m_server->getFileSystemServer().openiForWriting(fs::FileCategory::CONFIG, "serveroptions.txt"); file != nullptr)
	{
		file->clear();
		file->write(options.toStringView());
	}

	// Reload settings.
	log::printLine(log::rc, "{} has updated the server options.", account.name);

	// Send RC Information
	const CString outPacket = CString() >> (char)PLO_RC_CHAT << account.name << " has updated the server options.";
	for (auto& player : m_server->getPlayerList() | std::views::values)
	{
		if (player->getType() & PLTYPE_ANYRC)
		{
			player->sendPacket(outPacket);

			// Send the NC address information.
			if (m_server->hasNPCServer())
				m_server->getNPCServer()->sendNCLoginToPlayer(player);
		}
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FOLDERCONFIGGET(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to get the folder config.", account.name);
		return HandlePacketResult::Handled;
	}

	if (const auto file = m_server->getFileSystemServer().open(fs::FileCategory::CONFIG, "foldersconfig.txt"); file != nullptr)
	{
		const auto foldersConfig = string::toCSV(file->readAllLines());
		sendPacket(CString() >> (char)PLO_RC_FOLDERCONFIGGET << foldersConfig);
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FOLDERCONFIGSET(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_SETFOLDEROPTIONS))
	{
		if (isClient())
			log::printLine(log::rc, "[Hack] {} attempted to set the folder config.", account.name);
		else
			log::printLine(log::rc, "{} attempted to set the folder config.", account.name);

		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " is not authorized to change the folder config.");
		return HandlePacketResult::Handled;
	}

	if (const auto file = m_server->getFileSystemServer().openForWriting(fs::FileCategory::CONFIG, "foldersconfig.txt"); file != nullptr)
	{
		file->clear();
		const CString folders = pPacket.readString("");
		file->writeLines(string::fromCSV(folders.toStringView()));
	}

	log::printLine(log::rc, "{} updated the folder config.", account.name);
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " updated the folder config.");
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_RESPAWNSET(CString& pPacket)
{
	// Deprecated
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_HORSELIFESET(CString& pPacket)
{
	// Deprecated
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_APINCREMENTSET(CString& pPacket)
{
	// Deprecated
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_BADDYRESPAWNSET(CString& pPacket)
{
	// Deprecated
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERPROPSGET(CString& pPacket)
{
	// Deprecated
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERPROPSSET(CString& pPacket)
{
	// Deprecated?

	const auto p = m_server->getPlayer(pPacket.readGUShort(), PLTYPE_ANYPLAYER);
	if (p == nullptr) return HandlePacketResult::Handled;

	if (isClient() || (p->account.name != account.name && !account.hasRight(PLPERM_SETATTRIBUTES)) || (p->account.name == account.name && !account.hasRight(PLPERM_SETSELFATTRIBUTES)))
	{
		if (isClient())
			log::printLine(log::rc, "[Hack] {} attempted to set a player's properties.", account.name);
		else
			log::printLine(log::rc, "{} attempted to set a player's properties.", account.name);

		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " is not authorized to set the properties of " << p->account.name);
		return HandlePacketResult::Handled;
	}

	p->setPropsFromRCPacket(pPacket, this);
	m_server->getAccountLoader().saveAccount(p->account);

	log::printLine(log::rc, "{} set the attributes of player {}", account.name, p->account.name);
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " set the attributes of player " << p->account.name);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_DISCONNECTPLAYER(CString& pPacket)
{
	const auto p = m_server->getPlayer(pPacket.readGUShort(), PLTYPE_ANYPLAYER);
	if (p == nullptr) return HandlePacketResult::Handled;

	if (isClient() || !account.hasRight(PLPERM_DISCONNECT))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to disconnect {}.", account.name, p->account.name);
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " is not authorized to disconnect players.");
		return HandlePacketResult::Handled;
	}

	CString reason = pPacket.readString("");
	if (!reason.isEmpty())
		log::printLine(log::rc, "{} disconnected {}: {}", account.name, p->account.name, reason.text());
	else
		log::printLine(log::rc, "{} disconnected {}.", account.name, p->account.name);

	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " disconnected " << p->account.name);

	CString disconnectMessage = CString() << "One of the server administrators, " << account.name << ", has disconnected you";
	if (!reason.isEmpty())
		disconnectMessage << " for the following reason: " << reason;
	else
		disconnectMessage << ".";

	p->sendPacket(CString() >> (char)PLO_DISCMESSAGE << disconnectMessage);
	m_server->deletePlayer(p);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_UPDATELEVELS(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_UPDATELEVEL))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to update levels.", account.name);
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " is not authorized to update levels.");
		return HandlePacketResult::Handled;
	}

	const unsigned short levelCount = pPacket.readGUShort();
	for (int i = 0; i < levelCount; ++i)
	{
		auto levelName = pPacket.readChars(pPacket.readGUChar()).toString();
		auto level = m_server->getLoadedLevelNoHint(levelName);
		if (level) (void)level->reload(levelName);
	}
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_ADMINMESSAGE(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_ADMINMSG))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to send an admin message.", account.name);
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to send an admin message.");
		return HandlePacketResult::Handled;
	}

	m_server->sendPacketToAll(CString() >> (char)PLO_RC_ADMINMESSAGE << "Admin " << account.name << ":\xa7" << pPacket.readString(""), {m_id});
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PRIVADMINMESSAGE(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_ADMINMSG))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to send an admin message.", account.name);
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to send an admin message.");
		return HandlePacketResult::Handled;
	}

	const auto p = m_server->getPlayer(pPacket.readGUShort(), PLTYPE_ANYPLAYER);
	if (p == nullptr) return HandlePacketResult::Handled;

	p->sendPacket(CString() >> (char)PLO_RC_ADMINMESSAGE << "Admin " << account.name << ":\xa7" << pPacket.readString(""));
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_LISTRCS(CString& pPacket)
{
	// Deprecated
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_DISCONNECTRC(CString& pPacket)
{
	// Deprecated
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_APPLYREASON(CString& pPacket)
{
	// Deprecated
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_SERVERFLAGSGET(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to view the server flags.", account.name);
		return HandlePacketResult::Handled;
	}

	CString ret;
	ret >> (char)PLO_RC_SERVERFLAGSGET >> (short)m_server->Scripting.variables.store.size();
	for (const auto& [flag, value] : m_server->Scripting.variables.store | variables::serializable)
	{
		if (auto serialized = m_server->Scripting.variables.serializeModern(flag); serialized.has_value())
			ret >> (char)serialized.value().length() << serialized.value();
	}

	sendPacket(ret);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_SERVERFLAGSSET(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_SETSERVERFLAGS))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to set the server flags.", account.name);
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to set the server flags.");
		return HandlePacketResult::Handled;
	}

	// Open the serverflags.txt file.
	if (const auto file = m_server->getFileSystemServer().openiForWriting(fs::FileCategory::CONFIG, "serverflags.txt", true); file && file->opened())
	{
		file->clear();

		// Read the flags and store them in the file.
		const uint16_t count = pPacket.readGUShort();
		for (auto i = 0; i < count; ++i)
		{
			std::string flagPair = string::trimMutate(pPacket.readChars(pPacket.readGUChar()).toString());
			file->writeLine(flagPair);
		}
	}

	log::printLine(log::rc, "{} has updated the server flags.", account.name);
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has updated the server flags.");
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_ACCOUNTADD(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to add a new account.", account.name);
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to create new accounts.");
		return HandlePacketResult::Handled;
	}

	const std::string acc = pPacket.readChars(pPacket.readGUChar()).toString();
	const std::string pass = pPacket.readChars(pPacket.readGUChar()).toString();
	const std::string email = pPacket.readChars(pPacket.readGUChar()).toString();
	const bool banned = (pPacket.readGUChar() != 0);
	const bool onlyLoad = (pPacket.readGUChar() != 0);
	pPacket.readGUChar(); // Admin level, deprecated.

	Account newAccount;
	m_server->getAccountLoader().loadAccount(acc, newAccount);
	newAccount.banned = banned;
	newAccount.email = email;
	newAccount.loadOnly = onlyLoad;
	m_server->getAccountLoader().saveAccount(newAccount);

	log::printLine(log::rc, "{} has created a new account: {}", account.name, acc.c_str());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has created a new account: " << acc);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_ACCOUNTDEL(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to delete an account.", account.name);
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to delete accounts.");
		return HandlePacketResult::Handled;
	}

	// Prevent the defaultaccount from being deleted.
	CString acc = pPacket.readString("");
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);
	if (acc == "(defaultaccount)")
	{
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not allowed to delete the default account.");
		return HandlePacketResult::Handled;
	}

	// Get the account.
	const auto fileInfo = m_server->getFileSystemServer().infoi(fs::FileCategory::ACCOUNT, std::format("{}.txt", acc));
	if (fileInfo == nullptr)
		return HandlePacketResult::Handled;

	// Remove the account from the file system.
	if (fileInfo->deleteFile())
	{
		log::printLine(log::rc, "{} has deleted the account: {}", account.name, acc.text());
		m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has deleted the account: " << acc);
	}
	else
	{
		log::printLine(log::rc, "{} tried to delete account '{}', but the delete failed.", account.name, acc.text());
		m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " tried to delete account " << acc << ", but an error occurred.");
	}
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_ACCOUNTLISTGET(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to view the account listing.", account.name);
		return HandlePacketResult::Handled;
	}

	CString name = pPacket.readChars(pPacket.readGUChar());
	const std::string conditions = pPacket.readChars(pPacket.readGUChar()).toString();

	// Fix up name searching.
	name.replaceAllI("%", "*");
	if (name.length() == 0)
		name = "*";

	// Start our packet.
	CString ret;
	ret >> (char)PLO_RC_ACCOUNTLISTGET;

	// Search through all the accounts.
	for (auto& fileInfoPtr : m_server->getFileSystemServer().info(fs::FileCategory::ACCOUNT))
	{
		auto fileInfo = fileInfoPtr.lock();
		if (fileInfo == nullptr) continue;

		auto accountName = fileInfo->file.stem().generic_string();
		if (accountName.empty()) continue;
		if (!string::match<true>(std::string_view{accountName}, name.toStringView())) continue;
		if (conditions.empty() || m_server->getAccountLoader().checkSearchConditions(accountName, string::splitToVector(conditions, std::string_view(","))))
			ret >> (char)accountName.length() << accountName;
	}

	sendPacket(ret);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERPROPSGET2(CString& pPacket)
{
	const auto p = m_server->getPlayer(pPacket.readGUShort(), PLTYPE_ANYPLAYER | PLTYPE_NPCSERVER);
	if (p == nullptr) return HandlePacketResult::Handled;

	if (isClient() || !account.hasRight(PLPERM_VIEWATTRIBUTES))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to view the props of player {}.", account.name, p->account.name);
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to view player props.");
		return HandlePacketResult::Handled;
	}

	sendPacket(CString() >> (char)PLO_RC_PLAYERPROPSGET >> (short)p->getId() << p->getPropsForRCPacket());
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERPROPSGET3(CString& pPacket)
{
	CString acc = pPacket.readChars(pPacket.readGUChar());
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (!m_server->getFileSystemServer().hasi(fs::FileCategory::ACCOUNT, std::format("{}.txt", acc.toStringView())))
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	if (isClient() || !account.hasRight(PLPERM_VIEWATTRIBUTES))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to view the props of player {}.", account.name, p->account.name);
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to view player props.");
		return HandlePacketResult::Handled;
	}

	sendPacket(CString() >> (char)PLO_RC_PLAYERPROPSGET >> (short)p->getId() << p->getPropsForRCPacket());
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERPROPSRESET(CString& pPacket)
{
	CString acc = pPacket.readString("");
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || !account.hasRight(PLPERM_RESETATTRIBUTES))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to reset the account: {}", account.name, acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to reset accounts.");
		return HandlePacketResult::Handled;
	}

	// Get the player.  Create a new player if they are offline.
	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (!m_server->getFileSystemServer().hasi(fs::FileCategory::ACCOUNT, std::format("{}.txt", acc.toStringView())))
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	// Save RC stuff.
	const auto adminip = p->account.adminIpRange;
	const auto rights = p->account.adminRights;
	std::vector<std::string> folders;
	std::ranges::copy(p->account.folderList, std::back_inserter(folders));

	// Reset the player.
	m_server->getAccountLoader().loadAccount("(defaultaccount)", p->account);
	p->account.name = acc.toStringView();
	m_server->getAccountLoader().saveAccount(p->account);

	// Add the RC stuff back in.
	p->account.adminIpRange = adminip;
	p->account.adminRights = rights;
	p->account.folderList = folders;

	// Save the account.
	m_server->getAccountLoader().saveAccount(p->account);

	// If the player is online, boot him from the server.
	if (p->getId() != 0)
	{
		p->sendPacket(CString() >> (char)PLO_DISCMESSAGE << "Your account was reset by " << account.name);
		p->setLoaded(false); // Don't save the account when the player quits.
		m_server->deletePlayer(p);
	}

	// Log it.
	log::printLine(log::rc, "{} has reset the attributes of account: {}", account.name, acc.text());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has reset the attributes of account: " << acc);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERPROPSSET2(CString& pPacket)
{
	CString acc = pPacket.readChars(pPacket.readGUChar());
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (!m_server->getFileSystemServer().hasi(fs::FileCategory::ACCOUNT, std::format("{}.txt", acc.toStringView())))
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	if (isClient() || (p->account.name != account.name && !account.hasRight(PLPERM_SETATTRIBUTES)) || (p->account.name == account.name && !account.hasRight(PLPERM_SETSELFATTRIBUTES)))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to set a player's properties.", account.name);
		else
			log::printLine(log::rc, "{} attempted to set a player's properties.", account.name);
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: " << account.name << " is not authorized to set the properties of " << p->account.name);
		return HandlePacketResult::Handled;
	}

	// Only people with PLPERM_MODIFYSTAFFACCOUNT can alter the default account.
	if (!account.hasRight(PLPERM_MODIFYSTAFFACCOUNT) && acc == "(defaultaccount)")
	{
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to modify the default account.");
		return HandlePacketResult::Handled;
	}

	p->setPropsFromRCPacket(pPacket, this);
	m_server->getAccountLoader().saveAccount(p->account);
	log::printLine(log::rc, "{} set the attributes of player {}", account.name, p->account.name);
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " set the attributes of player " << p->account.name);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_ACCOUNTGET(CString& pPacket)
{
	CString acc = pPacket.readString("");
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to view the account: {}", account.name, acc.text());
		return HandlePacketResult::Handled;
	}

	// Get the player.
	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (!m_server->getFileSystemServer().hasi(fs::FileCategory::ACCOUNT, std::format("{}.txt", acc.toStringView())))
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	// clang-format off
	sendPacket(CString() >> (char)PLO_RC_ACCOUNTGET >> (char)acc.length() << acc
		>> (char)0 // >> (char)password_length << password
		>> (char)p->account.email.size() << p->account.email
		>> (char)(p->account.banned ? 1 : 0) >> (char)(p->account.loadOnly ? 1 : 0) >> (char)0 // admin level
		>> (char)4 << "main"
		>> (char)p->account.banLength.size() << p->account.banLength
		>> (char)p->account.banReason.size() << p->account.banReason);
	// clang-format on

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_ACCOUNTSET(CString& pPacket)
{
	CString acc = pPacket.readChars(pPacket.readGUChar());
	if (acc.length() == 0) return HandlePacketResult::Handled;
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || !account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to edit the account: {}", account.name, acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to edit accounts.");
		return HandlePacketResult::Handled;
	}

	const std::string pass = pPacket.readChars(pPacket.readGUChar()).toString();
	const std::string email = pPacket.readChars(pPacket.readGUChar()).toString();
	const bool banned = (pPacket.readGUChar() != 0 ? true : false);
	const bool loadOnly = (pPacket.readGUChar() != 0 ? true : false);
	pPacket.readGUChar();                    // admin level
	pPacket.readChars(pPacket.readGUChar()); // world
	const std::string banreason = pPacket.readChars(pPacket.readGUChar()).toString();

	// Get player.
	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (!m_server->getFileSystemServer().hasi(fs::FileCategory::ACCOUNT, std::format("{}.txt", acc.toStringView())))
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	// Set the new account stuff.
	p->account.email = email;
	p->account.loadOnly = loadOnly;
	if (account.hasRight(PLPERM_BAN))
	{
		p->account.banned = banned;
		p->account.banReason = banreason;
	}
	m_server->getAccountLoader().saveAccount(p->account);

	// If the account is currently on RC, reload it.
	if (const auto pRC = m_server->getPlayer(acc, PLTYPE_ANYRC); pRC)
		m_server->getAccountLoader().loadAccount(acc.toStringView(), pRC->account);

	// If the player was just now banned, kick him off the server.
	if (account.hasRight(PLPERM_BAN) && banned && p->getId() != 0)
	{
		const auto reason = string::join(string::fromCSV(banreason), std::string_view{"\r"});

		p->setLoaded(false);
		p->sendPacket(CString() >> (char)PLO_DISCMESSAGE << account.name << " has banned you.  Reason: " << reason);
		m_server->deletePlayer(p);
	}

	log::printLine(log::rc, "{} has modified the account: {}", account.name, acc.text());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has modified the account: " << acc);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_CHAT(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to send a message to RC.", account.name);
		return HandlePacketResult::Handled;
	}

	if (isNC())
	{
		// TODO(joey): All RC's with NC support are sending two messages at a time.
		//  Can use this section for npc-server related commands though.
		//m_server->sendToNC(CString(account.character.nickName) << ": " << message);
		return HandlePacketResult::Handled;
	}

	const CString message = pPacket.readString("");
	if (!m_server->processRCChat(message.toStringView(), shared_from_this()))
		m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.character.nickName << ": " << message);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_WARPPLAYER(CString& pPacket)
{
	if (isClient() || !account.hasRight(PLPERM_WARPTOPLAYER))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to warp a player.", account.name);
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to warp players.\n");
		return HandlePacketResult::Handled;
	}

	const auto p = m_server->getPlayer<PlayerClient>(pPacket.readGUShort(), PLTYPE_ANYPLAYER);
	if (p == nullptr) return HandlePacketResult::Handled;

	Position<int16_t> pos = {static_cast<int16_t>(pPacket.readGChar() * 8), static_cast<int16_t>(pPacket.readGChar() * 8)};
	CString wLevel = pPacket.readString("");
	p->warp(wLevel, pos);

	log::printLine(log::rc, "{} has warped {} to {} ({:.2f}, {:.2f})", account.name, p->account.name, wLevel.text(), static_cast<float>(pos.x()) / 16.0f, static_cast<float>(pos.y()) / 16.0f);
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERRIGHTSGET(CString& pPacket)
{
	CString acc = pPacket.readString("");
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || (acc != account.name && !account.hasRight(PLPERM_SETRIGHTS)))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to get the rights of {}", account.name, acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to view that player's rights.");
		return HandlePacketResult::Handled;
	}

	// Get player.
	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (!m_server->getFileSystemServer().hasi(fs::FileCategory::ACCOUNT, std::format("{}.txt", acc.toStringView())))
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	// Get the folder list.
	const auto folders = string::toCSV(p->account.folderList);

	// Send the packet.
	const auto adminIp = string::join(p->account.adminIpRange);
	sendPacket(CString() >> (char)PLO_RC_PLAYERRIGHTSGET >> (char)acc.length() << acc >> (long long)p->account.adminRights >> (char)adminIp.size() << adminIp >> (short)folders.length() << folders);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERRIGHTSSET(CString& pPacket)
{
	CString acc = pPacket.readChars(pPacket.readGUChar());
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || !account.hasRight(PLPERM_SETRIGHTS))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to set the rights of {}", account.name, acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to set player rights.");
		return HandlePacketResult::Handled;
	}

	// Get player.
	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (!m_server->getFileSystemServer().hasi(fs::FileCategory::ACCOUNT, std::format("{}.txt", acc.toStringView())))
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	// Don't allow RCs to give rights that they don't have.
	// Only affect people who don't have PLPERM_MODIFYSTAFFACCOUNT.
	auto n_adminRights = static_cast<uint32_t>(pPacket.readGUInt5());
	if (!account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		for (int i = 0; i < 20; ++i)
		{
			if ((account.adminRights & (1 << i)) == 0)
				n_adminRights &= ~(1 << i);
		}
	}

	// Don't allow you to remove your own PLPERM_MODIFYSTAFFACCOUNT or PLPERM_SETRIGHTS.
	if (string::equalsi(p->account.name, this->account.name))
	{
		if ((n_adminRights & PLPERM_MODIFYSTAFFACCOUNT) == 0)
			n_adminRights |= PLPERM_MODIFYSTAFFACCOUNT;
		if ((n_adminRights & PLPERM_SETRIGHTS) == 0)
			n_adminRights |= PLPERM_SETRIGHTS;
	}

	const auto changed_rights = account.adminRights ^ n_adminRights;
	p->account.adminRights = n_adminRights;

	const std::string adminIp = pPacket.readChars(pPacket.readGUChar()).toString();
	p->account.adminIpRange = string::splitToVector(adminIp, ","sv);

	// Untokenize and load the directories.
	std::vector<std::string> folders = string::fromCSV(pPacket.readChars(pPacket.readGUShort()).toString());

	// Remove any invalid directories.
	for (auto i = folders.begin(); i != folders.end();)
	{
		if (i->find(':') != std::string::npos || i->find("..") != std::string::npos || i->find(" /*") != std::string::npos)
			i = folders.erase(i);
		else
			++i;
	}

	// Assign the directories to the account.
	p->account.folderList = folders;

	// Save the account.
	m_server->getAccountLoader().saveAccount(p->account);

	// If the account is currently on RC, reload it.
	if (const auto pRC = m_server->getPlayer<PlayerRC>(acc, PLTYPE_ANYRC); pRC)
	{
		const std::string nickname = pRC->account.character.nickName;
		m_server->getAccountLoader().loadAccount(acc.toStringView(), pRC->account);
		pRC->account.character.nickName = nickname;

		if (changed_rights & PLPERM_NPCCONTROL)
		{
			if (!(n_adminRights & PLPERM_NPCCONTROL))
			{
				if (const auto pNC = m_server->getPlayer(acc, PLTYPE_ANYNC); pNC)
					pNC->disconnect();
			}
			else if (m_server->hasNPCServer())
			{
				m_server->getNPCServer()->sendNCLoginToPlayer(pRC);
			}
		}

		// If they are using the File Browser, reload it.
		if (pRC->isUsingFileBrowser())
			pRC->msgPLI_RC_FILEBROWSER_START(CString() << "");
	}

	log::printLine(log::rc, "{} has set the rights of {}", account.name, acc.text());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has set the rights of " << acc);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERCOMMENTSGET(CString& pPacket)
{
	CString acc = pPacket.readString("");
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to get the comments of {}", account.name, acc.text());
		return HandlePacketResult::Handled;
	}

	// Get player.
	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (!m_server->getFileSystemServer().hasi(fs::FileCategory::ACCOUNT, std::format("{}.txt", acc.toStringView())))
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	sendPacket(CString() >> (char)PLO_RC_PLAYERCOMMENTSGET >> (char)acc.length() << acc << p->account.comments);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERCOMMENTSSET(CString& pPacket)
{
	CString acc = pPacket.readChars(pPacket.readGUChar());
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || !account.hasRight(PLPERM_SETCOMMENTS))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to set the comments of {}", account.name, acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to set player comments.");
		return HandlePacketResult::Handled;
	}

	// Get player.
	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (!m_server->getFileSystemServer().hasi(fs::FileCategory::ACCOUNT, std::format("{}.txt", acc.toStringView())))
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	const CString comment = pPacket.readString("");
	p->account.comments = comment.toStringView();
	m_server->getAccountLoader().saveAccount(p->account);

	// If the account is currently on RC, reload it.
	if (const auto pRC = m_server->getPlayer(acc, PLTYPE_ANYRC); pRC)
		m_server->getAccountLoader().loadAccount(acc.toStringView(), pRC->account);

	log::printLine(log::rc, "{} has set the comments of {}", account.name, acc.text());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has set the comments of " << acc);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERBANGET(CString& pPacket)
{
	CString acc = pPacket.readString("");
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to view the ban of {}", account.name, acc.text());
		return HandlePacketResult::Handled;
	}

	// Get player.
	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (!m_server->getFileSystemServer().hasi(fs::FileCategory::ACCOUNT, std::format("{}.txt", acc.toStringView())))
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	sendPacket(CString() >> (char)PLO_RC_PLAYERBANGET >> (char)acc.length() << acc >> (char)(p->account.banned ? 1 : 0) << p->account.banReason);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_PLAYERBANSET(CString& pPacket)
{
	CString acc = pPacket.readChars(pPacket.readGUChar());
	if (acc.find("/") != -1) acc.removeI(acc.findl('/') + 1);
	if (acc.find("\\") != -1) acc.removeI(acc.findl('\\') + 1);

	if (isClient() || !account.hasRight(PLPERM_BAN))
	{
		if (isClient()) log::printLine(log::rc, "[Hack] {} attempted to set the ban of {}", account.name, acc.text());
		sendPacket(CString() >> (char)PLO_RC_CHAT << "Server: You are not authorized to set player bans.");
		return HandlePacketResult::Handled;
	}

	// Get player.
	auto p = m_server->getPlayer(acc, PLTYPE_ANYCLIENT);
	if (p == nullptr)
	{
		if (!m_server->getFileSystemServer().hasi(fs::FileCategory::ACCOUNT, std::format("{}.txt", acc.toStringView())))
			return HandlePacketResult::Handled;

		p = std::make_shared<Player>(nullptr, 0);
		if (!m_server->getAccountLoader().loadAccount(acc.toStringView(), p->account))
			return HandlePacketResult::Handled;
	}

	const bool banned = (pPacket.readGUChar() != 0);
	const CString reason = pPacket.readString("");
	p->account.banned = banned;
	p->account.banReason = reason.toStringView();
	m_server->getAccountLoader().saveAccount(p->account);

	// If the account is currently on RC, reload it.
	if (const auto pRC = m_server->getPlayer(acc, PLTYPE_ANYRC); pRC)
		m_server->getAccountLoader().loadAccount(acc.toStringView(), pRC->account);

	// If the player was just now banned, kick him off the server.
	if (banned && p->getId() != 0)
	{
		p->sendPacket(CString() >> (char)PLO_DISCMESSAGE << account.name << " has banned you.  Reason: " << reason.guntokenize().replaceAll("", "\r"));
		m_server->deletePlayer(p);
	}

	log::printLine(log::rc, "{} has set the ban of {}", account.name, acc.text());
	m_server->sendPacketToType(PLTYPE_ANYRC, CString() >> (char)PLO_RC_CHAT << account.name << " has set the ban of " << acc);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_START(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to open the File Browser.", account.name);
		return HandlePacketResult::Handled;
	}

	// If the player has no folder rights, don't open the File Browser.
	if (account.folderList.empty())
		return HandlePacketResult::Handled;

	// Get folder list to send to the client.
	auto folders = string::toCSV(account.folderList, true);

	// Send the folder list and the welcome message.
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_DIRLIST << folders);
	if (!m_isFtp) sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Welcome to the File Browser.");

	// Create a folder map.
	std::map<CString, CString> folderMap;
	for (auto& f : account.folderList)
	{
		CString rights("r");
		CString wild("*");
		CString folder(f);
		rights = folder.readString(" ").trim().toLower();
		folder.removeI(0, folder.readPos());
		folder.replaceAllI("\\", "/");
		folder.trimI();
		if (folder[folder.length() - 1] != '/')
		{
			if (int pos = folder.findl('/'); pos != -1)
			{
				wild = folder.subString(pos + 1);
				folder.removeI(pos + 1);
			}
		}
		folderMap[folder] << rights << ":" << wild << "\n";
	}

	// See if we can use our lastFolder.  If we can't, use the first folder.
	if (!folderMap.contains(account.lastFolderAccessed))
		account.lastFolderAccessed = folderMap.begin()->first.toStringView();

	// We want to end with a path separator.
	if (!account.lastFolderAccessed.ends_with('/'))
		account.lastFolderAccessed += '/';

	// Create the file system.
	std::filesystem::directory_iterator dirs{account.lastFolderAccessed};

	// Construct the file list.
	CString files;
	std::vector<CString> wildcards = folderMap[account.lastFolderAccessed].tokenize("\n");
	for (auto i = wildcards.begin(); i != wildcards.end(); ++i)
	{
		CString rights = i->readString(":");
		CString wildcard = i->readString("");
		i->setRead(0);
		for (auto& dirEntry : dirs)
		{
			if (!dirEntry.is_regular_file()) continue;
			CString fileName = fs::getANSIFileName(dirEntry.path());

			// See if the file matches the wildcard.
			if (!fileName.match(wildcard))
				continue;

			// Add the file now.
			auto modTime = clock::to_time_t(toSystemClock(dirEntry.last_write_time()));
			CString dir = CString() >> (char)fileName.length() << fileName >> (char)rights.length() << rights >> (long long)dirEntry.file_size() >> (long long)modTime;
			files << " " >> (char)dir.length() << dir;
		}
	}

	// Send packet.
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_DIR >> (char)account.lastFolderAccessed.length() << account.lastFolderAccessed << files);
	m_isFtp = true;

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_CD(CString& pPacket)
{
	if (isClient()) return HandlePacketResult::Handled;

	CString newFolder = pPacket.readString("");
	CString newRights, wildcard;
	newFolder.setRead(0);

	// Create a folder map.
	std::map<CString, CString> folderMap;
	for (auto i = account.folderList.begin(); i != account.folderList.end(); ++i)
	{
		CString rights("r");
		CString wild("*");
		CString folder(*i);
		rights = folder.readString(" ").trim().toLower();
		folder.removeI(0, folder.readPos());
		folder.replaceAllI("\\", "/");
		folder.trimI();
		if (folder[folder.length() - 1] != '/')
		{
			if (int pos = folder.findl('/'); pos != -1)
			{
				wild = folder.subString(pos + 1);
				folder.removeI(pos + 1);
			}
		}
		folderMap[folder] << rights << ":" << wild << "\n";
	}

	// See if newFolder is part of the folder map.
	// If it isn't, return.
	if (!folderMap.contains(newFolder))
		return HandlePacketResult::Handled;
	else
		account.lastFolderAccessed = newFolder.toStringView();

	// We want to end with a path separator.
	if (!account.lastFolderAccessed.ends_with('/'))
		account.lastFolderAccessed += '/';

	// Make sure our folder exists.
	std::filesystem::path fsPath{account.lastFolderAccessed};
	std::filesystem::create_directories(fsPath);
	std::filesystem::directory_iterator dirs{fsPath};

	// Construct the file list.
	// file packet: {CHAR name_length}{STRING name}{CHAR rights_length}{STRING rights}{INT5 file_size}{INT5 file_mod_time}
	// files: {CHAR file_packet_length}{file_packet}[space]{CHAR file_packet_length}{file_packet}[space]
	CString files;
	std::vector<CString> wildcards = folderMap[account.lastFolderAccessed].tokenize("\n");
	for (auto i = wildcards.begin(); i != wildcards.end(); ++i)
	{
		CString rights = i->readString(":");
		CString wildcard = i->readString("");
		i->setRead(0);
		for (auto& dirEntry : dirs)
		{
			if (!dirEntry.is_regular_file()) continue;
			CString fileName = fs::getANSIFileName(dirEntry.path());

			// See if the file matches the wildcard.
			if (!fileName.match(wildcard))
				continue;

			// Add the file now.
			auto size = dirEntry.file_size();
			auto modTime = clock::to_time_t(toSystemClock(dirEntry.last_write_time()));
			CString dir = CString() >> (char)fileName.length() << fileName >> (char)rights.length() << rights >> (long long)size >> (long long)modTime;
			files << " " >> (char)dir.length() << dir;
		}
	}

	// Send packet.
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Folder changed to " << account.lastFolderAccessed);
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_DIR >> (char)account.lastFolderAccessed.length() << account.lastFolderAccessed << files);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_END(CString& pPacket)
{
	if (isClient()) return HandlePacketResult::Handled;
	m_isFtp = false;

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_DOWN(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to download a file through the File Browser.", account.name);
		return HandlePacketResult::Handled;
	}

	// Send file.
	const std::filesystem::path file{pPacket.readString("").toString()};
	const std::filesystem::path lastFolderAccessed{account.lastFolderAccessed};
	const CString checkFile = (lastFolderAccessed / file).generic_string();

	// Don't let us download/view important files.
	if (!account.hasRight(PLPERM_MODIFYSTAFFACCOUNT))
	{
		for (const auto& file : ProtectedFiles)
		{
			if (checkFile == file)
			{
				sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Insufficient rights to download/view " << checkFile);
				return HandlePacketResult::Handled;
			}
		}
	}

	sendFile(lastFolderAccessed / file);

	log::printLine(log::rc, "{} downloaded file {}", account.name, file.generic_string());
	//sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Downloaded file " << file.generic_string());

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_UP(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to upload a file through the File Browser.", account.name);
		return HandlePacketResult::Handled;
	}

	const std::filesystem::path file{pPacket.readChars(pPacket.readGUChar()).toString()};
	const std::filesystem::path lastFolderAccessed{account.lastFolderAccessed};
	const CString fileData = pPacket.subString(pPacket.readPos());
	const CString checkFile = (lastFolderAccessed / file).generic_string();

	// Check if this is a protected file.
	bool isProtected = false;
	size_t fileID = std::numeric_limits<size_t>::max();
	for (size_t i = 0; i < ImportantFiles.size(); ++i)
	{
		if (checkFile == ImportantFiles[i])
		{
			fileID = i;
			isProtected = true;
			break;
		}
	}

	// If this file is protected, see if we have permission to upload this file.
	bool hasPermission = true;
	if (isProtected)
	{
		hasPermission = account.hasRight(PLPERM_MODIFYSTAFFACCOUNT);
		if (!hasPermission)
		{
			if (fileID < ImportantFileRights.size())
				hasPermission = account.hasRight(ImportantFileRights[fileID]);
		}
	}

	// Don't let us upload/overwrite important files.
	if (isProtected && !hasPermission)
	{
		sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Insufficent rights to upload " << checkFile);
		return HandlePacketResult::Handled;
	}

	// See if we are uploading a large file or not.
	if (!m_rcLargeFiles.contains(file))
	{
		// Normal file. Save it and display our message.
		(void)fileData.save(checkFile);

		log::printLine(log::rc, "{} uploaded file {}", account.name, file.generic_string());
		sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Uploaded file " << file.generic_string());
	}
	else
	{
		// Large file.  Store the data in memory.
		m_rcLargeFiles[file] << fileData;
	}

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_MOVE(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to move a file through the File Browser.", account.name);
		return HandlePacketResult::Handled;
	}

	const std::filesystem::path dir{pPacket.readChars(pPacket.readGUChar()).toString()};
	const std::filesystem::path file{pPacket.readString("").toString()};
	const std::filesystem::path lastFolderAccessed{account.lastFolderAccessed};

	// Assemble destination and source.
	const auto destination = dir / file;
	const auto source = lastFolderAccessed / file;

	// Don't let us move important files.
	for (const auto& importantFile : ImportantFiles)
	{
		if (source == importantFile)
		{
			sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Not allowed to move file " << source.generic_string());
			return HandlePacketResult::Handled;
		}
	}

	std::error_code ec;
	std::filesystem::rename(source, destination, ec);

	if (ec)
	{
		log::printLine(log::rc, "Error moving file: {}", ec.message());
		sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Error moving file: " << ec.message());
		return HandlePacketResult::Handled;
	}

	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Moved file " << source.generic_string() << " to " << destination.generic_string());
	log::printLine(log::rc, "{} moved file {} to {}", account.name, source.generic_string(), destination.generic_string());
	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_DELETE(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to delete a file through the File Browser.", account.name);
		return HandlePacketResult::Handled;
	}

	CString file = pPacket.readString("");
	const std::filesystem::path filePath = std::filesystem::path{account.lastFolderAccessed} / file.toStringView();

	// Don't let us delete important files.
	const CString checkFile = filePath.generic_string();
	for (const auto& file : ImportantFiles)
	{
		if (checkFile == file)
		{
			sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Not allowed to delete file " << checkFile);
			return HandlePacketResult::Handled;
		}
	}

	// Deleting our logs?  First, we need to close them.
	if (account.lastFolderAccessed == "logs/")
	{
		if (file == "rclog.txt") log::rc.close();
		else if (file == "serverlog.txt")
			log::server.close();
		else if (file == "npclog.txt")
			log::npc.close();
		else if (file == "scriptlog.txt")
			log::script.close();
	}

	// Do the deleting.
	std::error_code ec;
	std::filesystem::remove(filePath, ec);

	// Deleting our logs?  We can open them now.
	if (account.lastFolderAccessed == "logs/")
	{
		if (file == "rclog.txt") log::rc.reload();
		else if (file == "serverlog.txt")
			log::server.reload();
		else if (file == "npclog.txt")
			log::npc.reload();
		else if (file == "scriptlog.txt")
			log::script.reload();
	}

	// If we got an error, record it now.
	if (ec)
	{
		log::printLine(log::rc, "Error deleting file: {}", ec.message());
		sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Error deleting file: " << ec.message());
		return HandlePacketResult::Handled;
	}

	log::printLine(log::rc, "{} deleted file {}", account.name, file.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Deleted file " << file);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FILEBROWSER_RENAME(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to rename a file through the File Browser.", account.name);
		return HandlePacketResult::Handled;
	}

	CString f1 = pPacket.readChars(pPacket.readGUChar());
	CString f2 = pPacket.readChars(pPacket.readGUChar());

	const std::filesystem::path oldPath = std::filesystem::path{account.lastFolderAccessed} / f1.toStringView();
	const std::filesystem::path newPath = std::filesystem::path{account.lastFolderAccessed} / f2.toStringView();

	// Don't let us rename/overwrite important files.
	const CString checkFile1 = oldPath.generic_string();
	const CString checkFile2 = newPath.generic_string();
	for (const auto& file : ImportantFiles)
	{
		if (checkFile1 == file || checkFile2 == file)
		{
			sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Not allowed to rename/overwrite file " << checkFile1 << " or " << checkFile2);
			return HandlePacketResult::Handled;
		}
	}

	// Renaming our logs?  First, we need to close them.
	if (account.lastFolderAccessed == "logs/")
	{
		if (f1 == "rclog.txt") log::rc.close();
		else if (f1 == "serverlog.txt")
			log::server.close();
		else if (f1 == "npclog.txt")
			log::npc.close();
		else if (f1 == "scriptlog.txt")
			log::script.close();
	}

	// Do the renaming.
	std::error_code ec;
	std::filesystem::rename(oldPath, newPath, ec);

	// Renaming our logs?  We can open them now.
	if (account.lastFolderAccessed == "logs/")
	{
		if (f1 == "rclog.txt") log::rc.reload();
		else if (f1 == "serverlog.txt")
			log::server.reload();
		else if (f1 == "npclog.txt")
			log::npc.reload();
		else if (f1 == "scriptlog.txt")
			log::script.reload();
	}

	// If we got an error, record it now.
	if (ec)
	{
		log::printLine(log::rc, "Error renaming file: {}", ec.message());
		sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Error renaming file: " << ec.message());
		return HandlePacketResult::Handled;
	}

	log::printLine(log::rc, "{} renamed file {} to {}", account.name, f1.text(), f2.text());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Renamed file " << f1 << " to " << f2);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_LARGEFILESTART(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} is attempting to upload a file through the File Browser.", account.name);
		return HandlePacketResult::Handled;
	}

	const std::filesystem::path file{pPacket.readString("").toString()};
	m_rcLargeFiles[file] = CString();

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_LARGEFILEEND(CString& pPacket)
{
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to upload a file through the File Browser.", account.name);
		return HandlePacketResult::Handled;
	}

	const std::filesystem::path file{pPacket.readString("").toString()};
	const std::filesystem::path filePath = std::filesystem::path{account.lastFolderAccessed} / file;

	// Save the file.
	(void)m_rcLargeFiles[file].save(filePath.string());

	// Remove the data from memory.
	for (auto it = m_rcLargeFiles.begin(); it != m_rcLargeFiles.end(); ++it)
	{
		if (it->first == file)
		{
			m_rcLargeFiles.erase(it);
			break;
		}
	}

	log::printLine(log::rc, "{} uploaded large file {}", account.name, file.generic_string());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Uploaded large file " << file.generic_string());

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_FOLDERDELETE(CString& pPacket)
{
	const std::filesystem::path folder{pPacket.readString("").toString()};
	if (isClient())
	{
		log::printLine(log::rc, "[Hack] {} attempted to delete a folder through the File Browser: {}", account.name, folder.generic_string());
		return HandlePacketResult::Handled;
	}

	// Try to remove folder.
	if (!std::filesystem::remove(folder))
	{
		sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Error removing " << folder.generic_string() << ".  Folder may not exist or may not be empty.");
		return HandlePacketResult::Handled;
	}

	log::printLine(log::rc, "{} removed folder {}", account.name, folder.generic_string());
	sendPacket(CString() >> (char)PLO_RC_FILEBROWSER_MESSAGE << "Folder " << folder.generic_string() << " has been removed.\n");
	msgPLI_RC_FILEBROWSER_START(CString() << "");

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_NPCSERVERQUERY(CString& pPacket)
{
	if (!m_server->hasNPCServer())
		return HandlePacketResult::Handled;

	// Read Packet Data
	[[maybe_unused]] auto pid = static_cast<PlayerID>(pPacket.readGUShort());
	const CString message = pPacket.readString("");

	// Enact upon the message.
	if (message == "location")
		m_server->getNPCServer()->sendNCLoginToPlayer(shared_from_this());
	else
		log::printLine(log::server, "[RC] Received unknown PLI_NPCSERVERQUERY message: {}", message);

	return HandlePacketResult::Handled;
}

HandlePacketResult PlayerRC::msgPLI_RC_UNKNOWN162(CString& pPacket)
{
	// Stub.
	return HandlePacketResult::Handled;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
