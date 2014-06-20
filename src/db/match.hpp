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

#ifndef _MATCH_HPP_
#define _MATCH_HPP_

#include <string>
#include <cyvmath/rule_sets.hpp>

namespace cyvdb
{
	class Match
	{
		private:
			const std::string _id;

			cyvmath::RuleSet _ruleSet;
			bool _searchingForPlayer;

		public:
			static std::string getValidOrEmptyID(const std::string&);

			Match(const std::string& id = std::string(), cyvmath::RuleSet = cyvmath::RULESET_UNDEFINED,
			      bool searchingForPlayer = true);
			Match(const char* id, cyvmath::RuleSet = cyvmath::RULESET_UNDEFINED, bool searchingForPlayer = true);

			bool valid() const;

			explicit operator bool() const
			{
				return valid();
			}
	};
}

#endif // _MATCH_HPP_