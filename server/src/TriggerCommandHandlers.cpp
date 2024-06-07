#include "Level.h"
#include "NPC.h"
#include "Player.h"
#include "Server.h"
#include "Weapon.h"
#include "utilities/stringutils.h"

void TServer::createTriggerCommands(TriggerDispatcher::Builder builder)
{
	auto& dispatcher = triggerActionDispatcher;

#ifdef V8NPCSERVER
	builder.registerCommand("serverside", [&](TPlayer* player, std::vector<CString>& triggerData)
							{
								if (triggerData.size() > 1)
								{
									auto weaponObject = this->getWeapon(triggerData[1].toString());
									if (weaponObject != nullptr)
										weaponObject->queueWeaponAction(player, utilities::retokenizeArray(triggerData, 2));
								}

								return true;
							});

	builder.registerCommand("servernpc", [&](TPlayer* player, std::vector<CString>& triggerData)
							{
								if (triggerData.size() > 2)
								{
									auto npcObject = this->getNPCByName(triggerData[1].toString());
									if (npcObject != nullptr)
										npcObject->queueNpcTrigger(triggerData[2].toString(), player, utilities::retokenizeArray(triggerData, 3));
								}

								return true;
							});
#endif

	builder.registerCommand("gr.serverlist", [&](TPlayer* player, std::vector<CString>& triggerData)
							{
								auto& listServer       = getServerList();
								const auto& serverList = listServer.getServerList();

								CString actionData("clientside,-Serverlist_v4,updateservers,");
								for (auto& serverData: serverList)
									actionData << CString(serverData.first).gtokenize() << "," << CString(serverData.second) << ",";

								player->sendPacket(CString() >> (char)PLO_TRIGGERACTION >> (short)0 >> (int)0 >> (char)0 >> (char)0 << actionData);
								return true;
							});

	// Weapon management
	builder.registerCommand("gr.addweapon", [&](TPlayer* player, std::vector<CString>& triggerData)
							{
								if (getSettings().getBool("triggerhack_weapons", false))
								{
									for (auto i = 1; i < triggerData.size(); ++i)
										player->addWeapon(triggerData[i].trim().toString());
								}

								return true;
							});

	builder.registerCommand("gr.deleteweapon", [&](TPlayer* player, std::vector<CString>& triggerData)
							{
								if (getSettings().getBool("triggerhack_weapons", false))
								{
									for (auto i = 1; i < triggerData.size(); ++i)
										player->deleteWeapon(triggerData[i].trim().toString());
								}

								return true;
							});

	// Guild management
	builder.registerCommand("gr.addguildmember", [&](TPlayer* player, std::vector<CString>& triggerData)
							{
								if (getSettings().getBool("triggerhack_guilds", false))
								{
									CString guild, account, nick;
									if (triggerData.size() > 1) guild = triggerData[1];
									if (triggerData.size() > 2) account = triggerData[2];
									if (triggerData.size() > 3) nick = triggerData[3];

									if (!guild.isEmpty() && !account.isEmpty())
									{
										// Read the guild list.
										CFileSystem guildFS(this);
										guildFS.addDir("guilds");
										CString guildList = guildFS.load(CString() << "guild" << guild << ".txt");

										if (guildList.find(account) == -1)
										{
											if (guildList[guildList.length() - 1] != '\n') guildList << "\n";
											guildList << account;
											if (!nick.isEmpty()) guildList << ":" << nick;

											guildList.save(CString() << getServerPath() << "guilds/guild" << guild << ".txt");
										}
									}
								}

								return true;
							});

	builder.registerCommand("gr.removeguildmember", [&](TPlayer* player, std::vector<CString>& triggerData)
							{
								if (getSettings().getBool("triggerhack_guilds", false))
								{
									CString guild, account;
									if (triggerData.size() > 1) guild = triggerData[1];
									if (triggerData.size() > 2) account = triggerData[2];

									if (!guild.isEmpty() && !account.isEmpty())
									{
										// Read the guild list.
										CFileSystem guildFS(this);
										guildFS.addDir("guilds");
										CString guildList = guildFS.load(CString() << "guild" << guild << ".txt");

										if (guildList.find(account) != -1)
										{
											int pos    = guildList.find(account);
											int length = guildList.find("\n", pos) - pos;
											if (length < 0) length = -1;
											else
												++length;

											guildList.removeI(pos, length);
											guildList.save(CString() << getServerPath() << "guilds/guild" << guild << ".txt");
										}
									}
								}

								return true;
							});

	builder.registerCommand("gr.removeguild", [&](TPlayer* player, std::vector<CString>& triggerData)
							{
								if (getSettings().getBool("triggerhack_guilds", false))
								{
									CString guild;
									if (triggerData.size() > 1) guild = triggerData[1];

									if (!guild.isEmpty())
									{
										// Read the guild list.
										CFileSystem guildFS(this);
										guildFS.addDir("guilds");
										CString path = guildFS.find(CString() << "guild" << guild << ".txt");

										// Remove the guild.
										remove(path.text());

										// Remove the guild from all players.
										for (auto& [pid, p]: getPlayerList())
										{
											if (p->getGuild() == guild)
											{
												CString nick = p->getNickname();
												p->setNick(nick.readString("(").trimI());
												p->sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PLPROP_NICKNAME << p->getProp(PLPROP_NICKNAME));
												sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)p->getId() >> (char)PLPROP_NICKNAME << p->getProp(PLPROP_NICKNAME), { pid });
											}
										}
									}
								}

								return true;
							});

	builder.registerCommand("gr.setguild", [&](TPlayer* player, std::vector<CString>& triggerData)
							{
								if (getSettings().getBool("triggerhack_guilds", false))
								{
									CString guild, account;
									if (triggerData.size() > 1) guild = triggerData[1];
									if (triggerData.size() > 2) account = triggerData[2];

									if (!guild.isEmpty())
									{
										TPlayer* p = player;
										if (!account.isEmpty()) p = getPlayer(account, PLTYPE_ANYCLIENT).get();
										if (p)
										{
											CString nick = p->getNickname();
											p->setNick(CString() << nick.readString("(").trimI() << " (" << guild << ")", true);
											p->sendPacket(CString() >> (char)PLO_PLAYERPROPS >> (char)PLPROP_NICKNAME >> (char)p->getNickname().length() << p->getNickname());
											sendPacketToAll(CString() >> (char)PLO_OTHERPLPROPS >> (short)p->getId() >> (char)PLPROP_NICKNAME >> (char)p->getNickname().length() << p->getNickname(), { p->getId() });
										}
									}
								}

								return true;
							});

	// Group levels
	builder.registerCommand("gr.setgroup", [&](TPlayer* player, std::vector<CString>& triggerData)
							{
								if (getSettings().getBool("triggerhack_groups", true) && triggerData.size() == 2)
								{
									player->setGroup(triggerData[1]);
								}

								return true;
							});

	builder.registerCommand("gr.setlevelgroup", [&](TPlayer* player, std::vector<CString>& triggerData)
							{
								if (getSettings().getBool("triggerhack_groups", true) && triggerData.size() == 2)
								{
									auto playerList = player->getLevel()->getPlayerList();
									for (auto& id: playerList)
									{
										auto pl = getPlayer(id);
										pl->setGroup(triggerData[1]);
									}
								}

								return true;
							});

	builder.registerCommand("gr.setplayergroup", [&](TPlayer* player, std::vector<CString>& triggerData)
							{
								if (getSettings().getBool("triggerhack_groups", true) && triggerData.size() == 3)
								{
									auto player = getPlayer(triggerData[1], PLTYPE_ANYCLIENT);
									player->setGroup(triggerData[2]);
								}

								return true;
							});

	// RC triggers
	builder.registerCommand("gr.rcchat", [&](TPlayer* player, std::vector<CString>& triggerData)
							{
								if (getSettings().getBool("triggerhack_rc", false))
								{
									auto p = getPlayer(player->getId());

									CString msg;
									for (auto i = 1; i < triggerData.size(); ++i)
										msg << triggerData[i] << ",";
									sendToRC(msg, p);
								}

								return true;
							});

	// Level triggers
	builder.registerCommand("gr.npc.move", [&](TPlayer* player, std::vector<CString>& triggerData)
							{
								if (getSettings().getBool("triggerhack_levels", false) && triggerData.size() == 6)
								{
									unsigned int id = strtoint(triggerData[1]);
									int dx          = strtoint(triggerData[2]);
									int dy          = strtoint(triggerData[3]);
									float duration  = (float)strtofloat(triggerData[4]);
									int options     = strtoint(triggerData[5]);

									auto npc = getNPC(id);
									if (npc)
									{
										CString packet;
										packet >> (char)(npc->getX() / 8.0f) >> (char)(npc->getY() / 8.0f);
										packet >> (char)((dx * 2) + 100) >> (char)((dy * 2) + 100);
										packet >> (short)(duration / 0.05f);
										packet >> (char)options;
										sendPacketToLevelArea(CString() >> (char)PLO_MOVE >> (int)id << packet, getPlayer(player->getId()));

										npc->setX(npc->getX() + dx * 16);
										npc->setY(npc->getY() + dy * 16);
										//npc->setProps(CString() >> (char)NPCPROP_X >> (char)((npc->getX() + dx) * 2) >> (char)NPCPROP_Y >> (char)((npc->getY() + dy) * 2));
									}
								}

								return true;
							});

	builder.registerCommand("gr.npc.setpos", [&](TPlayer* player, std::vector<CString>& triggerData)
							{
								if (getSettings().getBool("triggerhack_levels", false) && triggerData.size() == 4)
								{
									unsigned int id = strtoint(triggerData[1]);
									float x         = (float)strtofloat(triggerData[2]);
									float y         = (float)strtofloat(triggerData[3]);

									auto npc = getNPC(id);
									if (npc)
									{
										npc->setX(int(x * 16.0));
										npc->setY(int(y * 16.0));

										// Send the prop packet to the level.
										CString packet;
										packet >> (char)NPCPROP_X >> (char)(x * 2.0f);
										packet >> (char)NPCPROP_Y >> (char)(y * 2.0f);
										sendPacketToLevelArea(CString() >> (char)PLO_NPCPROPS >> (int)id << packet, getPlayer(player->getId()));
									}
								}

								return true;
							});
}
