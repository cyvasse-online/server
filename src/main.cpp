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

#include <iostream>
#include <thread>
#include "cyvasse_server.hpp"

int main()
{
	try
	{
		CyvasseServer server_instance;

		// Start a thread to run the processing loop
		std::thread t(std::bind(&CyvasseServer::process_messages, &server_instance));

		// Run the asio loop with the main thread
		server_instance.run(9002);

		t.join();
	}
	catch(std::exception & e)
	{
		std::cerr << e.what() << std::endl;
	}
}
