/* Copyright 2014 - 2015 Jonas Platte
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

namespace Json { class Value; }
class CyvasseServer;
class ClientData;

using websocketpp::connection_hdl;

class Worker
{
	private:
		CyvasseServer& m_server;
		SharedServerData& m_data;

		std::thread m_thread;

		unsigned m_curMsgID;

		std::string newMatchID();
		std::string newPlayerID();

	public:
		Worker(CyvasseServer&, SharedServerData& data);
		~Worker();

		// JobHandler main loop
		void processMessages();

		void processServerRequest(connection_hdl, const Json::Value& msg);
		void processInitCommRequest(connection_hdl, const Json::Value& param);
		void processCreateGameRequest(connection_hdl, const Json::Value& param);
		void processJoinGameRequest(connection_hdl, const Json::Value& param);
		void processSetUsernameRequest(connection_hdl, const Json::Value& param);
		void processSubscrGameListRequest(connection_hdl, const Json::Value& param);
		void processUnsubscrGameListRequest(connection_hdl, const Json::Value& param);

		void processChatMsg(connection_hdl, const Json::Value& msg);

		void processGameMsg(connection_hdl, const Json::Value& msg);
		void processSetOpeningArrayMsg(ClientData&, const Json::Value& param);
		void processMoveMsg(ClientData&, const Json::Value& param);
		void processMoveCaptureMsg(ClientData&, const Json::Value& param);
		void processPromoteMsg(ClientData&, const Json::Value& param);

		void distributeMessage(connection_hdl, const Json::Value& msg);
};

#endif // _WORKER_HPP_
