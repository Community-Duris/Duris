#ifndef ZONE_RESET_ITEM_OWNER_H
#define ZONE_RESET_ITEM_OWNER_H

#include "economy/zone_reset_item_command.h"

struct obj_data;
class critical_zone_reset_item_publication_owner;
class zone_reset_item_owner;
class quest_mobile_native_item_stage;
struct critical_native_recovery_envelope;
struct critical_completion;
struct critical_command;
struct zone_reset_item_recovery_context;
struct quest_mobile_native_item_effect;

// Actual constructor/source observations, never a sealed forest or admission.
// Season, room CAS, complete descendants and hot placement are still separate.
struct zone_reset_item_root_facts
{
	critical_operation_id operation_id{};
	economic_source_event reset_source{};
	int32_t zone_vnum = -1, room_vnum = -1;
	int object_rnum = -1;
	uint64_t accepted_at_usec = 0;
	player_item_snapshot literal{};
	native_mobile_birth_item_recipe recipe{};
};

enum class zone_reset_item_root_result
{
	refused,
	construction_failed,
	native_owner_required,
	load_missed,
	captured,
	held_refusal
};

// Caller owns this handle before preparation. No automatic native destruction:
// retain it until discard_root confirms disposal, including every refusal.
class zone_reset_item_root_stage
{
    public:
	zone_reset_item_root_stage() noexcept = default;
	zone_reset_item_root_stage(const zone_reset_item_root_stage &) = delete;
	zone_reset_item_root_stage &operator=(const zone_reset_item_root_stage &) = delete;

    private:
	friend class zone_reset_item_owner;
	struct implementation;
	implementation *state_ = nullptr;
};

// Initial genuine P constructor facts from the actual reset invocation/cursor.
// No O factory receipt is needed. Zero scope_root_operation records absence;
// it is neither selected-parent ownership nor a substitute source claim.
struct zone_reset_item_child_facts
{
	critical_operation_id scope_root_operation{};
	economic_source_event reset_scope{};
	uint32_t original_command_slot = 0;
	int32_t zone_vnum = -1;
	int object_rnum = -1, target_rnum = -1, original_zone_percent = 0;
	player_item_snapshot literal{};
	native_mobile_birth_item_recipe recipe{};
};

enum class zone_reset_item_child_result
{
	refused,
	construction_failed,
	native_owner_required,
	constructed,
	held_refusal
};

// Caller retains this unselected handle through genuine target/decision and
// ownership-transfer handoffs. No automatic native destruction on refusal.
class zone_reset_item_child_stage
{
    public:
	zone_reset_item_child_stage() noexcept = default;
	zone_reset_item_child_stage(const zone_reset_item_child_stage &) = delete;
	zone_reset_item_child_stage &operator=(const zone_reset_item_child_stage &) = delete;

    private:
	friend class zone_reset_item_owner;
	struct implementation;
	implementation *state_ = nullptr;
};

// Complete actual warm literals and factory recipes only. This observation has
// no current season/room revision and cannot build or admit an accounting command.
struct zone_reset_item_warm_forest_facts
{
	critical_operation_id operation_id{};
	economic_source_event reset_source{};
	int32_t zone_vnum = -1, room_vnum = -1;
	std::vector<player_item_snapshot> items;
	std::vector<native_mobile_birth_item_recipe> recipes;
	std::vector<zone_reset_coin_output> coins;
};
enum class zone_reset_item_warm_result
{
	refused,
	constructed,
	captured,
	load_missed,
	target_missing,
	native_owner_required,
	held_refusal
};

bool zone_reset_room_item_restore(const critical_native_recovery_envelope &) noexcept;
void zone_reset_room_item_completions(const critical_completion *, size_t) noexcept;
void zone_reset_room_item_pulse(bool prepare_original_resets) noexcept;
void zone_reset_room_item_replay_ready(bool) noexcept;
bool zone_reset_room_item_lifecycle_ready() noexcept;
bool zone_reset_room_item_warm_pending_vnum(int32_t zone_vnum) noexcept;
bool zone_reset_room_item_recovery_pending() noexcept;

class zone_reset_item_owner final
{
	friend void reset_zone(int, int);
	friend bool
	zone_reset_room_item_restore(const critical_native_recovery_envelope &) noexcept;
	friend void zone_reset_room_item_completions(const critical_completion *, size_t) noexcept;
	friend void zone_reset_room_item_pulse(bool) noexcept;
	friend void zone_reset_room_item_replay_ready(bool) noexcept;
	friend bool zone_reset_room_item_lifecycle_ready() noexcept;
	friend bool zone_reset_room_item_warm_pending_vnum(int32_t) noexcept;
	friend bool zone_reset_room_item_recovery_pending() noexcept;
	static bool replay_ready_;
	static bool restore_original(const critical_native_recovery_envelope &) noexcept;
	static void pulse_original(bool) noexcept;
	static void completions_original(const critical_completion *, size_t) noexcept;
	static bool lifecycle_original() noexcept;
	static bool pending_original(int32_t, bool) noexcept;
	// Passive actual factory census for the original native prototype limit.
	// Root-owned caller must hold/refuse admission if this observation fails.
	static bool pending_items(int rnum, size_t *output) noexcept;
	static bool cleanup_refusal(const critical_command &, const critical_completion &,
				    void *) noexcept;

	friend class critical_zone_reset_item_publication_owner;
	friend class zone_reset_room_publication_owner;
	friend class quest_mobile_native_birth_owner;
	// Called at the original real O constructor cut after its limit/force and
	// if_flag decisions. Read rnum/chance from that authentic parsed command;
	// do not accept caller-supplied source, UID, chance or decision flags.
	static zone_reset_item_root_result prepare_root(uint32_t command_slot, int room_rnum,
							zone_reset_item_root_stage *) noexcept;
	static struct obj_data *object(const zone_reset_item_root_stage &) noexcept;
	static bool observe_root(const zone_reset_item_root_stage &,
				 zone_reset_item_root_facts *) noexcept;
	static bool discard_root(zone_reset_item_root_stage *) noexcept;
	static bool empty(const zone_reset_item_root_stage &) noexcept;
	// The actual P cut constructs BEFORE artifact-owned/target/respawn/load
	// decisions. Derive the original prototype/target/chance from that command;
	// no caller-supplied parent, source, UID, chance or load-success flags.
	static zone_reset_item_child_result prepare_child(uint32_t actual_P_slot,
							  zone_reset_item_child_stage *) noexcept;
	static struct obj_data *object(const zone_reset_item_child_stage &) noexcept;
	static bool observe_child(const zone_reset_item_child_stage &,
				  zone_reset_item_child_facts *) noexcept;
	static bool discard_child(zone_reset_item_child_stage *) noexcept;
	static bool empty(const zone_reset_item_child_stage &) noexcept;
	// Real reset invocation/cursor establishes scope, including P-before-O.
	// Factories stay gated; values never grant source or target-owner admission.
	struct warm_checkpoint;
	struct warm_root;
	struct warm_command_scratch;
	static bool begin_warm_command_scratch(warm_root &) noexcept;
	static bool reserve_warm_command_scratch(size_t, void *) noexcept;
	static bool rebase_warm_command_scratch(warm_command_scratch &, size_t) noexcept;
	static bool retain_warm_command_output(warm_command_scratch &,
		const critical_native_recovery_envelope &) noexcept;
	static void release_warm_command_scratch(warm_command_scratch &) noexcept;
	static bool release_retired(warm_root &) noexcept;
	static bool publish_warm(warm_root &) noexcept;
	static bool restore_cold_bindings(std::span<quest_mobile_native_item_stage *>,
					  void *) noexcept;
	static bool settle_warm_checkpoint(warm_root &) noexcept;
	static bool checkpoint_warm_context(warm_root &,
					    const zone_reset_item_recovery_context &) noexcept;
	static int prepare_warm_action(warm_root &, uint8_t, size_t, size_t) noexcept;
	static bool finish_warm_action(warm_root &,
				       const quest_mobile_native_item_effect &) noexcept;
	struct warm_child;
	struct warm_registry;
	static warm_registry *warm_head_, *warm_current_;
	static bool begin_warm_capture() noexcept;
	// Passive same-invocation cursor observations; these never establish a scope,
	// construct, roll, place or admit. False leaves caller outputs untouched.
	static bool warm_capture_retryable(uint32_t actual_slot) noexcept;
	static bool warm_object_current(struct obj_data *actual, uint64_t uid) noexcept;
	// Internal frozen-target lifetime check avoids recursive phase observation.
	// The same real registry/source/factory proofs remain mandatory; this grants
	// no effect permission. External original callers use the wrapper above.
	static bool warm_object_current_impl(struct obj_data *, uint64_t,
					     bool require_retryable) noexcept;
	// First original same-Rnum incumbent, including TAKE. The original caller
	// retains its TAKE branch and must separately reauthenticate a live pointer.
	static bool original_room_incumbent(uint32_t actual_O_slot, int room_rnum,
					    struct obj_data **, uint64_t *) noexcept;
	static zone_reset_item_root_result capture_warm_root(uint32_t actual_O_slot, int room_rnum,
							     struct obj_data **) noexcept;
	static zone_reset_item_warm_result capture_warm_child(uint32_t actual_P_slot,
							      struct obj_data **) noexcept;
	static zone_reset_item_warm_result place_warm_child(uint32_t actual_P_slot) noexcept;
	// Called before native finish_reset only at the actual observed S boundary.
	// Sequential processing/table/last_cmd proof is supplied by the native owner.
	static void abort_warm_capture() noexcept;
	static bool finish_warm_capture(uint32_t actual_S_slot, int original_last_cmd) noexcept;
	static bool finish_closed_warm_capture(warm_registry &) noexcept;
	static bool warm_scope_current(const warm_registry &) noexcept;
	static bool warm_retained_size(size_t *) noexcept;
	static bool seal_warm_root(warm_root &) noexcept;
	// Original warm owner supplies the sealed complete forest and retains the
	// exact initial command/envelope across every storage/canonical-copy refusal.
	// SQL preserves its original pool/confirmed rollback cut. Regular native flat
	// observes independent active season/full room and custody absence under ONE
	// recovered configured-root lock. No submission or publication; prospective
	// aggregate provider/caller reservations remain required before opening gates.
	// Never predicts the next room clock for another retained root.
	static bool prepare_warm_command(const critical_operation_id &,
					 critical_native_recovery_envelope *, warm_command_scratch &) noexcept;
	static bool prepare_warm_command_sql(const critical_operation_id &,
					     critical_native_recovery_envelope *) noexcept;
	static bool prepare_warm_command_flat(const critical_operation_id &,
					      critical_native_recovery_envelope *, warm_command_scratch &) noexcept;
	static bool warm_root_current(const warm_root &) noexcept;
	static bool observe_warm_forest(const critical_operation_id &,
					zone_reset_item_warm_forest_facts *) noexcept;
};

#endif
