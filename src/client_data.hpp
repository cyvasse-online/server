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

#ifndef _CLIENT_DATA_HPP_
#define _CLIENT_DATA_HPP_

#include <memory>
#include <websocketpp/common/connection_hdl.hpp>
#include <cyvmath/player.hpp>

using cyvmath::Player;

class MatchData;

class ClientData
{
	private:
		std::string m_id;
		std::unique_ptr<Player> m_player;

		websocketpp::connection_hdl m_connHdl;

		MatchData& m_matchData;

	public:
		ClientData(const std::string& id, std::unique_ptr<Player> player,
		           websocketpp::connection_hdl hdl, MatchData& matchData)
			: m_id(id)
			, m_player(std::move(player))
			, m_connHdl(hdl)
			, m_matchData(matchData)
		{ }

		const std::string& getID() const
		{ return m_id; }

		const std::unique_ptr<Player>& getPlayer() const
		{ return m_player; }

		websocketpp::connection_hdl getConnHdl()
		{ return m_connHdl; }

		MatchData& getMatchData()
		{ return m_matchData; }

		const MatchData& getMatchData() const
		{ return m_matchData; }

		bool operator==(const ClientData& other) const
		{ return m_id == other.m_id; }

		bool operator!=(const ClientData& other) const
		{ return m_id != other.m_id; }
};

#endif // _CLIENT_DATA_HPP_
