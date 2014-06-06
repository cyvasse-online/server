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

#ifndef _JOB_DATA_HPP_
#define _JOB_DATA_HPP_

#include <string>
#include <cyvmath/common.hpp>

using namespace cyvmath;

enum ActionType
{
	ACTION_UNDEFINED,
	ACTION_CREATE_GAME,
	ACTION_JOIN_GAME,
	ACTION_RESUME_GAME,
	ACTION_START,
	ACTION_MOVE_PIECE,
	ACTION_RESIGN
};

ENUM_STR_PROT(ActionType)

enum RuleSet
{
	RULESET_UNDEFINED,
	RULESET_MIKELEPAGE
};

ENUM_STR_PROT(RuleSet)

struct JobData
{
	struct CreateGameData
	{
		RuleSet ruleSet;
		PlayersColor color;
	};

	struct JoinGameData
	{
		char b64ID[5];
	};

	struct ResumeGameData
	{
		char resumeToken[9];
	};

	struct StartData
	{
		// piece positions
	};

	struct MovePieceData
	{
		// from, to
	};

	ActionType action;
	union
	{
		CreateGameData createGame;
		JoinGameData joinGame;
		ResumeGameData resumeGame;
		StartData start;
		MovePieceData movePiece;
	};

	JobData() = default;
	JobData(const std::string& json)
	{
		deserialize(json);
	}

	std::string serialize();
	void deserialize(const std::string& json);
};

#endif // _JOB_DATA_HPP_
