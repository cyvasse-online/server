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

#include "match.hpp"

using namespace cyvmath;

namespace cyvdb
{
	std::string Match::getValidOrEmptyID(const std::string& id)
	{
		if(id.length() == 4) return id;
		else                 return std::string();
	}

	Match::Match(const std::string& id, RuleSet ruleSet, bool searchingForPlayer)
		: _id(getValidOrEmptyID(id))
		, _ruleSet(ruleSet)
		, _searchingForPlayer(searchingForPlayer)
	{ }

	Match::Match(const char* id, RuleSet ruleSet, bool searchingForPlayer)
		: Match(std::string(id), ruleSet, searchingForPlayer) // constructor delegation
	{ }

	bool Match::valid() const
	{
		return _id[0] != '\0' // _id non-empty
			&& _ruleSet != RULESET_UNDEFINED;
	}
}
