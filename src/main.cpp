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
#include <fstream>
#include <iostream>
#include <memory>
#include <csignal>
#include <cstdio>

#include <unistd.h>
#include <yaml-cpp/yaml.h>
//#include <cyvdb/config.hpp>
#include "cyvasse_server.hpp"

using namespace std;

static constexpr const char* pidFileName = "cyvasse-server.pid";

void createPidFile();
void removePidFile();

unique_ptr<CyvasseServer> server;

void setupSignals();

int main()
{
	setupSignals();

	auto config = YAML::LoadFile("config.yml");
	auto listenPort   = config["listenPort"].as<int>();
	/*auto matchDataUrl = config["matchDataUrl"].as<string>();

	if(matchDataUrl.empty())
	{
		cerr << "Error: No database url set!" << endl;
		exit(1);
	}

	cyvdb::DBConfig::glob().setMatchDataUrl(matchDataUrl);*/

	int retVal = 0;

	try
	{
		createPidFile();

		server = make_unique<CyvasseServer>();
		server->run(listenPort, 1);
	}
	catch (std::exception& e)
	{
		cerr << "server exception: " << e.what() << endl;
		retVal = 1;
	}

	removePidFile();
	return retVal;
}

void createPidFile()
{
	ofstream pidFile(pidFileName);
	if (pidFile)
	{
		pidFile << getpid() << endl;
		pidFile.close();
	}
}

void removePidFile()
{
	remove(pidFileName);
}

extern "C"
{
	void stopServer(int /* signal */)
	{
		if (server)
		{
			server->stop();
			server.reset();

			removePidFile();
		}

		exit(0);
	}

	void maintainanceMode(int /* signal */)
	{
		if (!server)
			return;

		server->maintenanceMode();
	}
}

void setupSignals()
{
#ifdef HAVE_SIGACTION
	struct sigaction stopAction, oldAction;

	// setup sigaction struct for stopServer
	stopAction.sa_handler = stopServer;
	sigemptyset(&stopAction.sa_mask);
	stopAction.sa_flags = 0;

	sigaction(SIGHUP, nullptr, &oldAction);
	if (oldAction.sa_handler != SIG_IGN)
		sigaction(SIGHUP, &stopAction, nullptr);

	sigaction(SIGINT, nullptr, &oldAction);
	if (oldAction.sa_handler != SIG_IGN)
		sigaction(SIGINT, &stopAction, nullptr);

	sigaction(SIGTERM, nullptr, &oldAction);
	if (oldAction.sa_handler != SIG_IGN)
		sigaction(SIGTERM, &stopAction, nullptr);
#else
	if (signal(SIGHUP, stopServer) == SIG_IGN)
		signal(SIGHUP, SIG_IGN);

	if (signal(SIGINT, stopServer) == SIG_IGN)
		signal(SIGINT, SIG_IGN);

	if (signal(SIGTERM, stopServer) == SIG_IGN)
		signal(SIGTERM, SIG_IGN);
#endif

	signal(SIGUSR1, maintainanceMode);
}
