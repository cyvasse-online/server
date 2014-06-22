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

#include <set>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include "b64.hpp"
#include "cyvasse_server.hpp"

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

	auto& connectionMatches = _server._connectionMatches;
	auto& matchConnections = _server._matchConnections;
	auto& server = _server._wsServer;

	Json::FastWriter jsonWriter;

	while(_server._running)
	{
		std::unique_lock<std::mutex> lock(_server._jobMtx);

		while(_server._running && _server._jobQueue.empty())
			_server._jobCond.wait(lock);

		if(!_server._running)
			break;

		std::unique_ptr<Job> job = std::move(_server._jobQueue.front());
		_server._jobQueue.pop();

		lock.unlock();

		// Process job
		JobData data;
		try
		{
			data.deserialize(job->second->get_payload());
		}
		catch(JobDataParseError& e)
		{
			switch(data.messageType)
			{
				case MESSAGE_REQUEST:
					// reply with an error message
					break;
				case MESSAGE_REPLY:
					// send an error message to the other player
					break;
				// default: do nothing
			}
			return;
		}

		std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> otherPlayers;

		CyvasseServer::ConToMatchMap::const_iterator connectionMatchesIt;
		CyvasseServer::MatchToConMap::const_iterator matchConnectionsIt;

		connectionMatchesIt = connectionMatches.find(job->first);
		if(connectionMatchesIt != connectionMatches.end())
		{
			matchConnectionsIt = matchConnections.find(connectionMatchesIt->second);
			if(matchConnectionsIt != matchConnections.end())
			{
				for(websocketpp::connection_hdl hdl : matchConnectionsIt->second)
				{
					// owner-based ==
					if(!hdl.owner_before(job->first) && !job->first.owner_before(hdl))
						otherPlayers.insert(hdl);
				}
			}
		}

		switch(data.messageType)
		{
			case MESSAGE_REQUEST:
			{
				// reply object is created for every request but is discarded if action
				// is not {create, join, resume} game and it doesn't contain any error
				Json::Value reply;
				reply["messageType"] = "reply";
				reply["messageID"] = data.messageID;

				auto setError = [&](std::string error) {
						reply["success"] = false;
						reply["error"] = error;
					};

				std::unique_lock<std::mutex> lock(_server._connMapMtx);
				switch(data.requestData.action)
				{
					case ACTION_CREATE_GAME:
					{
						if(connectionMatchesIt != connectionMatches.end())
							setError("This connection is already in use for a running match");
						else
						{
							std::string matchID(int24ToB64ID(_server._int24Generator()));

							auto tmp1 = connectionMatches.emplace(job->first, matchID);
							auto tmp2 = matchConnections.emplace(matchID, std::vector<websocketpp::connection_hdl>());

							// both emplacements must be successful when the previous `break` wasn't run
							assert(tmp1.second);
							assert(tmp2.second);

							// Add the connection handle of the job to the new _matchConnections entries vector
							tmp2.first->second.push_back(job->first);

							std::string playerID(int48ToB64ID(_server._int48Generator()));

							reply["success"]  = true;
							reply["data"]["matchID"]  = matchID;
							reply["data"]["playerID"] = playerID;
						}
						break;
					}
					case ACTION_JOIN_GAME:
					{
						auto it = matchConnections.find(data.requestData.joinGame.matchID);

						if(connectionMatchesIt != connectionMatches.end())
							setError("This connection is already in use for a running match");
						else if(it == matchConnections.end())
							setError("Game not found");
						else
						{
							auto tmp = connectionMatches.emplace(job->first, data.requestData.joinGame.matchID);
							assert(tmp.second);

							it->second.push_back(job->first);

							std::string playerID(int48ToB64ID(_server._int48Generator()));

							reply["success"] = true;
							reply["data"]["playerID"] = playerID;
						}
						break;
					}
					case ACTION_RESUME_GAME:
						break;
					/*case ACTION_START:
						break;
					case ACTION_MOVE_PIECE:
						break;
					case ACTION_RESIGN:
						break;
					default:
						assert(0);
						break;*/
					default:
					{
						if(connectionMatchesIt == connectionMatches.end())
							setError("Could not find the match this message belongs to");
						else if(matchConnectionsIt == matchConnections.end())
							setError("Game this message belongs to not found");
						else if(otherPlayers.empty())
							setError("No other players in this match");
						else
						{
							for(auto hdl : otherPlayers)
							{
								server.send(hdl, jsonWriter.write(data.serialize()), opcode::text);
							}
							return; // don't send a reply to the requesting client
						}

						break;
					}
				}
				server.send(job->first, jsonWriter.write(reply), opcode::text);
				break;
			}
			case MESSAGE_REPLY:
			{
				if(data.replyData.success)
				{
					server.send(job->first, jsonWriter.write(data.serialize()), opcode::text);
				}
				else
				{
					data.replyData.error = data.replyData.error.empty() ?
						"The opponents client sent an error message without any detail" :
						"The opponents client sent the following error message: " + data.replyData.error;
				}
			}
			default: assert(0);
		}
	}
}
