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

#include <exception>
#include <iostream>
#include <memory>
#include <csignal>
#include <yaml-cpp/yaml.h>
#include <cyvdb/config.hpp>
#include <make_unique.hpp> // TODO
#include "cyvasse_server.hpp"

using namespace std;

unique_ptr<CyvasseServer> server;

extern "C" void stopServer(int /* signal */)
{
	if(server)
	{
		server->stop();
		server.reset();
	}

	exit(0);
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

	auto config = YAML::LoadFile("config.yml");
	auto listenPort   = config["listenPort"].as<int>();
	auto matchDataUrl = config["matchDataUrl"].as<string>();

	if(matchDataUrl.empty())
	{
		cerr << "Error: No database url set!" << endl;
		exit(1);
	}

	cyvdb::DBConfig::glob().setMatchDataUrl(matchDataUrl);

	try
	{
		server = make_unique<CyvasseServer>();
		server->run(listenPort, 1);
	}
	catch(std::exception& e)
	{
		cerr << e.what() << endl;
	}

	return 0;
}
