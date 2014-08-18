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

#include "client_data.hpp"
#include "job_handler.hpp"
#include "match_data.hpp"

CyvasseServer::CyvasseServer()
	: m_running(true)
{
	using std::placeholders::_1;
	using std::placeholders::_2;

	// Initialize Asio Transport
	m_wsServer.init_asio();

	// Register handler callback
	m_wsServer.set_message_handler(std::bind(&CyvasseServer::onMessage, this, _1, _2));
	m_wsServer.set_close_handler(std::bind(&CyvasseServer::onClose, this, _1));
	//m_wsServer.set_http_handler(std::bind(&CyvasseServer::onHttpRequest, this, _1));
}

CyvasseServer::~CyvasseServer()
{
	if(m_running) stop();
}

void CyvasseServer::run(uint16_t port, unsigned nWorkers)
{
	// start worker threads
	assert(nWorkers != 0);
	for(unsigned i = 0; i < nWorkers; i++)
		m_workers.emplace(new JobHandler(*this));

	// Listen on the specified port
	m_wsServer.listen(port);

	// Start the server accept loop
	m_wsServer.start_accept();

	// Start the ASIO io_service run loop
	m_wsServer.run();
}

void CyvasseServer::stop()
{
	m_running = false;
	m_jobCond.notify_all();
}

void CyvasseServer::onMessage(websocketpp::connection_hdl hdl, WSServer::message_ptr msg)
{
	// Queue message up for sending by processing thread
	std::unique_lock<std::mutex> lock(m_jobMtx);

	m_jobQueue.emplace(new Job(hdl, msg));

	lock.unlock();
	m_jobCond.notify_one();
}

void CyvasseServer::onClose(websocketpp::connection_hdl hdl)
{
	std::unique_lock<std::mutex> matchDataLock(m_matchDataMtx, std::defer_lock);
	std::unique_lock<std::mutex> clientDataLock(m_clientDataMtx);

	auto it1 = m_clientDataSets.find(hdl);
	if(it1 == m_clientDataSets.end())
		return;

	std::shared_ptr<ClientData> clientData = it1->second;
	m_clientDataSets.erase(it1);
	clientDataLock.unlock();

	auto& dataSets = clientData->getMatchData().getClientDataSets();

	auto it2 = dataSets.find(clientData);
	if(it2 != dataSets.end())
		dataSets.erase(it2);

	if(dataSets.empty())
	{
		// if this was the last / only player connected
		// to this match, remove the match completely

		matchDataLock.lock();
		auto it3 = m_matches.find(clientData->getMatchData().getID());
		if(it3 == m_matches.end())
			return; // or assert this won't happen?

		matchDataLock.unlock();
	}
}

void CyvasseServer::onHttpRequest(websocketpp::connection_hdl)
{
	// TODO: send 301 moved permanently -> domain:80
}
