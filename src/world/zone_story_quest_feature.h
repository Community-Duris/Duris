#ifndef ZONE_STORY_QUEST_FEATURE_H
#define ZONE_STORY_QUEST_FEATURE_H

#include "world/zone_story_quest_catalog.h"
#include "world/zone_story_quest_state_codec.h"

#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace zone_story_quest_feature
{
enum class result
{
	applied,
	already_applied,
	conflict,
	invalid,
	rejected,
	disabled,
	not_found,
	io_error,
};

enum class telemetry_outcome
{
	success,
	failure,
	abandoned,
	inaccessible,
	stale_revision,
};

enum class daily_status
{
	none,
	assigned,
	completed,
	expired,
	no_eligible_candidate,
	disabled,
};

struct completion_event
{
	zone_story_quest_tracking::completion_transaction transaction;
	std::string character_name;
	int level = 0;
	int racewar = 0;
	uint32_t party_size = 1;
	int strongest_party_level = 0;
	bool party_context_known = false;
	int64_t duration_seconds = 0;
	bool attempt_observed = true;
	telemetry_outcome outcome = telemetry_outcome::success;
};

struct telemetry_observation
{
	std::string observation_id;
	std::string quest_definition_id;
	uint32_t content_revision = 0;
	int64_t observed_at = 0;
	uint32_t pid = 0;
	int level = 0;
	int racewar = 0;
	uint32_t credit_mask = zone_story_quest_tracking::ZONE_STORY_CREDIT_NONE;
	uint32_t party_size = 0;
	int strongest_party_level = 0;
	int64_t duration_seconds = 0;
	telemetry_outcome outcome = telemetry_outcome::failure;
	bool accessible = false;
};

struct evidence_summary
{
	std::string quest_definition_id;
	uint32_t content_revision = 0;
	uint64_t observed_attempts = 0;
	uint64_t successful_attempts = 0;
	uint64_t failed_attempts = 0;
	uint64_t inaccessible_attempts = 0;
	uint64_t stale_revision_attempts = 0;
	uint64_t abandoned_attempts = 0;
	uint64_t unknown_party_context_attempts = 0;
	uint64_t carried_attempts = 0;
	uint64_t distinct_pids = 0;
	int minimum_level = 0;
	int maximum_level = 0;
	int minimum_racewar = 0;
	int maximum_racewar = 0;
	bool has_level_range = false;
	bool has_racewar_range = false;
	bool all_observed_accessible = false;
	bool suitable = false;
	std::string explanation;
};

struct daily_policy
{
	/* Shipped policy is deliberately disabled until telemetry has been reviewed. */
	bool enabled = false;
	uint32_t minimum_attempts = 20;
	uint32_t minimum_distinct_pids = 5;
	uint32_t minimum_successes = 1;
	int minimum_level = 1;
	int maximum_level = 0;
	int minimum_racewar = 0;
	int maximum_racewar = 0;
	int maximum_party_level_delta = 10;
	bool require_accessible_evidence = true;
	bool require_known_party_context = true;
	int64_t period_seconds = 24 * 60 * 60;
};

struct daily_assignment
{
	uint32_t season_id = 0;
	uint32_t pid = 0;
	int64_t period = 0;
	std::string quest_definition_id;
	uint32_t content_revision = 0;
	int64_t assigned_at = 0;
	int64_t expires_at = 0;
	daily_status status = daily_status::none;
	uint32_t reward_amount = 0;
	std::string completion_transaction_id;
};

struct zone_progress
{
	int32_t zone_number = 0;
	std::string zone_name;
	uint64_t completed = 0;
	uint64_t total = 0;
	bool available = false;
	bool milestone_25 = false;
	bool milestone_50 = false;
	bool milestone_75 = false;
	bool milestone_100 = false;
	bool discovered = false;
	int64_t discovered_at = 0;
};

struct personal_summary
{
	uint32_t season_id = 0;
	uint32_t pid = 0;
	std::string character_name;
	uint64_t completed = 0;
	uint64_t total = 0;
	uint64_t full_zones = 0;
	uint32_t renown = 0;
	std::vector<zone_progress> zones;
	uint64_t discovered_zones = 0;
};

struct leaderboard_entry
{
	uint32_t pid = 0;
	std::string character_name;
	int racewar = 0;
	uint64_t completed = 0;
	uint64_t total = 0;
	uint64_t full_zones = 0;
	uint64_t rank = 0;
};

struct leaderboard_page
{
	std::vector<leaderboard_entry> entries;
	uint64_t total_entries = 0;
	uint64_t own_rank = 0;
};

class service
{
    public:
	explicit service(zone_story_quest_catalog::catalog catalog = {});

	bool set_catalog(zone_story_quest_catalog::catalog catalog, std::string *error = nullptr);
	const zone_story_quest_catalog::catalog &catalog() const;

	void set_daily_policy(daily_policy policy);
	const daily_policy &get_daily_policy() const;
	result discover_zone(uint32_t season_id, uint32_t pid, int32_t zone_number,
			     int32_t room_vnum, int64_t visited_at, std::string_view source,
			     std::string *error = nullptr);
	bool has_discovered(uint32_t season_id, uint32_t pid, int32_t zone_number,
			    int64_t before = INT64_MAX) const;
	bool tracks_npc(int32_t mob_vnum) const;
	result meet_npc(uint32_t season_id, uint32_t pid, int32_t mob_vnum, int32_t room_vnum,
			int64_t encountered_at, std::string *error = nullptr);
	bool has_met_npc(uint32_t season_id, uint32_t pid, int32_t mob_vnum) const;
	std::string zone_command(int32_t zone_number) const;
	std::string encounter_hint(uint32_t season_id, uint32_t pid, int32_t zone_number,
				   int32_t mob_vnum) const;
	std::string completion_hint(uint32_t season_id, uint32_t pid,
				    std::string_view definition_id) const;
	int32_t resolve_zone(std::string_view name, std::string *error = nullptr) const;
	bool daily_eligible_for(uint32_t season_id, uint32_t pid, std::string_view definition_id,
				int level, int racewar, int strongest_party_level,
				int64_t now) const;
	bool
	existing_transaction(std::string_view id,
			     zone_story_quest_tracking::completion_transaction *transaction) const;
	std::string render_journal(
		uint32_t season_id, uint32_t pid, int32_t zone_number, int level, int racewar,
		int64_t now, bool daily_only, bool colors = true,
		const zone_story_quest_catalog::journal_inventory *inventory = nullptr) const;

	result record_completion(const completion_event &event, std::string *error = nullptr);
	result record_telemetry(const telemetry_observation &observation,
				std::string *error = nullptr);

	bool remember_character(uint32_t season_id, uint32_t pid, std::string character_name,
				bool leaderboard_eligible = true, int racewar = 0);
	bool erase_character(uint32_t season_id, uint32_t pid);
	bool erase_character_all_seasons(uint32_t pid, uint32_t current_season_id);
	zone_progress progress_for_zone(uint32_t season_id, uint32_t pid,
					int32_t zone_number) const;
	personal_summary summary_for(uint32_t season_id, uint32_t pid,
				     std::string_view fallback_name = {}) const;

	leaderboard_page leaderboard(uint32_t season_id, int32_t zone_number, uint64_t page,
				     uint64_t page_size, uint32_t viewer_pid = 0,
				     int64_t now = 0) const;

	evidence_summary evidence_for(std::string_view quest_definition_id,
				      uint32_t content_revision,
				      int64_t observed_before = INT64_MAX) const;

	static int64_t period_for(int64_t timestamp, int64_t period_seconds = 24 * 60 * 60);
	result complete_daily(uint32_t season_id, uint32_t pid, std::string_view transaction_id,
			      int64_t now, std::string *error = nullptr);
	daily_assignment daily_for(uint32_t season_id, uint32_t pid, int64_t period) const;

	std::string render_zone(uint32_t season_id, uint32_t pid, int32_t zone_number,
				std::string_view fallback_name = {}, bool colors = true) const;
	std::string render_summary(uint32_t season_id, uint32_t pid,
				   std::string_view fallback_name = {}, bool colors = true) const;
	std::string render_leaderboard(uint32_t season_id, int32_t zone_number, uint64_t page,
				       uint64_t page_size, uint32_t viewer_pid, bool colors = true,
				       int64_t now = 0) const;
	std::string render_daily(uint32_t season_id, uint32_t pid, int level, int racewar,
				 int64_t now, bool colors = true) const;
	std::string render_daily_score(uint32_t season_id, uint32_t pid, int level, int racewar,
				       int64_t now, bool colors = true) const;

	std::string serialize_state(std::string *error = nullptr) const;
	bool deserialize_state(std::string_view encoded, std::string *error = nullptr);
	zone_story_quest_state::changes changes_for_persistence() const;
	void mark_persisted();
	std::function<void()> checkpoint_for(uint32_t season, const std::vector<uint32_t> &pids,
					     std::string_view transaction_id = {});

    private:
	struct character_state
	{
		struct discovery
		{
			int32_t room_vnum = 0;
			int64_t visited_at = 0;
			std::string source;
		};
		uint32_t season_id = 0;
		uint32_t pid = 0;
		std::string character_name;
		int racewar = 0;
		std::map<std::string, uint32_t> credit_masks;
		std::map<std::string, int64_t> first_completion_times;
		std::map<int64_t, daily_assignment> daily_assignments;
		std::map<std::string, std::string> reward_keys;
		std::map<int32_t, discovery> discoveries;
		std::map<int64_t, std::set<std::string>> daily_completions;
		struct encounter
		{
			int32_t room_vnum = 0;
			int64_t encountered_at = 0;
		};
		std::map<int32_t, encounter> met_npcs;
	};

	struct stored_transaction
	{
		zone_story_quest_tracking::completion_transaction transaction;
		std::string encoded;
	};

	zone_story_quest_catalog::catalog catalog_;
	std::vector<zone_story_quest_catalog::quest_unit> quest_units_;
	std::set<int32_t> tracked_npcs_;
	daily_policy daily_policy_;
	std::map<std::pair<uint32_t, uint32_t>, character_state> characters_;
	std::set<std::pair<uint32_t, uint32_t>> deleted_characters_;
	std::set<std::pair<uint32_t, uint32_t>> leaderboard_exclusions_;
	std::map<std::string, stored_transaction> transactions_;
	std::map<std::string, telemetry_observation> telemetry_;
	std::map<std::string, std::set<std::string>> telemetry_by_definition_;
	int64_t checklist_starts_at_ = 0;
	bool all_dirty_ = true;
	std::set<std::pair<uint32_t, uint32_t>> dirty_characters_;
	std::set<std::string> dirty_transactions_;
	std::set<std::string> dirty_telemetry_;

	character_state &state_for(uint32_t season_id, uint32_t pid);
	const character_state *find_state(uint32_t season_id, uint32_t pid) const;
	const std::set<std::string> &daily_completed_ids(uint32_t season_id, uint32_t pid,
							 int64_t now) const;
	bool unit_encountered(uint32_t season_id, uint32_t pid,
			      const zone_story_quest_catalog::quest_unit &unit) const;
	std::set<std::string> completed_definition_ids(uint32_t season_id, uint32_t pid,
						       int64_t completed_before) const;
	zone_progress progress_for_zone_at(uint32_t season_id, uint32_t pid, int32_t zone_number,
					   const std::set<std::string> &completed_ids) const;
	personal_summary summary_for_at(uint32_t season_id, uint32_t pid,
					std::string_view fallback_name,
					int64_t completed_before) const;
	const zone_story_quest_tracking::quest_definition *
	find_definition(std::string_view definition_id) const;
	result
	apply_transaction(const zone_story_quest_tracking::completion_transaction &transaction,
			  std::string_view character_name, int racewar, bool allow_stale,
			  bool award_daily, std::string *error);
	void award_daily_for(const zone_story_quest_tracking::completion_transaction &transaction);
	void project_daily(const zone_story_quest_tracking::completion_transaction &transaction,
			   bool award_bonus);
	const zone_story_quest_catalog::zone_definition *find_zone(int32_t number) const;
	bool
	eligible_for_current_catalog(const zone_story_quest_tracking::completion_transaction &tx,
				     std::string *error) const;
	std::vector<leaderboard_entry> sorted_leaderboard(uint32_t season_id, int32_t zone_number,
							  int64_t completed_before) const;
};
} // namespace zone_story_quest_feature

#endif
