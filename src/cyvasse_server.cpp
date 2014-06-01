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

// default value
#define BUFFERSIZE 16777216
#include <b64/encode.h>
#include <b64/decode.h>

// these two function should be removed
// if libb64 gets a better API somewhen
// they work but have a very weird 'overflow'
// limit after which the conversions
// fail... This limit is:
// decimal: 17365880163140632575
// hexadecimal: F0FFFFFFFFFFFFFF
// Also, they don't use the encodeend stuff
// because I don't understand it and it
// produced weird output.
std::string intToB64ID(uint64_t intVal)
{
	base64::encoder enc;
	base64_init_encodestate(&enc._state);

	char* cstr = new char[11];
	enc.encode(reinterpret_cast<char*>(&intVal), 8, cstr);
	cstr[10] = '\0';

	std::string retStr(cstr);
	// it's no problem to use a '/' in the URL, but '_' looks better
	for(size_t pos = retStr.find('/'); pos != std::string::npos; pos = retStr.find('/', pos + 1))
		retStr.at(pos) = '_';

	return retStr;
}

uint64_t b64IDToInt(std::string b64ID)
{
	for(size_t pos = b64ID.find('_'); pos != std::string::npos; pos = b64ID.find('_', pos + 1))
		b64ID.at(pos) = '/';

	base64::decoder dec;
	base64_init_decodestate(&dec._state);

	char* ret = new char[8];
	dec.decode(b64ID.c_str(), 10, ret);

	return *(reinterpret_cast<uint64_t*>(ret));
}

uint64_t CyvasseServer::_nextID = 1;

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
		JobData data; // TODO: Deserialize job->second into this

		switch(data.requestedAction)
		{
			case UNDEFINED:
				// TODO: Write error message to client
				break;
			case CREATE_GAME:
				std::string b64ID = intToB64ID(_nextID++);
				// TODO: Compose answer message with
				// this ID and send it to the client
				break;
			case JOIN_GAME:
				// TODO
				break;
			default:
				assert(0);
				break;
		}
	}
}
