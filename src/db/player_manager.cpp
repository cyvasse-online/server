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

#include "player_manager.hpp"

#include <tntdb/connect.h>
#include <tntdb/error.h>
#include <tntdb/statement.h>
#include "db_config.hpp"

using namespace cyvmath;

namespace cyvdb
{
	PlayerManager::PlayerManager()
		try : _conn(tntdb::connectCached(DBConfig::glob().getMatchDataUrl()))
	{ }
	catch(tntdb::Error& e)
	{
		throw std::runtime_error(std::string("Couldn't connect to database: ") + e.what());
	}

	Player PlayerManager::getPlayer(const std::string& playerID)
	{
		try
		{
			tntdb::Row row =
				_conn.prepare(
					"SELECT match, color FROM players "
					"WHERE player_id = :id"
				)
				.set("id", playerID)
				.selectRow();

			return Player(playerID, row.getString(0), StrToPlayersColor(row.getString(1)));
		}
		catch(tntdb::NotFound&)
		{
			return Player();
		}
		catch(tntdb::Error& e)
		{
			throw std::runtime_error(std::string("Error in database query: ") + e.what());
		}
	}
}
