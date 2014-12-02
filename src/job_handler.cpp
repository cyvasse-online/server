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

#include "job_handler.hpp"

#include <chrono>
#include <random>
#include <set>
#include <stdexcept>
#include <json/value.h>
#include <json/reader.h>
#include <json/writer.h>
#include <tntdb/error.h>
#include <cyvdb/match_manager.hpp>
#include <cyvdb/player_manager.hpp>
#include <cyvmath/match.hpp>
#include <cyvmath/player.hpp>
#include <cyvmath/rule_set_create.hpp>
#include <cyvws/game_msg_action.hpp>
#include <cyvws/msg_type.hpp>
#include <cyvws/notification_type.hpp>
#include <cyvws/server_request_action.hpp>
#include "b64.hpp"
#include "cyvasse_server.hpp"
#include "client_data.hpp"
#include "match_data.hpp"

using namespace cyvmath;
using namespace std::chrono;

JobHandler::JobHandler(CyvasseServer& server)
	: m_server(server)
	, m_thread(std::bind(&JobHandler::processMessages, this))
{ }

JobHandler::~JobHandler()
{
	m_thread.join();
}

void JobHandler::processMessages()
{
	typedef CyvasseServer::Job Job;
	using namespace websocketpp::frame;

	auto& clientDataSets = m_server.m_clientDataSets;
	auto& matches = m_server.m_matches;
	auto& server = m_server.m_wsServer;

	Json::Reader reader;
	Json::FastWriter writer;

	while(m_server.m_running)
	{
		std::unique_lock<std::mutex> jobLock(m_server.m_jobMtx);

		while(m_server.m_running && m_server.m_jobQueue.empty())
			m_server.m_jobCond.wait(jobLock);

		if(!m_server.m_running)
			break;

		std::unique_ptr<Job> job = std::move(m_server.m_jobQueue.front());
		m_server.m_jobQueue.pop();

		jobLock.unlock();

		try
		{
			// Process job
			Json::Value recvdJson;
			if(!reader.parse(job->second->get_payload(), recvdJson, false))
			{
				// TODO: reply with error message
				return;
			}

			std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> otherClients;

			std::shared_ptr<ClientData> clientData;

			std::unique_lock<std::mutex> matchDataLock(m_server.m_matchDataMtx, std::defer_lock); // don't lock yet
			std::unique_lock<std::mutex> clientDataLock(m_server.m_clientDataMtx);

			auto it1 = clientDataSets.find(job->first);
			if(it1 != clientDataSets.end())
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
					std::string json = writer.write(recvdJson);
					for(auto hdl : otherClients)
						server.send(hdl, json, opcode::text);

					break;
				}
				case MsgType::SERVER_REQUEST:
				{
					Json::Value reply;

					auto& requestData = recvdJson["requestsData"];

					reply["msgType"] = MsgTypeToStr(MsgType::SERVER_REPLY);
					// cast to int and back to not allow any non-numeral data
					reply["msgID"] = recvdJson["msgID"].asInt();

					auto& replyData = reply["replyData"];
					replyData["success"] = true;

					auto setError = [&replyData](const std::string& error) {
							replyData["success"] = false;
							replyData["error"] = error;
						};

					switch(StrToServerRequestAction(requestData["action"].asString()))
					{
						case ServerRequestAction::INIT_COMM:
						{
							// TODO
							break;
						}
						case ServerRequestAction::CREATE_GAME:
						{
							if(clientData)
								setError("This connection is already in use for a running match");
							else
							{
								auto ruleSet = StrToRuleSet(requestData["ruleSet"].asString());
								auto color   = StrToPlayersColor(requestData["color"].asString());
								auto random  = requestData["random"].asBool();
								auto _public = requestData["public"].asBool();

								auto matchID  = newMatchID();
								auto playerID = newPlayerID();

								auto newMatchData = std::make_shared<MatchData>(createMatch(ruleSet, matchID));
								auto newClientData = std::make_shared<ClientData>(
									createPlayer(newMatchData->getMatch(), color, playerID),
									job->first, *newMatchData
								);

								newMatchData->getClientDataSets().insert(newClientData);

								clientDataLock.lock();
								auto tmp1 = clientDataSets.emplace(job->first, newClientData);
								clientDataLock.unlock();
								assert(tmp1.second);

								matchDataLock.lock();
								auto tmp2 = matches.emplace(matchID, newMatchData);
								matchDataLock.unlock();
								assert(tmp2.second);

								replyData["matchID"]  = matchID;
								replyData["playerID"] = playerID;

								// TODO
								std::thread([color, ruleSet, random, _public, matchID, playerID]() {
									std::this_thread::sleep_for(milliseconds(50));

									auto match = createMatch(ruleSet, matchID, random, _public);
									auto player = createPlayer(*match, color, playerID);

									cyvdb::MatchManager().addMatch(std::move(match));
									cyvdb::PlayerManager().addPlayer(std::move(player));
								}).detach();
							}
							break;
						}
						case ServerRequestAction::JOIN_GAME:
						{
							matchDataLock.lock();

							auto matchIt = matches.find(requestData["matchID"].asString());

							if(clientData)
								setError("This connection is already in use for a running match");
							else if(matchIt == matches.end())
								setError("Game not found");
							else
							{
								auto matchData = matchIt->second;
								auto matchClients = matchData->getClientDataSets();

								if(matchClients.size() == 0)
									setError("You tried to join a game without an active player, this doesn't work yet");
								else if(matchClients.size() > 1)
									setError("This game already has two players");
								else
								{
									auto ruleSet  = matchData->getMatch().getRuleSet();
									auto color    = !(*matchClients.begin())->getPlayer().getColor();
									auto matchID  = requestData["matchID"].asString();
									auto playerID = newPlayerID();

									auto newClientData = std::make_shared<ClientData>(
										createPlayer(matchData->getMatch(), color, playerID),
										job->first, *matchData
									);

									matchData->getClientDataSets().insert(newClientData);

									clientDataLock.lock();
									auto tmp = clientDataSets.emplace(job->first, newClientData);
									clientDataLock.unlock();
									assert(tmp.second);

									replyData["success"]  = true;
									replyData["color"]    = PlayersColorToStr(color);
									replyData["playerID"] = playerID;
									replyData["ruleSet"]  = RuleSetToStr(ruleSet);

									Json::Value notification;
									notification["msgType"] = "notification";
									// TODO: Remove msgID from the examples or use it?
									auto& notificationData = notification["notificationData"];
									notificationData["type"] = NotificationTypeToStr(NotificationType::USER_JOINED);
									//notificationData["screenName"] =
									//notificationData["registered"] =
									//notificationData["role"] =

									for(auto& clientIt : matchClients)
										server.send(clientIt->getConnHdl(), writer.write(notification), opcode::text);

									std::thread([ruleSet, color, matchID, playerID]() {
										std::this_thread::sleep_for(milliseconds(50));

										// TODO
										auto match = createMatch(ruleSet, matchID);
										cyvdb::PlayerManager().addPlayer(createPlayer(*match, color, playerID));
									}).detach();
								}
							}

							matchDataLock.unlock();

							break;
						}
						case ServerRequestAction::SUBSCR_GAME_LIST_UPDATES:
						case ServerRequestAction::UNSUBSCR_GAME_LIST_UPDATES:
						{
							// TODO
							break;
						}
						default:
							setError("unrecognized server request action");
					}

					server.send(job->first, writer.write(reply), opcode::text);

					break;
				}
				// case MsgType::NOTIFICATION:
				// case MsgType::SERVER_REPLY:
				// case MsgType::UNDEFINED:
				default: // TODO: reply with error message
					break;
			}
		}
		catch(std::error_code& e)
		{
			std::cerr << "Caught a std::error_code\n-----\n" << e << '\n' << e.message() << std::endl;
		}
		catch(std::exception& e)
		{
			std::cerr << "Caught a std::exception: " << e.what() << std::endl;
		}
		catch(...)
		{
			std::cerr << "Caught an unrecognized error (not derived from either std::exception or std::error_code)"
				<< std::endl;
		}
	}
}

std::string JobHandler::newMatchID()
{
	static std::ranlux24 int24Generator(system_clock::now().time_since_epoch().count());

	std::string res;
	std::unique_lock<std::mutex> matchDataLock(m_server.m_matchDataMtx);

	do
	{
		res = int24ToB64ID(int24Generator());
	}
	while(m_server.m_matches.find(res) != m_server.m_matches.end());

	matchDataLock.unlock();

	return res;
}

std::string JobHandler::newPlayerID()
{
	static std::ranlux48 int48Generator(system_clock::now().time_since_epoch().count());

	// TODO
	return int48ToB64ID(int48Generator());
}
