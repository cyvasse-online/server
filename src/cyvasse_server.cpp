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

CyvasseServer::CyvasseServer()
{
	// Initialize Asio Transport
	_server.init_asio();

	// Register handler callbacks
	_server.set_open_handler(bind(&CyvasseServer::on_open, this, ::_1));
	_server.set_close_handler(bind(&CyvasseServer::on_close, this, ::_1));
	_server.set_message_handler(bind(&CyvasseServer::on_message, this, ::_1, ::_2));
}

void CyvasseServer::run(uint16_t port)
{
	// listen on specified port
	_server.listen(port);

	// Start the server accept loop
	_server.start_accept();

	// Start the ASIO io_service run loop
	try
	{
		_server.run();
	}
	catch(const std::exception & e)
	{
		std::cerr << e.what() << std::endl;
	}
	catch(websocketpp::lib::error_code e)
	{
		std::cerr << e.message() << std::endl;
	}
	catch(...)
	{
		std::cerr << "other exception" << std::endl;
	}
}

void CyvasseServer::on_open(websocketpp::connection_hdl hdl)
{
	std::unique_lock<std::mutex> lock(_action_lock);
	//std::cout << "on_open" << std::endl;
	_actions.push(action(SUBSCRIBE,hdl));
	lock.unlock();
	_action_cond.notify_one();
}

void CyvasseServer::on_close(websocketpp::connection_hdl hdl)
{
	std::unique_lock<std::mutex> lock(_action_lock);
	//std::cout << "on_close" << std::endl;
	_actions.push(action(UNSUBSCRIBE,hdl));
	lock.unlock();
	_action_cond.notify_one();
}

void CyvasseServer::on_message(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
	// queue message up for sending by processing thread
	std::unique_lock<std::mutex> lock(_action_lock);
	//std::cout << "on_message" << std::endl;
	_actions.push(action(MESSAGE,hdl,msg));
	lock.unlock();
	_action_cond.notify_one();
}

void CyvasseServer::process_messages()
{
	while(true)
	{
		std::unique_lock<std::mutex> lock(_action_lock);

		while(_actions.empty())
		{
			_action_cond.wait(lock);
		}

		action a = _actions.front();
		_actions.pop();

		lock.unlock();

		if (a.type == SUBSCRIBE)
		{
			std::unique_lock<std::mutex> con_lock(_connection_lock);
			_connections.insert(a.hdl);
		}
		else if (a.type == UNSUBSCRIBE)
		{
			std::unique_lock<std::mutex> con_lock(_connection_lock);
			_connections.erase(a.hdl);
		}
		else if (a.type == MESSAGE)
		{
			std::unique_lock<std::mutex> con_lock(_connection_lock);

			for (con_list::iterator it = _connections.begin(); it != _connections.end(); ++it)
				_server.send(*it,a.msg);
		}
		else
		{
			// undefined.
		}
	}
}
