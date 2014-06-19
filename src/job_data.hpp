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
#include <cyvmath/enum_str.hpp>
#include <cyvmath/players_color.hpp>
#include <cyvmath/rule_sets.hpp>

enum MessageType
{
	MESSAGE_UNDEFINED,
	MESSAGE_REQUEST,
	MESSAGE_REPLY
};

ENUM_STR_PROT(MessageType)

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

struct JobData
{
	struct RequestData
	{
		struct CreateGameData
		{
			cyvmath::RuleSet ruleSet;
			cyvmath::PlayersColor color;
		};

		struct JoinGameData
		{
			char matchID[5];
		};

		struct ResumeGameData
		{
			char playerID[9];
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
	};

	struct ReplyData
	{
		bool success;
	};

	MessageType messageType;
	unsigned messageID;

	std::string error;

	union
	{
		RequestData requestData;
		ReplyData replyData;
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
