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

#include "message_data.hpp"

#include <map>
#include <boost/variant/static_visitor.hpp>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>

using namespace cyvmath;

ENUM_STR(MessageType, ({
	{MESSAGE_UNDEFINED, "undefined"},
	{MESSAGE_REQUEST, "request"},
	{MESSAGE_REPLY, "reply"}
}))

ENUM_STR(ActionType, ({
	{ACTION_UNDEFINED, "undefined"},
	{ACTION_CREATE_GAME, "create game"},
	{ACTION_JOIN_GAME, "join game"},
	{ACTION_RESUME_GAME, "resume game"},
	{ACTION_START, "start"},
	{ACTION_MOVE_PIECE, "move piece"},
	{ACTION_RESIGN, "resign"}
}))

class getReplyParam : public boost::static_visitor<Json::Value>
{
	public:
		Json::Value operator()(MessageData::ReplyData::CreateGameParam& data) const
		{
			if(data.matchID.empty()) return Json::Value();

			Json::Value val;
			val["matchID"]  = data.matchID;
			val["playerID"] = data.playerID;

			return val;
		}

		Json::Value operator()(MessageData::ReplyData::JoinGameParam& data) const
		{
			Json::Value val;
			val["ruleSet"]  = RuleSetToStr(data.ruleSet);
			val["playerID"] = data.playerID;
		}
};

std::string MessageData::serialize()
{
	Json::Value json;

	json["messageType"] = MessageTypeToStr(messageType);
	json["messageID"] = messageID;
	switch(messageType)
	{
		case MESSAGE_REQUEST:
		{
			auto& requestData = getRequestData();

			json["action"] = ActionTypeToStr(requestData.action);

			if(requestData.action != ACTION_UNDEFINED)
			{
				Json::Value& param = json["param"];

				switch(requestData.action)
				{
					case ACTION_CREATE_GAME:
					{
						auto& createGameData = requestData.getCreateGameParam();

						param["ruleSet"] = RuleSetToStr(createGameData.ruleSet);
						param["color"]  = PlayersColorToStr(createGameData.color);
						break;
					}
					case ACTION_JOIN_GAME:
					{
						auto& joinGameData = requestData.getJoinGameParam();
						param["matchID"] = joinGameData.matchID;
						break;
					}
					case ACTION_RESUME_GAME:
					{
						auto& resumeGameData = requestData.getResumeGameParam();

						param["playerID"] = resumeGameData.playerID;
						break;
					}
					case ACTION_START:
					{
						break;
					}
					case ACTION_MOVE_PIECE:
					{
						break;
					}
					case ACTION_RESIGN:
					{
						break;
					}
				}
			}
			break;
		}
		case MESSAGE_REPLY:
		{
			auto& replyData = getReplyData();

			json["success"] = replyData.success;
			if(!replyData.success)
				json["error"] = replyData.error;

			json["data"] = boost::apply_visitor(getReplyParam(), replyData.param);
			break;
		}
		// default: do nothing
	}

	return Json::FastWriter().write(json);
}

void MessageData::deserialize(const std::string& text)
{
	Json::Value json;
	Json::Reader reader;

	if(!reader.parse(text, json, false))
	{
		// jsoncpp 0.5 has a typo: getFormattedErrorMessages misses a 't'
		// TODO: check jsoncpp version through autoconf somehow
		std::cerr << "MessageData::deserialize() failed:\n"
		          << reader.getFormatedErrorMessages();
		throw MessageDataParseError("JSON parsing failed");
	}

	messageType = StrToMessageType(json.get("messageType", "undefined").asString());
	messageID = json.get("messageID", 0).asUInt();

	switch(messageType)
	{
		case MESSAGE_REQUEST:
		{
			data = RequestData();
			auto& requestData = getRequestData();

			requestData.action = StrToActionType(json.get("action", "undefined").asString());

			if(requestData.action == ACTION_UNDEFINED)
				throw MessageDataParseError("Unknown action");

			const Json::Value& param = json["param"];

			if(!param.isObject())
				throw MessageDataParseError("Member param missing or not an object");

			switch(requestData.action)
			{
				case ACTION_CREATE_GAME:
				{
					requestData.param = RequestData::CreateGameParam();
					auto& createGameData = requestData.getCreateGameParam();

					createGameData.ruleSet = StrToRuleSet(param["ruleSet"].asString());
					createGameData.color   = StrToPlayersColor(param["color"].asString());
					break;
				}
				case ACTION_JOIN_GAME:
				{
					requestData.param = RequestData::JoinGameParam();
					auto& joinGameData = requestData.getJoinGameParam();

					std::string tmp = param["matchID"].asString();
					if(tmp.length() == 4)
						joinGameData.matchID = tmp;
					else
						joinGameData.matchID[0] = '\0';
					break;
				}
				case ACTION_RESUME_GAME:
				{
					requestData.param = RequestData::ResumeGameParam();
					auto& resumeGameData = requestData.getResumeGameParam();

					std::string tmp = param["playerID"].asString();
					if(tmp.length() == 8)
						resumeGameData.playerID = tmp;
					else
						resumeGameData.playerID[0] = '\0';
					break;
				}
				case ACTION_START:
					break;
				case ACTION_MOVE_PIECE:
					break;
				case ACTION_RESIGN:
					break;
			}
			break;
		}
		case MESSAGE_REPLY:
		{
			data = ReplyData();
			auto& replyData = getReplyData();

			replyData.success = json.get("success", false).asBool();
			replyData.error = json.get("error", "").asString();
			break;
		}
		default:
			throw MessageDataParseError("Unknown message type");
	}
}
