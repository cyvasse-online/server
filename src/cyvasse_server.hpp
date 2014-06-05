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

#define _WEBSOCKETPP_CPP11_STL_

#include <atomic>
#include <iostream>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <set>
#include <thread>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

class CyvasseServer
{
	private:
		typedef websocketpp::server<websocketpp::config::asio> server;
		typedef std::map<websocketpp::connection_hdl, unsigned long, std::owner_less<websocketpp::connection_hdl>> ConList;
		typedef std::pair<websocketpp::connection_hdl, server::message_ptr> Job;
		typedef std::queue<std::unique_ptr<Job>> JobQueue;

		server _server;
		ConList _connections;
		JobQueue _jobQueue;
		std::set<std::unique_ptr<std::thread>> _workerThreads;

		std::mutex _jobLock;
		std::mutex _connectionLock;
		std::condition_variable _jobCond;

		std::atomic<bool> _running;

		std::ranlux24 _int24Generator;
		std::ranlux48 _int48Generator;

	public:
		CyvasseServer();
		~CyvasseServer();

		void run(uint16_t port, unsigned nWorkerThreads);
		void onMessage(websocketpp::connection_hdl hdl, server::message_ptr msg);
		void processMessages();
};

#endif // _CYVASSE_SERVER_HPP_
