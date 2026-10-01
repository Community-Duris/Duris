#include "world/zone_story_quest_feature.h"

#include "core/defines.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <tuple>
#include <utility>

namespace zone_story_quest_feature
{
namespace
{
bool fail(std::string *error, std::string message)
{
	if (error)
		*error = std::move(message);
	return false;
}

char hex_digit(uint8_t value)
{
	return value < 10 ? static_cast<char>('0' + value) : static_cast<char>('a' + value - 10);
}

int hex_value(char value)
{
	if (value >= '0' && value <= '9')
		return value - '0';
	if (value >= 'a' && value <= 'f')
		return value - 'a' + 10;
	if (value >= 'A' && value <= 'F')
		return value - 'A' + 10;
	return -1;
}

std::string hex_encode(std::string_view value)
{
	std::string encoded;
	encoded.reserve(value.size() * 2);
	for (unsigned char byte : value)
	{
		encoded.push_back(hex_digit(static_cast<uint8_t>(byte >> 4)));
		encoded.push_back(hex_digit(static_cast<uint8_t>(byte & 0x0f)));
	}
	return encoded;
}

bool hex_decode(std::string_view encoded, std::string *decoded)
{
	if (!decoded || encoded.size() % 2 != 0)
		return false;
	decoded->clear();
	decoded->reserve(encoded.size() / 2);
	for (size_t index = 0; index < encoded.size(); index += 2)
	{
		const int high = hex_value(encoded[index]);
		const int low = hex_value(encoded[index + 1]);
		if (high < 0 || low < 0)
			return false;
		decoded->push_back(static_cast<char>((high << 4) | low));
	}
	return true;
}

template <typename T> bool parse_integer(std::string_view token, T *value)
{
	if (!value || token.empty())
		return false;
	T parsed = {};
	const char *begin = token.data();
	const char *end = begin + token.size();
	const auto parsed_result = std::from_chars(begin, end, parsed);
	if (parsed_result.ec != std::errc() || parsed_result.ptr != end)
		return false;
	*value = parsed;
	return true;
}

std::vector<std::string_view> split(std::string_view line)
{
	std::vector<std::string_view> fields;
	size_t begin = 0;
	while (begin <= line.size())
	{
		const size_t separator = line.find('|', begin);
		if (separator == std::string_view::npos)
		{
			fields.push_back(line.substr(begin));
			break;
		}
		fields.push_back(line.substr(begin, separator - begin));
		begin = separator + 1;
	}
	return fields;
}

std::string lower_name(std::string_view name)
{
	std::string lowered(name);
	std::transform(lowered.begin(), lowered.end(), lowered.begin(),
		       [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
	return lowered;
}

const char *color(bool enabled, const char *code)
{
	return enabled ? code : "";
}

bool valid_period(int64_t period_seconds)
{
	return period_seconds >= 60 && period_seconds <= 7 * 24 * 60 * 60;
}

const char *racewar_name_color(int racewar)
{
	switch (racewar)
	{
	case RACEWAR_GOOD:
		return "&+Y";
	case RACEWAR_EVIL:
		return "&+R";
	default:
		return "";
	}
}

constexpr int64_t leaderboard_delay_seconds = 12 * 60 * 60;
constexpr int64_t no_completion_cutoff = std::numeric_limits<int64_t>::max();

int64_t leaderboard_cutoff(int64_t now)
{
	if (now <= 0)
		now = static_cast<int64_t>(std::time(nullptr));
	return now > leaderboard_delay_seconds ? now - leaderboard_delay_seconds : 0;
}

const char *status_name(daily_status status)
{
	switch (status)
	{
	case daily_status::none:
		return "none";
	case daily_status::assigned:
		return "assigned";
	case daily_status::completed:
		return "completed";
	case daily_status::expired:
		return "expired";
	case daily_status::no_eligible_candidate:
		return "no_eligible_candidate";
	case daily_status::disabled:
		return "disabled";
	}
	return "none";
}

bool parse_status(std::string_view value, daily_status *status)
{
	if (!status)
		return false;
	if (value == "none")
		*status = daily_status::none;
	else if (value == "assigned")
		*status = daily_status::assigned;
	else if (value == "completed")
		*status = daily_status::completed;
	else if (value == "expired")
		*status = daily_status::expired;
	else if (value == "no_eligible_candidate")
		*status = daily_status::no_eligible_candidate;
	else if (value == "disabled")
		*status = daily_status::disabled;
	else
		return false;
	return true;
}

const char *outcome_name(telemetry_outcome outcome)
{
	switch (outcome)
	{
	case telemetry_outcome::success:
		return "success";
	case telemetry_outcome::failure:
		return "failure";
	case telemetry_outcome::abandoned:
		return "abandoned";
	case telemetry_outcome::inaccessible:
		return "inaccessible";
	case telemetry_outcome::stale_revision:
		return "stale_revision";
	}
	return "failure";
}

bool parse_outcome(std::string_view value, telemetry_outcome *outcome)
{
	if (!outcome)
		return false;
	if (value == "success")
		*outcome = telemetry_outcome::success;
	else if (value == "failure")
		*outcome = telemetry_outcome::failure;
	else if (value == "abandoned")
		*outcome = telemetry_outcome::abandoned;
	else if (value == "inaccessible")
		*outcome = telemetry_outcome::inaccessible;
	else if (value == "stale_revision")
		*outcome = telemetry_outcome::stale_revision;
	else
		return false;
	return true;
}

std::string serialize_observation(const telemetry_observation &observation)
{
	return hex_encode(observation.observation_id) + ":" +
	       hex_encode(observation.quest_definition_id) + ":" +
	       std::to_string(observation.content_revision) + ":" +
	       std::to_string(observation.observed_at) + ":" + std::to_string(observation.pid) +
	       ":" + std::to_string(observation.level) + ":" + std::to_string(observation.racewar) +
	       ":" + std::to_string(observation.credit_mask) + ":" +
	       std::to_string(observation.party_size) + ":" +
	       std::to_string(observation.strongest_party_level) + ":" +
	       std::to_string(observation.duration_seconds) + ":" +
	       outcome_name(observation.outcome) + ":" + (observation.accessible ? "1" : "0");
}

bool deserialize_observation(std::string_view encoded, telemetry_observation *observation)
{
	if (!observation)
		return false;
	const std::vector<std::string_view> fields = [&]()
	{
		std::vector<std::string_view> result;
		size_t begin = 0;
		while (begin <= encoded.size())
		{
			const size_t separator = encoded.find(':', begin);
			if (separator == std::string_view::npos)
			{
				result.push_back(encoded.substr(begin));
				break;
			}
			result.push_back(encoded.substr(begin, separator - begin));
			begin = separator + 1;
		}
		return result;
	}();
	if (fields.size() != 13 || !hex_decode(fields[0], &observation->observation_id) ||
	    !hex_decode(fields[1], &observation->quest_definition_id) ||
	    !parse_integer(fields[2], &observation->content_revision) ||
	    !parse_integer(fields[3], &observation->observed_at) ||
	    !parse_integer(fields[4], &observation->pid) ||
	    !parse_integer(fields[5], &observation->level) ||
	    !parse_integer(fields[6], &observation->racewar) ||
	    !parse_integer(fields[7], &observation->credit_mask) ||
	    !parse_integer(fields[8], &observation->party_size) ||
	    !parse_integer(fields[9], &observation->strongest_party_level) ||
	    !parse_integer(fields[10], &observation->duration_seconds) ||
	    !parse_outcome(fields[11], &observation->outcome) ||
	    (fields[12] != "0" && fields[12] != "1"))
		return false;
	observation->accessible = fields[12] == "1";
	return !observation->observation_id.empty() && !observation->quest_definition_id.empty() &&
	       observation->content_revision > 0 && observation->observed_at > 0 &&
	       observation->pid > 0 && observation->level >= 0 && observation->party_size <= 1000 &&
	       observation->strongest_party_level >= 0 && observation->duration_seconds >= 0;
}

bool same_score(const leaderboard_entry &left, const leaderboard_entry &right)
{
	return left.completed == right.completed && left.total == right.total;
}

std::string display_count(uint64_t value)
{
	std::string output = std::to_string(value);
	for (size_t position = output.size(); position > 3;)
	{
		position -= 3;
		output.insert(position, 1, ',');
	}
	return output;
}

std::string display_percentage(uint64_t completed, uint64_t total)
{
	if (!total)
		return "N/A";
	const double percentage =
		static_cast<double>(completed) * 100.0 / static_cast<double>(total);
	if (completed > 0 && percentage < 0.01)
		return "<0.01%";
	std::ostringstream output;
	output << std::fixed << std::setprecision(2) << percentage << "%";
	return output.str();
}

/* Compare non-negative fractions without multiplying the operands.  The
 * continued-fraction form is exact even if a future catalog grows beyond the
 * range where numerator*denominator fits in a machine integer. */
int compare_fractions(uint64_t left_numerator, uint64_t left_denominator, uint64_t right_numerator,
		      uint64_t right_denominator)
{
	if (left_denominator == 0 || right_denominator == 0)
		return left_denominator == right_denominator ? 0 : left_denominator == 0 ? -1 : 1;
	bool reverse = false;
	for (;;)
	{
		const uint64_t left_quotient = left_numerator / left_denominator;
		const uint64_t right_quotient = right_numerator / right_denominator;
		if (left_quotient != right_quotient)
		{
			const int result = left_quotient < right_quotient ? -1 : 1;
			return reverse ? -result : result;
		}
		left_numerator %= left_denominator;
		right_numerator %= right_denominator;
		if (left_numerator == 0 || right_numerator == 0)
		{
			const int result = left_numerator == right_numerator ? 0 :
					   left_numerator == 0		     ? -1 :
									       1;
			return reverse ? -result : result;
		}
		std::swap(left_numerator, left_denominator);
		std::swap(right_numerator, right_denominator);
		reverse = !reverse;
	}
}

bool better_score(const leaderboard_entry &left, const leaderboard_entry &right)
{
	if (left.total == 0 && right.total != 0)
		return false;
	if (left.total != 0 && right.total == 0)
		return true;
	if (left.total != 0 && right.total != 0)
	{
		const int ratio =
			compare_fractions(left.completed, left.total, right.completed, right.total);
		if (ratio != 0)
			return ratio > 0;
	}
	if (left.completed != right.completed)
		return left.completed > right.completed;
	if (left.total != right.total)
		return left.total < right.total;
	const std::string left_name = lower_name(left.character_name);
	const std::string right_name = lower_name(right.character_name);
	if (left_name != right_name)
		return left_name < right_name;
	return left.pid < right.pid;
}

bool contains_pid(const zone_story_quest_tracking::completion_transaction &transaction,
		  uint32_t pid)
{
	return std::find(transaction.credited_pids.begin(), transaction.credited_pids.end(), pid) !=
	       transaction.credited_pids.end();
}

bool crossing(uint64_t completed, uint64_t total, uint64_t threshold)
{
	return total > 0 && compare_fractions(completed, total, threshold, 100) >= 0;
}

std::string display_character_name(const personal_summary &summary)
{
	return summary.character_name.empty() ? "your character" : summary.character_name;
}

std::string display_zone_name(const zone_progress &progress)
{
	return progress.zone_name.empty() ? "This area" : progress.zone_name;
}

std::string display_quest_name(const zone_story_quest_tracking::quest_definition *definition)
{
	if (!definition)
		return "Daily quest";
	if (!definition->display_name.empty())
		return definition->display_name;
	if (!definition->giver_name.empty())
		return "A request from " + definition->giver_name;
	return "Daily quest";
}

std::string display_remaining(int64_t seconds)
{
	if (seconds < 0)
		seconds = 0;
	const int64_t hours = seconds / (60 * 60);
	const int64_t minutes = (seconds % (60 * 60)) / 60;
	if (hours > 0)
		return std::to_string(hours) + "h " + std::to_string(minutes) + "m";
	if (minutes > 0)
		return std::to_string(minutes) + "m";
	return "less than a minute";
}
} // namespace

service::service(zone_story_quest_catalog::catalog catalog)
{
	std::string ignored;
	set_catalog(std::move(catalog), &ignored);
}

bool service::set_catalog(zone_story_quest_catalog::catalog catalog, std::string *error)
{
	std::vector<zone_story_quest_catalog::diagnostic> diagnostics;
	if (!zone_story_quest_catalog::validate(catalog, &diagnostics))
	{
		if (error)
		{
			*error = diagnostics.empty() ? "invalid zone-story quest catalog" :
						       diagnostics.front().code + ": " +
							       diagnostics.front().message;
		}
		return false;
	}
	catalog_ = std::move(catalog);
	return true;
}

const zone_story_quest_catalog::catalog &service::catalog() const
{
	return catalog_;
}

void service::set_daily_policy(daily_policy policy)
{
	daily_policy_ = policy;
	daily_policy_.period_seconds = 24 * 60 * 60;
	if (daily_policy_.maximum_party_level_delta < 0)
		daily_policy_.maximum_party_level_delta = 0;
}

const daily_policy &service::get_daily_policy() const
{
	return daily_policy_;
}

service::character_state &service::state_for(uint32_t season_id, uint32_t pid)
{
	auto &state = characters_[{ season_id, pid }];
	state.season_id = season_id;
	state.pid = pid;
	return state;
}

const service::character_state *service::find_state(uint32_t season_id, uint32_t pid) const
{
	const auto found = characters_.find({ season_id, pid });
	return found == characters_.end() ? nullptr : &found->second;
}

const zone_story_quest_tracking::quest_definition *
service::find_definition(std::string_view definition_id) const
{
	for (const auto &definition : catalog_.definitions)
		if (definition.definition_id == definition_id)
			return &definition;
	return nullptr;
}

bool service::eligible_for_current_catalog(
	const zone_story_quest_tracking::completion_transaction &transaction,
	std::string *error) const
{
	const auto *definition = find_definition(transaction.quest_definition_id);
	if (!definition)
		return fail(error, "quest definition is not in the active catalog");
	if (!definition->active)
		return fail(error, "quest definition is not eligible for zone completion");
	if (definition->content_revision != catalog_.content_revision ||
	    transaction.content_revision != definition->content_revision)
		return fail(error, "quest completion uses a stale content revision");
	if (definition->zone_number != transaction.zone_number)
		return fail(error, "quest completion zone does not match the catalog");
	return true;
}

void service::award_daily_for(const zone_story_quest_tracking::completion_transaction &transaction)
{
	if (!daily_policy_.enabled)
		return;
	for (uint32_t pid : transaction.credited_pids)
	{
		if (deleted_characters_.find({ transaction.season_id, pid }) !=
		    deleted_characters_.end())
			continue;
		auto &state = state_for(transaction.season_id, pid);
		const int64_t period =
			period_for(transaction.completed_at, daily_policy_.period_seconds);
		auto assignment = state.daily_assignments.find(period);
		if (assignment == state.daily_assignments.end())
			continue;
		auto &daily = assignment->second;
		if (daily.status != daily_status::assigned ||
		    daily.quest_definition_id != transaction.quest_definition_id ||
		    daily.content_revision != transaction.content_revision ||
		    transaction.completed_at < daily.assigned_at ||
		    transaction.completed_at >= daily.expires_at || !contains_pid(transaction, pid))
			continue;
		const std::string reward_key = std::to_string(transaction.season_id) + ":" +
					       std::to_string(pid) + ":" + std::to_string(period);
		if (state.reward_keys.emplace(reward_key, transaction.transaction_id).second)
		{
			daily.status = daily_status::completed;
			daily.reward_amount = 1;
			daily.completion_transaction_id = transaction.transaction_id;
		}
	}
}

result
service::apply_transaction(const zone_story_quest_tracking::completion_transaction &transaction,
			   std::string_view character_name, int racewar, bool allow_stale,
			   bool award_daily, std::string *error)
{
	std::string encoded;
	encoded = zone_story_quest_tracking::serialize_transaction(transaction, error);
	if (encoded.empty())
		return result::invalid;
	if (!allow_stale && !eligible_for_current_catalog(transaction, error))
		return result::rejected;

	const auto existing = transactions_.find(transaction.transaction_id);
	if (existing != transactions_.end())
	{
		if (existing->second.encoded != encoded)
		{
			fail(error, "completion transaction ID was reused with different data");
			return result::conflict;
		}
		return result::already_applied;
	}

	dirty_transactions_.insert(transaction.transaction_id);
	transactions_.emplace(transaction.transaction_id,
			      stored_transaction{ transaction, encoded });
	for (uint32_t pid : transaction.credited_pids)
	{
		if (deleted_characters_.find({ transaction.season_id, pid }) !=
		    deleted_characters_.end())
			continue;
		auto &state = state_for(transaction.season_id, pid);
		dirty_characters_.insert({ transaction.season_id, pid });
		const auto first =
			state.first_completion_times.find(transaction.quest_definition_id);
		if (first == state.first_completion_times.end() ||
		    transaction.completed_at < first->second)
			state.first_completion_times[transaction.quest_definition_id] =
				transaction.completed_at;
		state.credit_masks[transaction.quest_definition_id] |=
			zone_story_quest_tracking::credit_mask_for_pid(transaction, pid);
		if (pid == transaction.direct_completer_pid && !character_name.empty())
			state.character_name = character_name;
		if (pid == transaction.direct_completer_pid && racewar != RACEWAR_NONE)
			state.racewar = racewar;
	}
	project_daily(transaction, award_daily);
	if (award_daily && transaction.schema_version == 1)
		award_daily_for(transaction);
	return result::applied;
}

result service::record_completion(const completion_event &event, std::string *error)
{
	if (event.outcome != telemetry_outcome::success)
		return fail(error, "completion event outcome must be success"), result::invalid;
	if (deleted_characters_.find(
		    { event.transaction.season_id, event.transaction.direct_completer_pid }) !=
	    deleted_characters_.end())
		return fail(error,
			    "direct completer is deleted and must re-identify before earning credit"),
		       result::rejected;
	bool new_observation = false;
	std::string observation_id;
	if (event.attempt_observed)
	{
		telemetry_observation observation;
		observation.observation_id = "completion:" + event.transaction.transaction_id;
		observation.quest_definition_id = event.transaction.quest_definition_id;
		observation.content_revision = event.transaction.content_revision;
		observation.observed_at = event.transaction.completed_at;
		observation.pid = event.transaction.direct_completer_pid;
		observation.level = event.level;
		observation.racewar = event.racewar;
		observation.credit_mask = zone_story_quest_tracking::credit_mask_for_pid(
			event.transaction, event.transaction.direct_completer_pid);
		observation.party_size = event.party_context_known ? event.party_size : 0;
		observation.strongest_party_level =
			event.party_context_known ? event.strongest_party_level : 0;
		observation.duration_seconds = event.duration_seconds;
		observation.outcome = event.outcome;
		observation.accessible = true;
		observation_id = observation.observation_id;
		const result observed = record_telemetry(observation, error);
		if (observed == result::conflict || observed == result::invalid)
			return observed;
		new_observation = observed == result::applied;
	}
	const result recorded = apply_transaction(event.transaction, event.character_name,
						  event.racewar, false, true, error);
	if (new_observation && recorded != result::applied && recorded != result::already_applied)
	{
		telemetry_by_definition_[event.transaction.quest_definition_id].erase(
			observation_id);
		telemetry_.erase(observation_id);
		dirty_telemetry_.erase(observation_id);
	}
	return recorded;
}

result service::record_telemetry(const telemetry_observation &observation, std::string *error)
{
	if (observation.observation_id.empty() || observation.quest_definition_id.empty() ||
	    observation.content_revision == 0 || observation.observed_at <= 0 ||
	    observation.pid == 0 || observation.level < 0 || observation.party_size > 1000 ||
	    observation.strongest_party_level < 0 || observation.duration_seconds < 0)
		return fail(error, "invalid quest telemetry observation"), result::invalid;
	const std::string encoded = serialize_observation(observation);
	const auto found = telemetry_.find(observation.observation_id);
	if (found != telemetry_.end())
	{
		if (serialize_observation(found->second) != encoded)
			return fail(error,
				    "telemetry observation ID was reused with different data"),
			       result::conflict;
		return result::already_applied;
	}
	telemetry_.emplace(observation.observation_id, observation);
	telemetry_by_definition_[observation.quest_definition_id].insert(
		observation.observation_id);
	dirty_telemetry_.insert(observation.observation_id);
	return result::applied;
}

bool service::remember_character(uint32_t season_id, uint32_t pid, std::string character_name,
				 bool leaderboard_eligible, int racewar)
{
	if (!season_id || !pid)
		return false;
	const std::pair<uint32_t, uint32_t> key{ season_id, pid };
	if (deleted_characters_.count(key))
		return false;
	const auto *old_state = find_state(season_id, pid);
	if (leaderboard_eligible && old_state && old_state->character_name == character_name &&
	    (racewar == RACEWAR_NONE || old_state->racewar == racewar) &&
	    !leaderboard_exclusions_.count(key))
		return false;
	dirty_characters_.insert(key);
	if (!leaderboard_eligible)
	{
		const bool changed = leaderboard_exclusions_.insert(key).second;
		return changed;
	}
	leaderboard_exclusions_.erase(key);
	auto &state = state_for(season_id, pid);
	state.character_name = std::move(character_name);
	if (racewar != RACEWAR_NONE)
		state.racewar = racewar;

	const std::string normalized_name = lower_name(state.character_name);
	if (normalized_name.empty())
		return true;
	/* Names are unique for current player rows, but durable quest state can outlive
	 * a rename or deleted character.  Once a current PID re-identifies a name,
	 * keep any older PID carrying that same display name out of the public list. */
	for (const auto &[other_key, other_state] : characters_)
	{
		if (other_key == key || other_state.character_name.empty() ||
		    lower_name(other_state.character_name) != normalized_name)
			continue;
		leaderboard_exclusions_.insert(other_key);
		dirty_characters_.insert(other_key);
	}
	return true;
}

bool service::erase_character(uint32_t season_id, uint32_t pid)
{
	if (!season_id || !pid)
		return false;
	const std::pair<uint32_t, uint32_t> key{ season_id, pid };
	all_dirty_ = true;
	characters_.erase(key);
	leaderboard_exclusions_.erase(key);
	deleted_characters_.insert(key);
	for (auto transaction = transactions_.begin(); transaction != transactions_.end();)
	{
		if (contains_pid(transaction->second.transaction, pid))
			transaction = transactions_.erase(transaction);
		else
			++transaction;
	}
	for (auto telemetry = telemetry_.begin(); telemetry != telemetry_.end();)
	{
		if (telemetry->second.pid == pid)
		{
			telemetry_by_definition_[telemetry->second.quest_definition_id].erase(
				telemetry->first);
			telemetry = telemetry_.erase(telemetry);
		}
		else
			++telemetry;
	}
	return true;
}

bool service::erase_character_all_seasons(uint32_t pid, uint32_t current_season_id)
{
	if (!pid || !current_season_id)
		return false;
	all_dirty_ = true;
	std::set<uint32_t> seasons;
	seasons.insert(current_season_id);
	for (const auto &[key, state] : characters_)
	{
		(void)state;
		if (key.second == pid)
			seasons.insert(key.first);
	}
	for (const auto &[season, excluded_pid] : leaderboard_exclusions_)
		if (excluded_pid == pid)
			seasons.insert(season);
	for (const auto &[season, deleted_pid] : deleted_characters_)
		if (deleted_pid == pid)
			seasons.insert(season);
	for (uint32_t season : seasons)
	{
		characters_.erase({ season, pid });
		leaderboard_exclusions_.erase({ season, pid });
		deleted_characters_.insert({ season, pid });
	}
	for (auto transaction = transactions_.begin(); transaction != transactions_.end();)
	{
		if (contains_pid(transaction->second.transaction, pid))
			transaction = transactions_.erase(transaction);
		else
			++transaction;
	}
	for (auto telemetry = telemetry_.begin(); telemetry != telemetry_.end();)
	{
		if (telemetry->second.pid == pid)
		{
			telemetry_by_definition_[telemetry->second.quest_definition_id].erase(
				telemetry->first);
			telemetry = telemetry_.erase(telemetry);
		}
		else
			++telemetry;
	}
	return true;
}

std::set<std::string> service::completed_definition_ids(uint32_t season_id, uint32_t pid,
							int64_t completed_before) const
{
	const auto *state = find_state(season_id, pid);
	std::set<std::string> completed_ids;
	if (!state)
		return completed_ids;
	if (completed_before == no_completion_cutoff)
	{
		for (const auto &[definition_id, credit_mask] : state->credit_masks)
		{
			(void)credit_mask;
			completed_ids.insert(definition_id);
		}
		return completed_ids;
	}
	for (const auto &[definition_id, completed_at] : state->first_completion_times)
		if (completed_at > 0 && completed_at <= completed_before &&
		    state->credit_masks.count(definition_id))
			completed_ids.insert(definition_id);
	return completed_ids;
}

zone_progress service::progress_for_zone_at(uint32_t season_id, uint32_t pid, int32_t zone_number,
					    const std::set<std::string> &completed_ids) const
{
	(void)season_id;
	(void)pid;
	zone_progress progress;
	progress.zone_number = zone_number;
	const auto *registry = find_zone(zone_number);
	if (registry)
		progress.zone_name = registry->name;
	progress.discovered = has_discovered(season_id, pid, zone_number);
	const auto *discovery_state = find_state(season_id, pid);
	if (progress.discovered)
		progress.discovered_at = discovery_state->discoveries.at(zone_number).visited_at;
	for (const auto &definition : catalog_.definitions)
	{
		if (definition.zone_number != zone_number)
			continue;
		if (progress.zone_name.empty() && !definition.zone_name.empty())
			progress.zone_name = definition.zone_name;
		if (!definition.active || !definition.eligible_for_zone_completion ||
		    definition.content_revision != catalog_.content_revision)
			continue;
		progress.total++;
		if (completed_ids.find(definition.definition_id) != completed_ids.end())
			progress.completed++;
	}
	progress.available = progress.total > 0;
	progress.milestone_25 = crossing(progress.completed, progress.total, 25);
	progress.milestone_50 = crossing(progress.completed, progress.total, 50);
	progress.milestone_75 = crossing(progress.completed, progress.total, 75);
	progress.milestone_100 = crossing(progress.completed, progress.total, 100);
	return progress;
}

zone_progress service::progress_for_zone(uint32_t season_id, uint32_t pid,
					 int32_t zone_number) const
{
	return progress_for_zone_at(season_id, pid, zone_number,
				    completed_definition_ids(season_id, pid, no_completion_cutoff));
}

personal_summary service::summary_for_at(uint32_t season_id, uint32_t pid,
					 std::string_view fallback_name,
					 int64_t completed_before) const
{
	personal_summary summary;
	summary.season_id = season_id;
	summary.pid = pid;
	const auto *state = find_state(season_id, pid);
	summary.character_name = state && !state->character_name.empty() ?
					 state->character_name :
					 std::string(fallback_name);
	const std::set<std::string> all_completed_ids =
		completed_definition_ids(season_id, pid, completed_before);
	std::set<std::string> completed_ids;
	for (const auto &definition : catalog_.definitions)
	{
		if (!definition.active || !definition.eligible_for_zone_completion ||
		    definition.content_revision != catalog_.content_revision)
			continue;
		++summary.total;
		if (all_completed_ids.find(definition.definition_id) != all_completed_ids.end())
			completed_ids.insert(definition.definition_id);
	}
	summary.completed = completed_ids.size();
	if (state)
		summary.renown = static_cast<uint32_t>(state->reward_keys.size());
	std::set<int32_t> zones;
	for (const auto &definition : catalog_.definitions)
		if (definition.active && definition.eligible_for_zone_completion &&
		    definition.content_revision == catalog_.content_revision)
			zones.insert(definition.zone_number);
	if (state)
		for (const auto &[zone, discovery] : state->discoveries)
		{
			(void)discovery;
			zones.insert(zone);
			++summary.discovered_zones;
		}
	for (int32_t zone : zones)
	{
		zone_progress zone_state =
			progress_for_zone_at(season_id, pid, zone, all_completed_ids);
		if (zone_state.available && zone_state.completed == zone_state.total)
			++summary.full_zones;
		if (zone_state.completed > 0 || zone_state.discovered)
			summary.zones.push_back(std::move(zone_state));
	}
	std::sort(summary.zones.begin(), summary.zones.end(),
		  [](const auto &left, const auto &right)
		  {
			  if (left.completed != right.completed)
				  return left.completed > right.completed;
			  if (left.zone_name != right.zone_name)
				  return left.zone_name < right.zone_name;
			  return left.zone_number < right.zone_number;
		  });
	return summary;
}

personal_summary service::summary_for(uint32_t season_id, uint32_t pid,
				      std::string_view fallback_name) const
{
	return summary_for_at(season_id, pid, fallback_name, no_completion_cutoff);
}

std::vector<leaderboard_entry> service::sorted_leaderboard(uint32_t season_id, int32_t zone_number,
							   int64_t completed_before) const
{
	(void)zone_number;
	std::vector<leaderboard_entry> entries;
	std::map<std::string, leaderboard_entry> named_entries;
	for (const auto &[key, state] : characters_)
	{
		if (key.first != season_id ||
		    leaderboard_exclusions_.find(key) != leaderboard_exclusions_.end())
			continue;
		const personal_summary summary = summary_for_at(
			season_id, key.second, state.character_name, completed_before);
		if (summary.completed == 0)
			continue;
		leaderboard_entry entry{ .pid = key.second,
					 .character_name = summary.character_name.empty() ?
								   "Unknown adventurer" :
								   summary.character_name };
		entry.racewar = state.racewar;
		entry.completed = summary.completed;
		entry.total = summary.total;
		entry.full_zones = summary.full_zones;
		if (state.character_name.empty())
		{
			entries.push_back(std::move(entry));
			continue;
		}
		const std::string normalized_name = lower_name(state.character_name);
		const auto existing = named_entries.find(normalized_name);
		if (existing == named_entries.end() || better_score(entry, existing->second))
			named_entries[normalized_name] = std::move(entry);
	}
	for (auto &[name, entry] : named_entries)
	{
		(void)name;
		entries.push_back(std::move(entry));
	}
	std::sort(entries.begin(), entries.end(),
		  [](const auto &left, const auto &right) { return better_score(left, right); });
	uint64_t rank = 0;
	for (size_t index = 0; index < entries.size(); ++index)
	{
		if (index == 0 || !same_score(entries[index], entries[index - 1]))
			rank = index + 1;
		entries[index].rank = rank;
	}
	return entries;
}

leaderboard_page service::leaderboard(uint32_t season_id, int32_t zone_number, uint64_t page,
				      uint64_t page_size, uint32_t viewer_pid, int64_t now) const
{
	(void)zone_number;
	leaderboard_page output;
	if (page_size == 0)
		return output;
	const std::vector<leaderboard_entry> entries =
		sorted_leaderboard(season_id, 0, leaderboard_cutoff(now));
	output.total_entries = entries.size();
	for (const auto &entry : entries)
		if (entry.pid == viewer_pid)
			output.own_rank = entry.rank;
	const uint64_t begin = page > std::numeric_limits<uint64_t>::max() / page_size ?
				       std::numeric_limits<uint64_t>::max() :
				       page * page_size;
	if (begin >= entries.size())
		return output;
	const uint64_t end = std::min<uint64_t>(entries.size(), begin + page_size);
	output.entries.insert(output.entries.end(), entries.begin() + begin, entries.begin() + end);
	return output;
}

evidence_summary service::evidence_for(std::string_view quest_definition_id,
				       uint32_t content_revision, int64_t observed_before) const
{
	evidence_summary summary;
	summary.quest_definition_id = quest_definition_id;
	summary.content_revision = content_revision;
	std::set<uint32_t> pids;
	bool all_accessible = true;
	const auto ids = telemetry_by_definition_.find(std::string(quest_definition_id));
	const std::set<std::string> empty;
	for (const auto &id : ids == telemetry_by_definition_.end() ? empty : ids->second)
	{
		const auto found = telemetry_.find(id);
		if (found == telemetry_.end() || found->second.observed_at > observed_before)
			continue;
		const auto &observation = found->second;
		if (observation.quest_definition_id != quest_definition_id ||
		    observation.content_revision != content_revision)
			continue;
		++summary.observed_attempts;
		pids.insert(observation.pid);
		all_accessible = all_accessible && observation.accessible;
		if (observation.party_size == 0 || observation.strongest_party_level == 0)
			++summary.unknown_party_context_attempts;
		else if (daily_policy_.maximum_party_level_delta >= 0 &&
			 observation.strongest_party_level >
				 observation.level + daily_policy_.maximum_party_level_delta)
			++summary.carried_attempts;
		switch (observation.outcome)
		{
		case telemetry_outcome::success:
			++summary.successful_attempts;
			break;
		case telemetry_outcome::failure:
			++summary.failed_attempts;
			break;
		case telemetry_outcome::abandoned:
			++summary.abandoned_attempts;
			break;
		case telemetry_outcome::inaccessible:
			++summary.inaccessible_attempts;
			break;
		case telemetry_outcome::stale_revision:
			++summary.stale_revision_attempts;
			break;
		}
		if (!summary.has_level_range)
		{
			summary.minimum_level = summary.maximum_level = observation.level;
			summary.has_level_range = true;
		}
		else
		{
			summary.minimum_level = std::min(summary.minimum_level, observation.level);
			summary.maximum_level = std::max(summary.maximum_level, observation.level);
		}
		if (!summary.has_racewar_range)
		{
			summary.minimum_racewar = summary.maximum_racewar = observation.racewar;
			summary.has_racewar_range = true;
		}
		else
		{
			summary.minimum_racewar =
				std::min(summary.minimum_racewar, observation.racewar);
			summary.maximum_racewar =
				std::max(summary.maximum_racewar, observation.racewar);
		}
	}
	summary.distinct_pids = pids.size();
	summary.all_observed_accessible = summary.observed_attempts > 0 && all_accessible;
	const auto *definition = find_definition(quest_definition_id);
	if (!definition || !definition->active || definition->content_revision != content_revision)
	{
		summary.explanation = "definition is not active at the observed revision";
		return summary;
	}
	const bool level_ok = summary.has_level_range &&
			      summary.minimum_level >= daily_policy_.minimum_level &&
			      (daily_policy_.maximum_level == 0 ||
			       summary.maximum_level <= daily_policy_.maximum_level);
	const bool racewar_ok = !summary.has_racewar_range ||
				((daily_policy_.minimum_racewar == 0 ||
				  summary.minimum_racewar >= daily_policy_.minimum_racewar) &&
				 (daily_policy_.maximum_racewar == 0 ||
				  summary.maximum_racewar <= daily_policy_.maximum_racewar));
	const bool accessible_ok = !daily_policy_.require_accessible_evidence ||
				   summary.all_observed_accessible;
	const bool party_context_ok = summary.unknown_party_context_attempts == 0 &&
				      summary.carried_attempts == 0 &&
				      (!daily_policy_.require_known_party_context ||
				       summary.unknown_party_context_attempts == 0);
	summary.suitable = summary.observed_attempts >= daily_policy_.minimum_attempts &&
			   summary.distinct_pids >= daily_policy_.minimum_distinct_pids &&
			   summary.successful_attempts >= daily_policy_.minimum_successes &&
			   level_ok && racewar_ok && accessible_ok && party_context_ok &&
			   summary.inaccessible_attempts == 0 &&
			   summary.stale_revision_attempts == 0;
	if (summary.suitable)
		summary.explanation =
			"evidence meets the configured attempts, player, level, and access policy";
	else
	{
		std::ostringstream reason;
		reason << "observed " << summary.observed_attempts << " attempts from "
		       << summary.distinct_pids << " PIDs; policy requires "
		       << daily_policy_.minimum_attempts << "/"
		       << daily_policy_.minimum_distinct_pids;
		if (!accessible_ok)
			reason << "; inaccessible observations are present";
		if (!party_context_ok)
			reason << "; party context is missing or shows a stronger-party carry";
		if (!level_ok)
			reason << "; level range is outside policy";
		summary.explanation = reason.str();
	}
	return summary;
}

int64_t service::period_for(int64_t timestamp, int64_t period_seconds)
{
	if (timestamp < 0 || !valid_period(period_seconds))
		return 0;
	return timestamp / period_seconds;
}

const zone_story_quest_catalog::zone_definition *service::find_zone(int32_t number) const
{
	for (const auto &zone : catalog_.zones)
		if (zone.zone_number == number)
			return &zone;
	return nullptr;
}

bool service::has_discovered(uint32_t season, uint32_t pid, int32_t zone, int64_t before) const
{
	const auto *state = find_state(season, pid);
	if (!state)
		return false;
	const auto found = state->discoveries.find(zone);
	return found != state->discoveries.end() && found->second.visited_at <= before;
}

result service::discover_zone(uint32_t season, uint32_t pid, int32_t number, int32_t room,
			      int64_t visited_at, std::string_view source, std::string *error)
{
	const auto *zone = find_zone(number);
	if (!season || !pid || visited_at <= 0 || !zone || !zone->discoverable ||
	    room < zone->first_vnum || room > zone->last_vnum ||
	    (source != "arrival" && source != "completion-backfill") ||
	    deleted_characters_.count({ season, pid }) ||
	    leaderboard_exclusions_.count({ season, pid }))
		return fail(error, "invalid zone discovery evidence"), result::rejected;
	if (has_discovered(season, pid, number))
		return result::already_applied;
	dirty_characters_.insert({ season, pid });
	state_for(season, pid)
		.discoveries.emplace(number, character_state::discovery{ room, visited_at,
									 std::string(source) });
	return result::applied;
}

int32_t service::resolve_zone(std::string_view name, std::string *error) const
{
	const auto query = lower_name(name);
	std::vector<int32_t> exact, partial;
	for (const auto &zone : catalog_.zones)
	{
		if (!zone.discoverable || query.empty())
			continue;
		const auto candidate = lower_name(zone.name);
		if (candidate == query)
			exact.push_back(zone.zone_number);
		else if (candidate.find(query) != std::string::npos)
			partial.push_back(zone.zone_number);
	}
	const auto &matches = exact.empty() ? partial : exact;
	if (matches.size() == 1)
		return matches.front();
	fail(error, matches.empty() ? "No area matches that name." :
				      "That area name is ambiguous; use its full name.");
	return -1;
}

bool service::daily_eligible_for(uint32_t season, uint32_t pid, std::string_view id, int level,
				 int racewar, int strongest, int64_t now) const
{
	const auto *definition = find_definition(id);
	if (!daily_policy_.enabled || !season || !pid || now <= 0 || now < checklist_starts_at_ ||
	    !definition || !definition->active || !definition->repeatable ||
	    !definition->daily_eligible ||
	    definition->content_revision != catalog_.content_revision ||
	    !has_discovered(season, pid, definition->zone_number, now) ||
	    deleted_characters_.count({ season, pid }) ||
	    leaderboard_exclusions_.count({ season, pid }) || level < daily_policy_.minimum_level ||
	    strongest <= 0 || strongest > level + daily_policy_.maximum_party_level_delta ||
	    (daily_policy_.maximum_level && level > daily_policy_.maximum_level))
		return false;
	const auto evidence = evidence_for(id, definition->content_revision, now);
	if (!evidence.suitable || level < evidence.minimum_level || level > evidence.maximum_level)
		return false;
	bool faction_observed = false;
	const auto ids = telemetry_by_definition_.find(std::string(id));
	const std::set<std::string> empty;
	for (const auto &key : ids == telemetry_by_definition_.end() ? empty : ids->second)
	{
		const auto found = telemetry_.find(key);
		if (found == telemetry_.end())
			continue;
		const auto &observation = found->second;
		if (observation.quest_definition_id == id &&
		    observation.content_revision == definition->content_revision &&
		    observation.racewar == racewar &&
		    observation.outcome == telemetry_outcome::success && observation.accessible &&
		    observation.observed_at <= now)
			faction_observed = true;
	}
	if (!faction_observed)
		return false;
	const auto *state = find_state(season, pid);
	for (const auto &prerequisite : definition->prerequisites)
		if (!state || !state->credit_masks.count(prerequisite))
			return false;
	return true;
}

bool service::existing_transaction(
	std::string_view id, zone_story_quest_tracking::completion_transaction *transaction) const
{
	const auto found = transactions_.find(std::string(id));
	if (found == transactions_.end())
		return false;
	if (transaction)
		*transaction = found->second.transaction;
	return true;
}

void service::project_daily(const zone_story_quest_tracking::completion_transaction &tx,
			    bool award_bonus)
{
	if (tx.daily_policy_revision != 1 || tx.completed_at < checklist_starts_at_)
		return;
	const int64_t period = period_for(tx.completed_at);
	for (uint32_t pid : tx.daily_credited_pids)
	{
		if (deleted_characters_.count({ tx.season_id, pid }))
			continue;
		auto &state = state_for(tx.season_id, pid);
		state.daily_completions[period].insert(tx.quest_definition_id);
		if (award_bonus)
			state.reward_keys.emplace(std::to_string(tx.season_id) + ":" +
							  std::to_string(pid) + ":" +
							  std::to_string(period),
						  tx.transaction_id);
	}
}

std::string service::render_journal(uint32_t season, uint32_t pid, int32_t number, int level,
				    int racewar, int64_t now, bool daily_only, bool colors) const
{
	const auto *zone = find_zone(number);
	if (!zone || !zone->discoverable)
		return "No playable area matches that name.\r\n";
	std::ostringstream out;
	out << "\r\n"
	    << color(colors, "&+L") << zone->name
	    << (daily_only ? " daily quests" : " quest journal") << color(colors, "&n") << "\r\n";
	if (!has_discovered(season, pid, number))
		return out.str() +
		       "  Undiscovered: visit this area to unlock its journal and daily quests.\r\n";
	if (daily_only && !daily_policy_.enabled)
		return out.str() + "  Daily quests are currently disabled.\r\n";
	if (daily_only)
		out << "  Resets at 00:00 UTC in "
		    << display_remaining((period_for(now) + 1) * 86400 - now)
		    << ". First qualifying completion earns 1 renown today.\r\n";
	const auto *state = find_state(season, pid);
	const int64_t period = period_for(now);
	size_t count = 0;
	for (const auto &definition : catalog_.definitions)
	{
		if (definition.zone_number != number || !definition.active)
			continue;
		if (daily_only && !definition.daily_eligible)
			continue;
		++count;
		bool daily_done =
			state && state->daily_completions.count(period) &&
			state->daily_completions.at(period).count(definition.definition_id);
		bool story_done = state && state->credit_masks.count(definition.definition_id);
		std::string status =
			daily_only ? (daily_done ? "Done today" :
				      daily_eligible_for(season, pid, definition.definition_id,
							 level, racewar, level, now) ?
						   "Available" :
						   "Locked") :
				     (story_done ? "Story complete" : "Story incomplete");
		if (!daily_only && definition.daily_eligible && daily_policy_.enabled)
			status += daily_done ? "; Done today" :
				  daily_eligible_for(season, pid, definition.definition_id, level,
						     racewar, level, now) ?
					       "; Available today" :
					       "; Locked today";
		out << "  [" << status << "] " << display_quest_name(&definition) << "\r\n";
		if (!definition.objective.empty())
			out << "    " << definition.objective << "\r\n";
		if (!daily_only && !definition.daily_eligible)
			out << "    Story only: " << definition.daily_exclusion << "\r\n";
		if ((daily_only && status == "Locked") ||
		    status.find("Locked today") != std::string::npos)
			out << "    Not currently available. Complete earlier requests or return after more progress.\r\n";
	}
	if (!count)
		out << "  This area has no " << (daily_only ? "daily-suitable" : "tracked")
		    << " quests.\r\n";
	return out.str();
}

result service::complete_daily(uint32_t season, uint32_t pid, std::string_view id, int64_t now,
			       std::string *error)
{
	(void)now;
	const auto *state = find_state(season, pid);
	if (state)
		for (const auto &[key, transaction] : state->reward_keys)
		{
			(void)key;
			if (transaction == id)
				return result::already_applied;
		}
	return fail(error, "daily completion must be frozen in the authoritative turn-in receipt"),
	       result::rejected;
}

daily_assignment service::daily_for(uint32_t season_id, uint32_t pid, int64_t period) const
{
	const auto *state = find_state(season_id, pid);
	if (!state)
		return {};
	const auto assignment = state->daily_assignments.find(period);
	return assignment == state->daily_assignments.end() ? daily_assignment{} :
							      assignment->second;
}

std::string service::render_zone(uint32_t season_id, uint32_t pid, int32_t zone_number,
				 std::string_view fallback_name, bool colors) const
{
	const zone_progress progress = progress_for_zone(season_id, pid, zone_number);
	const personal_summary summary = summary_for(season_id, pid, fallback_name);
	std::ostringstream output;
	output << "\r\n"
	       << color(colors, "&+L") << display_zone_name(progress) << " completion for "
	       << display_character_name(summary) << color(colors, "&n") << "\r\n";
	output << "  Discovery achievement: "
	       << (progress.discovered ? "Discovered" : "Undiscovered") << "\r\n";
	if (find_zone(zone_number) && !progress.discovered)
		return output.str() + "  Visit this area to unlock its private quest progress.\r\n";
	if (daily_policy_.enabled)
	{
		const auto *state = find_state(season_id, pid);
		const auto period = period_for(static_cast<int64_t>(std::time(nullptr)));
		size_t done = 0;
		if (state && state->daily_completions.count(period))
			for (const auto &id : state->daily_completions.at(period))
			{
				const auto *definition = find_definition(id);
				if (definition && definition->zone_number == zone_number)
					++done;
			}
		output << "  Daily quests completed today: " << done << "\r\n";
	}
	if (!progress.available)
	{
		output << "  N/A: this zone has no active zone-story quests in the current catalog.\r\n";
		return output.str();
	}
	output << "  Completed: " << display_count(progress.completed) << " unique quests ("
	       << display_percentage(progress.completed, progress.total) << ")\r\n";
	const char *bar_color = progress.milestone_100 ? "&+G" :
				progress.milestone_75  ? "&+g" :
				progress.milestone_50  ? "&+y" :
							 "&+w";
	output << "  " << color(colors, bar_color) << "[";
	const uint64_t filled = std::min<uint64_t>(20, progress.completed * 20 / progress.total);
	for (uint64_t index = 0; index < 20; ++index)
		output << (index < filled ? '#' : '-');
	output << "]" << color(colors, "&n") << "\r\n";
	output << "  Milestones: " << (progress.milestone_25 ? "25 " : "")
	       << (progress.milestone_50 ? "50 " : "") << (progress.milestone_75 ? "75 " : "")
	       << (progress.milestone_100 ? "100" : "")
	       << (progress.milestone_25 || progress.milestone_50 || progress.milestone_75 ||
				   progress.milestone_100 ?
			   "% reached" :
			   "none")
	       << "\r\n";
	return output.str();
}

std::string service::render_summary(uint32_t season_id, uint32_t pid,
				    std::string_view fallback_name, bool colors) const
{
	const personal_summary summary = summary_for(season_id, pid, fallback_name);
	std::ostringstream output;
	output << "\r\n"
	       << color(colors, "&+L") << "Zone-story achievements for "
	       << display_character_name(summary) << color(colors, "&n") << "\r\n";
	output << "  Discovered zones: " << summary.discovered_zones << "\r\n";
	if (summary.total == 0)
	{
		output << "  N/A: the current production catalog contains no eligible quests.\r\n";
	}
	else
		output << "  Overall: " << display_count(summary.completed) << " unique quests ("
		       << display_percentage(summary.completed, summary.total) << ")\r\n";
	output << "  Fully completed zones: " << summary.full_zones << "\r\n";
	if (daily_policy_.enabled && summary.renown > 0)
		output << "  Daily renown: " << summary.renown << "\r\n";
	for (const auto &zone : summary.zones)
		output << "  " << display_zone_name(zone) << ": " << display_count(zone.completed)
		       << " unique quests (" << display_percentage(zone.completed, zone.total)
		       << ")" << (zone.milestone_100 ? " [100%]" : "") << "\r\n";
	return output.str();
}

std::string service::render_leaderboard(uint32_t season_id, int32_t zone_number, uint64_t page,
					uint64_t page_size, uint32_t viewer_pid, bool colors,
					int64_t now) const
{
	(void)zone_number;
	const int64_t completed_before = leaderboard_cutoff(now);
	const leaderboard_page board = leaderboard(season_id, 0, page, page_size, viewer_pid, now);
	std::ostringstream output;
	output << "\r\n"
	       << color(colors, "&+L") << "Worldwide quest completion leaderboard"
	       << color(colors, "&n") << "\r\n"
	       << color(colors, "&+L")
	       << "  Quest totals and percentages may lag actual completions by up to 12 hours."
	       << color(colors, "&n") << "\r\n";
	if (!board.total_entries)
	{
		output << "  No players with a completed quest are ranked for this season.\r\n";
		if (viewer_pid)
		{
			const personal_summary viewer =
				summary_for_at(season_id, viewer_pid, {}, completed_before);
			output << "  You have " << display_count(viewer.completed)
			       << " unique quests ("
			       << display_percentage(viewer.completed, viewer.total)
			       << ") and are unranked.\r\n";
		}
		return output.str();
	}
	for (const auto &entry : board.entries)
	{
		const bool viewer = entry.pid == viewer_pid;
		const char *name_color = racewar_name_color(entry.racewar);
		output << "  " << color(colors, viewer ? "&+Y" : "") << (viewer ? "* " : "  ")
		       << color(colors, viewer ? "&+Y" : "") << "#" << entry.rank
		       << color(colors, "&n") << " " << color(colors, name_color)
		       << entry.character_name << color(colors, "&n") << " " << color(colors, "&+C")
		       << display_count(entry.completed) << " unique quests" << color(colors, "&n")
		       << " " << color(colors, "&+W") << "("
		       << display_percentage(entry.completed, entry.total) << ")"
		       << color(colors, "&n") << "\r\n";
	}
	output << "  Page " << (page + 1) << ", " << display_count(board.total_entries)
	       << " ranked players";
	if (board.own_rank)
		output << "; your rank: #" << board.own_rank;
	else if (viewer_pid)
	{
		const personal_summary viewer =
			summary_for_at(season_id, viewer_pid, {}, completed_before);
		output << "; you are unranked (" << display_count(viewer.completed)
		       << " unique quests)";
	}
	output << "\r\n";
	return output.str();
}

std::string service::render_daily(uint32_t season, uint32_t pid, int level, int racewar,
				  int64_t now, bool colors) const
{
	if (!daily_policy_.enabled)
		return {};
	std::ostringstream out;
	out << "\r\n"
	    << color(colors, "&+L") << "Daily Quests" << color(colors, "&n") << "\r\n"
	    << "  All suitable quests in discovered areas can be completed once today.\r\n"
	    << "  First daily completion earns 1 renown; original quest rewards are unchanged.\r\n"
	    << "  Resets at 00:00 UTC in " << display_remaining((period_for(now) + 1) * 86400 - now)
	    << ".\r\n";
	const auto *state = find_state(season, pid);
	const auto period = period_for(now);
	size_t completed = state && state->daily_completions.count(period) ?
				   state->daily_completions.at(period).size() :
				   0;
	out << "  Completed today: " << completed << "; renown: " << summary_for(season, pid).renown
	    << "\r\n";
	if (now < checklist_starts_at_)
		out << "  The new checklist begins at the next UTC reset.\r\n";
	bool any = false;
	for (const auto &zone : catalog_.zones)
	{
		if (!zone.discoverable || !has_discovered(season, pid, zone.zone_number))
			continue;
		any = true;
		size_t available = 0, done = 0;
		for (const auto &definition : catalog_.definitions)
		{
			if (definition.zone_number != zone.zone_number ||
			    !definition.daily_eligible)
				continue;
			if (state && state->daily_completions.count(period) &&
			    state->daily_completions.at(period).count(definition.definition_id))
				++done;
			else if (daily_eligible_for(season, pid, definition.definition_id, level,
						    racewar, level, now))
				++available;
		}
		out << "  " << zone.name << ": " << done << " done today, " << available
		    << " available\r\n";
	}
	if (!any)
		out << "  Explore an area to discover its daily quests.\r\n";
	out << "  Use 'quest daily <area>' for its checklist.\r\n";
	return out.str();
}

std::string service::render_daily_score(uint32_t season, uint32_t pid, int level, int racewar,
					int64_t now, bool colors) const
{
	(void)level;
	(void)racewar;
	if (!daily_policy_.enabled)
		return {};
	const auto *state = find_state(season, pid);
	const auto period = period_for(now);
	const size_t done = state && state->daily_completions.count(period) ?
				    state->daily_completions.at(period).size() :
				    0;
	return "\r\n" + std::string(color(colors, "&+L")) + "Daily: " + color(colors, "&n") +
	       std::to_string(done) + " completed today; renown " +
	       std::to_string(summary_for(season, pid).renown) +
	       " - type 'quest daily' for details\r\n";
}

zone_story_quest_state::changes service::changes_for_persistence() const
{
	zone_story_quest_state::changes updates;
	updates.replace = all_dirty_;
	if (all_dirty_)
	{
		updates.values = zone_story_quest_state::split_document(serialize_state());
		return updates;
	}
	for (const auto &key : dirty_characters_)
	{
		service subset;
		const auto found = characters_.find(key);
		if (found != characters_.end())
			subset.characters_.emplace(key, found->second);
		if (deleted_characters_.count(key))
			subset.deleted_characters_.insert(key);
		if (leaderboard_exclusions_.count(key))
			subset.leaderboard_exclusions_.insert(key);
		auto values = zone_story_quest_state::split_document(subset.serialize_state());
		values.erase("meta");
		updates.values.insert(values.begin(), values.end());
	}
	for (const auto &id : dirty_transactions_)
	{
		const auto found = transactions_.find(id);
		updates.values["T:" + hex_encode(id)] =
			found == transactions_.end() ?
				std::string() :
				"T|" + hex_encode(id) + "|" + hex_encode(found->second.encoded) +
					"\n";
	}
	for (const auto &id : dirty_telemetry_)
	{
		const auto found = telemetry_.find(id);
		updates.values["E:" + hex_encode(id)] =
			found == telemetry_.end() ?
				std::string() :
				"E|" + hex_encode(id) + "|" +
					hex_encode(serialize_observation(found->second)) + "\n";
	}
	return updates;
}

void service::mark_persisted()
{
	all_dirty_ = false;
	dirty_characters_.clear();
	dirty_transactions_.clear();
	dirty_telemetry_.clear();
}

std::function<void()> service::checkpoint_for(uint32_t season, const std::vector<uint32_t> &pids,
					      std::string_view txid)
{
	std::map<std::pair<uint32_t, uint32_t>, std::optional<character_state>> states;
	for (uint32_t pid : pids)
	{
		const auto *state = find_state(season, pid);
		states[{ season, pid }] = state ? std::optional<character_state>(*state) :
						  std::nullopt;
	}
	const std::string id(txid), observation_id = "completion:" + id;
	const auto transaction = transactions_.find(id);
	const std::optional<stored_transaction> old_transaction =
		transaction == transactions_.end() ?
			std::nullopt :
			std::optional<stored_transaction>(transaction->second);
	const auto observation = telemetry_.find(observation_id);
	const std::optional<telemetry_observation> old_observation =
		observation == telemetry_.end() ?
			std::nullopt :
			std::optional<telemetry_observation>(observation->second);
	return [this, states = std::move(states), id, observation_id, old_transaction,
		old_observation, exclusions = leaderboard_exclusions_,
		deleted = deleted_characters_, all_dirty = all_dirty_,
		dirty_characters = dirty_characters_, dirty_transactions = dirty_transactions_,
		dirty_telemetry = dirty_telemetry_]()
	{
		for (const auto &[key, state] : states)
			if (state)
				characters_[key] = *state;
			else
				characters_.erase(key);
		if (!id.empty())
		{
			if (old_transaction)
				transactions_[id] = *old_transaction;
			else
				transactions_.erase(id);
			const auto current = telemetry_.find(observation_id);
			if (current != telemetry_.end())
				telemetry_by_definition_[current->second.quest_definition_id].erase(
					observation_id);
			if (old_observation)
			{
				telemetry_[observation_id] = *old_observation;
				telemetry_by_definition_[old_observation->quest_definition_id]
					.insert(observation_id);
			}
			else
				telemetry_.erase(observation_id);
		}
		leaderboard_exclusions_ = exclusions;
		deleted_characters_ = deleted;
		all_dirty_ = all_dirty;
		dirty_characters_ = dirty_characters;
		dirty_transactions_ = dirty_transactions;
		dirty_telemetry_ = dirty_telemetry;
	};
}

std::string service::serialize_state(std::string *error) const
{
	(void)error;
	std::ostringstream output;
	output << "ZSQF|2\nK|" << checklist_starts_at_ << "\n";
	for (const auto &[id, stored] : transactions_)
	{
		bool includes_deleted_pid = false;
		for (const uint32_t pid : stored.transaction.credited_pids)
			if (deleted_characters_.find({ stored.transaction.season_id, pid }) !=
			    deleted_characters_.end())
			{
				includes_deleted_pid = true;
				break;
			}
		if (!includes_deleted_pid)
			output << "T|" << hex_encode(id) << "|" << hex_encode(stored.encoded)
			       << "\n";
	}
	for (const auto &[key, state] : characters_)
	{
		if (deleted_characters_.find(key) != deleted_characters_.end())
			continue;
		if (!state.character_name.empty())
			output << "N|" << state.season_id << "|" << state.pid << "|"
			       << hex_encode(state.character_name) << "|" << state.racewar << "\n";
		for (const auto &[definition_id, credit_mask] : state.credit_masks)
			output << "C|" << state.season_id << "|" << state.pid << "|"
			       << hex_encode(definition_id) << "|" << credit_mask << "\n";
		for (const auto &[id, completed_at] : state.first_completion_times)
			output << "F|" << state.season_id << "|" << state.pid << "|"
			       << hex_encode(id) << "|" << completed_at << "\n";
		for (const auto &[period, definitions] : state.daily_completions)
			for (const auto &id : definitions)
				output << "P|" << state.season_id << "|" << state.pid << "|"
				       << period << "|" << hex_encode(id) << "\n";
		for (const auto &[zone, discovery] : state.discoveries)
			output << "V|" << state.season_id << "|" << state.pid << "|" << zone << "|"
			       << discovery.room_vnum << "|" << discovery.visited_at << "|"
			       << discovery.source << "\n";
		for (const auto &[period, assignment] : state.daily_assignments)
			output << "D|" << state.season_id << "|" << state.pid << "|" << period
			       << "|" << hex_encode(assignment.quest_definition_id) << "|"
			       << assignment.content_revision << "|" << assignment.assigned_at
			       << "|" << assignment.expires_at << "|"
			       << status_name(assignment.status) << "|" << assignment.reward_amount
			       << "|" << hex_encode(assignment.completion_transaction_id) << "\n";
		for (const auto &[reward_key, transaction_id] : state.reward_keys)
			output << "R|" << state.season_id << "|" << state.pid << "|"
			       << hex_encode(reward_key) << "|" << hex_encode(transaction_id)
			       << "\n";
	}
	for (const auto &[season, pid] : deleted_characters_)
		output << "X|" << season << "|" << pid << "\n";
	for (const auto &[season, pid] : leaderboard_exclusions_)
	{
		if (deleted_characters_.find({ season, pid }) == deleted_characters_.end())
			output << "H|" << season << "|" << pid << "\n";
	}
	for (const auto &[id, observation] : telemetry_)
	{
		bool deleted_pid = false;
		for (const auto &[season, pid] : deleted_characters_)
			if (pid == observation.pid)
			{
				deleted_pid = true;
				break;
			}
		if (!deleted_pid)
			output << "E|" << hex_encode(id) << "|"
			       << hex_encode(serialize_observation(observation)) << "\n";
	}
	return output.str();
}

bool service::deserialize_state(std::string_view encoded, std::string *error)
{
	std::vector<zone_story_quest_tracking::completion_transaction> transactions;
	std::vector<std::tuple<uint32_t, uint32_t, std::string, int>> names;
	std::vector<std::tuple<uint32_t, uint32_t, std::string, uint32_t>> credits;
	std::vector<daily_assignment> assignments;
	std::vector<std::tuple<uint32_t, uint32_t, std::string, std::string>> rewards;
	std::vector<telemetry_observation> observations;
	std::vector<std::pair<uint32_t, uint32_t>> deleted;
	std::vector<std::pair<uint32_t, uint32_t>> leaderboard_exclusions;
	std::vector<std::tuple<uint32_t, uint32_t, int32_t, int32_t, int64_t, std::string>>
		discoveries;
	std::vector<std::tuple<uint32_t, uint32_t, int64_t, std::string>> daily_projections;
	std::vector<std::tuple<uint32_t, uint32_t, std::string, int64_t>> first_completions;
	int version = 0;
	int64_t cutover = 0;
	bool header_seen = false;
	bool cutover_seen = false;
	size_t begin = 0;
	while (begin < encoded.size())
	{
		const size_t end = encoded.find('\n', begin);
		const std::string_view line = encoded.substr(begin, end == std::string_view::npos ?
									    encoded.size() - begin :
									    end - begin);
		begin = end == std::string_view::npos ? encoded.size() : end + 1;
		if (line.empty())
			continue;
		const auto fields = split(line);
		if (fields.size() == 2 && fields[0] == "ZSQF" &&
		    (fields[1] == "1" || fields[1] == "2") && !header_seen)
		{
			version = fields[1] == "1" ? 1 : 2;
			header_seen = true;
			continue;
		}
		if (!header_seen)
			return fail(error, "zone-story state header is missing");
		if (fields[0] == "K" && fields.size() == 2 && version == 2)
		{
			if (cutover_seen || !parse_integer(fields[1], &cutover) || cutover < 0)
				return fail(error, "invalid checklist cutover");
			cutover_seen = true;
		}
		else if (fields[0] == "F" && fields.size() == 5 && version == 2)
		{
			uint32_t season = 0, pid = 0;
			int64_t completed_at = 0;
			std::string id;
			if (!parse_integer(fields[1], &season) || !parse_integer(fields[2], &pid) ||
			    !hex_decode(fields[3], &id) ||
			    !parse_integer(fields[4], &completed_at) || !season || !pid ||
			    id.empty() || completed_at <= 0)
				return fail(error, "invalid first completion time");
			first_completions.emplace_back(season, pid, id, completed_at);
		}
		else if (fields[0] == "P" && fields.size() == 5 && version == 2)
		{
			uint32_t season = 0, pid = 0;
			int64_t period = 0;
			std::string id;
			if (!parse_integer(fields[1], &season) || !parse_integer(fields[2], &pid) ||
			    !parse_integer(fields[3], &period) || !hex_decode(fields[4], &id) ||
			    !season || !pid || period < 0 || id.empty())
				return fail(error, "invalid daily projection");
			daily_projections.emplace_back(season, pid, period, id);
		}
		else if (fields[0] == "V" && fields.size() == 7 && version == 2)
		{
			uint32_t season = 0, pid = 0;
			int32_t zone = 0, room = 0;
			int64_t at = 0;
			if (!parse_integer(fields[1], &season) || !parse_integer(fields[2], &pid) ||
			    !parse_integer(fields[3], &zone) || !parse_integer(fields[4], &room) ||
			    !parse_integer(fields[5], &at) || !season || !pid || zone <= 0 ||
			    room <= 0 || at <= 0 ||
			    (fields[6] != "arrival" && fields[6] != "completion-backfill"))
				return fail(error, "invalid discovery record");
			discoveries.emplace_back(season, pid, zone, room, at,
						 std::string(fields[6]));
		}
		else if (fields[0] == "T" && fields.size() == 3)
		{
			std::string transaction_encoded;
			if (!hex_decode(fields[2], &transaction_encoded))
				return fail(error, "invalid serialized completion transaction");
			zone_story_quest_tracking::completion_transaction transaction;
			if (!zone_story_quest_tracking::deserialize_transaction(
				    transaction_encoded, &transaction, error))
				return false;
			std::string stored_id;
			if (!hex_decode(fields[1], &stored_id) ||
			    stored_id != transaction.transaction_id)
				return fail(error, "completion record ID differs from its payload");
			transactions.push_back(std::move(transaction));
		}
		else if (fields[0] == "N" && (fields.size() == 4 || fields.size() == 5))
		{
			uint32_t season = 0, pid = 0;
			int racewar = RACEWAR_NONE;
			std::string name;
			if (!parse_integer(fields[1], &season) || !parse_integer(fields[2], &pid) ||
			    !hex_decode(fields[3], &name) ||
			    (fields.size() == 5 && !parse_integer(fields[4], &racewar)) ||
			    !season || !pid)
				return fail(error, "invalid character name record");
			names.emplace_back(season, pid, std::move(name), racewar);
		}
		else if (fields[0] == "C" && fields.size() == 5)
		{
			uint32_t season = 0, pid = 0, credit_mask = 0;
			std::string definition_id;
			if (!parse_integer(fields[1], &season) || !parse_integer(fields[2], &pid) ||
			    !hex_decode(fields[3], &definition_id) ||
			    !parse_integer(fields[4], &credit_mask) || !season || !pid ||
			    definition_id.empty() || !credit_mask)
				return fail(error, "invalid character credit record");
			credits.emplace_back(season, pid, std::move(definition_id), credit_mask);
		}
		else if (fields[0] == "D" && fields.size() == 11)
		{
			daily_assignment assignment;
			if (!parse_integer(fields[1], &assignment.season_id) ||
			    !parse_integer(fields[2], &assignment.pid) ||
			    !parse_integer(fields[3], &assignment.period) ||
			    !hex_decode(fields[4], &assignment.quest_definition_id) ||
			    !parse_integer(fields[5], &assignment.content_revision) ||
			    !parse_integer(fields[6], &assignment.assigned_at) ||
			    !parse_integer(fields[7], &assignment.expires_at) ||
			    !parse_status(fields[8], &assignment.status) ||
			    !parse_integer(fields[9], &assignment.reward_amount) ||
			    !hex_decode(fields[10], &assignment.completion_transaction_id) ||
			    !assignment.season_id || !assignment.pid)
				return fail(error, "invalid daily assignment record");
			assignments.push_back(std::move(assignment));
		}
		else if (fields[0] == "R" && fields.size() == 5)
		{
			uint32_t season = 0, pid = 0;
			std::string reward_key, transaction_id;
			if (!parse_integer(fields[1], &season) || !parse_integer(fields[2], &pid) ||
			    !hex_decode(fields[3], &reward_key) ||
			    !hex_decode(fields[4], &transaction_id) || !season || !pid)
				return fail(error, "invalid daily reward record");
			rewards.emplace_back(season, pid, std::move(reward_key),
					     std::move(transaction_id));
		}
		else if (fields[0] == "E" && fields.size() == 3)
		{
			std::string observation_encoded;
			if (!hex_decode(fields[2], &observation_encoded))
				return fail(error, "invalid serialized telemetry observation");
			telemetry_observation observation;
			if (!deserialize_observation(observation_encoded, &observation))
				return fail(error, "invalid telemetry observation");
			observations.push_back(std::move(observation));
		}
		else if (fields[0] == "X" && fields.size() == 3)
		{
			uint32_t season = 0, pid = 0;
			if (!parse_integer(fields[1], &season) || !parse_integer(fields[2], &pid) ||
			    !season || !pid)
				return fail(error, "invalid deleted character record");
			deleted.emplace_back(season, pid);
		}
		else if (fields[0] == "H" && fields.size() == 3)
		{
			uint32_t season = 0, pid = 0;
			if (!parse_integer(fields[1], &season) || !parse_integer(fields[2], &pid) ||
			    !season || !pid)
				return fail(error, "invalid leaderboard exclusion record");
			leaderboard_exclusions.emplace_back(season, pid);
		}
		else
			return fail(error, "unknown or malformed zone-story state record");
	}
	if (!header_seen)
		return fail(error, "zone-story state header is missing");
	if (version == 2 && !cutover_seen)
		return fail(error, "checklist cutover is missing");
	service candidate(catalog_);
	candidate.set_daily_policy(daily_policy_);
	candidate.checklist_starts_at_ = cutover;
	candidate.deleted_characters_.clear();
	candidate.leaderboard_exclusions_.clear();
	candidate.transactions_.clear();
	candidate.telemetry_.clear();
	for (const auto &[season, pid] : deleted)
		candidate.deleted_characters_.insert({ season, pid });
	for (const auto &[season, pid] : leaderboard_exclusions)
		if (candidate.deleted_characters_.find({ season, pid }) ==
		    candidate.deleted_characters_.end())
			candidate.leaderboard_exclusions_.insert({ season, pid });
	for (const auto &[season, pid, name, racewar] : names)
		if (candidate.deleted_characters_.find({ season, pid }) ==
		    candidate.deleted_characters_.end())
		{
			auto &state = candidate.state_for(season, pid);
			state.character_name = name;
			state.racewar = racewar;
		}
	for (const auto &[season, pid, definition_id, credit_mask] : credits)
		if (candidate.deleted_characters_.find({ season, pid }) ==
		    candidate.deleted_characters_.end())
			candidate.state_for(season, pid).credit_masks[definition_id] = credit_mask;
	for (const auto &[season, pid, id, completed_at] : first_completions)
		if (!candidate.deleted_characters_.count({ season, pid }))
		{
			auto &times = candidate.state_for(season, pid).first_completion_times;
			const auto old = times.find(id);
			if (old == times.end() || completed_at < old->second)
				times[id] = completed_at;
		}
	for (const auto &assignment : assignments)
		if (candidate.deleted_characters_.find({ assignment.season_id, assignment.pid }) ==
		    candidate.deleted_characters_.end())
			candidate.state_for(assignment.season_id, assignment.pid)
				.daily_assignments[assignment.period] = assignment;
	for (const auto &[season, pid, reward_key, transaction_id] : rewards)
		if (candidate.deleted_characters_.find({ season, pid }) ==
		    candidate.deleted_characters_.end())
			candidate.state_for(season, pid).reward_keys[reward_key] = transaction_id;
	for (const auto &observation : observations)
	{
		bool deleted_pid = false;
		for (const auto &[season, pid] : candidate.deleted_characters_)
			if (pid == observation.pid)
			{
				deleted_pid = true;
				break;
			}
		if (!deleted_pid)
		{
			candidate.telemetry_[observation.observation_id] = observation;
			candidate.telemetry_by_definition_[observation.quest_definition_id].insert(
				observation.observation_id);
		}
	}
	for (const auto &transaction : transactions)
	{
		const result applied = candidate.apply_transaction(transaction, {}, RACEWAR_NONE,
								   true, false, error);
		if (applied != result::applied && applied != result::already_applied)
			return false;
	}
	if (version == 1)
	{
		candidate.checklist_starts_at_ =
			(period_for(static_cast<int64_t>(std::time(nullptr))) + 1) * 86400;
		for (const auto &tx : transactions)
			for (const auto &zone : catalog_.zones)
				if (zone.discoverable && tx.room_vnum >= zone.first_vnum &&
				    tx.room_vnum <= zone.last_vnum)
					for (uint32_t pid : tx.credited_pids)
						candidate.discover_zone(tx.season_id, pid,
									zone.zone_number,
									tx.room_vnum,
									tx.completed_at,
									"completion-backfill");
	}
	for (const auto &[season, pid, zone, room, at, source] : discoveries)
		if (!candidate.deleted_characters_.count({ season, pid }))
			candidate.state_for(season, pid)
				.discoveries.emplace(zone, character_state::discovery{ room, at,
										       source });
	for (const auto &[season, pid, period, id] : daily_projections)
		if (!candidate.deleted_characters_.count({ season, pid }))
			candidate.state_for(season, pid).daily_completions[period].insert(id);
	*this = std::move(candidate);
	return true;
}
} // namespace zone_story_quest_feature
