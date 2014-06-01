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

const char* actionTypeToStr(ActionType a)
{
	static const char* data[] = {
			"undefined",
			"create game",
			"join game"
		};

	return data[a];
}

ActionType strToActionType(const std::string& s)
{
	static std::map<std::string, ActionType> data = {
			{"create game", CREATE_GAME},
			{"join game", JOIN_GAME}
		};

	auto it = data.find(s);
	if(it == data.end())
		return UNDEFINED;

	return it->second;
}

const char* ruleSetToStr(JobData::CreateGameData::RuleSet r)
{
	static const char* data[] = {
			"undefined",
			"mikelepage",
		};

	return data[r];
}

JobData::CreateGameData::RuleSet strToRuleSet(const std::string& s)
{
	static std::map<std::string, JobData::CreateGameData::RuleSet> data = {
			{"mikelepage", JobData::CreateGameData::MIKELEPAGE}
		};

	auto it = data.find(s);
	if(it == data.end())
		return JobData::CreateGameData::UNDEFINED;

	return it->second;
}

std::string JobData::serialize()
{
	Json::Value data;

	data["action"] = actionTypeToStr(action);

	switch(action)
	{
		case CREATE_GAME:
			data["param"]["ruleSet"] = ruleSetToStr(createGame.ruleSet);
			break;
		case JOIN_GAME:
			data["param"]["b64ID"] = joinGame.b64ID;
			break;
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

	action = strToActionType(data.get("action", "undefined").asString());

	if(!data.isMember("param"))
		action = UNDEFINED;

	switch(action)
	{
		case CREATE_GAME:
			createGame.ruleSet = strToRuleSet(data["param"].get("ruleSet", "undefined").asString());
			break;
		case JOIN_GAME:
			{
				std::string tmp = data["param"].get("b64ID", "").asString();
				if(tmp.length() == 10)
					strcpy(joinGame.b64ID, tmp.c_str());
				else
					joinGame.b64ID[0] = '\0';
			}
			break;
	}
}
