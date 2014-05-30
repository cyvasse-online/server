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
	using std::placeholders::_1;
	using std::placeholders::_2;

	// Initialize Asio Transport
	_server.init_asio();

	// Register handler callback
	_server.set_message_handler(std::bind(&CyvasseServer::onMessage, this, _1, _2));
}

CyvasseServer::~CyvasseServer()
{
	for(const std::unique_ptr<std::thread>& it : _workerThreads)
		it->join();
}

void CyvasseServer::run(uint16_t port, unsigned nWorkerThreads)
{
	// start worker threads
	assert(nWorkerThreads != 0);
	for(int i = 0; i < nWorkerThreads; i++)
		_workerThreads.emplace(new std::thread(std::bind(&CyvasseServer::processMessages, this)));

	// Listen on the specified port
	_server.listen(port);

	// Start the server accept loop
	_server.start_accept();

	// Start the ASIO io_service run loop
	_server.run();
}

void CyvasseServer::onMessage(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
	// Queue message up for sending by processing thread
	std::unique_lock<std::mutex> lock(_jobLock);

	_jobQueue.emplace(new Job(hdl, msg));

	lock.unlock();
	_jobCond.notify_one();
}

void CyvasseServer::processMessages()
{
	while(true)
	{
		std::unique_lock<std::mutex> lock(_jobLock);

		while(_jobQueue.empty())
			_jobCond.wait(lock);

		std::unique_ptr<Job> job = std::move(_jobQueue.front());
		_jobQueue.pop();

		lock.unlock();

		/* Process job */
	}
}
