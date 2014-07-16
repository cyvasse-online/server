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

#include "player.hpp"

using namespace cyvmath;

namespace cyvdb
{
	std::string Player::getValidOrEmptyID(const std::string& id)
	{
		if(id.length() == 8) return id;
		else                 return std::string();
	}

	Player::Player(const std::string& id, const std::string& matchID, PlayersColor color)
		: cyvmath::Player(color)
		, _id(getValidOrEmptyID(id))
		, _matchID(matchID)
	{ }

	Player::Player(const char* id, const std::string& matchID, PlayersColor color)
		: Player(std::string(id), matchID, color)
	{ }

	bool Player::valid() const
	{
		return _id[0] != '\0' // _id non-empty
			&& _color != PLAYER_UNDEFINED;
	}
}
