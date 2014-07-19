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

#ifndef _CYVASSE_SERVER_HPP_
#define _CYVASSE_SERVER_HPP_

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <utility>

#define _WEBSOCKETPP_CPP11_STL_
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#undef _WEBSOCKETPP_CPP11_STL_

class JobHandler;
class ClientData;
class MatchData;

class CyvasseServer
{
	friend JobHandler;

	private:
		typedef websocketpp::server<websocketpp::config::asio> WSServer;

		typedef std::map<websocketpp::connection_hdl, std::shared_ptr<ClientData>,
		                 std::owner_less<websocketpp::connection_hdl>> ClientDataMap;
		typedef std::map<std::string, std::shared_ptr<MatchData>> MatchMap;

		typedef std::pair<websocketpp::connection_hdl, WSServer::message_ptr> Job;
		typedef std::queue<std::unique_ptr<Job>> JobQueue;

		WSServer _wsServer;

		ClientDataMap _clientDataSets;
		MatchMap _matches;

		std::mutex _connMapMtx;

		JobQueue _jobQueue;
		std::set<std::unique_ptr<JobHandler>> _workers;
		std::mutex _jobMtx;
		std::condition_variable _jobCond;

		std::atomic<bool> _running;

	public:
		CyvasseServer();
		~CyvasseServer();

		void run(uint16_t port, unsigned nWorkers);
		void stop();

		void onMessage(websocketpp::connection_hdl hdl, WSServer::message_ptr msg);
};

#endif // _CYVASSE_SERVER_HPP_
