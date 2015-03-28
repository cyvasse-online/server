/* Copyright 2014 - 2015 Jonas Platte
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
#include <cassert>
#include <cyvmath/match.hpp>
#include <cyvmath/rule_sets.hpp>

class ClientData;

class MatchData
{
	public:
		typedef std::shared_ptr<ClientData> ClientDataPtr;
		typedef std::set<ClientDataPtr, std::owner_less<ClientDataPtr>> ClientDataSets;

	private:
		std::unique_ptr<cyvmath::Match> m_match;

		ClientDataSets m_clientDataSets;

	public:
		MatchData(std::unique_ptr<cyvmath::Match> match)
			: m_match(std::move(match))
		{
			assert(m_match);
		}

		cyvmath::Match& getMatch()
		{ return *m_match; }

		ClientDataSets& getClientDataSets()
		{ return m_clientDataSets; }

		const ClientDataSets& getClientDataSets() const
		{ return m_clientDataSets; }

		/*bool operator==(const MatchData& other) const
		{ return m_id == other.m_id; }

		bool operator!=(const MatchData& other) const
		{ return m_id != other.m_id; }*/
};

#endif // _MATCH_DATA_HPP_
