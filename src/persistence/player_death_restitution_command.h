#ifndef PLAYER_DEATH_RESTITUTION_COMMAND_H
#define PLAYER_DEATH_RESTITUTION_COMMAND_H

#include "persistence/critical_command.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// The native slice is deliberately bounded to individual item records.  It
// never carries or replaces a complete player snapshot.
constexpr uint16_t PLAYER_DEATH_RESTITUTION_PAYLOAD_VERSION = 2;
constexpr size_t PLAYER_DEATH_RESTITUTION_MAX_ITEMS = 64;
constexpr size_t PLAYER_DEATH_RESTITUTION_MAX_CLASSIFICATION_BYTES = 64;
constexpr size_t PLAYER_DEATH_RESTITUTION_MAX_NOTE_BYTES = 255;
constexpr size_t PLAYER_DEATH_RESTITUTION_MAX_ITEM_STATE_BYTES = 64 * 1024;
constexpr size_t PLAYER_DEATH_RESTITUTION_MAX_ORIGINAL_PAYLOAD_BYTES = 64 * 1024;
constexpr uint64_t PLAYER_DEATH_RESTITUTION_REVISION_WILDCARD = UINT64_MAX;
constexpr uint64_t PLAYER_DEATH_RESTITUTION_MAX_TIMER_SECONDS = 365ULL * 24ULL * 60ULL * 60ULL;
constexpr uint8_t PLAYER_DEATH_RESTITUTION_PLAYER_OWNER_TYPE = 1;
constexpr uint8_t PLAYER_DEATH_RESTITUTION_QUARANTINED_STATE = 3;

// Artifact authority evidence is part of the command identity.  A delivery
// may update only rows whose complete source signature still matches this
// evidence; no legacy row or canonical-domain identity is inferred at apply.
constexpr uint8_t PLAYER_DEATH_RESTITUTION_ARTIFACT_LOCATION_ON_PLAYER = 3;
constexpr uint8_t PLAYER_DEATH_RESTITUTION_ARTIFACT_LOCATION_ON_CORPSE = 5;
constexpr uint8_t PLAYER_DEATH_RESTITUTION_ARTIFACT_TYPE_MAJOR = 1;
constexpr uint8_t PLAYER_DEATH_RESTITUTION_ARTIFACT_TYPE_UNIQUE = 2;
constexpr uint8_t PLAYER_DEATH_RESTITUTION_ARTIFACT_TYPE_IOUN = 3;
constexpr uint8_t PLAYER_DEATH_RESTITUTION_ARTIFACT_LEGACY_GOD = 1U << 0;
constexpr uint8_t PLAYER_DEATH_RESTITUTION_ARTIFACT_LEGACY_MORTAL = 1U << 1;

enum class player_death_restitution_disposition : uint8_t
{
	deliver = 1,
	unresolved = 2,
	excluded = 3,
};

struct player_death_restitution_item_affect
{
	int16_t location;
	int16_t modifier;
};

struct player_death_restitution_item_extra_description
{
	std::vector<uint8_t> keyword;
	std::vector<uint8_t> description;
};

// One exact player_items row plus its child rows.  This is an item projection,
// not a complete player snapshot.  Container IDs are resolved from UIDs by
// the repository after parent rows have been inserted.
struct player_death_restitution_item_state
{
	uint64_t item_uid;
	uint32_t vnum;
	int16_t equip_slot;
	uint16_t quantity;
	int32_t weight;
	int32_t cost;
	int32_t timer;
	uint64_t extra_flags;
	int32_t wear_flags;
	int8_t item_type;
	std::array<int32_t, 8> values;
	std::array<std::vector<uint8_t>, 4> strings;
	std::array<bool, 4> string_present;
	std::array<uint64_t, 5> bitvectors;
	std::array<bool, 5> bitvector_present;
	int8_t material;
	int16_t condition;
	std::vector<player_death_restitution_item_affect> affects;
	std::vector<player_death_restitution_item_extra_description> extra_descriptions;
};

struct player_death_restitution_item
{
	uint64_t item_uid;
	uint64_t source_root_item_uid;
	uint64_t source_parent_item_uid;
	uint64_t delivered_root_item_uid;
	uint64_t delivered_parent_item_uid;
	// source_item_revision is the current item_current_owner revision used by
	// delivery/ownership locks; custody_item_revision is the immutable death
	// custody revision captured before quarantine.
	uint64_t source_item_revision;
	uint64_t custody_item_revision;
	uint64_t expected_item_revision;
	uint64_t expected_owner_revision;
	uint8_t expected_owner_state;
	uint8_t custody_state;
	uint8_t custody_owner_type;
	uint64_t custody_owner_id;
	uint64_t custody_owner_context_id;
	uint64_t custody_owner_revision;
	uint32_t vnum;
	uint32_t artifact_vnum;
	player_death_restitution_disposition disposition;
	bool artifact_timing_evidence_present;
	bool artifact_timing_uid_approved;
	uint64_t artifact_approval_uid;
	uint64_t artifact_loss_epoch;
	uint64_t artifact_source_timer_epoch;
	uint64_t artifact_usable_lifetime_seconds;
	// Exact source authority captured with the approval.  The projection mask
	// records which legacy tables existed; an absent row is itself fenced.
	uint8_t artifact_source_location_type;
	int32_t artifact_source_location;
	uint8_t artifact_type;
	bool artifact_domain_present;
	bool artifact_domain_item_uid_present;
	uint64_t artifact_domain_item_uid;
	uint64_t artifact_domain_item_revision;
	uint64_t artifact_domain_revision;
	bool artifact_baseline_present;
	uint64_t artifact_baseline_opening_timer_epoch;
	int32_t artifact_baseline_opening_bind_owner_pid;
	int64_t artifact_baseline_opening_bind_timer_epoch;
	uint64_t artifact_baseline_opening_revision;
	bool artifact_bind_present;
	int32_t artifact_bind_owner_pid;
	int64_t artifact_bind_timer_epoch;
	uint8_t artifact_legacy_projection_mask;
	std::string classification;
	std::string note;
	std::array<uint8_t, 32> metadata_digest;
	std::vector<uint8_t> metadata_payload;
	std::vector<uint8_t> original_payload;
};

struct player_death_restitution_plan
{
	uint32_t source_pid;
	uint64_t death_revision;
	uint32_t recipient_pid;
	critical_operation_id restitution_id;
	critical_operation_id death_operation_id;
	std::array<uint8_t, 32> evidence_digest;
	std::array<uint8_t, 32> plan_digest;
	uint64_t expected_recipient_save_revision;
	uint64_t expected_source_owner_revision;
	uint64_t expected_recipient_owner_revision;
	uint64_t loss_epoch;
	uint64_t accepted_at_usec;
	std::string actor;
	std::string reason;
	std::vector<player_death_restitution_item> items;
};

struct player_death_restitution_result
{
	critical_operation_id restitution_id;
	uint32_t source_pid;
	uint32_t recipient_pid;
	uint64_t delivery_epoch;
	uint64_t durable_revision;
	uint16_t candidate_count;
	uint16_t delivered_count;
	uint16_t unresolved_count;
	bool mutation_applied;
};

constexpr size_t PLAYER_DEATH_RESTITUTION_RESULT_BYTES = 64;

bool player_death_restitution_item_state_encode(const player_death_restitution_item_state &state,
						std::vector<uint8_t> *encoded);
bool player_death_restitution_item_state_decode(const uint8_t *encoded, size_t encoded_size,
						player_death_restitution_item_state *state);
bool player_death_restitution_item_state_valid(const player_death_restitution_item_state &state);

bool player_death_restitution_plan_valid(const player_death_restitution_plan &plan);
// Compute expiry from explicit remaining-at-loss evidence or UID-specific
// approval. Missing evidence never produces zero or a full reset.
bool player_death_restitution_artifact_delivery_timer(const player_death_restitution_item &item,
						      uint64_t delivery_epoch,
						      uint64_t *delivered_timer_epoch);
bool player_death_restitution_command_build(const player_death_restitution_plan &plan,
					    critical_command *command);
bool player_death_restitution_command_decode_payload(const critical_command &command,
						     player_death_restitution_plan *plan);

bool player_death_restitution_command_encode_result(
	const player_death_restitution_result &result,
	std::array<uint8_t, PLAYER_DEATH_RESTITUTION_RESULT_BYTES> *encoded);
bool player_death_restitution_command_decode_result(const uint8_t *encoded, size_t encoded_size,
						    player_death_restitution_result *result);

#endif
