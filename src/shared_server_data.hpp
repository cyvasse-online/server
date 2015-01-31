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

#ifndef _SHARED_SERVER_DATA_HPP_
#define _SHARED_SERVER_DATA_HPP_

#include <atomic>
#include <condition_variable>
#include <mutex>

#define _WEBSOCKETPP_CPP11_STL_
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#undef _WEBSOCKETPP_CPP11_STL_

typedef websocketpp::server<websocketpp::config::asio> WSServer;

class ClientData;
class MatchData;

struct Job
{
	websocketpp::connection_hdl conn_hdl;
	WSServer::message_ptr msg_ptr;

	Job(websocketpp::connection_hdl connHdl, WSServer::message_ptr msgPtr)
		: conn_hdl(connHdl)
		, msg_ptr(msgPtr)
	{ }
};

struct SharedServerData
{
	typedef std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>>
		ConnectionSet;

	typedef std::map<websocketpp::connection_hdl, std::shared_ptr<ClientData>, std::owner_less<websocketpp::connection_hdl>>
		ClientMap;

	typedef std::map<std::string, std::shared_ptr<MatchData>>
		MatchMap;

	std::atomic_bool running = {true};

	std::queue<Job> jobQueue;

	std::mutex jobMtx;
	std::condition_variable jobCond;

	ClientMap clientData;
	MatchMap matchData;

	std::mutex clientDataMtx;
	std::mutex matchDataMtx;

	ConnectionSet randomGamesSubscribers;
	ConnectionSet publicGamesSubscribers;
};

#endif // _SHARED_SERVER_DATA_HPP_
