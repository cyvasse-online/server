#include "match_data.hpp"

#include <cyvmath/match.hpp>

MatchData::MatchData(const std::string& id, RuleSet ruleSet, std::unique_ptr<Match> match)
	: _id(id)
	, _ruleSet(ruleSet)
	, _match(std::move(match))
{ }
