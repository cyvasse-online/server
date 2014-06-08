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

#include "job_data.hpp"

#include <map>
#include <cstring>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>

ENUM_STR(ActionType, (initMap<ActionType, std::string> {
	{ACTION_UNDEFINED, "undefined"},
	{ACTION_CREATE_GAME, "create game"},
	{ACTION_JOIN_GAME, "join game"},
	{ACTION_RESUME_GAME, "resume game"},
	{ACTION_START, "start"},
	{ACTION_MOVE_PIECE, "move piece"},
	{ACTION_RESIGN, "resign"}
}))

ENUM_STR(RuleSet, (initMap<RuleSet, std::string> {
	{RULESET_UNDEFINED, "undefined"},
	{RULESET_MIKELEPAGE, "mikelepage"}
}))

std::string JobData::serialize()
{
	Json::Value data;

	data["action"] = ActionTypeToStr(action);

	if(action != ACTION_UNDEFINED)
	{
		Json::Value& param = data["param"];

		switch(action)
		{
			case ACTION_CREATE_GAME:
				param["ruleSet"] = RuleSetToStr(createGame.ruleSet);
				param["color"]  = PlayersColorToStr(createGame.color);
				break;
			case ACTION_JOIN_GAME:
				param["matchID"] = joinGame.matchID;
				break;
			case ACTION_RESUME_GAME:
				param["playerID"] = resumeGame.playerID;
				break;
			case ACTION_START:
				break;
			case ACTION_MOVE_PIECE:
				break;
			case ACTION_RESIGN:
				break;
		}
	}

	return Json::FastWriter().write(data);
}

void JobData::deserialize(const std::string& json)
{
	Json::Value data;
	Json::Reader reader;

	if(!reader.parse(json, data, false))
	{
		// jsoncpp 0.5 has a typo: getFormattedErrorMessages misses a 't'
		// TODO: check jsoncpp version through autoconf somehow
		std::cerr << "JobData::deserialize() failed:\n"
		          << reader.getFormatedErrorMessages();
		error = "parsing the json failed";
		return;
	}

	action = StrToActionType(data.get("action", "undefined").asString());

	if(action == ACTION_UNDEFINED)
	{
		error = "the requested action is unknown";
		return;
	}

	const Json::Value& param = data["param"];

	if(!param.isObject())
	{
		error = "param missing or not an object";
		return;
	}

	switch(action)
	{
		case ACTION_CREATE_GAME:
			// TODO: error checking
			createGame.ruleSet = StrToRuleSet(param.get("ruleSet", "undefined").asString());
			createGame.color   = StrToPlayersColor(param["color"].asString());
			break;
		case ACTION_JOIN_GAME:
			{
				std::string tmp = param["matchID"].asString();
				if(tmp.length() == 4)
					strcpy(joinGame.matchID, tmp.c_str());
				else
					joinGame.matchID[0] = '\0';
			}
			break;
		case ACTION_RESUME_GAME:
			{
				std::string tmp = param["playerID"].asString();
				if(tmp.length() == 8)
					strcpy(resumeGame.playerID, tmp.c_str());
				else
					resumeGame.playerID[0] = '\0';
			}
			break;
		case ACTION_START:
			break;
		case ACTION_MOVE_PIECE:
			break;
		case ACTION_RESIGN:
			break;
	}
}
