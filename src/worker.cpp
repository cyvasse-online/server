/* Copyright 2014 Jonas Platte
 *
 * This file is part of Cyvasse Online.
 *
 * Cyvasse Online is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * Cyvasse Online is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include "worker.hpp"

#include <chrono>
#include <random>
#include <set>
#include <stdexcept>
#include <json/value.h>
#include <json/reader.h>
#include <json/writer.h>
#include <tntdb/error.h>
//#include <cyvdb/match_manager.hpp>
//#include <cyvdb/player_manager.hpp>
#include <cyvmath/match.hpp>
#include <cyvmath/player.hpp>
#include <cyvmath/rule_set_create.hpp>
#include <cyvws/game_msg_action.hpp>
#include <cyvws/msg_type.hpp>
#include <cyvws/notification_type.hpp>
#include <cyvws/server_request_action.hpp>
#include "b64.hpp"
#include "client_data.hpp"
#include "match_data.hpp"

using namespace cyvmath;
using namespace cyvws;

using namespace std;
using namespace std::chrono;
using namespace websocketpp;

Worker::Worker(SharedServerData& data, send_func_type sendFunc)
	: m_data{data}
	, m_sendFunc{sendFunc}
	, m_thread{bind(&Worker::processMessages, this)}
{ }

Worker::~Worker()
{
	m_thread.join();
}

void Worker::send(connection_hdl hdl, const string& data)
{
	m_sendFunc(hdl, data, frame::opcode::text);
}

void Worker::send(connection_hdl hdl, const Json::Value& data)
{
	send(hdl, Json::FastWriter().write(data));
}

void Worker::sendCommErr(connection_hdl hdl, const string& errMsg)
{
	Json::Value data;
	data["msgType"] = "notification";
	data["notificationData"]["type"] = "commError";
	data["notificationData"]["errMsg"] = errMsg;

	send(hdl, data);
}

void Worker::processMessages()
{
	Json::Reader reader;

	while(m_data.running)
	{
		unique_lock<mutex> jobLock(m_data.jobMtx);

		while(m_data.running && m_data.jobQueue.empty())
			m_data.jobCond.wait(jobLock);

		if(!m_data.running)
			break;

		auto job = m_data.jobQueue.front();
		m_data.jobQueue.pop();

		jobLock.unlock();

		try
		{
			// Process job
			Json::Value recvdJson;
			if(!reader.parse(job.msg_ptr->get_payload(), recvdJson, false))
			{
				sendCommErr(job.conn_hdl, "Received message is no valid JSON");
				continue;
			}

			set<connection_hdl, owner_less<connection_hdl>> otherClients;

			shared_ptr<ClientData> clientData;

			unique_lock<mutex> matchDataLock(m_data.matchDataMtx, defer_lock); // don't lock yet
			unique_lock<mutex> clientDataLock(m_data.clientDataMtx);

			auto it1 = m_data.clientData.find(job.conn_hdl);
			if(it1 != m_data.clientData.end())
			{
				clientData = it1->second;
				clientDataLock.unlock();

				for(auto it2 : clientData->getMatchData().getClientDataSets())
					if(*it2 != *clientData)
						otherClients.insert(it2->getConnHdl());
			}
			else clientDataLock.unlock();

			switch(StrToMsgType(recvdJson["msgType"].asString()))
			{
				case MsgType::CHAT_MSG:
				case MsgType::CHAT_MSG_ACK:
				case MsgType::GAME_MSG:
				case MsgType::GAME_MSG_ACK:
				case MsgType::GAME_MSG_ERR:
				{
					// TODO: Does re-creating the Json string
					// from the Json::Value make sense or should
					// we just resend the original Json string?
					string json = Json::FastWriter().write(recvdJson);

					for(auto hdl : otherClients)
						send(hdl, json);

					break;
				}
				case MsgType::SERVER_REQUEST:
				{
					const auto& requestData = recvdJson["requestData"];
					const auto& param = requestData["param"];

					Json::Value reply;
					reply["msgType"] = MsgTypeToStr(MsgType::SERVER_REPLY);
					// cast to int and back to not allow any non-numeral data
					reply["msgID"] = recvdJson["msgID"].asInt();

					auto& replyData = reply["replyData"];
					replyData["success"] = true;

					// g++ warns about the default argument for the lambda with -pedantic,
					// maybe change how serverReply errors are sent to fix that?
					auto setError = [&replyData](const string& error, const string& errorDetails = {}) {
						replyData["success"] = false;
						replyData["error"] = error;

						if(!errorDetails.empty())
							replyData["errorDetails"] = errorDetails;
					};

					switch(StrToServerRequestAction(requestData["action"].asString()))
					{
						case ServerRequestAction::INIT_COMM:
						{
							// TODO: move to somewhere else (probably cyvws)
							constexpr unsigned short protocolVersionMajor = 1;

							const auto& versionStr = requestData["param"]["protocolVersion"].asString();

							if(versionStr.empty())
								sendCommErr(job.conn_hdl, "Expected non-empty string value as protocolVersion in initComm");
							else
							{
								if(stoi(versionStr.substr(0, versionStr.find('.'))) != protocolVersionMajor)
									setError("differingMajorProtVersion", "Expected major protocol version " + to_string(protocolVersionMajor));

								send(job.conn_hdl, reply);
							}
							break;
						}
						case ServerRequestAction::CREATE_GAME:
						{
							if(clientData)
							{
								setError("connInUse");
								send(job.conn_hdl, reply);
							}
							else
							{
								auto ruleSet = StrToRuleSet(param["ruleSet"].asString());
								auto color   = StrToPlayersColor(param["color"].asString());
								auto random  = param["random"].asBool();
								auto _public = param["public"].asBool();

								auto matchID  = newMatchID();
								auto playerID = newPlayerID();

								// TODO: Check whether all necessary parameters are set and valid

								auto newMatchData = make_shared<MatchData>(createMatch(ruleSet, matchID));
								auto newClientData = make_shared<ClientData>(
									createPlayer(newMatchData->getMatch(), color, playerID),
									job.conn_hdl, *newMatchData
								);

								newMatchData->getClientDataSets().insert(newClientData);

								clientDataLock.lock();
								auto tmp1 = m_data.clientData.emplace(job.conn_hdl, newClientData);
								clientDataLock.unlock();
								assert(tmp1.second);

								matchDataLock.lock();
								auto tmp2 = m_data.matchData.emplace(matchID, newMatchData);
								matchDataLock.unlock();
								assert(tmp2.second);

								replyData["matchID"]  = matchID;
								replyData["playerID"] = playerID;

								send(job.conn_hdl, reply);

								// TODO: Update randomGames / publicGames lists and
								// send a notification to the corresponding subscribers

								// TODO
								/*thread([=] {
									this_thread::sleep_for(milliseconds(50));

									auto match = createMatch(ruleSet, matchID, random, _public);
									auto player = createPlayer(*match, color, playerID);

									cyvdb::MatchManager().addMatch(move(match));
									cyvdb::PlayerManager().addPlayer(move(player));
								}).detach();*/
							}

							break;
						}
						case ServerRequestAction::JOIN_GAME:
						{
							matchDataLock.lock();

							auto matchIt = m_data.matchData.find(requestData["matchID"].asString());

							if(clientData)
								setError("connInUse");
							else if(matchIt == m_data.matchData.end())
								setError("gameNotFound");
							else
							{
								auto matchData = matchIt->second;
								auto matchClients = matchData->getClientDataSets();

								if(matchClients.size() == 0)
									setError("gameEmpty");
								else if(matchClients.size() > 1)
									setError("gameFull");
								else
								{
									auto ruleSet  = matchData->getMatch().getRuleSet();
									auto color    = !(*matchClients.begin())->getPlayer().getColor();
									auto matchID  = requestData["matchID"].asString();
									auto playerID = newPlayerID();

									auto newClientData = make_shared<ClientData>(
										createPlayer(matchData->getMatch(), color, playerID),
										job.conn_hdl, *matchData
									);

									matchData->getClientDataSets().insert(newClientData);

									clientDataLock.lock();
									auto tmp = m_data.clientData.emplace(job.conn_hdl, newClientData);
									clientDataLock.unlock();
									assert(tmp.second);

									replyData["success"]  = true;
									replyData["color"]    = PlayersColorToStr(color);
									replyData["playerID"] = playerID;
									replyData["ruleSet"]  = RuleSetToStr(ruleSet);

									Json::Value notification;
									notification["msgType"] = "notification";

									auto& notificationData = notification["notificationData"];
									notificationData["type"] = NotificationTypeToStr(NotificationType::USER_JOINED);
									//notificationData["screenName"] =
									//notificationData["registered"] =
									//notificationData["role"] =

									auto&& msg = Json::FastWriter().write(notification);

									for(auto& clientIt : matchClients)
										send(clientIt->getConnHdl(), msg);

									send(job.conn_hdl, reply);

									/*thread([=] {
										this_thread::sleep_for(milliseconds(50));

										// TODO
										auto match = createMatch(ruleSet, matchID);
										cyvdb::PlayerManager().addPlayer(createPlayer(*match, color, playerID));
									}).detach();*/
								}
							}

							matchDataLock.unlock();

							send(job.conn_hdl, reply);

							break;
						}
						case ServerRequestAction::SUBSCR_GAME_LIST_UPDATES:
						{
							// ignore param["ruleSet"] for now
							for (const auto& list : param["lists"])
							{
								const auto& listName = list.asString();

								if (listName == "openRandomGames")
									m_data.randomGamesSubscribers.insert(job.conn_hdl);
								else if (listName == "runningPublicGames")
									m_data.publicGamesSubscribers.insert(job.conn_hdl);
								//else
									// send error message
							}
						}
						case ServerRequestAction::UNSUBSCR_GAME_LIST_UPDATES:
						{
							// ignore param["ruleSet"] for now
							for (const auto& list : param["lists"])
							{
								const auto& listName = list.asString();

								if (listName == "openRandomGames")
								{
									const auto& it = m_data.randomGamesSubscribers.find(job.conn_hdl);

									if(it != m_data.randomGamesSubscribers.end())
										m_data.randomGamesSubscribers.erase(job.conn_hdl);
								}
								else if (listName == "runningPublicGames")
								{
									const auto& it = m_data.publicGamesSubscribers.find(job.conn_hdl);

									if(it != m_data.publicGamesSubscribers.end())
										m_data.publicGamesSubscribers.erase(job.conn_hdl);
								}
								//else
								//{
								//	send error message
								//}
							}
							break;
						}
						default:
							sendCommErr(job.conn_hdl, "Unrecognized server request action");
					}

					break;
				}
				case MsgType::NOTIFICATION:
				case MsgType::SERVER_REPLY:
					sendCommErr(job.conn_hdl, "This msgType is not intended for client-to-server messages");
					break;
				case MsgType::UNDEFINED:
					sendCommErr(job.conn_hdl, "No valid msgType found");
					break;
				default:
					// MsgType enum value valid, implementation lacks support for some feature
					assert(0);
					break;
			}
		}
		catch(std::error_code& e)
		{
			cerr << "Caught a std::error_code\n-----\n" << e << '\n' << e.message() << endl;
		}
		catch(std::exception& e)
		{
			cerr << "Caught a std::exception: " << e.what() << endl;
		}
		catch(...)
		{
			cerr << "Caught an unrecognized error (not derived from either exception or error_code)" << endl;
		}
	}
}

string Worker::newMatchID()
{
	static ranlux24 int24Generator(system_clock::now().time_since_epoch().count());

	string res;
	unique_lock<mutex> matchDataLock(m_data.matchDataMtx);

	do
	{
		res = int24ToB64ID(int24Generator());
	}
	while(m_data.matchData.find(res) != m_data.matchData.end());

	matchDataLock.unlock();

	return res;
}

string Worker::newPlayerID()
{
	static ranlux48 int48Generator(system_clock::now().time_since_epoch().count());

	// TODO
	return int48ToB64ID(int48Generator());
}
