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

#include <chrono>

// default value
#define BUFFERSIZE 16777216
#include <b64/encode.h>
#include <b64/decode.h>
#undef BUFFERSIZE

#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include "job_data.hpp"

// this function starts with _ because it's not universally usable
// (it ignores the last 1/4 of bytes of the parameter value)
template <typename IntType>
std::string _intToB64ID(IntType intVal)
{
	const int size = sizeof(IntType) / 4 * 3; // ignore the first 1/4 bytes
	const int sizeEnc = sizeof(IntType); // base64 adds 1/3 of size again

	base64::encoder enc;
	base64_init_encodestate(&enc._state);

	char* cstr = new char[sizeEnc + 1];
	enc.encode(reinterpret_cast<char*>(&intVal), size, cstr);
	cstr[sizeEnc] = '\0';

	std::string retStr(cstr);
	// it's no problem to use a '/' in the URL, but '_' looks better
	for(size_t pos = retStr.find('/'); pos != std::string::npos; pos = retStr.find('/', pos + 1))
		retStr.at(pos) = '_';

	return retStr;
}

#define int24ToB64ID(x) \
	_intToB64ID<uint32_t>(x)
#define int48ToB64ID(x) \
	_intToB64ID<uint64_t>(x)

using std::chrono::system_clock;

CyvasseServer::CyvasseServer()
	: _int24Generator(system_clock::now().time_since_epoch().count())
	, _int48Generator(system_clock::now().time_since_epoch().count())
{
	using std::placeholders::_1;
	using std::placeholders::_2;

	_running = true;

	// Initialize Asio Transport
	_server.init_asio();

	// Register handler callback
	_server.set_message_handler(std::bind(&CyvasseServer::onMessage, this, _1, _2));
}

CyvasseServer::~CyvasseServer()
{
	_running = false;
	_jobCond.notify_all();

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
	std::unique_lock<std::mutex> lock(_jobMtx);

	_jobQueue.emplace(new Job(hdl, msg));

	lock.unlock();
	_jobCond.notify_one();
}

void CyvasseServer::processMessages()
{
	while(true)
	{
		std::unique_lock<std::mutex> lock(_jobMtx);

		while(_running && _jobQueue.empty())
			_jobCond.wait(lock);

		if(!_running)
			break;

		std::unique_ptr<Job> job = std::move(_jobQueue.front());
		_jobQueue.pop();

		lock.unlock();

		/* Process job */
		JobData data(job->second->get_payload());

		Json::Value retVal;
		auto setError = [&](std::string error) {
				retVal["success"] = false;
				retVal["error"] = error;
			};

		if(!data.error.empty())
		{
			setError(data.error);
		}
		else
		{
			std::unique_lock<std::mutex> lock(_connMapMtx);
			switch(data.action)
			{
				case ACTION_CREATE_GAME:
				{
					auto it = _connectionMatches.find(job->first);
					if(it != _connectionMatches.end())
					{
						setError("this connection is already in use for a running match");
						break;
					}

					std::string matchID(int24ToB64ID(_int24Generator()));

					auto tmp1 = _connectionMatches.emplace(job->first, matchID);
					auto tmp2 = _matchConnections.emplace(matchID, std::vector<websocketpp::connection_hdl>());

					// both emplacements must be successful when the previous `break` wasn't run
					assert(tmp1.second);
					assert(tmp2.second);

					// Add the connection handle of the job to the new _matchConnections entries vector
					tmp2.first->second.push_back(job->first);

					std::string playerID(int48ToB64ID(_int48Generator()));

					retVal["success"]  = true;
					retVal["matchID"]  = matchID;
					retVal["playerID"] = playerID;
					break;
				}
				case ACTION_JOIN_GAME:
				{
					auto it1 = _connectionMatches.find(job->first);
					if(it1 != _connectionMatches.end())
					{
						setError("this connection is already in use for a running match");
						break;
					}

					auto it2 = _matchConnections.find(data.joinGame.matchID);
					if(it2 == _matchConnections.end())
					{
						setError("game not found");
						break;
					}

					auto tmp = _connectionMatches.emplace(job->first, data.joinGame.matchID);
					assert(tmp.second);

					it2->second.push_back(job->first);

					std::string playerID(int48ToB64ID(_int48Generator()));

					retVal["success"]  = true;
					retVal["playerID"] = playerID;
					break;
				}
				case ACTION_RESUME_GAME:
					break;
				/*case ACTION_START:
					break;
				case ACTION_MOVE_PIECE:
					break;
				case ACTION_RESIGN:
					break;
				default:
					assert(0);
					break;*/
				default:
				{
					auto it1 = _connectionMatches.find(job->first);
					if(it1 == _connectionMatches.end())
					{
						setError("could not find the match this message belongs to");
						break;
					}

					auto it2 = _matchConnections.find(it1->second);
					if(it2 == _matchConnections.end())
					{
						setError("game this message belongs to not found");
						break;
					}

					for(websocketpp::connection_hdl hdl : it2->second)
					{
						//if(hdl != job->first)
							_server.send(hdl, job->second);
					}

					retVal["success"] = true;
					break;
				}
			}
		}
		_server.send(job->first, Json::FastWriter().write(retVal), websocketpp::frame::opcode::text);
	}
}
