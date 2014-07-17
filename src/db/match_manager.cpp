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

#include "match_manager.hpp"

#include <chrono>
#include <stdexcept>
#include <tntdb/connect.h>
#include <tntdb/error.h>
#include <tntdb/statement.h>
#include "../b64.hpp"
#include "match.hpp"
#include "db_config.hpp"

using std::chrono::system_clock;
using namespace cyvmath;

namespace cyvdb
{
	MatchManager::MatchManager(tntdb::Connection& conn)
		: _conn(conn)
		, _int24Generator(system_clock::now().time_since_epoch().count())
	{ }

	MatchManager::MatchManager()
		: _conn(tntdb::connectCached(DBConfig::glob().getMatchDataUrl()))
		, _int24Generator(system_clock::now().time_since_epoch().count())
	{ }

	Match MatchManager::getMatch(const std::string& matchID)
	{
		try
		{
			tntdb::Row row =
				_conn.prepare(
					"SELECT rule_set, searching_for_player FROM matches "
					"WHERE match_id = :id"
				)
				.set("id", matchID)
				.selectRow();

			return Match(matchID, StrToRuleSet(row.getString(0)), row.getBool(1));
		}
		catch(tntdb::NotFound&)
		{
			return Match();
		}
	}

	void MatchManager::addMatch(Match& match)
	{
		if(!match.valid())
			throw std::invalid_argument("The given Match object is invalid");

		// TODO: throw another std::invalid_argument if match.id is already in the db

		// TODO: add subquery or INSERT INTO ... SELECT
		// to conform with latest db model
		_conn.prepareCached(
			"INSERT INTO matches (match_id, rule_set, searching_for_player) "
			"VALUES (:id, :ruleSet, :searchingForPlayer)",
			"addMatch" // statement cache key
			)
			.set("id", match.id)
			.set("ruleSet", RuleSetToStr(match.ruleSet))
			.set("searchingForPlayer", match.searchingForPlayer)
			.execute();
	}

	std::string MatchManager::newMatchID()
	{
		std::string matchID;

		while(true)
			try
			{
				matchID = int24ToB64ID(_int24Generator());
				_conn.prepareCached("SELECT * FROM matches WHERE match_id = :matchID", "searchMatchID")
					.set("matchID", matchID)
					.selectRow();
			}
			catch(tntdb::NotFound&)
			{
				break;
			}

		return matchID;
	}
}
