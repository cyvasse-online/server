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

#ifndef _WORKER_HPP_
#define _WORKER_HPP_

#include <string>
#include <thread>
#include "shared_server_data.hpp"

class Worker
{
	public:
		typedef std::function<void(
				websocketpp::connection_hdl,
				const std::string&,
				websocketpp::frame::opcode::value
			)> send_func_type;

	private:
		SharedServerData& m_data;
		send_func_type send;

		std::thread m_thread;

	public:
		Worker(SharedServerData& data, send_func_type send_func);
		~Worker();

		// JobHandler main loop
		void processMessages();

		std::string newMatchID();
		std::string newPlayerID();
};

#endif // _WORKER_HPP_
