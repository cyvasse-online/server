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

ENUM_STR(ActionType, (initMap<ActionType, const char*> {
	{ACTION_UNDEFINED, "undefined"},
	{ACTION_CREATE_GAME, "create game"},
	{ACTION_JOIN_GAME, "join game"},
	{ACTION_RESUME_GAME, "resume game"},
	{ACTION_START, "start"},
	{ACTION_MOVE_PIECE, "move piece"},
	{ACTION_RESIGN, "resign"}
}))

ENUM_STR(RuleSet, (initMap<RuleSet, const char*> {
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
				param["color"] = PlayersColorToStr(createGame.color);
				break;
			case ACTION_JOIN_GAME:
				param["b64ID"] = joinGame.b64ID;
				break;
			case ACTION_RESUME_GAME:
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
		return;
	}

	action = StrToActionType(data.get("action", "undefined").asString());

	if(!data.isMember("param"))
		action = ACTION_UNDEFINED;

	switch(action)
	{
		case ACTION_CREATE_GAME:
			createGame.ruleSet = StrToRuleSet(data["param"].get("ruleSet", "undefined").asString());
			break;
		case ACTION_JOIN_GAME:
			{
				std::string tmp = data["param"].get("b64ID", "").asString();
				if(tmp.length() == 10)
					strcpy(joinGame.b64ID, tmp.c_str());
				else
					joinGame.b64ID[0] = '\0';
			}
			break;
		case ACTION_RESUME_GAME:
			break;
		case ACTION_START:
			break;
		case ACTION_MOVE_PIECE:
			break;
		case ACTION_RESIGN:
			break;
	}
}
