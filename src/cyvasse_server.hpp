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

#define _WEBSOCKETPP_CPP11_STL_

#include <iostream>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

typedef websocketpp::server<websocketpp::config::asio> server;

/* on_open insert connection_hdl into channel
 * on_close remove connection_hdl from channel
 * on_message queue send to all channels
 */

enum action_type
{
	SUBSCRIBE,
	UNSUBSCRIBE,
	MESSAGE
};

struct action
{
	action(action_type t, websocketpp::connection_hdl h)
		: type(t)
		, hdl(h)
	{
	}

	action(action_type t, websocketpp::connection_hdl h, server::message_ptr m)
		: type(t), hdl(h), msg(m)
	{
	}

	action_type type;
	websocketpp::connection_hdl hdl;
	server::message_ptr msg;
};

class CyvasseServer
{
	private:
		typedef std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> con_list;

		server _server;
		con_list _connections;
		std::queue<action> _actions;

		std::mutex _action_lock;
		std::mutex _connection_lock;
		std::condition_variable _action_cond;

	public:
		CyvasseServer();

		void run(uint16_t port);

		void on_open(websocketpp::connection_hdl hdl);
		void on_close(websocketpp::connection_hdl hdl);
		void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg);

		void process_messages();
};
