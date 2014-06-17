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

#ifndef _PLAYER_HPP_
#define _PLAYER_HPP_

#include <string>
#include <cyvmath/players_color.hpp>

namespace cyvdb
{
	class Player
	{
		private:
			const std::string _id;

			const std::string _matchID;
			cyvmath::PlayersColor _color;

		public:
			static std::string getValidOrEmptyID(const std::string&);

			Player(const std::string& id = std::string(), const std::string& matchID = std::string(),
			       cyvmath::PlayersColor = cyvmath::PLAYER_UNDEFINED);
			Player(const char* id, const std::string& matchID = std::string(),
			       cyvmath::PlayersColor = cyvmath::PLAYER_UNDEFINED);

			bool valid() const;

			explicit operator bool() const
			{
				return valid();
			}
	};
}

#endif // _PLAYER_HPP_
