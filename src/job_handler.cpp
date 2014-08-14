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
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/writer.h>
#include <tntdb/connect.h>
#include <tntdb/error.h>
#include <server_message.hpp>
#include <cyvmath/match.hpp>
#include <cyvmath/player.hpp>
#include <cyvmath/rule_set_create.hpp>
#include "b64.hpp"
#include "cyvasse_server.hpp"
#include "client_data.hpp"
#include "match_data.hpp"

using namespace cyvmath;

JobHandler::JobHandler(CyvasseServer& server)
	: _server(server)
	, _thread(std::bind(&JobHandler::processMessages, this))
{ }

JobHandler::~JobHandler()
{
	_thread.join();
}

void JobHandler::processMessages()
{
	typedef CyvasseServer::Job Job;
	using namespace websocketpp::frame;

	auto& clientDataSets = _server._clientDataSets;
	auto& matches = _server._matches;
	auto& server = _server._wsServer;

	Json::Reader reader;
	Json::FastWriter writer;

	while(_server._running)
	{
		std::unique_lock<std::mutex> jobLock(_server._jobMtx);

		while(_server._running && _server._jobQueue.empty())
			_server._jobCond.wait(jobLock);

		if(!_server._running)
			break;

		std::unique_ptr<Job> job = std::move(_server._jobQueue.front());
		_server._jobQueue.pop();

		jobLock.unlock();

		try
		{
			// Process job
			Json::Value msgData;
			if(!reader.parse(job->second->get_payload(), msgData, false))
			{
				// TODO: reply with error message
				return;
			}

			std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> otherClients;

			std::shared_ptr<ClientData> clientData;

			std::unique_lock<std::mutex> matchDataLock(_server._matchDataMtx, std::defer_lock); // don't lock yet
			std::unique_lock<std::mutex> clientDataLock(_server._clientDataMtx);

			auto it1 = clientDataSets.find(job->first);
			if(it1 != clientDataSets.end())
			{
				clientData = it1->second;
				clientDataLock.unlock();

				for(auto it2 : clientData->getMatchData().getClientDataSets())
				{
					if(*it2 != *clientData)
						otherClients.insert(it2->getConnHdl());
				}
			}
			else clientDataLock.unlock();

			switch(StrToMessage(msgData["messageType"].asString()))
			{
				case Message::REQUEST:
				{
					auto& param = msgData["param"];

					Json::Value reply;
					reply["messageType"] = MessageToStr(Message::REPLY);
					// cast to int and back to not allow any non-numeral data
					reply["messageID"] = msgData["messageID"].asInt();

					auto setError = [&](const std::string& error) {
							reply["success"] = false;
							reply["error"] = error;
						};

					switch(StrToAction(msgData["action"].asString()))
					{
						case Action::CREATE_GAME:
						{
							if(clientData)
								setError("This connection is already in use for a running match");
							else
							{
								try
								{
									auto matchID = newMatchID();
									auto playerID = newPlayerID();
									auto ruleSet = StrToRuleSet(param["ruleSet"].asString());

									auto matchData = std::make_shared<MatchData>(matchID, ruleSet, createMatch(ruleSet));

									auto clientData = std::make_shared<ClientData>(
										playerID,
										createPlayer(StrToPlayersColor(param["color"].asString()), *matchData->getMatch()),
										job->first,
										*matchData
									);

									matchData->getClientDataSets().insert(clientData);

									clientDataLock.lock();
									auto tmp1 = clientDataSets.emplace(job->first, clientData);
									clientDataLock.unlock();
									assert(tmp1.second);

									matchDataLock.lock();
									auto tmp2 = matches.emplace(matchID, matchData);
									matchDataLock.unlock();
									assert(tmp2.second);

									reply["success"] = true;
									reply["data"]["matchID"]  = matchID;
									reply["data"]["playerID"] = playerID;
								}
								catch(tntdb::Error& e)
								{
									setError(std::string("SQL error: ") + e.what());
								}
							}
							break;
						}
						case Action::JOIN_GAME:
						{
							matchDataLock.lock();
							auto it = matches.find(param["matchID"].asString());

							if(clientData)
								setError("This connection is already in use for a running match");
							else if(it == matches.end())
								setError("Game not found");
							else
							{
								auto matchID = param["matchID"].asString();
								auto playerID = newPlayerID();

								auto matchData = it->second;
								auto matchClients = matchData->getClientDataSets();

								if(matchClients.size() == 0)
									setError("You tried to join a game without an active player, this doesn't work yet");
								else if(matchClients.size() > 1)
									setError("This game already has two players");
								else
								{
									auto color = !(*matchClients.begin())->getPlayer()->getColor();

									auto clientData = std::make_shared<ClientData>(
										playerID,
										createPlayer(color, *matchData->getMatch()),
										job->first,
										*matchData
									);

									matchData->getClientDataSets().insert(clientData);

									clientDataLock.lock();
									auto tmp = clientDataSets.emplace(job->first, clientData);
									clientDataLock.unlock();
									assert(tmp.second);

									reply["success"] = true;
									reply["data"]["ruleSet"]  = RuleSetToStr(matchData->getRuleSet());
									reply["data"]["color"]    = PlayersColorToStr(color);
									reply["data"]["playerID"] = playerID;

									auto message = PlayersColorToStr(color) + " player joined.";
									message[0] -= ('a' - 'A'); // lowercase to uppercase

									Json::Value chatMsg;
									chatMsg["messageType"]      = "request";
									chatMsg["action"]           = "chat message";
									chatMsg["param"]["sender"]  = "Server";
									chatMsg["param"]["message"] = message;

									std::string json = writer.write(chatMsg);
									for(auto& it : matchClients)
										server.send(it->getConnHdl(), json, opcode::text);
								}
							}

							matchDataLock.unlock();

							break;
						}
						case Action::RESUME_GAME:
							// TODO
							break;
						case Action::CHAT_MSG:
						{
							std::string json = writer.write(msgData);
							for(auto hdl : otherClients)
								server.send(hdl, json, opcode::text);

							break;
						}
						default:
							setError("unrecognized action type");
					}

					server.send(job->first, writer.write(reply), opcode::text);

					break;
				}
				case Message::REPLY:
				{
					if(!msgData["success"].asBool())
					{
						msgData["error"] = msgData["error"].asString().empty() ?
							"The opponents client sent an error message without any detail" :
							"The opponents client sent the following error message: " + msgData["error"].asString();
					}

					std::string json = writer.write(msgData);
					for(auto hdl : otherClients)
						server.send(hdl, json, opcode::text);

					break;
				}
				case Message::GAME_UPDATE:
				{
					std::string json = writer.write(msgData);
					for(auto hdl : otherClients)
						server.send(hdl, json, opcode::text);

					break;
				}
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

using std::chrono::system_clock;

std::string JobHandler::newMatchID()
{
	static std::ranlux24 int24Generator(system_clock::now().time_since_epoch().count());

	std::string res;
	std::unique_lock<std::mutex> matchDataLock(_server._matchDataMtx);

	do
	{
		res = int24ToB64ID(int24Generator());
	}
	while(_server._matches.find(res) != _server._matches.end());

	matchDataLock.unlock();

	return res;
}

std::string JobHandler::newPlayerID()
{
	static std::ranlux48 int48Generator(system_clock::now().time_since_epoch().count());

	// TODO
	return int48ToB64ID(int48Generator());
}
