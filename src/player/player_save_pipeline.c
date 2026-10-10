#include "persistence/death_recovery_visibility.h"
#include "player/player_save_pipeline.h"
#include "net/network_wakeup.h"
#include "sql/sql_thread_init.h"
#include "persistence/persistence_observability.h"
#include <cstdlib>
#include <cstring>

#include "core/prototypes.h"
#include "core/files.h"
#include "classes/necromancy.h"
#include "flatfile/flatfile_player_repository.h"
#include "persistence/persistence_mode.h"
#include "player/player_save_journal.h"
#include "player/player_save_worker.h"
#include "player/player_retained_deque.h"
#include "player/player_save_execution_guard.h"
#include "player/player_save_replay_ownership.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "player/player_snapshot_repository.h"
#include "core/structs.h"
#include "core/utils.h"
#include "magic/spell_item_lifecycle.h"
#include "item/item_movement_transaction.h"
#include "item/ordinary_drop_recovery.h"
#include "economy/coin_physical_recovery.h"
#include "economy/currency_transaction.h"
#include "persistence/critical_command_coordinator.h"
#include "economy/item_transfer_accounting.h"
#include "economy/collector_accounting.h"
#include "economy/shop_trade_accounting.h"
#include "economy/shop_trade_recovery_manifest.h"
#include "economy/economic_gameplay_authority.h"
#include "persistence/sql_room_item_payload.h"
#include "world/quest_reward_recovery.h"
#include "world/native_quest_recovery_context.h"
#include "item/held_retirement_recovery.h"
#include "economy/auction_native_command_context.h"
#include "economy/auction_native_publication.h"
#include "economy/auction_repository.h"
#include "player/craft_progression_hooks.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <array>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <limits>
#include <mutex>
#include <new>
#include <optional>
#include <set>
#include <thread>
#include <type_traits>
#include <utility>

extern P_char character_list;

#ifndef __NO_MYSQL__
#include <mysql/mysql.h>
#ifdef TEST_MUD
#include "player/player_death_conflict_repository.h"
#endif
#endif

namespace
{
/** Cache the explicit diagnostic switch for player-save capture and durability tracing. */
bool trace_player_saves()
{
	static const bool enabled = []
	{
		const char *value = std::getenv("DURIS_NEVENT_TRACE_PLAYER");
		return value && std::strcmp(value, "1") == 0;
	}();
	return enabled;
}

std::mutex pipeline_mutex;
std::condition_variable append_available;
using player_save_execution_guard::resident_claim;
using player_save_execution_guard::execution_scope;
using player_save_execution_guard::ownership_status;

struct retained_snapshot
{
	resident_claim residence;
	player_snapshot body;
	retained_snapshot() noexcept = default;
	retained_snapshot(player_snapshot &&snapshot, resident_claim &&claim) noexcept
		: residence(std::move(claim))
		, body(std::move(snapshot))
	{
	}
	retained_snapshot(retained_snapshot &&) noexcept = default;
	retained_snapshot &operator=(retained_snapshot &&other) noexcept
	{
		if (this == &other)
			return *this;
		// Retire the old body before releasing its residence during deque moves.
		body = std::move(other.body);
		residence = std::move(other.residence);
		return *this;
	}
};
player_retained_deque<retained_snapshot> pending_append;
player_retained_deque<retained_snapshot> durable_ready;
// Allocation-free retention when a failed append cannot be requeued. Shutdown
// cannot destroy an unjournaled original merely because its thread has joined.
std::optional<retained_snapshot> append_retry;
std::thread dispatcher;
player_save_pipeline_health health = {};
player_save_pipeline_replay_gate replay_gate;
size_t retained_bytes = 0;
bool stop_requested = false;
bool accepting = false;
bool execution_started = false;
// Protected by pipeline_mutex. Only the actual dispatcher acknowledges entry;
// retain that observation even if its live running state subsequently clears.
bool dispatcher_entry_acknowledged = false;
bool shutdown_incomplete = false;
// Enabled close must not turn late teardown saves into epoch-zero synchronous
// writes. Only an explicit cancellation before stopping may reopen admission.
bool lifecycle_admission_closed = false;
bool lifecycle_stop_attempted = false;
bool append_inflight = false;
std::atomic<bool> replay_revisit_requested{ false };
int append_inflight_pid = 0;
player_revision_t append_inflight_revision = 0;
/* One failed graph may be recaptured once.  Keep the PID armed until a later
 * database acknowledgement proves custody and payload agree; otherwise every
 * rejected recapture creates a new revision and an unbounded wizlog loop. */
std::set<int32_t> custody_recapture_armed;

enum class literal_checkpoint_profile : uint8_t
{
	ordinary_drop,
	shop,
	flat_shop,
	native_quest,
	held_retirement,
	auction,
	flat_shop_restored,
	smith,
	flat_smith,
};

struct literal_inventory_checkpoint
{
	literal_checkpoint_profile profile = literal_checkpoint_profile::ordinary_drop;
	uint32_t level = 0;
	uint64_t held_selected_uid = 0;
	std::vector<uint8_t> held_before, held_after, held_attachment;
	// Same slot: exact attempted successor survives false/uncertain CAS.
	std::vector<uint8_t> held_attachment_successor;
	uint64_t held_successor_revision = 0;
	uint64_t held_recovery_revision = 0;
	size_t held_attachment_reserved_bytes = 0;
	bool held_passive_restored = false, held_runtime_rebound = false;
	std::array<uint64_t, 9> auction_literal_roots{};
	size_t auction_literal_root_count = 0;
	player_literal_inventory_token token = {};
	std::vector<uint8_t> payload;
	// Live shop admission retains its original whole-player checkpoint alongside
	// the frozen command, inside the same aggregate-bounded reservation.
	std::vector<uint8_t> original_shop_body;
	std::vector<uint8_t> original_native_quest_before, original_native_quest_after;
	// Separate literal money fence; item-list framing remains unchanged.
	bool native_money_only = false;
	std::array<int64_t, 4> native_money_before = {};
	uint64_t native_money_wallet_revision = 0;
	std::vector<uint8_t> original_auction_before, original_auction_after;
	std::vector<uint8_t> restored_auction_attachment;
	uint64_t restored_auction_revision = 0;
	size_t auction_attachment_reserved_bytes = 0;
	bool restored_auction = false, auction_runtime_rebound = false;
	// Exact passive replay identity only. No runtime actor identity is restored.
	std::vector<uint8_t> restored_native_quest_attachment;
	uint64_t restored_native_quest_revision = 0;
	// Capacity only: preserve original replay identity while reserving current
	// or uncertain successor attachment growth under the same aggregate limit.
	size_t native_quest_attachment_reserved_bytes = 0;
	bool restored_native_quest = false;
	// Set only by the private native owner after authentic player rebinding.
	// Original replay attachment/revision and execution hold remain unchanged.
	bool native_quest_runtime_rebound = false;
	player_revision_t captured_revision = 0;
	player_revision_t acknowledged_revision = 0;
	critical_operation_id operation_id = {};
	bool held = false;
	bool restored_sql_drop = false;
	uint64_t execution_hold_generation = 0;
	// Live regular-flat source identity, never restored from a value DTO.
	std::string flat_shop_root, flat_shop_account;
	uint8_t flat_shop_racewar = 0;
	economic_shop_checkpoint_projection flat_shop_mapping{};
	uint64_t flat_shop_ownership_epoch = 0;
	bool flat_shop_native_attempt_started = false;
	size_t flat_shop_native_reserved_bytes = 0;
	size_t flat_shop_command_reserved_bytes = 0;
	size_t flat_shop_payload_reserved_bytes = 0;
	size_t flat_shop_publication_reserved_bytes = 0;
	// Passive restored-native owner allocations plus exact slot buffer capacity
	// beyond existing size-based accounting. No attempted live source is invented.
	size_t flat_shop_restored_reserved_bytes = 0;
	size_t flat_shop_restored_native_owner_bytes = 0;
	// Separate immutable charge for the actual retained cold publication stage.
	size_t flat_shop_restored_publication_reserved_bytes = 0;
	// Separate exact journal bytes; original ACK/body and native charges stay.
	std::vector<uint8_t> flat_shop_journal_command;
};
bool flat_shop_context_matches(const literal_inventory_checkpoint &, P_char) noexcept;
bool smith_flat_context_matches(const literal_inventory_checkpoint &, P_char) noexcept;
bool smith_profile(literal_checkpoint_profile) noexcept;
uint64_t flat_shop_inventory_generation = 0;
std::array<literal_inventory_checkpoint, PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS>
	literal_inventory_checkpoints = {};
uint64_t literal_inventory_generation = 0;
constexpr player_component_mask_t LITERAL_INVENTORY_COMPONENTS = PLAYER_COMPONENT_EQUIPMENT |
								 PLAYER_COMPONENT_INVENTORY;

literal_inventory_checkpoint *find_literal_inventory_locked(int pid)
{
	for (auto &checkpoint : literal_inventory_checkpoints)
		if (checkpoint.token.pid == pid)
			return &checkpoint;
	return nullptr;
}

bool literal_inventory_capacity_locked(size_t incoming_bytes,
				       const literal_inventory_checkpoint *replacing = nullptr)
{
	if (incoming_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES)
		return false;
	for (const auto &checkpoint : literal_inventory_checkpoints)
	{
		if (&checkpoint == replacing)
			continue;
		if (checkpoint.payload.size() > PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming_bytes)
			return false;
		incoming_bytes += checkpoint.payload.size();
		const size_t original_body_bytes =
			smith_profile(checkpoint.profile) ?
				checkpoint.original_shop_body.capacity() :
				checkpoint.original_shop_body.size();
		if (original_body_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming_bytes)
			return false;
		incoming_bytes += original_body_bytes;
		for (const auto *identity :
		     { &checkpoint.flat_shop_root, &checkpoint.flat_shop_account })
		{
			const size_t amount =
				checkpoint.profile == literal_checkpoint_profile::flat_smith ?
					identity->capacity() :
					identity->size();
			if (amount > PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming_bytes)
				return false;
			incoming_bytes += amount;
		}
		if (checkpoint.flat_shop_native_reserved_bytes >
		    PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming_bytes)
			return false;
		incoming_bytes += checkpoint.flat_shop_native_reserved_bytes;
		if (checkpoint.flat_shop_command_reserved_bytes >
		    PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming_bytes)
			return false;
		incoming_bytes += checkpoint.flat_shop_command_reserved_bytes;
		if (checkpoint.flat_shop_payload_reserved_bytes >
		    PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming_bytes)
			return false;
		incoming_bytes += checkpoint.flat_shop_payload_reserved_bytes;
		if (checkpoint.flat_shop_publication_reserved_bytes >
		    PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming_bytes)
			return false;
		incoming_bytes += checkpoint.flat_shop_publication_reserved_bytes;
		if (checkpoint.flat_shop_restored_reserved_bytes >
		    PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming_bytes)
			return false;
		incoming_bytes += checkpoint.flat_shop_restored_reserved_bytes;
		if (checkpoint.flat_shop_restored_publication_reserved_bytes >
		    PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming_bytes)
			return false;
		incoming_bytes += checkpoint.flat_shop_restored_publication_reserved_bytes;
		if (checkpoint.flat_shop_journal_command.capacity() >
		    PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming_bytes)
			return false;
		incoming_bytes += checkpoint.flat_shop_journal_command.capacity();
		for (const auto *body :
		     { &checkpoint.original_native_quest_before,
		       &checkpoint.original_native_quest_after,
		       &checkpoint.restored_native_quest_attachment, &checkpoint.held_before,
		       &checkpoint.held_after, &checkpoint.held_attachment,
		       &checkpoint.original_auction_before, &checkpoint.original_auction_after,
		       &checkpoint.restored_auction_attachment })
		{
			if (body->size() > PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming_bytes)
				return false;
			incoming_bytes += body->size();
		}
		const size_t extra =
			checkpoint.native_quest_attachment_reserved_bytes >
					checkpoint.restored_native_quest_attachment.size() ?
				checkpoint.native_quest_attachment_reserved_bytes -
					checkpoint.restored_native_quest_attachment.size() :
				0;
		if (extra > PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming_bytes)
			return false;
		incoming_bytes += extra;
		const size_t held_extra = checkpoint.held_attachment_reserved_bytes >
							  checkpoint.held_attachment.size() ?
						  checkpoint.held_attachment_reserved_bytes -
							  checkpoint.held_attachment.size() :
						  0;
		if (held_extra > PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming_bytes)
			return false;
		incoming_bytes += held_extra;
		const size_t auction_extra =
			checkpoint.auction_attachment_reserved_bytes >
					checkpoint.restored_auction_attachment.size() ?
				checkpoint.auction_attachment_reserved_bytes -
					checkpoint.restored_auction_attachment.size() :
				0;
		if (auction_extra > PLAYER_SAVE_PIPELINE_MAX_BYTES - incoming_bytes)
			return false;
		incoming_bytes += auction_extra;
	}
	return true;
}

bool literal_inventory_blob(const player_snapshot &snapshot, uint64_t root_uid,
			    std::vector<uint8_t> *blob)
{
	if (!root_uid || !blob || snapshot.items.size() > PLAYER_SNAPSHOT_MAX_OBJECTS ||
	    (snapshot.components & LITERAL_INVENTORY_COMPONENTS) != LITERAL_INVENTORY_COMPONENTS)
		return false;
	try
	{
		std::vector<int32_t> indices(snapshot.items.size(), PLAYER_SNAPSHOT_NO_PARENT);
		std::vector<player_item_snapshot> selected;
		bool found = false;
		for (size_t index = 0; index < snapshot.items.size(); ++index)
		{
			const auto &item = snapshot.items[index];
			if (item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
			    item.parent_index >= static_cast<int32_t>(index))
				return false;
			const bool root = item.object_uid == root_uid;
			if (root && (found || item.parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
				     item.equipment_slot != 0))
				return false;
			const bool child = item.parent_index >= 0 &&
					   indices[item.parent_index] >= 0;
			if (!root && !child)
				continue;
			if (!item.object_uid || item.equipment_slot ||
			    item.string_mask !=
				    (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3))
				return false;
			found = found || root;
			indices[index] = static_cast<int32_t>(selected.size());
			selected.push_back(item);
			selected.back().parent_index = child ? indices[item.parent_index] :
							       PLAYER_SNAPSHOT_NO_PARENT;
		}
		return found &&
		       player_item_snapshot_list_encode(selected, blob) ==
			       player_snapshot_codec_result::ok &&
		       !blob->empty() && blob->size() <= ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

constexpr player_component_mask_t SHOP_CHECKPOINT_COMPONENTS = LITERAL_INVENTORY_COMPONENTS |
							       PLAYER_COMPONENT_STATUS;

constexpr player_component_mask_t HELD_RETIREMENT_CHECKPOINT_COMPONENTS =
	PLAYER_COMPONENT_STATUS | LITERAL_INVENTORY_COMPONENTS;
bool held_retirement_checkpoint_blob(const player_snapshot &snapshot, std::vector<uint8_t> *blob,
				     uint32_t *level = nullptr)
{
	if (!blob || (snapshot.components & HELD_RETIREMENT_CHECKPOINT_COMPONENTS) !=
			     HELD_RETIREMENT_CHECKPOINT_COMPONENTS)
		return false;
	size_t held = 0;
	for (const auto &item : snapshot.items)
		if (item.equipment_slot == HOLD + 1)
		{
			if (!item.object_uid || item.object_uid == UINT64_MAX ||
			    item.type != ITEM_PICK ||
			    item.parent_index != PLAYER_SNAPSHOT_NO_PARENT)
				return false;
			++held;
		}
	if (held != 1 || player_item_snapshot_list_encode(snapshot.items, blob) !=
				 player_snapshot_codec_result::ok)
		return false;
	if (level)
		*level = 0;
	return true;
}

player_component_mask_t literal_checkpoint_components(const literal_inventory_checkpoint &slot)
{
	if (slot.profile == literal_checkpoint_profile::held_retirement)
		return HELD_RETIREMENT_CHECKPOINT_COMPONENTS;
	return slot.profile == literal_checkpoint_profile::shop || smith_profile(slot.profile) ||
			       slot.profile == literal_checkpoint_profile::flat_shop ||
			       slot.profile == literal_checkpoint_profile::auction ||
			       slot.native_money_only ?
		       SHOP_CHECKPOINT_COMPONENTS :
		       LITERAL_INVENTORY_COMPONENTS;
}

bool native_money_snapshot_cash(const player_snapshot &snapshot,
				std::array<int64_t, 4> *output) noexcept
{
	if (!output ||
	    (snapshot.components & SHOP_CHECKPOINT_COMPONENTS) != SHOP_CHECKPOINT_COMPONENTS)
		return false;
	constexpr std::array<player_status_field, 4> fields{ player_status_field::copper,
							     player_status_field::silver,
							     player_status_field::gold,
							     player_status_field::platinum };
	std::array<int64_t, 4> cash{};
	std::array<bool, 4> seen{};
	for (const auto &row : snapshot.status_integers)
		for (size_t i = 0; i < fields.size(); ++i)
			if (row.field == fields[i])
			{
				if (seen[i] ||
				    (row.is_unsigned ?
					     row.unsigned_value > INT_MAX :
					     row.signed_value < 0 || row.signed_value > INT_MAX))
					return false;
				cash[i] = row.is_unsigned ?
						  static_cast<int64_t>(row.unsigned_value) :
						  row.signed_value;
				seen[i] = true;
			}
	if (std::find(seen.begin(), seen.end(), false) != seen.end())
		return false;
	*output = cash;
	return true;
}

bool shop_checkpoint_blob(const player_snapshot &snapshot, std::vector<uint8_t> *blob,
			  uint32_t *level_out = nullptr)
{
	if (!blob ||
	    (snapshot.components & SHOP_CHECKPOINT_COMPONENTS) != SHOP_CHECKPOINT_COMPONENTS ||
	    snapshot.items.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return false;
	uint32_t level = 0;
	for (const auto &row : snapshot.status_integers)
		if (row.field == player_status_field::level)
		{
			if (level ||
			    (row.is_unsigned ? !row.unsigned_value || row.unsigned_value > 255 :
					       row.signed_value <= 0 || row.signed_value > 255))
				return false;
			level = static_cast<uint32_t>(row.is_unsigned ? row.unsigned_value :
									row.signed_value);
		}
	if (!level)
		return false;
	try
	{
		std::vector<uint8_t> encoded;
		if (player_item_snapshot_list_encode(snapshot.items, &encoded) !=
			    player_snapshot_codec_result::ok ||
		    encoded.size() > PLAYER_SNAPSHOT_MAX_BYTES - sizeof(uint32_t))
			return false;
		// Whole persistent item list, then captured level; volatile status is
		// deliberately not part of this checkpoint's immutable comparison.
		for (unsigned shift = 0; shift < 32; shift += 8)
			encoded.push_back(static_cast<uint8_t>(level >> shift));
		*blob = std::move(encoded);
		if (level_out)
			*level_out = level;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool native_quest_checkpoint_blob(const player_snapshot &snapshot, std::vector<uint8_t> *blob,
				  uint32_t *level = nullptr)
{
	if (!blob ||
	    (snapshot.components & LITERAL_INVENTORY_COMPONENTS) != LITERAL_INVENTORY_COMPONENTS ||
	    snapshot.items.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return false;
	if (player_item_snapshot_list_encode(snapshot.items, blob) !=
	    player_snapshot_codec_result::ok)
		return false;
	if (level)
		*level = 0;
	return true;
}

bool literal_checkpoint_blob(const player_snapshot &snapshot,
			     const literal_inventory_checkpoint &checkpoint,
			     std::vector<uint8_t> *blob)
{
	if (checkpoint.profile == literal_checkpoint_profile::held_retirement)
		return economic_gameplay_authority::active_regular_sql() &&
		       held_retirement_checkpoint_blob(snapshot, blob);
	if (checkpoint.profile == literal_checkpoint_profile::native_quest)
	{
		std::array<int64_t, 4> cash{};
		return economic_gameplay_authority::active_regular_sql() &&
		       (!checkpoint.native_money_only ||
			(native_money_snapshot_cash(snapshot, &cash) &&
			 cash == checkpoint.native_money_before)) &&
		       native_quest_checkpoint_blob(snapshot, blob);
	}
	if (checkpoint.profile == literal_checkpoint_profile::flat_shop ||
	    checkpoint.profile == literal_checkpoint_profile::flat_smith)
		return economic_gameplay_authority::active_regular_flat() &&
		       shop_checkpoint_blob(snapshot, blob);
	if (checkpoint.profile == literal_checkpoint_profile::shop ||
	    checkpoint.profile == literal_checkpoint_profile::smith ||
	    checkpoint.profile == literal_checkpoint_profile::auction)
		return economic_gameplay_authority::active_regular_sql() &&
		       shop_checkpoint_blob(snapshot, blob);
	return literal_inventory_blob(snapshot, checkpoint.token.root_uid, blob);
}

// Retain the actual queue body, including its real save revision. Encoding and
// aggregate checks happen before the original queue ownership is changed.
bool prepare_flat_shop_enqueue_locked(const literal_inventory_checkpoint *slot,
				      const player_snapshot &snapshot, std::vector<uint8_t> *body)
{
	if (!slot || (slot->profile != literal_checkpoint_profile::flat_shop &&
		      !smith_profile(slot->profile)))
		return true;
	if (!body || slot->held || !snapshot.revision ||
	    (snapshot.components & SHOP_CHECKPOINT_COMPONENTS) != SHOP_CHECKPOINT_COMPONENTS ||
	    player_snapshot_encode(snapshot, body) != player_snapshot_codec_result::ok)
		return false;
	size_t bytes = slot->payload.size();
	if (bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES)
		return false;
	for (size_t amount : { smith_profile(slot->profile) ? body->capacity() : body->size(),
			       slot->profile == literal_checkpoint_profile::flat_smith ?
				       slot->flat_shop_root.capacity() :
				       slot->flat_shop_root.size(),
			       slot->profile == literal_checkpoint_profile::flat_smith ?
				       slot->flat_shop_account.capacity() :
				       slot->flat_shop_account.size() })
	{
		if (amount > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
			return false;
		bytes += amount;
	}
	return literal_inventory_capacity_locked(bytes, slot);
}

void note_literal_enqueue_locked(literal_inventory_checkpoint *checkpoint,
				 player_revision_t revision)
{
	if (checkpoint)
	{
		checkpoint->captured_revision = revision;
		checkpoint->acknowledged_revision = 0;
	}
}

player_save_apply_fn selected_snapshot_apply()
{
#ifdef __NO_MYSQL__
	return flatfile_player_snapshot_apply_selected;
#else
#ifdef TEST_MUD
	// Qualification only: no normal/production selection before the socket,
	// recovery-view and restart acceptance gates. Both worker and journal
	// replay use this selector; never substitute a different replay owner.
	const char *enabled = std::getenv("DURIS_TEST_SQL_DEATH_CONFLICT_RECOVERY");
	const char *disposable = std::getenv("TEST_DB_DISPOSABLE");
	const char *host = std::getenv("DB_HOST");
	const char *database = std::getenv("DB_NAME");
	constexpr char prefix[] = "corpse_journey_test_";
	constexpr size_t prefix_length = sizeof(prefix) - 1;
	if (enabled && std::strcmp(enabled, "1") == 0 && disposable &&
	    std::strcmp(disposable, "1") == 0 && host &&
	    (std::strcmp(host, "127.0.0.1") == 0 || std::strcmp(host, "localhost") == 0) &&
	    database && std::strlen(database) == prefix_length + 12 &&
	    std::strncmp(database, prefix, prefix_length) == 0 &&
	    std::strspn(database + prefix_length, "0123456789abcdef") == 12)
		return player_death_conflict_apply_from_pool;
#endif
	return player_snapshot_repository_apply_from_pool;
#endif
}

struct terminal_fence
{
	int pid = 0;
	player_revision_t revision = 0;
	bool journaled = false;
	bool acknowledged = false;
	bool death_pinned = false;
	uint64_t corpse_uid = 0;
	critical_operation_id operation_id = {};
	uint64_t wallet_pile_uid = 0;
	std::optional<player_snapshot> death_snapshot;
	resident_claim death_residence;
};

static_assert(std::is_nothrow_move_constructible_v<player_snapshot>);

std::array<terminal_fence, PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS> terminal_fences = {};

struct target_save_login_fence
{
	int pid = 0;
	player_revision_t expected_revision = 0;
};

std::array<target_save_login_fence, PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS>
	target_save_login_fences = {};

target_save_login_fence *find_target_save_login_fence_locked(int pid);

/** Find a player durability fence; the caller must hold pipeline_mutex. */
terminal_fence *find_terminal_fence_locked(int pid)
{
	for (terminal_fence &fence : terminal_fences)
		if (fence.pid == pid)
			return &fence;
	return nullptr;
}

terminal_fence *allocate_terminal_fence_locked(int pid)
{
	if (find_target_save_login_fence_locked(pid))
		return nullptr;
	if (terminal_fence *existing = find_terminal_fence_locked(pid))
		return existing;
	for (terminal_fence &fence : terminal_fences)
		if (!fence.pid)
		{
			fence.pid = pid;
			++health.terminal_fences;
			return &fence;
		}
	return nullptr;
}

/** Find a recipient-only save/login fence; the caller must hold pipeline_mutex. */
target_save_login_fence *find_target_save_login_fence_locked(int pid)
{
	for (target_save_login_fence &fence : target_save_login_fences)
		if (fence.pid == pid)
			return &fence;
	return nullptr;
}

target_save_login_fence *
allocate_target_save_login_fence_locked(int pid, player_revision_t expected_revision)
{
	if (find_target_save_login_fence_locked(pid))
		return nullptr;
	for (target_save_login_fence &fence : target_save_login_fences)
		if (!fence.pid)
		{
			fence.pid = pid;
			fence.expected_revision = expected_revision;
			return &fence;
		}
	return nullptr;
}

size_t pinned_death_count_locked();

/** Refresh queue depth and high-water counters while pipeline_mutex is held. */
void update_depth_locked()
{
	health.pending_append = pending_append.size() + (append_retry ? 1 : 0);
	health.durable_ready = durable_ready.size();
	health.retained_bytes = retained_bytes;
	health.accepting = accepting;
	health.append_inflight = append_inflight;
	health.high_water_snapshots = std::max(
		health.high_water_snapshots,
		static_cast<uint64_t>(health.pending_append + health.durable_ready +
				      pinned_death_count_locked() + (append_inflight ? 1 : 0)));
	health.high_water_bytes =
		std::max(health.high_water_bytes, static_cast<uint64_t>(retained_bytes));
}

size_t pinned_death_count_locked()
{
	return static_cast<size_t>(std::count_if(terminal_fences.begin(), terminal_fences.end(),
						 [](const terminal_fence &fence)
						 { return fence.death_pinned; }));
}

void clear_terminal_fence_locked(terminal_fence &fence)
{
	if (fence.death_pinned)
	{
		(void)player_revision_unpin_terminal_death(fence.pid, fence.revision);
		if (fence.death_snapshot)
		{
			const size_t bytes = fence.death_snapshot->encoded_size_bound;
			retained_bytes = bytes <= retained_bytes ? retained_bytes - bytes : 0;
		}
	}
	fence = {};
	update_depth_locked();
}

bool death_snapshot_identity_matches(const terminal_fence &fence)
{
	if (!fence.death_snapshot)
		return false;
	const player_snapshot &snapshot = *fence.death_snapshot;
	return player_snapshot_is_death_request_schema(snapshot.schema_version) &&
	       snapshot.revision == fence.revision && snapshot.death &&
	       snapshot.death->operation_id.bytes == fence.operation_id.bytes &&
	       snapshot.death->wallet_pile_uid == fence.wallet_pile_uid &&
	       !snapshot.death->corpse.empty() &&
	       snapshot.death->corpse.front().object_uid == fence.corpse_uid;
}

/** Replay the journal, then append queued snapshots before making them eligible for persistence workers. */
void dispatcher_main()
{
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		health.dispatcher_running = true;
		dispatcher_entry_acknowledged = true;
		append_available.notify_all();
	}
	player_save_journal_result replay = player_save_journal_result::replay_blocked;
#ifndef __NO_MYSQL__
	const bool mysql_ready = sql_worker_thread_init() == 0;
#else
	const bool mysql_ready = true;
#endif
	if (mysql_ready)
		replay = player_save_journal_replay(selected_snapshot_apply(), nullptr);
	std::vector<int> deferred_replay_pids;
	const auto remember_deferred = [&]()
	{
		deferred_replay_pids.clear();
		if (replay != player_save_journal_result::replay_deferred ||
		    !player_save_execution_guard::current_ownership_epoch())
			return;
		try
		{
			std::vector<player_save_journal_retained_frame> frames;
			const auto collected = player_save_journal_collect_retained(&frames);
			if (collected != player_save_journal_result::ok)
			{
				replay = collected;
				return;
			}
			deferred_replay_pids.reserve(frames.size());
			for (const auto &frame : frames)
				if (!frame.quarantined && !frame.policy_fenced)
					deferred_replay_pids.push_back(frame.snapshot.pid);
			std::sort(deferred_replay_pids.begin(), deferred_replay_pids.end());
			deferred_replay_pids.erase(std::unique(deferred_replay_pids.begin(),
							       deferred_replay_pids.end()),
						   deferred_replay_pids.end());
		}
		catch (...)
		{
			deferred_replay_pids.clear();
			replay = player_save_journal_result::replay_blocked;
		}
	};
	remember_deferred();
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		health.replay_complete = replay == player_save_journal_result::ok;
		health.replay_blocked = replay != player_save_journal_result::ok;
		replay_gate.finish_replay(replay == player_save_journal_result::ok &&
					  health.initialized && !stop_requested);
	}

	for (;;)
	{
		const auto epoch = player_save_execution_guard::current_ownership_epoch();
		// Observe before selecting any release/revisit work: a notification
		// arriving after this observation must also prevent the final wait.
		const auto observed = player_save_execution_guard::observe_ownership(epoch, 1);
		if (epoch)
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			if (stop_requested || !observed.available)
			{
				if (!observed.available)
				{
					accepting = false;
					health.replay_complete = false;
					health.replay_blocked = true;
					replay_gate.begin_replay();
					update_depth_locked();
				}
				break;
			}
		}
		if (epoch && replay == player_save_journal_result::replay_deferred)
			for (const int pid : deferred_replay_pids)
			{
				const auto state =
					player_save_execution_guard::observe_ownership(epoch, pid);
				if (state.available && !state.publication_held &&
				    !state.resident_count && !state.executing &&
				    !state.replay_pending && !state.replay_reserved)
				{
					replay_revisit_requested.store(true);
					break;
				}
			}
		if (epoch && mysql_ready && replay == player_save_journal_result::replay_deferred &&
		    replay_revisit_requested.exchange(false))
		{
			// Replay owns per-PID reservations and rereads after acquisition. This
			// is a retained release notice, never an unguarded second replay pass.
			replay = player_save_journal_replay(selected_snapshot_apply(), nullptr);
			remember_deferred();
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			health.replay_complete = replay == player_save_journal_result::ok;
			health.replay_blocked = replay != player_save_journal_result::ok;
			replay_gate.finish_replay(health.replay_complete && health.initialized &&
						  !stop_requested);
		}
		if (epoch && observed.available)
		{
			std::array<player_save_ownership_waiter, PLAYER_SAVE_WORKER_MAX_PIDS>
				waiters;
			const size_t count = player_save_worker_ownership_waiters(waiters.data(),
										  waiters.size());
			for (size_t i = 0; i < count; ++i)
			{
				const auto &waiter = waiters[i];
				const auto state = player_save_execution_guard::observe_ownership(
					epoch, waiter.identity.pid);
				if (waiter.epoch == epoch && state.available && !state.executing &&
				    !state.replay_reserved && !state.publication_held)
					(void)player_save_worker_resume_deferred_exact(
						waiter.identity);
			}
		}
		retained_snapshot retained;
		std::optional<execution_scope> append_scope;
		bool selected = false;
		bool requeue_failed = false;
		{
			std::unique_lock<std::mutex> lock(pipeline_mutex);
			if (epoch && (stop_requested || !observed.available))
			{
				if (!observed.available)
				{
					accepting = false;
					health.replay_blocked = true;
					update_depth_locked();
				}
				break;
			}
			if (append_retry)
			{
				bool retry_allowed = !epoch;
				if (epoch &&
				    append_retry->residence.matches_current(append_retry->body.pid))
				{
					append_scope.emplace(append_retry->residence);
					retry_allowed = append_scope->result() ==
							ownership_status::allowed;
					if (!retry_allowed)
						append_scope.reset();
				}
				if (retry_allowed)
				{
					// Retry native append directly even when deque storage is still
					// unavailable. This preserves the existing inactive OOM path.
					retained = std::move(*append_retry);
					append_retry.reset();
					selected = true;
				}
				else
					try
					{
						pending_append.push_front(std::move(*append_retry));
						append_retry.reset();
					}
					catch (const std::bad_alloc &)
					{
						++health.overloads;
						requeue_failed = true;
					}
			}
			if (!selected && !requeue_failed && !epoch)
			{
				append_available.wait(
					lock,
					[] { return stop_requested || !pending_append.empty(); });
				if (stop_requested && pending_append.empty())
					break;
			}
			if (!selected && !requeue_failed)
			{
				for (auto entry = pending_append.begin();
				     entry != pending_append.end(); ++entry)
				{
					if (epoch)
					{
						if (!entry->residence.matches_current(
							    entry->body.pid))
						{
							accepting = false;
							health.replay_blocked = true;
							break;
						}
						append_scope.emplace(entry->residence);
						const auto admitted = append_scope->result();
						if (admitted != ownership_status::allowed)
						{
							append_scope.reset();
							if (admitted == ownership_status::busy ||
							    admitted == ownership_status::held)
								continue;
							accepting = false;
							health.replay_blocked = true;
							break;
						}
					}
					retained = std::move(*entry);
					pending_append.erase(entry);
					selected = true;
					break;
				}
			}
			if (selected)
			{
				append_inflight = true;
				append_inflight_pid = retained.body.pid;
				append_inflight_revision = retained.body.revision;
			}
			update_depth_locked();
		}
		if (requeue_failed)
		{
			// Existing allocation/I/O backoff, never an ownership-contention retry.
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}
		if (!selected)
		{
			if (epoch)
				player_save_execution_guard::wait_ownership_change(
					epoch, observed.change_sequence);
			continue;
		}
		auto &snapshot = retained.body;
		const player_save_journal_result appended = player_save_journal_append(snapshot);
		persistence_trace_event journal_trace;
		journal_trace.stage = persistence_trace_stage::save_journal;
		journal_trace.pid = snapshot.pid;
		journal_trace.revision = snapshot.revision;
		journal_trace.outcome = static_cast<uint32_t>(appended);
		persistence_trace_record(journal_trace);
		const bool quarantined = appended == player_save_journal_result::quarantined_pid;
		const bool quarantine_preserved =
			quarantined && player_save_journal_archive_quarantined(snapshot) ==
					       player_save_journal_result::ok;
		append_scope.reset();
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			if (quarantine_preserved)
			{
				retained_bytes -= snapshot.encoded_size_bound;
				++health.durable_spills;
			}
			else if (appended == player_save_journal_result::ok)
			{
				try
				{
					durable_ready.push_back(std::move(retained));
					network_wakeup_notify();
					if (terminal_fence *fence = find_terminal_fence_locked(
						    durable_ready.back().body.pid);
					    fence &&
					    fence->revision == durable_ready.back().body.revision)
						fence->journaled = true;
				}
				catch (const std::bad_alloc &)
				{
					retained_bytes -= snapshot.encoded_size_bound;
					++health.durable_spills;
					++health.overloads;
				}
			}
			else
			{
				++health.append_failures;
				try
				{
					if (quarantined)
						pending_append.push_back(std::move(retained));
					else
						pending_append.push_front(std::move(retained));
				}
				catch (const std::bad_alloc &)
				{
					// Retain complete bytes and claim in a stable owner across stop.
					append_retry.emplace(std::move(retained));
					++health.overloads;
				}
			}
			append_inflight = false;
			append_inflight_pid = 0;
			append_inflight_revision = 0;
			update_depth_locked();
		}
		if (appended != player_save_journal_result::ok && !quarantine_preserved)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	health.dispatcher_running = false;
#ifndef __NO_MYSQL__
	if (mysql_ready)
		mysql_thread_end();
#endif
}

/** Check retained queues for an exact player revision while pipeline_mutex is held. */
bool snapshot_is_retained_locked(int pid, player_revision_t revision)
{
	if (append_retry && append_retry->body.pid == pid &&
	    append_retry->body.revision == revision)
		return true;
	for (const auto &retained : pending_append)
		if (retained.body.pid == pid && retained.body.revision == revision)
			return true;
	for (const auto &retained : durable_ready)
		if (retained.body.pid == pid && retained.body.revision == revision)
			return true;
	return false;
}

/** Check retained queues for any snapshot belonging to a player. */
bool any_snapshot_is_retained_locked(int pid)
{
	if (append_retry && append_retry->body.pid == pid)
		return true;
	for (const auto &retained : pending_append)
		if (retained.body.pid == pid)
			return true;
	for (const auto &retained : durable_ready)
		if (retained.body.pid == pid)
			return true;
	return false;
}

bool merge_quest_xp_receipts(player_snapshot *target, const player_snapshot &source)
{
	if (!target || source.quest_xp_receipts.empty())
		return true;
	if (source.death ||
	    (!player_snapshot_is_death_request_schema(target->schema_version) &&
	     target->schema_version != PLAYER_SNAPSHOT_SCHEMA_VERSION &&
	     target->schema_version != PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION &&
	     target->schema_version != PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION &&
	     target->schema_version != PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION))
		return false;
	size_t added = 0;
	for (const auto &candidate : source.quest_xp_receipts)
	{
		auto found = std::find_if(
			target->quest_xp_receipts.begin(), target->quest_xp_receipts.end(),
			[&](const auto &existing)
			{
				return existing.offering_operation.bytes ==
					       candidate.offering_operation.bytes &&
				       existing.reward_index == candidate.reward_index;
			});
		if (found != target->quest_xp_receipts.end())
		{
			if (found->amount != candidate.amount)
				return false;
			continue;
		}
		++added;
	}
	if (!added)
		return true;
	const size_t schema_overhead =
		target->schema_version == PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION ? 8 :
		(target->schema_version == PLAYER_SNAPSHOT_SCHEMA_VERSION ||
		 target->schema_version == PLAYER_SNAPSHOT_DEATH_SPELL_RECEIPT_SCHEMA_VERSION) ?
										 4 :
										 0;
	if (target->quest_xp_receipts.size() + added > 64 ||
	    target->encoded_size_bound > PLAYER_SNAPSHOT_MAX_BYTES ||
	    schema_overhead > PLAYER_SNAPSHOT_MAX_BYTES - target->encoded_size_bound ||
	    added > (PLAYER_SNAPSHOT_MAX_BYTES - target->encoded_size_bound - schema_overhead) / 24)
		return false;
	try
	{
		target->quest_xp_receipts.reserve(target->quest_xp_receipts.size() + added);
		for (const auto &candidate : source.quest_xp_receipts)
		{
			const bool present = std::any_of(
				target->quest_xp_receipts.begin(), target->quest_xp_receipts.end(),
				[&](const auto &existing)
				{
					return existing.offering_operation.bytes ==
						       candidate.offering_operation.bytes &&
					       existing.reward_index == candidate.reward_index;
				});
			if (!present)
				target->quest_xp_receipts.push_back(candidate);
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (target->death && !player_snapshot_has_craft_receipt_schema(target->schema_version))
		target->schema_version = PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION;
	else if (target->schema_version == PLAYER_SNAPSHOT_SCHEMA_VERSION)
		target->schema_version = PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION;
	target->encoded_size_bound += schema_overhead + 24 * added;
	return true;
}

bool merge_spell_effect_receipts(player_snapshot *target, const player_snapshot &source)
{
	if (!target || source.spell_effect_receipts.empty())
		return true;
	if (source.death ||
	    (!player_snapshot_is_death_request_schema(target->schema_version) &&
	     target->schema_version != PLAYER_SNAPSHOT_SCHEMA_VERSION &&
	     target->schema_version != PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION &&
	     target->schema_version != PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION &&
	     target->schema_version != PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION))
		return false;
	size_t added = 0;
	for (const auto &candidate : source.spell_effect_receipts)
	{
		auto found = std::find_if(
			target->spell_effect_receipts.begin(), target->spell_effect_receipts.end(),
			[&](const auto &existing)
			{ return existing.operation_id.bytes == candidate.operation_id.bytes; });
		if (found != target->spell_effect_receipts.end())
		{
			if (found->effect_id != candidate.effect_id)
				return false;
			continue;
		}
		++added;
	}
	if (!added)
		return true;
	const size_t schema_overhead =
		target->schema_version == PLAYER_SNAPSHOT_SCHEMA_VERSION ? 8 :
		target->schema_version == PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION ||
				target->schema_version == PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION ?
									   4 :
									   0;
	if (target->spell_effect_receipts.size() + added > PLAYER_SPELL_EFFECT_RECEIPT_MAX ||
	    target->encoded_size_bound > PLAYER_SNAPSHOT_MAX_BYTES ||
	    schema_overhead > PLAYER_SNAPSHOT_MAX_BYTES - target->encoded_size_bound ||
	    added > (PLAYER_SNAPSHOT_MAX_BYTES - target->encoded_size_bound - schema_overhead) / 20)
		return false;
	try
	{
		target->spell_effect_receipts.reserve(target->spell_effect_receipts.size() + added);
		for (const auto &candidate : source.spell_effect_receipts)
		{
			const bool present =
				std::any_of(target->spell_effect_receipts.begin(),
					    target->spell_effect_receipts.end(),
					    [&](const auto &existing) {
						    return existing.operation_id.bytes ==
							   candidate.operation_id.bytes;
					    });
			if (!present)
				target->spell_effect_receipts.push_back(candidate);
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (!player_snapshot_has_craft_receipt_schema(target->schema_version))
		target->schema_version =
			target->death ?
				(target->quest_xp_receipts.empty() ?
					 PLAYER_SNAPSHOT_DEATH_SPELL_RECEIPT_SCHEMA_VERSION :
					 PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION) :
				PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION;
	target->encoded_size_bound += schema_overhead + 20 * added;
	return true;
}

bool merge_craft_receipts(player_snapshot *target, const player_snapshot &source)
{
	if (!target || source.craft_receipts.empty())
		return true;
	size_t added = 0;
	for (const auto &candidate : source.craft_receipts)
	{
		const auto found = std::find_if(
			target->craft_receipts.begin(), target->craft_receipts.end(),
			[&](const auto &existing)
			{ return existing.operation_id.bytes == candidate.operation_id.bytes; });
		if (found != target->craft_receipts.end())
		{
			if (found->discipline != candidate.discipline ||
			    found->experience != candidate.experience)
				return false;
		}
		else
			++added;
	}
	const size_t overhead =
		player_snapshot_has_craft_receipt_schema(target->schema_version) ?
			0 :
			4 +
				(player_snapshot_has_quest_receipt_schema(target->schema_version) ?
					 0 :
					 4) +
				(player_snapshot_has_spell_receipt_schema(target->schema_version) ?
					 0 :
					 4);
	if (target->craft_receipts.size() + added > PLAYER_CRAFT_RECEIPT_MAX ||
	    (target->components & CRAFT_PROGRESSION_COMPONENTS) != CRAFT_PROGRESSION_COMPONENTS ||
	    target->encoded_size_bound > PLAYER_SNAPSHOT_MAX_BYTES ||
	    overhead > PLAYER_SNAPSHOT_MAX_BYTES - target->encoded_size_bound ||
	    added > (PLAYER_SNAPSHOT_MAX_BYTES - target->encoded_size_bound - overhead) / 24)
		return false;
	try
	{
		target->craft_receipts.reserve(target->craft_receipts.size() + added);
		for (const auto &candidate : source.craft_receipts)
			if (std::none_of(target->craft_receipts.begin(),
					 target->craft_receipts.end(),
					 [&](const auto &existing) {
						 return existing.operation_id.bytes ==
							candidate.operation_id.bytes;
					 }))
				target->craft_receipts.push_back(candidate);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	target->schema_version =
		target->death ? (target->death->conflict_evidence ?
					 PLAYER_SNAPSHOT_DEATH_CRAFT_EVIDENCE_SCHEMA_VERSION :
					 PLAYER_SNAPSHOT_DEATH_CRAFT_RECEIPT_SCHEMA_VERSION) :
				PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION;
	target->encoded_size_bound += overhead + 24 * added;
	return true;
}

/** Admit or coalesce a snapshot within queue and byte limits before notifying the dispatcher. */
player_save_pipeline_result enqueue_snapshot(player_snapshot snapshot, resident_claim residence)
{
	const player_revision_t literal_revision = snapshot.revision;
	if (player_save_journal_pid_quarantined(snapshot.pid))
		return player_save_pipeline_result::unavailable;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (!health.initialized || stop_requested ||
	    find_target_save_login_fence_locked(snapshot.pid))
		return player_save_pipeline_result::unavailable;
	literal_inventory_checkpoint *literal = find_literal_inventory_locked(snapshot.pid);
	if (literal)
	{
		std::vector<uint8_t> blob;
		if (literal->held || !literal_checkpoint_blob(snapshot, *literal, &blob) ||
		    blob != literal->payload)
			return player_save_pipeline_result::capture_failed;
	}
	if (terminal_fence *fence = find_terminal_fence_locked(snapshot.pid);
	    fence && fence->death_pinned)
		return player_save_pipeline_result::unavailable;
	for (auto &retained : pending_append)
	{
		auto &queued = retained.body;
		if (queued.pid != snapshot.pid)
			continue;
		if (snapshot.revision <= queued.revision)
		{
			for (const auto &receipt : snapshot.quest_xp_receipts)
			{
				const auto found = std::find_if(
					queued.quest_xp_receipts.begin(),
					queued.quest_xp_receipts.end(),
					[&](const auto &pending)
					{
						return pending.offering_operation.bytes ==
							       receipt.offering_operation.bytes &&
						       pending.reward_index ==
							       receipt.reward_index &&
						       pending.amount == receipt.amount;
					});
				if (found == queued.quest_xp_receipts.end())
					return player_save_pipeline_result::capture_failed;
			}
			for (const auto &receipt : snapshot.spell_effect_receipts)
			{
				const auto found = std::find_if(
					queued.spell_effect_receipts.begin(),
					queued.spell_effect_receipts.end(),
					[&](const auto &pending)
					{
						return pending.operation_id.bytes ==
							       receipt.operation_id.bytes &&
						       pending.effect_id == receipt.effect_id;
					});
				if (found == queued.spell_effect_receipts.end())
					return player_save_pipeline_result::capture_failed;
			}
			for (const auto &receipt : snapshot.craft_receipts)
				if (std::none_of(
					    queued.craft_receipts.begin(),
					    queued.craft_receipts.end(),
					    [&](const auto &pending)
					    {
						    return pending.operation_id.bytes ==
								   receipt.operation_id.bytes &&
							   pending.discipline ==
								   receipt.discipline &&
							   pending.experience == receipt.experience;
					    }))
					return player_save_pipeline_result::capture_failed;
			return player_save_pipeline_result::coalesced;
		}
		if (!merge_quest_xp_receipts(&snapshot, queued) ||
		    !merge_spell_effect_receipts(&snapshot, queued) ||
		    !merge_craft_receipts(&snapshot, queued))
			return player_save_pipeline_result::capture_failed;
		if ((snapshot.components & queued.components) != queued.components)
			return player_save_pipeline_result::capture_failed;
		if (snapshot.encoded_size_bound >
		    PLAYER_SAVE_PIPELINE_MAX_BYTES - (retained_bytes - queued.encoded_size_bound))
		{
			++health.overloads;
			return player_save_pipeline_result::overloaded;
		}
		std::vector<uint8_t> flat_checkpoint_body;
		if (!prepare_flat_shop_enqueue_locked(literal, snapshot, &flat_checkpoint_body))
			return player_save_pipeline_result::capture_failed;
		retained_bytes =
			retained_bytes - queued.encoded_size_bound + snapshot.encoded_size_bound;
		retained = retained_snapshot(std::move(snapshot), std::move(residence));
		if (literal && (literal->profile == literal_checkpoint_profile::flat_shop ||
				smith_profile(literal->profile)))
			literal->original_shop_body = std::move(flat_checkpoint_body);
		note_literal_enqueue_locked(literal, literal_revision);
		++health.coalesced;
		update_depth_locked();
		player_save_execution_guard::signal_ownership_change(
			player_save_execution_guard::current_ownership_epoch());
		return player_save_pipeline_result::coalesced;
	}
	if (pending_append.size() + durable_ready.size() + (append_retry ? 1 : 0) +
			    (append_inflight ? 1 : 0) + pinned_death_count_locked() >=
		    PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS ||
	    snapshot.encoded_size_bound > PLAYER_SAVE_PIPELINE_MAX_BYTES - retained_bytes)
	{
		++health.overloads;
		return player_save_pipeline_result::overloaded;
	}
	std::vector<uint8_t> flat_checkpoint_body;
	if (!prepare_flat_shop_enqueue_locked(literal, snapshot, &flat_checkpoint_body))
		return player_save_pipeline_result::capture_failed;
	retained_bytes += snapshot.encoded_size_bound;
	try
	{
		pending_append.emplace_back(std::move(snapshot), std::move(residence));
	}
	catch (const std::bad_alloc &)
	{
		retained_bytes -= snapshot.encoded_size_bound;
		++health.overloads;
		return player_save_pipeline_result::overloaded;
	}
	if (literal && (literal->profile == literal_checkpoint_profile::flat_shop ||
			smith_profile(literal->profile)))
		literal->original_shop_body = std::move(flat_checkpoint_body);
	note_literal_enqueue_locked(literal, literal_revision);
	++health.captured;
	update_depth_locked();
	append_available.notify_one();
	player_save_execution_guard::signal_ownership_change(
		player_save_execution_guard::current_ownership_epoch());
	return player_save_pipeline_result::queued;
}
} // namespace

bool player_save_pipeline_prepare(const char *journal_directory, void (*verify_resolved_recovery)())
{
	std::lock_guard<std::mutex> lifecycle(player_save_pipeline_lifecycle_detail::mutex);
	if (!journal_directory || journal_directory[0] != '/')
		return false;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (health.initialized || shutdown_incomplete || lifecycle_admission_closed)
			return false;
	}
	if (!player_save_execution_guard::begin_registration())
		return false;
	replay_gate.begin_replay();
	if (!player_save_journal_init(journal_directory, PLAYER_SAVE_JOURNAL_MAX_BYTES))
	{
		player_save_execution_guard::end_registration();
		return false;
	}
	try
	{
		if (verify_resolved_recovery)
			verify_resolved_recovery();
	}
	catch (...)
	{
		player_save_journal_shutdown();
		player_save_execution_guard::end_registration();
		return false;
	}
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		health = {};
		health.initialized = true;
		stop_requested = false;
		accepting = false;
		execution_started = false;
		dispatcher_entry_acknowledged = false;
		append_inflight = false;
		append_inflight_pid = 0;
		append_inflight_revision = 0;
		replay_revisit_requested.store(false);
	}
	return true;
}

bool player_save_pipeline_start(void)
{
	std::lock_guard<std::mutex> lifecycle(player_save_pipeline_lifecycle_detail::mutex);
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!health.initialized || stop_requested || execution_started ||
		    dispatcher.joinable())
			return false;
	}
	try
	{
		if (!player_save_worker_init(selected_snapshot_apply(), nullptr))
			return false;
		if (!player_save_worker_set_journal_hooks(nullptr, player_save_journal_worker_ack,
							  nullptr,
							  player_save_journal_worker_terminal))
		{
			player_save_worker_shutdown();
			return false;
		}
		dispatcher = std::thread(dispatcher_main);
	}
	catch (...)
	{
		player_save_worker_shutdown();
		return false;
	}
	{
		std::unique_lock<std::mutex> lock(pipeline_mutex);
		execution_started = true;
		player_save_execution_guard::end_registration();
		accepting = true;
		update_depth_locked();
		// Reuse the original stop notification. A latched entry survives later
		// dispatcher failure; shutdown cancellation also ends this wait.
		append_available.wait(lock,
				      [] {
					      return dispatcher_entry_acknowledged ||
						     stop_requested || !health.initialized;
				      });
		return dispatcher_entry_acknowledged && health.dispatcher_running &&
		       !stop_requested && health.initialized;
	}
}

bool player_save_pipeline_init(const char *journal_directory, void (*verify_resolved_recovery)())
{
	if (!player_save_pipeline_prepare(journal_directory, verify_resolved_recovery))
		return false;
	if (player_save_pipeline_start())
		return true;
	player_save_pipeline_shutdown();
	return false;
}

/** Join the dispatcher and workers, then release retained pipeline state. */
void player_save_pipeline_shutdown(void)
{
	std::lock_guard<std::mutex> lifecycle(player_save_pipeline_lifecycle_detail::mutex);
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		replay_gate.begin_replay();
		stop_requested = true;
		accepting = false;
		append_available.notify_all();
		player_save_execution_guard::signal_ownership_change(
			player_save_execution_guard::current_ownership_epoch());
	}
	if (dispatcher.joinable())
		dispatcher.join();
	player_save_worker_shutdown();
	if (player_save_execution_guard::current_ownership_epoch())
	{
		// Joined workers can still own original slots/results, and the pipeline
		// can own unjournaled bytes and death pins. Until the lifecycle owner
		// proves their durable handoff, preserve journal namespace and all holds.
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		shutdown_incomplete = true;
		execution_started = false;
		update_depth_locked();
		return;
	}
	player_save_journal_shutdown();
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	pending_append.clear();
	durable_ready.clear();
	append_retry.reset();
	for (terminal_fence &fence : terminal_fences)
		clear_terminal_fence_locked(fence);
	target_save_login_fences.fill({});
	shutdown_incomplete = !player_save_execution_guard::discard_quiesced_holds();
	if (!shutdown_incomplete)
		literal_inventory_checkpoints.fill({});
	// A later drain alone must not permit preparation against another journal.
	// Only a successful explicit shutdown retry clears this resident state.
	retained_bytes = 0;
	accepting = false;
	execution_started = false;
	append_inflight = false;
	append_inflight_pid = 0;
	append_inflight_revision = 0;
	health.initialized = false;
	update_depth_locked();
}

/** Mark player components dirty and advance any outstanding terminal fence to the new revision. */
bool player_save_pipeline_mark(int pid, player_component_mask_t components)
{
	/* Equipment and inventory are one custody graph.  Saving either half alone can
	 * delete container descendants or make an exact custody comparison impossible. */
	if (components & (PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY))
		components |= PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY;
	if (player_save_journal_pid_quarantined(pid))
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (!accepting)
		return false;
	if (find_target_save_login_fence_locked(pid))
		return false;
	terminal_fence *existing_fence = find_terminal_fence_locked(pid);
	if (existing_fence && existing_fence->death_pinned)
		return false;
	const bool fenced = existing_fence != nullptr;
	player_revision_t revision = 0;
	// Keep admission and the revision transition under the same pipeline lock
	// as target-fence acquisition. Otherwise a save could mark dirty between
	// the offline preflight and the recipient-only fence.
	if (!player_revision_mark(pid, components, &revision))
		return false;
	if (fenced)
	{
		if (terminal_fence *fence = find_terminal_fence_locked(pid))
		{
			fence->revision = revision;
			fence->journaled = false;
			fence->acknowledged = false;
		}
	}
	++health.marked;
	return true;
}

/** Capture and enqueue pending player components with the supplied save intent and room. */
static player_save_pipeline_result checkpoint_dirty_with_quest_xp(
	P_char ch, int save_intent, int room_vnum, const player_quest_xp_receipt_snapshot *receipts,
	size_t receipt_count,
	const player_spell_effect_receipt_snapshot *spell_effect_receipt = nullptr,
	resident_claim *original_residence = nullptr)
{
	if (!ch || IS_NPC(ch) || GET_PID(ch) <= 0)
		return player_save_pipeline_result::invalid;
	if (player_save_journal_pid_quarantined(GET_PID(ch)))
		return player_save_pipeline_result::unavailable;
	if (IS_SET(ch->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED))
		return player_save_pipeline_result::unavailable;
	// Pending grants must publish before capturing their recipient inventory.
	if (item_movement_transaction_player_creation_busy(ch) ||
	    item_creation_grant_player_publication_pending(ch))
		return player_save_pipeline_result::unavailable;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (find_target_save_login_fence_locked(GET_PID(ch)))
			return player_save_pipeline_result::unavailable;
		if (terminal_fence *fence = find_terminal_fence_locked(GET_PID(ch));
		    fence && fence->death_pinned)
			return player_save_pipeline_result::unavailable;
	}
	uint64_t literal_root_uid = 0;
	bool auction_literal_capture = false;
	std::array<uint64_t, 9> auction_literal_roots{};
	size_t auction_literal_root_count = 0;
	player_component_mask_t literal_required_components = 0;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (auto *literal = find_literal_inventory_locked(GET_PID(ch)))
		{
			if ((literal->profile == literal_checkpoint_profile::flat_shop &&
			     !flat_shop_context_matches(*literal, ch)) ||
			    (literal->profile == literal_checkpoint_profile::flat_smith &&
			     !smith_flat_context_matches(*literal, ch)))
				return player_save_pipeline_result::unavailable;
			if (literal->held || literal->token.actor_runtime_id != ch->runtime_id ||
			    ((literal->profile == literal_checkpoint_profile::shop ||
			      literal->profile == literal_checkpoint_profile::smith ||
			      literal->profile == literal_checkpoint_profile::auction ||
			      literal->profile == literal_checkpoint_profile::native_quest ||
			      literal->profile == literal_checkpoint_profile::held_retirement) &&
			     !economic_gameplay_authority::active_regular_sql()))
				return player_save_pipeline_result::unavailable;
			literal_root_uid = literal->token.root_uid;
			if (literal->profile == literal_checkpoint_profile::auction)
			{
				if (literal->auction_literal_root_count >
				    auction_literal_roots.size())
					return player_save_pipeline_result::unavailable;
				auction_literal_capture = true;
				auction_literal_roots = literal->auction_literal_roots;
				auction_literal_root_count = literal->auction_literal_root_count;
			}
			literal_required_components = literal_checkpoint_components(*literal);
		}
	}
	const uint64_t ownership_epoch = player_save_execution_guard::current_ownership_epoch();
	resident_claim residence;
	if (ownership_epoch)
	{
		residence = original_residence ? std::move(*original_residence) :
						 resident_claim(ownership_epoch, GET_PID(ch));
		if (!residence.matches_current(GET_PID(ch)))
			return player_save_pipeline_result::unavailable;
	}
	player_snapshot pending_effects;
	player_component_mask_t required_components = literal_required_components;
	player_component_mask_t quest_components = 0;
	if (!quest_reward_recovery_pending_save_receipts(
		    GET_PID(ch), &pending_effects.quest_xp_receipts, &quest_components))
		return player_save_pipeline_result::capture_failed;
	required_components |= quest_components;
	if (!spell_component_retirement_pending_save_receipts(
		    GET_PID(ch), &pending_effects.spell_effect_receipts))
		return player_save_pipeline_result::capture_failed;
	if (!craft_progression_pending_save_receipts(GET_PID(ch), &pending_effects.craft_receipts))
		return player_save_pipeline_result::capture_failed;
	if (!pending_effects.craft_receipts.empty())
		required_components |= CRAFT_PROGRESSION_COMPONENTS;
	if (spell_effect_receipt)
	{
		try
		{
			const auto found = std::find_if(
				pending_effects.spell_effect_receipts.begin(),
				pending_effects.spell_effect_receipts.end(),
				[&](const auto &existing) {
					return existing.operation_id.bytes ==
					       spell_effect_receipt->operation_id.bytes;
				});
			if (found != pending_effects.spell_effect_receipts.end())
			{
				if (found->effect_id != spell_effect_receipt->effect_id)
					return player_save_pipeline_result::invalid;
			}
			else
				pending_effects.spell_effect_receipts.push_back(
					*spell_effect_receipt);
		}
		catch (const std::bad_alloc &)
		{
			return player_save_pipeline_result::capture_failed;
		}
	}
	// Applied rewards/effects need their component boundary even when the first
	// checkpoint never queued or this caller only dirtied another component.
	player_revision_snapshot revision = {};
	if (!player_revision_snapshot_copy(GET_PID(ch), &revision))
		return player_save_pipeline_result::unavailable;
	if (!pending_effects.spell_effect_receipts.empty())
		required_components |= PLAYER_COMPONENT_AFFECTS;
	const auto missing_components = required_components & ~revision.dirty_components;
	if (missing_components)
	{
		if (!player_save_pipeline_mark(GET_PID(ch), missing_components) ||
		    !player_revision_snapshot_copy(GET_PID(ch), &revision))
			return player_save_pipeline_result::capture_failed;
	}
	if (!revision.dirty_components)
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (revision.inflight_components || !revision.queued_components ||
		    snapshot_is_retained_locked(GET_PID(ch), revision.queued_revision))
		{
			++health.unchanged;
			return player_save_pipeline_result::unchanged;
		}
	}
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (terminal_fence *fence = find_terminal_fence_locked(GET_PID(ch));
		    fence && fence->death_pinned)
			return player_save_pipeline_result::unavailable;
	}
	player_revision_t queued_revision = 0;
	player_component_mask_t components = 0;
	if (!player_revision_queue(GET_PID(ch), &queued_revision, &components))
		return player_save_pipeline_result::capture_failed;
	player_snapshot snapshot;
	const auto captured =
		auction_literal_capture ?
			player_snapshot_capture_literal_inventory_roots(
				ch, queued_revision, components, save_intent, room_vnum,
				std::span<const uint64_t>(auction_literal_roots.data(),
							  auction_literal_root_count),
				&snapshot) :
		literal_root_uid ?
			player_snapshot_capture_literal_inventory(ch, queued_revision, components,
								  save_intent, room_vnum,
								  literal_root_uid, &snapshot) :
			player_snapshot_capture(ch, queued_revision, components, save_intent,
						room_vnum, &snapshot);
	if (captured != player_snapshot_capture_result::ok)
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		++health.capture_failures;
		return player_save_pipeline_result::capture_failed;
	}
	if (receipt_count)
	{
		if (!receipts || receipt_count > 64 || snapshot.death ||
		    snapshot.encoded_size_bound > PLAYER_SNAPSHOT_MAX_BYTES ||
		    snapshot.encoded_size_bound > PLAYER_SNAPSHOT_MAX_BYTES - 4 ||
		    receipt_count >
			    (PLAYER_SNAPSHOT_MAX_BYTES - snapshot.encoded_size_bound - 4) / 24)
			return player_save_pipeline_result::invalid;
		for (size_t index = 0; index < receipt_count; ++index)
		{
			const auto &receipt = receipts[index];
			const bool empty_operation =
				std::all_of(receipt.offering_operation.bytes.begin(),
					    receipt.offering_operation.bytes.end(),
					    [](uint8_t byte) { return !byte; });
			if (empty_operation || receipt.reward_index >= 64 || !receipt.amount)
				return player_save_pipeline_result::invalid;
			for (size_t prior = 0; prior < index; ++prior)
				if (receipts[prior].offering_operation.bytes ==
					    receipt.offering_operation.bytes &&
				    receipts[prior].reward_index == receipt.reward_index)
					return player_save_pipeline_result::invalid;
		}
		try
		{
			snapshot.quest_xp_receipts.assign(receipts, receipts + receipt_count);
		}
		catch (const std::bad_alloc &)
		{
			return player_save_pipeline_result::capture_failed;
		}
		snapshot.schema_version = PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION;
		snapshot.encoded_size_bound += 4 + 24 * receipt_count;
		std::vector<uint8_t> encoded;
		if (player_snapshot_encode(snapshot, &encoded) != player_snapshot_codec_result::ok)
			return player_save_pipeline_result::capture_failed;
	}
	if (!pending_effects.quest_xp_receipts.empty() ||
	    !pending_effects.spell_effect_receipts.empty() ||
	    !pending_effects.craft_receipts.empty())
	{
		if (!merge_quest_xp_receipts(&snapshot, pending_effects) ||
		    !merge_spell_effect_receipts(&snapshot, pending_effects) ||
		    !merge_craft_receipts(&snapshot, pending_effects))
			return player_save_pipeline_result::capture_failed;
		std::vector<uint8_t> encoded;
		if (player_snapshot_encode(snapshot, &encoded) != player_snapshot_codec_result::ok)
			return player_save_pipeline_result::capture_failed;
	}
	persistence_trace_event capture_trace;
	capture_trace.stage = persistence_trace_stage::save_capture;
	capture_trace.pid = snapshot.pid;
	capture_trace.revision = snapshot.revision;
	capture_trace.components = snapshot.components;
	persistence_trace_record(capture_trace);
	if (trace_player_saves())
		logit(LOG_STATUS,
		      "PLAYER SAVE TRACE: stage=capture mono_us=%llu pid=%d revision=%llu components=%llu intent=%d room=%d",
		      (unsigned long long)persistence_observability_now_usec(), GET_PID(ch),
		      (unsigned long long)queued_revision, (unsigned long long)components,
		      save_intent, room_vnum);
	return enqueue_snapshot(std::move(snapshot), std::move(residence));
}

namespace
{
bool literal_actor_matches(const player_literal_inventory_token &token, P_char actor)
{
	return actor && IS_PC(actor) && actor->only.pc && GET_PID(actor) == token.pid &&
	       actor->runtime_id && actor->runtime_id == token.actor_runtime_id &&
	       find_character_by_runtime_id(token.actor_runtime_id) == actor && token.root_uid &&
	       token.generation && !IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED);
}
}

player_literal_inventory_state
player_save_pipeline_literal_inventory_begin(P_char actor, P_obj root, int room_vnum,
					     player_literal_inventory_token *token_out)
{
#ifdef __NO_MYSQL__
	(void)actor;
	(void)root;
	(void)room_vnum;
	(void)token_out;
	return player_literal_inventory_state::refused;
#else
	if (!actor || !root || !token_out || !IS_PC(actor) || !actor->only.pc ||
	    GET_PID(actor) <= 0 || !actor->runtime_id ||
	    find_character_by_runtime_id(actor->runtime_id) != actor || !root->obj_uid ||
	    !OBJ_CARRIED_BY(root, actor) ||
	    IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED) ||
	    player_save_journal_pid_quarantined(GET_PID(actor)))
		return player_literal_inventory_state::refused;
	player_snapshot captured;
	std::vector<uint8_t> blob;
	if (player_snapshot_capture_literal_inventory(
		    actor, 1, LITERAL_INVENTORY_COMPONENTS, RENT_CRASH, room_vnum, root->obj_uid,
		    &captured) != player_snapshot_capture_result::ok ||
	    !literal_inventory_blob(captured, root->obj_uid, &blob))
		return player_literal_inventory_state::refused;
	player_literal_inventory_token token;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!health.initialized || stop_requested || !accepting ||
		    !health.replay_complete || health.replay_blocked ||
		    find_target_save_login_fence_locked(GET_PID(actor)) ||
		    find_terminal_fence_locked(GET_PID(actor)))
			return player_literal_inventory_state::refused;
		if (auto *existing = find_literal_inventory_locked(GET_PID(actor)))
		{
			if (existing->profile != literal_checkpoint_profile::ordinary_drop ||
			    !literal_actor_matches(existing->token, actor) ||
			    existing->token.root_uid != root->obj_uid ||
			    existing->payload != blob || existing->held)
				return player_literal_inventory_state::refused;
			*token_out = existing->token;
			return player_literal_inventory_state::pending;
		}
		if (literal_inventory_generation == std::numeric_limits<uint64_t>::max())
			return player_literal_inventory_state::refused;
		literal_inventory_checkpoint *slot = nullptr;
		for (auto &candidate : literal_inventory_checkpoints)
			if (!candidate.token.pid)
			{
				slot = &candidate;
				break;
			}
		if (!slot || !literal_inventory_capacity_locked(blob.size()))
			return player_literal_inventory_state::refused;
		token = { GET_PID(actor), actor->runtime_id, root->obj_uid,
			  ++literal_inventory_generation };
		slot->token = token;
		slot->payload = std::move(blob);
	}
	const auto queued = player_save_pipeline_request(actor, LITERAL_INVENTORY_COMPONENTS,
							 RENT_CRASH, room_vnum);
	if (queued != player_save_pipeline_result::queued &&
	    queued != player_save_pipeline_result::coalesced)
	{
		player_save_pipeline_literal_inventory_cancel(token);
		return player_literal_inventory_state::refused;
	}
	*token_out = token;
	return player_literal_inventory_state::pending;
#endif
}

player_literal_inventory_state
player_save_pipeline_literal_inventory_poll(const player_literal_inventory_token &token,
					    P_char actor)
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	return player_literal_inventory_state::refused;
#else
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto *literal = find_literal_inventory_locked(token.pid);
		if (literal && literal->profile != literal_checkpoint_profile::ordinary_drop)
			return player_literal_inventory_state::refused;
	}
	if (!literal_actor_matches(token, actor) || player_save_journal_pid_quarantined(token.pid))
	{
		player_save_pipeline_literal_inventory_cancel(token);
		return player_literal_inventory_state::refused;
	}
	player_snapshot captured;
	std::vector<uint8_t> blob;
	if (player_snapshot_capture_literal_inventory(
		    actor, 1, LITERAL_INVENTORY_COMPONENTS, RENT_CRASH, NOWHERE, token.root_uid,
		    &captured) != player_snapshot_capture_result::ok ||
	    !literal_inventory_blob(captured, token.root_uid, &blob))
	{
		player_save_pipeline_literal_inventory_cancel(token);
		return player_literal_inventory_state::refused;
	}
	if (player_save_worker_pid_pending(token.pid))
		return player_literal_inventory_state::pending;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	if (!literal || literal->profile != literal_checkpoint_profile::ordinary_drop ||
	    literal->token != token || literal->held)
		return player_literal_inventory_state::refused;
	if (literal->payload != blob)
	{
		*literal = {};
		return player_literal_inventory_state::refused;
	}
	player_revision_snapshot revision = {};
	if (!player_revision_snapshot_copy(token.pid, &revision) || revision.overflowed)
		return player_literal_inventory_state::refused;
	if (!literal->captured_revision ||
	    literal->acknowledged_revision != literal->captured_revision ||
	    revision.current_revision != literal->captured_revision ||
	    revision.acknowledged_revision != literal->captured_revision ||
	    revision.dirty_components || revision.unacknowledged_components ||
	    revision.queued_components || revision.inflight_components ||
	    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
		return player_literal_inventory_state::pending;
	return player_literal_inventory_state::database_acknowledged;
#endif
}

bool player_save_pipeline_literal_inventory_hold(const player_literal_inventory_token &token,
						 const critical_operation_id &operation_id)
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)operation_id;
	return false;
#else
	if (std::all_of(operation_id.bytes.begin(), operation_id.bytes.end(),
			[](uint8_t byte) { return !byte; }) ||
	    player_save_pipeline_literal_inventory_poll(
		    token, find_character_by_runtime_id(token.actor_runtime_id)) !=
		    player_literal_inventory_state::database_acknowledged ||
	    player_save_worker_pid_pending(token.pid))
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	player_revision_snapshot revision = {};
	if (!health.initialized || stop_requested || !accepting || !health.replay_complete ||
	    health.replay_blocked || find_terminal_fence_locked(token.pid) ||
	    find_target_save_login_fence_locked(token.pid) || !literal ||
	    literal->profile != literal_checkpoint_profile::ordinary_drop ||
	    literal->token != token || literal->held || !literal->captured_revision ||
	    literal->acknowledged_revision != literal->captured_revision ||
	    !player_revision_snapshot_copy(token.pid, &revision) || revision.overflowed ||
	    revision.current_revision != literal->captured_revision ||
	    revision.acknowledged_revision != literal->captured_revision ||
	    revision.dirty_components || revision.unacknowledged_components ||
	    revision.queued_components || revision.inflight_components ||
	    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
		return false;
	literal->operation_id = operation_id;
	literal->held = true;
	return true;
#endif
}

bool player_save_pipeline_literal_inventory_release(const player_literal_inventory_token &token,
						    const critical_operation_id &operation_id)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	if (!literal || literal->profile != literal_checkpoint_profile::ordinary_drop ||
	    literal->token != token || !literal->held || literal->restored_sql_drop ||
	    literal->operation_id.bytes != operation_id.bytes)
		return false;
	*literal = {};
	return true;
}

bool player_save_pipeline_literal_inventory_cancel(const player_literal_inventory_token &token)
{
	if (token.pid <= 0 || !token.actor_runtime_id || !token.root_uid || !token.generation)
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	if (!literal || literal->profile != literal_checkpoint_profile::ordinary_drop ||
	    literal->token != token || literal->held)
		return false;
	*literal = {};
	return true;
}

namespace
{
player_literal_inventory_token shop_checkpoint_identity(const player_shop_checkpoint_token &token)
{
	return { token.pid, token.actor_runtime_id, token.root_uid, token.generation };
}

bool shop_actor_matches(const player_shop_checkpoint_token &token, P_char actor)
{
	return actor && IS_PC(actor) && actor->only.pc && token.pid > 0 &&
	       GET_PID(actor) == token.pid && token.actor_runtime_id && token.generation &&
	       actor->runtime_id == token.actor_runtime_id &&
	       find_character_by_runtime_id(token.actor_runtime_id) == actor &&
	       !IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED);
}

bool literal_checkpoint_actor_matches(const literal_inventory_checkpoint &checkpoint, P_char actor)
{
	if (checkpoint.profile == literal_checkpoint_profile::flat_shop)
		return flat_shop_context_matches(checkpoint, actor);
	if (checkpoint.profile == literal_checkpoint_profile::flat_smith)
		return smith_flat_context_matches(checkpoint, actor);
	if (checkpoint.profile == literal_checkpoint_profile::native_quest ||
	    checkpoint.profile == literal_checkpoint_profile::held_retirement)
		return economic_gameplay_authority::active_regular_sql() && actor && IS_PC(actor) &&
		       actor->only.pc && GET_PID(actor) == checkpoint.token.pid &&
		       actor->runtime_id == checkpoint.token.actor_runtime_id &&
		       find_character_by_runtime_id(checkpoint.token.actor_runtime_id) == actor &&
		       !IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED);
	if (checkpoint.profile == literal_checkpoint_profile::ordinary_drop)
		return literal_actor_matches(checkpoint.token, actor);
	return economic_gameplay_authority::active_regular_sql() &&
	       shop_actor_matches({ checkpoint.token.pid, checkpoint.token.actor_runtime_id,
				    checkpoint.token.root_uid, checkpoint.token.generation },
				  actor);
}

}

player_literal_inventory_state
player_save_pipeline_shop_checkpoint_begin(P_char actor, P_obj root, int room_vnum,
					   player_shop_checkpoint_token *token_out)
{
#ifdef __NO_MYSQL__
	(void)actor;
	(void)root;
	(void)room_vnum;
	(void)token_out;
	return player_literal_inventory_state::refused;
#else
	if (!actor || !token_out || !IS_PC(actor) || !actor->only.pc || GET_PID(actor) <= 0 ||
	    !actor->runtime_id || find_character_by_runtime_id(actor->runtime_id) != actor ||
	    !economic_gameplay_authority::active_regular_sql() ||
	    (root && (!root->obj_uid || !OBJ_CARRIED_BY(root, actor))) ||
	    IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED) ||
	    player_save_journal_pid_quarantined(GET_PID(actor)))
		return player_literal_inventory_state::refused;
	const uint64_t root_uid = root ? root->obj_uid : 0;
	player_snapshot captured;
	std::vector<uint8_t> blob;
	uint32_t level = 0;
	if (player_snapshot_capture_literal_inventory(actor, 1, SHOP_CHECKPOINT_COMPONENTS,
						      RENT_CRASH, room_vnum, root_uid, &captured) !=
		    player_snapshot_capture_result::ok ||
	    !shop_checkpoint_blob(captured, &blob, &level))
		return player_literal_inventory_state::refused;
	player_shop_checkpoint_token token;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!economic_gameplay_authority::active_regular_sql() || !health.initialized ||
		    stop_requested || !accepting || !health.replay_complete ||
		    health.replay_blocked || find_target_save_login_fence_locked(GET_PID(actor)) ||
		    find_terminal_fence_locked(GET_PID(actor)))
			return player_literal_inventory_state::refused;
		if (auto *existing = find_literal_inventory_locked(GET_PID(actor)))
		{
			if (existing->profile != literal_checkpoint_profile::shop ||
			    existing->token.actor_runtime_id != actor->runtime_id ||
			    existing->token.root_uid != root_uid || existing->payload != blob ||
			    existing->held)
				return player_literal_inventory_state::refused;
			*token_out = { existing->token.pid, existing->token.actor_runtime_id,
				       existing->token.root_uid, existing->token.generation };
			return player_literal_inventory_state::pending;
		}
		if (literal_inventory_generation == std::numeric_limits<uint64_t>::max())
			return player_literal_inventory_state::refused;
		literal_inventory_checkpoint *slot = nullptr;
		for (auto &candidate : literal_inventory_checkpoints)
			if (!candidate.token.pid)
			{
				slot = &candidate;
				break;
			}
		if (!slot || !literal_inventory_capacity_locked(blob.size()))
			return player_literal_inventory_state::refused;
		token = { GET_PID(actor), actor->runtime_id, root_uid,
			  ++literal_inventory_generation };
		slot->profile = literal_checkpoint_profile::shop;
		slot->level = level;
		slot->token = shop_checkpoint_identity(token);
		slot->payload = std::move(blob);
	}
	// Make the installed original slot recoverable before enqueue can throw.
	*token_out = token;
	player_save_pipeline_result queued;
	try
	{
		queued = player_save_pipeline_request(actor, SHOP_CHECKPOINT_COMPONENTS, RENT_CRASH,
						      room_vnum);
	}
	catch (...)
	{
		// Queue outcome is unresolved. Original token and frozen save policy remain.
		return player_literal_inventory_state::pending;
	}
	if (queued != player_save_pipeline_result::queued &&
	    queued != player_save_pipeline_result::coalesced)
	{
		player_save_pipeline_shop_checkpoint_cancel(token);
		return player_literal_inventory_state::refused;
	}
	*token_out = token;
	return player_literal_inventory_state::pending;
#endif
}

player_literal_inventory_state
player_save_pipeline_shop_checkpoint_poll(const player_shop_checkpoint_token &token, P_char actor,
					  player_shop_checkpoint_stage *stage_out)
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)stage_out;
	return player_literal_inventory_state::refused;
#else
	const auto identity = shop_checkpoint_identity(token);
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto *literal = find_literal_inventory_locked(token.pid);
		if (!literal || literal->profile != literal_checkpoint_profile::shop ||
		    literal->token != identity || literal->held)
			return player_literal_inventory_state::refused;
	}
	if (!economic_gameplay_authority::active_regular_sql() ||
	    !shop_actor_matches(token, actor) || player_save_journal_pid_quarantined(token.pid))
	{
		player_save_pipeline_shop_checkpoint_cancel(token);
		return player_literal_inventory_state::refused;
	}
	player_snapshot captured;
	std::vector<uint8_t> blob;
	if (player_snapshot_capture_literal_inventory(
		    actor, 1, SHOP_CHECKPOINT_COMPONENTS, RENT_CRASH, NOWHERE, token.root_uid,
		    &captured) != player_snapshot_capture_result::ok ||
	    !shop_checkpoint_blob(captured, &blob))
	{
		player_save_pipeline_shop_checkpoint_cancel(token);
		return player_literal_inventory_state::refused;
	}
	if (player_save_worker_pid_pending(token.pid))
		return player_literal_inventory_state::pending;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	if (!literal || literal->profile != literal_checkpoint_profile::shop ||
	    literal->token != identity || literal->held)
		return player_literal_inventory_state::refused;
	if (literal->payload != blob)
	{
		*literal = {};
		return player_literal_inventory_state::refused;
	}
	player_revision_snapshot revision = {};
	if (!player_revision_snapshot_copy(token.pid, &revision) || revision.overflowed)
		return player_literal_inventory_state::refused;
	if (!literal->captured_revision ||
	    literal->acknowledged_revision != literal->captured_revision ||
	    revision.current_revision != literal->captured_revision ||
	    revision.acknowledged_revision != literal->captured_revision ||
	    revision.dirty_components || revision.unacknowledged_components ||
	    revision.queued_components || revision.inflight_components ||
	    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
		return player_literal_inventory_state::pending;
	if (stage_out)
		*stage_out = { literal->acknowledged_revision, literal->level };
	return player_literal_inventory_state::database_acknowledged;
#endif
}

bool player_save_pipeline_shop_checkpoint_hold(const player_shop_checkpoint_token &token,
					       const critical_operation_id &operation_id)
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)operation_id;
	return false;
#else
	if (std::all_of(operation_id.bytes.begin(), operation_id.bytes.end(),
			[](uint8_t byte) { return !byte; }) ||
	    player_save_pipeline_shop_checkpoint_poll(
		    token, find_character_by_runtime_id(token.actor_runtime_id)) !=
		    player_literal_inventory_state::database_acknowledged ||
	    player_save_worker_pid_pending(token.pid))
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	player_revision_snapshot revision = {};
	if (!economic_gameplay_authority::active_regular_sql() || !health.initialized ||
	    stop_requested || !accepting || !health.replay_complete || health.replay_blocked ||
	    find_terminal_fence_locked(token.pid) ||
	    find_target_save_login_fence_locked(token.pid) || !literal ||
	    literal->profile != literal_checkpoint_profile::shop ||
	    literal->token != shop_checkpoint_identity(token) || literal->held ||
	    !literal->captured_revision ||
	    literal->acknowledged_revision != literal->captured_revision ||
	    !player_revision_snapshot_copy(token.pid, &revision) || revision.overflowed ||
	    revision.current_revision != literal->captured_revision ||
	    revision.acknowledged_revision != literal->captured_revision ||
	    revision.dirty_components || revision.unacknowledged_components ||
	    revision.queued_components || revision.inflight_components ||
	    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
		return false;
	literal->operation_id = operation_id;
	literal->held = true;
	return true;
#endif
}

bool player_save_pipeline_shop_checkpoint_release(const player_shop_checkpoint_token &token,
						  const critical_operation_id &operation_id)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	if (!literal || literal->profile != literal_checkpoint_profile::shop ||
	    literal->token != shop_checkpoint_identity(token) || !literal->held ||
	    literal->restored_sql_drop || literal->operation_id.bytes != operation_id.bytes)
		return false;
	*literal = {};
	return true;
}

bool player_save_pipeline_shop_checkpoint_cancel(const player_shop_checkpoint_token &token)
{
	if (token.pid <= 0 || !token.actor_runtime_id || !token.generation)
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	// Poll may already have cancelled this unheld checkpoint on a changed body.
	// Absence consumes no save/publication reservation and grants no ACK authority.
	if (!literal)
		return true;
	if (literal->profile != literal_checkpoint_profile::shop ||
	    literal->token != shop_checkpoint_identity(token) || literal->held)
		return false;
	*literal = {};
	return true;
}

bool player_save_shop_checkpoint_owner::observe_held(
	const player_shop_checkpoint_token &token, P_char actor,
	const critical_operation_id &operation, player_shop_checkpoint_stage *output,
	std::vector<player_item_snapshot> *items_out) noexcept
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)operation;
	(void)output;
	(void)items_out;
	return false;
#else
	if (!output || !nevent_is_game_thread() ||
	    !economic_gameplay_authority::active_regular_sql() ||
	    !shop_actor_matches(token, actor) || player_save_journal_pid_quarantined(token.pid) ||
	    player_save_worker_pid_pending(token.pid))
		return false;
	try
	{
		player_snapshot captured;
		std::vector<uint8_t> blob;
		uint32_t level = 0;
		if (player_snapshot_capture_literal_inventory(
			    actor, 1, SHOP_CHECKPOINT_COMPONENTS, RENT_CRASH, NOWHERE,
			    token.root_uid, &captured) != player_snapshot_capture_result::ok ||
		    !shop_checkpoint_blob(captured, &blob, &level))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto *slot = find_literal_inventory_locked(token.pid);
		if (!health.initialized || stop_requested || !accepting ||
		    !health.replay_complete || health.replay_blocked ||
		    find_terminal_fence_locked(token.pid) ||
		    find_target_save_login_fence_locked(token.pid) || !slot ||
		    slot->profile != literal_checkpoint_profile::shop ||
		    slot->token != shop_checkpoint_identity(token) || !slot->held ||
		    slot->restored_sql_drop || slot->operation_id.bytes != operation.bytes ||
		    slot->payload != blob || slot->level != level || !slot->captured_revision ||
		    slot->acknowledged_revision != slot->captured_revision ||
		    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
			return false;
		// Unrelated dirty STATUS marks may advance while the native operation is held.
		// Frozen inventory/level and the actual acknowledged save revision must match.
		// This exact current body was compared to the original held slot.
		// Copying values exposes no execution hold or ACK capability.
		if (items_out)
			*items_out = std::move(captured.items);
		*output = { slot->acknowledged_revision, slot->level };
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

namespace
{
player_literal_inventory_token smith_checkpoint_identity(const player_smith_checkpoint_token &token)
{
	return { token.pid, token.actor_runtime_id, token.root_uid, token.generation };
}
bool smith_actor_matches(const player_smith_checkpoint_token &token, P_char actor)
{
	// Resolve the original generation before any remembered actor dereference.
	return !token.root_uid && token.actor_runtime_id &&
	       find_character_by_runtime_id(token.actor_runtime_id) == actor &&
	       shop_actor_matches({ token.pid, token.actor_runtime_id, 0, token.generation },
				  actor) &&
	       !actor->only.pc->load_degraded_components;
}
bool smith_profile(literal_checkpoint_profile p) noexcept
{
	return p == literal_checkpoint_profile::smith ||
	       p == literal_checkpoint_profile::flat_smith;
}
bool smith_flat_source_cut(P_char actor, std::string *root,
			   economic_native_money_checkpoint_projection *mapping,
			   uint64_t *ownership_epoch) noexcept
{
	try
	{
		if (!actor || !root || !mapping || !ownership_epoch || !nevent_is_game_thread() ||
		    !IS_PC(actor) || !actor->only.pc || GET_PID(actor) <= 0 || !actor->runtime_id ||
		    find_character_by_runtime_id(actor->runtime_id) != actor ||
		    IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED) ||
		    actor->only.pc->load_degraded_components ||
		    !economic_gameplay_authority::active_regular_flat() ||
		    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY ||
		    persistence_mode_requires_mysql() ||
		    selected_snapshot_apply() != flatfile_player_snapshot_apply_selected)
			return false;
		const char *selected = persistence_mode_flatfile_root(),
			   *account = get_account_name_safe(actor);
		const int racewar = GET_RACEWAR(actor);
		const uint64_t epoch = player_save_execution_guard::current_ownership_epoch();
		if (!selected || !*selected || !account || !*account || racewar < 0 ||
		    racewar > INT8_MAX || !epoch)
			return false;
		std::string selected_root(selected);
		economic_native_money_checkpoint_projection actual{};
		if (!economic_gameplay_authority::observe_craft_wallet_checkpoint(GET_PID(actor),
										  &actual) ||
		    !persistence_mode_flatfile_root() ||
		    selected_root != persistence_mode_flatfile_root() ||
		    player_save_execution_guard::current_ownership_epoch() != epoch)
			return false;
		*root = std::move(selected_root);
		*mapping = actual;
		*ownership_epoch = epoch;
		return true;
	}
	catch (...)
	{
		return false;
	}
}
bool smith_flat_context_matches(const literal_inventory_checkpoint &slot, P_char actor) noexcept
{
	try
	{
		if (slot.profile != literal_checkpoint_profile::flat_smith ||
		    find_character_by_runtime_id(slot.token.actor_runtime_id) != actor ||
		    !smith_actor_matches({ slot.token.pid, slot.token.actor_runtime_id,
					   slot.token.root_uid, slot.token.generation },
					 actor))
			return false;
		const char *account = get_account_name_safe(actor);
		std::string root;
		economic_native_money_checkpoint_projection mapping{};
		uint64_t epoch = 0;
		return account && slot.flat_shop_account == account &&
		       GET_RACEWAR(actor) == slot.flat_shop_racewar &&
		       smith_flat_source_cut(actor, &root, &mapping, &epoch) &&
		       slot.flat_shop_root == root && slot.flat_shop_ownership_epoch == epoch &&
		       slot.flat_shop_mapping.lineage.bytes == mapping.lineage.bytes &&
		       slot.flat_shop_mapping.epoch.bytes == mapping.epoch.bytes &&
		       economic_account_key_equal(slot.flat_shop_mapping.wallet,
						  mapping.player_wallet);
	}
	catch (...)
	{
		return false;
	}
}
bool smith_checkpoint_mode_matches(const literal_inventory_checkpoint &slot, P_char actor) noexcept
{
	return slot.profile == literal_checkpoint_profile::smith ?
		       economic_gameplay_authority::active_regular_sql() :
		       smith_flat_context_matches(slot, actor);
}
bool smith_checkpoint_pipeline_ready_locked(const literal_inventory_checkpoint &slot) noexcept
{
	return health.initialized && !stop_requested && accepting && health.replay_complete &&
	       !health.replay_blocked && !find_terminal_fence_locked(slot.token.pid) &&
	       !find_target_save_login_fence_locked(slot.token.pid) &&
	       (slot.profile != literal_checkpoint_profile::flat_smith || execution_started);
}
}

player_literal_inventory_state
player_save_smith_checkpoint_owner::begin(P_char actor, uint64_t original_runtime, int room_vnum,
					  player_smith_checkpoint_token *token_out)
{
	P_obj root = nullptr; // Smith consumes NPC stock, never a PC carried root.

	if (!nevent_is_game_thread() || !actor || !token_out || !original_runtime ||
	    find_character_by_runtime_id(original_runtime) != actor ||
	    actor->runtime_id != original_runtime || !IS_PC(actor) || !actor->only.pc ||
	    GET_PID(actor) <= 0 || !actor->runtime_id ||
	    find_character_by_runtime_id(actor->runtime_id) != actor ||
	    (!economic_gameplay_authority::active_regular_sql() &&
	     !economic_gameplay_authority::active_regular_flat()) ||
	    (root && (!root->obj_uid || !OBJ_CARRIED_BY(root, actor))) ||
	    IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED) ||
	    player_save_journal_pid_quarantined(GET_PID(actor)))
		return player_literal_inventory_state::refused;
	const uint64_t root_uid = root ? root->obj_uid : 0;
	player_snapshot captured;
	std::vector<uint8_t> blob;
	uint32_t level = 0;
	if (player_snapshot_capture_literal_inventory(actor, 1, SHOP_CHECKPOINT_COMPONENTS,
						      RENT_CRASH, room_vnum, root_uid, &captured) !=
		    player_snapshot_capture_result::ok ||
	    !shop_checkpoint_blob(captured, &blob, &level))
		return player_literal_inventory_state::refused;
	literal_inventory_checkpoint prepared;
	prepared.profile = economic_gameplay_authority::active_regular_sql() ?
				   literal_checkpoint_profile::smith :
				   literal_checkpoint_profile::flat_smith;
	prepared.level = level;
	prepared.payload = std::move(blob);
	if (prepared.profile == literal_checkpoint_profile::flat_smith)
	{
		economic_native_money_checkpoint_projection mapping{};
		if (!smith_flat_source_cut(actor, &prepared.flat_shop_root, &mapping,
					   &prepared.flat_shop_ownership_epoch))
			return player_literal_inventory_state::refused;
		try
		{
			prepared.flat_shop_account = get_account_name_safe(actor);
		}
		catch (...)
		{
			return player_literal_inventory_state::refused;
		}
		prepared.flat_shop_racewar = static_cast<uint8_t>(GET_RACEWAR(actor));
		// Existing cut storage holds only the genuine Smith wallet. No bank authority.
		prepared.flat_shop_mapping = {
			mapping.lineage, mapping.epoch, mapping.player_wallet, {}
		};
	}
	player_smith_checkpoint_token token;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if ((prepared.profile == literal_checkpoint_profile::smith ?
			     !economic_gameplay_authority::active_regular_sql() :
			     (!economic_gameplay_authority::active_regular_flat() ||
			      !execution_started)) ||
		    !health.initialized || stop_requested || !accepting ||
		    !health.replay_complete || health.replay_blocked ||
		    find_target_save_login_fence_locked(GET_PID(actor)) ||
		    find_terminal_fence_locked(GET_PID(actor)))
			return player_literal_inventory_state::refused;
		if (auto *existing = find_literal_inventory_locked(GET_PID(actor)))
		{
			if (existing->profile != prepared.profile ||
			    !smith_checkpoint_mode_matches(*existing, actor) ||
			    existing->token.actor_runtime_id != actor->runtime_id ||
			    existing->token.root_uid != root_uid ||
			    existing->payload != prepared.payload || existing->held)
				return player_literal_inventory_state::refused;
			*token_out = { existing->token.pid, existing->token.actor_runtime_id,
				       existing->token.root_uid, existing->token.generation };
			return player_literal_inventory_state::pending;
		}
		if (literal_inventory_generation == std::numeric_limits<uint64_t>::max())
			return player_literal_inventory_state::refused;
		literal_inventory_checkpoint *slot = nullptr;
		for (auto &candidate : literal_inventory_checkpoints)
			if (!candidate.token.pid)
			{
				slot = &candidate;
				break;
			}
		size_t bytes = prepared.payload.size();
		for (size_t amount : { prepared.profile == literal_checkpoint_profile::flat_smith ?
					       prepared.flat_shop_root.capacity() :
					       prepared.flat_shop_root.size(),
				       prepared.profile == literal_checkpoint_profile::flat_smith ?
					       prepared.flat_shop_account.capacity() :
					       prepared.flat_shop_account.size() })
		{
			if (bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
			    amount > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
				return player_literal_inventory_state::refused;
			bytes += amount;
		}
		if (!slot || !literal_inventory_capacity_locked(bytes))
			return player_literal_inventory_state::refused;
		token = { GET_PID(actor), actor->runtime_id, root_uid,
			  literal_inventory_generation + 1 };
		prepared.token = smith_checkpoint_identity(token);
		if (!smith_checkpoint_mode_matches(prepared, actor))
			return player_literal_inventory_state::refused;
		static_assert(std::is_nothrow_move_assignable_v<literal_inventory_checkpoint>);
		*slot = std::move(prepared);
		++literal_inventory_generation;
	}
	// Make the installed original slot recoverable before enqueue can throw.
	*token_out = token;
	player_save_pipeline_result queued;
	try
	{
		queued = player_save_pipeline_request(actor, SHOP_CHECKPOINT_COMPONENTS, RENT_CRASH,
						      room_vnum);
	}
	catch (...)
	{
		// Queue outcome is unresolved. Original token and frozen save policy remain.
		return player_literal_inventory_state::pending;
	}
	if (queued != player_save_pipeline_result::queued &&
	    queued != player_save_pipeline_result::coalesced)
	{
		player_save_smith_checkpoint_owner::cancel(token);
		return player_literal_inventory_state::refused;
	}
	*token_out = token;
	return player_literal_inventory_state::pending;
}

player_literal_inventory_state
player_save_smith_checkpoint_owner::poll(const player_smith_checkpoint_token &token, P_char actor,
					 player_smith_checkpoint_stage *stage_out)
{
	if (!nevent_is_game_thread() || token.root_uid)
		return player_literal_inventory_state::refused;
	const auto identity = smith_checkpoint_identity(token);
	bool flat = false;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto *literal = find_literal_inventory_locked(token.pid);
		if (!literal || !smith_profile(literal->profile) || literal->token != identity ||
		    literal->held ||
		    (literal->profile == literal_checkpoint_profile::flat_smith &&
		     (!smith_checkpoint_pipeline_ready_locked(*literal) ||
		      !smith_flat_context_matches(*literal, actor))))
			return player_literal_inventory_state::refused;
		flat = literal->profile == literal_checkpoint_profile::flat_smith;
	}
	if ((flat ? !economic_gameplay_authority::active_regular_flat() :
		    !economic_gameplay_authority::active_regular_sql()) ||
	    !smith_actor_matches(token, actor) || player_save_journal_pid_quarantined(token.pid))
	{
		player_save_smith_checkpoint_owner::cancel(token);
		return player_literal_inventory_state::refused;
	}
	player_snapshot captured;
	std::vector<uint8_t> blob;
	if (player_snapshot_capture_literal_inventory(
		    actor, 1, SHOP_CHECKPOINT_COMPONENTS, RENT_CRASH, NOWHERE, token.root_uid,
		    &captured) != player_snapshot_capture_result::ok ||
	    !shop_checkpoint_blob(captured, &blob))
	{
		player_save_smith_checkpoint_owner::cancel(token);
		return player_literal_inventory_state::refused;
	}
	if (player_save_worker_pid_pending(token.pid))
		return player_literal_inventory_state::pending;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	if (!literal || !smith_profile(literal->profile) || literal->token != identity ||
	    literal->held || !smith_checkpoint_mode_matches(*literal, actor) ||
	    (literal->profile == literal_checkpoint_profile::flat_smith &&
	     !smith_checkpoint_pipeline_ready_locked(*literal)))
		return player_literal_inventory_state::refused;
	if (literal->payload != blob)
	{
		*literal = {};
		return player_literal_inventory_state::refused;
	}
	player_revision_snapshot revision = {};
	if (!player_revision_snapshot_copy(token.pid, &revision) || revision.overflowed)
		return player_literal_inventory_state::refused;
	if (literal->original_shop_body.empty() || !literal->captured_revision ||
	    literal->acknowledged_revision != literal->captured_revision ||
	    revision.current_revision != literal->captured_revision ||
	    revision.acknowledged_revision != literal->captured_revision ||
	    revision.dirty_components || revision.unacknowledged_components ||
	    revision.queued_components || revision.inflight_components ||
	    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
		return player_literal_inventory_state::pending;
	if (stage_out)
		*stage_out = { literal->acknowledged_revision, literal->level };
	return player_literal_inventory_state::database_acknowledged;
}

bool player_save_smith_checkpoint_owner::hold(const player_smith_checkpoint_token &token,
					      const critical_operation_id &operation_id)
{
	if (!nevent_is_game_thread() || token.root_uid ||
	    std::all_of(operation_id.bytes.begin(), operation_id.bytes.end(),
			[](uint8_t byte) { return !byte; }) ||
	    player_save_smith_checkpoint_owner::poll(
		    token, find_character_by_runtime_id(token.actor_runtime_id)) !=
		    player_literal_inventory_state::database_acknowledged ||
	    player_save_worker_pid_pending(token.pid))
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	player_revision_snapshot revision = {};
	if (!literal ||
	    !smith_checkpoint_mode_matches(*literal,
					   find_character_by_runtime_id(token.actor_runtime_id)) ||
	    !smith_checkpoint_pipeline_ready_locked(*literal) || !health.initialized ||
	    stop_requested || !accepting || !health.replay_complete || health.replay_blocked ||
	    find_terminal_fence_locked(token.pid) ||
	    find_target_save_login_fence_locked(token.pid) || !literal ||
	    !smith_profile(literal->profile) ||
	    literal->token != smith_checkpoint_identity(token) || literal->held ||
	    !literal->captured_revision ||
	    literal->acknowledged_revision != literal->captured_revision ||
	    !player_revision_snapshot_copy(token.pid, &revision) || revision.overflowed ||
	    revision.current_revision != literal->captured_revision ||
	    revision.acknowledged_revision != literal->captured_revision ||
	    revision.dirty_components || revision.unacknowledged_components ||
	    revision.queued_components || revision.inflight_components ||
	    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
		return false;
	if (literal->profile == literal_checkpoint_profile::flat_smith)
	{
		uint64_t generation = 0;
		if (!player_save_execution_guard::install_live_publication_hold(
			    token.pid, operation_id, &generation))
			return false;
		// No fallible work follows the real original operation exclusion.
		literal->execution_hold_generation = generation;
	}
	literal->operation_id = operation_id;
	literal->held = true;
	return true;
}

bool player_save_smith_checkpoint_owner::cancel(const player_smith_checkpoint_token &token)
{
	if (!nevent_is_game_thread() || token.root_uid || token.pid <= 0 ||
	    !token.actor_runtime_id || !token.generation)
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	// Poll may already have cancelled this unheld checkpoint on a changed body.
	// Absence consumes no save/publication reservation and grants no ACK authority.
	if (!literal)
		return true;
	if (!smith_profile(literal->profile) ||
	    literal->token != smith_checkpoint_identity(token) || literal->held ||
	    literal->execution_hold_generation)
		return false;
	*literal = {};
	return true;
}

bool player_save_smith_checkpoint_owner::observe_held(
	const player_smith_checkpoint_token &token, P_char actor,
	const critical_operation_id &operation, player_smith_checkpoint_stage *output,
	std::vector<player_item_snapshot> *items_out, player_snapshot *original_acknowledged_body,
	player_smith_flat_checkpoint_cut *flat_cut_out,
	economic_native_money_checkpoint_projection *flat_wallet_out) noexcept
{
	if (!output || !nevent_is_game_thread() || token.root_uid ||
	    (!economic_gameplay_authority::active_regular_sql() &&
	     !economic_gameplay_authority::active_regular_flat()) ||
	    !smith_actor_matches(token, actor) || player_save_journal_pid_quarantined(token.pid) ||
	    player_save_worker_pid_pending(token.pid))
		return false;
	try
	{
		player_snapshot captured;
		std::vector<uint8_t> blob;
		uint32_t level = 0;
		if (player_snapshot_capture_literal_inventory(
			    actor, 1, SHOP_CHECKPOINT_COMPONENTS, RENT_CRASH, NOWHERE,
			    token.root_uid, &captured) != player_snapshot_capture_result::ok ||
		    !shop_checkpoint_blob(captured, &blob, &level))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto *slot = find_literal_inventory_locked(token.pid);
		if (!health.initialized || stop_requested || !accepting ||
		    !health.replay_complete || health.replay_blocked ||
		    find_terminal_fence_locked(token.pid) ||
		    find_target_save_login_fence_locked(token.pid) || !slot ||
		    !smith_profile(slot->profile) || !smith_checkpoint_mode_matches(*slot, actor) ||
		    !smith_checkpoint_pipeline_ready_locked(*slot) ||
		    slot->token != smith_checkpoint_identity(token) || !slot->held ||
		    slot->restored_sql_drop || slot->operation_id.bytes != operation.bytes ||
		    slot->payload != blob || slot->level != level || !slot->captured_revision ||
		    slot->acknowledged_revision != slot->captured_revision ||
		    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
			return false;
		player_smith_flat_checkpoint_cut flat_cut;
		economic_native_money_checkpoint_projection flat_wallet{};
		std::optional<player_save_execution_guard::held_publication_reservation> reservation;
		if (slot->profile == literal_checkpoint_profile::flat_smith)
		{
			if (!slot->execution_hold_generation || !slot->flat_shop_ownership_epoch)
				return false;
			reservation.emplace(slot->flat_shop_ownership_epoch, token.pid, operation,
					    slot->execution_hold_generation);
			if (!reservation->matches_pid(token.pid))
				return false;
			flat_cut = { slot->flat_shop_root, slot->flat_shop_ownership_epoch,
				     slot->execution_hold_generation };
			flat_wallet = { slot->flat_shop_mapping.lineage,
					slot->flat_shop_mapping.epoch,
					slot->flat_shop_mapping.wallet };
		}
		else if (flat_cut_out || flat_wallet_out)
			return false;
		player_snapshot acknowledged;
		std::vector<uint8_t> canonical, acknowledged_blob;
		uint32_t acknowledged_level = 0;
		if (slot->original_shop_body.empty() ||
		    player_snapshot_decode(slot->original_shop_body.data(),
					   slot->original_shop_body.size(),
					   &acknowledged) != player_snapshot_codec_result::ok ||
		    acknowledged.pid != token.pid ||
		    acknowledged.revision != slot->acknowledged_revision ||
		    !shop_checkpoint_blob(acknowledged, &acknowledged_blob, &acknowledged_level) ||
		    acknowledged_blob != slot->payload || acknowledged_level != slot->level ||
		    player_snapshot_encode(acknowledged, &canonical) !=
			    player_snapshot_codec_result::ok ||
		    canonical != slot->original_shop_body)
			return false;
		static_assert(std::is_nothrow_move_assignable_v<player_snapshot>);
		static_assert(std::is_nothrow_move_assignable_v<player_smith_flat_checkpoint_cut>);
		if (reservation &&
		    (!reservation->valid() || !smith_flat_context_matches(*slot, actor)))
			return false;
		if (original_acknowledged_body)
			*original_acknowledged_body = std::move(acknowledged);
		// Unrelated dirty STATUS marks may advance while the native operation is held.
		// Frozen inventory/level and the actual acknowledged save revision must match.
		// This exact current body was compared to the original held slot.
		// Copying values exposes no execution hold or ACK capability.
		if (items_out)
			*items_out = std::move(captured.items);
		if (flat_cut_out)
			*flat_cut_out = std::move(flat_cut);
		if (flat_wallet_out)
			*flat_wallet_out = flat_wallet;
		*output = { slot->acknowledged_revision, slot->level };
		return true;
	}
	catch (...)
	{
		return false;
	}
}

namespace
{
player_literal_inventory_token
flat_shop_checkpoint_identity(const player_flat_shop_checkpoint_token &token)
{
	return { token.pid, token.actor_runtime_id, token.root_uid, token.generation };
}

bool flat_shop_mapping_equal(const economic_shop_checkpoint_projection &a,
			     const economic_shop_checkpoint_projection &b) noexcept
{
	return a.lineage.bytes == b.lineage.bytes && a.epoch.bytes == b.epoch.bytes &&
	       economic_account_key_equal(a.wallet, b.wallet) &&
	       economic_account_key_equal(a.bank, b.bank);
}

bool flat_shop_source_cut(P_char actor, std::string *root,
			  economic_shop_checkpoint_projection *mapping, uint64_t *epoch)
{
	if (!actor || !IS_PC(actor) || !actor->only.pc || GET_PID(actor) <= 0 ||
	    !actor->runtime_id || find_character_by_runtime_id(actor->runtime_id) != actor ||
	    !nevent_is_game_thread() || !root || !mapping || !epoch ||
	    IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED) ||
	    !economic_gameplay_authority::active_regular_flat() ||
	    selected_snapshot_apply() != flatfile_player_snapshot_apply_selected ||
	    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY)
		return false;
	const char *selected = persistence_mode_flatfile_root();
	const char *account = get_account_name_safe(actor);
	const int racewar = GET_RACEWAR(actor);
	const uint64_t original_epoch = player_save_execution_guard::current_ownership_epoch();
	if (!selected || !*selected || !account || !*account || racewar < 0 || racewar > INT8_MAX ||
	    !original_epoch)
		return false;
	std::string selected_root(selected);
	economic_shop_checkpoint_projection projection{};
	if (!economic_gameplay_authority::observe_flat_shop_checkpoint(
		    GET_PID(actor), account, static_cast<uint8_t>(racewar), &projection) ||
	    !persistence_mode_flatfile_root() ||
	    selected_root != persistence_mode_flatfile_root() ||
	    player_save_execution_guard::current_ownership_epoch() != original_epoch)
		return false;
	*root = std::move(selected_root);
	*mapping = projection;
	*epoch = original_epoch;
	return true;
}

bool flat_shop_context_matches(const literal_inventory_checkpoint &slot, P_char actor) noexcept
{
	try
	{
		if (slot.profile != literal_checkpoint_profile::flat_shop ||
		    !shop_actor_matches({ slot.token.pid, slot.token.actor_runtime_id,
					  slot.token.root_uid, slot.token.generation },
					actor))
			return false;
		const char *account = get_account_name_safe(actor);
		std::string selected_root;
		economic_shop_checkpoint_projection mapping{};
		uint64_t epoch = 0;
		return account && slot.flat_shop_account == account &&
		       GET_RACEWAR(actor) == slot.flat_shop_racewar &&
		       flat_shop_source_cut(actor, &selected_root, &mapping, &epoch) &&
		       slot.flat_shop_root == selected_root &&
		       slot.flat_shop_ownership_epoch == epoch &&
		       flat_shop_mapping_equal(slot.flat_shop_mapping, mapping);
	}
	catch (...)
	{
		return false;
	}
}

bool flat_shop_pipeline_ready_locked(int pid)
{
	return health.initialized && execution_started && !stop_requested && accepting &&
	       health.replay_complete && !health.replay_blocked &&
	       !find_terminal_fence_locked(pid) && !find_target_save_login_fence_locked(pid);
}

bool flat_shop_drained_locked(const literal_inventory_checkpoint &slot)
{
	player_revision_snapshot revision{};
	return slot.captured_revision && slot.acknowledged_revision == slot.captured_revision &&
	       !slot.original_shop_body.empty() &&
	       player_revision_snapshot_copy(slot.token.pid, &revision) && !revision.overflowed &&
	       revision.current_revision == slot.captured_revision &&
	       revision.acknowledged_revision == slot.captured_revision &&
	       !revision.dirty_components && !revision.unacknowledged_components &&
	       !revision.queued_components && !revision.inflight_components &&
	       append_inflight_pid != slot.token.pid &&
	       !any_snapshot_is_retained_locked(slot.token.pid);
}

bool flat_shop_held_slot_matches(const literal_inventory_checkpoint &slot,
				 const player_flat_shop_checkpoint_token &token,
				 const critical_operation_id &operation)
{
	return slot.profile == literal_checkpoint_profile::flat_shop && slot.held &&
	       slot.token == flat_shop_checkpoint_identity(token) && !slot.restored_sql_drop &&
	       slot.operation_id.bytes == operation.bytes &&
	       !critical_operation_id_is_zero(operation) && slot.execution_hold_generation &&
	       slot.flat_shop_ownership_epoch &&
	       flat_shop_context_matches(slot,
					 find_character_by_runtime_id(token.actor_runtime_id));
}
}

player_literal_inventory_state
player_save_pipeline_flat_shop_checkpoint_begin(P_char actor, P_obj root, int room_vnum,
						player_flat_shop_checkpoint_token *token_out)
{
	if (!token_out || !actor || !nevent_is_game_thread() ||
	    (root && (!root->obj_uid || !OBJ_CARRIED_BY(root, actor))))
		return player_literal_inventory_state::refused;
	try
	{
		std::string selected_root;
		economic_shop_checkpoint_projection mapping{};
		uint64_t epoch = 0;
		if (!flat_shop_source_cut(actor, &selected_root, &mapping, &epoch) ||
		    player_save_journal_pid_quarantined(GET_PID(actor)))
			return player_literal_inventory_state::refused;
		const uint64_t root_uid = root ? root->obj_uid : 0;
		player_snapshot captured;
		std::vector<uint8_t> blob;
		uint32_t level = 0;
		if (player_snapshot_capture_literal_inventory(
			    actor, 1, SHOP_CHECKPOINT_COMPONENTS, RENT_CRASH, room_vnum, root_uid,
			    &captured) != player_snapshot_capture_result::ok ||
		    !shop_checkpoint_blob(captured, &blob, &level))
			return player_literal_inventory_state::refused;
		literal_inventory_checkpoint prepared;
		prepared.profile = literal_checkpoint_profile::flat_shop;
		prepared.payload = std::move(blob);
		prepared.level = level;
		prepared.flat_shop_root = std::move(selected_root);
		prepared.flat_shop_account = get_account_name_safe(actor);
		prepared.flat_shop_racewar = static_cast<uint8_t>(GET_RACEWAR(actor));
		prepared.flat_shop_mapping = mapping;
		prepared.flat_shop_ownership_epoch = epoch;
		player_flat_shop_checkpoint_token token;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			if (!flat_shop_pipeline_ready_locked(GET_PID(actor)))
				return player_literal_inventory_state::refused;
			if (auto *existing = find_literal_inventory_locked(GET_PID(actor)))
			{
				if (existing->profile != literal_checkpoint_profile::flat_shop ||
				    existing->held ||
				    existing->token.actor_runtime_id != actor->runtime_id ||
				    existing->token.root_uid != root_uid ||
				    existing->payload != prepared.payload ||
				    !flat_shop_context_matches(*existing, actor))
					return player_literal_inventory_state::refused;
				*token_out = { existing->token.pid,
					       existing->token.actor_runtime_id,
					       existing->token.root_uid,
					       existing->token.generation };
				return player_literal_inventory_state::pending;
			}
			if (flat_shop_inventory_generation == std::numeric_limits<uint64_t>::max())
				return player_literal_inventory_state::refused;
			literal_inventory_checkpoint *slot = nullptr;
			for (auto &candidate : literal_inventory_checkpoints)
				if (!candidate.token.pid)
				{
					slot = &candidate;
					break;
				}
			size_t bytes = prepared.payload.size();
			if (bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES)
				return player_literal_inventory_state::refused;
			for (size_t amount :
			     { prepared.flat_shop_root.size(), prepared.flat_shop_account.size() })
			{
				if (amount > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
					return player_literal_inventory_state::refused;
				bytes += amount;
			}
			token = { GET_PID(actor), actor->runtime_id, root_uid,
				  flat_shop_inventory_generation + 1 };
			prepared.token = flat_shop_checkpoint_identity(token);
			if (!slot || !literal_inventory_capacity_locked(bytes) ||
			    !flat_shop_context_matches(prepared, actor))
				return player_literal_inventory_state::refused;
			static_assert(
				std::is_nothrow_move_assignable_v<literal_inventory_checkpoint>);
			*slot = std::move(prepared);
			++flat_shop_inventory_generation;
		}
		// This original slot is recoverable even if enqueue has uncertain outcome.
		const auto previous_output = *token_out;
		*token_out = token;
		player_save_pipeline_result queued;
		try
		{
			queued = player_save_pipeline_request(actor, SHOP_CHECKPOINT_COMPONENTS,
							      RENT_CRASH, room_vnum);
		}
		catch (...)
		{
			return player_literal_inventory_state::pending;
		}
		if (queued != player_save_pipeline_result::queued &&
		    queued != player_save_pipeline_result::coalesced)
		{
			if (!player_save_pipeline_flat_shop_checkpoint_cancel(token))
				return player_literal_inventory_state::pending;
			*token_out = previous_output;
			return player_literal_inventory_state::refused;
		}
		return player_literal_inventory_state::pending;
	}
	catch (...)
	{
		return player_literal_inventory_state::refused;
	}
}

player_literal_inventory_state
player_save_pipeline_flat_shop_checkpoint_poll(const player_flat_shop_checkpoint_token &token,
					       P_char actor,
					       player_shop_checkpoint_stage *stage_out)
{
	if (!nevent_is_game_thread())
		return player_literal_inventory_state::refused;
	try
	{
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			const auto *slot = find_literal_inventory_locked(token.pid);
			if (!slot || slot->profile != literal_checkpoint_profile::flat_shop ||
			    slot->token != flat_shop_checkpoint_identity(token) || slot->held)
				return player_literal_inventory_state::refused;
			if (!flat_shop_pipeline_ready_locked(token.pid) ||
			    !flat_shop_context_matches(*slot, actor) ||
			    player_save_journal_pid_quarantined(token.pid))
				return player_literal_inventory_state::refused;
		}
		player_snapshot captured;
		std::vector<uint8_t> blob;
		if (player_snapshot_capture_literal_inventory(
			    actor, 1, SHOP_CHECKPOINT_COMPONENTS, RENT_CRASH, NOWHERE,
			    token.root_uid, &captured) != player_snapshot_capture_result::ok ||
		    !shop_checkpoint_blob(captured, &blob))
			return player_literal_inventory_state::refused;
		if (player_save_worker_pid_pending(token.pid))
			return player_literal_inventory_state::pending;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(token.pid);
		if (!slot || slot->profile != literal_checkpoint_profile::flat_shop ||
		    slot->token != flat_shop_checkpoint_identity(token) || slot->held ||
		    !flat_shop_pipeline_ready_locked(token.pid) ||
		    !flat_shop_context_matches(*slot, actor))
			return player_literal_inventory_state::refused;
		if (slot->payload != blob)
			return player_literal_inventory_state::refused;
		if (!flat_shop_drained_locked(*slot))
			return player_literal_inventory_state::pending;
		if (stage_out)
			*stage_out = { slot->acknowledged_revision, slot->level };
		return player_literal_inventory_state::database_acknowledged;
	}
	catch (...)
	{
		return player_literal_inventory_state::refused;
	}
}

bool player_save_pipeline_flat_shop_checkpoint_cancel(const player_flat_shop_checkpoint_token &token)
{
	if (!nevent_is_game_thread() || token.pid <= 0 || !token.actor_runtime_id ||
	    !token.generation)
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *slot = find_literal_inventory_locked(token.pid);
	if (!slot)
		return true;
	if (slot->profile != literal_checkpoint_profile::flat_shop ||
	    slot->token != flat_shop_checkpoint_identity(token) || slot->held ||
	    slot->execution_hold_generation || slot->flat_shop_native_attempt_started)
		return false;
	*slot = {};
	return true;
}

bool player_save_shop_checkpoint_owner::hold_flat(const player_flat_shop_checkpoint_token &token,
						  const critical_operation_id &operation) noexcept
{
	try
	{
		if (!nevent_is_game_thread() || critical_operation_id_is_zero(operation) ||
		    player_save_pipeline_flat_shop_checkpoint_poll(
			    token, find_character_by_runtime_id(token.actor_runtime_id)) !=
			    player_literal_inventory_state::database_acknowledged ||
		    player_save_worker_pid_pending(token.pid))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(token.pid);
		if (!slot || slot->profile != literal_checkpoint_profile::flat_shop ||
		    slot->token != flat_shop_checkpoint_identity(token) || slot->held ||
		    !flat_shop_pipeline_ready_locked(token.pid) ||
		    !flat_shop_context_matches(
			    *slot, find_character_by_runtime_id(token.actor_runtime_id)) ||
		    !flat_shop_drained_locked(*slot))
			return false;
		uint64_t generation = 0;
		if (!player_save_execution_guard::install_live_publication_hold(
			    token.pid, operation, &generation))
			return false;
		// No fallible work follows the genuine exclusion. Same original slot.
		slot->execution_hold_generation = generation;
		slot->operation_id = operation;
		slot->held = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool player_save_shop_checkpoint_owner::observe_held_flat(
	const player_flat_shop_checkpoint_token &token, P_char actor,
	const critical_operation_id &operation, player_snapshot *body_out,
	player_shop_checkpoint_stage *stage_out, economic_shop_checkpoint_projection *mapping_out,
	player_flat_shop_checkpoint_cut *cut_out) noexcept
{
	try
	{
		if (!nevent_is_game_thread() || !body_out || !stage_out || !mapping_out ||
		    !cut_out || player_save_journal_pid_quarantined(token.pid) ||
		    player_save_worker_pid_pending(token.pid))
			return false;
		player_snapshot current;
		std::vector<uint8_t> blob;
		uint32_t level = 0;
		if (!shop_actor_matches({ token.pid, token.actor_runtime_id, token.root_uid,
					  token.generation },
					actor) ||
		    player_snapshot_capture_literal_inventory(
			    actor, 1, SHOP_CHECKPOINT_COMPONENTS, RENT_CRASH, NOWHERE,
			    token.root_uid, &current) != player_snapshot_capture_result::ok ||
		    !shop_checkpoint_blob(current, &blob, &level))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto *slot = find_literal_inventory_locked(token.pid);
		if (!slot || !flat_shop_pipeline_ready_locked(token.pid) ||
		    !flat_shop_held_slot_matches(*slot, token, operation) ||
		    slot->payload != blob || slot->level != level || !slot->captured_revision ||
		    slot->acknowledged_revision != slot->captured_revision ||
		    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
			return false;
		player_save_execution_guard::held_publication_reservation reservation(
			slot->flat_shop_ownership_epoch, token.pid, operation,
			slot->execution_hold_generation);
		if (!reservation.matches_pid(token.pid))
			return false;
		player_snapshot original;
		std::vector<uint8_t> original_blob;
		if (player_snapshot_decode(slot->original_shop_body.data(),
					   slot->original_shop_body.size(),
					   &original) != player_snapshot_codec_result::ok ||
		    original.pid != token.pid || original.revision != slot->captured_revision ||
		    !shop_checkpoint_blob(original, &original_blob) ||
		    original_blob != slot->payload)
			return false;
		player_flat_shop_checkpoint_cut cut{ slot->flat_shop_root,
						     slot->flat_shop_ownership_epoch,
						     slot->execution_hold_generation };
		static_assert(std::is_nothrow_move_assignable_v<player_snapshot>);
		static_assert(std::is_nothrow_move_assignable_v<player_flat_shop_checkpoint_cut>);
		if (!reservation.valid() || !flat_shop_context_matches(*slot, actor))
			return false;
		*body_out = std::move(original);
		*stage_out = { slot->acknowledged_revision, slot->level };
		*mapping_out = slot->flat_shop_mapping;
		*cut_out = std::move(cut);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool player_save_shop_checkpoint_owner::reserve_flat_native_checkpoint(
	const player_flat_shop_checkpoint_token &token, const critical_operation_id &operation,
	size_t retained_bytes) noexcept
{
	try
	{
		if (!nevent_is_game_thread() || !retained_bytes ||
		    retained_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
		    critical_operation_id_is_zero(operation))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(token.pid);
		if (!slot || !flat_shop_held_slot_matches(*slot, token, operation) ||
		    slot->flat_shop_native_attempt_started ||
		    !flat_shop_pipeline_ready_locked(token.pid) ||
		    !flat_shop_context_matches(
			    *slot, find_character_by_runtime_id(token.actor_runtime_id)))
			return false;
		player_save_execution_guard::held_publication_reservation reservation(
			slot->flat_shop_ownership_epoch, token.pid, operation,
			slot->execution_hold_generation);
		if (!reservation.matches_pid(token.pid))
			return false;
		// An exact repeated observation cannot shrink/grow the original stage's
		// reservation. No uncertain attempt or public caller can reclaim it.
		if (slot->flat_shop_native_reserved_bytes)
			return slot->flat_shop_native_reserved_bytes == retained_bytes;
		// Include this slot's real ACK/payload/root/account plus every other
		// original profile and reservation. No parallel private budget exists.
		if (!literal_inventory_capacity_locked(retained_bytes) || !reservation.valid())
			return false;
		slot->flat_shop_native_reserved_bytes = retained_bytes;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool player_save_shop_checkpoint_owner::reserve_flat_command_checkpoint(
	const player_flat_shop_checkpoint_token &token, const critical_operation_id &operation,
	const player_flat_shop_checkpoint_cut &original_cut, size_t bytes) noexcept
{
	try
	{
		if (!nevent_is_game_thread() || !bytes || bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
		    critical_operation_id_is_zero(operation))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(token.pid);
		if (!slot || !flat_shop_held_slot_matches(*slot, token, operation) ||
		    !slot->flat_shop_native_attempt_started ||
		    !slot->flat_shop_native_reserved_bytes ||
		    !flat_shop_pipeline_ready_locked(token.pid) ||
		    slot->flat_shop_root != original_cut.selected_root ||
		    slot->flat_shop_ownership_epoch != original_cut.ownership_epoch ||
		    slot->execution_hold_generation != original_cut.execution_hold_generation ||
		    !flat_shop_context_matches(
			    *slot, find_character_by_runtime_id(token.actor_runtime_id)))
			return false;
		player_save_execution_guard::held_publication_reservation reservation(
			original_cut.ownership_epoch, token.pid, operation,
			original_cut.execution_hold_generation);
		if (!reservation.matches_pid(token.pid))
			return false;
		if (slot->flat_shop_command_reserved_bytes)
			return slot->flat_shop_command_reserved_bytes == bytes &&
			       reservation.valid();
		if (!literal_inventory_capacity_locked(bytes) || !reservation.valid())
			return false;
		// No native reservation resize or attempted-leaf reset is permitted.
		slot->flat_shop_command_reserved_bytes = bytes;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool player_save_shop_checkpoint_owner::begin_native_attempt_flat(
	const player_flat_shop_checkpoint_token &token,
	const critical_operation_id &operation) noexcept
{
	player_snapshot body;
	player_shop_checkpoint_stage stage;
	economic_shop_checkpoint_projection mapping;
	player_flat_shop_checkpoint_cut cut;
	if (!observe_held_flat(token, find_character_by_runtime_id(token.actor_runtime_id),
			       operation, &body, &stage, &mapping, &cut))
		return false;
	try
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(token.pid);
		if (!slot || !flat_shop_held_slot_matches(*slot, token, operation) ||
		    slot->flat_shop_native_attempt_started ||
		    !slot->flat_shop_native_reserved_bytes ||
		    !flat_shop_pipeline_ready_locked(token.pid) ||
		    slot->execution_hold_generation != cut.execution_hold_generation ||
		    slot->flat_shop_ownership_epoch != cut.ownership_epoch)
			return false;
		player_save_execution_guard::held_publication_reservation reservation(
			cut.ownership_epoch, token.pid, operation, cut.execution_hold_generation);
		if (!reservation.matches_pid(token.pid))
			return false;
		slot->flat_shop_native_attempt_started = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool player_save_shop_checkpoint_owner::release_unattempted_flat(
	const player_flat_shop_checkpoint_token &token,
	const critical_operation_id &operation) noexcept
{
	try
	{
		if (!nevent_is_game_thread())
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(token.pid);
		if (!slot || !flat_shop_held_slot_matches(*slot, token, operation) ||
		    slot->flat_shop_native_attempt_started)
			return false;
		{
			player_save_execution_guard::held_publication_reservation reservation(
				slot->flat_shop_ownership_epoch, token.pid, operation,
				slot->execution_hold_generation);
			if (!reservation.matches_pid(token.pid))
				return false;
		}
		// Game-thread owner has never handed this exact hold to a native attempt.
		// Release only its real leaf; no arbitrary safe/rolled-back caller flag.
		if (!player_save_execution_guard::release_hold(token.pid, operation,
							       slot->execution_hold_generation))
			return false;
		*slot = {};
		return true;
	}
	catch (...)
	{
		return false;
	}
}

namespace
{
player_literal_inventory_token
auction_checkpoint_identity(const player_auction_checkpoint_token &token)
{
	return { token.pid, token.actor_runtime_id, token.root_uid, token.generation };
}

#ifndef __NO_MYSQL__
bool auction_actor_matches(const player_auction_checkpoint_token &token, P_char actor)
{
	return !token.root_uid && actor && IS_PC(actor) && actor->only.pc && token.pid > 0 &&
	       GET_PID(actor) == token.pid && token.actor_runtime_id && token.generation &&
	       actor->runtime_id == token.actor_runtime_id &&
	       find_character_by_runtime_id(token.actor_runtime_id) == actor &&
	       !IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED);
}
#endif
}

player_literal_inventory_state
player_save_pipeline_auction_checkpoint_begin(P_char actor, int room_vnum,
					      std::span<const uint64_t> selected_roots,
					      player_auction_checkpoint_token *token_out)
{
	P_obj root = nullptr; // Auctions pin the whole player body, including empty inventory.
#ifdef __NO_MYSQL__
	(void)actor;
	(void)root;
	(void)room_vnum;
	(void)selected_roots;
	(void)token_out;
	return player_literal_inventory_state::refused;
#else
	if (selected_roots.size() > 9 || !actor || !token_out || !IS_PC(actor) || !actor->only.pc ||
	    GET_PID(actor) <= 0 || !actor->runtime_id ||
	    find_character_by_runtime_id(actor->runtime_id) != actor ||
	    !economic_gameplay_authority::active_regular_sql() ||
	    (root && (!root->obj_uid || !OBJ_CARRIED_BY(root, actor))) ||
	    IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED) ||
	    player_save_journal_pid_quarantined(GET_PID(actor)))
		return player_literal_inventory_state::refused;
	const uint64_t root_uid = root ? root->obj_uid : 0;
	player_snapshot captured;
	std::vector<uint8_t> blob;
	uint32_t level = 0;
	if (player_snapshot_capture_literal_inventory_roots(
		    actor, 1, SHOP_CHECKPOINT_COMPONENTS, RENT_CRASH, room_vnum, selected_roots,
		    &captured) != player_snapshot_capture_result::ok ||
	    !shop_checkpoint_blob(captured, &blob, &level))
		return player_literal_inventory_state::refused;
	player_auction_checkpoint_token token;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!economic_gameplay_authority::active_regular_sql() || !health.initialized ||
		    stop_requested || !accepting || !health.replay_complete ||
		    health.replay_blocked || find_target_save_login_fence_locked(GET_PID(actor)) ||
		    find_terminal_fence_locked(GET_PID(actor)))
			return player_literal_inventory_state::refused;
		if (auto *existing = find_literal_inventory_locked(GET_PID(actor)))
		{
			if (existing->profile != literal_checkpoint_profile::auction ||
			    existing->token.actor_runtime_id != actor->runtime_id ||
			    existing->token.root_uid != root_uid || existing->payload != blob ||
			    existing->auction_literal_root_count != selected_roots.size() ||
			    !std::equal(selected_roots.begin(), selected_roots.end(),
					existing->auction_literal_roots.begin()) ||
			    existing->held)
				return player_literal_inventory_state::refused;
			*token_out = { existing->token.pid, existing->token.actor_runtime_id,
				       existing->token.root_uid, existing->token.generation };
			return player_literal_inventory_state::pending;
		}
		if (literal_inventory_generation == std::numeric_limits<uint64_t>::max())
			return player_literal_inventory_state::refused;
		literal_inventory_checkpoint *slot = nullptr;
		for (auto &candidate : literal_inventory_checkpoints)
			if (!candidate.token.pid)
			{
				slot = &candidate;
				break;
			}
		if (!slot || !literal_inventory_capacity_locked(blob.size()))
			return player_literal_inventory_state::refused;
		token = { GET_PID(actor), actor->runtime_id, root_uid,
			  ++literal_inventory_generation };
		slot->profile = literal_checkpoint_profile::auction;
		slot->level = level;
		slot->auction_literal_root_count = selected_roots.size();
		std::copy(selected_roots.begin(), selected_roots.end(),
			  slot->auction_literal_roots.begin());
		slot->token = auction_checkpoint_identity(token);
		slot->payload = std::move(blob);
	}
	// Make the installed original slot recoverable before enqueue can throw.
	*token_out = token;
	player_save_pipeline_result queued;
	try
	{
		queued = player_save_pipeline_request(actor, SHOP_CHECKPOINT_COMPONENTS, RENT_CRASH,
						      room_vnum);
	}
	catch (...)
	{
		// Queue outcome is unresolved. Original token and frozen save policy remain.
		return player_literal_inventory_state::pending;
	}
	if (queued != player_save_pipeline_result::queued &&
	    queued != player_save_pipeline_result::coalesced)
	{
		player_save_pipeline_auction_checkpoint_cancel(token);
		return player_literal_inventory_state::refused;
	}
	*token_out = token;
	return player_literal_inventory_state::pending;
#endif
}

player_literal_inventory_state
player_save_pipeline_auction_checkpoint_poll(const player_auction_checkpoint_token &token,
					     P_char actor,
					     player_auction_checkpoint_stage *stage_out)
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)stage_out;
	return player_literal_inventory_state::refused;
#else
	const auto identity = auction_checkpoint_identity(token);
	std::array<uint64_t, 9> selected_roots{};
	size_t selected_count = 0;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto *literal = find_literal_inventory_locked(token.pid);
		if (!literal || literal->profile != literal_checkpoint_profile::auction ||
		    literal->token != identity || literal->held ||
		    literal->auction_literal_root_count > selected_roots.size())
			return player_literal_inventory_state::refused;
		selected_roots = literal->auction_literal_roots;
		selected_count = literal->auction_literal_root_count;
	}
	if (!economic_gameplay_authority::active_regular_sql() ||
	    !auction_actor_matches(token, actor) || player_save_journal_pid_quarantined(token.pid))
	{
		player_save_pipeline_auction_checkpoint_cancel(token);
		return player_literal_inventory_state::refused;
	}
	player_snapshot captured;
	std::vector<uint8_t> blob;
	if (player_snapshot_capture_literal_inventory_roots(
		    actor, 1, SHOP_CHECKPOINT_COMPONENTS, RENT_CRASH, NOWHERE,
		    std::span<const uint64_t>(selected_roots.data(), selected_count),
		    &captured) != player_snapshot_capture_result::ok ||
	    !shop_checkpoint_blob(captured, &blob))
	{
		player_save_pipeline_auction_checkpoint_cancel(token);
		return player_literal_inventory_state::refused;
	}
	if (player_save_worker_pid_pending(token.pid))
		return player_literal_inventory_state::pending;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	if (!literal || literal->profile != literal_checkpoint_profile::auction ||
	    literal->token != identity || literal->held)
		return player_literal_inventory_state::refused;
	if (literal->payload != blob)
	{
		*literal = {};
		return player_literal_inventory_state::refused;
	}
	player_revision_snapshot revision = {};
	if (!player_revision_snapshot_copy(token.pid, &revision) || revision.overflowed)
		return player_literal_inventory_state::refused;
	if (!literal->captured_revision ||
	    literal->acknowledged_revision != literal->captured_revision ||
	    revision.current_revision != literal->captured_revision ||
	    revision.acknowledged_revision != literal->captured_revision ||
	    revision.dirty_components || revision.unacknowledged_components ||
	    revision.queued_components || revision.inflight_components ||
	    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
		return player_literal_inventory_state::pending;
	if (stage_out)
		*stage_out = { literal->acknowledged_revision, literal->level };
	return player_literal_inventory_state::database_acknowledged;
#endif
}

bool player_save_pipeline_auction_checkpoint_hold(const player_auction_checkpoint_token &token,
						  const critical_operation_id &operation_id)
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)operation_id;
	return false;
#else
	if (std::all_of(operation_id.bytes.begin(), operation_id.bytes.end(),
			[](uint8_t byte) { return !byte; }) ||
	    player_save_pipeline_auction_checkpoint_poll(
		    token, find_character_by_runtime_id(token.actor_runtime_id)) !=
		    player_literal_inventory_state::database_acknowledged ||
	    player_save_worker_pid_pending(token.pid))
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	player_revision_snapshot revision = {};
	if (!economic_gameplay_authority::active_regular_sql() || !health.initialized ||
	    stop_requested || !accepting || !health.replay_complete || health.replay_blocked ||
	    find_terminal_fence_locked(token.pid) ||
	    find_target_save_login_fence_locked(token.pid) || !literal ||
	    literal->profile != literal_checkpoint_profile::auction ||
	    literal->token != auction_checkpoint_identity(token) || literal->held ||
	    !literal->captured_revision ||
	    literal->acknowledged_revision != literal->captured_revision ||
	    !player_revision_snapshot_copy(token.pid, &revision) || revision.overflowed ||
	    revision.current_revision != literal->captured_revision ||
	    revision.acknowledged_revision != literal->captured_revision ||
	    revision.dirty_components || revision.unacknowledged_components ||
	    revision.queued_components || revision.inflight_components ||
	    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
		return false;
	literal->operation_id = operation_id;
	literal->held = true;
	return true;
#endif
}

bool player_save_pipeline_auction_checkpoint_release(const player_auction_checkpoint_token &token,
						     const critical_operation_id &operation_id)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	if (!literal || literal->profile != literal_checkpoint_profile::auction ||
	    literal->token != auction_checkpoint_identity(token) || !literal->held ||
	    literal->restored_sql_drop || literal->operation_id.bytes != operation_id.bytes)
		return false;
	*literal = {};
	return true;
}

bool player_save_pipeline_auction_checkpoint_cancel(const player_auction_checkpoint_token &token)
{
	if (token.pid <= 0 || !token.actor_runtime_id || !token.generation)
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	// Poll may already have cancelled this unheld checkpoint on a changed body.
	// Absence consumes no save/publication reservation and grants no ACK authority.
	if (!literal)
		return true;
	if (literal->profile != literal_checkpoint_profile::auction ||
	    literal->token != auction_checkpoint_identity(token) || literal->held)
		return false;
	*literal = {};
	return true;
}

bool player_save_auction_checkpoint_owner::observe_held(
	const player_auction_checkpoint_token &token, P_char actor,
	const critical_operation_id &operation, player_auction_checkpoint_stage *output,
	std::vector<player_item_snapshot> *items_out) noexcept
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)operation;
	(void)output;
	(void)items_out;
	return false;
#else
	if (!output || !nevent_is_game_thread() ||
	    !economic_gameplay_authority::active_regular_sql() ||
	    !auction_actor_matches(token, actor) ||
	    player_save_journal_pid_quarantined(token.pid) ||
	    player_save_worker_pid_pending(token.pid))
		return false;
	try
	{
		std::array<uint64_t, 9> selected_roots{};
		size_t selected_count = 0;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			const auto *slot = find_literal_inventory_locked(token.pid);
			if (!slot || slot->profile != literal_checkpoint_profile::auction ||
			    slot->token != auction_checkpoint_identity(token) || !slot->held ||
			    slot->operation_id.bytes != operation.bytes ||
			    slot->auction_literal_root_count > selected_roots.size())
				return false;
			selected_roots = slot->auction_literal_roots;
			selected_count = slot->auction_literal_root_count;
		}
		player_snapshot captured;
		std::vector<uint8_t> blob;
		uint32_t level = 0;
		if (player_snapshot_capture_literal_inventory_roots(
			    actor, 1, SHOP_CHECKPOINT_COMPONENTS, RENT_CRASH, NOWHERE,
			    std::span<const uint64_t>(selected_roots.data(), selected_count),
			    &captured) != player_snapshot_capture_result::ok ||
		    !shop_checkpoint_blob(captured, &blob, &level))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto *slot = find_literal_inventory_locked(token.pid);
		if (!health.initialized || stop_requested || !accepting ||
		    !health.replay_complete || health.replay_blocked ||
		    find_terminal_fence_locked(token.pid) ||
		    find_target_save_login_fence_locked(token.pid) || !slot ||
		    slot->profile != literal_checkpoint_profile::auction ||
		    slot->token != auction_checkpoint_identity(token) || !slot->held ||
		    slot->restored_sql_drop || slot->operation_id.bytes != operation.bytes ||
		    slot->payload != blob || slot->level != level || !slot->captured_revision ||
		    slot->acknowledged_revision != slot->captured_revision ||
		    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
			return false;
		// Unrelated dirty STATUS marks may advance while the native operation is held.
		// Frozen inventory/level and the actual acknowledged save revision must match.
		// This exact current body was compared to the original held slot.
		// Copying values exposes no execution hold or ACK capability.
		if (items_out)
			*items_out = std::move(captured.items);
		*output = { slot->acknowledged_revision, slot->level };
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

static bool restore_sql_publication_obligation(
	const critical_command &command, int pid, uint64_t root_uid,
	literal_checkpoint_profile profile = literal_checkpoint_profile::ordinary_drop)
{
	std::vector<uint8_t> frozen;
	if (pid <= 0 || !root_uid ||
	    critical_command_encode(command, &frozen) != critical_command_codec_result::ok)
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	// The typed caller classified the original immutable room operation.
	// Registration remains closed before any execution owner starts.
	if (!health.initialized || stop_requested || execution_started)
		return false;
	literal_inventory_checkpoint *slot = nullptr;
	for (auto &candidate : literal_inventory_checkpoints)
	{
		if (candidate.token.pid == pid ||
		    (candidate.held && candidate.operation_id.bytes == command.operation_id.bytes))
		{
			uint64_t generation = 0;
			if (candidate.profile != profile || !candidate.restored_sql_drop ||
			    !candidate.held || candidate.token.pid != pid ||
			    candidate.token.root_uid != root_uid ||
			    candidate.operation_id.bytes != command.operation_id.bytes ||
			    candidate.payload != frozen ||
			    !player_save_execution_guard::install_hold(pid, command.operation_id,
								       &generation))
				return false;
			if (generation != candidate.execution_hold_generation)
			{
				player_save_execution_guard::poison_integrity();
				return false;
			}
			return true;
		}
		if (!candidate.token.pid && !slot)
			slot = &candidate;
	}
	if (!slot || !literal_inventory_capacity_locked(frozen.size()))
		return false;
	uint64_t generation = 0;
	if (!player_save_execution_guard::install_hold(pid, command.operation_id, &generation))
		return false;
	// All fallible command allocation/validation precedes guard installation.
	slot->execution_hold_generation = generation;
	slot->profile = profile;
	slot->token.pid = pid;
	slot->token.root_uid = root_uid;
	slot->payload = std::move(frozen);
	slot->operation_id = command.operation_id;
	slot->held = true;
	slot->restored_sql_drop = true;
	return true;
}

bool player_save_pipeline_restore_sql_drop_obligation(const critical_command &command)
{
#ifdef __NO_MYSQL__
	(void)command;
	return false;
#else
	try
	{
		if (!command.publication_required || !critical_command_envelope_valid(command) ||
		    !item_transfer_accounting_command_supported(command))
			return false;
		item_transfer_payload payload = {};
		sql_room_item_payload_batch captured;
		if (!item_transfer_command_decode_payload(command, &payload) ||
		    !sql_room_item_payload_capture(payload, &captured))
			return false;
		return restore_sql_publication_obligation(command,
							  static_cast<int>(payload.from_owner.id),
							  payload.selected_item_uid);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
#endif
}

bool player_save_pipeline_restore_sql_coin_obligation(const critical_command &command)
{
	try
	{
		int pid = 0;
		uint64_t uid = 0;
		if (!coin_physical_recovery_identity(command, &pid, &uid))
			return false;
		return restore_sql_publication_obligation(command, pid, uid);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

#ifndef __NO_MYSQL__
namespace
{
bool shop_publication_identity(const critical_command &command, int *pid, uint64_t *uid,
			       uint64_t *save_revision = nullptr, uint32_t *level = nullptr)
{
	economic_frozen_intent intent;
	shop_trade_payload payload = {};
	economic_account_key wallet, bank, counterparty;
	if (!command.publication_required ||
	    !shop_trade_payload_version_is_accounted(command.payload_version) ||
	    shop_trade_accounting_decode(command, &intent, &payload, &wallet, &bank,
					 &counterparty) != economic_accounting_error::ok ||
	    !payload.player_pid || payload.player_pid > INT_MAX || !payload.selected_item_uid ||
	    !payload.expected_player_save_revision || !payload.expected_player_level)
		return false;
	*pid = static_cast<int>(payload.player_pid);
	*uid = payload.selected_item_uid;
	if (save_revision)
		*save_revision = payload.expected_player_save_revision;
	if (level)
		*level = payload.expected_player_level;
	return true;
}

}
#endif
namespace
{
bool collector_purchase_identity(const critical_command &command, int *pid, uint64_t *uid)
{
	economic_frozen_intent intent;
	collector_command_payload payload;
	collector::record original;
	economic_account_key wallet, bank;
	if (!command.publication_required || !critical_command_envelope_valid(command) ||
	    collector_purchase_accounting_decode(command, &intent, &payload, &original, &wallet,
						 &bank) != economic_accounting_error::ok ||
	    !payload.actor_pid || payload.actor_pid > INT_MAX ||
	    payload.action != collector_action::purchase || payload.item_count != 1 ||
	    !payload.selected_item_uid)
		return false;
	*pid = static_cast<int>(payload.actor_pid);
	*uid = payload.selected_item_uid;
	return true;
}
}

bool player_save_pipeline_restore_sql_collector_purchase_obligation(const critical_command &command)
{
	try
	{
		int pid = 0;
		uint64_t uid = 0;
		return collector_purchase_identity(command, &pid, &uid) &&
		       restore_sql_publication_obligation(command, pid, uid);
	}
	catch (...)
	{
		return false;
	}
}

bool player_save_pipeline_restore_sql_shop_obligation(const critical_command &command)
{
#ifdef __NO_MYSQL__
	(void)command;
	return false;
#else
	try
	{
		int pid = 0;
		uint64_t uid = 0;
		return shop_publication_identity(command, &pid, &uid) &&
		       restore_sql_publication_obligation(command, pid, uid,
							  literal_checkpoint_profile::shop);
	}
	catch (...)
	{
		return false;
	}
#endif
}

critical_submit_result
player_save_shop_checkpoint_owner::submit_owned(const player_shop_checkpoint_token &token,
						critical_command command,
						bool *checkpoint_released) noexcept
{
	if (!checkpoint_released)
		return critical_submit_result::invalid;
	*checkpoint_released = false;
#ifdef __NO_MYSQL__
	(void)token;
	(void)command;
	return critical_submit_result::unavailable;
#else
	if (!nevent_is_game_thread())
		return critical_submit_result::unavailable;
	int pid = 0;
	uint64_t uid = 0, revision = 0, generation = 0;
	uint32_t level = 0;
	std::vector<uint8_t> frozen;
	bool new_hold = false;
	const auto operation = command.operation_id;
	try
	{
		if (!shop_publication_identity(command, &pid, &uid, &revision, &level) ||
		    pid != token.pid || !token.actor_runtime_id || !token.generation ||
		    critical_command_encode(command, &frozen) !=
			    critical_command_codec_result::ok ||
		    frozen.size() > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
		    player_save_worker_pid_pending(pid))
			return critical_submit_result::invalid;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(pid);
		if (!health.initialized || stop_requested || !accepting || !execution_started ||
		    !health.replay_complete || health.replay_blocked ||
		    find_terminal_fence_locked(pid) || find_target_save_login_fence_locked(pid) ||
		    append_inflight_pid == pid || any_snapshot_is_retained_locked(pid) || !slot)
			return critical_submit_result::unavailable;
		if (slot->profile != literal_checkpoint_profile::shop || !slot->held ||
		    slot->token != shop_checkpoint_identity(token) ||
		    slot->operation_id.bytes != operation.bytes)
			return critical_submit_result::identity_conflict;
		if (slot->restored_sql_drop)
		{
			if (slot->payload != frozen || !slot->execution_hold_generation)
				return critical_submit_result::identity_conflict;
			generation = slot->execution_hold_generation;
		}
		else
		{
			if (slot->captured_revision != revision ||
			    slot->acknowledged_revision != revision || slot->level != level ||
			    slot->payload.size() < sizeof(uint32_t) ||
			    !slot->original_shop_body.empty() ||
			    slot->payload.size() > PLAYER_SAVE_PIPELINE_MAX_BYTES - frozen.size() ||
			    !literal_inventory_capacity_locked(frozen.size() + slot->payload.size(),
							       slot) ||
			    !player_save_execution_guard::install_live_publication_hold(
				    pid, operation, &generation))
				return critical_submit_result::unavailable;
			// All allocation and combined capacity checks precede the leaf hold.
			// Move the actual original body, then the command; neither move throws.
			slot->original_shop_body = std::move(slot->payload);
			slot->payload = std::move(frozen);
			slot->execution_hold_generation = generation;
			slot->restored_sql_drop = true;
			new_hold = true;
		}
	}
	catch (...)
	{
		return critical_submit_result::invalid;
	}
	critical_submit_result submitted;
	try
	{
		// Never enter coordinator_mutex while pipeline_mutex is held.
		submitted = critical_command_coordinator_submit_for_publication(std::move(command));
	}
	catch (...)
	{
		// Admission may have happened; the exact original slot remains fenced.
		return critical_submit_result::journal_uncertain;
	}
	if (!critical_submit_result_keeps_operation(submitted) && new_hold)
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(pid);
		if (!slot || slot->profile != literal_checkpoint_profile::shop || !slot->held ||
		    !slot->restored_sql_drop || slot->token != shop_checkpoint_identity(token) ||
		    slot->operation_id.bytes != operation.bytes ||
		    slot->execution_hold_generation != generation)
		{
			player_save_execution_guard::poison_integrity();
			return critical_submit_result::journal_uncertain;
		}
		if (!player_save_execution_guard::release_hold(pid, operation, generation))
			return critical_submit_result::journal_uncertain;
		*slot = {};
		*checkpoint_released = true;
		// Definite pre-journal refusal releases only this exact reservation.
		// The preparation owner separately retires its native checkpoint facts.
		return submitted;
	}
	return critical_submit_result_keeps_operation(submitted) ?
		       submitted :
		       critical_submit_result::journal_uncertain;
#endif
}

bool player_save_shop_checkpoint_owner::original_held_body(
	const player_shop_checkpoint_token &token, const critical_command &command,
	std::vector<player_item_snapshot> *items_out,
	player_shop_checkpoint_stage *stage_out) noexcept
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)command;
	(void)items_out;
	(void)stage_out;
	return false;
#else
	if (!items_out || !stage_out || !nevent_is_game_thread() ||
	    !economic_gameplay_authority::active_regular_sql())
		return false;
	try
	{
		int pid = 0;
		uint64_t uid = 0, revision = 0;
		uint32_t level = 0;
		std::vector<uint8_t> frozen;
		if (!shop_publication_identity(command, &pid, &uid, &revision, &level) ||
		    pid != token.pid || !token.actor_runtime_id || !token.generation ||
		    critical_command_encode(command, &frozen) !=
			    critical_command_codec_result::ok ||
		    player_save_worker_pid_pending(pid))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto *slot = find_literal_inventory_locked(pid);
		if (!health.initialized || stop_requested || !slot || !slot->held ||
		    !slot->restored_sql_drop || slot->profile != literal_checkpoint_profile::shop ||
		    slot->token != shop_checkpoint_identity(token) ||
		    slot->operation_id.bytes != command.operation_id.bytes ||
		    slot->payload != frozen || !slot->execution_hold_generation ||
		    slot->captured_revision != revision ||
		    slot->acknowledged_revision != revision || slot->level != level ||
		    slot->original_shop_body.size() < sizeof(uint32_t) ||
		    slot->original_shop_body.size() > PLAYER_SNAPSHOT_MAX_BYTES ||
		    find_terminal_fence_locked(pid) || find_target_save_login_fence_locked(pid) ||
		    append_inflight_pid == pid || any_snapshot_is_retained_locked(pid))
			return false;
		const auto &body = slot->original_shop_body;
		const size_t item_bytes = body.size() - sizeof(uint32_t);
		uint32_t stored_level = 0;
		for (unsigned index = 0; index < sizeof(uint32_t); ++index)
			stored_level |= static_cast<uint32_t>(body[item_bytes + index])
					<< (index * 8);
		std::vector<player_item_snapshot> items;
		if (stored_level != level ||
		    player_item_snapshot_list_decode(body.data(), item_bytes, &items) !=
			    player_snapshot_codec_result::ok)
			return false;
		// No row, runtime graph or ACK is authorized by this original value copy.
		*items_out = std::move(items);
		*stage_out = { revision, stored_level };
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

critical_submit_result collector_purchase_submit_owned(critical_command command)
{
	if (!nevent_is_game_thread())
		return critical_submit_result::unavailable;
	int pid = 0;
	uint64_t uid = 0, generation = 0;
	std::vector<uint8_t> frozen;
	bool new_hold = false;
	try
	{
		if (!collector_purchase_identity(command, &pid, &uid) ||
		    critical_command_encode(command, &frozen) !=
			    critical_command_codec_result::ok ||
		    player_save_worker_pid_pending(pid))
			return critical_submit_result::invalid;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!health.initialized || stop_requested || !accepting || !execution_started ||
		    !health.replay_complete || health.replay_blocked ||
		    find_terminal_fence_locked(pid) || find_target_save_login_fence_locked(pid) ||
		    append_inflight_pid == pid || any_snapshot_is_retained_locked(pid))
			return critical_submit_result::unavailable;
		auto *slot = find_literal_inventory_locked(pid);
		if (slot)
		{
			if (!slot->held || !slot->restored_sql_drop || slot->payload != frozen ||
			    slot->token.root_uid != uid ||
			    slot->operation_id.bytes != command.operation_id.bytes)
				return critical_submit_result::identity_conflict;
			generation = slot->execution_hold_generation;
		}
		else
		{
			player_revision_snapshot revision = {};
			if (player_revision_snapshot_copy(pid, &revision) &&
			    (revision.overflowed || revision.dirty_components ||
			     revision.unacknowledged_components || revision.queued_components ||
			     revision.inflight_components ||
			     revision.current_revision != revision.acknowledged_revision))
				return critical_submit_result::unavailable;
			for (auto &candidate : literal_inventory_checkpoints)
				if (!candidate.token.pid)
				{
					slot = &candidate;
					break;
				}
			if (!slot || !literal_inventory_capacity_locked(frozen.size()) ||
			    !player_save_execution_guard::install_live_publication_hold(
				    pid, command.operation_id, &generation))
				return critical_submit_result::unavailable;
			slot->token.pid = pid;
			slot->token.root_uid = uid;
			slot->execution_hold_generation = generation;
			slot->operation_id = command.operation_id;
			slot->payload = std::move(frozen);
			slot->held = slot->restored_sql_drop = true;
			new_hold = true;
		}
	}
	catch (...)
	{
		return critical_submit_result::invalid;
	}
	// No fallible command allocation follows hold installation: ownership passes
	// by move. A thrown coordinator path may have admitted it, so remains held.
	critical_submit_result submitted;
	const critical_operation_id operation = command.operation_id;
	try
	{
		submitted = critical_command_coordinator_submit_for_publication(std::move(command));
	}
	catch (...)
	{
		return critical_submit_result::journal_uncertain;
	}
	if (!critical_submit_result_keeps_operation(submitted) && new_hold)
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(pid);
		if (!slot || !slot->held || !slot->restored_sql_drop ||
		    slot->operation_id.bytes != operation.bytes ||
		    slot->execution_hold_generation != generation)
		{
			player_save_execution_guard::poison_integrity();
			return critical_submit_result::journal_uncertain;
		}
		// Exact synchronous refusal occurred before coordinator journal admission.
		if (!player_save_execution_guard::release_hold(pid, operation, generation))
			return critical_submit_result::journal_uncertain;
		*slot = {};
	}
	return critical_submit_result_keeps_operation(submitted) || new_hold ?
		       submitted :
		       critical_submit_result::journal_uncertain;
}

bool player_save_restored_publication_owner::publish_collector(
	const critical_command &original, const critical_completion &completion,
	bool (*native_publish)(const critical_command &, const critical_completion &,
			       void *) noexcept,
	void *context) noexcept
{
	if (!nevent_is_game_thread() || !native_publish ||
	    !critical_completion_disposition_valid(completion) ||
	    original.operation_id.bytes != completion.operation_id.bytes)
		return false;
	try
	{
		int pid = 0;
		uint64_t uid = 0, generation = 0;
		critical_command command;
		std::vector<uint8_t> frozen;
		if (!collector_purchase_identity(original, &pid, &uid) ||
		    critical_command_encode(original, &frozen) != critical_command_codec_result::ok)
			return false;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			const auto *slot = find_literal_inventory_locked(pid);
			if (!health.initialized || stop_requested || !slot || !slot->held ||
			    !slot->restored_sql_drop || slot->token.root_uid != uid ||
			    slot->payload != frozen ||
			    slot->operation_id.bytes != completion.operation_id.bytes ||
			    find_terminal_fence_locked(pid) ||
			    find_target_save_login_fence_locked(pid) ||
			    append_inflight_pid == pid || any_snapshot_is_retained_locked(pid) ||
			    critical_command_decode(frozen.data(), frozen.size(), &command) !=
				    critical_command_codec_result::ok)
				return false;
			generation = slot->execution_hold_generation;
		}
		player_save_restored_publication_owner owner(
			std::move(command), std::move(frozen), completion,
			player_save_execution_guard::current_ownership_epoch(), pid, generation);
		if (!owner.reservation_.valid() || player_save_worker_pid_pending(pid))
			return false;
		if (completion.disposition == critical_completion_disposition::never_admitted)
			return critical_command_coordinator_cancel_collector_publication(owner);
		player_revision_snapshot revision = {};
		if (player_revision_snapshot_copy(pid, &revision) &&
		    (revision.overflowed || revision.dirty_components ||
		     revision.unacknowledged_components || revision.queued_components ||
		     revision.inflight_components ||
		     revision.current_revision != revision.acknowledged_revision))
			return false;
		std::vector<player_save_journal_retained_frame> originals;
		if (player_save_journal_collect_publication_frames(
			    pid, owner.reservation_, &originals) != player_save_journal_result::ok)
			return false;
		if (!originals.empty())
		{
			player_save_covered_revision covered;
			if (!player_snapshot_repository_observe_covered_revision(
				    pid, owner.reservation_, &covered) ||
			    player_save_journal_retire_covered_ordinary(pid, owner.reservation_,
									covered, originals) !=
				    player_save_journal_result::ok)
				return false;
		}
		if (player_save_journal_publication_census(pid, owner.reservation_) !=
			    player_save_journal_result::ok ||
		    !native_publish(owner.command_, completion, context) ||
		    !owner.reservation_.valid() ||
		    player_save_journal_publication_census(pid, owner.reservation_) !=
			    player_save_journal_result::ok)
			return false;
		owner.publication_proven_ = true;
		return critical_command_coordinator_acknowledge_publication(owner);
	}
	catch (...)
	{
		return false;
	}
}

void player_save_pipeline_sql_drop_publication_acknowledged(
	const critical_operation_id &operation_id) noexcept
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	for (auto &checkpoint : literal_inventory_checkpoints)
		if (checkpoint.profile == literal_checkpoint_profile::ordinary_drop &&
		    checkpoint.token.pid && checkpoint.held && !checkpoint.restored_sql_drop &&
		    checkpoint.operation_id.bytes == operation_id.bytes)
			checkpoint = {};
	// Restored holds are consumed only by the private guarded-ACK owner. An
	// ID-only assertion cannot release them, including before epoch enable.
}

bool player_save_restored_publication_owner::publish_shop(
	const critical_command &original, const critical_completion &completion,
	bool (*native_publish)(const critical_command &, const critical_completion &,
			       void *) noexcept,
	void *context) noexcept
{
#ifdef __NO_MYSQL__
	(void)original;
	(void)completion;
	(void)native_publish;
	(void)context;
	return false;
#else
	if (!nevent_is_game_thread() || !native_publish ||
	    !critical_completion_disposition_valid(completion) ||
	    original.operation_id.bytes != completion.operation_id.bytes)
		return false;
	try
	{
		int pid = 0;
		uint64_t uid = 0, generation = 0;
		critical_command command;
		std::vector<uint8_t> frozen;
		if (!shop_publication_identity(original, &pid, &uid) ||
		    critical_command_encode(original, &frozen) != critical_command_codec_result::ok)
			return false;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			const auto *slot = find_literal_inventory_locked(pid);
			if (!health.initialized || stop_requested || !slot || !slot->held ||
			    !slot->restored_sql_drop ||
			    slot->profile != literal_checkpoint_profile::shop ||
			    slot->payload != frozen ||
			    slot->operation_id.bytes != completion.operation_id.bytes ||
			    find_terminal_fence_locked(pid) ||
			    find_target_save_login_fence_locked(pid) ||
			    append_inflight_pid == pid || any_snapshot_is_retained_locked(pid) ||
			    critical_command_decode(frozen.data(), frozen.size(), &command) !=
				    critical_command_codec_result::ok)
				return false;
			generation = slot->execution_hold_generation;
		}
		player_save_restored_publication_owner owner(
			std::move(command), std::move(frozen), completion,
			player_save_execution_guard::current_ownership_epoch(), pid, generation);
		if (!owner.reservation_.valid() || player_save_worker_pid_pending(pid))
			return false;
		if (completion.disposition == critical_completion_disposition::never_admitted)
			return critical_command_coordinator_cancel_shop_publication(
				owner, native_publish, context);
		// Held native publication proves the frozen inventory/level itself. Dirty
		// volatile STATUS marks remain eligible for a later ordinary capture.
		player_revision_snapshot revision = {};
		if (player_revision_snapshot_copy(pid, &revision) &&
		    (revision.overflowed || revision.queued_components ||
		     revision.inflight_components))
			return false;
		std::vector<player_save_journal_retained_frame> originals;
		if (player_save_journal_collect_publication_frames(
			    pid, owner.reservation_, &originals) != player_save_journal_result::ok)
			return false;
		if (!originals.empty())
		{
			player_save_covered_revision covered;
			if (!player_snapshot_repository_observe_covered_revision(
				    pid, owner.reservation_, &covered) ||
			    player_save_journal_retire_covered_ordinary(pid, owner.reservation_,
									covered, originals) !=
				    player_save_journal_result::ok)
				return false;
		}
		if (player_save_journal_publication_census(pid, owner.reservation_) !=
			    player_save_journal_result::ok ||
		    !native_publish(owner.command_, completion, context) ||
		    !owner.reservation_.valid() ||
		    player_save_journal_publication_census(pid, owner.reservation_) !=
			    player_save_journal_result::ok)
			return false;
		owner.publication_proven_ = true;
		return critical_command_coordinator_acknowledge_publication(owner);
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_restored_publication_owner::publish(const critical_completion &completion) noexcept
{
	if (!nevent_is_game_thread())
		return false;
	try
	{
		critical_command command = {};
		std::vector<uint8_t> frozen;
		int pid = 0;
		uint64_t generation = 0;
		std::unique_lock<std::mutex> lock(pipeline_mutex);
		if (!health.initialized || stop_requested)
			return false;
		for (const auto &checkpoint : literal_inventory_checkpoints)
			if (checkpoint.restored_sql_drop && checkpoint.held &&
			    checkpoint.operation_id.bytes == completion.operation_id.bytes)
			{
				pid = checkpoint.token.pid;
				generation = checkpoint.execution_hold_generation;
				frozen = checkpoint.payload;
				break;
			}
		if (pid <= 0 || find_terminal_fence_locked(pid) ||
		    find_target_save_login_fence_locked(pid) || append_inflight_pid == pid ||
		    any_snapshot_is_retained_locked(pid) ||
		    critical_command_decode(frozen.data(), frozen.size(), &command) !=
			    critical_command_codec_result::ok)
			return false;
		player_save_restored_publication_owner owner(
			std::move(command), std::move(frozen), completion,
			player_save_execution_guard::current_ownership_epoch(), pid, generation);
		lock.unlock();
		const bool coin = owner.command_.type == critical_command_type::coin_transfer;
#ifdef __NO_MYSQL__
		// Only the typed ordinary-room coin scope has a flat native publisher.
		if (!coin)
			return false;
#endif
		if (coin &&
		    !currency_transaction_restored_coin_receipt_current(owner.command_, completion))
			return false;
		if (!owner.reservation_.valid() || player_save_worker_pid_pending(pid))
			return false;
		player_revision_snapshot revision = {};
		if (player_revision_snapshot_copy(pid, &revision) &&
		    (revision.overflowed || revision.dirty_components ||
		     revision.unacknowledged_components || revision.queued_components ||
		     revision.inflight_components ||
		     revision.current_revision != revision.acknowledged_revision))
			return false;
		std::vector<player_save_journal_retained_frame> originals;
		if (player_save_journal_collect_publication_frames(
			    pid, owner.reservation_, &originals) != player_save_journal_result::ok)
			return false;
		if (!originals.empty())
		{
			player_save_covered_revision covered;
			if (!player_snapshot_repository_observe_covered_revision(
				    pid, owner.reservation_, &covered) ||
			    player_save_journal_retire_covered_ordinary(pid, owner.reservation_,
									covered, originals) !=
				    player_save_journal_result::ok)
				return false;
		}
		if (player_save_journal_publication_census(pid, owner.reservation_) !=
		    player_save_journal_result::ok)
			return false;
		bool publication_complete = false;
		if (coin)
			publication_complete =
				coin_physical_recovery_publish(owner.command_, completion);
		else
		{
			const auto published =
				ordinary_drop_recovery_publish(owner.command_, completion);
			publication_complete =
				published.status ==
					ordinary_drop_observation_status::verified_existing ||
				published.status == ordinary_drop_observation_status::published ||
				(published.status ==
					 ordinary_drop_observation_status::verified_rejected &&
				 completion.disposition ==
					 critical_completion_disposition::execution &&
				 completion.outcome == critical_apply_outcome::terminal_failure &&
				 completion.error_code &&
				 completion.failure_stage == critical_failure_stage::none);
		}
		if (!publication_complete ||
		    player_save_journal_publication_census(pid, owner.reservation_) !=
			    player_save_journal_result::ok)
			return false;
		if (coin &&
		    !currency_transaction_restored_coin_receipt_current(owner.command_, completion))
			return false;
		owner.publication_proven_ = true;
		// The original hold/reservation survives fresh proof, confirmed native
		// cleanup and critical checkpoint. Failure retries the whole observation.
		return critical_command_coordinator_acknowledge_publication(owner);
	}
	catch (...)
	{
		return false;
	}
}

bool coin_physical_publication_restore_and_acknowledge(const critical_command &command,
						       const critical_completion &completion)
{
	if (!currency_transaction_restored_coin_receipt_current(command, completion))
		return false;
	// The slot's original immutable encoding must match the domain handoff;
	// an ID-only call cannot select or consume another publication obligation.
	std::vector<uint8_t> frozen;
	if (critical_command_encode(command, &frozen) != critical_command_codec_result::ok)
		return false;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		bool found = false;
		for (const auto &slot : literal_inventory_checkpoints)
			if (slot.restored_sql_drop && slot.held &&
			    slot.operation_id.bytes == command.operation_id.bytes)
			{
				if (slot.payload != frozen)
					return false;
				found = true;
				break;
			}
		if (!found)
			return false;
	}
	return player_save_restored_publication_owner::publish(completion);
}

bool player_save_restored_publication_owner::consume_acknowledged_hold() noexcept
{
	if (flat_shop_restored_)
		return consume_acknowledged_flat_shop_restored_hold();
	if (flat_shop_)
		return consume_acknowledged_flat_shop_hold();
	if (!acknowledged_)
		return false;
	player_save_deferred_identity wake = {};
	bool active = false;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *checkpoint = find_literal_inventory_locked(pid_);
		if (!checkpoint || !checkpoint->restored_sql_drop || !checkpoint->held ||
		    checkpoint->execution_hold_generation != generation_ ||
		    checkpoint->operation_id.bytes != completion_.operation_id.bytes ||
		    checkpoint->payload != frozen_)
		{
			player_save_execution_guard::poison_integrity();
			return false;
		}
		active = player_save_worker_deferred_identity(pid_, &wake);
		if (!reservation_.consume_acknowledged_hold())
		{
			player_save_execution_guard::poison_integrity();
			return false;
		}
		*checkpoint = {};
	}
	if (active)
		(void)player_save_worker_resume_deferred_exact(wake);
	replay_revisit_requested.store(true);
	player_save_execution_guard::signal_ownership_change(
		player_save_execution_guard::current_ownership_epoch());
	return true;
}

player_save_pipeline_result player_save_pipeline_checkpoint_dirty(P_char ch, int save_intent,
								  int room_vnum)
{
	return checkpoint_dirty_with_quest_xp(ch, save_intent, room_vnum, nullptr, 0);
}

/** Mark requested components and capture a checkpoint with the supplied intent and room. */
player_save_pipeline_result player_save_pipeline_request(P_char ch,
							 player_component_mask_t components,
							 int save_intent, int room_vnum)
{
	if (ch && IS_SET(ch->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED))
		return player_save_pipeline_result::unavailable;
	if (ch && player_save_journal_pid_quarantined(GET_PID(ch)))
		return player_save_pipeline_result::unavailable;
	if (!ch || IS_NPC(ch) || !player_save_pipeline_mark(GET_PID(ch), components))
		return player_save_pipeline_result::invalid;
	return player_save_pipeline_checkpoint_dirty(ch, save_intent, room_vnum);
}

player_save_pipeline_result
player_save_pipeline_request_quest_xp(P_char ch, player_component_mask_t components,
				      const player_quest_xp_receipt_snapshot *receipts,
				      size_t receipt_count, int room_vnum)
{
	if (!ch || IS_NPC(ch) || !receipts || !receipt_count || receipt_count > 64 ||
	    !(components & PLAYER_COMPONENT_STATUS) ||
	    (components & ~PLAYER_CHECKPOINT_COMPONENT_ALL) ||
	    IS_SET(ch->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED))
		return player_save_pipeline_result::invalid;
	if (!player_save_pipeline_mark(GET_PID(ch), components))
		return player_save_pipeline_result::unavailable;
	return checkpoint_dirty_with_quest_xp(ch, RENT_CRASH, room_vnum, receipts, receipt_count);
}

player_save_pipeline_result
player_save_pipeline_request_spell_effect(P_char ch, player_component_mask_t components,
					  const player_spell_effect_receipt_snapshot *receipt,
					  int room_vnum)
{
	if (!ch || IS_NPC(ch) || !receipt || !(components & PLAYER_COMPONENT_AFFECTS) ||
	    (components & ~PLAYER_CHECKPOINT_COMPONENT_ALL) ||
	    IS_SET(ch->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED))
		return player_save_pipeline_result::invalid;
	if (!player_save_pipeline_mark(GET_PID(ch), components))
		return player_save_pipeline_result::unavailable;
	return checkpoint_dirty_with_quest_xp(ch, RENT_CRASH, room_vnum, nullptr, 0, receipt);
}

namespace
{
/** Reserve this player's durability fence and mark the revision the caller will wait on. */
bool begin_terminal_fence(int pid, player_revision_t *revision, bool pin_death = false)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (!health.initialized || find_target_save_login_fence_locked(pid))
		return false;
	if (auto *literal = find_literal_inventory_locked(pid))
	{
		if (literal->held)
			return false;
		// No operation was admitted. A terminal intent supersedes this capture
		// request, while its already queued immutable saves retain normal order.
		*literal = {};
	}
	if (terminal_fence *existing = find_terminal_fence_locked(pid);
	    existing && existing->death_pinned)
		return false;
	terminal_fence *fence = allocate_terminal_fence_locked(pid);
	if (!fence)
		return false;
	// Ordinary terminal intents deliberately take a fresh revision on every
	// call. Death retries use the separate pinned request path below.
	if (!player_revision_mark(pid, PLAYER_CHECKPOINT_COMPONENT_ALL, revision))
	{
		clear_terminal_fence_locked(*fence);
		return false;
	}
	if (pin_death && !player_revision_pin_terminal_death(pid, *revision))
	{
		clear_terminal_fence_locked(*fence);
		return false;
	}
	fence->pid = pid;
	fence->revision = *revision;
	fence->journaled = false;
	fence->acknowledged = false;
	fence->death_pinned = pin_death;
	return true;
}

bool retain_and_enqueue_death_snapshot(player_snapshot snapshot, uint64_t corpse_uid,
				       uint64_t wallet_pile_uid,
				       const critical_operation_id &operation_id,
				       resident_claim residence)
{
	const size_t snapshot_bytes = snapshot.encoded_size_bound;
	resident_claim pinned_residence;
	const auto epoch = player_save_execution_guard::current_ownership_epoch();
	if (epoch)
	{
		if (!residence.matches_current(snapshot.pid))
			return false;
		pinned_residence = resident_claim::derive_existing(residence);
		if (!pinned_residence.matches_current(snapshot.pid))
			return false;
	}
	player_snapshot pinned_copy;
	try
	{
		pinned_copy = snapshot;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}

	std::lock_guard<std::mutex> lock(pipeline_mutex);
	terminal_fence *fence = find_terminal_fence_locked(snapshot.pid);
	if (auto *literal = find_literal_inventory_locked(snapshot.pid); literal && literal->held)
		return false;
	if (!health.initialized || stop_requested || !fence || !fence->death_pinned ||
	    fence->revision != snapshot.revision || fence->death_snapshot ||
	    !player_snapshot_is_death_request_schema(snapshot.schema_version) || !snapshot.death ||
	    snapshot.death->operation_id.bytes != operation_id.bytes ||
	    snapshot.death->wallet_pile_uid != wallet_pile_uid || snapshot.death->corpse.empty() ||
	    snapshot.death->corpse.front().object_uid != corpse_uid)
		return false;
	if (pending_append.size() + durable_ready.size() + (append_retry ? 1 : 0) +
			    (append_inflight ? 1 : 0) + pinned_death_count_locked() >=
		    PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS ||
	    retained_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
	    snapshot_bytes > (PLAYER_SAVE_PIPELINE_MAX_BYTES - retained_bytes) / 2)
	{
		++health.overloads;
		return false;
	}
	try
	{
		pending_append.emplace_back(std::move(snapshot), std::move(residence));
	}
	catch (const std::bad_alloc &)
	{
		++health.overloads;
		return false;
	}
	fence->corpse_uid = corpse_uid;
	fence->operation_id = operation_id;
	fence->wallet_pile_uid = pinned_copy.death->wallet_pile_uid;
	fence->death_snapshot.emplace(std::move(pinned_copy));
	fence->death_residence = std::move(pinned_residence);
	retained_bytes += snapshot_bytes * 2;
	++health.captured;
	update_depth_locked();
	append_available.notify_one();
	player_save_execution_guard::signal_ownership_change(
		player_save_execution_guard::current_ownership_epoch());
	return true;
}

bool requeue_pinned_death(int pid, uint64_t corpse_uid);
player_save_terminal_result await_terminal_fence(int pid, player_revision_t revision,
						 uint64_t timeout_msec, bool allow_journal_handoff);
} // namespace

/** Capture fresh terminal intent and wait for its durability fence within the caller timeout. */
player_save_terminal_result player_save_pipeline_terminal(P_char ch, int save_intent, int room_vnum,
							  uint64_t timeout_msec,
							  bool allow_journal_handoff)
{
	if (!ch || IS_NPC(ch) || GET_PID(ch) <= 0 || !timeout_msec)
		return player_save_terminal_result::invalid;
	if (player_save_journal_pid_quarantined(GET_PID(ch)))
		return player_save_terminal_result::unavailable;
	if (IS_SET(ch->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED))
		return player_save_terminal_result::unavailable;
	const int pid = GET_PID(ch);
	resident_claim residence;
	const auto epoch = player_save_execution_guard::current_ownership_epoch();
	if (epoch)
	{
		residence = resident_claim(epoch, pid);
		if (!residence.matches_current(pid))
			return player_save_terminal_result::unavailable;
	}
	player_revision_t revision = 0;
	if (!begin_terminal_fence(pid, &revision, false))
		return player_save_terminal_result::unavailable;
	const auto checkpoint = checkpoint_dirty_with_quest_xp(ch, save_intent, room_vnum, nullptr,
							       0, nullptr, &residence);
	if (trace_player_saves())
		logit(LOG_STATUS,
		      "PLAYER SAVE TRACE: stage=terminal_begin mono_us=%llu pid=%d revision=%llu checkpoint=%u intent=%d timeout_ms=%llu journal_allowed=%d",
		      (unsigned long long)persistence_observability_now_usec(), pid,
		      (unsigned long long)revision, (unsigned)checkpoint, save_intent,
		      (unsigned long long)timeout_msec, allow_journal_handoff);
	return await_terminal_fence(pid, revision, timeout_msec, allow_journal_handoff);
}

/** Record one immutable death disposition; retries resume its exact pinned bytes. */
player_save_terminal_result
player_save_pipeline_terminal_death(P_char ch, P_obj corpse, P_obj wallet_pile,
				    const critical_operation_id &operation_id, int room_vnum,
				    uint64_t timeout_msec, bool allow_journal_handoff)
{
	if (!ch || IS_NPC(ch) || GET_PID(ch) <= 0 || !timeout_msec)
		return player_save_terminal_result::invalid;
	const player_save_terminal_result resumed =
		player_save_pipeline_terminal_death_resume(ch, 0, timeout_msec);
	if (resumed != player_save_terminal_result::not_pending)
		return resumed;
	if (!corpse)
		return player_save_terminal_result::invalid;
	if (player_save_journal_pid_quarantined(GET_PID(ch)))
		return player_save_terminal_result::unavailable;
	// A payload-gap load keeps its valid item graph read-only. Its only safe
	// terminal write is the immutable death disposition, which records that
	// graph and quarantines matching durable custody. Every other degraded load
	// may be missing state the disposition cannot reconstruct.
	if (IS_SET(ch->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED) &&
	    !IS_SET(ch->runtime_flags, CHAR_RFLAG_LOAD_ITEM_PAYLOAD_GAP))
		return player_save_terminal_result::unavailable;
	const int pid = GET_PID(ch);
	const auto epoch = player_save_execution_guard::current_ownership_epoch();
	resident_claim residence;
	if (epoch)
	{
		residence = resident_claim(epoch, pid);
		if (!residence.matches_current(pid))
			return player_save_terminal_result::unavailable;
	}
	player_revision_t revision = 0;
	if (!begin_terminal_fence(pid, &revision, true))
		return player_save_terminal_result::unavailable;
	struct unqueued_fence_guard
	{
		int pid;
		bool queued = false;
		~unqueued_fence_guard()
		{
			if (!queued)
			{
				std::lock_guard<std::mutex> lock(pipeline_mutex);
				if (terminal_fence *fence = find_terminal_fence_locked(pid))
					clear_terminal_fence_locked(*fence);
			}
		}
	} guard{ pid };
	player_snapshot snapshot;
	if (player_death_snapshot_capture(ch, corpse, wallet_pile, operation_id, revision,
					  room_vnum, {},
					  &snapshot) != player_snapshot_capture_result::ok ||
	    !player_snapshot_is_death_request_schema(snapshot.schema_version) || !snapshot.death)
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		++health.capture_failures;
		return player_save_terminal_result::invalid;
	}
	player_snapshot pending_effects;
	player_component_mask_t required_components = 0;
	if (!quest_reward_recovery_pending_save_receipts(pid, &pending_effects.quest_xp_receipts,
							 &required_components) ||
	    !merge_quest_xp_receipts(&snapshot, pending_effects) ||
	    !spell_component_retirement_pending_save_receipts(
		    pid, &pending_effects.spell_effect_receipts) ||
	    !merge_spell_effect_receipts(&snapshot, pending_effects) ||
	    !craft_progression_pending_save_receipts(pid, &pending_effects.craft_receipts) ||
	    !merge_craft_receipts(&snapshot, pending_effects))
		return player_save_terminal_result::unavailable;
	std::vector<uint8_t> encoded;
	if (player_snapshot_encode(snapshot, &encoded) != player_snapshot_codec_result::ok)
		return player_save_terminal_result::invalid;
	player_revision_t queued_revision = 0;
	player_component_mask_t components = 0;
	if (!player_revision_queue(pid, &queued_revision, &components) ||
	    queued_revision != revision || components != snapshot.components)
		return player_save_terminal_result::unavailable;
	const uint64_t wallet_pile_uid = wallet_pile ? wallet_pile->obj_uid : 0;
	if (!retain_and_enqueue_death_snapshot(std::move(snapshot), corpse->obj_uid,
					       wallet_pile_uid, operation_id, std::move(residence)))
		return player_save_terminal_result::unavailable;
	guard.queued = true;
	char correlation[33] = {};
	death_recovery_correlation((static_cast<uint64_t>(pid) << 32) |
					   static_cast<uint32_t>(corpse->value[CORPSE_SAVEID]),
				   correlation);
	if (trace_player_saves())
		logit(LOG_STATUS,
		      "PLAYER SAVE TRACE: correlation=%s stage=terminal_death_begin mono_us=%llu pid=%d revision=%llu room=%d timeout_ms=%llu journal_allowed=0",
		      correlation, (unsigned long long)persistence_observability_now_usec(), pid,
		      (unsigned long long)revision, room_vnum, (unsigned long long)timeout_msec);
	(void)allow_journal_handoff;
	return await_terminal_fence(pid, revision, timeout_msec, false);
}

player_save_terminal_result
player_save_pipeline_terminal_death_resume(P_char ch, uint64_t corpse_uid, uint64_t timeout_msec)
{
	if (!ch || IS_NPC(ch) || GET_PID(ch) <= 0 || !timeout_msec)
		return player_save_terminal_result::invalid;
	const int pid = GET_PID(ch);
	player_revision_t revision = 0;
	bool acknowledged = false;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		terminal_fence *fence = find_terminal_fence_locked(pid);
		if (!fence || !fence->death_pinned)
			return player_save_terminal_result::not_pending;
		if ((corpse_uid && corpse_uid != fence->corpse_uid) ||
		    !death_snapshot_identity_matches(*fence))
			return player_save_terminal_result::unavailable;
		revision = fence->revision;
		acknowledged = fence->acknowledged;
	}
	if (!acknowledged && !requeue_pinned_death(pid, corpse_uid))
		return player_save_terminal_result::unavailable;
	return await_terminal_fence(pid, revision, timeout_msec, false);
}

namespace
{
/** Requeue only the pinned immutable bytes after the worker has released a failed slot. */
bool requeue_pinned_death(int pid, uint64_t corpse_uid)
{
	resident_claim residence;
	player_snapshot retry_snapshot;
	player_revision_t revision = 0;
	size_t snapshot_bytes = 0;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		terminal_fence *fence = find_terminal_fence_locked(pid);
		if (!fence || !fence->death_pinned ||
		    (corpse_uid && corpse_uid != fence->corpse_uid) ||
		    !death_snapshot_identity_matches(*fence))
			return false;
		if (fence->acknowledged || snapshot_is_retained_locked(pid, fence->revision) ||
		    (append_inflight_pid == pid && append_inflight_revision == fence->revision))
			return true;
		const auto epoch = player_save_execution_guard::current_ownership_epoch();
		if (epoch)
		{
			residence = resident_claim::derive_existing(fence->death_residence);
			if (!residence.matches_current(pid))
				return false;
		}
		revision = fence->revision;
		snapshot_bytes = fence->death_snapshot->encoded_size_bound;
		try
		{
			retry_snapshot = *fence->death_snapshot;
		}
		catch (const std::bad_alloc &)
		{
			++health.overloads;
			return false;
		}
	}
	if (player_save_worker_pid_pending(pid))
		return true;
	player_revision_snapshot current = {};
	if (!player_revision_snapshot_copy(pid, &current) || current.current_revision != revision ||
	    current.queued_revision != revision ||
	    current.queued_components != retry_snapshot.components || current.inflight_components)
		return false;

	std::lock_guard<std::mutex> lock(pipeline_mutex);
	terminal_fence *fence = find_terminal_fence_locked(pid);
	if (!fence || !fence->death_pinned || fence->revision != revision ||
	    !death_snapshot_identity_matches(*fence))
		return false;
	if (fence->acknowledged || snapshot_is_retained_locked(pid, revision) ||
	    (append_inflight_pid == pid && append_inflight_revision == revision))
		return true;
	if (pending_append.size() + durable_ready.size() + (append_retry ? 1 : 0) +
			    (append_inflight ? 1 : 0) + pinned_death_count_locked() >=
		    PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS ||
	    retained_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
	    snapshot_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES - retained_bytes)
	{
		++health.overloads;
		return false;
	}
	try
	{
		pending_append.emplace_back(std::move(retry_snapshot), std::move(residence));
	}
	catch (const std::bad_alloc &)
	{
		++health.overloads;
		return false;
	}
	retained_bytes += snapshot_bytes;
	++health.terminal_death_requeues;
	update_depth_locked();
	append_available.notify_one();
	player_save_execution_guard::signal_ownership_change(
		player_save_execution_guard::current_ownership_epoch());
	return true;
}

bool acknowledge_terminal_fence_completion_locked(const player_save_completion &completion)
{
	terminal_fence *fence = find_terminal_fence_locked(completion.pid);
	if (!fence || fence->revision != completion.revision)
		return false;
	const bool applied = completion.outcome == player_save_apply_outcome::applied ||
			     completion.outcome == player_save_apply_outcome::already_applied;
	if (fence->death_pinned)
	{
		if (!applied || completion.durable_revision != fence->revision ||
		    !death_snapshot_identity_matches(*fence) ||
		    completion.components != fence->death_snapshot->components)
			return false;
		fence->acknowledged = true;
		return true;
	}
	if ((applied || completion.outcome == player_save_apply_outcome::stale_revision) &&
	    completion.durable_revision >= fence->revision)
		fence->acknowledged = true;
	return fence->acknowledged;
}

/** Pump the pipeline until this player revision is durable or the deadline passes. */
player_save_terminal_result await_terminal_fence(int pid, player_revision_t revision,
						 uint64_t timeout_msec, bool allow_journal_handoff)
{
	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_msec);
	while (std::chrono::steady_clock::now() < deadline)
	{
		player_save_pipeline_pulse();
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			terminal_fence *fence = find_terminal_fence_locked(pid);
			if (!fence || fence->revision != revision)
				return player_save_terminal_result::unavailable;
			if (fence->acknowledged)
			{
				++health.terminal_database_acks;
				clear_terminal_fence_locked(*fence);
				return player_save_terminal_result::database_acknowledged;
			}
			if (!fence->death_pinned && allow_journal_handoff && fence->journaled)
			{
				++health.terminal_journal_handoffs;
				clear_terminal_fence_locked(*fence);
				return player_save_terminal_result::journal_durable;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (trace_player_saves())
	{
		const terminal_fence *fence = find_terminal_fence_locked(pid);
		player_revision_snapshot current = {};
		player_revision_snapshot_copy(pid, &current);
		logit(LOG_STATUS,
		      "PLAYER SAVE TRACE: stage=terminal_timeout mono_us=%llu pid=%d target=%llu fence=%llu journaled=%d acknowledged=%d current=%llu ack=%llu dirty=%llu queued=%llu inflight=%llu append=%llu ready=%llu append_failures=%llu",
		      (unsigned long long)persistence_observability_now_usec(), pid,
		      (unsigned long long)revision,
		      (unsigned long long)(fence ? fence->revision : 0), fence && fence->journaled,
		      fence && fence->acknowledged, (unsigned long long)current.current_revision,
		      (unsigned long long)current.acknowledged_revision,
		      (unsigned long long)current.dirty_components,
		      (unsigned long long)current.queued_revision,
		      (unsigned long long)current.inflight_revision,
		      (unsigned long long)health.pending_append,
		      (unsigned long long)health.durable_ready,
		      (unsigned long long)health.append_failures);
	}
	persistence_trace_event timeout_trace;
	timeout_trace.stage = persistence_trace_stage::save_timeout;
	timeout_trace.pid = pid;
	timeout_trace.revision = revision;
	timeout_trace.incident = true;
	persistence_trace_record(timeout_trace);
	++health.terminal_timeouts;
	return player_save_terminal_result::timed_out;
}
} // namespace

/** Apply bounded worker completions and submit journaled snapshots from the game thread. */
void player_save_pipeline_pulse(void)
{
	player_save_owned_completion owned_completions[PLAYER_SAVE_PIPELINE_PULSE_BUDGET] = {};
	int32_t missing_baseline[PLAYER_SAVE_PIPELINE_PULSE_BUDGET] = {};
	size_t missing_baseline_count = 0;
	struct custody_diagnostic
	{
		int32_t pid = 0;
		player_revision_t revision = 0;
		player_component_mask_t components = 0;
		player_save_custody_diagnosis custody_diagnosis =
			player_save_custody_diagnosis::none;
	};
	custody_diagnostic custody_mismatches[PLAYER_SAVE_PIPELINE_PULSE_BUDGET] = {};
	bool custody_recapture_allowed[PLAYER_SAVE_PIPELINE_PULSE_BUDGET] = {};
	size_t custody_mismatch_count = 0;
	const size_t completed = player_save_worker_pulse_owned(owned_completions,
								PLAYER_SAVE_PIPELINE_PULSE_BUDGET);
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		for (auto &literal : literal_inventory_checkpoints)
			if (literal.token.pid && !literal.held)
			{
				P_char actor = find_character_by_runtime_id(
					literal.token.actor_runtime_id);
				if (!literal_checkpoint_actor_matches(literal, actor))
					literal = {};
			}
		health.completions += completed;
		for (size_t index = 0; index < completed; ++index)
		{
			if (owned_completions[index].completion.outcome ==
				    player_save_apply_outcome::applied ||
			    owned_completions[index].completion.outcome ==
				    player_save_apply_outcome::already_applied)
				custody_recapture_armed.erase(
					owned_completions[index].completion.pid);
			if (trace_player_saves())
			{
				const auto &completion = owned_completions[index].completion;
				logit(LOG_STATUS,
				      "PLAYER SAVE TRACE: stage=completion mono_us=%llu pid=%d revision=%llu components=%llu outcome=%u durable=%llu error=%u custody_diagnosis=%s retries=%u queued_us=%llu started_us=%llu completed_us=%llu",
				      (unsigned long long)persistence_observability_now_usec(),
				      completion.pid, (unsigned long long)completion.revision,
				      (unsigned long long)completion.components,
				      (unsigned)completion.outcome,
				      (unsigned long long)completion.durable_revision,
				      completion.error_code,
				      player_save_custody_diagnosis_name(
					      completion.custody_diagnosis),
				      completion.retry_count,
				      (unsigned long long)completion.queued_at_usec,
				      (unsigned long long)completion.started_at_usec,
				      (unsigned long long)completion.completed_at_usec);
			}
			(void)acknowledge_terminal_fence_completion_locked(
				owned_completions[index].completion);
			const auto &completion = owned_completions[index].completion;
			if (auto *literal = find_literal_inventory_locked(completion.pid);
			    literal && !literal->held &&
			    literal->captured_revision == completion.revision &&
			    completion.durable_revision == completion.revision &&
			    !completion.error_code &&
			    (completion.components & literal_checkpoint_components(*literal)) ==
				    literal_checkpoint_components(*literal) &&
			    (completion.outcome == player_save_apply_outcome::applied ||
			     completion.outcome == player_save_apply_outcome::already_applied))
				literal->acknowledged_revision = completion.revision;
			// The worker only ever UPDATEs player_data. A missing row means the
			// character never got its baseline INSERT, and every further async
			// save would fail the same way; record it for the sync fallback.
			if (owned_completions[index].completion.outcome ==
				    player_save_apply_outcome::terminal_failure &&
			    owned_completions[index].completion.error_code == ENOENT &&
			    owned_completions[index].completion.pid > 0)
				missing_baseline[missing_baseline_count++] =
					owned_completions[index].completion.pid;
			if (owned_completions[index].completion.outcome ==
				    player_save_apply_outcome::terminal_failure &&
			    owned_completions[index].completion.error_code ==
				    PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH &&
			    owned_completions[index].completion.pid > 0)
			{
				custody_mismatches[custody_mismatch_count] = {
					completion.pid, completion.revision, completion.components,
					completion.custody_diagnosis
				};
				try
				{
					custody_recapture_allowed[custody_mismatch_count] =
						custody_recapture_armed.insert(completion.pid)
							.second;
				}
				catch (const std::bad_alloc &)
				{
					++health.overloads;
				}
				++custody_mismatch_count;
			}
		}
	}
	for (size_t index = 0; index < completed; ++index)
	{
		if (craft_progression_hooks.saved)
		{
			if (!owned_completions[index].completion.craft_receipts.empty())
				craft_progression_hooks.saved(
					owned_completions[index].completion.pid, true,
					owned_completions[index].completion.craft_receipts.data(),
					owned_completions[index].completion.craft_receipts.size());
			if (!owned_completions[index].completion.failed_craft_receipts.empty())
				craft_progression_hooks.saved(
					owned_completions[index].completion.pid, false,
					owned_completions[index]
						.completion.failed_craft_receipts.data(),
					owned_completions[index]
						.completion.failed_craft_receipts.size());
		}
		if (!owned_completions[index].completion.quest_xp_receipts.empty())
			quest_reward_recovery_save_acknowledged(
				owned_completions[index].completion.pid,
				owned_completions[index].completion.revision,
				owned_completions[index].completion.quest_xp_receipts.data(),
				owned_completions[index].completion.quest_xp_receipts.size());
		if (!owned_completions[index].completion.spell_effect_receipts.empty())
			spell_component_retirement_save_completed(
				owned_completions[index].completion.pid, true,
				owned_completions[index].completion.spell_effect_receipts.data(),
				owned_completions[index].completion.spell_effect_receipts.size());
		if (!owned_completions[index].completion.failed_spell_effect_receipts.empty())
			spell_component_retirement_save_completed(
				owned_completions[index].completion.pid, false,
				owned_completions[index]
					.completion.failed_spell_effect_receipts.data(),
				owned_completions[index]
					.completion.failed_spell_effect_receipts.size());
	}
	for (size_t index = 0; index < custody_mismatch_count; ++index)
	{
		bool recapture_scheduled = false;
		for (P_char ch = custody_recapture_allowed[index] ? character_list : NULL; ch;
		     ch = ch->next)
			if (IS_PC(ch) && GET_PID(ch) == custody_mismatches[index].pid &&
			    GET_STAT(ch) != STAT_DEAD &&
			    !IS_SET(ch->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED))
			{
				/* Custody may advance after a snapshot is sealed (for example,
				 * an item granted during login).  Preserve the rejection, then
				 * capture the current graph instead of retrying stale bytes or
				 * requiring the player to issue a manual save. */
				persistence_schedule_character_save(ch, RENT_CRASH, 2,
								    "custody-mismatch-recapture");
				recapture_scheduled = true;
				break;
			}
		persistence_alert(
			AVATAR, "player_save", "redacted", "none", "none",
			"custody_payload_mismatch_rejected",
			"pid=%d revision=%llu components=%llu destructive_write=0 "
			"custody_diagnosis_code=%u recapture_scheduled=%d",
			custody_mismatches[index].pid,
			(unsigned long long)custody_mismatches[index].revision,
			(unsigned long long)custody_mismatches[index].components,
			static_cast<unsigned>(custody_mismatches[index].custody_diagnosis),
			recapture_scheduled ? 1 : 0);
	}
	for (size_t index = 0; index < missing_baseline_count; ++index)
	{
		const int32_t pid = missing_baseline[index];
		bool rearmed = false;
		for (P_char ch = character_list; ch; ch = ch->next)
			if (IS_PC(ch) && GET_PID(ch) == pid)
			{
				SET_BIT(ch->runtime_flags, CHAR_RFLAG_NO_DB_BASELINE);
				rearmed = true;
				break;
			}
		logit(LOG_PLAYER,
		      "player_save_pipeline_pulse: component=apply outcome=missing_baseline "
		      "pid=%d sync_fallback=%d",
		      pid, rearmed ? 1 : 0);
	}
	for (size_t count = 0; count < PLAYER_SAVE_PIPELINE_PULSE_BUDGET; ++count)
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (durable_ready.empty())
			break;
		auto selected = durable_ready.begin();
		size_t snapshot_bytes = 0;
		int trace_pid = 0;
		player_revision_t trace_revision = 0;
		player_save_submit_result submitted = player_save_submit_result::replay_busy;
		for (size_t visited = 0; selected != durable_ready.end() &&
					 visited < PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS;
		     ++visited, ++selected)
		{
			snapshot_bytes = selected->body.encoded_size_bound;
			trace_pid = selected->body.pid;
			trace_revision = selected->body.revision;
			submitted = player_save_worker_submit_owned_retained(&selected->body,
									     selected->residence);
			if (submitted != player_save_submit_result::replay_busy)
				break;
		}
		if (selected == durable_ready.end() ||
		    submitted == player_save_submit_result::replay_busy)
			break;

		if (trace_player_saves())
			logit(LOG_STATUS,
			      "PLAYER SAVE TRACE: stage=submit mono_us=%llu pid=%d revision=%llu outcome=%u append=%llu ready=%llu",
			      (unsigned long long)persistence_observability_now_usec(), trace_pid,
			      (unsigned long long)trace_revision, (unsigned)submitted,
			      (unsigned long long)health.pending_append,
			      (unsigned long long)health.durable_ready);
		if (submitted == player_save_submit_result::worker_unavailable ||
		    submitted == player_save_submit_result::capacity_exceeded)
			break;
		retained_bytes -= snapshot_bytes;
		durable_ready.erase(selected);
		if (submitted == player_save_submit_result::durably_spilled)
			++health.durable_spills;
		else
			++health.dispatched;
		update_depth_locked();
	}
}

void player_save_pipeline_quiesce(void)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	accepting = false;
	update_depth_locked();
}

void player_save_pipeline_resume(void)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (health.initialized && execution_started && !stop_requested && !lifecycle_stop_attempted)
	{
		lifecycle_admission_closed = false;
		accepting = true;
	}
	update_depth_locked();
}

bool player_save_pipeline_drain(uint64_t timeout_msec)
{
	if (!timeout_msec)
		return false;
	player_save_pipeline_quiesce();
	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_msec);
	while (std::chrono::steady_clock::now() < deadline)
	{
		player_save_pipeline_pulse();
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			if (pending_append.empty() && !append_retry && !append_inflight)
				return true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	++health.drain_failures;
	return false;
}

namespace
{
bool owned_pipeline_idle_locked()
{
	if (!pending_append.empty() || append_retry || append_inflight || !durable_ready.empty() ||
	    retained_bytes || append_inflight_pid || append_inflight_revision)
		return false;
	for (const auto &fence : terminal_fences)
		if (fence.death_pinned || fence.death_snapshot || fence.death_residence)
			return false;
	for (const auto &literal : literal_inventory_checkpoints)
		if (literal.token.pid || literal.held || literal.execution_hold_generation)
			return false;
	return true;
}

bool owned_coordinator_idle()
{
	const auto state = critical_command_coordinator_health_copy();
	return !state.queued && !state.inflight && !state.blocked && !state.publication_pending &&
	       !state.native_continuation_pending && !state.awaiting_durability &&
	       !state.admission_queue_bytes && !state.append_inflight;
}

bool owned_lifecycle_idle(uint64_t epoch)
{
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!health.initialized || !owned_pipeline_idle_locked())
			return false;
	}
	if (!player_save_worker_idle() || !owned_coordinator_idle() ||
	    !player_save_execution_guard::ownership_epoch_quiescent(epoch))
		return false;
	// A cached frame list or scalar revision is not a namespace proof. Retained
	// quarantine/policy originals stay byte-for-byte on disk across clean close.
	std::vector<player_save_journal_retained_frame> frames;
	if (player_save_journal_collect_lifecycle_frames(&frames) != player_save_journal_result::ok)
		return false;
	for (const auto &frame : frames)
		if (!frame.quarantined && !frame.policy_fenced)
			return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	return owned_pipeline_idle_locked() && player_save_worker_idle() &&
	       player_save_execution_guard::ownership_epoch_quiescent(epoch);
}
} // namespace

bool player_save_pipeline_drain_owned(uint64_t timeout_msec)
{
	const auto epoch = player_save_execution_guard::current_ownership_epoch();
	if (!epoch || !timeout_msec || !nevent_is_game_thread() ||
	    !critical_command_coordinator_lifecycle_guard_held_by_current_thread())
		return false;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		lifecycle_admission_closed = true;
		accepting = false;
		if (!health.initialized || stop_requested || !execution_started)
			return false;
		update_depth_locked();
	}
	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_msec);
	try
	{
		while (std::chrono::steady_clock::now() < deadline)
		{
			player_save_pipeline_pulse();
			const auto now = std::chrono::steady_clock::now();
			if (now >= deadline)
				break;
			const auto remaining =
				std::chrono::duration_cast<std::chrono::milliseconds>(deadline -
										      now)
					.count();
			// Its game-thread observer pumps save receipts before publication,
			// even when the critical completion batch is empty.
			if (!critical_command_coordinator_drain(static_cast<uint64_t>(remaining)))
				break;
			if (owned_lifecycle_idle(epoch) &&
			    std::chrono::steady_clock::now() < deadline)
				return true;
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	catch (...)
	{
		// An unavailable proof or callback leaves the original owners running;
		// it never authorizes stop, epoch end or journal namespace teardown.
	}
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	++health.drain_failures;
	return false;
}

bool player_save_pipeline_shutdown_owned(void)
{
	std::lock_guard<std::mutex> lifecycle(player_save_pipeline_lifecycle_detail::mutex);
	const auto epoch = player_save_execution_guard::current_ownership_epoch();
	if (!epoch || !nevent_is_game_thread() ||
	    !critical_command_coordinator_lifecycle_guard_held_by_current_thread())
		return false;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		lifecycle_admission_closed = true;
		accepting = false;
		update_depth_locked();
	}
	try
	{
		// No callbacks or native work are pumped after destructive world teardown.
		if (!owned_lifecycle_idle(epoch))
			return false;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			if (!owned_pipeline_idle_locked())
				return false;
			lifecycle_stop_attempted = true;
			stop_requested = true;
			replay_gate.begin_replay();
			append_available.notify_all();
		}
		player_save_execution_guard::signal_ownership_change(epoch);
		if (dispatcher.joinable())
			dispatcher.join();
		if (!player_save_worker_shutdown_if_idle() || !owned_lifecycle_idle(epoch) ||
		    !player_save_execution_guard::end_ownership_epoch(epoch))
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			shutdown_incomplete = true;
			return false;
		}
		// Namespace cleanup cannot clear owners while this epoch still exists.
		player_save_journal_shutdown();
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		health.initialized = false;
		execution_started = false;
		shutdown_incomplete = false;
		update_depth_locked();
		return true;
	}
	catch (...)
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		shutdown_incomplete = true;
		return false;
	}
}

player_save_pipeline_health player_save_pipeline_health_copy(void)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	return health;
}

player_save_pipeline_diagnostic player_save_pipeline_diagnostic_copy(int pid)
{
	player_save_pipeline_diagnostic result;
	std::unique_lock<std::mutex> lock(pipeline_mutex, std::try_to_lock);
	if (!lock.owns_lock())
		return result;
	result.available = true;
	result.health = health;
	const terminal_fence *fence = find_terminal_fence_locked(pid);
	const auto *literal = find_literal_inventory_locked(pid);
	result.pid_admission_open = !find_target_save_login_fence_locked(pid) &&
				    !(fence && fence->death_pinned) && !(literal && literal->held);
	result.retained_save = !health.initialized || stop_requested || !accepting || fence ||
			       literal || append_inflight_pid == pid ||
			       any_snapshot_is_retained_locked(pid);
	return result;
}

bool player_save_pipeline_loads_allowed(void)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (lifecycle_admission_closed)
		return false;
	return replay_gate.loads_allowed();
}

size_t player_save_pipeline_dirty_count(void)
{
	return player_revision_dirty_count();
}

/** Classify save intents that do not require a terminal durability fence. */
bool player_save_pipeline_is_nonterminal_type(int save_intent)
{
	return save_intent != RENT_INN && save_intent != RENT_LINKDEAD &&
	       save_intent != RENT_CAMPED && save_intent != RENT_DEATH &&
	       save_intent != RENT_POOFARTI && save_intent != RENT_SWAPARTI &&
	       save_intent != RENT_FIGHTARTI;
}

bool player_save_pipeline_sealed_save_pending(int pid)
{
	if (pid <= 0 || player_save_journal_pid_quarantined(pid))
		return true;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!health.initialized || stop_requested || !accepting ||
		    find_target_save_login_fence_locked(pid) || find_terminal_fence_locked(pid) ||
		    find_literal_inventory_locked(pid) || append_inflight_pid == pid ||
		    any_snapshot_is_retained_locked(pid))
			return true;
	}
	if (player_save_worker_pid_pending(pid))
		return true;
	player_revision_snapshot revision = {};
	if (!player_revision_snapshot_copy(pid, &revision))
		return false;
	return revision.overflowed || revision.queued_components || revision.inflight_components;
}

bool player_save_pipeline_target_save_pending(int pid)
{
	if (pid <= 0)
		return true;
	if (player_save_journal_pid_quarantined(pid))
		return true;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!health.initialized || stop_requested || !accepting ||
		    find_terminal_fence_locked(pid) || find_literal_inventory_locked(pid) ||
		    append_inflight_pid == pid || any_snapshot_is_retained_locked(pid))
			return true;
	}
	if (player_save_worker_pid_pending(pid))
		return true;
	player_revision_snapshot revision = {};
	if (!player_revision_snapshot_copy(pid, &revision))
		return false;
	return revision.overflowed || revision.dirty_components ||
	       revision.unacknowledged_components || revision.queued_components ||
	       revision.inflight_components ||
	       revision.current_revision != revision.acknowledged_revision;
}

bool player_save_pipeline_acquire_target_save_login_fence(int pid,
							  player_revision_t expected_revision)
{
	if (pid <= 0 || expected_revision == std::numeric_limits<player_revision_t>::max())
		return false;
	if (player_save_journal_pid_quarantined(pid))
		return false;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!health.initialized || stop_requested || !accepting ||
		    find_terminal_fence_locked(pid) || find_target_save_login_fence_locked(pid) ||
		    find_literal_inventory_locked(pid) || append_inflight_pid == pid ||
		    any_snapshot_is_retained_locked(pid) ||
		    !allocate_target_save_login_fence_locked(pid, expected_revision))
			return false;
	}

	// Reserve before checking the worker/revision state. New marks are rejected
	// while reserved, so the final check cannot race a newly admitted save.
	if (player_save_worker_pid_pending(pid))
	{
		player_save_pipeline_release_target_save_login_fence(pid, expected_revision);
		return false;
	}
	player_revision_snapshot revision = {};
	if (player_revision_snapshot_copy(pid, &revision) &&
	    (revision.overflowed || revision.dirty_components ||
	     revision.unacknowledged_components || revision.queued_components ||
	     revision.inflight_components || revision.current_revision != expected_revision ||
	     revision.acknowledged_revision != expected_revision))
	{
		player_save_pipeline_release_target_save_login_fence(pid, expected_revision);
		return false;
	}

	std::lock_guard<std::mutex> lock(pipeline_mutex);
	target_save_login_fence *fence = find_target_save_login_fence_locked(pid);
	if (!fence || fence->expected_revision != expected_revision || append_inflight_pid == pid ||
	    any_snapshot_is_retained_locked(pid))
	{
		if (fence && fence->expected_revision == expected_revision)
			*fence = {};
		return false;
	}
	return true;
}

void player_save_pipeline_release_target_save_login_fence(int pid,
							  player_revision_t expected_revision)
{
	if (pid <= 0)
		return;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (target_save_login_fence *fence = find_target_save_login_fence_locked(pid))
		if (fence->expected_revision == expected_revision)
			*fence = {};
}

bool player_save_pipeline_target_save_login_fenced(int pid)
{
	if (pid <= 0)
		return false;
	if (player_save_journal_pid_quarantined(pid))
		return true;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	return find_target_save_login_fence_locked(pid) != nullptr;
}

bool player_save_pipeline_save_admitted(int pid)
{
	if (pid <= 0)
		return false;
	if (player_save_journal_pid_quarantined(pid))
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (lifecycle_admission_closed || (health.initialized && !execution_started))
		return false;
	if (find_target_save_login_fence_locked(pid))
		return false;
	if (auto *literal = find_literal_inventory_locked(pid); literal && literal->held)
		return false;
	if (terminal_fence *fence = find_terminal_fence_locked(pid); fence && fence->death_pinned)
		return false;
	return true;
}

bool player_save_pipeline_authoritative_hydration_admitted(int pid)
{
	if (pid <= 0 || player_save_journal_pid_quarantined(pid))
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	if (lifecycle_admission_closed || (health.initialized && !execution_started))
		return false;
	if (find_target_save_login_fence_locked(pid))
		return false;
	if (auto *literal = find_literal_inventory_locked(pid);
	    literal && literal->held && !literal->restored_sql_drop)
		return false;
#ifdef __NO_MYSQL__
	if (auto *literal = find_literal_inventory_locked(pid); literal && literal->held)
	{
		try
		{
			critical_command original;
			int original_pid = 0;
			uint64_t root_uid = 0;
			if (critical_command_decode(literal->payload.data(),
						    literal->payload.size(), &original) !=
				    critical_command_codec_result::ok ||
			    !coin_physical_recovery_identity(original, &original_pid, &root_uid) ||
			    original_pid != pid || root_uid != literal->token.root_uid)
				return false;
		}
		catch (...)
		{
			return false;
		}
	}
#endif
	if (terminal_fence *fence = find_terminal_fence_locked(pid); fence && fence->death_pinned)
		return false;
	return true;
}

/** Stop the pipeline and clear worker, revision, and health state for an isolated test. */
void player_save_pipeline_reset_for_tests(void)
{
	player_save_pipeline_shutdown();
	std::lock_guard<std::mutex> lifecycle(player_save_pipeline_lifecycle_detail::mutex);
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (shutdown_incomplete)
			return;
	}
	player_save_worker_reset_for_tests();
	player_revision_reset_for_tests();
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	health = {};
	lifecycle_admission_closed = false;
	lifecycle_stop_attempted = false;
	replay_gate.begin_replay();
	stop_requested = false;
	accepting = false;
	append_inflight = false;
	append_inflight_pid = 0;
	append_inflight_revision = 0;
	target_save_login_fences.fill({});
}

namespace
{
player_literal_inventory_token
native_quest_checkpoint_identity(const player_native_quest_checkpoint_token &token)
{
	return { token.pid, token.actor_runtime_id, token.root_uid, token.generation };
}
[[maybe_unused]] bool native_quest_actor_matches(const player_native_quest_checkpoint_token &token,
						 P_char actor)
{
	return actor && IS_PC(actor) && actor->only.pc && token.pid > 0 &&
	       GET_PID(actor) == token.pid && token.actor_runtime_id && token.generation &&
	       actor->runtime_id == token.actor_runtime_id &&
	       find_character_by_runtime_id(token.actor_runtime_id) == actor &&
	       !IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED);
}
}

static player_literal_inventory_state
native_quest_checkpoint_begin(P_char actor, P_obj root, int room_vnum,
			      player_native_quest_checkpoint_token *token_out, bool money_only)
{
#ifdef __NO_MYSQL__
	(void)actor;
	(void)root;
	(void)room_vnum;
	(void)token_out;
	(void)money_only;
	return player_literal_inventory_state::refused;
#else
	if (!actor || !token_out || !IS_PC(actor) || !actor->only.pc || GET_PID(actor) <= 0 ||
	    !actor->runtime_id || find_character_by_runtime_id(actor->runtime_id) != actor ||
	    !economic_gameplay_authority::active_regular_sql() ||
	    (root && (!root->obj_uid || !OBJ_CARRIED_BY(root, actor))) ||
	    IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED) ||
	    player_save_journal_pid_quarantined(GET_PID(actor)))
		return player_literal_inventory_state::refused;
	if (money_only && root)
		return player_literal_inventory_state::refused;
	const uint64_t root_uid = root ? root->obj_uid : 0;
	const auto components = money_only ? SHOP_CHECKPOINT_COMPONENTS :
					     LITERAL_INVENTORY_COMPONENTS;
	const uint64_t wallet_revision = money_only ? actor->only.pc->wallet_revision : 0;
	std::array<int64_t, 4> cash{};
	player_snapshot captured;
	std::vector<uint8_t> blob;
	uint32_t level = 0;
	if (player_snapshot_capture_literal_inventory(actor, 1, components, RENT_CRASH, room_vnum,
						      root_uid, &captured) !=
		    player_snapshot_capture_result::ok ||
	    !native_quest_checkpoint_blob(captured, &blob, &level) ||
	    (money_only && (!native_money_snapshot_cash(captured, &cash) ||
			    actor->only.pc->wallet_revision != wallet_revision)))
		return player_literal_inventory_state::refused;
	player_native_quest_checkpoint_token token;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!economic_gameplay_authority::active_regular_sql() || !health.initialized ||
		    stop_requested || !accepting || !health.replay_complete ||
		    health.replay_blocked || find_target_save_login_fence_locked(GET_PID(actor)) ||
		    find_terminal_fence_locked(GET_PID(actor)))
			return player_literal_inventory_state::refused;
		if (auto *existing = find_literal_inventory_locked(GET_PID(actor)))
		{
			if (existing->profile != literal_checkpoint_profile::native_quest ||
			    existing->token.actor_runtime_id != actor->runtime_id ||
			    existing->token.root_uid != root_uid || existing->payload != blob ||
			    existing->native_money_only != money_only ||
			    (money_only &&
			     (existing->native_money_before != cash ||
			      existing->native_money_wallet_revision != wallet_revision)) ||
			    existing->held)
				return player_literal_inventory_state::refused;
			*token_out = { existing->token.pid, existing->token.actor_runtime_id,
				       existing->token.root_uid, existing->token.generation,
				       money_only };
			return player_literal_inventory_state::pending;
		}
		if (literal_inventory_generation == std::numeric_limits<uint64_t>::max())
			return player_literal_inventory_state::refused;
		literal_inventory_checkpoint *slot = nullptr;
		for (auto &candidate : literal_inventory_checkpoints)
			if (!candidate.token.pid)
			{
				slot = &candidate;
				break;
			}
		if (!slot || !literal_inventory_capacity_locked(blob.size()))
			return player_literal_inventory_state::refused;
		token = { GET_PID(actor), actor->runtime_id, root_uid,
			  ++literal_inventory_generation, money_only };
		slot->profile = literal_checkpoint_profile::native_quest;
		slot->level = level;
		slot->native_money_only = money_only;
		slot->native_money_before = cash;
		slot->native_money_wallet_revision = wallet_revision;
		slot->token = native_quest_checkpoint_identity(token);
		slot->payload = std::move(blob);
	}
	// Make the installed original slot recoverable before enqueue can throw.
	*token_out = token;
	player_save_pipeline_result queued;
	try
	{
		queued = player_save_pipeline_request(actor, components, RENT_CRASH, room_vnum);
	}
	catch (...)
	{
		// Queue outcome is unresolved. Original token and frozen save policy remain.
		return player_literal_inventory_state::pending;
	}
	if (queued != player_save_pipeline_result::queued &&
	    queued != player_save_pipeline_result::coalesced)
	{
		player_save_pipeline_native_quest_checkpoint_cancel(token);
		return player_literal_inventory_state::refused;
	}
	*token_out = token;
	return player_literal_inventory_state::pending;
#endif
}

player_literal_inventory_state
player_save_pipeline_native_quest_checkpoint_begin(P_char actor, P_obj root, int room_vnum,
						   player_native_quest_checkpoint_token *output)
{
	return native_quest_checkpoint_begin(actor, root, room_vnum, output, false);
}
player_literal_inventory_state
player_save_pipeline_native_money_checkpoint_begin(P_char actor, int room_vnum,
						   player_native_quest_checkpoint_token *output)
{
	return native_quest_checkpoint_begin(actor, nullptr, room_vnum, output, true);
}

player_literal_inventory_state
player_save_pipeline_native_quest_checkpoint_poll(const player_native_quest_checkpoint_token &token,
						  P_char actor,
						  player_native_quest_checkpoint_stage *stage_out)
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)stage_out;
	return player_literal_inventory_state::refused;
#else
	const auto identity = native_quest_checkpoint_identity(token);
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto *literal = find_literal_inventory_locked(token.pid);
		if (!literal || literal->profile != literal_checkpoint_profile::native_quest ||
		    literal->token != identity || literal->native_money_only != token.money_only ||
		    literal->held)
			return player_literal_inventory_state::refused;
	}
	if (!economic_gameplay_authority::active_regular_sql() ||
	    !native_quest_actor_matches(token, actor) ||
	    player_save_journal_pid_quarantined(token.pid))
	{
		player_save_pipeline_native_quest_checkpoint_cancel(token);
		return player_literal_inventory_state::refused;
	}
	player_snapshot captured;
	std::vector<uint8_t> blob;
	if (player_snapshot_capture_literal_inventory(
		    actor, 1,
		    token.money_only ? SHOP_CHECKPOINT_COMPONENTS : LITERAL_INVENTORY_COMPONENTS,
		    RENT_CRASH, NOWHERE, token.root_uid,
		    &captured) != player_snapshot_capture_result::ok ||
	    !native_quest_checkpoint_blob(captured, &blob))
	{
		player_save_pipeline_native_quest_checkpoint_cancel(token);
		return player_literal_inventory_state::refused;
	}
	if (player_save_worker_pid_pending(token.pid))
		return player_literal_inventory_state::pending;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	if (!literal || literal->profile != literal_checkpoint_profile::native_quest ||
	    literal->token != identity || literal->native_money_only != token.money_only ||
	    literal->held)
		return player_literal_inventory_state::refused;
	// Money tokens additionally compare the original cash and wallet revision.
	std::array<int64_t, 4> cash{};
	if (literal->payload != blob ||
	    (token.money_only &&
	     (token.root_uid || !native_money_snapshot_cash(captured, &cash) ||
	      cash != literal->native_money_before ||
	      actor->only.pc->wallet_revision != literal->native_money_wallet_revision)))
	{
		*literal = {};
		return player_literal_inventory_state::refused;
	}
	player_revision_snapshot revision = {};
	if (!player_revision_snapshot_copy(token.pid, &revision) || revision.overflowed)
		return player_literal_inventory_state::refused;
	if (!literal->captured_revision ||
	    literal->acknowledged_revision != literal->captured_revision ||
	    revision.current_revision != literal->captured_revision ||
	    revision.acknowledged_revision != literal->captured_revision ||
	    revision.dirty_components || revision.unacknowledged_components ||
	    revision.queued_components || revision.inflight_components ||
	    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
		return player_literal_inventory_state::pending;
	if (stage_out)
		*stage_out = { literal->acknowledged_revision };
	return player_literal_inventory_state::database_acknowledged;
#endif
}

bool player_save_pipeline_native_quest_checkpoint_hold(
	const player_native_quest_checkpoint_token &token,
	const critical_operation_id &operation_id)
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)operation_id;
	return false;
#else
	if (std::all_of(operation_id.bytes.begin(), operation_id.bytes.end(),
			[](uint8_t byte) { return !byte; }) ||
	    player_save_pipeline_native_quest_checkpoint_poll(
		    token, find_character_by_runtime_id(token.actor_runtime_id)) !=
		    player_literal_inventory_state::database_acknowledged ||
	    player_save_worker_pid_pending(token.pid))
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	player_revision_snapshot revision = {};
	if (!economic_gameplay_authority::active_regular_sql() || !health.initialized ||
	    stop_requested || !accepting || !health.replay_complete || health.replay_blocked ||
	    find_terminal_fence_locked(token.pid) ||
	    find_target_save_login_fence_locked(token.pid) || !literal ||
	    literal->profile != literal_checkpoint_profile::native_quest ||
	    literal->token != native_quest_checkpoint_identity(token) ||
	    literal->native_money_only != token.money_only || literal->held ||
	    !literal->captured_revision ||
	    literal->acknowledged_revision != literal->captured_revision ||
	    !player_revision_snapshot_copy(token.pid, &revision) || revision.overflowed ||
	    revision.current_revision != literal->captured_revision ||
	    revision.acknowledged_revision != literal->captured_revision ||
	    revision.dirty_components || revision.unacknowledged_components ||
	    revision.queued_components || revision.inflight_components ||
	    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
		return false;
	literal->operation_id = operation_id;
	literal->held = true;
	return true;
#endif
}

bool player_save_pipeline_native_quest_checkpoint_release(
	const player_native_quest_checkpoint_token &token,
	const critical_operation_id &operation_id)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	if (!literal || literal->profile != literal_checkpoint_profile::native_quest ||
	    literal->token != native_quest_checkpoint_identity(token) ||
	    literal->native_money_only != token.money_only || !literal->held ||
	    literal->restored_sql_drop || literal->operation_id.bytes != operation_id.bytes)
		return false;
	*literal = {};
	return true;
}

bool player_save_pipeline_native_quest_checkpoint_cancel(
	const player_native_quest_checkpoint_token &token)
{
	if (token.pid <= 0 || !token.actor_runtime_id || !token.generation)
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	// Poll may already have cancelled this unheld checkpoint on a changed body.
	// Absence consumes no save/publication reservation and grants no ACK authority.
	if (!literal)
		return true;
	if (literal->profile != literal_checkpoint_profile::native_quest ||
	    literal->token != native_quest_checkpoint_identity(token) ||
	    literal->native_money_only != token.money_only || literal->held)
		return false;
	*literal = {};
	return true;
}

bool player_save_native_quest_checkpoint_owner::observe_held(
	const player_native_quest_checkpoint_token &token, P_char actor,
	const critical_operation_id &operation, player_native_quest_checkpoint_stage *output,
	std::vector<player_item_snapshot> *items_out) noexcept
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)operation;
	(void)output;
	(void)items_out;
	return false;
#else
	if (!output || !nevent_is_game_thread() ||
	    !economic_gameplay_authority::active_regular_sql() ||
	    !native_quest_actor_matches(token, actor) ||
	    player_save_journal_pid_quarantined(token.pid) ||
	    player_save_worker_pid_pending(token.pid))
		return false;
	try
	{
		player_snapshot captured;
		std::vector<uint8_t> blob;
		uint32_t level = 0;
		if (player_snapshot_capture_literal_inventory(
			    actor, 1,
			    token.money_only ? SHOP_CHECKPOINT_COMPONENTS :
					       LITERAL_INVENTORY_COMPONENTS,
			    RENT_CRASH, NOWHERE, token.root_uid,
			    &captured) != player_snapshot_capture_result::ok ||
		    !native_quest_checkpoint_blob(captured, &blob, &level))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto *slot = find_literal_inventory_locked(token.pid);
		std::array<int64_t, 4> cash{};
		if (!health.initialized || stop_requested || !accepting ||
		    !health.replay_complete || health.replay_blocked ||
		    find_terminal_fence_locked(token.pid) ||
		    find_target_save_login_fence_locked(token.pid) || !slot ||
		    slot->profile != literal_checkpoint_profile::native_quest ||
		    slot->token != native_quest_checkpoint_identity(token) ||
		    slot->native_money_only != token.money_only || !slot->held ||
		    slot->restored_sql_drop || slot->operation_id.bytes != operation.bytes ||
		    slot->payload != blob || slot->level != level ||
		    (token.money_only &&
		     (token.root_uid || !native_money_snapshot_cash(captured, &cash) ||
		      cash != slot->native_money_before ||
		      actor->only.pc->wallet_revision != slot->native_money_wallet_revision)) ||
		    !slot->captured_revision ||
		    slot->acknowledged_revision != slot->captured_revision ||
		    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
			return false;
		// Unrelated dirty STATUS marks may advance while the native operation is held.
		// Frozen inventory/level and the actual acknowledged save revision must match.
		// This exact current body was compared to the original held slot.
		// Copying values exposes no execution hold or ACK capability.
		if (items_out)
			*items_out = std::move(captured.items);
		*output = { slot->acknowledged_revision };
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

#ifndef __NO_MYSQL__
namespace
{
bool native_quest_publication_identity(const critical_command &command,
				       item_transfer_payload *payload)
{
	economic_frozen_intent intent;
	if (!payload || !command.publication_required || !command.accepted_at_usec ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !item_transfer_native_mobile_acknowledged_version(command.payload_version) ||
	    !critical_command_envelope_valid(command) ||
	    !item_transfer_command_decode_payload(command, payload) ||
	    !item_transfer_native_mobile_recovery_shape_valid(*payload) ||
	    economic_intent_decode(command.accounting_intent, &intent) !=
		    economic_accounting_error::ok ||
	    economic_intent_verify_binding(command, intent) != economic_accounting_error::ok)
		return false;
	auto original = command;
	original.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	original.accounting_intent.clear();
	original.accepted_at_usec = 0;
	original.publication_required = false;
	std::vector<uint8_t> expected;
	const auto &metadata = intent.admission.metadata;
	return item_native_mobile_accounting_intent(
		       original, metadata.lineage, metadata.epoch,
		       payload->native_mobile.final_giver_pid,
		       metadata.source_event ? &*metadata.source_event : nullptr,
		       &expected) == economic_accounting_error::ok &&
	       expected == command.accounting_intent;
}
bool native_quest_body_pair(const item_transfer_payload &payload, std::span<const uint8_t> before,
			    std::vector<uint8_t> *after)
{
	if (!after)
		return false;
	if (payload.native_money.present || payload.native_cost.fee_only)
	{
		if (!item_transfer_native_mobile_recovery_shape_valid(payload) ||
		    !shop_trade_recovery_forest_verify(
			    before, shop_trade_recovery_forest_role::player_before,
			    payload.native_recovery.player_before) ||
		    !shop_trade_recovery_forest_verify(
			    before, shop_trade_recovery_forest_role::player_after,
			    payload.native_recovery.player_after))
			return false;
		std::vector<uint8_t> unchanged(before.begin(), before.end());
		*after = std::move(unchanged);
		return true;
	}
	if (payload.native_mobile.action == item_native_mobile_action::consumption)
	{
		// Real final-giver checkpoint is retained; no inventory mutation or
		// fabricated complete binding is introduced for native destruction.
		*after = std::vector<uint8_t>(before.begin(), before.end());
		return true;
	}
	std::vector<player_item_snapshot> full, selected, remaining;
	std::vector<uint8_t> selected_bytes, after_bytes;
	if (!shop_trade_recovery_forest_verify(before,
					       shop_trade_recovery_forest_role::player_before,
					       payload.native_recovery.player_before) ||
	    player_item_snapshot_list_decode(before.data(), before.size(), &full) !=
		    player_snapshot_codec_result::ok ||
	    player_item_snapshot_extract_subtree(full, item_transfer_result_root(payload),
						 &selected,
						 &remaining) != player_snapshot_codec_result::ok ||
	    player_item_snapshot_list_encode(selected, &selected_bytes) !=
		    player_snapshot_codec_result::ok ||
	    selected_bytes.size() != payload.item_blob_size ||
	    !std::equal(selected_bytes.begin(), selected_bytes.end(), payload.item_blob.begin()) ||
	    player_item_snapshot_list_encode(remaining, &after_bytes) !=
		    player_snapshot_codec_result::ok ||
	    !shop_trade_recovery_forest_verify(after_bytes,
					       shop_trade_recovery_forest_role::player_after,
					       payload.native_recovery.player_after))
		return false;
	*after = std::move(after_bytes);
	return true;
}
bool native_quest_slot_money_shape(const literal_inventory_checkpoint &slot,
				   const item_transfer_payload &payload) noexcept
{
	if (!payload.native_money.present)
		return !slot.native_money_only && !slot.native_money_wallet_revision &&
		       slot.native_money_before == std::array<int64_t, 4>{};
	return slot.native_money_only && !slot.token.root_uid &&
	       slot.native_money_before == payload.native_money.projection.player_before &&
	       slot.native_money_wallet_revision ==
		       payload.native_money.projection.player_before_revision;
}

bool native_quest_slot_runtime_shape(const literal_inventory_checkpoint &slot) noexcept
{
	if (!slot.restored_native_quest)
		return !slot.native_quest_runtime_rebound && slot.token.actor_runtime_id &&
		       slot.token.generation;
	if (!slot.restored_native_quest_revision || slot.restored_native_quest_attachment.empty())
		return false;
	return slot.native_quest_runtime_rebound ?
		       (slot.token.actor_runtime_id && slot.token.generation) :
		       (!slot.token.actor_runtime_id && !slot.token.generation);
}
bool native_quest_envelope_slot_matches_locked(const critical_native_recovery_envelope &envelope)
{
	item_transfer_payload payload{};
	native_quest_recovery_context recovery;
	std::vector<uint8_t> frozen, before, after;
	if (!envelope.revision || !native_quest_publication_identity(envelope.command, &payload) ||
	    native_quest_recovery_context_decode(envelope.command, envelope.attachment,
						 &recovery) != player_snapshot_codec_result::ok ||
	    critical_command_encode(envelope.command, &frozen) !=
		    critical_command_codec_result::ok ||
	    player_item_snapshot_list_encode(recovery.player_before, &before) !=
		    player_snapshot_codec_result::ok ||
	    !native_quest_body_pair(payload, before, &after) ||
	    (envelope.phase != critical_native_recovery_phase::execution_pending &&
	     !(envelope.phase == critical_native_recovery_phase::continuation_pending &&
	       payload.native_cost.fee_only && recovery.receipt.present &&
	       recovery.publication_stage ==
		       native_quest_recovery_publication_stage::physically_proven)))
		return false;
	const int pid = static_cast<int>(payload.native_recovery.player_pid);
	const auto revision = payload.native_recovery.acknowledged_save_revision;
	const auto *slot = find_literal_inventory_locked(pid);
	return health.initialized && !stop_requested && slot && slot->held &&
	       slot->profile == literal_checkpoint_profile::native_quest &&
	       native_quest_slot_money_shape(*slot, payload) && slot->restored_sql_drop &&
	       slot->execution_hold_generation && slot->captured_revision == revision &&
	       slot->acknowledged_revision == revision &&
	       slot->operation_id.bytes == envelope.command.operation_id.bytes &&
	       slot->payload == frozen && slot->original_native_quest_before == before &&
	       slot->original_native_quest_after == after &&
	       native_quest_slot_runtime_shape(*slot) &&
	       player_save_execution_guard::publication_operation_held(
		       envelope.command.operation_id);
}

}
#endif

bool player_save_native_quest_publication_owner::restore_recovery_checkpoint(
	const critical_native_recovery_envelope &envelope) noexcept
{
#ifdef __NO_MYSQL__
	(void)envelope;
	return false;
#else
	try
	{
		item_transfer_payload payload{};
		native_quest_recovery_context recovery;
		std::vector<uint8_t> frozen, before, after, attachment;
		if (!envelope.revision ||
		    envelope.phase != critical_native_recovery_phase::execution_pending ||
		    !native_quest_publication_identity(envelope.command, &payload) ||
		    native_quest_recovery_context_decode(envelope.command, envelope.attachment,
							 &recovery) !=
			    player_snapshot_codec_result::ok ||
		    critical_command_encode(envelope.command, &frozen) !=
			    critical_command_codec_result::ok ||
		    player_item_snapshot_list_encode(recovery.player_before, &before) !=
			    player_snapshot_codec_result::ok ||
		    !native_quest_body_pair(payload, before, &after))
			return false;
		size_t bytes = 0;
		for (const size_t size :
		     { frozen.size(), before.size(), after.size(), envelope.attachment.size() })
		{
			if (size > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
				return false;
			bytes += size;
		}
		attachment = envelope.attachment;
		const int pid = static_cast<int>(payload.native_recovery.player_pid);
		const uint64_t revision = payload.native_recovery.acknowledged_save_revision;
		const uint64_t root = payload.native_mobile.action ==
						      item_native_mobile_action::acceptance ?
					      payload.selected_item_uid :
					      0;
		if (pid <= 0 || !revision)
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		// Critical replay installs only a prepared passive original hold. No
		// actor runtime identity, save token generation or effects are restored.
		if (!health.initialized || stop_requested || execution_started)
			return false;
		literal_inventory_checkpoint *slot = nullptr;
		for (auto &candidate : literal_inventory_checkpoints)
		{
			if (candidate.token.pid == pid ||
			    (candidate.held &&
			     candidate.operation_id.bytes == envelope.command.operation_id.bytes))
			{
				uint64_t generation = 0;
				if (candidate.profile != literal_checkpoint_profile::native_quest ||
				    !candidate.restored_native_quest ||
				    candidate.native_quest_runtime_rebound ||
				    !candidate.restored_sql_drop || !candidate.held ||
				    candidate.token.pid != pid ||
				    candidate.token.root_uid != root ||
				    candidate.native_money_only != payload.native_money.present ||
				    (payload.native_money.present &&
				     (candidate.native_money_before !=
					      payload.native_money.projection.player_before ||
				      candidate.native_money_wallet_revision !=
					      payload.native_money.projection
						      .player_before_revision)) ||
				    candidate.token.actor_runtime_id ||
				    candidate.token.generation || candidate.payload != frozen ||
				    candidate.original_native_quest_before != before ||
				    candidate.original_native_quest_after != after ||
				    candidate.restored_native_quest_revision != envelope.revision ||
				    candidate.restored_native_quest_attachment != attachment ||
				    candidate.captured_revision != revision ||
				    candidate.acknowledged_revision != revision ||
				    candidate.operation_id.bytes !=
					    envelope.command.operation_id.bytes ||
				    !player_save_execution_guard::install_hold(
					    pid, envelope.command.operation_id, &generation))
					return false;
				if (generation != candidate.execution_hold_generation)
				{
					player_save_execution_guard::poison_integrity();
					return false;
				}
				return true;
			}
			if (!candidate.token.pid && !slot)
				slot = &candidate;
		}
		if (!slot || !literal_inventory_capacity_locked(bytes))
			return false;
		uint64_t generation = 0;
		if (!player_save_execution_guard::install_hold(pid, envelope.command.operation_id,
							       &generation))
			return false;
		// All canonical decode, forest derivation, allocation and capacity checks
		// precede hold installation. These default-allocator moves cannot throw.
		slot->profile = literal_checkpoint_profile::native_quest;
		slot->token = { pid, 0, root, 0 };
		slot->native_money_only = payload.native_money.present;
		if (payload.native_money.present)
		{
			slot->native_money_before = payload.native_money.projection.player_before;
			slot->native_money_wallet_revision =
				payload.native_money.projection.player_before_revision;
		}
		slot->payload = std::move(frozen);
		slot->original_native_quest_before = std::move(before);
		slot->original_native_quest_after = std::move(after);
		slot->restored_native_quest_attachment = std::move(attachment);
		slot->restored_native_quest_revision = envelope.revision;
		slot->captured_revision = revision;
		slot->acknowledged_revision = revision;
		slot->operation_id = envelope.command.operation_id;
		slot->execution_hold_generation = generation;
		slot->held = true;
		slot->restored_sql_drop = true;
		slot->restored_native_quest = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_native_quest_publication_owner::rebind_recovery_checkpoint(
	const critical_native_recovery_envelope &original,
	uint64_t actual_player_runtime_id) noexcept
{
#ifdef __NO_MYSQL__
	(void)original;
	(void)actual_player_runtime_id;
	return false;
#else
	if (!actual_player_runtime_id || !nevent_is_game_thread())
		return false;
	try
	{
		item_transfer_payload payload{};
		if (!native_quest_publication_identity(original.command, &payload))
			return false;
		const int pid = static_cast<int>(payload.native_recovery.player_pid);
		P_char actor = find_character_by_runtime_id(actual_player_runtime_id);
		if (pid <= 0 || !actor || !IS_PC(actor) || !actor->only.pc ||
		    actor->runtime_id != actual_player_runtime_id || GET_PID(actor) != pid ||
		    IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED) ||
		    player_save_journal_pid_quarantined(pid) || player_save_worker_pid_pending(pid))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!native_quest_envelope_slot_matches_locked(original))
			return false;
		auto *slot = find_literal_inventory_locked(pid);
		const uint64_t root = payload.native_mobile.action ==
						      item_native_mobile_action::acceptance ?
					      payload.selected_item_uid :
					      0;
		if (!slot || !slot->restored_native_quest || slot->token.pid != pid ||
		    slot->token.root_uid != root ||
		    slot->restored_native_quest_revision != original.revision ||
		    slot->restored_native_quest_attachment != original.attachment ||
		    find_terminal_fence_locked(pid) || find_target_save_login_fence_locked(pid) ||
		    append_inflight_pid == pid || any_snapshot_is_retained_locked(pid) ||
		    find_character_by_runtime_id(actual_player_runtime_id) != actor)
			return false;
		if (slot->native_quest_runtime_rebound)
		{
			if (slot->token.actor_runtime_id == actual_player_runtime_id)
				return true;
			// A reconnect may bind a fresh local identity only after the old actor
			// is absent. A still-registered actor is an unresolved ownership conflict.
			if (find_character_by_runtime_id(slot->token.actor_runtime_id))
				return false;
		}
		if (literal_inventory_generation == std::numeric_limits<uint64_t>::max())
			return false;
		// All canonical and original-body checks precede these nonallocating
		// assignments. No coordinator, journal, save revision or hold is changed.
		slot->token.actor_runtime_id = actual_player_runtime_id;
		slot->token.generation = ++literal_inventory_generation;
		slot->native_quest_runtime_rebound = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_native_quest_publication_owner::copy_recovery_context(
	const critical_command &command, critical_native_recovery_envelope *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)output;
	return false;
#else
	if (!output || !nevent_is_game_thread())
		return false;
	try
	{
		critical_native_recovery_envelope envelope;
		// Never acquire coordinator_mutex while pipeline_mutex is held.
		if (!critical_native_quest_publication_owner::copy_context(command, &envelope))
			return false;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			if (!native_quest_envelope_slot_matches_locked(envelope))
				return false;
		}
		*output = std::move(envelope);
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_native_quest_publication_owner::checkpoint_recovery_context(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor) noexcept
{
#ifdef __NO_MYSQL__
	(void)expected;
	(void)successor;
	return false;
#else
	if (!nevent_is_game_thread() ||
	    expected.attachment.size() > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
	    successor.attachment.size() > PLAYER_SAVE_PIPELINE_MAX_BYTES)
		return false;
	try
	{
		item_transfer_payload payload{};
		std::vector<uint8_t> frozen, original_before, original_after, original_attachment;
		if (!native_quest_publication_identity(expected.command, &payload) ||
		    critical_command_encode(expected.command, &frozen) !=
			    critical_command_codec_result::ok)
			return false;
		const int pid = static_cast<int>(payload.native_recovery.player_pid);
		literal_inventory_checkpoint *pinned = nullptr;
		player_literal_inventory_token token{};
		uint64_t generation = 0, replay_revision = 0;
		player_revision_t captured_revision = 0, acknowledged_revision = 0;
		bool restored = false;
		size_t reserved = 0;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			if (!native_quest_envelope_slot_matches_locked(expected) ||
			    !native_quest_envelope_slot_matches_locked(successor))
				return false;
			pinned = find_literal_inventory_locked(pid);
			if (!pinned)
				return false;
			// All snapshot copies precede capacity reservation and journal I/O.
			// They permit exact nonallocating original-slot checks after the CAS.
			original_before = pinned->original_native_quest_before;
			original_after = pinned->original_native_quest_after;
			original_attachment = pinned->restored_native_quest_attachment;
			token = pinned->token;
			generation = pinned->execution_hold_generation;
			replay_revision = pinned->restored_native_quest_revision;
			captured_revision = pinned->captured_revision;
			acknowledged_revision = pinned->acknowledged_revision;
			restored = pinned->restored_native_quest;
			reserved =
				std::max({ pinned->native_quest_attachment_reserved_bytes,
					   original_attachment.size(), expected.attachment.size(),
					   successor.attachment.size() });
			size_t bytes = 0;
			for (const size_t size : { pinned->payload.size(), original_before.size(),
						   original_after.size(), reserved })
			{
				if (size > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
					return false;
				bytes += size;
			}
			if (!literal_inventory_capacity_locked(bytes, pinned))
				return false;
			pinned->native_quest_attachment_reserved_bytes = reserved;
		}
		// No pipeline mutex across the exact coordinator/journal CAS. A false
		// result may represent uncertainty, so its high-water charge remains.
		const bool confirmed = critical_native_quest_publication_owner::checkpoint_context(
			expected, successor);
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(pid);
		// No encode/decode/command equality allocation after durable I/O.
		if (!slot || slot != pinned || !slot->held || !slot->restored_sql_drop ||
		    slot->profile != literal_checkpoint_profile::native_quest ||
		    !native_quest_slot_money_shape(*slot, payload) || slot->token != token ||
		    slot->execution_hold_generation != generation ||
		    slot->operation_id.bytes != expected.command.operation_id.bytes ||
		    slot->payload != frozen ||
		    slot->original_native_quest_before != original_before ||
		    slot->original_native_quest_after != original_after ||
		    slot->restored_native_quest_attachment != original_attachment ||
		    slot->restored_native_quest_revision != replay_revision ||
		    slot->captured_revision != captured_revision ||
		    slot->acknowledged_revision != acknowledged_revision ||
		    slot->restored_native_quest != restored ||
		    slot->native_quest_attachment_reserved_bytes != reserved ||
		    !player_save_execution_guard::publication_operation_held(
			    expected.command.operation_id))
		{
			player_save_execution_guard::poison_integrity();
			return false;
		}
		if (confirmed)
			slot->native_quest_attachment_reserved_bytes =
				std::max(original_attachment.size(), successor.attachment.size());
		return confirmed;
	}
	catch (...)
	{
		// Every allocation is before reservation. If a later coordinator call
		// unexpectedly throws, the already-reserved high-water charge is retained.
		return false;
	}
#endif
}

critical_submit_result player_save_native_quest_checkpoint_owner::submit_owned(
	const player_native_quest_checkpoint_token &token, critical_command command,
	std::span<const uint8_t> original_native_before, bool *checkpoint_released) noexcept
{
	if (!checkpoint_released)
		return critical_submit_result::invalid;
	*checkpoint_released = false;
#ifdef __NO_MYSQL__
	(void)token;
	(void)command;
	(void)original_native_before;
	return critical_submit_result::unavailable;
#else
	if (!nevent_is_game_thread())
		return critical_submit_result::unavailable;
	int pid = 0;
	uint64_t revision = 0, generation = 0;
	item_transfer_payload native_payload{};
	std::vector<uint8_t> after_body, retained_attachment;
	std::vector<uint8_t> frozen;
	bool new_hold = false;
	critical_native_recovery_envelope envelope;
	native_quest_recovery_context recovery;
	const auto operation = command.operation_id;
	try
	{
		if (!native_quest_publication_identity(command, &native_payload) ||
		    player_item_snapshot_list_decode(
			    original_native_before.data(), original_native_before.size(),
			    &recovery.native_before) != player_snapshot_codec_result::ok)
			return critical_submit_result::invalid;
		pid = static_cast<int>(native_payload.native_recovery.player_pid);
		revision = native_payload.native_recovery.acknowledged_save_revision;
		if (pid != token.pid || !token.actor_runtime_id || !token.generation ||
		    token.money_only != native_payload.native_money.present ||
		    (token.money_only &&
		     (token.root_uid ||
		      command.payload_version !=
			      ITEM_TRANSFER_NATIVE_MOBILE_MONEY_RECOVERY_PAYLOAD_VERSION)) ||
		    (native_payload.native_mobile.action == item_native_mobile_action::acceptance ?
			     token.root_uid != native_payload.selected_item_uid :
			     token.root_uid != 0) ||
		    critical_command_encode(command, &frozen) !=
			    critical_command_codec_result::ok ||
		    frozen.size() > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
		    player_save_worker_pid_pending(pid))
			return critical_submit_result::invalid;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(pid);
		if (!health.initialized || stop_requested || !accepting || !execution_started ||
		    !health.replay_complete || health.replay_blocked ||
		    find_terminal_fence_locked(pid) || find_target_save_login_fence_locked(pid) ||
		    append_inflight_pid == pid || any_snapshot_is_retained_locked(pid) || !slot)
			return critical_submit_result::unavailable;
		if (slot->profile != literal_checkpoint_profile::native_quest || !slot->held ||
		    slot->token != native_quest_checkpoint_identity(token) ||
		    slot->native_money_only != token.money_only ||
		    slot->operation_id.bytes != operation.bytes)
			return critical_submit_result::identity_conflict;
		if (token.money_only &&
		    (slot->native_money_before !=
			     native_payload.native_money.projection.player_before ||
		     slot->native_money_wallet_revision !=
			     native_payload.native_money.projection.player_before_revision))
			return critical_submit_result::identity_conflict;
		// The original native owner supplies its captured literal stock. The
		// save owner supplies the actual acknowledged giver forest. Encode both
		// before acquiring a new leaf hold or entering the coordinator.
		const auto &original_player_before = slot->restored_sql_drop ?
							     slot->original_native_quest_before :
							     slot->payload;
		if (player_item_snapshot_list_decode(
			    original_player_before.data(), original_player_before.size(),
			    &recovery.player_before) != player_snapshot_codec_result::ok)
			return critical_submit_result::invalid;
		recovery.consumed_root_steps.assign(
			native_payload.native_recovery.consumed_root_order.size(), 0);
		envelope.command = command;
		envelope.revision = 1;
		envelope.phase = critical_native_recovery_phase::execution_pending;
		if (native_quest_recovery_context_encode(command, recovery, &envelope.attachment) !=
		    player_snapshot_codec_result::ok)
			return critical_submit_result::invalid;
		if (slot->restored_sql_drop)
		{
			if (slot->payload != frozen || !slot->execution_hold_generation ||
			    slot->restored_native_quest ||
			    slot->restored_native_quest_revision != envelope.revision ||
			    slot->restored_native_quest_attachment != envelope.attachment)
				return critical_submit_result::identity_conflict;
			generation = slot->execution_hold_generation;
		}
		else
		{
			if (!native_quest_body_pair(native_payload, slot->payload, &after_body))
				return critical_submit_result::invalid;
			size_t recoverable_slot_bytes = 0;
			for (const size_t size : { frozen.size(), slot->payload.size(),
						   after_body.size(), envelope.attachment.size() })
			{
				if (size > PLAYER_SAVE_PIPELINE_MAX_BYTES - recoverable_slot_bytes)
					return critical_submit_result::overloaded;
				recoverable_slot_bytes += size;
			}
			if (slot->captured_revision != revision ||
			    slot->acknowledged_revision != revision ||
			    slot->payload.size() < sizeof(uint32_t) ||
			    !slot->original_native_quest_before.empty() ||
			    !slot->original_native_quest_after.empty() ||
			    slot->restored_native_quest || slot->restored_native_quest_revision ||
			    !slot->restored_native_quest_attachment.empty() ||
			    slot->payload.size() > PLAYER_SAVE_PIPELINE_MAX_BYTES - frozen.size() ||
			    after_body.size() > PLAYER_SAVE_PIPELINE_MAX_BYTES - frozen.size() -
							slot->payload.size() ||
			    !literal_inventory_capacity_locked(recoverable_slot_bytes, slot))
				return critical_submit_result::unavailable;
			// Retain the exact original recovery bytes and aggregate charge before
			// installing the hold. Allocation failure leaves the original slot intact.
			retained_attachment = envelope.attachment;
			if (!player_save_execution_guard::install_live_publication_hold(
				    pid, operation, &generation))
				return critical_submit_result::unavailable;
			// All allocation and combined capacity checks precede the leaf hold.
			// Move the actual original body, then the command; neither move throws.
			slot->original_native_quest_before = std::move(slot->payload);
			slot->original_native_quest_after = std::move(after_body);
			slot->payload = std::move(frozen);
			slot->restored_native_quest_attachment = std::move(retained_attachment);
			slot->restored_native_quest_revision = envelope.revision;
			// This remains a real live actor slot, not passive restored enrollment.
			slot->execution_hold_generation = generation;
			slot->restored_sql_drop = true;
			new_hold = true;
		}
	}
	catch (...)
	{
		return critical_submit_result::invalid;
	}
	critical_submit_result submitted;
	try
	{
		// Never enter coordinator_mutex while pipeline_mutex is held.
		submitted = critical_native_quest_submission_owner::submit(std::move(envelope));
	}
	catch (...)
	{
		// Admission may have happened; the exact original slot remains fenced.
		return critical_submit_result::journal_uncertain;
	}
	if (!critical_submit_result_keeps_operation(submitted) && new_hold)
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(pid);
		if (!slot || slot->profile != literal_checkpoint_profile::native_quest ||
		    !slot->held || !slot->restored_sql_drop ||
		    slot->token != native_quest_checkpoint_identity(token) ||
		    slot->native_money_only != token.money_only ||
		    slot->operation_id.bytes != operation.bytes ||
		    slot->execution_hold_generation != generation)
		{
			player_save_execution_guard::poison_integrity();
			return critical_submit_result::journal_uncertain;
		}
		if (!player_save_execution_guard::release_hold(pid, operation, generation))
			return critical_submit_result::journal_uncertain;
		*slot = {};
		*checkpoint_released = true;
		// Definite pre-journal refusal releases only this exact reservation.
		// The preparation owner separately retires its native checkpoint facts.
		return submitted;
	}
	return critical_submit_result_keeps_operation(submitted) ?
		       submitted :
		       critical_submit_result::journal_uncertain;
#endif
}

bool player_save_native_quest_checkpoint_owner::original_fee_acceptance(
	const critical_command &actual_fee_action, critical_command *original_acceptance,
	std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> *original_typed48) noexcept
{
#ifdef __NO_MYSQL__
	(void)actual_fee_action;
	(void)original_acceptance;
	(void)original_typed48;
	return false;
#else
	if (!original_acceptance || !original_typed48 || original_acceptance == &actual_fee_action)
		return false;
	try
	{
		critical_native_recovery_envelope parent;
		native_quest_recovery_context context;
		if (!critical_native_quest_publication_owner::copy_fee_acceptance_context(
			    actual_fee_action, &parent) ||
		    native_quest_recovery_context_decode(parent.command, parent.attachment,
							 &context) !=
			    player_snapshot_codec_result::ok ||
		    !context.receipt.present ||
		    context.receipt.result_size != ITEM_TRANSFER_RESULT_BYTES)
			return false;
		std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> receipt{};
		std::copy_n(context.receipt.result_payload.begin(), receipt.size(),
			    receipt.begin());
		static_assert(noexcept(std::declval<critical_command &>() =
					       std::declval<critical_command &&>()));
		*original_acceptance = std::move(parent.command);
		*original_typed48 = receipt;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_native_quest_checkpoint_owner::original_held_bodies(
	const critical_command &command, std::vector<player_item_snapshot> *before,
	std::vector<player_item_snapshot> *after,
	player_native_quest_checkpoint_stage *stage) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)before;
	(void)after;
	(void)stage;
	return false;
#else
	if (!before || !after || before == after || !stage)
		return false;
	try
	{
		item_transfer_payload payload{};
		std::vector<uint8_t> frozen;
		if (!native_quest_publication_identity(command, &payload) ||
		    critical_command_encode(command, &frozen) !=
			    critical_command_codec_result::ok ||
		    player_save_worker_pid_pending(payload.native_recovery.player_pid))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto pid = static_cast<int>(payload.native_recovery.player_pid);
		const auto *slot = find_literal_inventory_locked(pid);
		const auto revision = payload.native_recovery.acknowledged_save_revision;
		if (!health.initialized || stop_requested || !slot || !slot->held ||
		    !slot->restored_sql_drop ||
		    slot->profile != literal_checkpoint_profile::native_quest ||
		    !native_quest_slot_money_shape(*slot, payload) ||
		    slot->operation_id.bytes != command.operation_id.bytes ||
		    slot->payload != frozen || !slot->execution_hold_generation ||
		    slot->captured_revision != revision ||
		    slot->acknowledged_revision != revision ||
		    !native_quest_slot_runtime_shape(*slot) || find_terminal_fence_locked(pid) ||
		    find_target_save_login_fence_locked(pid) || append_inflight_pid == pid ||
		    any_snapshot_is_retained_locked(pid) ||
		    !player_save_execution_guard::publication_operation_held(command.operation_id))
			return false;
		std::vector<uint8_t> expected_after;
		if (!native_quest_body_pair(payload, slot->original_native_quest_before,
					    &expected_after) ||
		    expected_after != slot->original_native_quest_after)
			return false;
		std::vector<player_item_snapshot> a, b;
		if (player_item_snapshot_list_decode(slot->original_native_quest_before.data(),
						     slot->original_native_quest_before.size(),
						     &a) != player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_decode(slot->original_native_quest_after.data(),
						     slot->original_native_quest_after.size(),
						     &b) != player_snapshot_codec_result::ok)
			return false;
		// Item consumption takes absent spans. The genuine fee participant
		// must prove the complete unchanged original player forest instead.
		if (payload.native_mobile.action == item_native_mobile_action::consumption &&
		    !payload.native_cost.fee_only)
		{
			a.clear();
			b.clear();
		}
		*before = std::move(a);
		*after = std::move(b);
		*stage = { revision };
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_restored_publication_owner::retire_covered_ordinary(
	player_save_restored_publication_owner &owner,
	const std::vector<player_save_journal_retained_frame> &originals) noexcept
{
#ifdef __NO_MYSQL__
	(void)owner;
	(void)originals;
	return false;
#else
	if (!nevent_is_game_thread() || !owner.reservation_.valid() || owner.pid_ <= 0 ||
	    player_save_worker_pid_pending(owner.pid_))
		return false;
	try
	{
		player_save_covered_revision covered;
		return player_snapshot_repository_observe_covered_revision(
			       owner.pid_, owner.reservation_, &covered) &&
		       player_save_journal_retire_covered_ordinary(owner.pid_, owner.reservation_,
								   covered, originals) ==
			       player_save_journal_result::ok;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_native_quest_publication_owner::publish_native_quest(
	const critical_command &original, const critical_completion &completion,
	bool (*native_publish)(const critical_command &, const critical_completion &,
			       void *) noexcept,
	void *context) noexcept
{
#ifdef __NO_MYSQL__
	(void)original;
	(void)completion;
	(void)native_publish;
	(void)context;
	return false;
#else
	if (!nevent_is_game_thread() || !native_publish ||
	    !critical_completion_disposition_valid(completion) ||
	    completion.disposition != critical_completion_disposition::execution ||
	    completion.failure_stage != critical_failure_stage::none ||
	    original.operation_id.bytes != completion.operation_id.bytes)
		return false;
	try
	{
		item_transfer_payload payload{};
		std::vector<uint8_t> frozen;
		critical_command command;
		if (!native_quest_publication_identity(original, &payload) ||
		    critical_command_encode(original, &frozen) != critical_command_codec_result::ok)
			return false;
		const int pid = static_cast<int>(payload.native_recovery.player_pid);
		uint64_t generation = 0;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			const auto *slot = find_literal_inventory_locked(pid);
			std::vector<uint8_t> expected_after;
			if (!health.initialized || stop_requested || !slot || !slot->held ||
			    !slot->restored_sql_drop ||
			    slot->profile != literal_checkpoint_profile::native_quest ||
			    !native_quest_slot_money_shape(*slot, payload) ||
			    !slot->token.actor_runtime_id || !slot->token.generation ||
			    slot->payload != frozen || !slot->execution_hold_generation ||
			    slot->captured_revision !=
				    payload.native_recovery.acknowledged_save_revision ||
			    slot->acknowledged_revision != slot->captured_revision ||
			    slot->original_native_quest_before.size() < sizeof(uint32_t) ||
			    !native_quest_body_pair(payload, slot->original_native_quest_before,
						    &expected_after) ||
			    expected_after != slot->original_native_quest_after ||
			    slot->operation_id.bytes != completion.operation_id.bytes ||
			    find_terminal_fence_locked(pid) ||
			    find_target_save_login_fence_locked(pid) ||
			    append_inflight_pid == pid || any_snapshot_is_retained_locked(pid) ||
			    critical_command_decode(frozen.data(), frozen.size(), &command) !=
				    critical_command_codec_result::ok)
				return false;
			generation = slot->execution_hold_generation;
		}
		player_save_restored_publication_owner owner(
			std::move(command), std::move(frozen), completion,
			player_save_execution_guard::current_ownership_epoch(), pid, generation);
		if (!owner.reservation_.valid() || player_save_worker_pid_pending(pid))
			return false;
		// Frozen native/player inventory and level are proved by the native
		// participant; later volatile STATUS dirt remains eligible for capture.
		player_revision_snapshot revision{};
		if (player_revision_snapshot_copy(pid, &revision) &&
		    (revision.overflowed || revision.queued_components ||
		     revision.inflight_components))
			return false;
		std::vector<player_save_journal_retained_frame> originals;
		if (player_save_journal_collect_publication_frames(
			    pid, owner.reservation_, &originals) != player_save_journal_result::ok)
			return false;
		if (!originals.empty())
		{
			if (!player_save_restored_publication_owner::retire_covered_ordinary(
				    owner, originals))
				return false;
		}
		if (player_save_journal_publication_census(pid, owner.reservation_) !=
			    player_save_journal_result::ok ||
		    !native_publish(owner.command_, completion, context) ||
		    !owner.reservation_.valid() ||
		    player_save_journal_publication_census(pid, owner.reservation_) !=
			    player_save_journal_result::ok)
			return false;
		owner.publication_proven_ = true;
		return critical_command_coordinator_acknowledge_publication(owner);
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_native_quest_publication_owner::publication_held_bodies(
	const critical_command &command, std::vector<player_item_snapshot> *before,
	std::vector<player_item_snapshot> *after,
	player_native_quest_checkpoint_stage *stage) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)before;
	(void)after;
	(void)stage;
	return false;
#else
	if (!before || !after || before == after || !stage)
		return false;
	try
	{
		item_transfer_payload payload{};
		std::vector<uint8_t> frozen;
		if (!native_quest_publication_identity(command, &payload) ||
		    critical_command_encode(command, &frozen) !=
			    critical_command_codec_result::ok ||
		    player_save_worker_pid_pending(payload.native_recovery.player_pid))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto pid = static_cast<int>(payload.native_recovery.player_pid);
		const auto *slot = find_literal_inventory_locked(pid);
		const auto revision = payload.native_recovery.acknowledged_save_revision;
		if (!health.initialized || stop_requested || !slot || !slot->held ||
		    !slot->restored_sql_drop ||
		    slot->profile != literal_checkpoint_profile::native_quest ||
		    !native_quest_slot_money_shape(*slot, payload) ||
		    slot->operation_id.bytes != command.operation_id.bytes ||
		    slot->payload != frozen || !slot->execution_hold_generation ||
		    slot->captured_revision != revision ||
		    slot->acknowledged_revision != revision || !slot->token.actor_runtime_id ||
		    !slot->token.generation || find_terminal_fence_locked(pid) ||
		    find_target_save_login_fence_locked(pid) || append_inflight_pid == pid ||
		    any_snapshot_is_retained_locked(pid) ||
		    !player_save_execution_guard::publication_operation_held(command.operation_id))
			return false;
		std::vector<uint8_t> expected_after;
		if (!native_quest_body_pair(payload, slot->original_native_quest_before,
					    &expected_after) ||
		    expected_after != slot->original_native_quest_after)
			return false;
		std::vector<player_item_snapshot> a, b;
		if (player_item_snapshot_list_decode(slot->original_native_quest_before.data(),
						     slot->original_native_quest_before.size(),
						     &a) != player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_decode(slot->original_native_quest_after.data(),
						     slot->original_native_quest_after.size(),
						     &b) != player_snapshot_codec_result::ok)
			return false;
		// Publication authenticates the actual final-giver checkpoint even when
		// consumption mutates no player inventory. Do not return an empty model.
		*before = std::move(a);
		*after = std::move(b);
		*stage = { revision };
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

#ifndef __NO_MYSQL__
namespace
{
bool auction_publication_identity(const critical_command &command,
				  auction_native_command_context *payload)
{
	return payload && command.type == critical_command_type::auction &&
	       command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
	       command.publication_required && command.accepted_at_usec &&
	       critical_command_envelope_valid(command) &&
	       auction_repository_frozen_accounting_valid(command) &&
	       auction_native_command_decode(command, payload) == economic_accounting_error::ok &&
	       payload->payload.actor_pid &&
	       payload->payload.actor_pid <=
		       static_cast<uint32_t>(std::numeric_limits<int>::max()) &&
	       payload->acknowledged_save_revision && payload->original_level;
}
bool auction_body_pair(const critical_command &command, const auction_recovery_context &recovery,
		       std::span<const uint8_t> before, std::vector<uint8_t> *after)
{
	std::vector<player_item_snapshot> full, expected;
	std::vector<uint8_t> canonical, body;
	std::vector<uint8_t> authenticated;
	auction_native_command_context accepted;
	if (!after || !auction_publication_identity(command, &accepted) ||
	    player_item_snapshot_list_decode(before.data(), before.size(), &full) !=
		    player_snapshot_codec_result::ok ||
	    player_item_snapshot_list_encode(recovery.player_before, &canonical) !=
		    player_snapshot_codec_result::ok ||
	    canonical.size() != before.size() ||
	    !std::equal(canonical.begin(), canonical.end(), before.begin()) ||
	    auction_recovery_context_encode(command, recovery, &authenticated) !=
		    player_snapshot_codec_result::ok ||
	    !auction_native_expected_player_forest(accepted.payload, full,
						   recovery.selected_literals, false,
						   accepted.original_level, &expected) ||
	    player_item_snapshot_list_encode(expected, &body) != player_snapshot_codec_result::ok)
		return false;
	*after = std::move(body);
	return true;
}
bool auction_checkpoint_roots_match(const literal_inventory_checkpoint &slot,
				    const auction_command_payload &payload) noexcept
{
	const size_t count = payload.action == auction_action::list ? payload.item_count : 0;
	if (count > slot.auction_literal_roots.size() || slot.auction_literal_root_count != count)
		return false;
	for (size_t i = 0; i < slot.auction_literal_roots.size(); ++i)
		if (slot.auction_literal_roots[i] != (i < count ? payload.items[i].item_uid : 0))
			return false;
	return true;
}
bool auction_slot_body_pair(const critical_command &command,
			    const literal_inventory_checkpoint &slot, std::vector<uint8_t> *after)
{
	auction_recovery_context original;
	return auction_recovery_context_decode(command, slot.restored_auction_attachment,
					       &original) == player_snapshot_codec_result::ok &&
	       auction_body_pair(command, original, slot.original_auction_before, after);
}
bool auction_slot_runtime_shape(const literal_inventory_checkpoint &slot) noexcept
{
	if (!slot.restored_auction)
		return !slot.auction_runtime_rebound && slot.token.actor_runtime_id &&
		       slot.token.generation;
	if (!slot.restored_auction_revision || slot.restored_auction_attachment.empty())
		return false;
	return slot.auction_runtime_rebound ?
		       (slot.token.actor_runtime_id && slot.token.generation) :
		       (!slot.token.actor_runtime_id && !slot.token.generation);
}
bool auction_envelope_slot_matches_locked(const critical_native_recovery_envelope &envelope)
{
	auction_native_command_context payload{};
	auction_recovery_context recovery;
	std::vector<uint8_t> frozen, before, after;
	if (!auction_recovery_envelope_valid(envelope) ||
	    !auction_publication_identity(envelope.command, &payload) ||
	    auction_recovery_context_decode(envelope.command, envelope.attachment, &recovery) !=
		    player_snapshot_codec_result::ok ||
	    critical_command_encode(envelope.command, &frozen) !=
		    critical_command_codec_result::ok ||
	    player_item_snapshot_list_encode(recovery.player_before, &before) !=
		    player_snapshot_codec_result::ok ||
	    !auction_body_pair(envelope.command, recovery, before, &after))
		return false;
	const int pid = static_cast<int>(payload.payload.actor_pid);
	const auto revision = payload.acknowledged_save_revision;
	const auto *slot = find_literal_inventory_locked(pid);
	return health.initialized && !stop_requested && slot && slot->held &&
	       slot->profile == literal_checkpoint_profile::auction && slot->restored_sql_drop &&
	       slot->execution_hold_generation && slot->level == payload.original_level &&
	       auction_checkpoint_roots_match(*slot, payload.payload) &&
	       slot->captured_revision == revision && slot->acknowledged_revision == revision &&
	       slot->operation_id.bytes == envelope.command.operation_id.bytes &&
	       slot->payload == frozen && slot->original_auction_before == before &&
	       slot->original_auction_after == after && auction_slot_runtime_shape(*slot) &&
	       player_save_execution_guard::publication_operation_held(
		       envelope.command.operation_id);
}

}
#endif

bool player_save_auction_publication_owner::restore_recovery_checkpoint(
	const critical_native_recovery_envelope &envelope) noexcept
{
#ifdef __NO_MYSQL__
	(void)envelope;
	return false;
#else
	try
	{
		auction_native_command_context payload{};
		auction_recovery_context recovery;
		std::vector<uint8_t> frozen, before, after, attachment;
		if (!auction_recovery_envelope_valid(envelope) ||
		    envelope.phase != critical_native_recovery_phase::execution_pending ||
		    !auction_publication_identity(envelope.command, &payload) ||
		    auction_recovery_context_decode(envelope.command, envelope.attachment,
						    &recovery) !=
			    player_snapshot_codec_result::ok ||
		    critical_command_encode(envelope.command, &frozen) !=
			    critical_command_codec_result::ok ||
		    player_item_snapshot_list_encode(recovery.player_before, &before) !=
			    player_snapshot_codec_result::ok ||
		    !auction_body_pair(envelope.command, recovery, before, &after))
			return false;
		size_t bytes = 0;
		for (const size_t size :
		     { frozen.size(), before.size(), after.size(), envelope.attachment.size() })
		{
			if (size > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
				return false;
			bytes += size;
		}
		attachment = envelope.attachment;
		const int pid = static_cast<int>(payload.payload.actor_pid);
		const uint64_t revision = payload.acknowledged_save_revision;
		const uint64_t root = 0;
		if (pid <= 0 || !revision)
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		// Critical replay installs only a prepared passive original hold. No
		// actor runtime identity, save token generation or effects are restored.
		if (!health.initialized || stop_requested || execution_started)
			return false;
		literal_inventory_checkpoint *slot = nullptr;
		for (auto &candidate : literal_inventory_checkpoints)
		{
			if (candidate.token.pid == pid ||
			    (candidate.held &&
			     candidate.operation_id.bytes == envelope.command.operation_id.bytes))
			{
				uint64_t generation = 0;
				if (candidate.profile != literal_checkpoint_profile::auction ||
				    !candidate.restored_auction ||
				    candidate.auction_runtime_rebound ||
				    !candidate.restored_sql_drop || !candidate.held ||
				    candidate.token.pid != pid ||
				    candidate.token.root_uid != root ||
				    candidate.token.actor_runtime_id ||
				    candidate.token.generation || candidate.payload != frozen ||
				    candidate.original_auction_before != before ||
				    candidate.original_auction_after != after ||
				    candidate.restored_auction_revision != envelope.revision ||
				    candidate.restored_auction_attachment != attachment ||
				    candidate.level != payload.original_level ||
				    !auction_checkpoint_roots_match(candidate, payload.payload) ||
				    candidate.captured_revision != revision ||
				    candidate.acknowledged_revision != revision ||
				    candidate.operation_id.bytes !=
					    envelope.command.operation_id.bytes ||
				    !player_save_execution_guard::install_hold(
					    pid, envelope.command.operation_id, &generation))
					return false;
				if (generation != candidate.execution_hold_generation)
				{
					player_save_execution_guard::poison_integrity();
					return false;
				}
				return true;
			}
			if (!candidate.token.pid && !slot)
				slot = &candidate;
		}
		if (!slot || !literal_inventory_capacity_locked(bytes))
			return false;
		uint64_t generation = 0;
		if (!player_save_execution_guard::install_hold(pid, envelope.command.operation_id,
							       &generation))
			return false;
		// All canonical decode, forest derivation, allocation and capacity checks
		// precede hold installation. These default-allocator moves cannot throw.
		slot->profile = literal_checkpoint_profile::auction;
		slot->token = { pid, 0, root, 0 };
		slot->payload = std::move(frozen);
		slot->original_auction_before = std::move(before);
		slot->original_auction_after = std::move(after);
		slot->restored_auction_attachment = std::move(attachment);
		slot->restored_auction_revision = envelope.revision;
		slot->level = payload.original_level;
		slot->auction_literal_root_count = payload.payload.action == auction_action::list ?
							   payload.payload.item_count :
							   0;
		for (size_t i = 0; i < slot->auction_literal_root_count; ++i)
			slot->auction_literal_roots[i] = payload.payload.items[i].item_uid;
		slot->captured_revision = revision;
		slot->acknowledged_revision = revision;
		slot->operation_id = envelope.command.operation_id;
		slot->execution_hold_generation = generation;
		slot->held = true;
		slot->restored_sql_drop = true;
		slot->restored_auction = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_auction_publication_owner::rebind_recovery_checkpoint(
	const critical_native_recovery_envelope &original,
	uint64_t actual_player_runtime_id) noexcept
{
#ifdef __NO_MYSQL__
	(void)original;
	(void)actual_player_runtime_id;
	return false;
#else
	if (!actual_player_runtime_id || !nevent_is_game_thread())
		return false;
	try
	{
		auction_native_command_context payload{};
		if (!auction_publication_identity(original.command, &payload))
			return false;
		const int pid = static_cast<int>(payload.payload.actor_pid);
		P_char actor = find_character_by_runtime_id(actual_player_runtime_id);
		if (pid <= 0 || !actor || !IS_PC(actor) || !actor->only.pc ||
		    actor->runtime_id != actual_player_runtime_id || GET_PID(actor) != pid ||
		    IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED) ||
		    player_save_journal_pid_quarantined(pid) || player_save_worker_pid_pending(pid))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!auction_envelope_slot_matches_locked(original))
			return false;
		auto *slot = find_literal_inventory_locked(pid);
		const uint64_t root = 0;
		if (!slot || !slot->restored_auction || slot->token.pid != pid ||
		    slot->token.root_uid != root ||
		    slot->restored_auction_revision != original.revision ||
		    slot->restored_auction_attachment != original.attachment ||
		    find_terminal_fence_locked(pid) || find_target_save_login_fence_locked(pid) ||
		    append_inflight_pid == pid || any_snapshot_is_retained_locked(pid) ||
		    find_character_by_runtime_id(actual_player_runtime_id) != actor)
			return false;
		if (slot->auction_runtime_rebound)
		{
			if (slot->token.actor_runtime_id == actual_player_runtime_id)
				return true;
			// A reconnect may bind a fresh local identity only after the old actor
			// is absent. A still-registered actor is an unresolved ownership conflict.
			if (find_character_by_runtime_id(slot->token.actor_runtime_id))
				return false;
		}
		if (literal_inventory_generation == std::numeric_limits<uint64_t>::max())
			return false;
		// All canonical and original-body checks precede these nonallocating
		// assignments. No coordinator, journal, save revision or hold is changed.
		slot->token.actor_runtime_id = actual_player_runtime_id;
		slot->token.generation = ++literal_inventory_generation;
		slot->auction_runtime_rebound = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_auction_publication_owner::copy_recovery_context(
	const critical_command &command, critical_native_recovery_envelope *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)output;
	return false;
#else
	if (!output || !nevent_is_game_thread())
		return false;
	try
	{
		critical_native_recovery_envelope envelope;
		// Never acquire coordinator_mutex while pipeline_mutex is held.
		if (!critical_native_auction_publication_owner::copy_context(command, &envelope))
			return false;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			if (!auction_envelope_slot_matches_locked(envelope))
				return false;
		}
		*output = std::move(envelope);
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_auction_publication_owner::checkpoint_recovery_context(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor) noexcept
{
#ifdef __NO_MYSQL__
	(void)expected;
	(void)successor;
	return false;
#else
	if (!nevent_is_game_thread() || !auction_recovery_successor_valid(expected, successor) ||
	    expected.attachment.size() > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
	    successor.attachment.size() > PLAYER_SAVE_PIPELINE_MAX_BYTES)
		return false;
	try
	{
		auction_recovery_context successor_context;
		if (auction_recovery_context_decode(successor.command, successor.attachment,
						    &successor_context) !=
		    player_snapshot_codec_result::ok)
			return false;
		const bool restored_after = successor_context.stage ==
					    auction_recovery_stage::restored_after_proven;
		auction_native_command_context payload{};
		std::vector<uint8_t> frozen, original_before, original_after, original_attachment;
		if (!auction_publication_identity(expected.command, &payload) ||
		    critical_command_encode(expected.command, &frozen) !=
			    critical_command_codec_result::ok)
			return false;
		const int pid = static_cast<int>(payload.payload.actor_pid);
		literal_inventory_checkpoint *pinned = nullptr;
		player_literal_inventory_token token{};
		uint64_t generation = 0, replay_revision = 0;
		player_revision_t captured_revision = 0, acknowledged_revision = 0;
		bool restored = false;
		size_t reserved = 0;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			if (!auction_envelope_slot_matches_locked(expected) ||
			    !auction_envelope_slot_matches_locked(successor))
				return false;
			pinned = find_literal_inventory_locked(pid);
			if (!pinned)
				return false;
			// Only an authentic passive replay slot rebound to the actual loader
			// can adopt AFTER without asserting that old callbacks returned.
			// The private native caller separately proves original SQL/current
			// forest/custody and complete world equality before this checkpoint.
			if (restored_after)
			{
				P_char actor = find_character_by_runtime_id(
					pinned->token.actor_runtime_id);
				if (!pinned->restored_auction || !pinned->auction_runtime_rebound ||
				    !actor || !IS_PC(actor) || !actor->only.pc ||
				    GET_PID(actor) != pid ||
				    IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED))
					return false;
			}
			// All snapshot copies precede capacity reservation and journal I/O.
			// They permit exact nonallocating original-slot checks after the CAS.
			original_before = pinned->original_auction_before;
			original_after = pinned->original_auction_after;
			original_attachment = pinned->restored_auction_attachment;
			token = pinned->token;
			generation = pinned->execution_hold_generation;
			replay_revision = pinned->restored_auction_revision;
			captured_revision = pinned->captured_revision;
			acknowledged_revision = pinned->acknowledged_revision;
			restored = pinned->restored_auction;
			reserved =
				std::max({ pinned->auction_attachment_reserved_bytes,
					   original_attachment.size(), expected.attachment.size(),
					   successor.attachment.size() });
			size_t bytes = 0;
			for (const size_t size : { pinned->payload.size(), original_before.size(),
						   original_after.size(), reserved })
			{
				if (size > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
					return false;
				bytes += size;
			}
			if (!literal_inventory_capacity_locked(bytes, pinned))
				return false;
			pinned->auction_attachment_reserved_bytes = reserved;
		}
		// No pipeline mutex across the exact coordinator/journal CAS. A false
		// result may represent uncertainty, so its high-water charge remains.
		const bool confirmed =
			critical_native_auction_publication_owner::checkpoint_context(expected,
										      successor);
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(pid);
		// No encode/decode/command equality allocation after durable I/O.
		if (!slot || slot != pinned || !slot->held || !slot->restored_sql_drop ||
		    slot->profile != literal_checkpoint_profile::auction || slot->token != token ||
		    slot->execution_hold_generation != generation ||
		    slot->operation_id.bytes != expected.command.operation_id.bytes ||
		    slot->payload != frozen || slot->original_auction_before != original_before ||
		    slot->original_auction_after != original_after ||
		    slot->restored_auction_attachment != original_attachment ||
		    slot->restored_auction_revision != replay_revision ||
		    slot->captured_revision != captured_revision ||
		    slot->acknowledged_revision != acknowledged_revision ||
		    slot->restored_auction != restored ||
		    slot->auction_attachment_reserved_bytes != reserved ||
		    !player_save_execution_guard::publication_operation_held(
			    expected.command.operation_id))
		{
			player_save_execution_guard::poison_integrity();
			return false;
		}
		if (confirmed)
			slot->auction_attachment_reserved_bytes =
				std::max(original_attachment.size(), successor.attachment.size());
		return confirmed;
	}
	catch (...)
	{
		// Every allocation is before reservation. If a later coordinator call
		// unexpectedly throws, the already-reserved high-water charge is retained.
		return false;
	}
#endif
}

critical_submit_result player_save_auction_checkpoint_owner::submit_owned(
	const player_auction_checkpoint_token &token, critical_command command,
	std::span<const uint8_t> original_native_before, bool *checkpoint_released) noexcept
{
	if (!checkpoint_released)
		return critical_submit_result::invalid;
	*checkpoint_released = false;
#ifdef __NO_MYSQL__
	(void)token;
	(void)command;
	(void)original_native_before;
	return critical_submit_result::unavailable;
#else
	if (!nevent_is_game_thread())
		return critical_submit_result::unavailable;
	int pid = 0;
	uint64_t revision = 0, generation = 0;
	auction_native_command_context native_payload{};
	std::vector<uint8_t> after_body, retained_attachment;
	std::vector<uint8_t> frozen;
	bool new_hold = false;
	critical_native_recovery_envelope envelope;
	auction_recovery_context recovery;
	const auto operation = command.operation_id;
	try
	{
		if (!auction_publication_identity(command, &native_payload) ||
		    player_item_snapshot_list_decode(
			    original_native_before.data(), original_native_before.size(),
			    &recovery.selected_literals) != player_snapshot_codec_result::ok)
			return critical_submit_result::invalid;
		pid = static_cast<int>(native_payload.payload.actor_pid);
		revision = native_payload.acknowledged_save_revision;
		if (pid != token.pid || !token.actor_runtime_id || !token.generation ||
		    token.root_uid != 0 ||
		    critical_command_encode(command, &frozen) !=
			    critical_command_codec_result::ok ||
		    frozen.size() > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
		    player_save_worker_pid_pending(pid))
			return critical_submit_result::invalid;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(pid);
		if (!health.initialized || stop_requested || !accepting || !execution_started ||
		    !health.replay_complete || health.replay_blocked ||
		    find_terminal_fence_locked(pid) || find_target_save_login_fence_locked(pid) ||
		    append_inflight_pid == pid || any_snapshot_is_retained_locked(pid) || !slot)
			return critical_submit_result::unavailable;
		if (slot->profile != literal_checkpoint_profile::auction || !slot->held ||
		    slot->token != auction_checkpoint_identity(token) ||
		    slot->operation_id.bytes != operation.bytes ||
		    !auction_checkpoint_roots_match(*slot, native_payload.payload))
			return critical_submit_result::identity_conflict;
		// The original native owner supplies its captured literal stock. The
		// save owner supplies the actual acknowledged giver forest. Encode both
		// before acquiring a new leaf hold or entering the coordinator.
		const auto &original_player_before =
			slot->restored_sql_drop ? slot->original_auction_before : slot->payload;
		if (player_item_snapshot_list_decode(
			    original_player_before.data(), original_player_before.size(),
			    &recovery.player_before) != player_snapshot_codec_result::ok)
			return critical_submit_result::invalid;
		recovery.actor_pid = static_cast<uint32_t>(pid);
		recovery.original_level = native_payload.original_level;
		recovery.before_save_revision = revision;
		recovery.before_present = true;
		recovery.reload.resize(recovery.selected_literals.size());
		recovery.proclib.resize(recovery.selected_literals.size());
		envelope.command = command;
		envelope.revision = 1;
		envelope.phase = critical_native_recovery_phase::execution_pending;
		if (auction_recovery_context_encode(command, recovery, &envelope.attachment) !=
			    player_snapshot_codec_result::ok ||
		    !auction_recovery_initial_valid(envelope))
			return critical_submit_result::invalid;
		if (slot->restored_sql_drop)
		{
			if (slot->payload != frozen || !slot->execution_hold_generation ||
			    slot->restored_auction ||
			    slot->restored_auction_revision != envelope.revision ||
			    slot->restored_auction_attachment != envelope.attachment)
				return critical_submit_result::identity_conflict;
			generation = slot->execution_hold_generation;
		}
		else
		{
			if (!auction_body_pair(command, recovery, slot->payload, &after_body))
				return critical_submit_result::invalid;
			size_t recoverable_slot_bytes = 0;
			for (const size_t size : { frozen.size(), slot->payload.size(),
						   after_body.size(), envelope.attachment.size() })
			{
				if (size > PLAYER_SAVE_PIPELINE_MAX_BYTES - recoverable_slot_bytes)
					return critical_submit_result::overloaded;
				recoverable_slot_bytes += size;
			}
			if (slot->level != native_payload.original_level ||
			    !auction_checkpoint_roots_match(*slot, native_payload.payload) ||
			    slot->captured_revision != revision ||
			    slot->acknowledged_revision != revision ||
			    slot->payload.size() < sizeof(uint32_t) ||
			    !slot->original_auction_before.empty() ||
			    !slot->original_auction_after.empty() || slot->restored_auction ||
			    slot->restored_auction_revision ||
			    !slot->restored_auction_attachment.empty() ||
			    slot->payload.size() > PLAYER_SAVE_PIPELINE_MAX_BYTES - frozen.size() ||
			    after_body.size() > PLAYER_SAVE_PIPELINE_MAX_BYTES - frozen.size() -
							slot->payload.size() ||
			    !literal_inventory_capacity_locked(recoverable_slot_bytes, slot))
				return critical_submit_result::unavailable;
			// Retain the exact original recovery bytes and aggregate charge before
			// installing the hold. Allocation failure leaves the original slot intact.
			retained_attachment = envelope.attachment;
			if (!player_save_execution_guard::install_live_publication_hold(
				    pid, operation, &generation))
				return critical_submit_result::unavailable;
			// All allocation and combined capacity checks precede the leaf hold.
			// Move the actual original body, then the command; neither move throws.
			slot->original_auction_before = std::move(slot->payload);
			slot->original_auction_after = std::move(after_body);
			slot->payload = std::move(frozen);
			slot->restored_auction_attachment = std::move(retained_attachment);
			slot->restored_auction_revision = envelope.revision;
			// This remains a real live actor slot, not passive restored enrollment.
			slot->execution_hold_generation = generation;
			slot->restored_sql_drop = true;
			new_hold = true;
		}
	}
	catch (...)
	{
		return critical_submit_result::invalid;
	}
	critical_submit_result submitted;
	try
	{
		// Never enter coordinator_mutex while pipeline_mutex is held.
		submitted = critical_native_auction_submission_owner::submit(std::move(envelope));
	}
	catch (...)
	{
		// Admission may have happened; the exact original slot remains fenced.
		return critical_submit_result::journal_uncertain;
	}
	if (!critical_submit_result_keeps_operation(submitted) && new_hold)
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(pid);
		if (!slot || slot->profile != literal_checkpoint_profile::auction || !slot->held ||
		    !slot->restored_sql_drop || slot->token != auction_checkpoint_identity(token) ||
		    slot->operation_id.bytes != operation.bytes ||
		    slot->execution_hold_generation != generation)
		{
			player_save_execution_guard::poison_integrity();
			return critical_submit_result::journal_uncertain;
		}
		if (!player_save_execution_guard::release_hold(pid, operation, generation))
			return critical_submit_result::journal_uncertain;
		*slot = {};
		*checkpoint_released = true;
		// Definite pre-journal refusal releases only this exact reservation.
		// The preparation owner separately retires its native checkpoint facts.
		return submitted;
	}
	return critical_submit_result_keeps_operation(submitted) ?
		       submitted :
		       critical_submit_result::journal_uncertain;
#endif
}

bool player_save_auction_checkpoint_owner::original_held_bodies(
	const critical_command &command, std::vector<player_item_snapshot> *before,
	std::vector<player_item_snapshot> *after, player_auction_checkpoint_stage *stage) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)before;
	(void)after;
	(void)stage;
	return false;
#else
	if (!before || !after || before == after || !stage)
		return false;
	try
	{
		auction_native_command_context payload{};
		std::vector<uint8_t> frozen;
		if (!auction_publication_identity(command, &payload) ||
		    critical_command_encode(command, &frozen) !=
			    critical_command_codec_result::ok ||
		    player_save_worker_pid_pending(payload.payload.actor_pid))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto pid = static_cast<int>(payload.payload.actor_pid);
		const auto *slot = find_literal_inventory_locked(pid);
		const auto revision = payload.acknowledged_save_revision;
		if (!health.initialized || stop_requested || !slot || !slot->held ||
		    !slot->restored_sql_drop ||
		    slot->profile != literal_checkpoint_profile::auction ||
		    slot->operation_id.bytes != command.operation_id.bytes ||
		    slot->payload != frozen || !slot->execution_hold_generation ||
		    slot->level != payload.original_level ||
		    !auction_checkpoint_roots_match(*slot, payload.payload) ||
		    slot->captured_revision != revision ||
		    slot->acknowledged_revision != revision || !auction_slot_runtime_shape(*slot) ||
		    find_terminal_fence_locked(pid) || find_target_save_login_fence_locked(pid) ||
		    append_inflight_pid == pid || any_snapshot_is_retained_locked(pid) ||
		    !player_save_execution_guard::publication_operation_held(command.operation_id))
			return false;
		std::vector<uint8_t> expected_after;
		if (!auction_slot_body_pair(command, *slot, &expected_after) ||
		    expected_after != slot->original_auction_after)
			return false;
		std::vector<player_item_snapshot> a, b;
		if (player_item_snapshot_list_decode(slot->original_auction_before.data(),
						     slot->original_auction_before.size(),
						     &a) != player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_decode(slot->original_auction_after.data(),
						     slot->original_auction_after.size(),
						     &b) != player_snapshot_codec_result::ok)
			return false;
		*before = std::move(a);
		*after = std::move(b);
		*stage = { revision, slot->level };
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_auction_publication_owner::publish_auction(
	const critical_command &original, const critical_completion &completion,
	bool (*native_publish)(const critical_command &, const critical_completion &,
			       void *) noexcept,
	void *context) noexcept
{
#ifdef __NO_MYSQL__
	(void)original;
	(void)completion;
	(void)native_publish;
	(void)context;
	return false;
#else
	if (!nevent_is_game_thread() || !native_publish ||
	    !critical_completion_disposition_valid(completion) ||
	    (completion.disposition != critical_completion_disposition::execution &&
	     completion.disposition != critical_completion_disposition::never_admitted) ||
	    original.operation_id.bytes != completion.operation_id.bytes)
		return false;
	try
	{
		auction_native_command_context payload{};
		std::vector<uint8_t> frozen;
		critical_command command;
		if (!auction_publication_identity(original, &payload) ||
		    critical_command_encode(original, &frozen) != critical_command_codec_result::ok)
			return false;
		const int pid = static_cast<int>(payload.payload.actor_pid);
		uint64_t generation = 0;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			const auto *slot = find_literal_inventory_locked(pid);
			std::vector<uint8_t> expected_after;
			if (!health.initialized || stop_requested || !slot || !slot->held ||
			    !slot->restored_sql_drop ||
			    slot->profile != literal_checkpoint_profile::auction ||
			    !slot->token.actor_runtime_id || !slot->token.generation ||
			    slot->payload != frozen || !slot->execution_hold_generation ||
			    slot->captured_revision != payload.acknowledged_save_revision ||
			    slot->acknowledged_revision != slot->captured_revision ||
			    slot->original_auction_before.size() < sizeof(uint32_t) ||
			    !auction_slot_body_pair(original, *slot, &expected_after) ||
			    expected_after != slot->original_auction_after ||
			    slot->operation_id.bytes != completion.operation_id.bytes ||
			    find_terminal_fence_locked(pid) ||
			    find_target_save_login_fence_locked(pid) ||
			    append_inflight_pid == pid || any_snapshot_is_retained_locked(pid) ||
			    critical_command_decode(frozen.data(), frozen.size(), &command) !=
				    critical_command_codec_result::ok)
				return false;
			generation = slot->execution_hold_generation;
		}
		player_save_restored_publication_owner owner(
			std::move(command), std::move(frozen), completion,
			player_save_execution_guard::current_ownership_epoch(), pid, generation);
		if (!owner.reservation_.valid() || player_save_worker_pid_pending(pid))
			return false;
		// Frozen native/player inventory and level are proved by the native
		// participant; later volatile STATUS dirt remains eligible for capture.
		if (completion.disposition == critical_completion_disposition::never_admitted)
			return critical_command_coordinator_cancel_auction_publication(
				owner, native_publish, context);
		player_revision_snapshot revision{};
		if (player_revision_snapshot_copy(pid, &revision) &&
		    (revision.overflowed || revision.queued_components ||
		     revision.inflight_components))
			return false;
		std::vector<player_save_journal_retained_frame> originals;
		if (player_save_journal_collect_publication_frames(
			    pid, owner.reservation_, &originals) != player_save_journal_result::ok)
			return false;
		if (!originals.empty())
		{
			if (!player_save_restored_publication_owner::retire_covered_ordinary(
				    owner, originals))
				return false;
		}
		if (player_save_journal_publication_census(pid, owner.reservation_) !=
			    player_save_journal_result::ok ||
		    !native_publish(owner.command_, completion, context) ||
		    !owner.reservation_.valid() ||
		    player_save_journal_publication_census(pid, owner.reservation_) !=
			    player_save_journal_result::ok)
			return false;
		critical_native_recovery_envelope terminal;
		if (!copy_recovery_context(original, &terminal) ||
		    !auction_recovery_publication_context_valid(terminal, completion))
			return false;
		owner.publication_proven_ = true;
		return critical_command_coordinator_acknowledge_publication(owner);
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_auction_publication_owner::publication_held_bodies(
	const critical_command &command, std::vector<player_item_snapshot> *before,
	std::vector<player_item_snapshot> *after, player_auction_checkpoint_stage *stage) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)before;
	(void)after;
	(void)stage;
	return false;
#else
	if (!before || !after || before == after || !stage)
		return false;
	try
	{
		auction_native_command_context payload{};
		std::vector<uint8_t> frozen;
		if (!auction_publication_identity(command, &payload) ||
		    critical_command_encode(command, &frozen) !=
			    critical_command_codec_result::ok ||
		    player_save_worker_pid_pending(payload.payload.actor_pid))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto pid = static_cast<int>(payload.payload.actor_pid);
		const auto *slot = find_literal_inventory_locked(pid);
		const auto revision = payload.acknowledged_save_revision;
		if (!health.initialized || stop_requested || !slot || !slot->held ||
		    !slot->restored_sql_drop ||
		    slot->profile != literal_checkpoint_profile::auction ||
		    slot->operation_id.bytes != command.operation_id.bytes ||
		    slot->payload != frozen || !slot->execution_hold_generation ||
		    slot->level != payload.original_level ||
		    !auction_checkpoint_roots_match(*slot, payload.payload) ||
		    slot->captured_revision != revision ||
		    slot->acknowledged_revision != revision || !slot->token.actor_runtime_id ||
		    !slot->token.generation || find_terminal_fence_locked(pid) ||
		    find_target_save_login_fence_locked(pid) || append_inflight_pid == pid ||
		    any_snapshot_is_retained_locked(pid) ||
		    !player_save_execution_guard::publication_operation_held(command.operation_id))
			return false;
		std::vector<uint8_t> expected_after;
		if (!auction_slot_body_pair(command, *slot, &expected_after) ||
		    expected_after != slot->original_auction_after)
			return false;
		std::vector<player_item_snapshot> a, b;
		if (player_item_snapshot_list_decode(slot->original_auction_before.data(),
						     slot->original_auction_before.size(),
						     &a) != player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_decode(slot->original_auction_after.data(),
						     slot->original_auction_after.size(),
						     &b) != player_snapshot_codec_result::ok)
			return false;
		// Publication authenticates the actual final-giver checkpoint even when
		// consumption mutates no player inventory. Do not return an empty model.
		*before = std::move(a);
		*after = std::move(b);
		*stage = { revision, slot->level };
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

// Original held-retirement typed profile. No quest or SHOP capabilities.
namespace
{
player_literal_inventory_token
held_retirement_checkpoint_identity(const player_held_retirement_checkpoint_token &token)
{
	return { token.pid, token.actor_runtime_id, token.root_uid, token.generation };
}
[[maybe_unused]] bool
held_retirement_actor_matches(const player_held_retirement_checkpoint_token &token, P_char actor)
{
	return actor && IS_PC(actor) && actor->only.pc && token.pid > 0 &&
	       GET_PID(actor) == token.pid && token.actor_runtime_id && token.generation &&
	       actor->runtime_id == token.actor_runtime_id &&
	       find_character_by_runtime_id(token.actor_runtime_id) == actor &&
	       !IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED) && token.root_uid == 0 &&
	       token.selected_uid && actor->equipment[HOLD] &&
	       actor->equipment[HOLD]->obj_uid == token.selected_uid &&
	       OBJ_WORN_BY(actor->equipment[HOLD], actor) &&
	       actor->equipment[HOLD]->type == ITEM_PICK && !actor->equipment[HOLD]->contains;
}
}

player_literal_inventory_state player_save_pipeline_held_retirement_checkpoint_begin(
	P_char actor, P_obj root, int room_vnum, player_held_retirement_checkpoint_token *token_out)
{
#ifdef __NO_MYSQL__
	(void)actor;
	(void)root;
	(void)room_vnum;
	(void)token_out;
	return player_literal_inventory_state::refused;
#else
	if (!actor || !token_out || !IS_PC(actor) || !actor->only.pc || GET_PID(actor) <= 0 ||
	    !actor->runtime_id || find_character_by_runtime_id(actor->runtime_id) != actor ||
	    !economic_gameplay_authority::active_regular_sql() ||
	    (!root || !root->obj_uid || actor->equipment[HOLD] != root ||
	     !OBJ_WORN_BY(root, actor) || root->type != ITEM_PICK || root->contains) ||
	    IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED) ||
	    player_save_journal_pid_quarantined(GET_PID(actor)))
		return player_literal_inventory_state::refused;
	const uint64_t root_uid = 0;
	player_snapshot captured;
	std::vector<uint8_t> blob;
	uint32_t level = 0;
	if (player_snapshot_capture_literal_inventory(
		    actor, 1, HELD_RETIREMENT_CHECKPOINT_COMPONENTS, RENT_CRASH, room_vnum,
		    root_uid, &captured) != player_snapshot_capture_result::ok ||
	    !held_retirement_checkpoint_blob(captured, &blob, &level))
		return player_literal_inventory_state::refused;
	player_held_retirement_checkpoint_token token;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!economic_gameplay_authority::active_regular_sql() || !health.initialized ||
		    stop_requested || !accepting || !health.replay_complete ||
		    health.replay_blocked || find_target_save_login_fence_locked(GET_PID(actor)) ||
		    find_terminal_fence_locked(GET_PID(actor)))
			return player_literal_inventory_state::refused;
		if (auto *existing = find_literal_inventory_locked(GET_PID(actor)))
		{
			if (existing->profile != literal_checkpoint_profile::held_retirement ||
			    existing->token.actor_runtime_id != actor->runtime_id ||
			    existing->token.root_uid != root_uid ||
			    existing->held_selected_uid != root->obj_uid ||
			    existing->payload != blob || existing->held)
				return player_literal_inventory_state::refused;
			*token_out = { existing->token.pid, existing->token.actor_runtime_id,
				       existing->token.root_uid, existing->token.generation,
				       existing->held_selected_uid };
			return player_literal_inventory_state::pending;
		}
		if (literal_inventory_generation == std::numeric_limits<uint64_t>::max())
			return player_literal_inventory_state::refused;
		literal_inventory_checkpoint *slot = nullptr;
		for (auto &candidate : literal_inventory_checkpoints)
			if (!candidate.token.pid)
			{
				slot = &candidate;
				break;
			}
		if (!slot || !literal_inventory_capacity_locked(blob.size()))
			return player_literal_inventory_state::refused;
		token = { GET_PID(actor), actor->runtime_id, root_uid,
			  ++literal_inventory_generation, root->obj_uid };
		slot->profile = literal_checkpoint_profile::held_retirement;
		slot->held_selected_uid = token.selected_uid;
		slot->level = level;
		slot->token = held_retirement_checkpoint_identity(token);
		slot->payload = std::move(blob);
	}
	// Make the installed original slot recoverable before enqueue can throw.
	*token_out = token;
	player_save_pipeline_result queued;
	try
	{
		queued = player_save_pipeline_request(actor, HELD_RETIREMENT_CHECKPOINT_COMPONENTS,
						      RENT_CRASH, room_vnum);
	}
	catch (...)
	{
		// Queue outcome is unresolved. Original token and frozen save policy remain.
		return player_literal_inventory_state::pending;
	}
	if (queued != player_save_pipeline_result::queued &&
	    queued != player_save_pipeline_result::coalesced)
	{
		player_save_pipeline_held_retirement_checkpoint_cancel(token);
		return player_literal_inventory_state::refused;
	}
	*token_out = token;
	return player_literal_inventory_state::pending;
#endif
}

player_literal_inventory_state player_save_pipeline_held_retirement_checkpoint_poll(
	const player_held_retirement_checkpoint_token &token, P_char actor,
	player_held_retirement_checkpoint_stage *stage_out)
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)stage_out;
	return player_literal_inventory_state::refused;
#else
	const auto identity = held_retirement_checkpoint_identity(token);
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto *literal = find_literal_inventory_locked(token.pid);
		if (!literal || literal->profile != literal_checkpoint_profile::held_retirement ||
		    literal->token != identity ||
		    literal->held_selected_uid != token.selected_uid || literal->held)
			return player_literal_inventory_state::refused;
	}
	if (!economic_gameplay_authority::active_regular_sql() ||
	    !held_retirement_actor_matches(token, actor) ||
	    player_save_journal_pid_quarantined(token.pid))
	{
		player_save_pipeline_held_retirement_checkpoint_cancel(token);
		return player_literal_inventory_state::refused;
	}
	player_snapshot captured;
	std::vector<uint8_t> blob;
	if (player_snapshot_capture_literal_inventory(
		    actor, 1, HELD_RETIREMENT_CHECKPOINT_COMPONENTS, RENT_CRASH, NOWHERE,
		    token.root_uid, &captured) != player_snapshot_capture_result::ok ||
	    !held_retirement_checkpoint_blob(captured, &blob))
	{
		player_save_pipeline_held_retirement_checkpoint_cancel(token);
		return player_literal_inventory_state::refused;
	}
	if (player_save_worker_pid_pending(token.pid))
		return player_literal_inventory_state::pending;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	if (!literal || literal->profile != literal_checkpoint_profile::held_retirement ||
	    literal->token != identity || literal->held_selected_uid != token.selected_uid ||
	    literal->held)
		return player_literal_inventory_state::refused;
	if (literal->payload != blob)
	{
		*literal = {};
		return player_literal_inventory_state::refused;
	}
	player_revision_snapshot revision = {};
	if (!player_revision_snapshot_copy(token.pid, &revision) || revision.overflowed)
		return player_literal_inventory_state::refused;
	if (!literal->captured_revision ||
	    literal->acknowledged_revision != literal->captured_revision ||
	    revision.current_revision != literal->captured_revision ||
	    revision.acknowledged_revision != literal->captured_revision ||
	    revision.dirty_components || revision.unacknowledged_components ||
	    revision.queued_components || revision.inflight_components ||
	    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
		return player_literal_inventory_state::pending;
	if (stage_out)
		*stage_out = { literal->acknowledged_revision };
	return player_literal_inventory_state::database_acknowledged;
#endif
}

bool player_save_pipeline_held_retirement_checkpoint_hold(
	const player_held_retirement_checkpoint_token &token,
	const critical_operation_id &operation_id)
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)operation_id;
	return false;
#else
	if (std::all_of(operation_id.bytes.begin(), operation_id.bytes.end(),
			[](uint8_t byte) { return !byte; }) ||
	    player_save_pipeline_held_retirement_checkpoint_poll(
		    token, find_character_by_runtime_id(token.actor_runtime_id)) !=
		    player_literal_inventory_state::database_acknowledged ||
	    player_save_worker_pid_pending(token.pid))
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	player_revision_snapshot revision = {};
	if (!economic_gameplay_authority::active_regular_sql() || !health.initialized ||
	    stop_requested || !accepting || !health.replay_complete || health.replay_blocked ||
	    find_terminal_fence_locked(token.pid) ||
	    find_target_save_login_fence_locked(token.pid) || !literal ||
	    literal->profile != literal_checkpoint_profile::held_retirement ||
	    literal->held_selected_uid != token.selected_uid ||
	    literal->token != held_retirement_checkpoint_identity(token) || literal->held ||
	    !literal->captured_revision ||
	    literal->acknowledged_revision != literal->captured_revision ||
	    !player_revision_snapshot_copy(token.pid, &revision) || revision.overflowed ||
	    revision.current_revision != literal->captured_revision ||
	    revision.acknowledged_revision != literal->captured_revision ||
	    revision.dirty_components || revision.unacknowledged_components ||
	    revision.queued_components || revision.inflight_components ||
	    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
		return false;
	literal->operation_id = operation_id;
	literal->held = true;
	return true;
#endif
}

bool player_save_pipeline_held_retirement_checkpoint_release(
	const player_held_retirement_checkpoint_token &token,
	const critical_operation_id &operation_id)
{
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	if (!literal || literal->profile != literal_checkpoint_profile::held_retirement ||
	    literal->held_selected_uid != token.selected_uid ||
	    literal->token != held_retirement_checkpoint_identity(token) || !literal->held ||
	    literal->restored_sql_drop || literal->operation_id.bytes != operation_id.bytes)
		return false;
	*literal = {};
	return true;
}

bool player_save_pipeline_held_retirement_checkpoint_cancel(
	const player_held_retirement_checkpoint_token &token)
{
	if (token.pid <= 0 || !token.actor_runtime_id || !token.generation)
		return false;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *literal = find_literal_inventory_locked(token.pid);
	// Poll may already have cancelled this unheld checkpoint on a changed body.
	// Absence consumes no save/publication reservation and grants no ACK authority.
	if (!literal)
		return true;
	if (literal->profile != literal_checkpoint_profile::held_retirement ||
	    literal->held_selected_uid != token.selected_uid ||
	    literal->token != held_retirement_checkpoint_identity(token) || literal->held)
		return false;
	*literal = {};
	return true;
}

bool player_save_held_retirement_checkpoint_owner::observe_held(
	const player_held_retirement_checkpoint_token &token, P_char actor,
	const critical_operation_id &operation, player_held_retirement_checkpoint_stage *output,
	std::vector<player_item_snapshot> *items_out) noexcept
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)operation;
	(void)output;
	(void)items_out;
	return false;
#else
	if (!output || !nevent_is_game_thread() ||
	    !economic_gameplay_authority::active_regular_sql() ||
	    !held_retirement_actor_matches(token, actor) ||
	    player_save_journal_pid_quarantined(token.pid) ||
	    player_save_worker_pid_pending(token.pid))
		return false;
	try
	{
		player_snapshot captured;
		std::vector<uint8_t> blob;
		uint32_t level = 0;
		if (player_snapshot_capture_literal_inventory(
			    actor, 1, HELD_RETIREMENT_CHECKPOINT_COMPONENTS, RENT_CRASH, NOWHERE,
			    token.root_uid, &captured) != player_snapshot_capture_result::ok ||
		    !held_retirement_checkpoint_blob(captured, &blob, &level))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto *slot = find_literal_inventory_locked(token.pid);
		if (!health.initialized || stop_requested || !accepting ||
		    !health.replay_complete || health.replay_blocked ||
		    find_terminal_fence_locked(token.pid) ||
		    find_target_save_login_fence_locked(token.pid) || !slot ||
		    slot->profile != literal_checkpoint_profile::held_retirement ||
		    slot->held_selected_uid != token.selected_uid ||
		    slot->token != held_retirement_checkpoint_identity(token) || !slot->held ||
		    slot->restored_sql_drop || slot->operation_id.bytes != operation.bytes ||
		    slot->payload != blob || slot->level != level || !slot->captured_revision ||
		    slot->acknowledged_revision != slot->captured_revision ||
		    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
			return false;
		// Unrelated dirty STATUS marks may advance while the native operation is held.
		// Frozen inventory/level and the actual acknowledged save revision must match.
		// This exact current body was compared to the original held slot.
		// Copying values exposes no execution hold or ACK capability.
		if (items_out)
			*items_out = std::move(captured.items);
		*output = { slot->acknowledged_revision };
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

namespace
{
[[maybe_unused]] bool held_retirement_slot_command(const literal_inventory_checkpoint &slot,
						   const critical_command &command,
						   held_retirement_recovery *context = nullptr)
{
	std::vector<uint8_t> frozen;
	lockpick_retirement_terms terms;
	held_retirement_recovery original;
	if (!slot.held || !slot.restored_sql_drop ||
	    slot.profile != literal_checkpoint_profile::held_retirement ||
	    !slot.execution_hold_generation || !slot.held_recovery_revision ||
	    !slot.captured_revision || slot.captured_revision != slot.acknowledged_revision ||
	    !held_retirement_command_identity(command, nullptr, &terms) ||
	    critical_command_encode(command, &frozen) != critical_command_codec_result::ok ||
	    slot.payload != frozen || slot.operation_id.bytes != command.operation_id.bytes ||
	    slot.token.pid != static_cast<int32_t>(terms.actor_pid) || slot.token.root_uid ||
	    slot.held_selected_uid != terms.item_uid ||
	    !held_retirement_recovery_decode(command, slot.held_attachment, &original) ||
	    original.save_revision != slot.acknowledged_revision ||
	    !player_save_execution_guard::publication_operation_held(command.operation_id))
		return false;
	if (slot.held_passive_restored && !slot.held_runtime_rebound)
	{
		if (slot.token.actor_runtime_id || slot.token.generation)
			return false;
	}
	else if (!slot.token.actor_runtime_id || !slot.token.generation)
		return false;
	std::vector<uint8_t> before, after;
	if (player_item_snapshot_list_encode(original.before, &before) !=
		    player_snapshot_codec_result::ok ||
	    player_item_snapshot_list_encode(original.after, &after) !=
		    player_snapshot_codec_result::ok ||
	    slot.held_before != before || slot.held_after != after)
		return false;
	if (context)
		*context = std::move(original);
	return true;
}
}

critical_submit_result player_save_held_retirement_checkpoint_owner::submit_owned(
	const player_held_retirement_checkpoint_token &token, critical_command command,
	bool *checkpoint_released) noexcept
{
	if (!checkpoint_released)
		return critical_submit_result::invalid;
	*checkpoint_released = false;
#ifdef __NO_MYSQL__
	(void)token;
	(void)command;
	return critical_submit_result::unavailable;
#else
	if (!nevent_is_game_thread())
		return critical_submit_result::unavailable;
	critical_native_recovery_envelope envelope;
	bool new_hold = false;
	uint64_t generation = 0;
	const auto operation = command.operation_id;
	try
	{
		lockpick_retirement_terms terms;
		item_transfer_payload payload;
		std::vector<uint8_t> frozen, after, attachment;
		if (!held_retirement_command_identity(command, &payload, &terms) ||
		    token.root_uid || token.selected_uid != terms.item_uid ||
		    token.pid != static_cast<int32_t>(terms.actor_pid) || !token.actor_runtime_id ||
		    !token.generation || player_save_worker_pid_pending(token.pid) ||
		    critical_command_encode(command, &frozen) != critical_command_codec_result::ok)
			return critical_submit_result::invalid;
		envelope.command = command;
		envelope.revision = 1;
		envelope.phase = critical_native_recovery_phase::execution_pending;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(token.pid);
		if (!health.initialized || stop_requested || !accepting || !execution_started ||
		    !health.replay_complete || health.replay_blocked ||
		    find_terminal_fence_locked(token.pid) ||
		    find_target_save_login_fence_locked(token.pid) ||
		    append_inflight_pid == token.pid ||
		    any_snapshot_is_retained_locked(token.pid) || !slot)
			return critical_submit_result::unavailable;
		if (slot->profile != literal_checkpoint_profile::held_retirement || !slot->held ||
		    slot->token != held_retirement_checkpoint_identity(token) ||
		    slot->held_selected_uid != token.selected_uid ||
		    slot->operation_id.bytes != operation.bytes)
			return critical_submit_result::identity_conflict;
		if (slot->restored_sql_drop)
		{
			if (slot->held_passive_restored ||
			    !held_retirement_slot_command(*slot, command))
				return critical_submit_result::identity_conflict;
			envelope.revision = slot->held_recovery_revision;
			envelope.attachment = slot->held_attachment;
			generation = slot->execution_hold_generation;
		}
		else
		{
			held_retirement_recovery original;
			original.save_revision = slot->acknowledged_revision;
			if (!original.save_revision ||
			    slot->captured_revision != original.save_revision ||
			    !held_retirement_body_pair(payload, slot->payload, &after) ||
			    player_item_snapshot_list_decode(
				    slot->payload.data(), slot->payload.size(), &original.before) !=
				    player_snapshot_codec_result::ok ||
			    player_item_snapshot_list_decode(after.data(), after.size(),
							     &original.after) !=
				    player_snapshot_codec_result::ok ||
			    !held_retirement_recovery_encode(command, original,
							     &envelope.attachment))
				return critical_submit_result::invalid;
			size_t bytes = 0;
			for (size_t size : { frozen.size(), slot->payload.size(), after.size(),
					     envelope.attachment.size() })
			{
				if (size > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
					return critical_submit_result::overloaded;
				bytes += size;
			}
			if (!literal_inventory_capacity_locked(bytes, slot))
				return critical_submit_result::overloaded;
			attachment = envelope.attachment;
			if (!player_save_execution_guard::install_hold(token.pid, operation,
								       &generation))
				return critical_submit_result::unavailable;
			// All allocation/canonical/capacity checks precede the execution hold.
			slot->held_before = std::move(slot->payload);
			slot->held_after = std::move(after);
			slot->payload = std::move(frozen);
			slot->held_attachment = std::move(attachment);
			slot->held_recovery_revision = envelope.revision;
			slot->execution_hold_generation = generation;
			slot->restored_sql_drop = true;
			new_hold = true;
		}
	}
	catch (...)
	{
		return critical_submit_result::invalid;
	}
	critical_submit_result submitted;
	try
	{
		submitted = critical_held_retirement_submission_owner::submit(std::move(envelope));
	}
	catch (...)
	{
		return critical_submit_result::journal_uncertain;
	}
	if (critical_submit_result_keeps_operation(submitted))
		return submitted;
	if (!new_hold)
		return critical_submit_result::journal_uncertain;
	std::lock_guard<std::mutex> lock(pipeline_mutex);
	auto *slot = find_literal_inventory_locked(token.pid);
	if (!slot || slot->profile != literal_checkpoint_profile::held_retirement || !slot->held ||
	    !slot->restored_sql_drop || slot->token != held_retirement_checkpoint_identity(token) ||
	    slot->held_selected_uid != token.selected_uid ||
	    slot->operation_id.bytes != operation.bytes ||
	    slot->execution_hold_generation != generation ||
	    !player_save_execution_guard::release_hold(token.pid, operation, generation))
	{
		player_save_execution_guard::poison_integrity();
		return critical_submit_result::journal_uncertain;
	}
	*slot = {};
	*checkpoint_released = true;
	return submitted;
#endif
}

bool player_save_held_retirement_checkpoint_owner::original_held_bodies(
	const critical_command &command, std::vector<player_item_snapshot> *before,
	std::vector<player_item_snapshot> *after,
	player_held_retirement_checkpoint_stage *stage) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)before;
	(void)after;
	(void)stage;
	return false;
#else
	if (!before || !after || before == after || !stage)
		return false;
	try
	{
		lockpick_retirement_terms terms;
		if (!held_retirement_command_identity(command, nullptr, &terms) ||
		    player_save_worker_pid_pending(terms.actor_pid))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto *slot = find_literal_inventory_locked(terms.actor_pid);
		held_retirement_recovery original;
		if (!health.initialized || stop_requested || !slot ||
		    find_terminal_fence_locked(terms.actor_pid) ||
		    find_target_save_login_fence_locked(terms.actor_pid) ||
		    append_inflight_pid == static_cast<int>(terms.actor_pid) ||
		    any_snapshot_is_retained_locked(terms.actor_pid) ||
		    !held_retirement_slot_command(*slot, command, &original))
			return false;
		*before = std::move(original.before);
		*after = std::move(original.after);
		*stage = { original.save_revision };
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_held_retirement_publication_owner::restore_recovery_checkpoint(
	const critical_native_recovery_envelope &envelope) noexcept
{
#ifdef __NO_MYSQL__
	(void)envelope;
	return false;
#else
	try
	{
		lockpick_retirement_terms terms;
		held_retirement_recovery original;
		std::vector<uint8_t> frozen, before, after, attachment;
		if (!envelope.revision ||
		    envelope.phase != critical_native_recovery_phase::execution_pending ||
		    !held_retirement_command_identity(envelope.command, nullptr, &terms) ||
		    !held_retirement_recovery_decode(envelope.command, envelope.attachment,
						     &original) ||
		    critical_command_encode(envelope.command, &frozen) !=
			    critical_command_codec_result::ok ||
		    player_item_snapshot_list_encode(original.before, &before) !=
			    player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_encode(original.after, &after) !=
			    player_snapshot_codec_result::ok)
			return false;
		attachment = envelope.attachment;
		size_t bytes = 0;
		for (size_t size :
		     { frozen.size(), before.size(), after.size(), attachment.size() })
		{
			if (size > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
				return false;
			bytes += size;
		}
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		if (!health.initialized || stop_requested || execution_started)
			return false;
		literal_inventory_checkpoint *slot = nullptr;
		for (auto &candidate : literal_inventory_checkpoints)
		{
			if (candidate.token.pid == static_cast<int>(terms.actor_pid) ||
			    (candidate.held &&
			     candidate.operation_id.bytes == envelope.command.operation_id.bytes))
			{
				uint64_t generation = 0;
				if (!candidate.held_passive_restored ||
				    candidate.held_runtime_rebound ||
				    candidate.held_recovery_revision != envelope.revision ||
				    candidate.held_attachment != attachment ||
				    candidate.token.actor_runtime_id ||
				    candidate.token.generation ||
				    !held_retirement_slot_command(candidate, envelope.command) ||
				    !player_save_execution_guard::install_hold(
					    terms.actor_pid, envelope.command.operation_id,
					    &generation) ||
				    generation != candidate.execution_hold_generation)
					return false;
				return true;
			}
			if (!candidate.token.pid && !slot)
				slot = &candidate;
		}
		if (!slot || !literal_inventory_capacity_locked(bytes))
			return false;
		uint64_t generation = 0;
		if (!player_save_execution_guard::install_hold(
			    terms.actor_pid, envelope.command.operation_id, &generation))
			return false;
		slot->profile = literal_checkpoint_profile::held_retirement;
		slot->token = { static_cast<int32_t>(terms.actor_pid), 0, 0, 0 };
		slot->held_selected_uid = terms.item_uid;
		slot->payload = std::move(frozen);
		slot->held_before = std::move(before);
		slot->held_after = std::move(after);
		slot->held_attachment = std::move(attachment);
		slot->held_recovery_revision = envelope.revision;
		slot->captured_revision = slot->acknowledged_revision = original.save_revision;
		slot->operation_id = envelope.command.operation_id;
		slot->execution_hold_generation = generation;
		slot->held = slot->restored_sql_drop = slot->held_passive_restored = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_held_retirement_publication_owner::rebind_recovery_checkpoint(
	const critical_native_recovery_envelope &envelope, uint64_t runtime_id) noexcept
{
#ifdef __NO_MYSQL__
	(void)envelope;
	(void)runtime_id;
	return false;
#else
	if (!runtime_id || !nevent_is_game_thread())
		return false;
	try
	{
		lockpick_retirement_terms terms;
		if (!held_retirement_command_identity(envelope.command, nullptr, &terms))
			return false;
		P_char actor = find_character_by_runtime_id(runtime_id);
		if (!actor || !IS_PC(actor) || !actor->only.pc ||
		    GET_PID(actor) != static_cast<int>(terms.actor_pid) ||
		    actor->runtime_id != runtime_id ||
		    IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED) ||
		    player_save_worker_pid_pending(terms.actor_pid) ||
		    player_save_journal_pid_quarantined(terms.actor_pid))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(terms.actor_pid);
		if (!slot || !held_retirement_slot_command(*slot, envelope.command) ||
		    slot->held_recovery_revision != envelope.revision ||
		    slot->held_attachment != envelope.attachment ||
		    find_terminal_fence_locked(terms.actor_pid) ||
		    find_target_save_login_fence_locked(terms.actor_pid) ||
		    append_inflight_pid == static_cast<int>(terms.actor_pid) ||
		    any_snapshot_is_retained_locked(terms.actor_pid))
			return false;
		if (slot->token.actor_runtime_id)
		{
			if (slot->token.actor_runtime_id == runtime_id)
				return true;
			if (find_character_by_runtime_id(slot->token.actor_runtime_id))
				return false;
		}
		if (literal_inventory_generation == UINT64_MAX)
			return false;
		slot->token.actor_runtime_id = runtime_id;
		slot->token.generation = ++literal_inventory_generation;
		slot->held_runtime_rebound = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_held_retirement_publication_owner::copy_recovery_context(
	const critical_command &command, critical_native_recovery_envelope *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)output;
	return false;
#else
	if (!output || !nevent_is_game_thread())
		return false;
	try
	{
		critical_native_recovery_envelope envelope;
		lockpick_retirement_terms terms;
		if (!critical_held_retirement_publication_owner::copy_context(command, &envelope) ||
		    !held_retirement_command_identity(command, nullptr, &terms))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto *slot = find_literal_inventory_locked(terms.actor_pid);
		if (!slot || !held_retirement_slot_command(*slot, command) ||
		    slot->held_attachment != envelope.attachment ||
		    slot->held_recovery_revision != envelope.revision)
			return false;
		*output = std::move(envelope);
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_held_retirement_publication_owner::resume_recovery_context(
	const critical_command &command) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	return false;
#else
	if (!nevent_is_game_thread())
		return false;
	try
	{
		lockpick_retirement_terms terms;
		critical_native_recovery_envelope expected, successor;
		if (!held_retirement_command_identity(command, nullptr, &terms))
			return false;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			const auto *slot = find_literal_inventory_locked(terms.actor_pid);
			if (!slot || !held_retirement_slot_command(*slot, command))
				return false;
			if (slot->held_attachment_successor.empty())
				return !slot->held_successor_revision;
			if (slot->held_recovery_revision == UINT64_MAX ||
			    slot->held_successor_revision != slot->held_recovery_revision + 1)
				return false;
			expected.command = command;
			expected.revision = slot->held_recovery_revision;
			expected.phase = critical_native_recovery_phase::execution_pending;
			expected.attachment = slot->held_attachment;
			successor = expected;
			successor.revision = slot->held_successor_revision;
			successor.attachment = slot->held_attachment_successor;
		}
		// Same exact CAS can prove rename/fsync uncertainty. No absence inference.
		return checkpoint_recovery_context(expected, successor);
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_held_retirement_publication_owner::copy_publication_context(
	const critical_command &command, const critical_completion &completion,
	held_retirement_publication_snapshot *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)completion;
	(void)output;
	return false;
#else
	if (!output || !nevent_is_game_thread() || !resume_recovery_context(command))
		return false;
	try
	{
		lockpick_retirement_terms terms;
		held_retirement_publication_snapshot snapshot;
		if (!held_retirement_command_identity(command, nullptr, &terms) ||
		    !critical_held_retirement_publication_owner::copy_publication(
			    command, completion, &snapshot) ||
		    !held_retirement_publication_snapshot_valid(command, completion, snapshot))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto *slot = find_literal_inventory_locked(terms.actor_pid);
		if (!slot || !held_retirement_slot_command(*slot, command) ||
		    slot->held_attachment != snapshot.envelope.attachment ||
		    slot->held_recovery_revision != snapshot.envelope.revision ||
		    !slot->held_attachment_successor.empty() || slot->held_successor_revision)
			return false;
		*output = std::move(snapshot);
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_held_retirement_publication_owner::checkpoint_recovery_context(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor) noexcept
{
#ifdef __NO_MYSQL__
	(void)expected;
	(void)successor;
	return false;
#else
	if (!nevent_is_game_thread() ||
	    !held_retirement_recovery_transition_valid(expected, successor))
		return false;
	try
	{
		lockpick_retirement_terms terms;
		if (!held_retirement_command_identity(expected.command, nullptr, &terms))
			return false;
		auto retained = successor.attachment; // Allocate before journal/leaf state changes.
		player_literal_inventory_token identity{};
		uint64_t generation = 0;
		literal_inventory_checkpoint *pinned = nullptr;
		size_t reserved = 0;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			auto *slot = find_literal_inventory_locked(terms.actor_pid);
			if (!slot || !held_retirement_slot_command(*slot, expected.command) ||
			    slot->held_recovery_revision != expected.revision ||
			    slot->held_attachment != expected.attachment)
				return false;
			pinned = slot;
			identity = slot->token;
			generation = slot->execution_hold_generation;
			if (!slot->held_attachment_successor.empty() &&
			    (slot->held_successor_revision != successor.revision ||
			     slot->held_attachment_successor != retained))
				return false;
			if (retained.size() >
			    PLAYER_SAVE_PIPELINE_MAX_BYTES - slot->held_attachment.size())
				return false;
			// Existing high-water includes both exact expected and successor.
			reserved = slot->held_attachment.size() + retained.size();
			size_t bytes = 0;
			for (size_t size : { slot->payload.size(), slot->held_before.size(),
					     slot->held_after.size(), reserved })
			{
				if (size > PLAYER_SAVE_PIPELINE_MAX_BYTES - bytes)
					return false;
				bytes += size;
			}
			if (!literal_inventory_capacity_locked(bytes, slot))
				return false;
			slot->held_attachment_successor = std::move(retained);
			slot->held_successor_revision = successor.revision;
			slot->held_attachment_reserved_bytes = reserved;
		}
		const bool confirmed =
			critical_held_retirement_publication_owner::checkpoint_context(expected,
										       successor);
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(terms.actor_pid);
		if (!slot || slot != pinned || slot->token != identity ||
		    slot->execution_hold_generation != generation ||
		    slot->held_recovery_revision != expected.revision ||
		    slot->held_attachment != expected.attachment ||
		    slot->held_attachment_reserved_bytes != reserved ||
		    slot->held_successor_revision != successor.revision ||
		    slot->held_attachment_successor != successor.attachment || !slot->held ||
		    !slot->restored_sql_drop ||
		    slot->operation_id.bytes != expected.command.operation_id.bytes ||
		    !player_save_execution_guard::publication_operation_held(
			    expected.command.operation_id))
		{
			player_save_execution_guard::poison_integrity();
			return false;
		}
		if (confirmed)
		{
			slot->held_attachment = std::move(slot->held_attachment_successor);
			slot->held_attachment_successor.clear();
			slot->held_successor_revision = 0;
			slot->held_recovery_revision = successor.revision;
			slot->held_attachment_reserved_bytes = slot->held_attachment.size();
		}
		return confirmed;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_held_retirement_publication_owner::publication_held_bodies(
	const critical_command &command, std::vector<player_item_snapshot> *before,
	std::vector<player_item_snapshot> *after,
	player_held_retirement_checkpoint_stage *stage) noexcept
{
	if (!player_save_held_retirement_checkpoint_owner::original_held_bodies(command, before,
										after, stage))
		return false;
#ifdef __NO_MYSQL__
	return false;
#else
	try
	{
		lockpick_retirement_terms terms;
		if (!held_retirement_command_identity(command, nullptr, &terms))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto *slot = find_literal_inventory_locked(terms.actor_pid);
		if (!slot || !held_retirement_slot_command(*slot, command) ||
		    !slot->token.actor_runtime_id || !slot->token.generation)
			return false;
		P_char actor = find_character_by_runtime_id(slot->token.actor_runtime_id);
		return actor && IS_PC(actor) && actor->only.pc &&
		       GET_PID(actor) == static_cast<int>(terms.actor_pid) &&
		       !IS_SET(actor->runtime_flags, CHAR_RFLAG_LOAD_DEGRADED);
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_held_retirement_publication_owner::publish_held_retirement(
	const critical_command &command, const critical_completion &completion,
	const held_retirement_publication_snapshot &snapshot,
	bool (*native_publish)(const critical_command &, const critical_completion &,
			       void *) noexcept,
	void *context) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)completion;
	(void)snapshot;
	(void)native_publish;
	(void)context;
	return false;
#else
	if (!nevent_is_game_thread() || !native_publish ||
	    !critical_completion_disposition_valid(completion) ||
	    !held_retirement_publication_snapshot_valid(command, completion, snapshot) ||
	    command.operation_id.bytes != completion.operation_id.bytes)
		return false;
	try
	{
		lockpick_retirement_terms terms;
		std::vector<uint8_t> frozen;
		if (!held_retirement_command_identity(command, nullptr, &terms) ||
		    critical_command_encode(command, &frozen) != critical_command_codec_result::ok)
			return false;
		uint64_t generation = 0;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			const auto *slot = find_literal_inventory_locked(terms.actor_pid);
			if (!health.initialized || stop_requested || !slot ||
			    !held_retirement_slot_command(*slot, command) ||
			    slot->held_attachment != snapshot.envelope.attachment ||
			    slot->held_recovery_revision != snapshot.envelope.revision ||
			    !slot->held_attachment_successor.empty() ||
			    slot->held_successor_revision || !slot->token.actor_runtime_id ||
			    !slot->token.generation ||
			    find_terminal_fence_locked(terms.actor_pid) ||
			    find_target_save_login_fence_locked(terms.actor_pid) ||
			    append_inflight_pid == static_cast<int>(terms.actor_pid) ||
			    any_snapshot_is_retained_locked(terms.actor_pid))
				return false;
			generation = slot->execution_hold_generation;
		}
		critical_command owned = command;
		player_save_restored_publication_owner owner(
			std::move(owned), std::move(frozen), completion,
			player_save_execution_guard::current_ownership_epoch(), terms.actor_pid,
			generation);
		if (!owner.reservation_.valid() || player_save_worker_pid_pending(terms.actor_pid))
			return false;
		if (completion.disposition == critical_completion_disposition::never_admitted)
			return critical_command_coordinator_cancel_held_retirement_publication(
				owner, snapshot.envelope, native_publish, context);
		if (completion.failure_stage != critical_failure_stage::none)
			return false;
		player_revision_snapshot revision{};
		if (player_revision_snapshot_copy(terms.actor_pid, &revision) &&
		    (revision.overflowed || revision.queued_components ||
		     revision.inflight_components))
			return false;
		std::vector<player_save_journal_retained_frame> originals;
		if (player_save_journal_collect_publication_frames(
			    terms.actor_pid, owner.reservation_, &originals) !=
			    player_save_journal_result::ok ||
		    (!originals.empty() &&
		     !player_save_restored_publication_owner::retire_covered_ordinary(owner,
										      originals)) ||
		    player_save_journal_publication_census(terms.actor_pid, owner.reservation_) !=
			    player_save_journal_result::ok ||
		    !native_publish(owner.command_, completion, context) ||
		    !owner.reservation_.valid() ||
		    player_save_journal_publication_census(terms.actor_pid, owner.reservation_) !=
			    player_save_journal_result::ok)
			return false;
		owner.publication_proven_ = true;
		return critical_command_coordinator_acknowledge_publication(owner);
	}
	catch (...)
	{
		return false;
	}
#endif
}

critical_submit_result player_save_shop_checkpoint_owner::submit_owned_flat(
	const player_flat_shop_checkpoint_token &token,
	const player_flat_shop_checkpoint_cut &original_cut, critical_command command) noexcept
{
#ifndef __NO_MYSQL__
	(void)token;
	(void)original_cut;
	(void)command;
	return critical_submit_result::unavailable;
#else
	if (!nevent_is_game_thread())
		return critical_submit_result::unavailable;
	bool retained = false;
	try
	{
		economic_frozen_intent intent;
		shop_trade_payload payload{};
		economic_account_key wallet, bank, counterparty;
		std::vector<uint8_t> frozen;
		if (!command.publication_required ||
		    command.payload_version != SHOP_TRADE_RECOVERY_PAYLOAD_VERSION ||
		    shop_trade_accounting_decode(command, &intent, &payload, &wallet, &bank,
						 &counterparty) != economic_accounting_error::ok ||
		    !token.actor_runtime_id || !token.generation || token.pid <= 0 ||
		    payload.player_pid != static_cast<uint32_t>(token.pid) ||
		    !payload.expected_player_save_revision || !payload.expected_player_level ||
		    critical_command_encode(command, &frozen) !=
			    critical_command_codec_result::ok ||
		    frozen.empty() || frozen.capacity() > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
		    player_save_worker_pid_pending(token.pid) ||
		    player_save_journal_pid_quarantined(token.pid))
			return critical_submit_result::invalid;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			auto *slot = find_literal_inventory_locked(token.pid);
			if (!slot || !flat_shop_pipeline_ready_locked(token.pid) ||
			    !flat_shop_held_slot_matches(*slot, token, command.operation_id) ||
			    !slot->flat_shop_native_attempt_started ||
			    !slot->flat_shop_native_reserved_bytes ||
			    !slot->flat_shop_command_reserved_bytes ||
			    slot->flat_shop_root != original_cut.selected_root ||
			    slot->flat_shop_ownership_epoch != original_cut.ownership_epoch ||
			    slot->execution_hold_generation !=
				    original_cut.execution_hold_generation ||
			    slot->captured_revision != payload.expected_player_save_revision ||
			    slot->acknowledged_revision != payload.expected_player_save_revision ||
			    slot->level != payload.expected_player_level ||
			    !flat_shop_drained_locked(*slot) || slot->payload.empty() ||
			    !economic_account_key_equal(wallet, slot->flat_shop_mapping.wallet) ||
			    !economic_account_key_equal(bank, slot->flat_shop_mapping.bank))
				return critical_submit_result::identity_conflict;
			player_save_execution_guard::held_publication_reservation reservation(
				original_cut.ownership_epoch, token.pid, command.operation_id,
				original_cut.execution_hold_generation);
			if (!reservation.matches_pid(token.pid))
				return critical_submit_result::unavailable;
			if (!slot->flat_shop_journal_command.empty())
			{
				if (slot->flat_shop_journal_command != frozen ||
				    !reservation.valid())
					return critical_submit_result::identity_conflict;
			}
			else
			{
				// Charge the actual additional retained capacity; native/command
				// reservations and original player bytes are never reclaimed.
				if (!literal_inventory_capacity_locked(frozen.capacity()) ||
				    !reservation.valid())
					return critical_submit_result::unavailable;
				slot->flat_shop_journal_command = std::move(frozen);
			}
			retained = true;
		}
		// Coordinator exclusion is acquired only after pipeline/leaf locks end.
		// The real allowlist remains closed until full publication is qualified.
		return critical_command_coordinator_submit_for_publication(std::move(command));
	}
	catch (...)
	{
		// The same original hold and frozen bytes survive every uncertain tail.
		return retained ? critical_submit_result::journal_uncertain :
				  critical_submit_result::invalid;
	}
#endif
}

bool player_save_shop_checkpoint_owner::reserve_flat_payload_checkpoint(
	const player_flat_shop_checkpoint_token &token, const critical_operation_id &operation,
	const player_flat_shop_checkpoint_cut &original_cut, size_t bytes) noexcept
{
	try
	{
		if (!nevent_is_game_thread() || !bytes || bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
		    critical_operation_id_is_zero(operation))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(token.pid);
		if (!slot || !flat_shop_held_slot_matches(*slot, token, operation) ||
		    !slot->flat_shop_native_attempt_started ||
		    !slot->flat_shop_native_reserved_bytes ||
		    !slot->flat_shop_command_reserved_bytes ||
		    !flat_shop_pipeline_ready_locked(token.pid) ||
		    slot->flat_shop_root != original_cut.selected_root ||
		    slot->flat_shop_ownership_epoch != original_cut.ownership_epoch ||
		    slot->execution_hold_generation != original_cut.execution_hold_generation ||
		    !flat_shop_context_matches(
			    *slot, find_character_by_runtime_id(token.actor_runtime_id)))
			return false;
		player_save_execution_guard::held_publication_reservation reservation(
			original_cut.ownership_epoch, token.pid, operation,
			original_cut.execution_hold_generation);
		if (!reservation.matches_pid(token.pid))
			return false;
		if (slot->flat_shop_payload_reserved_bytes)
			return slot->flat_shop_payload_reserved_bytes == bytes &&
			       reservation.valid();
		if (!literal_inventory_capacity_locked(bytes) || !reservation.valid())
			return false;
		// No native reservation resize or attempted-leaf reset is permitted.
		slot->flat_shop_payload_reserved_bytes = bytes;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool player_save_shop_checkpoint_owner::original_held_body_flat(
	const player_flat_shop_checkpoint_token &token, const player_flat_shop_checkpoint_cut &cut,
	const player_save_restored_publication_owner &publication, player_snapshot *body_out,
	player_shop_checkpoint_stage *stage_out) noexcept
{
#ifndef __NO_MYSQL__
	(void)token;
	(void)cut;
	(void)publication;
	(void)body_out;
	(void)stage_out;
	return false;
#else
	if (!publication.flat_shop_ || publication.acknowledged_ || publication.pid_ != token.pid ||
	    publication.generation_ != cut.execution_hold_generation ||
	    !publication.reservation_.matches_pid(token.pid) || !nevent_is_game_thread() ||
	    !body_out || !stage_out || token.pid <= 0 || !token.actor_runtime_id ||
	    !token.generation || !economic_gameplay_authority::active_regular_flat() ||
	    player_save_worker_pid_pending(token.pid) ||
	    player_save_journal_pid_quarantined(token.pid))
		return false;
	try
	{
		economic_frozen_intent intent;
		shop_trade_payload payload{};
		economic_account_key wallet, bank, counterparty;
		std::vector<uint8_t> frozen;
		const auto &command = publication.command_;
		if (!command.publication_required ||
		    command.payload_version != SHOP_TRADE_RECOVERY_PAYLOAD_VERSION ||
		    shop_trade_accounting_decode(command, &intent, &payload, &wallet, &bank,
						 &counterparty) != economic_accounting_error::ok ||
		    payload.player_pid != static_cast<uint32_t>(token.pid) ||
		    !payload.expected_player_save_revision || !payload.expected_player_level ||
		    critical_command_encode(command, &frozen) != critical_command_codec_result::ok)
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto *slot = find_literal_inventory_locked(token.pid);
		const char *selected = persistence_mode_flatfile_root();
		P_char actor = find_character_by_runtime_id(token.actor_runtime_id);
		const char *account = actor ? get_account_name_safe(actor) : nullptr;
		// Do not call observe_held_flat/flat_shop_context_matches: they reread
		// BEFORE inventory and acquire a root lock already borrowed by native
		// publication. CURRENT proof belongs to the genuine backend owner.
		if (!slot || !flat_shop_pipeline_ready_locked(token.pid) ||
		    slot->profile != literal_checkpoint_profile::flat_shop || !slot->held ||
		    slot->restored_sql_drop ||
		    slot->token != flat_shop_checkpoint_identity(token) ||
		    slot->operation_id.bytes != command.operation_id.bytes ||
		    !slot->flat_shop_native_attempt_started ||
		    !slot->flat_shop_native_reserved_bytes ||
		    !slot->flat_shop_command_reserved_bytes ||
		    !slot->flat_shop_payload_reserved_bytes ||
		    slot->flat_shop_journal_command.empty() ||
		    slot->flat_shop_journal_command != frozen ||
		    slot->flat_shop_root != cut.selected_root || !selected ||
		    slot->flat_shop_root != selected ||
		    slot->flat_shop_ownership_epoch != cut.ownership_epoch ||
		    cut.ownership_epoch != player_save_execution_guard::current_ownership_epoch() ||
		    !cut.execution_hold_generation ||
		    slot->execution_hold_generation != cut.execution_hold_generation ||
		    slot->captured_revision != payload.expected_player_save_revision ||
		    slot->acknowledged_revision != slot->captured_revision ||
		    slot->level != payload.expected_player_level ||
		    slot->original_shop_body.empty() || slot->payload.empty() ||
		    !economic_account_key_equal(wallet, slot->flat_shop_mapping.wallet) ||
		    !economic_account_key_equal(bank, slot->flat_shop_mapping.bank) ||
		    !shop_actor_matches({ token.pid, token.actor_runtime_id, token.root_uid,
					  token.generation },
					actor) ||
		    !account || slot->flat_shop_account != account ||
		    GET_RACEWAR(actor) != slot->flat_shop_racewar ||
		    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
			return false;
		// Borrow the actual outer owner; constructing another ticket here
		// would conflict with the publication reservation already holding PID.
		const auto &reservation = publication.reservation_;
		if (publication.frozen_ != frozen || !reservation.matches_pid(token.pid))
			return false;
		player_snapshot original;
		std::vector<uint8_t> original_blob;
		uint32_t original_level = 0;
		if (player_snapshot_decode(slot->original_shop_body.data(),
					   slot->original_shop_body.size(),
					   &original) != player_snapshot_codec_result::ok ||
		    original.pid != token.pid || original.revision != slot->captured_revision ||
		    !shop_checkpoint_blob(original, &original_blob, &original_level) ||
		    original_blob != slot->payload || original_level != slot->level ||
		    !reservation.valid())
			return false;
		static_assert(std::is_nothrow_move_assignable_v<player_snapshot>);
		*body_out = std::move(original);
		*stage_out = { slot->acknowledged_revision, original_level };
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_restored_publication_owner::restore_shop_flat_obligation(
	const critical_command &command, size_t native_owner_retained_bytes) noexcept
{
#ifndef __NO_MYSQL__
	(void)command;
	(void)native_owner_retained_bytes;
	return false;
#else
	try
	{
		economic_frozen_intent intent;
		shop_trade_payload payload{};
		economic_account_key wallet, bank, counterparty;
		std::vector<uint8_t> frozen;
		if (!nevent_is_game_thread() ||
		    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY ||
		    !command.publication_required ||
		    command.payload_version != SHOP_TRADE_RECOVERY_PAYLOAD_VERSION ||
		    !native_owner_retained_bytes ||
		    native_owner_retained_bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
		    shop_trade_accounting_decode(command, &intent, &payload, &wallet, &bank,
						 &counterparty) != economic_accounting_error::ok ||
		    !payload.recovery_manifest_recorded || !payload.player_pid ||
		    payload.player_pid > INT_MAX || !payload.selected_item_uid ||
		    critical_command_encode(command, &frozen) != critical_command_codec_result::ok)
			return false;
		const char *selected = persistence_mode_flatfile_root();
		if (!selected || !*selected)
			return false;
		std::string root(selected);
		const int pid = static_cast<int>(payload.player_pid);
		// All actual allocations precede hold installation. Existing slot
		// accounting charges payload/root sizes; this separate delta charges
		// their remaining capacities plus the native owner's complete census.
		size_t extra = native_owner_retained_bytes;
		for (size_t bytes :
		     { frozen.capacity() - frozen.size(), root.capacity() - root.size() })
		{
			if (bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES - extra)
				return false;
			extra += bytes;
		}
		size_t total = extra;
		for (size_t bytes : { frozen.size(), root.size() })
		{
			if (bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES - total)
				return false;
			total += bytes;
		}
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		const auto epoch = player_save_execution_guard::current_ownership_epoch();
		if (!health.initialized || stop_requested || execution_started || !epoch ||
		    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY ||
		    !persistence_mode_flatfile_root() || root != persistence_mode_flatfile_root())
			return false;
		literal_inventory_checkpoint *slot = nullptr;
		for (auto &candidate : literal_inventory_checkpoints)
		{
			if (candidate.token.pid == pid ||
			    (candidate.held &&
			     candidate.operation_id.bytes == command.operation_id.bytes))
			{
				uint64_t generation = 0;
				if (candidate.profile !=
					    literal_checkpoint_profile::flat_shop_restored ||
				    !candidate.held || candidate.restored_sql_drop ||
				    candidate.token.pid != pid ||
				    candidate.token.actor_runtime_id ||
				    candidate.token.generation ||
				    candidate.token.root_uid != payload.selected_item_uid ||
				    candidate.operation_id.bytes != command.operation_id.bytes ||
				    candidate.payload != frozen ||
				    candidate.flat_shop_root != root ||
				    candidate.flat_shop_ownership_epoch != epoch ||
				    candidate.flat_shop_restored_native_owner_bytes !=
					    native_owner_retained_bytes ||
				    !candidate.flat_shop_restored_reserved_bytes ||
				    candidate.flat_shop_native_attempt_started ||
				    candidate.flat_shop_native_reserved_bytes ||
				    candidate.flat_shop_command_reserved_bytes ||
				    candidate.flat_shop_payload_reserved_bytes ||
				    !candidate.flat_shop_journal_command.empty() ||
				    !candidate.original_shop_body.empty() ||
				    !player_save_execution_guard::install_hold(
					    pid, command.operation_id, &generation))
					return false;
				if (generation != candidate.execution_hold_generation)
				{
					player_save_execution_guard::poison_integrity();
					return false;
				}
				return true;
			}
			if (!candidate.token.pid && !slot)
				slot = &candidate;
		}
		if (!slot || !literal_inventory_capacity_locked(total))
			return false;
		uint64_t generation = 0;
		if (!player_save_execution_guard::install_hold(pid, command.operation_id,
							       &generation))
			return false;
		// No allocation or native effect follows installation. Retry identity
		// is passive and exact; the later cold publisher must supply all proof.
		slot->profile = literal_checkpoint_profile::flat_shop_restored;
		slot->token.pid = pid;
		slot->token.root_uid = payload.selected_item_uid;
		slot->payload = std::move(frozen);
		slot->flat_shop_root = std::move(root);
		slot->flat_shop_restored_reserved_bytes = extra;
		slot->flat_shop_restored_native_owner_bytes = native_owner_retained_bytes;
		slot->flat_shop_ownership_epoch = epoch;
		slot->execution_hold_generation = generation;
		slot->operation_id = command.operation_id;
		slot->held = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_restored_publication_owner::publish_shop_flat(
	const critical_command &original, const critical_completion &completion,
	bool (*native_publish)(player_save_restored_publication_owner &, void *) noexcept,
	void *context) noexcept
{
#ifndef __NO_MYSQL__
	(void)original;
	(void)completion;
	(void)native_publish;
	(void)context;
	return false;
#else
	if (!nevent_is_game_thread() || !native_publish ||
	    !economic_gameplay_authority::active_regular_flat() ||
	    !critical_completion_disposition_valid(completion) ||
	    original.operation_id.bytes != completion.operation_id.bytes)
		return false;
	try
	{
		economic_frozen_intent intent;
		shop_trade_payload payload{};
		economic_account_key wallet, bank, counterparty;
		critical_command command;
		std::vector<uint8_t> frozen;
		if (!original.publication_required ||
		    original.payload_version != SHOP_TRADE_RECOVERY_PAYLOAD_VERSION ||
		    shop_trade_accounting_decode(original, &intent, &payload, &wallet, &bank,
						 &counterparty) != economic_accounting_error::ok ||
		    !payload.player_pid || payload.player_pid > INT_MAX ||
		    !payload.expected_player_save_revision || !payload.expected_player_level ||
		    critical_command_encode(original, &frozen) != critical_command_codec_result::ok)
			return false;
		const int pid = static_cast<int>(payload.player_pid);
		if (player_save_worker_pid_pending(pid) || player_save_journal_pid_quarantined(pid))
			return false;
		uint64_t generation = 0, epoch = 0;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			const auto *slot = find_literal_inventory_locked(pid);
			const char *selected = persistence_mode_flatfile_root();
			if (!slot || !flat_shop_pipeline_ready_locked(pid) ||
			    slot->profile != literal_checkpoint_profile::flat_shop || !slot->held ||
			    slot->restored_sql_drop || !slot->token.actor_runtime_id ||
			    !slot->token.generation || !slot->flat_shop_native_attempt_started ||
			    !slot->flat_shop_native_reserved_bytes ||
			    !slot->flat_shop_command_reserved_bytes ||
			    !slot->flat_shop_payload_reserved_bytes ||
			    slot->flat_shop_journal_command.empty() ||
			    slot->flat_shop_journal_command != frozen ||
			    slot->operation_id.bytes != completion.operation_id.bytes ||
			    !slot->execution_hold_generation || !slot->flat_shop_ownership_epoch ||
			    slot->flat_shop_ownership_epoch !=
				    player_save_execution_guard::current_ownership_epoch() ||
			    !selected || slot->flat_shop_root != selected ||
			    slot->captured_revision != payload.expected_player_save_revision ||
			    slot->acknowledged_revision != slot->captured_revision ||
			    slot->level != payload.expected_player_level ||
			    slot->original_shop_body.empty() || slot->payload.empty() ||
			    !economic_account_key_equal(wallet, slot->flat_shop_mapping.wallet) ||
			    !economic_account_key_equal(bank, slot->flat_shop_mapping.bank) ||
			    append_inflight_pid == pid || any_snapshot_is_retained_locked(pid) ||
			    critical_command_decode(slot->flat_shop_journal_command.data(),
						    slot->flat_shop_journal_command.size(),
						    &command) != critical_command_codec_result::ok)
				return false;
			generation = slot->execution_hold_generation;
			epoch = slot->flat_shop_ownership_epoch;
		}
		player_save_restored_publication_owner owner(std::move(command), std::move(frozen),
							     completion, epoch, pid, generation);
		owner.flat_shop_ = true;
		if (!owner.reservation_.valid() || player_save_worker_pid_pending(pid))
			return false;
		// Actual retained coordinator refusal must precede native cleanup.
		// Receipt absence alone never grants cancellation or hold consumption.
		if (completion.disposition == critical_completion_disposition::never_admitted)
		{
			struct original_cleanup
			{
				player_save_restored_publication_owner &owner;
				bool (*publish)(player_save_restored_publication_owner &,
						void *) noexcept;
				void *context;
			} cleanup{ owner, native_publish, context };
			const auto borrowed_cleanup = [](const critical_command &refused_command,
							 const critical_completion &sealed,
							 void *opaque) noexcept
			{
				if (!opaque)
					return false;
				auto &actual = *static_cast<original_cleanup *>(opaque);
				// Coordinator already proved the complete original refusal.
				// This synchronous private adapter borrows that exact owner.
				return critical_command_equal(actual.owner.command_,
							      refused_command) &&
				       actual.owner.completion_.operation_id.bytes ==
					       sealed.operation_id.bytes &&
				       actual.publish(actual.owner, actual.context);
			};
			return critical_command_coordinator_cancel_shop_publication(
				owner, borrowed_cleanup, &cleanup);
		}
		if (completion.failure_stage != critical_failure_stage::none)
			return false;
		player_revision_snapshot revision{};
		if (player_revision_snapshot_copy(pid, &revision) &&
		    (revision.overflowed || revision.queued_components ||
		     revision.inflight_components))
			return false;
		std::vector<player_save_journal_retained_frame> originals;
		if (player_save_journal_collect_publication_frames(
			    pid, owner.reservation_, &originals) != player_save_journal_result::ok)
			return false;
		if (!originals.empty())
		{
			// The genuine repository observer already supports flat snapshots;
			// the older SQL-only owner helper must not substitute its refusal.
			player_save_covered_revision covered;
			if (!player_snapshot_repository_observe_covered_revision(
				    pid, owner.reservation_, &covered) ||
			    player_save_journal_retire_covered_ordinary(pid, owner.reservation_,
									covered, originals) !=
				    player_save_journal_result::ok)
				return false;
		}
		if (player_save_journal_publication_census(pid, owner.reservation_) !=
			    player_save_journal_result::ok ||
		    !native_publish(owner, context) || !owner.reservation_.valid() ||
		    player_save_journal_publication_census(pid, owner.reservation_) !=
			    player_save_journal_result::ok)
			return false;
		owner.publication_proven_ = true;
		return critical_command_coordinator_acknowledge_publication(owner);
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_restored_publication_owner::consume_acknowledged_flat_shop_hold() noexcept
{
#ifndef __NO_MYSQL__
	return false;
#else
	// Durable ACK already passed global validity; exact local cleanup uses
	// the original private reservation matcher even if another PID is poisoned.
	if (!flat_shop_ || !acknowledged_)
		return false;
	player_save_deferred_identity wake{};
	bool active = false;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(pid_);
		if (!slot || slot->profile != literal_checkpoint_profile::flat_shop ||
		    !slot->held || slot->restored_sql_drop ||
		    !slot->flat_shop_native_attempt_started ||
		    !slot->flat_shop_native_reserved_bytes ||
		    !slot->flat_shop_command_reserved_bytes ||
		    !slot->flat_shop_payload_reserved_bytes ||
		    slot->execution_hold_generation != generation_ ||
		    slot->flat_shop_ownership_epoch !=
			    player_save_execution_guard::current_ownership_epoch() ||
		    slot->operation_id.bytes != completion_.operation_id.bytes ||
		    slot->flat_shop_journal_command.empty() ||
		    slot->flat_shop_journal_command != frozen_)
		{
			player_save_execution_guard::poison_integrity();
			return false;
		}
		active = player_save_worker_deferred_identity(pid_, &wake);
		if (!reservation_.consume_acknowledged_hold())
		{
			player_save_execution_guard::poison_integrity();
			return false;
		}
		*slot = {};
	}
	if (active)
		(void)player_save_worker_resume_deferred_exact(wake);
	replay_revisit_requested.store(true);
	player_save_execution_guard::signal_ownership_change(
		player_save_execution_guard::current_ownership_epoch());
	return true;
#endif
}

bool player_save_shop_checkpoint_owner::reserve_flat_publication_checkpoint(
	const player_flat_shop_checkpoint_token &token, const critical_operation_id &operation,
	const player_flat_shop_checkpoint_cut &cut,
	const player_save_restored_publication_owner &publication, size_t bytes) noexcept
{
#ifndef __NO_MYSQL__
	(void)token;
	(void)operation;
	(void)cut;
	(void)publication;
	(void)bytes;
	return false;
#else
	try
	{
		if (!nevent_is_game_thread() || !bytes || bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
		    !publication.flat_shop_ || publication.acknowledged_ || token.pid <= 0 ||
		    !token.actor_runtime_id || !token.generation || publication.pid_ != token.pid ||
		    publication.generation_ != cut.execution_hold_generation ||
		    critical_operation_id_is_zero(operation) ||
		    publication.command_.operation_id.bytes != operation.bytes ||
		    publication.completion_.operation_id.bytes != operation.bytes ||
		    !publication.reservation_.matches_pid(token.pid) ||
		    !economic_gameplay_authority::active_regular_flat() ||
		    player_save_worker_pid_pending(token.pid) ||
		    player_save_journal_pid_quarantined(token.pid))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(token.pid);
		const char *selected = persistence_mode_flatfile_root();
		if (!slot || !flat_shop_pipeline_ready_locked(token.pid) ||
		    slot->profile != literal_checkpoint_profile::flat_shop || !slot->held ||
		    slot->restored_sql_drop ||
		    slot->token != flat_shop_checkpoint_identity(token) ||
		    slot->operation_id.bytes != operation.bytes ||
		    !slot->flat_shop_native_attempt_started ||
		    !slot->flat_shop_native_reserved_bytes ||
		    !slot->flat_shop_command_reserved_bytes ||
		    !slot->flat_shop_payload_reserved_bytes ||
		    slot->flat_shop_journal_command.empty() ||
		    slot->flat_shop_journal_command != publication.frozen_ ||
		    slot->original_shop_body.empty() || slot->payload.empty() ||
		    slot->flat_shop_root != cut.selected_root || !selected ||
		    slot->flat_shop_root != selected || !cut.ownership_epoch ||
		    slot->flat_shop_ownership_epoch != cut.ownership_epoch ||
		    cut.ownership_epoch != player_save_execution_guard::current_ownership_epoch() ||
		    !cut.execution_hold_generation ||
		    slot->execution_hold_generation != cut.execution_hold_generation ||
		    append_inflight_pid == token.pid || any_snapshot_is_retained_locked(token.pid))
			return false;
		// Borrow the existing outer ticket. Native publication already owns its
		// root lock; no context recapture, nested ticket or root reacquisition.
		if (slot->flat_shop_publication_reserved_bytes)
			return slot->flat_shop_publication_reserved_bytes == bytes &&
			       publication.reservation_.valid();
		if (!literal_inventory_capacity_locked(bytes) || !publication.reservation_.valid())
			return false;
		// The native owner has counted every actual retained allocation. Only
		// one charge precedes its first nonthrowing moves; retries cannot resize.
		slot->flat_shop_publication_reserved_bytes = bytes;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

namespace
{
#ifdef __NO_MYSQL__
// Pure exact stored-slot identity only. Callers already own pipeline_mutex and
// derive every expectation from the private canonical command/outer ticket.
// This never constructs a hold, reads source authority, or reacquires root.
bool flat_shop_restored_slot_matches(const literal_inventory_checkpoint &slot, int pid,
				     uint64_t selected_uid, const critical_operation_id &operation,
				     const std::vector<uint8_t> &frozen, uint64_t epoch,
				     uint64_t generation) noexcept
{
	const char *selected = persistence_mode_flatfile_root();
	return pid > 0 && selected_uid && epoch && generation &&
	       persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY &&
	       selected_snapshot_apply() == flatfile_player_snapshot_apply_selected && selected &&
	       *selected && slot.profile == literal_checkpoint_profile::flat_shop_restored &&
	       slot.held && !slot.restored_sql_drop && slot.token.pid == pid &&
	       !slot.token.actor_runtime_id && !slot.token.generation &&
	       slot.token.root_uid == selected_uid && slot.operation_id.bytes == operation.bytes &&
	       !critical_operation_id_is_zero(operation) && !frozen.empty() &&
	       slot.payload == frozen && !slot.flat_shop_root.empty() &&
	       slot.flat_shop_root == selected && slot.flat_shop_ownership_epoch == epoch &&
	       epoch == player_save_execution_guard::current_ownership_epoch() &&
	       slot.execution_hold_generation == generation &&
	       slot.flat_shop_restored_native_owner_bytes &&
	       slot.flat_shop_restored_reserved_bytes >=
		       slot.flat_shop_restored_native_owner_bytes &&
	       !slot.flat_shop_native_attempt_started && !slot.flat_shop_native_reserved_bytes &&
	       !slot.flat_shop_command_reserved_bytes && !slot.flat_shop_payload_reserved_bytes &&
	       !slot.flat_shop_publication_reserved_bytes &&
	       slot.flat_shop_journal_command.empty() && slot.original_shop_body.empty();
}
#endif
}

bool player_save_restored_publication_owner::publish_shop_flat_restored(
	const critical_command &original, const critical_completion &completion,
	bool (*native_publish)(player_save_restored_publication_owner &, void *) noexcept,
	void *context) noexcept
{
#ifndef __NO_MYSQL__
	(void)original;
	(void)completion;
	(void)native_publish;
	(void)context;
	return false;
#else
	// No cold terminal-native cleanup proof is present. A refused admission or
	// absent store receipt cannot release this restored original hold.
	if (!nevent_is_game_thread() || !native_publish ||
	    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY ||
	    !critical_completion_disposition_valid(completion) ||
	    completion.disposition != critical_completion_disposition::execution ||
	    completion.failure_stage != critical_failure_stage::none ||
	    (completion.outcome != critical_apply_outcome::applied &&
	     completion.outcome != critical_apply_outcome::already_applied &&
	     completion.outcome != critical_apply_outcome::terminal_failure) ||
	    original.operation_id.bytes != completion.operation_id.bytes)
		return false;
	try
	{
		economic_frozen_intent intent;
		shop_trade_payload payload{};
		economic_account_key wallet, bank, counterparty;
		critical_command command;
		std::vector<uint8_t> frozen;
		if (!original.publication_required ||
		    original.payload_version != SHOP_TRADE_RECOVERY_PAYLOAD_VERSION ||
		    shop_trade_accounting_decode(original, &intent, &payload, &wallet, &bank,
						 &counterparty) != economic_accounting_error::ok ||
		    !payload.recovery_manifest_recorded || !payload.player_pid ||
		    payload.player_pid > INT_MAX || !payload.selected_item_uid ||
		    !payload.expected_player_save_revision || !payload.expected_player_level ||
		    critical_command_encode(original, &frozen) != critical_command_codec_result::ok)
			return false;
		const int pid = static_cast<int>(payload.player_pid);
		if (player_save_worker_pid_pending(pid) || player_save_journal_pid_quarantined(pid))
			return false;
		uint64_t epoch = 0, generation = 0;
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			const auto *slot = find_literal_inventory_locked(pid);
			if (!health.initialized || stop_requested || !slot ||
			    !flat_shop_restored_slot_matches(*slot, pid, payload.selected_item_uid,
							     original.operation_id, frozen,
							     slot->flat_shop_ownership_epoch,
							     slot->execution_hold_generation) ||
			    find_terminal_fence_locked(pid) ||
			    find_target_save_login_fence_locked(pid) ||
			    append_inflight_pid == pid || any_snapshot_is_retained_locked(pid) ||
			    critical_command_decode(slot->payload.data(), slot->payload.size(),
						    &command) !=
				    critical_command_codec_result::ok ||
			    !critical_command_equal(command, original))
				return false;
			epoch = slot->flat_shop_ownership_epoch;
			generation = slot->execution_hold_generation;
		}
		player_save_restored_publication_owner owner(std::move(command), std::move(frozen),
							     completion, epoch, pid, generation);
		owner.flat_shop_restored_ = true;
		owner.flat_shop_restored_root_uid_ = payload.selected_item_uid;
		owner.flat_shop_restored_epoch_ = epoch;
		const auto original_slot_current = [&](bool require_stage_charge) noexcept
		{
			std::lock_guard<std::mutex> lock(pipeline_mutex);
			const auto *slot = find_literal_inventory_locked(pid);
			return health.initialized && !stop_requested && slot &&
			       flat_shop_restored_slot_matches(
				       *slot, pid, owner.flat_shop_restored_root_uid_,
				       owner.command_.operation_id, owner.frozen_,
				       owner.flat_shop_restored_epoch_, owner.generation_) &&
			       (!require_stage_charge ||
				slot->flat_shop_restored_publication_reserved_bytes) &&
			       !find_terminal_fence_locked(pid) &&
			       !find_target_save_login_fence_locked(pid) &&
			       append_inflight_pid != pid && !any_snapshot_is_retained_locked(pid);
		};
		if (!owner.reservation_.matches_pid(pid) || player_save_worker_pid_pending(pid) ||
		    !critical_command_coordinator_restored_shop_publication_current(owner))
			return false;
		player_revision_snapshot revision{};
		if (player_revision_snapshot_copy(pid, &revision) &&
		    (revision.overflowed || revision.queued_components ||
		     revision.inflight_components))
			return false;
		std::vector<player_save_journal_retained_frame> originals;
		if (player_save_journal_collect_publication_frames(
			    pid, owner.reservation_, &originals) != player_save_journal_result::ok)
			return false;
		if (!originals.empty())
		{
			// The genuine repository observer supports flat saved files. Do this
			// before the native callback obtains root; no nested root/player lock.
			player_save_covered_revision covered;
			if (!player_snapshot_repository_observe_covered_revision(
				    pid, owner.reservation_, &covered) ||
			    !critical_command_coordinator_restored_shop_publication_current(
				    owner) ||
			    !original_slot_current(false) ||
			    player_save_journal_retire_covered_ordinary(pid, owner.reservation_,
									covered, originals) !=
				    player_save_journal_result::ok)
				return false;
		}
		if (!original_slot_current(false) || !owner.reservation_.matches_pid(pid) ||
		    player_save_worker_pid_pending(pid) ||
		    player_save_journal_publication_census(pid, owner.reservation_) !=
			    player_save_journal_result::ok ||
		    !critical_command_coordinator_restored_shop_publication_current(owner) ||
		    !native_publish(owner, context) || !owner.reservation_.matches_pid(pid) ||
		    !original_slot_current(true) || player_save_worker_pid_pending(pid) ||
		    player_save_journal_publication_census(pid, owner.reservation_) !=
			    player_save_journal_result::ok ||
		    !critical_command_coordinator_restored_shop_publication_current(owner))
			return false;
		owner.publication_proven_ = true;
		return critical_command_coordinator_acknowledge_publication(owner);
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_restored_publication_owner::reserve_shop_flat_restored_publication_checkpoint(
	size_t bytes) noexcept
{
#ifndef __NO_MYSQL__
	(void)bytes;
	return false;
#else
	try
	{
		if (!nevent_is_game_thread() || !bytes || bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES ||
		    !flat_shop_restored_ || flat_shop_ || publication_proven_ || acknowledged_ ||
		    pid_ <= 0 || !flat_shop_restored_root_uid_ || !flat_shop_restored_epoch_ ||
		    !generation_ || command_.operation_id.bytes != completion_.operation_id.bytes ||
		    completion_.disposition != critical_completion_disposition::execution ||
		    !reservation_.matches_pid(pid_) || player_save_worker_pid_pending(pid_) ||
		    player_save_journal_pid_quarantined(pid_))
			return false;
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(pid_);
		if (!health.initialized || stop_requested || !slot ||
		    !flat_shop_restored_slot_matches(*slot, pid_, flat_shop_restored_root_uid_,
						     command_.operation_id, frozen_,
						     flat_shop_restored_epoch_, generation_) ||
		    find_terminal_fence_locked(pid_) || find_target_save_login_fence_locked(pid_) ||
		    append_inflight_pid == pid_ || any_snapshot_is_retained_locked(pid_))
			return false;
		// Borrow this outer ticket. No hold registration, root lookup, backend
		// recovery, temporary ticket, recapture or attempted-native flag exists.
		if (slot->flat_shop_restored_publication_reserved_bytes)
			return slot->flat_shop_restored_publication_reserved_bytes == bytes &&
			       reservation_.valid();
		if (!literal_inventory_capacity_locked(bytes) || !reservation_.valid())
			return false;
		// The genuine native owner has allocated/counted its COMPLETE cold stage;
		// only nonthrowing exact retention follows. No resize/reclaim on retries.
		slot->flat_shop_restored_publication_reserved_bytes = bytes;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool player_save_restored_publication_owner::consume_acknowledged_flat_shop_restored_hold() noexcept
{
#ifndef __NO_MYSQL__
	return false;
#else
	// Global admission poison is deliberately not consulted after durable ACK.
	// The original private matcher consumes only this exact local ticket/hold.
	if (!flat_shop_restored_ || flat_shop_ || !acknowledged_ || pid_ <= 0 ||
	    command_.operation_id.bytes != completion_.operation_id.bytes)
		return false;
	player_save_deferred_identity wake{};
	bool active = false;
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		auto *slot = find_literal_inventory_locked(pid_);
		if (!slot ||
		    !flat_shop_restored_slot_matches(*slot, pid_, flat_shop_restored_root_uid_,
						     command_.operation_id, frozen_,
						     flat_shop_restored_epoch_, generation_) ||
		    !slot->flat_shop_restored_publication_reserved_bytes)
		{
			player_save_execution_guard::poison_integrity();
			return false;
		}
		active = player_save_worker_deferred_identity(pid_, &wake);
		if (!reservation_.consume_acknowledged_hold())
		{
			player_save_execution_guard::poison_integrity();
			return false;
		}
		*slot = {};
	}
	if (active)
		(void)player_save_worker_resume_deferred_exact(wake);
	replay_revisit_requested.store(true);
	player_save_execution_guard::signal_ownership_change(
		player_save_execution_guard::current_ownership_epoch());
	return true;
#endif
}

namespace
{
bool coin_save_pool_add(size_t &total, size_t value) noexcept
{
	if (value > SIZE_MAX - total)
		return false;
	total += value;
	return true;
}
// Caller genuinely owns pipeline_mutex. This is the physical current of the
// complete literal checkpoint pool, not the other pipeline workers/queues or
// execution-guard owner. Actual reserved logical numbers are not extra heaps.
bool coin_save_literal_pool_current_locked(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	size_t total = sizeof(literal_inventory_checkpoints) +
		       sizeof(flat_shop_inventory_generation) +
		       sizeof(literal_inventory_generation);
	for (const auto &slot : literal_inventory_checkpoints)
	{
		for (const auto *body :
		     { &slot.held_before, &slot.held_after, &slot.held_attachment,
		       &slot.held_attachment_successor, &slot.payload, &slot.original_shop_body,
		       &slot.original_native_quest_before, &slot.original_native_quest_after,
		       &slot.original_auction_before, &slot.original_auction_after,
		       &slot.restored_auction_attachment, &slot.restored_native_quest_attachment,
		       &slot.flat_shop_journal_command })
			if (!coin_save_pool_add(total, body->capacity()))
				return false;
		for (const auto *identity : { &slot.flat_shop_root, &slot.flat_shop_account })
		{
			const size_t capacity = identity->capacity();
			if (capacity > 15 &&
			    (!coin_save_pool_add(total, capacity) || !coin_save_pool_add(total, 1)))
				return false;
		}
	}
	*bytes = total;
	return true;
#else
	return false;
#endif
}
} // namespace
bool player_save_pipeline_literal_replay_storage_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
	try
	{
		std::lock_guard<std::mutex> lock(pipeline_mutex);
		return coin_save_literal_pool_current_locked(bytes);
	}
	catch (...)
	{
		return false;
	}
}

namespace
{
bool coin_save_exclusive_add(size_t outer, size_t extra, size_t &result) noexcept
{
	if (extra > SIZE_MAX - outer)
		return false;
	result = outer + extra;
	return true;
}
constexpr size_t coin_save_allocator_frames =
	// _M_allocate, allocator_traits::allocate, allocator::allocate (C++20):
	// each this/allocator reference, n and returned pointer; new_allocator
	// adds its genuine hint pointer; operator new n and returned pointer.
	3 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	sizeof(void *) + sizeof(size_t) +
	// _M_deallocate/traits/allocator/new_allocator: allocator/this+p+n,
	// then sized operator delete p+n. Trivial element _Destroy closures.
	4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t) +
	(3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *)) +
	// vector max_size/_S_max_size/traits max_size/new_allocator::_M_max_size
	// references/results and actual diffmax/allocmax locals. C++20 allocator
	// has no max_size member; that inactive C++17 branch is not counted.
	4 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(size_t) +
	// traits::construct -> construct_at -> forward -> placement-new; all
	// constructor arguments here are real references to trivial values.
	3 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t);
constexpr size_t coin_save_copy_frames =
	// __uninitialized_move_if_noexcept_a and __uninitialized_copy_a: 3
	// iterators+allocator-reference+returned iterator each. Runtime ordinary
	// uninitialized_copy's two boolean locals and __uninit_copy carrier.
	2 * (4 * sizeof(void *) + sizeof(void *)) + 3 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(bool) + 3 * sizeof(void *) + sizeof(void *) +
	// copy/copy_move_a/a1/a2/copy_m, each3 iterator params+return; real
	// miter/niter/wrap/assign_one and memmove argument/result scopes.
	5 * (3 * sizeof(void *) + sizeof(void *)) + 2 * (sizeof(void *) + sizeof(void *)) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// distance/__distance and normal-iterator subtraction/base/dereference/
	// ++/comparison/constructor source parameter/return scopes.
	2 * (2 * sizeof(void *) + sizeof(std::ptrdiff_t)) + sizeof(char) +
	6 * (2 * sizeof(void *)) + sizeof(std::ptrdiff_t) + sizeof(bool) +
	// Fitting forward insert reaches advance(__mid,__elems_after), even zero.
	// advance: iterator-reference, size_t n, real local difference_type __d;
	// __iterator_category: iterator-reference and actual returned RA tag;
	// __advance: iterator-reference, difference n and by-value RA tag;
	// actual += this/n/reference-return, plus source ++/-- alternatives.
	sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	4 * sizeof(void *);
constexpr size_t coin_save_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t coin_save_default_frames =
	// Runtime default_n_a/default_n/default_n_1<true>: real first/n/allocator
	// reference, can_fill and val locals, actual returned pointer carriers.
	(3 * sizeof(void *) + sizeof(size_t)) +
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(3 * sizeof(void *) + sizeof(size_t)) +
	// _Construct's real location plus placement-new n/location/result.
	sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	// fill_n/__fill_n_a<random_access>: first/n/value/result/tag;
	// __size_to_integer argument/result; __fill_a/__fill_a1 scalar __tmp.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(char) + 2 * sizeof(size_t) +
	2 * (3 * sizeof(void *)) + sizeof(uint64_t);
constexpr size_t coin_save_vector_frames =
	coin_save_allocator_frames + coin_save_copy_frames + coin_save_relocate_frames +
	coin_save_default_frames +
	// reserve this/n/old_size/tmp; assign public/forward-aux and exact
	// _M_allocate_and_copy's this/n/first/last/result/returned pointer.
	2 * sizeof(void *) + 2 * sizeof(size_t) + 7 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(char) + 5 * sizeof(void *) + sizeof(size_t) +
	// push_back/emplace_back and real realloc_insert old/new start/finish,
	// len/elems_before/position/forward value reference; _M_check_len.
	2 * sizeof(void *) + 3 * sizeof(void *) + 7 * sizeof(void *) + 2 * sizeof(size_t) +
	2 * sizeof(void *) + 3 * sizeof(size_t) +
	// C++20 forward insert public/range-insert (no old dispatch), offset/elems_after/
	// len/old-start/finish/mid/new-start/finish/iterator return/tag scopes.
	15 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(char) +
	// default_append's n/size/navail/len and real old/new/destroy pointers.
	5 * sizeof(void *) + 4 * sizeof(size_t) +
	// begin/end/cbegin/size/capacity/get-allocator declared carriers and
	// iterator-category/std::max arguments/results on the real call paths.
	7 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(char) + 3 * sizeof(void *);
constexpr size_t coin_save_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + coin_save_allocator_frames;
constexpr size_t coin_save_vector_constructor_frames =
	2 * sizeof(void *) + 3 * sizeof(std::allocator<int32_t>) + 2 * sizeof(void *) +
	sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	8 * (sizeof(void *) + sizeof(size_t)) + coin_save_vector_frames;
constexpr size_t coin_save_command_default_frames =
	// Real command generated default/destructor and four vector default
	// constructor/_Vector_base/_Vector_impl/_Vector_impl_data/allocator
	// carriers; current object inline is separately owned by its lifetime.
	2 * sizeof(void *) + 4 * (4 * sizeof(void *) + sizeof(std::allocator<uint8_t>)) +
	4 * (sizeof(void *) + coin_save_allocator_frames);
constexpr size_t coin_save_critical_codec_frames =
	// Original encoder, working-bytes and bounded-encode parameter/return/
	// wire_bytes/status scopes, loop key+revision refs/endpoints/pad locals.
	10 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(critical_command_codec_result) +
	6 * sizeof(void *) + 2 * sizeof(unsigned int) +
	// append_le genuine widest uint64_t value plus byte loop and vector
	// reference; array begin/end and data query sources.
	sizeof(void *) + sizeof(uint64_t) + sizeof(size_t) + 6 * (sizeof(void *) + sizeof(size_t)) +
	// Actual original decoder/bounded counterpart fixed scalar locals:
	// encoded/size/destination/reserve/context/outer/heap output, live,
	// offset/type/source/3 counts/auction flag/limit/required/intent locals.
	5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(critical_command_codec_result) +
	2 * sizeof(size_t) + 2 * sizeof(uint16_t) + 3 * sizeof(uint32_t) + sizeof(bool) +
	sizeof(size_t) + sizeof(uint64_t) + 2 * sizeof(size_t) + sizeof(uint32_t) +
	// Key/revision loop indices and padding, prospective request/extra,
	// retained scalar and original bad_alloc reference. Object carriers
	// decoded/key/revision are admitted by existing real decoder itself.
	2 * sizeof(uint32_t) + 2 * sizeof(size_t) + 4 * sizeof(size_t) + sizeof(size_t) +
	sizeof(void *) +
	// Genuine widest read_le input/size/offset/value/decoded/index/return;
	// decode_add/admit/heap actual parameters/locals/query scopes.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(uint64_t) + sizeof(size_t) + sizeof(bool) +
	10 * sizeof(void *) + 9 * sizeof(size_t) + 3 * sizeof(bool) +
	// vector constructions/destruction/calls, allocator and all fitting
	// insert/assign/append profiles, original command nonthrow final move.
	coin_save_command_default_frames + coin_save_vector_frames + 4 * coin_save_move_frames;

constexpr size_t coin_save_vector_defaults = 5 * sizeof(void *) + sizeof(std::allocator<uint8_t>);
constexpr size_t coin_save_vector_move_frames = coin_save_move_frames;
constexpr size_t coin_save_bytes_equal_frames =
	2 * sizeof(void *) + sizeof(bool) + 6 * (sizeof(void *) + sizeof(void *)) +
	4 * (3 * sizeof(void *) + sizeof(bool)) + sizeof(std::ptrdiff_t) + 2 * sizeof(void *) +
	sizeof(size_t) + sizeof(int);
constexpr size_t coin_save_lock_frames =
	// Actual unique_lock constructor(this,mutex ref), addressof arg/result,
	// unique_lock::lock this, mutex::lock this/error int, gthread mutex
	// argument/int-return. Destructor/unlock and real mutex getter/owns.
	2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(int) +
	sizeof(void *) + sizeof(int) + 3 * sizeof(void *) + sizeof(int) + 2 * sizeof(void *) +
	sizeof(bool) + 2 * sizeof(void *);
constexpr size_t coin_save_pool_observation_frames =
	// Prefix/admit/exclusive bridge parameters/results/live/pool scalars,
	// actual locked pool observer pointer/scalar/range refs/iterators;
	// genuine 13 and 2 pointer initializer backing arrays and descriptors.
	9 * sizeof(void *) + 9 * sizeof(size_t) + 5 * sizeof(bool) + sizeof(void *) +
	2 * sizeof(size_t) + 4 * sizeof(void *) + 13 * sizeof(void *) +
	2 * sizeof(std::initializer_list<const std::vector<uint8_t> *>) + 2 * sizeof(void *) +
	2 * sizeof(std::initializer_list<const std::string *>) +
	6 * (sizeof(void *) + sizeof(size_t));
constexpr size_t coin_save_map_lookup_frames =
	// Genuine map count/find (this,key,size/iterator-return), tree find
	// (this,key,j,iterator-return), iterative lower_bound(this,x,y,key,
	// iterator-return), _S_key + key-of-value / node-value / less comparator.
	// No allocation, recursion, insertion or rebalance is reached.
	2 * (2 * sizeof(void *) + sizeof(size_t) + sizeof(void *)) + 2 * (4 * sizeof(void *)) +
	2 * (5 * sizeof(void *)) + 2 * (6 * sizeof(void *) + sizeof(bool)) +
	// Actual begin/end/root/left/right getters, iterator constructor,
	// iterator equality input refs/bool and key comparison arguments.
	8 * (2 * sizeof(void *)) + 2 * sizeof(void *) + sizeof(bool) + 3 * sizeof(void *) +
	sizeof(bool);
constexpr size_t coin_save_hold_frames =
	// Original install_hold pid/op/generation and bool return, nonzero and
	// byte, actual detail mutex lock_guard, owner iterator/available pointer,
	// hold ref/range pointers, fixed actual assignment temporary held_pid.
	sizeof(int) + 2 * sizeof(void *) + sizeof(bool) + sizeof(bool) + sizeof(uint8_t) +
	sizeof(std::lock_guard<std::mutex>) + coin_save_lock_frames + sizeof(void *) +
	sizeof(void *) + 3 * sizeof(void *) +
	sizeof(player_save_execution_guard::detail::held_pid) + 4 * sizeof(void *) +
	// Four-element operation byte-array equality and generation limit getter.
	coin_save_bytes_equal_frames + sizeof(uint64_t) +
	// Separate original mismatch poison owns a fresh lock_guard after
	// install_hold has returned. Keep this existing conservative sum EXACT:
	// the dead install_hold carriers may cover that distinct poison branch.
	// Its full source-attributed notification profile is proved below.
	sizeof(std::lock_guard<std::mutex>) + coin_save_lock_frames + coin_save_map_lookup_frames;
// Actual duplicate-generation mismatch alone reaches poison_integrity.
// install_hold success has NO changed_locked call; do not attribute the
// adjacent install_live_publication_hold notification to this replay route.
constexpr size_t coin_save_poison_notification_frames =
	// poison_integrity owns its actual new guard lock after install_hold's
	// local lock/iterators/held_pid temporary have died. No map lookup here.
	sizeof(std::lock_guard<std::mutex>) + coin_save_lock_frames +
	// changed_locked: numeric_limits<uint64_t>::max returned scalar; its
	// no-argument void call has no parameter/local object. ownership_event
	// returns an actual reference carrier. Nonzero epoch proves its static
	// condition variable was initialized by begin_ownership_epoch BEFORE
	// epoch publication; no first-use constructor/heap request on this path.
	sizeof(uint64_t) + sizeof(void *) +
	// Installed GCC13 public condition_variable::notify_all (out-of-line
	// primary GCC13.3 implementation): this pointer, no explicit local.
	sizeof(void *) +
	// Actual std::__condvar::notify_all: this pointer and local int __e.
	sizeof(void *) + sizeof(int) +
	// Actual __gthread_cond_broadcast: condition pointer + int result;
	// pthread_cond_broadcast declaration boundary: pointer + int result.
	// No __gthread_active_p is called by this unconditional broadcast path.
	sizeof(void *) + sizeof(int) + sizeof(void *) + sizeof(int);
static_assert(coin_save_hold_frames >= coin_save_poison_notification_frames,
	      "Existing hold allowance covers the separate poison notification carriers");
constexpr size_t coin_save_capacity_frames =
	// Complete original literal_inventory_capacity_locked parameters/ref,
	// original_body_bytes/amount/extra/held_extra/auction_extra, all fixed
	// actual initializer lists: 2 identity refs and 9 body refs plus descriptors.
	sizeof(size_t) + sizeof(void *) + sizeof(bool) + sizeof(void *) + 5 * sizeof(size_t) +
	2 * sizeof(void *) + 9 * sizeof(void *) +
	sizeof(std::initializer_list<const std::string *>) +
	sizeof(std::initializer_list<const std::vector<uint8_t> *>) + 4 * sizeof(void *) +
	8 * (sizeof(void *) + sizeof(size_t)) +
	// Original smith_profile enum parameter/bool return.
	sizeof(literal_checkpoint_profile) + sizeof(bool);
constexpr size_t coin_save_restore_frames =
	// Original/new obligation parameters/result, real profile/exclusive
	// prefix/scalar, slot/candidate refs and both generation locals.
	4 * sizeof(void *) + sizeof(int) + sizeof(uint64_t) + 2 * sizeof(size_t) + sizeof(bool) +
	sizeof(literal_checkpoint_profile) + sizeof(size_t) + 3 * sizeof(void *) +
	2 * sizeof(uint64_t) +
	// Actual prefix lambda closure this/frozen ref/outer, arguments/results
	// and local live scalar. Command/frozen input frames remain caller-owned.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(size_t) + sizeof(void *) + sizeof(bool) +
	sizeof(size_t) + coin_save_bytes_equal_frames + coin_save_capacity_frames +
	coin_save_hold_frames + coin_save_vector_move_frames;
} // namespace

player_save_coin_replay_budget_scope_owner::player_save_coin_replay_budget_scope_owner(
	bool (*reserve)(size_t, void *) noexcept, void *context)
	: lock_(pipeline_mutex)
	, reserve_(reserve)
	, context_(context)
{
}
player_save_coin_replay_budget_scope_owner::~player_save_coin_replay_budget_scope_owner() noexcept =
	default;
bool player_save_coin_replay_budget_scope_owner::locked() const noexcept
{
	return lock_.mutex() == &pipeline_mutex && lock_.owns_lock();
}
namespace
{
bool coin_save_other_pipeline_current_locked(size_t *) noexcept;
}

bool player_save_coin_replay_budget_scope_owner::prefix(size_t exclusive,
							size_t &result) const noexcept
{
	if (!prepared() || !reserve_)
		return false;
	size_t pool = 0, other = 0, worker = 0, total = exclusive;
	if (!coin_save_literal_pool_current_locked(&pool) ||
	    !coin_save_other_pipeline_current_locked(&other) ||
	    !prepared_worker_storage_bytes(&worker) || !coin_save_pool_add(total, pool) ||
	    !coin_save_pool_add(total, other) || !coin_save_pool_add(total, worker) ||
	    !coin_save_pool_add(total, sizeof(*this)) ||
	    !coin_save_pool_add(total, observer_frame_bytes()))
		return false;
	result = total;
	return true;
}
bool player_save_coin_replay_budget_scope_owner::admit(size_t exclusive) const noexcept
{
	size_t live = 0;
	return prefix(exclusive, live) && reserve_(live, context_);
}
bool player_save_coin_replay_budget_scope_owner::reserve_exclusive(size_t exclusive,
								   void *context) noexcept
{
	const auto *scope = static_cast<player_save_coin_replay_budget_scope_owner *>(context);
	return scope && scope->admit(exclusive);
}
bool player_save_coin_replay_budget_scope_owner::restore_obligation(const critical_command &command,
								    int pid, uint64_t root_uid,
								    size_t outer_live) const
{
	const auto profile = literal_checkpoint_profile::ordinary_drop;
	constexpr size_t frames = coin_save_restore_frames;
	size_t admission_prefix = 0;
	if (!coin_save_exclusive_add(
		    outer_live, frames + sizeof(std::vector<uint8_t>) + coin_save_vector_defaults,
		    admission_prefix) ||
	    !admit(admission_prefix))
		return false;
	std::vector<uint8_t> frozen;
	const auto prefix = [this, &frozen, outer_live](size_t extra, size_t &output) noexcept
	{
		size_t live = outer_live;
		if (!coin_save_pool_add(live, coin_save_restore_frames + sizeof(frozen)) ||
		    !coin_save_pool_add(live, frozen.capacity()) ||
		    !coin_save_pool_add(live, extra))
			return false;
		output = live;
		return true;
	};
	if (pid <= 0 || !root_uid ||
	    (!prefix(coin_save_critical_codec_frames, admission_prefix) ?
		     critical_command_codec_result::overflow :
		     critical_command_encode_bounded(
			     command, &frozen, reserve_exclusive,
			     const_cast<player_save_coin_replay_budget_scope_owner *>(this),
			     admission_prefix)) != critical_command_codec_result::ok)
		return false;
	// The real enclosing scope owns this same pipeline_mutex unique_lock.
	if (!locked())
		return false;
	// The typed caller classified the original immutable room operation.
	// Registration remains closed before any execution owner starts.
	if (!health.initialized || stop_requested || execution_started)
		return false;
	literal_inventory_checkpoint *slot = nullptr;
	for (auto &candidate : literal_inventory_checkpoints)
	{
		if (!prefix(coin_save_hold_frames + coin_save_bytes_equal_frames,
			    admission_prefix) ||
		    !admit(admission_prefix))
			return false;
		if (candidate.token.pid == pid ||
		    (candidate.held && candidate.operation_id.bytes == command.operation_id.bytes))
		{
			uint64_t generation = 0;
			if (candidate.profile != profile || !candidate.restored_sql_drop ||
			    !candidate.held || candidate.token.pid != pid ||
			    candidate.token.root_uid != root_uid ||
			    candidate.operation_id.bytes != command.operation_id.bytes ||
			    candidate.payload != frozen ||
			    !player_save_execution_guard::install_hold(pid, command.operation_id,
								       &generation))
				return false;
			if (generation != candidate.execution_hold_generation)
			{
				player_save_execution_guard::poison_integrity();
				return false;
			}
			return true;
		}
		if (!candidate.token.pid && !slot)
			slot = &candidate;
	}
	if (!slot || !literal_inventory_capacity_locked(frozen.size()))
		return false;
	uint64_t generation = 0;
	if (!prefix(coin_save_hold_frames + coin_save_capacity_frames +
			    coin_save_vector_move_frames,
		    admission_prefix) ||
	    !admit(admission_prefix))
		return false;
	if (!player_save_execution_guard::install_hold(pid, command.operation_id, &generation))
		return false;
	// All fallible command allocation/validation precedes guard installation.
	slot->execution_hold_generation = generation;
	slot->profile = profile;
	slot->token.pid = pid;
	slot->token.root_uid = root_uid;
	slot->payload = std::move(frozen);
	slot->operation_id = command.operation_id;
	slot->held = true;
	slot->restored_sql_drop = true;
	return true;
}

bool player_save_coin_replay_budget_scope_owner::restore(const critical_command &command,
							 size_t outer_live) const noexcept
{
	try
	{
		constexpr size_t frames = 5 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(int) +
					  sizeof(uint64_t) + 3 * sizeof(bool) + sizeof(void *);
		size_t admission_prefix = 0;
		if (!coin_save_exclusive_add(outer_live, frames, admission_prefix) ||
		    !admit(admission_prefix))
			return false;
		int pid = 0;
		uint64_t uid = 0;
		if (!coin_physical_recovery_identity_bounded(
			    command, &pid, &uid, reserve_exclusive,
			    const_cast<player_save_coin_replay_budget_scope_owner *>(this),
			    admission_prefix))
			return false;
		return restore_obligation(command, pid, uid, admission_prefix);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

namespace
{
// Caller genuinely owns pipeline_mutex and has proved PREPARED startup, so no
// dispatcher stack body exists outside these retained owners. Literal pool and
// its two generations are excluded and observed by their existing provider.
bool coin_save_other_pipeline_current_locked(size_t *output) noexcept
{
	if (!output)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	using recapture_node = std::_Rb_tree_node<int32_t>;
	size_t bytes = sizeof(player_save_pipeline_lifecycle_detail::mutex) +
		       sizeof(pipeline_mutex) + sizeof(append_available) + sizeof(pending_append) +
		       sizeof(durable_ready) + sizeof(append_retry) + sizeof(dispatcher) +
		       sizeof(health) + sizeof(replay_gate) + sizeof(retained_bytes) +
		       sizeof(stop_requested) + sizeof(accepting) + sizeof(execution_started) +
		       sizeof(dispatcher_entry_acknowledged) + sizeof(shutdown_incomplete) +
		       sizeof(lifecycle_admission_closed) + sizeof(lifecycle_stop_attempted) +
		       sizeof(append_inflight) + sizeof(replay_revisit_requested) +
		       sizeof(append_inflight_pid) + sizeof(append_inflight_revision) +
		       sizeof(custody_recapture_armed) + sizeof(terminal_fences) +
		       sizeof(target_save_login_fences) +
		       // Actual trace_player_saves function-local cached bool and lazy guard
		       // occupy static storage even without calling/initializing the switch.
		       sizeof(bool) + sizeof(__cxxabiv1::__guard);
	size_t heap = 0;
	if (!pending_append.current_heap_bytes(&heap) || !coin_save_pool_add(bytes, heap) ||
	    !durable_ready.current_heap_bytes(&heap) || !coin_save_pool_add(bytes, heap))
		return false;
	const auto add_body = [&bytes](const player_snapshot &body) noexcept
	{
		size_t retained = 0;
		return player_snapshot_current_heap_bytes(body, &retained) &&
		       coin_save_pool_add(bytes, retained);
	};
	for (const auto &entry : pending_append)
		if (!add_body(entry.body))
			return false;
	for (const auto &entry : durable_ready)
		if (!add_body(entry.body))
			return false;
	if (append_retry && !add_body(append_retry->body))
		return false;
	for (const auto &fence : terminal_fences)
		if (fence.death_snapshot && !add_body(*fence.death_snapshot))
			return false;
	if (custody_recapture_armed.size() > SIZE_MAX / sizeof(recapture_node) ||
	    !coin_save_pool_add(bytes, custody_recapture_armed.size() * sizeof(recapture_node)))
		return false;
	*output = bytes;
	return true;
#else
	return false;
#endif
}
} // namespace

bool player_save_coin_replay_budget_scope_owner::prepared() const noexcept
{
	// Actual same-lock lifecycle state, never an idle/zero-size substitute.
	// The ROOT startup owner separately excludes concurrent lifecycle callers.
	return locked() && health.initialized && !stop_requested && !accepting &&
	       !execution_started && !dispatcher.joinable() && !health.dispatcher_running &&
	       !dispatcher_entry_acknowledged && !append_inflight && !shutdown_incomplete &&
	       !lifecycle_admission_closed && !lifecycle_stop_attempted &&
	       !replay_gate.loads_allowed();
}

namespace
{
// Complete additional observer carriers beyond the preserved original pool
// profile. True simultaneous parent-child lifetimes use a maximum of separate
// branches; no guessed cushion substitutes for an undisclosed source path.
constexpr size_t coin_save_prepared_queries =
	// prepared()/locked(): this and bool results, unique_lock mutex/owns getters.
	sizeof(void *) + sizeof(bool) +
	player_retained_observer_source::maximum(
		sizeof(void *) + sizeof(bool) + 2 * sizeof(void *),
		player_retained_observer_source::maximum(
			// thread::joinable, id temporary, by-value equality operands,
			// real id default constructor and both bool result carriers.
			2 * sizeof(void *) + 3 * sizeof(std::thread::id) + 2 * sizeof(bool),
			// loads_allowed -> atomic<bool>::load -> __atomic_base::load;
			// real __b local, memory_order mask operator and builtin load.
			3 * sizeof(void *) + 3 * sizeof(std::memory_order) + 3 * sizeof(bool) +
				player_retained_observer_source::maximum(
					3 * sizeof(std::memory_order),
					sizeof(void *) + sizeof(int) + sizeof(bool))));
constexpr size_t coin_save_deque_observer_queries =
	// this/output/impl refs, bool return, maximum/nodes/elements/block/
	// map_bytes/node_bytes; numeric limit and real deque_buf_size query.
	3 * sizeof(void *) + sizeof(bool) + 6 * sizeof(size_t) + sizeof(std::ptrdiff_t) +
	3 * sizeof(size_t);
constexpr size_t coin_save_deque_range_live =
	2 * sizeof(void *) + 2 * sizeof(decltype(pending_append)::iterator);
constexpr size_t coin_save_deque_range_query = player_retained_observer_source::maximum(
	// begin/end return real four-pointer iterator; converting/copy constructor.
	3 * sizeof(void *) + sizeof(decltype(pending_append)::iterator),
	// ++ -> _M_set_node -> _S_buffer_size -> __deque_buf_size. All genuine
	// parameter/return scopes; compare and dereference are smaller branches.
	4 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::ptrdiff_t));
constexpr size_t coin_save_body_lambda_frames =
	2 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	player_retained_observer_source::maximum(player_retained_observer_source::snapshot_request,
						 player_retained_observer_source::checked_add);
constexpr size_t coin_save_other_pipeline_observer_frames =
	// output/result; bytes/heap locals; the actual one-reference add_body
	// closure persists while deque/optional/fence observations execute.
	2 * sizeof(void *) + sizeof(bool) + 2 * sizeof(size_t) +
	player_retained_observer_source::maximum(
		coin_save_deque_observer_queries,
		player_retained_observer_source::maximum(
			coin_save_deque_range_live +
				player_retained_observer_source::maximum(
					coin_save_deque_range_query, coin_save_body_lambda_frames),
			player_retained_observer_source::maximum(
				player_retained_observer_source::optional_arrow +
					coin_save_body_lambda_frames,
				player_retained_observer_source::array_range_live +
					player_retained_observer_source::maximum(
						player_retained_observer_source::array_range_query,
						player_retained_observer_source::optional_value +
							coin_save_body_lambda_frames)))) +
	// Recapture set/tree size query and exact original checked-add call.
	player_retained_observer_source::maximum(2 * sizeof(void *) + 2 * sizeof(size_t),
						 player_retained_observer_source::checked_add);
} // namespace

size_t player_save_coin_replay_budget_scope_owner::observer_frame_bytes() noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	// Original pool/lock profiles remain whole and unchanged. Two new prefix
	// scalars plus bootstrap(this/output/result and all four named scalars)
	// are real additional owning callers. Worker profile is separately typed
	// in its own TU; both pure fixed getters' result carriers are included.
	return coin_save_lock_frames + coin_save_pool_observation_frames + 2 * sizeof(size_t) +
	       2 * sizeof(void *) + sizeof(bool) + 4 * sizeof(size_t) +
	       // Both getter return carriers; all nine real worker getter constexpr
	       // automatic scalar locals; final maximum(a,b) parameters/result and
	       // its genuine conditional comparison carrier. No emitted optimization
	       // is assumed to erase these source-declared owning objects.
	       2 * sizeof(size_t) + 9 * sizeof(size_t) + 3 * sizeof(size_t) + sizeof(bool) +
	       coin_save_prepared_queries + coin_save_other_pipeline_observer_frames +
	       prepared_worker_observer_frame_bytes();
#else
	return 0;
#endif
}

bool player_save_coin_replay_budget_scope_owner::bootstrap_storage_bytes(
	size_t *output) const noexcept
{
	if (!output || !prepared())
		return false;
	size_t pool = 0, other = 0, worker = 0, bytes = 0;
	if (!coin_save_literal_pool_current_locked(&pool) ||
	    !coin_save_other_pipeline_current_locked(&other) ||
	    !prepared_worker_storage_bytes(&worker) || !coin_save_pool_add(bytes, pool) ||
	    !coin_save_pool_add(bytes, other) || !coin_save_pool_add(bytes, worker) ||
	    !coin_save_pool_add(bytes, sizeof(*this)))
		return false;
	// Fresh true storage on this SAME still-held pipeline scope, no caching.
	// Fixed observer frames are handed off separately to the ROOT lender.
	*output = bytes;
	return true;
}

size_t player_save_sql_drop_replay_owner::frame_bytes() noexcept
{
	// Full restore(original command/scope refs + outer + bool), typed payload,
	// full original captured SQL literal batch, two prospective prefix scalars,
	// original pid cast/root UID arguments, relay and genuine heap observers.
	return 8 * sizeof(void *) + 7 * sizeof(size_t) + 5 * sizeof(bool) + sizeof(int) +
	       sizeof(uint64_t) + sizeof(item_transfer_payload) +
	       sizeof(sql_room_item_payload_batch) + item_transfer_payload_copy_frame_bytes() +
	       player_item_snapshot_copy_frame_bytes() + critical_command_valid_frame_bytes();
}

bool player_save_sql_drop_replay_owner::current_storage_bytes(
	player_save_coin_replay_budget_scope_owner &scope, size_t *output) noexcept
{
	// Caller owns this genuine uninterrupted scope. Includes its inline lock/
	// callback/context storage once; unlike the public external physical getter.
	return scope.bootstrap_storage_bytes(output);
}

bool player_save_sql_drop_replay_owner::restore(const critical_command &command,
						player_save_coin_replay_budget_scope_owner &scope,
						size_t exclusive_outer) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)scope;
	(void)exclusive_outer;
	return false; // Original flatfile early refusal, no provider/decoder entered.
#else
	if (!scope.prepared() || !scope.reserve_)
		return false;
	try
	{
		size_t prefix = exclusive_outer, heap = 0;
		if (!coin_save_pool_add(prefix, frame_bytes()) || !scope.admit(prefix))
			return false;
		if (!command.publication_required || !critical_command_envelope_valid(command) ||
		    !item_transfer_accounting_command_supported_bounded(
			    command, player_save_coin_replay_budget_scope_owner::reserve_exclusive,
			    &scope, prefix))
			return false;
		item_transfer_payload payload = {};
		sql_room_item_payload_batch captured;
		if (!item_transfer_command_decode_payload_bounded(
			    command, &payload,
			    player_save_coin_replay_budget_scope_owner::reserve_exclusive, &scope,
			    prefix) ||
		    !item_transfer_payload_current_heap_bytes(payload, &heap) ||
		    !coin_save_pool_add(prefix, heap) ||
		    !sql_room_item_payload_capture_bounded(
			    payload, &captured,
			    player_save_coin_replay_budget_scope_owner::reserve_exclusive, &scope,
			    prefix) ||
		    !sql_room_item_payload_batch_current_heap_bytes(captured, &heap) ||
		    !coin_save_pool_add(prefix, heap))
			return false;
		// SAME actual scope, complete original ordinary-drop registration.
		// All original profile/duplicate/generation/poison/capacity/hold laws are
		// retained. restore_obligation has no fallible callback after attachment.
		return scope.restore_obligation(command, static_cast<int>(payload.from_owner.id),
						payload.selected_item_uid, prefix);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
#endif
}

bool player_save_pipeline_replay_current_storage_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	try
	{
		player_save_coin_replay_budget_scope_owner scope(nullptr, nullptr);
		size_t bytes = 0;
		if (!scope.bootstrap_storage_bytes(&bytes) || bytes < sizeof(scope))
			return false;
		// The transient observer scope is not retained pipeline storage. This
		// external physical owner excludes it; borrowed getter includes actual
		// caller-held scope instead. Lifecycle/PREPARED stability is mandatory.
		*output = bytes - sizeof(scope);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool player_save_pipeline_restore_sql_drop_obligation_bounded(
	const critical_command &command, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t exclusive_outer) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)reserve;
	(void)context;
	(void)exclusive_outer;
	return false;
#else
	if (!reserve)
		return false;
	try
	{
		// Authentic standalone owner for callers which do not hold pipeline_mutex.
		// A mixed startup caller MUST use the borrowed bridge above, not this.
		player_save_coin_replay_budget_scope_owner scope(reserve, context);
		size_t prefix = exclusive_outer;
		if (!coin_save_pool_add(prefix,
					4 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(bool)))
			return false;
		return player_save_sql_drop_replay_owner::restore(command, scope, prefix);
	}
	catch (...)
	{
		return false;
	}
#endif
}
