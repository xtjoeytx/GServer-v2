#include <vector>

#include <CString.h>
#include <IEnums.h>

#include <Server.h>
#include <network/IPacketHandler.h>
#include <object/Player.h>
#include <player/PlayerProps.h>
#include <utilities/Log.h>
#include <utilities/StringUtils.h>

///////////////////////////////////////////////////////////////////////////////
namespace preagonal
{
///////////////////////////////////////////////////////////////////////////////

HandlePacketResult Player::msgPLI_REQUESTTEXT(CString& pPacket)
{
	// TODO(joey): So I believe these are just requests for information, while sendtext is used to actually do things.

	const CString packet = pPacket.readString("");
	CString data = packet.guntokenize();

	const CString weapon = data.readString("\n");
	const CString type = data.readString("\n");
	const CString option = data.readString("\n");

	auto& list = m_server->getServerList();
	if (type == "lister")
	{
		if (option == "simplelist")
		{
			const auto output = string::toCSVFromPack(
				weapon.toStringView(),
				type.toStringView(),
				"simpleserverlist"sv
			);
			list.sendPacket(CString() >> (char)SVO_REQUESTLIST >> (short)m_id << output);
		}
		else if (option == "rebornlist")
			list.sendPacket(CString() >> (char)SVO_REQUESTLIST >> (short)m_id << packet);
		else if (option == "subscriptions")
		{
			const std::string output = string::toCSVFromPack(
				weapon.toString(),
				type.toString(),
				"subscriptions"s,
				string::toCSVFromPack("unlimited"sv, "Unlimited Subscription"sv, ""sv)
			);

			// Some versions of the loginserver scripts expected the response of subscriptions2 rather than subscriptions
			sendPacket(CString() >> (char)PLO_SERVERTEXT << output);
		}
		else if (option == "bantypes")
		{
			// Type,duration (seconds)
			constexpr std::array builtInBanTypes{
				"Event Interruption,259200"sv,
				"Message Code Abuse,259200"sv,
				"General Scamming,604800"sv,
				"Advertising,604800"sv,
				"General Harassment,604800"sv,
				"Racism or Severe Vulgarity,1209600"sv,
				"Sexual Harassment,1209600"sv,
				"Cheating,2592000"sv,
				"Advertising Money Trade,2592000"sv,
				"Ban Evasion,2592000"sv,
				"Speed Hacking,2592000"sv,
				"Bug Abuse,2592000"sv,
				"Multiple Jailings,2592000"sv,
				"Server Destruction,3888000"sv,
				"Leaking Information,3888000"sv,
				"Account Scam,7776000"sv,
				"Account Sharing,315360000"sv,
				"Hacking,315360000"sv,
				"Multiple Bans,315360000"sv,
				"Other Unlimited,315360001"sv,
			};
			static std::string banTypes = string::toCSV(builtInBanTypes);
			sendPacket(CString() >> (char)PLO_SERVERTEXT << packet << "," << banTypes);
		}
		else if (option == "getglobalitems")
		{
			// Properties for a global tradeable gold item.
			constexpr std::array goldItemTestProps{
				"autobill=1"sv,
				"autobillmine=1"sv,
				"bundle=1"sv,
				"creationtime=1212768763"sv,
				"currenttime=1353248504"sv,
				"description=Gives"sv,
				"duration=2629800"sv,
				"flags=subscription"sv,
				"icon=graalicon_big.png"sv,
				"itemid=1"sv,
				"lifetime=1"sv,
				"owner=global"sv,
				"ownertype=server"sv,
				"price=100"sv,
				"quantity=988506"sv,
				"svtatus=available"sv,
				"title=Gold"sv,
				"tradable=1"sv,
				"typeid=62"sv,
				"world=global"sv,
			} ;
			static std::string goldItemTest = string::toCSV(goldItemTestProps);

			// List of all global items.
			std::array items{
				goldItemTest
			};

			// The output elements.
			const std::string output = string::toCSVFromPack(
				weapon.toString(),
				type.toString(),
				"globalitems"s,
				account.name,
				string::toCSV(items)
			);

			sendPacket(CString() >> (char)PLO_SERVERTEXT << output);
		}
		else if (option == "serverinfo")
		{
			list.sendPacket(CString() >> (char)SVO_REQUESTSVRINFO >> (short)m_id << packet);
		}
	}
	else if (type == "pmservers" || type == "pmguilds")
	{
		list.sendPacket(CString() >> (char)SVO_REQUESTLIST >> (short)m_id << packet);
	}
	else if (type == "pmserverplayers")
		addPMServer(option);
	else if (type == "pmunmapserver")
		remPMServer(option);
	else if (type == "irc")
	{
	}
	else if (type == "packageinfo")
	{
		if (const auto updatePackage = m_server->getPackageManager().findOrAddResource(option.text()))
		{
			const auto packageArgs = string::toCSVFromPack(
				weapon.toString(),
				type.toString(),
				option.toString(),
				string::to_string(updatePackage->getFileList().size()),
				string::to_string(updatePackage->getPackageSize())
			);

			sendPacket(CString() >> (char)PLO_SERVERTEXT << packageArgs);
		}
	}

	log::printLine(log::server, "[ IN] [RequestText] from {} -> {}", string::toCSV(account.name), packet);
	return HandlePacketResult::Handled;
}

HandlePacketResult Player::msgPLI_SENDTEXT(CString& pPacket)
{
	CString packet = pPacket.readString("");
	CString data = packet.guntokenize();
	const auto params = data.tokenize("\n");

	const auto weapon = data.readString("\n");
	const auto type = data.readString("\n");
	const auto option = data.readString("\n");
	const auto params2 = data.readString("").tokenize("\n");

	auto& list = m_server->getServerList();

	//if (weapon == "GraalEngine")
	{
		if (type == "irc")
		{
			if (option == "login")
			{
				// If client/rc sends "GraalEngine,irc,login,-" it should return all existing "IRC" channels as players.
				// How should we handle that?
				CString channel = "#graal";
				CString channelAccount = CString() << "irc:" << channel;
				CString channelNick = channel << " (1,0)";

				// RC uses addplayer/delplayer
				if (isRC())
				{
					// Irc players start at 16k
					sendPacket(CString() >> (char)PLO_ADDPLAYER >> (short)(16000 + 0) >> (char)channelAccount.length() << channelAccount >> (char)PlayerProp::NICKNAME >> (char)channelNick.length() << channelNick >> (char)PlayerProp::PLAYERLISTCATEGORY >> (char)PlayerListCategory::EXTERNAL);
				}
				else
				{
					sendPacket(CString() >> (char)PLO_OTHERPLPROPS >> (short)(16000 + 0) >> (char)PlayerProp::ACCOUNTNAME >> (char)channelAccount.length() << channelAccount >> (char)PlayerProp::NICKNAME >> (char)channelNick.length() << channelNick >> (char)PlayerProp::PLAYERLISTCATEGORY >> (char)PlayerListCategory::EXTERNAL);
				}
			}
			else if (params.size() > 3)
			{
				if (option == "join")
				{
					const CString& channel = params[3];
					CString sendMsg = "GraalEngine,irc,join,";
					sendMsg << channel.gtokenize();
					list.sendTextForPlayer(shared_from_this(), sendMsg);
				}
				else if (option == "part")
				{
					const CString& channel = params[3];
					CString sendMsg = "GraalEngine,irc,part,";
					sendMsg << channel.gtokenize();
					list.sendTextForPlayer(shared_from_this(), sendMsg);
				}
				else if (option == "topic")
				{
					// GraalEngine,irc,topic,#graal,topic
					//CString channel = params[0];
					//sendPacket(CString() >> (char)PLO_SERVERTEXT << "GraalEngine,irc,part," << channel);
				}
				else if (option == "privmsg" && params.size() > 4)
				{
					const CString& channel = params[3];
					const CString& msg = params[4];

					if (channel == "IRCBot")
					{
						const auto params3 = msg.guntokenize().tokenize("\n");
						if (params3[0] == "!getserverinfo")
						{
							//list->sendPacket(CString() >> (char)SVO_REQUESTSVRINFO >> (short)id << weapon << ",irc,privmsg," << params3[1].gtokenize());
							log::printLine(log::server, "[ IN] [SVO_SERVERINFO] {},{}", string::toCSV(account.name), packet);
							//list->sendPacket(CString() >> (char)SVO_SERVERINFO >> (short)id << params3[1]); // <-- this solves it for now

							// I believe the following data is what it's looking for:
							// "era,Era,93,English,""Welcome to Era, a modernised server. Please visit the website for more information."",http://era.graal.net/,""Graal 5.1-5.2"""
						}
					}
					else
					{
						CString sendMsg = "GraalEngine,irc,privmsg,";
						sendMsg << account.name << "," << channel.gtokenize() << "," << msg.gtokenize();
						list.handleText(sendMsg);
						list.sendTextForPlayer(shared_from_this(), sendMsg);
					}
				}
			}
		}
		else if (type == "lister")
		{
			if (option == "serverinfo")
				list.sendPacket(CString() >> (char)SVO_REQUESTSVRINFO >> (short)m_id << packet);

			if (!isGuest())
			{
				if (option == "verifybuddies" || option == "addbuddy" || option == "deletebuddy")
					list.sendTextForPlayer(shared_from_this(), packet);
			}

			if (isRC())
			{
				// TODO(joey): Implement for RC3
				//	banhistory - each comma separated item per line, just text
				//	staffactivity - each comma separated item per line, just text
				//	localbans - each comma separated item per line, just text (each person banned)
				//	ban - read below

				if (option == "getban")
				{
					// Send param is computer id. Either 0, or the id. It is required though
					sendPacket(CString() >> (char)PLO_SERVERTEXT << "GraalEngine,lister,ban," << params[0] << "," << "0");
					//msgPLI_RC_PLAYERBANGET(params[0]);
				}
			}
		}
	}

	log::printLine(log::server, "[ IN] [SendText] {}: {}", string::toCSV(account.name), packet);

	return HandlePacketResult::Handled;
}

///////////////////////////////////////////////////////////////////////////////
} // end namespace preagonal
