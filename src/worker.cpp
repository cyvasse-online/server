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
#include "cyvasse_server.hpp"
#include "b64.hpp"
#include "client_data.hpp"
#include "match_data.hpp"

using namespace cyvmath;
using namespace cyvws;

using namespace std;
using namespace std::chrono;
using namespace websocketpp;

Worker::Worker(CyvasseServer& server, SharedServerData& data)
	: m_server(server)
	, m_data(data)
	, m_thread(bind(&Worker::processMessages, this))
	, m_curMsgID{0}
{ }

Worker::~Worker()
{
	m_thread.join();
}

shared_ptr<ClientData> Worker::getClientData(connection_hdl hdl)
{
	shared_ptr<ClientData> ret;

	lock_guard<mutex> clientDataLock(m_data.clientDataMtx);

	auto it1 = m_data.clientData.find(hdl);
	if(it1 != m_data.clientData.end())
		ret = it1->second;

	return ret;
}

string Worker::newMatchID()
{
	static ranlux24 int24Generator(system_clock::now().time_since_epoch().count());

	string res;
	lock_guard<mutex> matchDataLock(m_data.matchDataMtx);

	do
	{
		res = int24ToB64ID(int24Generator());
	}
	while(m_data.matchData.find(res) != m_data.matchData.end());

	return res;
}

string Worker::newPlayerID()
{
	static ranlux48 int48Generator(system_clock::now().time_since_epoch().count());

	// TODO
	return int48ToB64ID(int48Generator());
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
			const Json::Value recvdJson = [&] {
				Json::Value ret;

				if(!reader.parse(job.msg_ptr->get_payload(), ret, false))
					m_server.sendCommErr(job.conn_hdl, "Received message is no valid JSON");

				return ret;
			}();

			if(recvdJson.isNull())
				continue;

			if(!recvdJson["msgID"].isNull())
				m_curMsgID = recvdJson["msgID"].asUInt();

			switch(StrToMsgType(recvdJson["msgType"].asString()))
			{
				case MsgType::CHAT_MSG:
				case MsgType::CHAT_MSG_ACK:
				case MsgType::GAME_MSG:
				case MsgType::GAME_MSG_ACK:
				case MsgType::GAME_MSG_ERR:
				{
					auto clientData = getClientData(job.conn_hdl);

					if(clientData)
					{
						set<connection_hdl, owner_less<connection_hdl>> otherClients;

						for(auto it2 : clientData->getMatchData().getClientDataSets())
							if(*it2 != *clientData)
								otherClients.insert(it2->getConnHdl());

						if(!otherClients.empty())
						{
							// TODO: Does re-creating the Json string
							// from the Json::Value make sense or should
							// we just resend the original Json string?
							string json = Json::FastWriter().write(recvdJson);

							for(auto&& hdl : otherClients)
								m_server.send(hdl, json);
						}
					}
					// else?

					break;
				}
				case MsgType::SERVER_REQUEST:
					processServerRequest(job.conn_hdl, recvdJson);
					break;
				case MsgType::NOTIFICATION:
				case MsgType::SERVER_REPLY:
					m_server.sendCommErr(job.conn_hdl, "This msgType is not intended for client-to-server messages");
					break;
				case MsgType::UNDEFINED:
					m_server.sendCommErr(job.conn_hdl, "No valid msgType found");
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

void Worker::processServerRequest(connection_hdl clientConnHdl, const Json::Value& recvdJson)
{
	const auto& requestData = recvdJson["requestData"];

	switch(StrToServerRequestAction(requestData["action"].asString()))
	{
		case ServerRequestAction::INIT_COMM:
			processInitCommRequest(clientConnHdl, requestData["param"]);
			break;
		case ServerRequestAction::CREATE_GAME:
			processCreateGameRequest(clientConnHdl, requestData["param"]);
			break;
		case ServerRequestAction::JOIN_GAME:
			processJoinGameRequest(clientConnHdl, requestData["param"]);
			break;
		case ServerRequestAction::SUBSCR_GAME_LIST_UPDATES:
			processSubscrGameListRequest(clientConnHdl, requestData["param"]);
			break;
		case ServerRequestAction::UNSUBSCR_GAME_LIST_UPDATES:
			processUnsubscrGameListRequest(clientConnHdl, requestData["param"]);
			break;
		default:
			m_server.sendCommErr(clientConnHdl, "Unrecognized server request action");
	}
}

void Worker::processInitCommRequest(connection_hdl clientConnHdl, const Json::Value& param)
{
	// TODO: move to somewhere else (probably cyvws)
	constexpr unsigned short protocolVersionMajor = 1;

	const auto& versionStr = param["protocolVersion"].asString();

	if(versionStr.empty())
		m_server.sendCommErr(clientConnHdl, "Expected non-empty string value as protocolVersion in initComm");
	else
	{
		if(stoi(versionStr.substr(0, versionStr.find('.'))) != protocolVersionMajor)
		{
			m_server.sendRequestErr(clientConnHdl, m_curMsgID, "differingMajorProtVersion",
				"Expected major protocol version " + to_string(protocolVersionMajor));
		}

		m_server.sendInitCommSuccess(clientConnHdl, m_curMsgID);
	}
}

void Worker::processCreateGameRequest(connection_hdl clientConnHdl, const Json::Value& param)
{
	if(getClientData(clientConnHdl))
		m_server.sendRequestErr(clientConnHdl, m_curMsgID, "connInUse");
	else
	{
		auto ruleSet = StrToRuleSet(param["ruleSet"].asString());
		auto color   = StrToPlayersColor(param["color"].asString());
		auto random  = param["random"].asBool();
		auto _public = param["public"].asBool();

		auto matchID  = newMatchID();
		auto playerID = newPlayerID();

		// TODO: Check whether all necessary parameters are set and valid

		auto matchData = make_shared<MatchData>(createMatch(ruleSet, matchID));
		auto clientData = make_shared<ClientData>(
			createPlayer(matchData->getMatch(), color, playerID),
			clientConnHdl, *matchData
		);

		matchData->getClientDataSets().insert(clientData);

		{
			lock_guard<mutex> clientDataLock(m_data.clientDataMtx);
			auto tmp = m_data.clientData.emplace(clientConnHdl, clientData);
			assert(tmp.second);
		}

		{
			lock_guard<mutex> matchDataLock(m_data.matchDataMtx);
			auto tmp = m_data.matchData.emplace(matchID, matchData);
			assert(tmp.second);
		}

		m_server.sendCreateGameSuccess(clientConnHdl, m_curMsgID, matchID, playerID);

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
}

void Worker::processJoinGameRequest(connection_hdl clientConnHdl, const Json::Value& param)
{
	unique_lock<mutex> matchDataLock(m_data.matchDataMtx);

	auto matchIt = m_data.matchData.find(param["matchID"].asString());

	if(getClientData(clientConnHdl))
		m_server.sendRequestErr(clientConnHdl, m_curMsgID, "connInUse");
	else if(matchIt == m_data.matchData.end())
		m_server.sendRequestErr(clientConnHdl, m_curMsgID, "gameNotFound");
	else
	{
		auto matchData = matchIt->second;
		auto matchClients = matchData->getClientDataSets();

		if(matchClients.size() == 0)
			m_server.sendRequestErr(clientConnHdl, m_curMsgID, "gameEmpty");
		else if(matchClients.size() > 1)
			m_server.sendRequestErr(clientConnHdl, m_curMsgID, "gameFull");
		else
		{
			auto ruleSet  = matchData->getMatch().getRuleSet();
			auto color    = !(*matchClients.begin())->getPlayer().getColor();
			auto matchID  = param["matchID"].asString();
			auto playerID = newPlayerID();

			auto clientData = make_shared<ClientData>(
				createPlayer(matchData->getMatch(), color, playerID),
				clientConnHdl, *matchData
			);

			matchData->getClientDataSets().insert(clientData);
			matchDataLock.unlock();

			{
				lock_guard<mutex> clientDataLock(m_data.clientDataMtx);
				auto tmp = m_data.clientData.emplace(clientConnHdl, clientData);
				assert(tmp.second);
			}

			Json::Value replyData;
			replyData["success"]  = true;
			replyData["color"]    = PlayersColorToStr(color);
			replyData["playerID"] = playerID;
			replyData["ruleSet"]  = RuleSetToStr(ruleSet);

			m_server.sendReply(clientConnHdl, m_curMsgID, replyData);

			Json::Value notification;
			notification["msgType"] = "notification";

			auto& notificationData = notification["notificationData"];
			notificationData["type"] = NotificationTypeToStr(NotificationType::USER_JOINED);
			//notificationData["screenName"] =
			//notificationData["registered"] =
			//notificationData["role"] =

			auto&& msg = Json::FastWriter().write(notification);

			for(auto& clientIt : matchClients)
				m_server.send(clientIt->getConnHdl(), msg);

			/*thread([=] {
				this_thread::sleep_for(milliseconds(50));

				// TODO
				auto match = createMatch(ruleSet, matchID);
				cyvdb::PlayerManager().addPlayer(createPlayer(*match, color, playerID));
			}).detach();*/
		}
	}
}

void Worker::processSubscrGameListRequest(connection_hdl clientConnHdl, const Json::Value& param)
{
	// ignore param["ruleSet"] for now
	for (const auto& list : param["lists"])
	{
		const auto& listName = list.asString();

		if (listName == "openRandomGames")
			m_data.randomGamesSubscribers.insert(clientConnHdl);
		else if (listName == "runningPublicGames")
			m_data.publicGamesSubscribers.insert(clientConnHdl);
		else
			m_server.sendRequestErr(clientConnHdl, m_curMsgID, "listDoesNotExist");
	}
}

void Worker::processUnsubscrGameListRequest(connection_hdl clientConnHdl, const Json::Value& param)
{
	// ignore param["ruleSet"] for now
	for (const auto& list : param["lists"])
	{
		const auto& listName = list.asString();

		if (listName == "openRandomGames")
		{
			const auto& it = m_data.randomGamesSubscribers.find(clientConnHdl);

			if(it != m_data.randomGamesSubscribers.end())
				m_data.randomGamesSubscribers.erase(clientConnHdl);
		}
		else if (listName == "runningPublicGames")
		{
			const auto& it = m_data.publicGamesSubscribers.find(clientConnHdl);

			if(it != m_data.publicGamesSubscribers.end())
				m_data.publicGamesSubscribers.erase(clientConnHdl);
		}
		else
			m_server.sendRequestErr(clientConnHdl, m_curMsgID, "listDoesNotExist");
	}
}
