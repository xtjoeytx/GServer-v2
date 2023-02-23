#include "TPlayer.h"
#include "TServer.h"
#include "CFileSystem.h"

bool TPlayer::msgPLI_REQUESTTEXT(CString& pPacket)
{
	// TODO(joey): So I believe these are just requests for information, while sendtext is used to actually do things.

	CString packet = pPacket.readString("");
	CString data = packet.guntokenize();

	CString weapon = data.readString("\n");
	CString type = data.readString("\n");
	CString option = data.readString("\n");

	auto& list = server->getServerList();
	if (type == "lister")
	{
		if (option == "simplelist")
			list.sendPacket({SVO_REQUESTLIST, CString() >> (short)id << CString(weapon << "\n" << type << "\n" << "simpleserverlist" << "\n").gtokenizeI()});
		else if (option == "rebornlist")
			list.sendPacket({SVO_REQUESTLIST, CString() >> (short)id << packet});
		else if (option == "subscriptions") {
			// some versions of the loginserver scripts expected the response of subscriptions2 rather than subscriptions
			sendPacket({PLO_SERVERTEXT, CString() << CString(CString() << weapon << "\n" << type << "\n" << "subscriptions" << "\n" << CString(CString() << "unlimited" << "\n" << "Unlimited Subscription" << "\n" << "\"\"" << "\n").gtokenizeI()).gtokenizeI()});
		}
		else if (option == "bantypes")
			sendPacket({PLO_SERVERTEXT, CString() << packet << ",\"\"\"Event Interruption\"\",259200\",\"\"\"Message Code Abuse\"\",259200\",\"\"\"General Scamming\"\",604800\",\"Advertising,604800\",\"\"\"General Harassment\"\",604800\",\"\"\"Racism or Severe Vulgarity\"\",1209600\",\"\"\"Sexual Harassment\"\",1209600\",\"Cheating,2592000\",\"\"\"Advertising Money Trade\"\",2592000\",\"\"\"Ban Evasion\"\",2592000\",\"\"\"Speed Hacking\"\",2592000\",\"\"\"Bug Abuse\"\",2592000\",\"\"\"Multiple Jailings\"\",2592000\",\"\"\"Server Destruction\"\",3888000\",\"\"\"Leaking Information\"\",3888000\",\"\"\"Account Scam\"\",7776000\",\"\"\"Account Sharing\"\",315360000\",\"Hacking,315360000\",\"\"\"Multiple Bans\"\",315360000\",\"\"\"Other Unlimited\"\",315360001\""});
		else if (option == "getglobalitems")
			sendPacket({PLO_SERVERTEXT, CString() << CString(weapon << "\n" << type << "\n" << "globalitems" << "\n" << accountName.text() << "\n" << CString(CString(CString() << "autobill=1"  << "\n" << "autobillmine=1"  << "\n" << "bundle=1"  << "\n" << "creationtime=1212768763"  << "\n" << "currenttime=1353248504"  << "\n" << "description=Gives" << "\n" << "duration=2629800"  << "\n" << "flags=subscription"  << "\n" << "icon=graalicon_big.png"  << "\n" << "itemid=1"  << "\n" << "lifetime=1"  << "\n" << "owner=global"  << "\n" << "ownertype=server"  << "\n" << "price=100"  << "\n" << "quantity=988506"  << "\n" << "status=available"  << "\n" << "title=Gold"  << "\n" << "tradable=1"  << "\n" << "typeid=62"  << "\n" << "world=global"  << "\n").gtokenizeI()).gtokenizeI()).gtokenizeI()});
		else if (option == "serverinfo") {
			list.sendPacket({SVO_REQUESTSVRINFO, CString() >> (short)id << packet});
		}

	}
	else if (type == "pmservers" || type == "pmguilds") {
		list.sendPacket({SVO_REQUESTLIST, CString() >> (short)id << packet});
	}
	else if (type == "pmserverplayers")
		addPMServer(option);
	else if (type == "pmunmapserver")
		remPMServer(option);
	else if (type == "irc") {
	} else if (type == "packageinfo") {
		std::vector<CString> updatePackage = server->getFileSystem()->load(option).tokenize("\n");
		int files = 0;
		int totalFileSize = 0;
		for (const auto& line : updatePackage) {
			if (line.findi("FILE") > -1) {
				CString file = line.subString(line.findi("FILE") + 5);
				totalFileSize += server->getFileSystem()->getFileSize(file.trimI());
				files++;
			}
		}
		sendPacket({PLO_SERVERTEXT, CString() << CString(weapon << "\n" << type << "\n" << option << "\n" << /* File count */ CString(files) << "\n" << /* Total size in bytes */ CString(totalFileSize) << "\n").gtokenizeI()});
	}


	server->getServerLog().out("[ IN] [RequestText] from %s -> %s\n", accountName.gtokenize().text(),packet.text());
	return true;
}

bool TPlayer::msgPLI_SENDTEXT(CString& pPacket)
{
	CString packet = pPacket.readString("");
	CString data = packet.guntokenize();
	std::vector<CString> params = data.tokenize("\n");

	CString weapon = data.readString("\n");
	CString type = data.readString("\n");
	CString option = data.readString("\n");
	std::vector<CString> params2 = data.readString("").tokenize("\n");

	auto& list = server->getServerList();

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
					sendPacket({PLO_ADDPLAYER, CString() >> (short)(16000 + 0) >> (char)channelAccount.length() << channelAccount >> (char)PLPROP_NICKNAME >> (char)channelNick.length() << channelNick >> (char)PLPROP_UNKNOWN81 >> (char)3});
				}
				else sendPacket({PLO_OTHERPLPROPS, CString() >> (short)(16000 + 0) >> (char)PLPROP_ACCOUNTNAME >> (char)channelAccount.length() << channelAccount >> (char)PLPROP_NICKNAME >> (char)channelNick.length() << channelNick >> (char)PLPROP_UNKNOWN81 >> (char)3});
			}
			else if (params.size() > 3)
			{
				if (option == "join")
				{
					CString channel = params[3];
					CString sendMsg = "GraalEngine,irc,join,";
					sendMsg << channel.gtokenize();
					list.sendTextForPlayer(shared_from_this(), sendMsg);
				}
				else if (option == "part")
				{
					CString channel = params[3];
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
					CString channel = params[3];
					CString msg = params[4];

					if (channel == "IRCBot")
					{
						std::vector<CString> params3 = msg.guntokenize().tokenize("\n");
						if (params3[0] == "!getserverinfo")
						{
							//list->sendPacket(CString() >> (char)SVO_REQUESTSVRINFO >> (short)id << weapon << ",irc,privmsg," << params3[1].gtokenize());
							server->getServerLog().out("[ IN] [SVO_SERVERINFO] %s,%s\n", accountName.gtokenize().text(), packet.text());
							//list->sendPacket(CString() >> (char)SVO_SERVERINFO >> (short)id << params3[1]); // <-- this solves it for now

							// I believe the following data is what it's looking for:
							// "era,Era,93,English,""Welcome to Era, a modernised server. Please visit the website for more information."",http://era.graal.net/,""Graal 5.1-5.2"""
						}
					}
					else
					{
						CString sendMsg = "GraalEngine,irc,privmsg,";
						sendMsg << accountName << "," << channel.gtokenize() << "," << msg.gtokenize();
						list.handleText(sendMsg);
						list.sendTextForPlayer(shared_from_this(), sendMsg);
					}
				}
			}
		}
		else if (type == "lister")
		{
			if (option == "serverinfo")
				list.sendPacket({SVO_REQUESTSVRINFO, CString() >> (short)id << packet});

			if (!getGuest())
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
					sendPacket({PLO_SERVERTEXT, CString() << "GraalEngine,lister,ban," << params[0] << "," << "0"});
					//msgPLI_RC_PLAYERBANGET(params[0]);
				}
			}
		}
	}

	server->getServerLog().out("[ IN] [SendText] %s: %s\n", accountName.gtokenize().text(), packet.text());

	return true;
}
