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

namespace cyvmath { class Player; }
using cyvmath::Player;

class MatchData;

class ClientData
{
	private:
		std::string _id;
		std::unique_ptr<Player> _player;

		websocketpp::connection_hdl _connHdl;

		MatchData& _matchData;

	public:
		ClientData(const std::string& id, std::unique_ptr<Player> player,
		           websocketpp::connection_hdl hdl, MatchData& matchData)
			: _id(id)
			, _player(std::move(player))
			, _connHdl(hdl)
			, _matchData(matchData)
		{ }

		const std::string& getID() const
		{ return _id; }

		const std::unique_ptr<Player>& getPlayer() const
		{ return _player; }

		websocketpp::connection_hdl getConnHdl()
		{ return _connHdl; }

		MatchData& getMatchData()
		{ return _matchData; }

		const MatchData& getMatchData() const
		{ return _matchData; }

		bool operator==(const ClientData& other) const
		{ return _id == other._id; }

		bool operator!=(const ClientData& other) const
		{ return _id != other._id; }
};

#endif // _CLIENT_DATA_HPP_
