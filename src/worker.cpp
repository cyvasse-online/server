/* Copyright 2014 - 2015 Jonas Platte
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
#include <map>
#include <random>
#include <set>
#include <stdexcept>

#include <json/value.h>
#include <json/reader.h>
#include <json/writer.h>
#include <tntdb/error.h>

#include <optional.hpp>
//#include <cyvdb/match_manager.hpp>
//#include <cyvdb/player_manager.hpp>
#include <cyvasse/match.hpp>
#include <cyvasse/player.hpp>
#include <cyvasse/piece.hpp>
#include <cyvws/chat_msg.hpp>
#include <cyvws/common.hpp>
#include <cyvws/game_msg.hpp>
#include <cyvws/init_comm.hpp>
#include <cyvws/json_game_msg.hpp>
#include <cyvws/json_notification.hpp>
#include <cyvws/json_server_reply.hpp>
#include <cyvws/msg.hpp>
#include <cyvws/notification.hpp>
#include <cyvws/server_reply.hpp>
#include <cyvws/server_request.hpp>

#include "cyvasse_server.hpp"
#include "b64.hpp"
#include "client_data.hpp"
#include "match_data.hpp"

using namespace cyvasse;
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
	if (it1 != m_data.clientData.end())
		ret = it1->second;

	return ret;
}

string Worker::newMatchID()
{
	static ranlux24 int24Generator(system_clock::now().time_since_epoch().count());

	string res;
	lock_guard<mutex> lock(m_data.matchDataMtx);

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

		if (!m_data.running)
			break;

		auto job = m_data.jobQueue.front();
		m_data.jobQueue.pop();

		jobLock.unlock();

		try
		{
			// Process job
			const Json::Value recvdJson = [&] {
				Json::Value ret;

				if (!reader.parse(job.msg_ptr->get_payload(), ret, false))
					m_server.send(job.conn_hdl, json::commErr("Received message is no valid JSON"));

				return ret;
			}();

			if (recvdJson.isNull())
				continue;

			if (!recvdJson[MSG_ID].isNull())
				m_curMsgID = recvdJson[MSG_ID].asUInt();

			const auto& msgType = recvdJson[MSG_TYPE].asString();

			if (msgType == MsgType::CHAT_MSG)
				processChatMsg(job.conn_hdl, recvdJson);
			else if (msgType == MsgType::GAME_MSG)
				processGameMsg(job.conn_hdl, recvdJson);
			else if (msgType == MsgType::CHAT_MSG_ACK ||
					msgType == MsgType::GAME_MSG_ACK ||
					msgType == MsgType::GAME_MSG_ERR)
				distributeMessage(job.conn_hdl, recvdJson);
			else if (msgType == MsgType::SERVER_REQUEST)
				processServerRequest(job.conn_hdl, recvdJson);
			else if (msgType ==  MsgType::NOTIFICATION || msgType == MsgType::SERVER_REPLY)
				m_server.send(job.conn_hdl, json::commErr("This msgType is not intended for client-to-server messages"));
			else
				m_server.send(job.conn_hdl, json::commErr("msgType \"" + msgType + "\" is invalid"));
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

void Worker::processServerRequest(connection_hdl clientConnHdl, const Json::Value& msg)
{
	const auto& requestData = msg[REQUEST_DATA];

	const auto& action = requestData[ACTION].asString();
	const auto& param  = requestData[PARAM];

	if      (action == ServerRequestAction::INIT_COMM)                  processInitCommRequest(clientConnHdl, param);
	else if (action == ServerRequestAction::CREATE_GAME)                processCreateGameRequest(clientConnHdl, param);
	else if (action == ServerRequestAction::JOIN_GAME)                  processJoinGameRequest(clientConnHdl, param);
	else if (action == ServerRequestAction::SUBSCR_GAME_LIST_UPDATES)   processSubscrGameListRequest(clientConnHdl, param);
	else if (action == ServerRequestAction::UNSUBSCR_GAME_LIST_UPDATES) processUnsubscrGameListRequest(clientConnHdl, param);
	else
		m_server.send(clientConnHdl, json::commErr("Unrecognized server request action"));
}

void Worker::processInitCommRequest(connection_hdl clientConnHdl, const Json::Value& param)
{
	// TODO: move to somewhere else (probably cyvws)
	constexpr unsigned short protocolVersionMajor = 1;

	const auto& versionStr = param[PROTOCOL_VERSION].asString();

	if (versionStr.empty())
		m_server.send(clientConnHdl, json::commErr("Expected non-empty string value as protocolVersion in initComm"));
	else
	{
		if (stoi(versionStr.substr(0, versionStr.find('.'))) != protocolVersionMajor)
		{
			m_server.send(clientConnHdl, json::requestErr(m_curMsgID, ServerReplyErrMsg::DIFF_MAJOR_PROT_V,
				"Expected major protocol version " + to_string(protocolVersionMajor)));
		}

		m_server.send(clientConnHdl, json::requestSuccess(m_curMsgID));
	}
}

void Worker::processCreateGameRequest(connection_hdl clientConnHdl, const Json::Value& param)
{
	if (getClientData(clientConnHdl))
		m_server.send(clientConnHdl, json::requestErr(m_curMsgID, ServerReplyErrMsg::CONN_IN_USE));
	else
	{
		//auto ruleSet = StrToRuleSet(param[RULE_SET].asString());
		auto color   = StrToPlayersColor(param[COLOR].asString());
		auto random  = param[RANDOM].asBool();
		//auto _public = param[PUBLIC].asBool(); // TODO

		auto matchID  = newMatchID();
		auto playerID = newPlayerID();

		// TODO: Check whether all necessary parameters are set and valid

		auto matchData = make_shared<MatchData>(matchID);
		auto clientData = make_shared<ClientData>(
			matchData->getMatch(), color, playerID, clientConnHdl, *matchData
		);

		matchData->getClientDataSets().insert(clientData);

		{
			lock_guard<mutex> lock(m_data.clientDataMtx);
			auto tmp = m_data.clientData.emplace(clientConnHdl, clientData);
			assert(tmp.second);
		}

		{
			lock_guard<mutex> lock(m_data.matchDataMtx);
			auto tmp = m_data.matchData.emplace(matchID, matchData);
			assert(tmp.second);
		}

		m_server.send(clientConnHdl, json::createGameSuccess(m_curMsgID, matchID, playerID));

		if (random)
		{
			{
				lock_guard<mutex> lock(m_data.gameListsMtx[RANDOM_GAMES]);
				// TODO: send a meaningful title instead of "A game"
				m_data.gameLists[RANDOM_GAMES].emplace(matchID, GamesListMappedType { "A game", !color });
			}

			m_server.listUpdated(RANDOM_GAMES);
		}

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

	auto matchIt = m_data.matchData.find(param[MATCH_ID].asString());

	if (getClientData(clientConnHdl))
		m_server.send(clientConnHdl, json::requestErr(m_curMsgID, ServerReplyErrMsg::CONN_IN_USE));
	else if (matchIt == m_data.matchData.end())
		m_server.send(clientConnHdl, json::requestErr(m_curMsgID, ServerReplyErrMsg::GAME_NOT_FOUND));
	else
	{
		auto matchData = matchIt->second;
		auto matchClients = matchData->getClientDataSets();

		if (matchClients.size() == 0)
			m_server.send(clientConnHdl, json::requestErr(m_curMsgID, ServerReplyErrMsg::GAME_EMPTY));
		else if (matchClients.size() > 1)
			m_server.send(clientConnHdl, json::requestErr(m_curMsgID, ServerReplyErrMsg::GAME_FULL));
		else
		{
			//auto ruleSet  = matchData->getMatch().getRuleSet();
			auto color    = !(*matchClients.begin())->getPlayer().getColor();
			auto matchID  = param[MATCH_ID].asString();
			auto playerID = newPlayerID();

			auto clientData = make_shared<ClientData>(
				matchData->getMatch(), color, playerID,
				clientConnHdl, *matchData
			);

			matchData->getClientDataSets().insert(clientData);
			matchDataLock.unlock();

			{
				lock_guard<mutex> lock(m_data.clientDataMtx);
				auto tmp = m_data.clientData.emplace(clientConnHdl, clientData);
				assert(tmp.second);
			}

			{ // TODO: moving this to cyvws seems like a good idea
				Json::Value replyData;
				replyData[SUCCESS]   = true;
				replyData[COLOR]     = PlayersColorToStr(color);
				replyData[PLAYER_ID] = playerID;
				//replyData[RULE_SET]  = RuleSetToStr(ruleSet);

				auto& match = matchData->getMatch();

				auto& gameStatus = replyData[GAME_STATUS];
				gameStatus[SETUP] = match.inSetup();

				auto& pieces = match.getActivePieces();
				if (!pieces.empty())
					gameStatus[PIECE_POSITIONS] = json::pieceMap(pieces);

				m_server.send(clientConnHdl, json::serverReply(m_curMsgID, replyData));
			}

			Json::Value notification;
			notification[MSG_TYPE] = MsgType::NOTIFICATION;

			auto& notificationData = notification[NOTIFICATION_DATA];
			notificationData[TYPE] = NotificationType::USER_JOINED;

			// TODO: Update when there are registered user names
			notificationData[REGISTERED] = false;
			notificationData[USERNAME]   = PlayersColorToPrettyStr(color);
			//notificationData[ROLE] =

			auto&& msg = Json::FastWriter().write(notification);

			for (auto& clientIt : matchClients)
				m_server.send(clientIt->getConnHdl(), msg);

			auto it = m_data.gameLists[RANDOM_GAMES].find(matchID);
			if (it != m_data.gameLists[RANDOM_GAMES].end())
			{
				{
					lock_guard<mutex> lock(m_data.gameListsMtx[RANDOM_GAMES]);
					m_data.gameLists[RANDOM_GAMES].erase(it);
				}

				m_server.listUpdated(RANDOM_GAMES);
			}

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
	vector<Json::Value> listUpdates;

	// ignore param["ruleSet"] for now
	for (const auto& listVal : param[LISTS])
	{
		const auto& listName = listVal.asString();
		optional<GamesListID> optList;

		if (listName == GamesList::OPEN_RANDOM_GAMES)
			optList = RANDOM_GAMES;
		else if (listName == GamesList::RUNNING_PUBLIC_GAMES)
			optList = PUBLIC_GAMES;
		else
		{
			// Might have subscribed to correct ones, but that's okay.
			// If this error ever appeared it would pretty surely be fixed quickly.

			// TODO: error message doesn't include the failed list name
			m_server.send(clientConnHdl, json::requestErr(m_curMsgID, ServerReplyErrMsg::LIST_DOES_NOT_EXIST));
			return;
		}

		if (optList)
		{
			auto list = *optList;

			if (!m_data.gameLists[list].empty())
				listUpdates.push_back(json::listUpdate(listName, m_data.gameLists[list]));

			lock_guard<mutex> lock(m_data.listSubscribersMtx[list]);
			m_data.listSubscribers[list].insert(clientConnHdl);
		}
	}

	m_server.send(clientConnHdl, json::requestSuccess(m_curMsgID));

	for (auto&& jsonVal : listUpdates)
		m_server.send(clientConnHdl, jsonVal);
}

void Worker::processUnsubscrGameListRequest(connection_hdl clientConnHdl, const Json::Value& param)
{
	// ignore param["ruleSet"] for now
	for (const auto& listVal : param[LISTS])
	{
		const auto& listName = listVal.asString();
		optional<GamesListID> optList;

		if (listName == GamesList::OPEN_RANDOM_GAMES)
			optList = RANDOM_GAMES;
		else if (listName == GamesList::RUNNING_PUBLIC_GAMES)
			optList = PUBLIC_GAMES;
		else
		{
			// Might have unsubscribed from correct ones, but that's okay.
			// If this error ever appeared it would pretty surely be fixed quickly.

			// TODO: error message doesn't include the failed list name
			m_server.send(clientConnHdl, json::requestErr(m_curMsgID, ServerReplyErrMsg::LIST_DOES_NOT_EXIST));
			return;
		}

		if (optList)
			m_server.unsubscribe(clientConnHdl, *optList);
	}

	m_server.send(clientConnHdl, json::requestSuccess(m_curMsgID));
}

void Worker::processChatMsg(connection_hdl clientConnHdl, const Json::Value& msg)
{
	auto clientData = getClientData(clientConnHdl);
	if (!clientData)
		return; // TODO: log an error

	Json::Value newMsg = msg;
	newMsg[MSG_DATA][USER] = clientData->username;

	distributeMessage(clientConnHdl, newMsg);
}

void Worker::processGameMsg(connection_hdl clientConnHdl, const Json::Value& msg)
{
	auto clientData = getClientData(clientConnHdl);
	if (!clientData)
		return; // TODO: log an error

	const auto& msgData = msg[MSG_DATA];
	const auto& action = msgData[ACTION].asString();
	const auto& param = msgData[PARAM];

	if (action == GameMsgAction::MOVE)
		processMoveMsg(*clientData, param);
	else if (action == GameMsgAction::MOVE_CAPTURE)
		processMoveCaptureMsg(*clientData, param);
	else if (action == GameMsgAction::PROMOTE)
		processPromoteMsg(*clientData, param);
	else if (action == GameMsgAction::SET_OPENING_ARRAY)
		processSetOpeningArrayMsg(*clientData, param);

	distributeMessage(clientConnHdl, msg);
}

void Worker::processSetOpeningArrayMsg(ClientData& clientData, const Json::Value& param)
{
	const auto& pieces = json::pieceMap(param);
	evalOpeningArray(pieces);

	auto& player = clientData.getPlayer();
	auto& match  = clientData.getMatchData().getMatch();

	for (const auto& pmIt : pieces)
	{
		for (const auto& coord : pmIt.second)
		{
			if (pmIt.first == PieceType::KING)
				player.getFortress().setCoord(coord);

			match.getActivePieces().emplace(coord, make_shared<Piece>(
				player.getColor(), pmIt.first, coord, match
			));
		}
	}

	player.setupDone();

	auto opColor = !player.getColor();
	if (match.hasPlayer(opColor) && match.getPlayer(opColor).isSetupDone())
		match.setupDone();
}

void Worker::processMoveMsg(ClientData& clientData, const Json::Value& param)
{
	// TODO
}

void Worker::processMoveCaptureMsg(ClientData& clientData, const Json::Value& param)
{
	// TODO
}

void Worker::processPromoteMsg(ClientData& clientData, const Json::Value& param)
{
	// TODO
}

void Worker::distributeMessage(connection_hdl clientConnHdl, const Json::Value& msg)
{
	auto clientData = getClientData(clientConnHdl);

	if (clientData)
	{
		const auto& clientDataSets = clientData->getMatchData().getClientDataSets();

		if (clientDataSets.size() > 1)
		{
			string json = Json::FastWriter().write(msg);

			for (auto it : clientDataSets)
				if (*it != *clientData)
					m_server.send(it->getConnHdl(), json);
		}
	}
	//else?
}
