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

#include <stdexcept>
#include <string>
#include <cyvmath/coordinate.hpp>
#include <cyvmath/enum_str.hpp>
#include <cyvmath/piece.hpp>
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
			std::string matchID;
		};

		struct ResumeGameData
		{
			std::string playerID;
		};

		struct StartData
		{
			// TODO: make this a dynamic list when the union is replaced
			// has to be done at latest when an additional rule set is added
			std::pair<cyvmath::CoordinateDcUqP, cyvmath::PieceType> positions;
		};

		struct MovePieceData
		{
			cyvmath::CoordinateDcUqP startPos;
			cyvmath::CoordinateDcUqP targetPos;
			// debugging hint
			cyvmath::PieceType pieceType;
		};

		ActionType action;
		// Without this being a union, this struct is (relatively seen)
		// much bigger. This problem will be fixed soon though.
		//union
		//{
			CreateGameData createGame;
			JoinGameData joinGame;
			ResumeGameData resumeGame;
			StartData start;
			MovePieceData movePiece;
		//};
	};

	struct ReplyData
	{
		bool success;
		std::string error;
	};

	MessageType messageType;
	unsigned messageID;

	//union
	//{
		RequestData requestData;
		ReplyData replyData;
	//};

	JobData();
	JobData(const std::string& json)
	{
		deserialize(json);
	}

	std::string serialize();
	void deserialize(const std::string& json);
};

class JobDataParseError : public std::runtime_error
{
	public:
		explicit JobDataParseError(const std::string& what_arg)
			: std::runtime_error(what_arg)
		{ }
		explicit JobDataParseError(const char* what_arg)
			: std::runtime_error(what_arg)
		{ }
};

#endif // _JOB_DATA_HPP_
