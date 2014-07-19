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

#ifndef _MATCH_DATA_HPP_
#define _MATCH_DATA_HPP_

#include <memory>
#include <set>
#include <cyvmath/rule_sets.hpp>

namespace cyvmath { class Match; }
using cyvmath::Match;
using cyvmath::RuleSet;

class ClientData;

class MatchData
{
	public:
		typedef std::shared_ptr<ClientData> ClientDataPtr;
		typedef std::set<ClientDataPtr, std::owner_less<ClientDataPtr>> ClientDataSets;

	private:
		std::string _id;
		RuleSet _ruleSet;
		std::unique_ptr<Match> _match;

		ClientDataSets _clientDataSets;

	public:
		MatchData(const std::string& id, RuleSet, std::unique_ptr<Match>);

		const std::string& getID() const
		{ return _id; }

		RuleSet getRuleSet() const
		{ return _ruleSet; }

		const std::unique_ptr<Match>& getMatch() const
		{ return _match; }

		ClientDataSets& getClientDataSets()
		{ return _clientDataSets; }

		const ClientDataSets& getClientDataSets() const
		{ return _clientDataSets; }

		bool operator==(const MatchData& other) const
		{ return _id == other._id; }

		bool operator!=(const MatchData& other) const
		{ return _id != other._id; }
};

#endif // _MATCH_DATA_HPP_
