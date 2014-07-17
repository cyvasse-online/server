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

#ifndef _PLAYER_MANAGER_HPP_
#define _PLAYER_MANAGER_HPP_

#include <random>
#include <tntdb/connection.h>

namespace cyvdb
{
	class Player;

	class PlayerManager
	{
		private:
			tntdb::Connection _conn;
			std::ranlux48 _int48Generator;

		public:
			explicit PlayerManager(tntdb::Connection& conn);

			PlayerManager();

			// queries
			Player getPlayer(const std::string& playerID);

			// modifications
			void addPlayer(Player&);

			// other stuff
			std::string newPlayerID();
	};
}

#endif // _PLAYER_MANAGER_HPP_
