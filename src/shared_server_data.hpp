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

#ifndef _SHARED_SERVER_DATA_HPP_
#define _SHARED_SERVER_DATA_HPP_

#include <array>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <cyvws/notification.hpp>

#define _WEBSOCKETPP_CPP11_STL_
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#undef _WEBSOCKETPP_CPP11_STL_

typedef websocketpp::server<websocketpp::config::asio> WSServer;

class ClientData;
class MatchData;

using websocketpp::connection_hdl;

struct Job
{
	connection_hdl conn_hdl;
	WSServer::message_ptr msg_ptr;

	Job(connection_hdl connHdl, WSServer::message_ptr msgPtr)
		: conn_hdl(connHdl)
		, msg_ptr(msgPtr)
	{ }
};

struct SharedServerData
{
	using ClientMap     = std::map<connection_hdl, std::shared_ptr<ClientData>, std::owner_less<connection_hdl>>;
	using MatchMap      = std::map<std::string, std::shared_ptr<MatchData>>;
	using ConnectionSet = std::set<connection_hdl, std::owner_less<connection_hdl>>;

	std::atomic_bool running = {true};
	std::atomic_bool maintenance = {false};

	std::queue<Job> jobQueue;

	std::mutex jobMtx;
	std::condition_variable jobCond;

	ClientMap clientData;
	MatchMap matchData;

	std::mutex clientDataMtx;
	std::mutex matchDataMtx;

	std::array<cyvws::GamesListMap, 2> gameLists;
	std::array<std::mutex, 2>          gameListsMtx;

	std::array<ConnectionSet, 2> listSubscribers;
	std::array<std::mutex, 2>    listSubscribersMtx;

	auto getClientData(connection_hdl hdl) -> std::shared_ptr<ClientData>;
};

enum GamesListID
{
	RANDOM_GAMES = 0,
	PUBLIC_GAMES = 1
};

#endif // _SHARED_SERVER_DATA_HPP_
