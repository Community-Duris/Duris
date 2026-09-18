#include "world/zone_story_quest_feature.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <limits>
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
	if (!valid_period(daily_policy_.period_seconds))
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
	if (!definition->active || !definition->eligible_for_zone_completion)
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
			   std::string_view character_name, bool allow_stale, bool award_daily,
			   std::string *error)
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
		if (award_daily)
			award_daily_for(transaction);
		return result::already_applied;
	}

	transactions_.emplace(transaction.transaction_id,
			      stored_transaction{ transaction, encoded });
	for (uint32_t pid : transaction.credited_pids)
	{
		if (deleted_characters_.find({ transaction.season_id, pid }) !=
		    deleted_characters_.end())
			continue;
		auto &state = state_for(transaction.season_id, pid);
		state.transaction_ids.emplace(transaction.transaction_id, encoded);
		state.credit_masks[transaction.quest_definition_id] |=
			zone_story_quest_tracking::credit_mask_for_pid(transaction, pid);
		if (pid == transaction.direct_completer_pid && !character_name.empty())
			state.character_name = character_name;
	}
	if (award_daily)
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
	const std::string before = serialize_state();
	const result recorded =
		apply_transaction(event.transaction, event.character_name, false, true, error);
	if (recorded == result::invalid || recorded == result::conflict ||
	    recorded == result::rejected)
		return recorded;

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
		const result telemetry_result = record_telemetry(observation, error);
		if (telemetry_result == result::conflict || telemetry_result == result::invalid)
		{
			std::string restore_error;
			deserialize_state(before, &restore_error);
			return telemetry_result;
		}
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
	return result::applied;
}

void service::remember_character(uint32_t season_id, uint32_t pid, std::string character_name)
{
	if (!season_id || !pid)
		return;
	deleted_characters_.erase({ season_id, pid });
	state_for(season_id, pid).character_name = std::move(character_name);
}

bool service::erase_character(uint32_t season_id, uint32_t pid)
{
	if (!season_id || !pid)
		return false;
	characters_.erase({ season_id, pid });
	deleted_characters_.insert({ season_id, pid });
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
			telemetry = telemetry_.erase(telemetry);
		else
			++telemetry;
	}
	return true;
}

bool service::erase_character_all_seasons(uint32_t pid, uint32_t current_season_id)
{
	if (!pid || !current_season_id)
		return false;
	std::set<uint32_t> seasons;
	seasons.insert(current_season_id);
	for (const auto &[key, state] : characters_)
		if (key.second == pid)
			seasons.insert(key.first);
	for (const auto &[season, deleted_pid] : deleted_characters_)
		if (deleted_pid == pid)
			seasons.insert(season);
	for (uint32_t season : seasons)
	{
		characters_.erase({ season, pid });
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
			telemetry = telemetry_.erase(telemetry);
		else
			++telemetry;
	}
	return true;
}

zone_progress service::progress_for_zone(uint32_t season_id, uint32_t pid,
					 int32_t zone_number) const
{
	zone_progress progress;
	progress.zone_number = zone_number;
	const auto *state = find_state(season_id, pid);
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
		if (state &&
		    state->credit_masks.find(definition.definition_id) != state->credit_masks.end())
			progress.completed++;
	}
	progress.available = progress.total > 0;
	progress.milestone_25 = crossing(progress.completed, progress.total, 25);
	progress.milestone_50 = crossing(progress.completed, progress.total, 50);
	progress.milestone_75 = crossing(progress.completed, progress.total, 75);
	progress.milestone_100 = crossing(progress.completed, progress.total, 100);
	return progress;
}

personal_summary service::summary_for(uint32_t season_id, uint32_t pid,
				      std::string_view fallback_name) const
{
	personal_summary summary;
	summary.season_id = season_id;
	summary.pid = pid;
	const auto *state = find_state(season_id, pid);
	summary.character_name = state && !state->character_name.empty() ?
					 state->character_name :
					 std::string(fallback_name);
	std::set<std::string> completed_ids;
	for (const auto &definition : catalog_.definitions)
	{
		if (!definition.active || !definition.eligible_for_zone_completion ||
		    definition.content_revision != catalog_.content_revision)
			continue;
		++summary.total;
		if (state &&
		    state->credit_masks.find(definition.definition_id) != state->credit_masks.end())
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
	for (int32_t zone : zones)
	{
		zone_progress zone_state = progress_for_zone(season_id, pid, zone);
		if (zone_state.available && zone_state.completed == zone_state.total)
			++summary.full_zones;
		summary.zones.push_back(zone_state);
	}
	return summary;
}

std::vector<leaderboard_entry> service::sorted_leaderboard(uint32_t season_id,
							   int32_t zone_number) const
{
	(void)zone_number;
	std::vector<leaderboard_entry> entries;
	for (const auto &[key, state] : characters_)
	{
		if (key.first != season_id)
			continue;
		const personal_summary summary =
			summary_for(season_id, key.second, state.character_name);
		leaderboard_entry entry{ .pid = key.second,
					 .character_name =
						 summary.character_name.empty() ?
							 "Unknown adventurer" :
							 summary.character_name };
		entry.completed = summary.completed;
		entry.total = summary.total;
		entry.full_zones = summary.full_zones;
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
				      uint64_t page_size, uint32_t viewer_pid) const
{
	(void)zone_number;
	leaderboard_page output;
	if (page_size == 0)
		return output;
	const std::vector<leaderboard_entry> entries = sorted_leaderboard(season_id, 0);
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
				       uint32_t content_revision) const
{
	evidence_summary summary;
	summary.quest_definition_id = quest_definition_id;
	summary.content_revision = content_revision;
	std::set<uint32_t> pids;
	bool all_accessible = true;
	for (const auto &[id, observation] : telemetry_)
	{
		(void)id;
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

daily_assignment service::assign_daily(uint32_t season_id, uint32_t pid, int level, int racewar,
				       int64_t now, std::string *error)
{
	daily_assignment empty;
	empty.season_id = season_id;
	empty.pid = pid;
	if (!season_id || !pid || now <= 0 || level < 0 ||
	    !valid_period(daily_policy_.period_seconds))
	{
		fail(error, "invalid daily assignment context");
		return empty;
	}
	const int64_t period = period_for(now, daily_policy_.period_seconds);
	daily_assignment assignment;
	assignment.season_id = season_id;
	assignment.pid = pid;
	assignment.period = period;
	assignment.assigned_at = period * daily_policy_.period_seconds;
	assignment.expires_at = (period + 1) * daily_policy_.period_seconds;
	const auto *existing_state = find_state(season_id, pid);
	if (existing_state)
	{
		const auto existing = existing_state->daily_assignments.find(period);
		if (existing != existing_state->daily_assignments.end())
			return existing->second;
	}
	assignment.status = daily_policy_.enabled ? daily_status::no_eligible_candidate :
						    daily_status::disabled;
	if (!daily_policy_.enabled)
		return assignment;
	auto &state = state_for(season_id, pid);
	std::vector<std::string> candidates;
	for (const auto &definition : catalog_.definitions)
	{
		if (!definition.active || !definition.eligible_for_zone_completion ||
		    definition.content_revision != catalog_.content_revision)
			continue;
		const evidence_summary evidence =
			evidence_for(definition.definition_id, definition.content_revision);
		if (!evidence.suitable || (level < daily_policy_.minimum_level) ||
		    (daily_policy_.maximum_level > 0 && level > daily_policy_.maximum_level) ||
		    (daily_policy_.minimum_racewar > 0 &&
		     racewar < daily_policy_.minimum_racewar) ||
		    (daily_policy_.maximum_racewar > 0 && racewar > daily_policy_.maximum_racewar))
			continue;
		if (evidence.has_level_range &&
		    (level < evidence.minimum_level || level > evidence.maximum_level))
			continue;
		if (evidence.has_racewar_range &&
		    (racewar < evidence.minimum_racewar || racewar > evidence.maximum_racewar))
			continue;
		candidates.push_back(definition.definition_id);
	}
	std::sort(candidates.begin(), candidates.end());
	if (!candidates.empty())
	{
		/* Stable, deterministic selection is intentional: there is no reroll path. */
		uint64_t hash = 1469598103934665603ULL;
		const std::string seed = std::to_string(season_id) + ":" + std::to_string(pid) +
					 ":" + std::to_string(period);
		for (unsigned char value : seed)
		{
			hash ^= value;
			hash *= 1099511628211ULL;
		}
		assignment.quest_definition_id = candidates[hash % candidates.size()];
		assignment.content_revision = catalog_.content_revision;
		assignment.status = daily_status::assigned;
	}
	state.daily_assignments.emplace(period, assignment);
	return assignment;
}

result service::complete_daily(uint32_t season_id, uint32_t pid, std::string_view transaction_id,
			       int64_t now, std::string *error)
{
	if (!daily_policy_.enabled)
		return fail(error, "daily quests are disabled"), result::disabled;
	if (!season_id || !pid || transaction_id.empty() || now <= 0)
		return fail(error, "invalid daily completion context"), result::invalid;
	const int64_t period = period_for(now, daily_policy_.period_seconds);
	const auto *state = find_state(season_id, pid);
	if (!state)
		return fail(error, "daily assignment was not found"), result::not_found;
	const auto assignment = state->daily_assignments.find(period);
	if (assignment == state->daily_assignments.end())
		return fail(error, "daily assignment was not found"), result::not_found;
	if (assignment->second.status == daily_status::completed)
		return result::already_applied;
	if (assignment->second.status != daily_status::assigned ||
	    now >= assignment->second.expires_at)
		return fail(error, "daily assignment is not active"), result::rejected;
	const auto transaction = transactions_.find(std::string(transaction_id));
	if (transaction == transactions_.end())
		return fail(error, "authoritative completion transaction was not found"),
		       result::not_found;
	if (transaction->second.transaction.season_id != season_id ||
	    transaction->second.transaction.quest_definition_id !=
		    assignment->second.quest_definition_id ||
	    transaction->second.transaction.content_revision !=
		    assignment->second.content_revision ||
	    transaction->second.transaction.completed_at < assignment->second.assigned_at ||
	    transaction->second.transaction.completed_at >= assignment->second.expires_at ||
	    !contains_pid(transaction->second.transaction, pid))
		return fail(error, "completion does not satisfy the assigned daily quest"),
		       result::rejected;
	auto &mutable_state = state_for(season_id, pid);
	const std::string reward_key = std::to_string(season_id) + ":" + std::to_string(pid) + ":" +
				       std::to_string(period);
	if (!mutable_state.reward_keys.emplace(reward_key, std::string(transaction_id)).second)
		return result::already_applied;
	mutable_state.daily_assignments[period].status = daily_status::completed;
	mutable_state.daily_assignments[period].reward_amount = 1;
	mutable_state.daily_assignments[period].completion_transaction_id = transaction_id;
	return result::applied;
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
	       << display_character_name(summary)
	       << color(colors, "&n") << "\r\n";
	if (!progress.available)
	{
		output << "  N/A: this zone has no active zone-story quests in the current catalog.\r\n";
		return output.str();
	}
	const uint64_t percent = progress.total ? progress.completed * 100 / progress.total : 0;
	output << "  Completed: " << progress.completed << "/" << progress.total << " (" << percent
	       << "%, exact numerator/denominator)\r\n";
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
	       << display_character_name(summary)
	       << color(colors, "&n") << "\r\n";
	if (summary.total == 0)
	{
		output << "  N/A: the current production catalog contains no eligible quests.\r\n";
		return output.str();
	}
	const uint64_t overall_percent = summary.total ? summary.completed * 100 / summary.total : 0;
	output << "  Overall: " << summary.completed << "/" << summary.total << " ("
	       << overall_percent << "%, exact distinct completions)\r\n";
	output << "  Fully completed zones: " << summary.full_zones << "\r\n";
	if (daily_policy_.enabled && summary.renown > 0)
		output << "  Daily renown: " << summary.renown << "\r\n";
	for (const auto &zone : summary.zones)
		output << "  " << display_zone_name(zone) << ": " << zone.completed << "/"
		       << zone.total << (zone.milestone_100 ? " [100%]" : "") << "\r\n";
	return output.str();
}

std::string service::render_leaderboard(uint32_t season_id, int32_t zone_number, uint64_t page,
					uint64_t page_size, uint32_t viewer_pid, bool colors) const
{
	(void)zone_number;
	const leaderboard_page board = leaderboard(season_id, 0, page, page_size, viewer_pid);
	std::ostringstream output;
	output << "\r\n"
	       << color(colors, "&+L") << "Worldwide quest completion leaderboard"
	       << color(colors, "&n") << "\r\n";
	if (!board.total_entries)
	{
		output << "  No character completion records are available for this season.\r\n";
		return output.str();
	}
	for (const auto &entry : board.entries)
	{
		const uint64_t percentage = entry.total ? entry.completed * 100 / entry.total : 0;
		const bool viewer = entry.pid == viewer_pid;
		output << "  " << color(colors, viewer ? "&+Y" : "") << (viewer ? "* " : "  ")
		       << color(colors, viewer ? "&+Y" : "") << "#" << entry.rank << " "
		       << entry.character_name << " " << percentage << "%";
		output << color(colors, "&n") << "\r\n";
	}
	output << "  Page " << (page + 1) << ", " << board.total_entries << " players";
	if (board.own_rank)
		output << "; your rank: #" << board.own_rank;
	output << "\r\n";
	return output.str();
}

std::string service::render_daily(uint32_t season_id, uint32_t pid, int level, int racewar,
				  int64_t now, bool colors)
{
	if (!daily_policy_.enabled)
		return {};
	const daily_assignment assignment = assign_daily(season_id, pid, level, racewar, now);
	if (assignment.status != daily_status::assigned && assignment.status != daily_status::completed &&
	    assignment.status != daily_status::expired)
		return {};
	const auto *definition = find_definition(assignment.quest_definition_id);
	if (!definition)
		return {};
	std::ostringstream output;
	output << "\r\n"
	       << color(colors, "&+L") << "Daily Quest" << color(colors, "&n") << "\r\n";
	const personal_summary summary = summary_for(season_id, pid);
	output << "  Renown: " << summary.renown << "\r\n";
	output << "  Quest: " << display_quest_name(definition) << "\r\n";
	if (!definition->zone_name.empty())
		output << "  Area: " << definition->zone_name << "\r\n";
	if (!definition->giver_name.empty())
		output << "  From: " << definition->giver_name << "\r\n";
	if (!definition->objective.empty())
		output << "  Objective: " << definition->objective << "\r\n";
	output << "  Reward: 1 renown\r\n";
	switch (assignment.status)
	{
	case daily_status::assigned:
		output << "  Status: Ready to complete\r\n"
		       << "  Resets in: " << display_remaining(assignment.expires_at - now) << "\r\n";
		break;
	case daily_status::completed:
		output << "  Status: Completed; reward claimed\r\n";
		break;
	case daily_status::expired:
		output << "  Status: Expired\r\n";
		break;
	default:
		break;
	}
	return output.str();
}

std::string service::render_daily_score(uint32_t season_id, uint32_t pid, int level, int racewar,
					int64_t now, bool colors)
{
	if (!daily_policy_.enabled)
		return {};
	const daily_assignment assignment = assign_daily(season_id, pid, level, racewar, now);
	const personal_summary summary = summary_for(season_id, pid);
	const bool active = assignment.status == daily_status::assigned;
	const bool completed = assignment.status == daily_status::completed;
	if (!active && !completed && summary.renown == 0)
		return {};
	std::ostringstream output;
	output << "\r\n" << color(colors, "&+L") << "Daily: " << color(colors, "&n");
	if (active)
		output << "quest available - type 'quest' for details";
	else if (completed)
		output << "quest completed";
	if (summary.renown > 0)
	{
		if (active || completed)
			output << "; ";
		output << "renown " << summary.renown;
	}
	output << "\r\n";
	return output.str();
}

std::string service::serialize_state(std::string *error) const
{
	(void)error;
	std::ostringstream output;
	output << "ZSQF|1\n";
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
			       << hex_encode(state.character_name) << "\n";
		for (const auto &[definition_id, credit_mask] : state.credit_masks)
			output << "C|" << state.season_id << "|" << state.pid << "|"
			       << hex_encode(definition_id) << "|" << credit_mask << "\n";
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
	std::vector<std::tuple<uint32_t, uint32_t, std::string>> names;
	std::vector<std::tuple<uint32_t, uint32_t, std::string, uint32_t>> credits;
	std::vector<daily_assignment> assignments;
	std::vector<std::tuple<uint32_t, uint32_t, std::string, std::string>> rewards;
	std::vector<telemetry_observation> observations;
	std::vector<std::pair<uint32_t, uint32_t>> deleted;
	bool header_seen = false;
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
		if (fields.size() == 2 && fields[0] == "ZSQF" && fields[1] == "1")
		{
			header_seen = true;
			continue;
		}
		if (!header_seen)
			return fail(error, "zone-story state header is missing");
		if (fields[0] == "T" && fields.size() == 3)
		{
			std::string transaction_encoded;
			if (!hex_decode(fields[2], &transaction_encoded))
				return fail(error, "invalid serialized completion transaction");
			zone_story_quest_tracking::completion_transaction transaction;
			if (!zone_story_quest_tracking::deserialize_transaction(
				    transaction_encoded, &transaction, error))
				return false;
			transactions.push_back(std::move(transaction));
		}
		else if (fields[0] == "N" && fields.size() == 4)
		{
			uint32_t season = 0, pid = 0;
			std::string name;
			if (!parse_integer(fields[1], &season) || !parse_integer(fields[2], &pid) ||
			    !hex_decode(fields[3], &name) || !season || !pid)
				return fail(error, "invalid character name record");
			names.emplace_back(season, pid, std::move(name));
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
		else
			return fail(error, "unknown or malformed zone-story state record");
	}
	if (!header_seen)
		return fail(error, "zone-story state header is missing");
	characters_.clear();
	deleted_characters_.clear();
	transactions_.clear();
	telemetry_.clear();
	for (const auto &[season, pid] : deleted)
		deleted_characters_.insert({ season, pid });
	for (const auto &[season, pid, name] : names)
		if (deleted_characters_.find({ season, pid }) == deleted_characters_.end())
			state_for(season, pid).character_name = name;
	for (const auto &[season, pid, definition_id, credit_mask] : credits)
		if (deleted_characters_.find({ season, pid }) == deleted_characters_.end())
			state_for(season, pid).credit_masks[definition_id] = credit_mask;
	for (const auto &assignment : assignments)
		if (deleted_characters_.find({ assignment.season_id, assignment.pid }) ==
		    deleted_characters_.end())
			state_for(assignment.season_id, assignment.pid)
				.daily_assignments[assignment.period] = assignment;
	for (const auto &[season, pid, reward_key, transaction_id] : rewards)
		if (deleted_characters_.find({ season, pid }) == deleted_characters_.end())
			state_for(season, pid).reward_keys[reward_key] = transaction_id;
	for (const auto &observation : observations)
	{
		bool deleted_pid = false;
		for (const auto &[season, pid] : deleted_characters_)
			if (pid == observation.pid)
			{
				deleted_pid = true;
				break;
			}
		if (!deleted_pid)
			telemetry_[observation.observation_id] = observation;
	}
	for (const auto &transaction : transactions)
	{
		const result applied = apply_transaction(transaction, {}, true, false, error);
		if (applied != result::applied && applied != result::already_applied)
			return false;
	}
	return true;
}
} // namespace zone_story_quest_feature
