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

#include "cyvasse_server.hpp"

#include <thread>
#include <cassert>
#include <json/value.h>
#include <json/writer.h>
//#include <cyvdb/match_manager.hpp>
#include <cyvws/json_notification.hpp>
#include <cyvws/json_server_reply.hpp>
#include "client_data.hpp"
#include "match_data.hpp"
#include "worker.hpp"

using namespace std;
using namespace websocketpp;

using namespace cyvws;

CyvasseServer::CyvasseServer()
{
	using placeholders::_1;
	using placeholders::_2;

	// Initialize Asio Transport
	m_wsServer.init_asio();
	m_wsServer.set_reuse_addr(true);

	// Register handler callback
	m_wsServer.set_message_handler(bind(&CyvasseServer::onMessage, this, _1, _2));
	m_wsServer.set_close_handler(bind(&CyvasseServer::onClose, this, _1));
	//m_wsServer.set_http_handler(bind(&CyvasseServer::onHttpRequest, this, _1));
}

CyvasseServer::~CyvasseServer()
{
	if (m_data.running)
		stop();
}

void CyvasseServer::run(uint16_t port, unsigned nWorkers)
{
	// Start worker threads
	assert(nWorkers != 0);
	for (unsigned i = 0; i < nWorkers; i++)
		m_workers.emplace(new Worker(*this, m_data));

	// Listen on the specified port
	m_wsServer.listen(port);

	// Start the server accept loop
	m_wsServer.start_accept();

	// Start the ASIO io_service run loop
	m_wsServer.run();
}

void CyvasseServer::stop()
{
	m_data.running = false;
	m_data.jobCond.notify_all();
}

void CyvasseServer::listUpdated(GamesListID list)
{
	if (!m_data.listSubscribers[list].empty())
	{
		string listName;

		if (list == RANDOM_GAMES)
			listName = GamesList::OPEN_RANDOM_GAMES;
		else if (list == PUBLIC_GAMES)
			listName = GamesList::RUNNING_PUBLIC_GAMES;

		assert(!listName.empty());

		auto jsonStr = Json::FastWriter().write(json::listUpdate(listName, m_data.gameLists[list]));
		for (auto&& hdl : m_data.listSubscribers[list])
			send(hdl, jsonStr);
	}
}

void CyvasseServer::unsubscribe(connection_hdl hdl, GamesListID list)
{
	auto it = m_data.listSubscribers[list].find(hdl);
	if (it != m_data.listSubscribers[list].end())
	{
		lock_guard<mutex> lock(m_data.listSubscribersMtx[list]);
		m_data.listSubscribers[list].erase(it);
	}
}

void CyvasseServer::unsubscribeAll(connection_hdl hdl)
{
	unsubscribe(hdl, RANDOM_GAMES);
	unsubscribe(hdl, PUBLIC_GAMES);
}

void CyvasseServer::onMessage(connection_hdl hdl, WSServer::message_ptr msg)
{
	// Queue message up for sending by processing thread
	lock_guard<mutex> lock(m_data.jobMtx);

	m_data.jobQueue.emplace(hdl, msg);
	m_data.jobCond.notify_one();
}

void CyvasseServer::onClose(connection_hdl hdl)
{
	unsubscribeAll(hdl);

	shared_ptr<ClientData> clientData = m_data.getClientData(hdl);

	if (!clientData)
		return;

	{
		lock_guard<mutex> lock(m_data.clientDataMtx);
		m_data.clientData.erase(m_data.clientData.find(hdl));
	}

	auto& dataSets = clientData->getMatchData().getClientDataSets();

	{
		auto it = dataSets.find(clientData);
		if (it != dataSets.end())
			dataSets.erase(it);
	}

	for (auto&& it : dataSets)
		send(it->getConnHdl(), json::userLeft(clientData->username));

	// if this was the last / only player connected
	// to this match, remove the match completely
	if (dataSets.empty())
	{
		auto matchID = clientData->getMatchData().getMatch().getID();

		{
			lock_guard<mutex> lock(m_data.matchDataMtx);

			auto it = m_data.matchData.find(matchID);
			if (it != m_data.matchData.end())
				m_data.matchData.erase(it);
		}

		for (GamesListID list : { RANDOM_GAMES, PUBLIC_GAMES })
		{
			auto it = m_data.gameLists[list].find(matchID);
			if (it != m_data.gameLists[list].end())
			{
				{
					lock_guard<mutex> lock(m_data.gameListsMtx[list]);
					m_data.gameLists[list].erase(it);
				}

				listUpdated(list);
			}
		}

		/*thread([=] {
			this_thread::sleep_for(milliseconds(50));
			cyvdb::MatchManager().removeMatch(matchID);
		}).detach();*/
	}
}

void CyvasseServer::onHttpRequest(connection_hdl)
{
	// TODO: send 301 moved permanently -> domain:80
}

void CyvasseServer::send(connection_hdl hdl, const string& data)
{
	m_wsServer.send(hdl, data, frame::opcode::text);
}

void CyvasseServer::send(connection_hdl hdl, const Json::Value& data)
{
	send(hdl, Json::FastWriter().write(data));
}
