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
#include <csignal>
#include "cyvasse_server.hpp"

CyvasseServer* s = nullptr;

extern "C"
{
	void stopServer(int /* signal */)
	{
		if(s) s->stop();
		exit(0);
	}
}

int main()
{
#ifdef HAVE_SIGACTION
	struct sigaction newAction, oldAction;

	// setup sigaction struct for stopServer
	newAction.sa_handler = stopServer;
	sigemptyset(&newAction.sa_mask);
    newAction.sa_flags = 0;

	sigaction(SIGHUP, nullptr, &oldAction);
	if(oldAction.sa_handler != SIG_IGN)
		sigaction(SIGHUP, &newAction, nullptr);

	sigaction(SIGINT, nullptr, &oldAction);
	if(oldAction.sa_handler != SIG_IGN)
		sigaction(SIGINT, &newAction, nullptr);

	sigaction(SIGTERM, nullptr, &oldAction);
	if(oldAction.sa_handler != SIG_IGN)
		sigaction(SIGTERM, &newAction, nullptr);
#else
	if(signal(SIGHUP, stopServer) == SIG_IGN)
		signal(SIGHUP, SIG_IGN);

	if(signal(SIGINT, stopServer) == SIG_IGN)
		signal(SIGINT, SIG_IGN);

	if(signal(SIGTERM, stopServer) == SIG_IGN)
		signal(SIGTERM, SIG_IGN);
#endif

	try
	{
		s = new CyvasseServer;
		s->run(2516, 1);
	}
	catch(std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	if(s) delete s;

	return 0;
}
