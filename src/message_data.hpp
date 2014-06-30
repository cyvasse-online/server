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
#include <boost/variant/get.hpp>
#include <boost/variant/variant.hpp>
#include <cyvmath/coordinate.hpp>
#include <cyvmath/enum_str.hpp>
#include <cyvmath/piece.hpp>
#include <cyvmath/players_color.hpp>
#include <cyvmath/rule_sets.hpp>

#include <iostream>

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
	ACTION_RESIGN,
	ACTION_CHAT_MSG
};

ENUM_STR_PROT(ActionType)

struct MessageData
{
	struct RequestData
	{
		struct CreateGameParam
		{
			cyvmath::RuleSet ruleSet;
			cyvmath::PlayersColor color;

			CreateGameParam()
				: ruleSet(cyvmath::RULESET_UNDEFINED)
				, color(cyvmath::PLAYER_UNDEFINED)
			{ }
		};

		CreateGameParam& getCreateGameParam()
		{ return boost::get<CreateGameParam>(param); }

		struct JoinGameParam
		{
			std::string matchID;
		};

		JoinGameParam& getJoinGameParam()
		{ return boost::get<JoinGameParam>(param); }

		struct ResumeGameParam
		{
			std::string playerID;
		};

		ResumeGameParam& getResumeGameParam()
		{ return boost::get<ResumeGameParam>(param); }

		struct StartParam
		{
			std::vector<std::pair<cyvmath::CoordinateDcUqP, cyvmath::PieceType>> positions;
		};

		StartParam& getStartParam()
		{ return boost::get<StartParam>(param); }

		struct MovePieceParam
		{
			cyvmath::CoordinateDcUqP startPos;
			cyvmath::CoordinateDcUqP targetPos;
			// debugging hint
			cyvmath::PieceType pieceType;

			MovePieceParam()
				: pieceType(cyvmath::PIECE_UNDEFINED)
			{ }
		};

		MovePieceParam& getMovePieceParam()
		{ return boost::get<MovePieceParam>(param); }

		struct ChatMsgParam
		{
			std::string content;
		};

		ChatMsgParam& getChatMsgParam()
		{ return boost::get<ChatMsgParam>(param); }

		ActionType action;

		boost::variant<
			CreateGameParam,
			JoinGameParam,
			ResumeGameParam,
			StartParam,
			MovePieceParam,
			ChatMsgParam
		> param;

		RequestData()
			: action(ACTION_UNDEFINED)
		{ }
	};

	RequestData& getRequestData()
	{ return boost::get<RequestData>(data); }

	struct ReplyData
	{
		struct CreateGameParam
		{
			std::string matchID;
			std::string playerID;
		};

		CreateGameParam& getCreateGameParam()
		{ return boost::get<CreateGameParam>(param); }

		struct JoinGameParam
		{
			cyvmath::RuleSet ruleSet;
			std::string playerID;
		};

		JoinGameParam& getJoinGameParam()
		{ return boost::get<JoinGameParam>(param); }

		bool success;
		std::string error;
		boost::variant<CreateGameParam, JoinGameParam> param;
	};

	ReplyData& getReplyData()
	{ return boost::get<ReplyData>(data); }

	MessageType messageType;
	unsigned messageID;
	boost::variant<RequestData, ReplyData> data;

	MessageData()
		: messageType(MESSAGE_UNDEFINED)
		, messageID(0)
	{ }

	MessageData(const std::string& json)
	{
		deserialize(json);
	}

	std::string serialize();
	void deserialize(const std::string& json);
};

class MessageDataParseError : public std::runtime_error
{
	public:
		explicit MessageDataParseError(const std::string& what_arg)
			: std::runtime_error(what_arg)
		{ }
		explicit MessageDataParseError(const char* what_arg)
			: std::runtime_error(what_arg)
		{ }
};

#endif // _JOB_DATA_HPP_
