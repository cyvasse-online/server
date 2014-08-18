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

#ifndef _JOB_HANDLER_HPP_
#define _JOB_HANDLER_HPP_

#include <string>
#include <thread>

class CyvasseServer;

struct JobHandler
{
	private:
		CyvasseServer& m_server;

		std::thread m_thread;

	public:
		JobHandler(CyvasseServer&);
		~JobHandler();

		// JobHandler main loop
		void processMessages();

		std::string newMatchID();
		std::string newPlayerID();
};

#endif // _JOB_HANDLER_HPP_
