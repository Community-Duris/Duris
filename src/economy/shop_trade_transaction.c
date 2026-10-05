#include "economy/shop_trade_publication.h"

#include "economy/currency_transaction.h"
#include "item/item_ownership_runtime.h"
#include "core/prototypes.h"
#include "economy/shop_trade_runtime.h"
#include "economy/shop_trade_world_witness.h"
#include "core/utils.h"
#include "world/handler.h"

#include <algorithm>
#include <cerrno>
#include <new>
#include <map>
#include <climits>
#include <chrono>
#include "economy/shop_trade_accounting.h"
#include "economy/economic_command_admission.h"
#include <cstring>
#include <memory>
#include <limits>
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "persistence/shop_item_runtime_payload.h"
#include "economy/shop_trade_item_payload.h"
#include "economy/shop_trade_recovery_image.h"
#include "persistence/economic_sql_shop_trade_transaction.h"
#include "persistence/critical_command_repository.h"
#include <set>
#ifndef __NO_MYSQL__
#include "player/player_sql_transaction_cleanup.h"
#include "sql/sql_pool.h"
#endif

extern struct shop_data *shop_index;
extern int number_of_shops;

// The implementation owns pipeline friendship. Neither an operation ID nor a
// caller-supplied physical success callback can construct its guarded proof.
class shop_trade_native_publication_owner final
{
	friend class shop_trade_preparation_owner;
	static bool expected_player_forest(const shop_trade_payload &,
					   const std::vector<player_item_snapshot> &, bool rejected,
					   std::vector<player_item_snapshot> *) noexcept;
	// Pure ordering conversion after the owner has censused these transient
	// bodies/objects. Values and pointers alone never grant native/ACK authority.
	static bool expected_player_order(const shop_trade_payload &, P_char actor, P_obj selected,
					  P_obj destination,
					  const std::vector<player_item_snapshot> &,
					  std::vector<player_item_snapshot> *) noexcept;
	static bool expected_keeper_forest(const shop_trade_payload &, P_char keeper,
					   P_obj selected,
					   const std::vector<player_item_snapshot> &, bool rejected,
					   std::vector<player_item_snapshot> *) noexcept;
	static bool expected_destination_forest(const shop_trade_payload &, P_char actor,
						P_obj selected, P_obj destination,
						std::span<const uint8_t> original,
						std::vector<player_item_snapshot> *) noexcept;
	static bool native_publish(const critical_command &, const critical_completion &,
				   void *) noexcept;
	static bool no_native_effect(const critical_command &, const critical_completion &,
				     void *) noexcept
	{
		return false;
	}

    public:
	static bool publish_retained(const critical_command &command,
				     const critical_completion &sealed, void *entry) noexcept
	{
		return player_save_restored_publication_owner::publish_shop(command, sealed,
									    native_publish, entry);
	}
#ifndef __NO_MYSQL__
	// Borrowed original-transaction values only. This does not place an object,
	// publish a balance, clean up a lease or grant guarded ACK authority.
	static bool current_native_image(MYSQL *, const critical_command &,
					 const critical_completion &,
					 const player_shop_checkpoint_token &,
					 economic_sql_shop_trade_publication *) noexcept;
	// Cold v8 value counterpart, never a substitute token or native/ACK owner.
	// The exact historical receipt is verified under the same original SQL cut.
	static bool current_native_image(MYSQL *, const critical_command &,
					 const critical_completion &,
					 economic_sql_shop_trade_publication *) noexcept;

    private:
	static bool verified_current_receipt(MYSQL *, const critical_command &,
					     const critical_completion &,
					     const economic_sql_shop_trade_publication &) noexcept;

    public:
#endif
	static bool retire_never_admitted(const critical_command &command,
					  const critical_completion &completion) noexcept
	{
		return completion.disposition == critical_completion_disposition::never_admitted &&
		       player_save_restored_publication_owner::publish_shop(
			       command, completion, no_native_effect, nullptr);
	}
};

namespace
{
struct trade_preparation
{
	uint64_t generation = 0, actor_runtime_id = 0, keeper_runtime_id = 0;
	economic_shop_checkpoint_projection mapping{};
	player_shop_checkpoint_token player_token{};
	player_shop_checkpoint_stage player_stage{};
	bool player_held = false;
	bool submission_started = false;
	bool destination_shell_started = false, destination_shell_returned = false;
	shop_trade_destination_weight destination_weight{};
	bool destination_weight_ready = false;
	std::unique_ptr<critical_command> command;
#ifndef __NO_MYSQL__
	bool native_attempted = false, native_sealed = false, native_ready = false;
	shop_trade_native_checkpoint_stage native_stage;
#endif
	std::vector<uint8_t> selected, stock, destination, keeper_blob;
	size_t selected_capture_bytes = 0, destination_capture_bytes = 0;
	std::vector<player_item_snapshot> keeper_items;
	std::vector<uint64_t> fenced_uids;
};
uint64_t preparation_generation = 0;
struct pending_trade
{
	uint32_t player_pid = 0;
	std::unique_ptr<trade_preparation> preparation;
	shop_trade_payload payload = {};

	shop_trade_completion_fn completion = nullptr;
	shop_trade_preparation_refusal_fn refusal = nullptr;
	bool production_owned = false, driving = false;
	bool local_refused = false, refusal_attempted = false;
	unsigned int refusal_error = ECANCELED;
	bool completion_ready = false;
	// Exact never-journaled hold cancellation is independent of a live body.
	// Keep this original domain/produced continuation until notification returns.
	bool never_admitted_retired = false;
	bool native_acknowledged = false;
	bool native_receipt_verified = false;
	// These are original ordered values; never SQL UID-map iteration order.
	bool publication_forests_ready = false;
	std::vector<player_item_snapshot> before_player, after_player, after_keeper;
	std::vector<uint8_t> after_destination;

	critical_completion completed = {};
	shop_trade_physical_publication_fn publication = nullptr;
	shop_trade_accounted_publication_fn accounted_publication = nullptr;
	uint32_t physical_stages = 0;
	bool balances_published = false;
	uint64_t balance_runtime_id = 0;
	bool custody_published = false;
	bool revision_published = false;
	bool physical_published = false;
	bool publishing = false;
	bool blocked = false;
};

// Fixed keys and node insertion keep lookup and exception restoration allocation
// free. Native callbacks may reenter admission; map iterators remain valid.
std::map<std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>, pending_trade> pending;
size_t notifying = 0;

bool player_pending(uint32_t pid)
{
	return std::any_of(pending.begin(), pending.end(),
			   [pid](const auto &entry) { return entry.second.player_pid == pid; });
}

bool publish_ownership(const shop_trade_payload &payload, const shop_trade_result &result)
{
	const item_owner_identity player = { item_owner_type::player, payload.player_pid, 0 };
	const item_owner_identity shop = { item_owner_type::shopkeeper,
					   item_shopkeeper_owner_id(payload.shop_id), 0 };
	const item_owner_identity system = { item_owner_type::system, 0, 0 };
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	item_transfer_payload transfer = {};
	if (payload.action == shop_trade_action::buy_existing)
	{
		transfer.from_owner = shop;
		transfer.to_owner = player;
		transfer.reason = item_transfer_reason::shop_buy;
	}
	else if (payload.action == shop_trade_action::buy_produced)
	{
		transfer.from_owner = system;
		transfer.to_owner = player;
		transfer.reason = item_transfer_reason::creation;
	}
	else if (payload.action == shop_trade_action::sell_store)
	{
		transfer.from_owner = player;
		transfer.to_owner = shop;
		transfer.reason = item_transfer_reason::shop_sell;
	}
	else if (payload.action == shop_trade_action::sell_destroy)
	{
		transfer.from_owner = player;
		transfer.to_owner = destruction;
		transfer.reason = item_transfer_reason::destruction;
	}
	else if (payload.action == shop_trade_action::discard_invalid)
	{
		transfer.from_owner = shop;
		transfer.to_owner = destruction;
		transfer.reason = item_transfer_reason::destruction;
	}
	else
		return false;
	transfer.reason_id = payload.shop_id;
	transfer.selected_item_uid = payload.selected_item_uid;
	transfer.target_root_item_uid = payload.target_root_item_uid ?
						payload.target_root_item_uid :
						payload.selected_item_uid;
	transfer.target_parent_item_uid = payload.target_parent_item_uid;
	transfer.expected_target_parent_revision = payload.expected_target_parent_revision;
	transfer.item_count = payload.item_count;
	for (size_t index = 0; index < payload.item_count; ++index)
		transfer.items[index] = { payload.items[index].item_uid,
					  payload.items[index].root_item_uid,
					  payload.items[index].parent_item_uid,
					  payload.items[index].expected_item_revision,
					  payload.items[index].vnum,
					  payload.items[index].expected_state };
	const bool buying = payload.action == shop_trade_action::buy_existing ||
			    payload.action == shop_trade_action::buy_produced;
	const bool cleanup = payload.action == shop_trade_action::discard_invalid;
	item_transfer_result transfer_result = {
		.root_item_uid = payload.selected_item_uid,
		.item_count = result.item_count,
		.from_owner_revision = cleanup ? result.player_owner_revision :
				       buying  ? result.counterparty_owner_revision :
						 result.player_owner_revision,
		.to_owner_revision = cleanup ? result.counterparty_owner_revision :
				     buying  ? result.player_owner_revision :
					       result.counterparty_owner_revision,
		.max_item_revision = 0,
		.corpse_revision = 0,
	};
	for (size_t index = 0; index < result.item_count; ++index)
		transfer_result.max_item_revision =
			std::max(transfer_result.max_item_revision, result.item_revisions[index]);
	return item_ownership_runtime_apply(transfer, transfer_result);
}

bool same_receipt(const critical_completion &left, const critical_completion &right)
{
	const auto committed = [](critical_apply_outcome outcome)
	{
		return outcome == critical_apply_outcome::applied ||
		       outcome == critical_apply_outcome::already_applied;
	};
	const bool same_committed_result = committed(left.outcome) && committed(right.outcome);
	return left.disposition == right.disposition &&
	       (same_committed_result ||
		(left.outcome == right.outcome && left.attempt == right.attempt)) &&
	       left.durable_revision == right.durable_revision &&
	       left.error_code == right.error_code && left.failure_stage == right.failure_stage &&
	       left.result_size == right.result_size && left.result_payload == right.result_payload;
}

// Schema2 native delivery metadata can differ; the first verified original
// canonical result remains sealed, including its full fixed payload tail.
bool same_native_receipt(const critical_completion &left, const critical_completion &right)
{
	auto committed = [](critical_apply_outcome value)
	{
		return value == critical_apply_outcome::applied ||
		       value == critical_apply_outcome::already_applied;
	};
	return left.operation_id.bytes == right.operation_id.bytes &&
	       left.disposition == right.disposition &&
	       ((committed(left.outcome) && committed(right.outcome)) ||
		left.outcome == right.outcome) &&
	       left.durable_revision == right.durable_revision &&
	       left.error_code == right.error_code && left.failure_stage == right.failure_stage &&
	       left.result_size == right.result_size && left.result_payload == right.result_payload;
}

bool committed_result_matches(const pending_trade &entry, const shop_trade_result &result)
{
	const auto &payload = entry.payload;
	if (result.action != payload.action || result.item_count != payload.item_count ||
	    entry.completed.error_code || payload.expected_shop_revision == UINT64_MAX ||
	    result.shop_revision != payload.expected_shop_revision + 1)
		return false;
	if (result.keeper_cash_recorded)
	{
		int64_t expected_cash = payload.expected_keeper_cash;
		if (payload.action == shop_trade_action::buy_existing ||
		    payload.action == shop_trade_action::buy_produced)
			expected_cash += payload.price;
		else if (payload.action != shop_trade_action::discard_invalid &&
			 expected_cash >= payload.price)
			expected_cash -= payload.price;
		if (result.keeper_cash != expected_cash)
			return false;
	}
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &item = payload.items[index];
		const uint64_t expected = item.expected_item_revision ==
							  ITEM_TRANSFER_ABSENT_REVISION ?
						  1 :
						  item.expected_item_revision + 1;
		if (result.item_uids[index] != item.item_uid || !expected ||
		    result.item_revisions[index] != expected)
			return false;
	}
	return true;
}

bool publish(decltype(pending)::iterator found, P_char character)
{
	pending_trade &entry = found->second;
	if (entry.blocked || entry.publishing)
		return false;
	if (entry.preparation)
	{
		if (!entry.preparation->submission_started || !entry.preparation->command)
			return false;
		if (entry.completed.disposition == critical_completion_disposition::never_admitted)
		{
			if (!entry.never_admitted_retired &&
			    !shop_trade_native_publication_owner::retire_never_admitted(
				    *entry.preparation->command, entry.completed))
				return false;
			entry.never_admitted_retired = true;
			entry.preparation.reset();
		}
		else if (!entry.native_acknowledged)
		{
			const critical_completion original = entry.completed;
			entry.publishing = true;
			const bool acknowledged =
				shop_trade_native_publication_owner::publish_retained(
					*entry.preparation->command, original, &entry);
			entry.publishing = false;
			if (!acknowledged || entry.blocked ||
			    !same_native_receipt(original, entry.completed))
				return false;
			entry.native_acknowledged = true;
		}
	}
	if (!critical_completion_disposition_valid(entry.completed))
	{
		entry.blocked = true;
		return false;
	}
	const bool committed = entry.completed.outcome == critical_apply_outcome::applied ||
			       entry.completed.outcome == critical_apply_outcome::already_applied;
	if (!committed && entry.completed.outcome != critical_apply_outcome::terminal_failure)
		return false;
	shop_trade_result result = {};
	const bool decoded = shop_trade_command_decode_result(entry.completed.result_payload.data(),
							      entry.completed.result_size, &result);
	if (committed && (!decoded || !committed_result_matches(entry, result)))
	{
		entry.blocked = true;
		return false;
	}
	const bool legacy_committed = committed && !entry.preparation;
	entry.publishing = true;
	try
	{
		if (legacy_committed && !entry.revision_published &&
		    !shop_trade_runtime_can_advance(entry.payload.shop_id,
						    entry.payload.expected_shop_revision,
						    result.shop_revision))
		{
			entry.publishing = false;
			return false;
		}
		if (decoded && !entry.preparation &&
		    (!entry.balances_published ||
		     entry.balance_runtime_id != character->runtime_id))
		{
			const uint64_t runtime_id = character->runtime_id;
			if (!currency_transaction_publish_balances(
				    character, entry.payload.account_name.data(),
				    entry.payload.racewar, result.wallet, result.bank,
				    result.wallet_revision, result.bank_revision))
			{
				entry.publishing = false;
				return false;
			}
			entry.balances_published = true;
			entry.balance_runtime_id = runtime_id;
		}
		if (entry.blocked)
		{
			entry.publishing = false;
			return false;
		}
		if (legacy_committed && !entry.custody_published)
		{
			if (!publish_ownership(entry.payload, result))
			{
				entry.publishing = false;
				return false;
			}
			entry.custody_published = true;
		}
		if (entry.blocked)
		{
			entry.publishing = false;
			return false;
		}
		if (legacy_committed && !entry.revision_published)
		{
			if (!shop_trade_runtime_advance(entry.payload.shop_id,
							entry.payload.expected_shop_revision,
							result.shop_revision))
			{
				entry.publishing = false;
				return false;
			}
			entry.revision_published = true;
		}
		if (entry.blocked)
		{
			entry.publishing = false;
			return false;
		}
		if (legacy_committed && !entry.physical_published)
		{
			P_char current = find_player_by_pid(entry.player_pid);
			if (!current || current != character ||
			    current->runtime_id != entry.balance_runtime_id)
			{
				entry.publishing = false;
				return false;
			}
			if (entry.publication &&
			    !entry.publication(character, result, entry.payload,
					       entry.physical_stages))
			{
				entry.publishing = false;
				return false;
			}
			entry.physical_published = true;
		}
		if (entry.blocked)
		{
			entry.publishing = false;
			return false;
		}
	}
	catch (...)
	{
		entry.publishing = false;
		return false;
	}
	entry.publishing = false;
	if (entry.preparation)
	{
		character = find_character_by_runtime_id(entry.preparation->actor_runtime_id);
		if (!character || !IS_PC(character) || !character->only.pc ||
		    static_cast<uint32_t>(GET_PID(character)) != entry.player_pid)
			return false;
	}
	// Own the original context independently while a successful notification
	// starts another purchase. A thrown notification is retained but never
	// automatically repeated: it may already have advanced that continuation.
	auto node = pending.extract(found);
	++notifying;
	try
	{
		const auto &finished = node.mapped();
		if (finished.completion)
			finished.completion(character, committed, result,
					    finished.completed.error_code, finished.payload);
	}
	catch (...)
	{
		node.mapped().blocked = true;
		pending.insert(std::move(node));
		--notifying;
		return false;
	}
	--notifying;
	return true;
}

#ifndef __NO_MYSQL__
bool shop_remove_selected(const std::vector<player_item_snapshot> &input,
			  const shop_trade_payload &payload,
			  std::vector<player_item_snapshot> *output)
{
	std::set<uint64_t> selected;
	for (size_t index = 0; index < payload.item_count; ++index)
		selected.insert(payload.items[index].item_uid);
	std::vector<player_item_snapshot> result;
	std::vector<int32_t> positions(input.size(), PLAYER_SNAPSHOT_NO_PARENT);
	for (size_t index = 0; index < input.size(); ++index)
	{
		const auto &item = input[index];
		if (selected.count(item.object_uid))
			continue;
		if (item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
		    item.parent_index >= static_cast<int32_t>(index) ||
		    (item.parent_index >= 0 && positions[item.parent_index] < 0))
			return false;
		positions[index] = static_cast<int32_t>(result.size());
		result.push_back(item);
		if (item.parent_index >= 0)
			result.back().parent_index = positions[item.parent_index];
	}
	*output = std::move(result);
	return true;
}

bool shop_image_matches(const std::vector<player_item_snapshot> &forest,
			const shop_item_runtime_image &image)
{
	if (forest.size() != image.size())
		return false;
	std::vector<uint64_t> roots(forest.size());
	for (size_t index = 0; index < forest.size(); ++index)
	{
		const auto &item = forest[index];
		if (item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
		    item.parent_index >= static_cast<int32_t>(index))
			return false;
		roots[index] = item.parent_index < 0 ? item.object_uid : roots[item.parent_index];
		const auto found = image.find(item.object_uid);
		if (found == image.end() || !found->second.payload_present ||
		    found->second.root_uid != roots[index] ||
		    found->second.slot != item.equipment_slot)
			return false;
		const uint64_t parent_id =
			item.parent_index < 0 ? 0 :
						image.at(forest[item.parent_index].object_uid).id;
		if (found->second.parent_id != parent_id)
			return false;
		auto expected = item;
		expected.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		std::vector<uint8_t> a, b;
		if (player_item_snapshot_list_encode({ expected }, &a) !=
			    player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_encode({ found->second.item }, &b) !=
			    player_snapshot_codec_result::ok ||
		    a != b)
			return false;
	}
	return true;
}

bool shop_actor_matches(P_char actor, P_char keeper, const pending_trade &entry)
{
	if (!entry.preparation || !actor || !IS_PC(actor) || !actor->only.pc || !keeper ||
	    !IS_NPC(keeper) || static_cast<uint32_t>(GET_PID(actor)) != entry.player_pid ||
	    actor->runtime_id != entry.preparation->actor_runtime_id ||
	    keeper->runtime_id != entry.preparation->keeper_runtime_id ||
	    GET_VNUM(keeper) != entry.payload.keeper_vnum ||
	    static_cast<uint32_t>(GET_LEVEL(actor)) != entry.payload.expected_player_level ||
	    GET_RACEWAR(actor) != entry.payload.racewar || !shop_index ||
	    entry.payload.shop_id >= static_cast<uint32_t>(number_of_shops) ||
	    GET_RNUM(keeper) != shop_index[entry.payload.shop_id].keeper ||
	    (shop_index[entry.payload.shop_id].shop_is_roaming != 0) !=
		    (entry.payload.keeper_roaming != 0))
		return false;
	const char *account = get_account_name_safe(actor);
	return account && !strcmp(account, entry.payload.account_name.data());
}

bool preparation_destination_tree(P_obj, std::vector<uint8_t> *, std::vector<uint64_t> * = nullptr,
				  size_t *estimated_bytes = nullptr);

bool shop_world(pending_trade &entry, const std::vector<player_item_snapshot> &player,
		const std::vector<player_item_snapshot> &keeper,
		const std::vector<player_item_snapshot> &detached,
		const economic_sql_shop_trade_publication *native, bool final,
		shop_trade_world_witness *output)
{
	std::vector<shop_trade_world_uid_expectation> locations;
	std::set<uint64_t> included;
	const auto append = [&](const auto &forest, bool is_keeper, bool is_detached,
				const shop_item_runtime_image *image) -> bool
	{
		for (size_t index = 0; index < forest.size(); ++index)
		{
			const auto &item = forest[index];
			if (!included.insert(item.object_uid).second || item.parent_index < -1 ||
			    item.parent_index >= static_cast<int32_t>(index))
				return false;
			const uint64_t parent =
				item.parent_index < 0 ? 0 : forest[item.parent_index].object_uid;
			int32_t row_id = -1;
			if (image)
			{
				const auto found = image->find(item.object_uid);
				if (found == image->end() || !found->second.id ||
				    found->second.id > INT_MAX)
					return false;
				row_id = static_cast<int32_t>(found->second.id);
			}
			const auto location =
				parent	    ? shop_trade_world_location::inside :
				is_detached ? shop_trade_world_location::detached :
				is_keeper   ? (item.equipment_slot ?
						       shop_trade_world_location::keeper_equipment :
						       shop_trade_world_location::keeper_inventory) :
					      (item.equipment_slot ?
						       shop_trade_world_location::actor_equipment :
						       shop_trade_world_location::actor_inventory);
			locations.push_back(
				{ item.object_uid, location, parent, item.equipment_slot, row_id });
		}
		return true;
	};
	if (!append(player, false, false, native ? &native->whole_player_items : nullptr) ||
	    !append(keeper, true, false, native ? &native->keeper_items : nullptr) ||
	    !append(detached, false, true, nullptr))
		return false;
	if (final)
		for (size_t index = 0; index < entry.payload.item_count; ++index)
			if (!included.count(entry.payload.items[index].item_uid))
				locations.push_back({ entry.payload.items[index].item_uid,
						      shop_trade_world_location::absent, 0, 0,
						      -1 });
	std::vector<uint64_t> literal;
	if (std::any_of(player.begin(), player.end(), [&](const auto &item)
			{ return item.object_uid == entry.payload.selected_item_uid; }))
		literal.push_back(entry.payload.selected_item_uid);
	shop_trade_world_expectation expected{};
	expected.actor_pid = entry.player_pid;
	expected.actor_runtime_id = entry.preparation->actor_runtime_id;
	expected.keeper_runtime_id = entry.preparation->keeper_runtime_id;
	expected.shop_id = entry.payload.shop_id;
	expected.keeper_vnum = entry.payload.keeper_vnum;
	expected.player_items = player;
	expected.keeper_items = keeper;
	expected.detached_items = detached;
	expected.literal_player_root_uids = literal;
	expected.uid_locations = locations;
	return shop_trade_world_witness_observe(expected, output) &&
	       shop_actor_matches(output->actor, output->keeper, entry);
}

P_obj shop_witness_object(const shop_trade_world_witness &witness, uint64_t uid)
{
	if (!uid)
		return nullptr;
	const auto found = std::find_if(witness.objects.begin(), witness.objects.end(),
					[uid](P_obj object)
					{ return object && object->obj_uid == uid; });
	return found == witness.objects.end() ? nullptr : *found;
}
#endif

} // namespace

bool shop_trade_native_publication_owner::expected_player_forest(
	const shop_trade_payload &payload, const std::vector<player_item_snapshot> &original,
	bool rejected, std::vector<player_item_snapshot> *output) noexcept
{
	if (!output || original.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return false;
	try
	{
		std::vector<player_item_snapshot> selected;
		if (!shop_trade_accounted_after_items(payload, &selected))
			return false;
		std::map<uint64_t, size_t> original_indices;
		std::vector<uint64_t> original_roots(original.size());
		for (size_t index = 0; index < original.size(); ++index)
		{
			const auto &item = original[index];
			if (!item.object_uid || item.object_uid == UINT64_MAX ||
			    !original_indices.emplace(item.object_uid, index).second ||
			    item.parent_index < PLAYER_SNAPSHOT_NO_PARENT ||
			    item.parent_index >= static_cast<int32_t>(index))
				return false;
			original_roots[index] = item.parent_index < 0 ?
							item.object_uid :
							original_roots[item.parent_index];
		}
		std::set<uint64_t> selected_uids;
		for (const auto &item : selected)
			if (!selected_uids.insert(item.object_uid).second)
				return false;
		const bool selling = payload.action == shop_trade_action::sell_store ||
				     payload.action == shop_trade_action::sell_destroy;
		const bool buying = payload.action == shop_trade_action::buy_existing ||
				    payload.action == shop_trade_action::buy_produced;
		if (!selling && !buying && payload.action != shop_trade_action::discard_invalid)
			return false;
		for (const auto &item : selected)
		{
			const auto found = original_indices.find(item.object_uid);
			if (!selling)
			{
				if (found != original_indices.end())
					return false;
				continue;
			}
			if (found == original_indices.end())
				return false;
			const size_t index = found->second;
			const auto &before = original[index];
			const uint64_t parent_uid =
				before.parent_index < 0 ? 0 :
							  original[before.parent_index].object_uid;
			const auto expected = std::find_if(
				payload.items.begin(), payload.items.begin() + payload.item_count,
				[&](const auto &entry)
				{ return entry.item_uid == item.object_uid; });
			if (expected == payload.items.begin() + payload.item_count ||
			    original_roots[index] != expected->root_item_uid ||
			    parent_uid != expected->parent_item_uid || before.equipment_slot)
				return false;
			auto before_literal = before;
			auto selected_literal = item;
			before_literal.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			selected_literal.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			std::vector<uint8_t> before_bytes, selected_bytes;
			if (player_item_snapshot_list_encode({ before_literal }, &before_bytes) !=
				    player_snapshot_codec_result::ok ||
			    player_item_snapshot_list_encode({ selected_literal },
							     &selected_bytes) !=
				    player_snapshot_codec_result::ok ||
			    before_bytes != selected_bytes)
				return false;
		}
		std::vector<player_item_snapshot> candidate;
		std::vector<int32_t> retained_indices(original.size(), PLAYER_SNAPSHOT_NO_PARENT);
		candidate.reserve(original.size());
		for (size_t index = 0; index < original.size(); ++index)
		{
			const auto &item = original[index];
			if (!rejected && selling && selected_uids.count(item.object_uid))
				continue;
			if (item.parent_index >= 0 && retained_indices[item.parent_index] < 0)
				return false; // Never leave an unselected child of a removed source.
			retained_indices[index] = static_cast<int32_t>(candidate.size());
			candidate.push_back(item);
			if (item.parent_index >= 0)
				candidate.back().parent_index = retained_indices[item.parent_index];
		}
		if (!rejected && buying)
		{
			if (selected.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - candidate.size())
				return false;
			int32_t target_parent = PLAYER_SNAPSHOT_NO_PARENT;
			if (payload.target_parent_item_uid)
			{
				const auto found =
					original_indices.find(payload.target_parent_item_uid);
				if (found == original_indices.end() ||
				    original_roots[found->second] != payload.target_root_item_uid)
					return false;
				target_parent = retained_indices[found->second];
				if (target_parent < 0)
					return false;
				if (payload.native_destination_weight_recorded)
				{
					if (!shop_trade_destination_weight_verify(
						    candidate[target_parent],
						    selected.front().weight,
						    payload.destination_weight))
						return false;
					candidate[target_parent].weight =
						payload.destination_weight.after;
				}
			}
			const size_t base = candidate.size();
			for (const auto &item : selected)
			{
				candidate.push_back(item);
				candidate.back().parent_index =
					item.parent_index < 0 ?
						target_parent :
						static_cast<int32_t>(base + item.parent_index);
			}
		}
		std::vector<uint8_t> canonical;
		if (player_item_snapshot_list_encode(candidate, &canonical) !=
			    player_snapshot_codec_result::ok ||
		    canonical.size() > PLAYER_SNAPSHOT_MAX_BYTES - sizeof(uint32_t))
			return false;
		*output = std::move(candidate);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_native_publication_owner::expected_player_order(
	const shop_trade_payload &payload, P_char actor, P_obj selected, P_obj destination,
	const std::vector<player_item_snapshot> &values,
	std::vector<player_item_snapshot> *output) noexcept
{
	return shop_trade_world_expected_player_order(payload, actor, selected, destination, values,
						      output);
}

#ifndef __NO_MYSQL__
bool shop_trade_native_publication_owner::expected_keeper_forest(
	const shop_trade_payload &payload, P_char keeper, P_obj selected,
	const std::vector<player_item_snapshot> &original, bool rejected,
	std::vector<player_item_snapshot> *output) noexcept
{
	if (!output || original.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return false;
	try
	{
		auto candidate = original;
		if (!rejected && (payload.action == shop_trade_action::buy_existing ||
				  payload.action == shop_trade_action::discard_invalid))
		{
			if (!shop_remove_selected(original, payload, &candidate))
				return false;
		}
		if (!rejected && payload.action == shop_trade_action::sell_store)
		{
			std::vector<player_item_snapshot> selected_after;
			if (!shop_trade_accounted_after_items(payload, &selected_after) ||
			    selected_after.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - candidate.size())
				return false;
			const auto base = static_cast<int32_t>(candidate.size());
			for (auto item : selected_after)
			{
				if (item.parent_index >= 0)
					item.parent_index += base;
				candidate.push_back(std::move(item));
			}
			std::vector<player_item_snapshot> ordered;
			if (!shop_trade_world_expected_keeper_order(payload, keeper, selected,
								    candidate, &ordered))
				return false;
			candidate = std::move(ordered);
		}
		std::vector<uint8_t> canonical;
		if (player_item_snapshot_list_encode(candidate, &canonical) !=
			    player_snapshot_codec_result::ok ||
		    canonical.size() > PLAYER_SNAPSHOT_MAX_BYTES - sizeof(uint32_t) ||
		    !shop_trade_world_player_values_supported(candidate))
			return false;
		*output = std::move(candidate);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_native_publication_owner::expected_destination_forest(
	const shop_trade_payload &payload, P_char actor, P_obj selected, P_obj destination,
	std::span<const uint8_t> original, std::vector<player_item_snapshot> *output) noexcept
{
	if (!output || !destination || !payload.target_parent_item_uid ||
	    !payload.native_destination_weight_recorded)
		return false;
	try
	{
		std::vector<player_item_snapshot> literal, selected_after, ordered;
		if (player_item_snapshot_list_decode(original.data(), original.size(), &literal) !=
			    player_snapshot_codec_result::ok ||
		    literal.empty() ||
		    literal.front().object_uid != payload.target_parent_item_uid ||
		    !shop_trade_accounted_after_items(payload, &selected_after) ||
		    selected_after.empty() ||
		    !shop_trade_destination_weight_verify(literal.front(),
							  selected_after.front().weight,
							  payload.destination_weight) ||
		    selected_after.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - literal.size())
			return false;
		literal.front().weight = payload.destination_weight.after;
		const auto base = static_cast<int32_t>(literal.size());
		for (auto item : selected_after)
		{
			item.parent_index = item.parent_index < 0 ? 0 : base + item.parent_index;
			literal.push_back(std::move(item));
		}
		if (!expected_player_order(payload, actor, selected, destination, literal,
					   &ordered))
			return false;
		*output = std::move(ordered);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_native_publication_owner::verified_current_receipt(
	MYSQL *connection, const critical_command &command, const critical_completion &sealed,
	const economic_sql_shop_trade_publication &current) noexcept
{
	try
	{
		const bool success = sealed.outcome == critical_apply_outcome::applied ||
				     sealed.outcome == critical_apply_outcome::already_applied;
		// SQL identities are uint64 values, while live obj_data::db_item_id
		// is a signed int. Reject an unrepresentable complete native cut before
		// any future physical row-ID assignment, callback or ACK proof.
		const auto native_rows_fit = [](const shop_item_runtime_image &image)
		{
			return std::all_of(image.begin(), image.end(),
					   [](const auto &entry)
					   {
						   const auto &row = entry.second;
						   return row.id && row.id <= INT_MAX &&
							  row.parent_id <= INT_MAX;
					   });
		};
		if (!native_rows_fit(current.whole_player_items) ||
		    !native_rows_fit(current.keeper_items) ||
		    !native_rows_fit(current.player_items))
			return false;
		const auto authentic = critical_command_repository_verify_shop_trade_in_transaction(
			connection, command);
		const bool authentic_success =
			authentic.outcome == critical_apply_outcome::applied ||
			authentic.outcome == critical_apply_outcome::already_applied;
		if ((success ? !authentic_success : authentic.outcome != sealed.outcome) ||
		    authentic.error_code != sealed.error_code ||
		    authentic.failure_stage != sealed.failure_stage ||
		    authentic.durable_revision != sealed.durable_revision ||
		    authentic.result_size != sealed.result_size ||
		    authentic.result_payload != sealed.result_payload || !connection ||
		    mysql_thread_id(connection) != current.session_id ||
		    !(connection->server_status & SERVER_STATUS_IN_TRANS))
			return false;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_native_publication_owner::current_native_image(
	MYSQL *connection, const critical_command &command, const critical_completion &sealed,
	economic_sql_shop_trade_publication *output) noexcept
{
	if (!output || !nevent_is_game_thread() ||
	    sealed.disposition != critical_completion_disposition::execution ||
	    sealed.operation_id.bytes != command.operation_id.bytes ||
	    command.payload_version != SHOP_TRADE_RECOVERY_MANIFEST_VERSION)
		return false;
	try
	{
		shop_trade_payload payload{};
		if (!shop_trade_command_decode_payload(command, &payload) ||
		    !payload.recovery_manifest_recorded)
			return false;
		economic_sql_shop_trade_publication current;
		if (economic_sql_shop_trade_lock_publication(connection, command, sealed,
							     &current) ||
		    !verified_current_receipt(connection, command, sealed, current))
			return false;
		*output = std::move(current);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_native_publication_owner::current_native_image(
	MYSQL *connection, const critical_command &command, const critical_completion &sealed,
	const player_shop_checkpoint_token &token,
	economic_sql_shop_trade_publication *output) noexcept
{
	if (!output || !nevent_is_game_thread() ||
	    sealed.disposition != critical_completion_disposition::execution ||
	    sealed.operation_id.bytes != command.operation_id.bytes)
		return false;
	try
	{
		shop_trade_payload payload{};
		std::vector<player_item_snapshot> original, expected;
		player_shop_checkpoint_stage stage{};
		const bool success = sealed.outcome == critical_apply_outcome::applied ||
				     sealed.outcome == critical_apply_outcome::already_applied;
		if ((!success && sealed.outcome != critical_apply_outcome::terminal_failure) ||
		    !shop_trade_command_decode_payload(command, &payload) ||
		    !player_save_shop_checkpoint_owner::original_held_body(token, command,
									   &original, &stage) ||
		    stage.save_revision != payload.expected_player_save_revision ||
		    stage.level != payload.expected_player_level ||
		    !expected_player_forest(payload, original, !success, &expected))
			return false;
		economic_sql_shop_trade_publication current;
		if (economic_sql_shop_trade_lock_publication(connection, command, sealed, expected,
							     &current))
			return false;
		if (payload.recovery_manifest_recorded)
		{
			std::vector<uint8_t> original_bytes;
			std::vector<player_item_snapshot> player_cut, keeper_cut;
			const auto &manifest = payload.recovery_manifest;
			if (player_item_snapshot_list_encode(original, &original_bytes) !=
				    player_snapshot_codec_result::ok ||
			    !shop_trade_recovery_forest_verify(
				    original_bytes, shop_trade_recovery_forest_role::player_before,
				    manifest.player_before) ||
			    !shop_trade_recovery_image_reconstruct(
				    success ? manifest.player_after : manifest.player_before,
				    success ? shop_trade_recovery_forest_role::player_after :
					      shop_trade_recovery_forest_role::player_before,
				    current.whole_player_items, &player_cut) ||
			    !shop_trade_recovery_image_reconstruct(
				    success ? manifest.keeper_after : manifest.keeper_before,
				    success ? shop_trade_recovery_forest_role::keeper_after :
					      shop_trade_recovery_forest_role::keeper_before,
				    current.keeper_items, &keeper_cut))
				return false;
		}
		if (!verified_current_receipt(connection, command, sealed, current))
			return false;
		*output = std::move(current);
		return true;
	}
	catch (...)
	{
		return false;
	}
}
#endif

bool shop_trade_native_publication_owner::native_publish(const critical_command &command,
							 const critical_completion &sealed,
							 void *opaque) noexcept
{
#ifdef __NO_MYSQL__
	(void)command;
	(void)sealed;
	(void)opaque;
	return false;
#else
	if (!opaque || !nevent_is_game_thread())
		return false;
	auto &entry = *static_cast<pending_trade *>(opaque);
	if (!entry.preparation || !entry.preparation->command || entry.blocked ||
	    !same_native_receipt(entry.completed, sealed) ||
	    command.operation_id.bytes != sealed.operation_id.bytes || !entry.accounted_publication)
		return false;
	try
	{
		auto &prepared = *entry.preparation;
		MYSQL *connection = sql_pool_acquire();
		player_sql_pool_lease lease(connection);
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup cleanup;
		player_sql_transaction_cleanup transaction(connection, cleanup);
		transaction.starting();
		bool proven = false;
		try
		{
			if (mysql_real_query(connection, "START TRANSACTION", 17))
				throw EIO;
			economic_sql_shop_trade_publication current;
			if (!current_native_image(connection, command, sealed,
						  prepared.player_token, &current))
				throw EAGAIN;
			entry.native_receipt_verified = true;
			const bool success = sealed.outcome == critical_apply_outcome::applied ||
					     sealed.outcome ==
						     critical_apply_outcome::already_applied;
			std::vector<player_item_snapshot> original, source;
			player_shop_checkpoint_stage held{};
			if (!player_save_shop_checkpoint_owner::original_held_body(
				    prepared.player_token, command, &original, &held) ||
			    held.save_revision != entry.payload.expected_player_save_revision ||
			    held.level != entry.payload.expected_player_level ||
			    player_item_snapshot_list_decode(
				    entry.payload.item_blob.data(), entry.payload.item_blob_size,
				    &source) != player_snapshot_codec_result::ok)
				throw EAGAIN;
			const bool produced = entry.payload.action ==
					      shop_trade_action::buy_produced;
			const bool buying = produced ||
					    entry.payload.action == shop_trade_action::buy_existing;
			const bool selling =
				entry.payload.action == shop_trade_action::sell_store ||
				entry.payload.action == shop_trade_action::sell_destroy;
			const bool cleanup_action = entry.payload.action ==
						    shop_trade_action::discard_invalid;
			std::vector<player_item_snapshot> staged =
				produced ? source : std::vector<player_item_snapshot>{};
			shop_trade_world_witness observed;
			if (!entry.publication_forests_ready)
			{
				// This original full BEFORE census is indispensable. A matching
				// target graph alone cannot invent completed native handler tails.
				if (!shop_world(entry, original, prepared.keeper_items, staged,
						nullptr, false, &observed))
					throw EAGAIN;
				P_obj selected = shop_witness_object(
					observed, entry.payload.selected_item_uid);
				P_obj destination = shop_witness_object(
					observed, entry.payload.target_parent_item_uid);
				if (entry.payload.native_destination_weight_recorded && destination)
				{
					std::vector<uint8_t> literal;
					if (!preparation_destination_tree(destination, &literal) ||
					    literal != prepared.destination)
						throw EAGAIN;
				}
				std::vector<player_item_snapshot> player, keeper, selected_after;
				if (!selected ||
				    !expected_player_forest(entry.payload, original, !success,
							    &player) ||
				    !shop_trade_accounted_after_items(entry.payload,
								      &selected_after))
					throw EAGAIN;
				if (!expected_keeper_forest(entry.payload, observed.keeper,
							    selected, prepared.keeper_items,
							    !success, &keeper))
					throw EAGAIN;
				if (success && buying)
				{
					std::vector<player_item_snapshot> ordered;
					if (!expected_player_order(entry.payload, observed.actor,
								   selected, destination, player,
								   &ordered))
						throw EAGAIN;
					player = std::move(ordered);
				}
				if (!shop_image_matches(player, current.whole_player_items) ||
				    !shop_image_matches(keeper, current.keeper_items))
					throw EAGAIN;
				if (success && entry.payload.native_destination_weight_recorded &&
				    destination)
				{
					std::vector<player_item_snapshot> literal;
					if (!expected_destination_forest(
						    entry.payload, observed.actor, selected,
						    destination, prepared.destination, &literal) ||
					    player_item_snapshot_list_encode(
						    literal, &entry.after_destination) !=
						    player_snapshot_codec_result::ok)
						throw EAGAIN;
				}
				// All allocation-backed retained copies precede callbacks.
				entry.before_player = std::move(original);
				entry.after_player = std::move(player);
				entry.after_keeper = std::move(keeper);
				entry.publication_forests_ready = true;
			}
			else if (!shop_image_matches(entry.after_player,
						     current.whole_player_items) ||
				 !shop_image_matches(entry.after_keeper, current.keeper_items))
				throw EAGAIN;
			const auto destination_matches = [&](bool after)
			{
				if (!entry.payload.native_destination_weight_recorded ||
				    !entry.payload.target_parent_item_uid)
					return true;
				P_obj destination = shop_witness_object(
					observed, entry.payload.target_parent_item_uid);
				std::vector<uint8_t> literal;
				const auto &expected = after ? entry.after_destination :
							       prepared.destination;
				return destination && !expected.empty() &&
				       preparation_destination_tree(destination, &literal) &&
				       literal == expected;
			};
			const auto observe_final = [&](bool row_ids)
			{
				return shop_world(entry, entry.after_player, entry.after_keeper,
						  (!success && produced) ?
							  staged :
							  std::vector<player_item_snapshot>{},
						  row_ids ? &current : nullptr, true, &observed) &&
				       destination_matches(success);
			};
			if (entry.physical_published || (entry.physical_stages & 32U) ||
			    (success && (entry.physical_stages & 4096U) &&
			     (entry.physical_stages & 131072U)))
			{
				if (!observe_final(false))
					throw EAGAIN;
			}
			else if (!shop_world(entry, entry.before_player, prepared.keeper_items,
					     staged, nullptr, false, &observed))
			{
				// A normally returned departure can resume placement. Incomplete
				// started handler tails remain held by the physical callback's
				// explicit stage checks; topology cannot discharge them.
				std::vector<player_item_snapshot> player = entry.before_player;
				std::vector<player_item_snapshot> keeper = prepared.keeper_items;
				if (selling &&
				    !shop_remove_selected(player, entry.payload, &player))
					throw EAGAIN;
				if ((buying && !produced) || cleanup_action)
					if (!shop_remove_selected(keeper, entry.payload, &keeper))
						throw EAGAIN;
				auto detached = source;
				if (buying && (entry.physical_stages & 65536U))
					detached.front().extra2_flags |= ITEM2_STOREITEM;
				if (buying && (entry.physical_stages & 1024U) &&
				    !shop_trade_accounted_after_items(entry.payload, &detached))
					throw EAGAIN;
				bool resumed = (entry.physical_stages & 65536U) &&
					       shop_world(entry, player, keeper, detached, nullptr,
							  false, &observed);
				if (!resumed && buying && entry.payload.target_parent_item_uid &&
				    (entry.physical_stages & 1024U))
				{
					// obj_to_char returned before the existing nesting leg. Its
					// temporary carried-root placement has the same native grouping
					// policy; never pretend it is the final container placement.
					auto temporary = entry.payload;
					temporary.target_parent_item_uid = 0;
					temporary.target_root_item_uid =
						temporary.selected_item_uid;
					temporary.expected_target_parent_revision = 0;
					temporary.destination_weight = {};
					std::vector<player_item_snapshot> values, ordered;
					P_char actor = find_character_by_runtime_id(
						prepared.actor_runtime_id);
					P_obj selected = nullptr;
					// Bounded temporary carrying observation resolves the pointer;
					// the subsequent full world census must still prove uniqueness.
					if (expected_player_forest(temporary, entry.before_player,
								   false, &values))
					{
						// The ordering helper checks the complete target chain;
						// the following full census must prove every alias surface.
						std::set<P_obj> seen;
						for (P_obj object = actor ? actor->carrying :
									    nullptr;
						     object; object = object->next_content)
						{
							if (seen.size() >=
								    PLAYER_SNAPSHOT_MAX_OBJECTS ||
							    !seen.insert(object).second ||
							    !OBJ_CARRIED_BY(object, actor))
								throw EAGAIN;
							if (object->obj_uid ==
							    temporary.selected_item_uid)
							{
								selected = object;
								break;
							}
						}
						if (selected && expected_player_order(
									temporary, actor, selected,
									nullptr, values, &ordered))
							resumed = shop_world(entry, ordered, keeper,
									     {}, nullptr, false,
									     &observed);
					}
				}
				if (!resumed)
					throw EAGAIN;
			}
			// Before nesting, bind every original literal child. Normally
			// returned nesting instead requires the retained complete afterimage.
			if (!destination_matches(success && (entry.physical_stages & 4096U) &&
						 (entry.physical_stages & 131072U)))
				throw EAGAIN;
			if (!shop_actor_matches(observed.actor, observed.keeper, entry) ||
			    entry.blocked || !same_native_receipt(entry.completed, sealed) ||
			    !transaction.same_session())
				throw EAGAIN;
			shop_trade_result projected{};
			if (!shop_trade_command_decode_result(sealed.result_payload.data(),
							      sealed.result_size, &projected))
				throw EAGAIN;
			projected.wallet = current.wallet;
			projected.bank = current.bank;
			projected.wallet_revision = current.wallet_revision;
			projected.bank_revision = current.bank_revision;
			projected.keeper_cash = current.keeper_cash;
			projected.keeper_cash_recorded = true;
			projected.shop_revision = current.shop_revision;
			projected.player_owner_revision = current.player_owner_revision;
			projected.counterparty_owner_revision = current.counterparty_owner_revision;
			// Checked native placement consumes CURRENT destination custody. Publish
			// this original-session complete cut after retaining the original world
			// and ordered AFTER values, before any placement/extraction callback.
			// Keep the final full runtime/world verification below before guarded ACK.
			if (!shop_trade_current_runtime_owner::publish(connection, entry.payload,
								       current))
				throw EAGAIN;
			if (success)
			{
				// Retained old callback owns audit/notice/native handler stages.
				// It receives only current value projections, never a replacement
				// receipt; SQL authority and the original reservation remain held.
				const bool returned = entry.accounted_publication(
					observed.actor, observed.keeper, projected, entry.payload,
					entry.physical_stages);
				if (returned)
					entry.physical_published = true;
				if (!returned || entry.blocked ||
				    !same_native_receipt(entry.completed, sealed) ||
				    !observe_final(false))
					throw EAGAIN;
			}
			else if (!observe_final(false))
				throw EAGAIN;
			// Reobserve both original bodies after every callback. No raw body or
			// object pointer survives a pulse, and no historical cash is restored.
			if (!shop_actor_matches(observed.actor, observed.keeper, entry) ||
			    observed.actor->only.pc->wallet_revision > current.wallet_revision ||
			    observed.actor->only.pc->bank_revision > current.bank_revision ||
			    !currency_transaction_publish_balances(
				    observed.actor, entry.payload.account_name.data(),
				    entry.payload.racewar, current.wallet, current.bank,
				    current.wallet_revision, current.bank_revision) ||
			    entry.blocked || !same_native_receipt(entry.completed, sealed) ||
			    !observe_final(false))
				throw EAGAIN;
			int64_t cash = current.keeper_cash;
			GET_PLATINUM(observed.keeper) = static_cast<int>(cash / 1000);
			cash %= 1000;
			GET_GOLD(observed.keeper) = static_cast<int>(cash / 100);
			cash %= 100;
			GET_SILVER(observed.keeper) = static_cast<int>(cash / 10);
			GET_COPPER(observed.keeper) = static_cast<int>(cash % 10);
			// Checked signed native row IDs are assigned only to the freshly
			// censused final physical graph; no old row ID becomes custody proof.
			for (P_obj object : observed.objects)
				if (object)
				{
					const auto player_row =
						current.whole_player_items.find(object->obj_uid);
					const auto keeper_row =
						current.keeper_items.find(object->obj_uid);
					const uint64_t row_id =
						player_row != current.whole_player_items.end() ?
							player_row->second.id :
						keeper_row != current.keeper_items.end() ?
							keeper_row->second.id :
							0;
					if (!row_id && !success && produced &&
					    std::any_of(source.begin(), source.end(),
							[&](const auto &item) {
								return item.object_uid ==
								       object->obj_uid;
							}))
						continue; // Original rejected staged output is not a native holding.
					if (!row_id || row_id > INT_MAX)
						throw EAGAIN;
					object->db_item_id = static_cast<int>(row_id);
				}
			if (!shop_trade_current_runtime_owner::publish(connection, entry.payload,
								       current) ||
			    !observe_final(true) || entry.blocked ||
			    !same_native_receipt(entry.completed, sealed) ||
			    !transaction.same_session() ||
			    mysql_thread_id(connection) != current.session_id ||
			    !(connection->server_status & SERVER_STATUS_IN_TRANS))
				throw EAGAIN;
			const auto actor = observed.actor;
			proven = actor->only.pc->wallet_revision == current.wallet_revision &&
				 actor->only.pc->bank_revision == current.bank_revision &&
				 GET_COPPER(actor) == current.wallet.amount[0] &&
				 GET_SILVER(actor) == current.wallet.amount[1] &&
				 GET_GOLD(actor) == current.wallet.amount[2] &&
				 GET_PLATINUM(actor) == current.wallet.amount[3] &&
				 current.bank.amount ==
					 std::array<int64_t, 4>{ GET_BALANCE_COPPER(actor),
								 GET_BALANCE_SILVER(actor),
								 GET_BALANCE_GOLD(actor),
								 GET_BALANCE_PLATINUM(actor) } &&
				 current.keeper_cash ==
					 static_cast<int64_t>(GET_COPPER(observed.keeper)) +
						 10LL * GET_SILVER(observed.keeper) +
						 100LL * GET_GOLD(observed.keeper) +
						 1000LL * GET_PLATINUM(observed.keeper);
		}
		catch (...)
		{
			proven = false;
		}
		transaction.finish();
		if (!cleanup.rollback_confirmed || cleanup.cleanup_error ||
		    cleanup.disposition != player_sql_cleanup_disposition::idle_verified)
			return false;
		lease.reuse(cleanup);
		return proven && !entry.blocked && same_native_receipt(entry.completed, sealed);
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool shop_trade_transaction_submit_with_publication(P_char character,
						    const shop_trade_payload &payload,
						    shop_trade_physical_publication_fn publication,
						    shop_trade_completion_fn completion)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0 ||
	    static_cast<uint32_t>(GET_PID(character)) != payload.player_pid ||
	    pending.size() + notifying >= SHOP_TRADE_PENDING_MAX ||
	    player_pending(payload.player_pid))
		return false;
	critical_operation_id operation_id = {};
	critical_command command = {};
	try
	{
		if (!critical_operation_id_generate(&operation_id) ||
		    !shop_trade_command_build(&command, operation_id, payload,
					      critical_source_site::command,
					      critical_deadline_class::interactive))
			return false;
		pending_trade entry = {};
		entry.player_pid = payload.player_pid;
		entry.payload = payload;
		entry.completion = completion;
		entry.publication = publication;
		if (!pending.emplace(operation_id.bytes, std::move(entry)).second)
			return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	try
	{
		const auto submitted = critical_command_coordinator_submit(std::move(command));
		if (!critical_submit_result_keeps_operation(submitted))
		{
			pending.erase(operation_id.bytes);
			return false;
		}
	}
	catch (...)
	{
		// Admission may already own the operation. Do not fabricate rejection.
		return true;
	}
	return true;
}

bool shop_trade_transaction_submit(P_char character, const shop_trade_payload &payload,
				   shop_trade_completion_fn completion)
{
	return shop_trade_transaction_submit_with_publication(character, payload, nullptr,
							      completion);
}

void shop_trade_transaction_handle_completions(const critical_completion *completions, size_t count)
{
	if (count && !completions)
		return;
	std::array<std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>, SHOP_TRADE_PENDING_MAX>
		attempted = {};
	size_t attempt_count = 0;
	for (size_t index = 0; index < count; ++index)
	{
		auto found = pending.find(completions[index].operation_id.bytes);
		if (found == pending.end())
			continue;
		auto &entry = found->second;
		if (entry.local_refused ||
		    (entry.preparation && !entry.preparation->submission_started))
			continue;
		const bool changed_retired_timing =
			entry.never_admitted_retired &&
			(entry.completed.queued_at_usec != completions[index].queued_at_usec ||
			 entry.completed.started_at_usec != completions[index].started_at_usec ||
			 entry.completed.completed_at_usec != completions[index].completed_at_usec);
		if (entry.completion_ready &&
		    (!(entry.native_receipt_verified ?
			       same_native_receipt(entry.completed, completions[index]) :
			       same_receipt(entry.completed, completions[index])) ||
		     changed_retired_timing))
		{
			const bool original_committed =
				entry.completed.outcome == critical_apply_outcome::applied ||
				entry.completed.outcome == critical_apply_outcome::already_applied;
			if (original_committed || entry.native_receipt_verified ||
			    entry.never_admitted_retired || entry.publishing || entry.blocked)
			{
				entry.blocked = true;
				continue;
			}
		}
		// An exact readback may say already applied; keep the first committed
		// receipt and projection stages while accepting that equivalent proof.
		if (!entry.completion_ready ||
		    (!entry.never_admitted_retired && !entry.native_receipt_verified &&
		     entry.completed.outcome != critical_apply_outcome::applied &&
		     entry.completed.outcome != critical_apply_outcome::already_applied))
			entry.completed = completions[index];
		entry.completion_ready = true;
		if (std::find(attempted.begin(), attempted.begin() + attempt_count,
			      completions[index].operation_id.bytes) !=
		    attempted.begin() + attempt_count)
			continue;
		if (attempt_count == attempted.size())
			continue;
		attempted[attempt_count++] = completions[index].operation_id.bytes;
		if (P_char character = find_player_by_pid(entry.player_pid))
			publish(found, character);
		else if (!entry.blocked && !entry.publishing && entry.preparation &&
			 !entry.never_admitted_retired && entry.preparation->submission_started &&
			 entry.preparation->command &&
			 shop_trade_native_publication_owner::retire_never_admitted(
				 *entry.preparation->command, entry.completed))
		{
			// The private owner has cancelled the exact original hold. No live
			// callback is safe here: shop_trade_completion(NULL) would skip its
			// produced-item/sequence cleanup. Retain the immutable command,
			// original payload and callback for player_ready, with no new queue.
			entry.never_admitted_retired = true;
		}
	}
}

void shop_trade_transaction_player_ready(P_char character)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0)
		return;
	// One owner per PID and one bounded attempt per invocation. Notification can
	// create a new owner; do not chase it or spin on a retained failed stage.
	auto found =
		std::find_if(pending.begin(), pending.end(),
			     [&](const auto &entry)
			     {
				     return entry.second.player_pid ==
						    static_cast<uint32_t>(GET_PID(character)) &&
					    entry.second.completion_ready;
			     });
	if (found != pending.end())
		publish(found, character);
}

bool shop_trade_transaction_player_busy(P_char character)
{
	return character && !IS_NPC(character) && GET_PID(character) > 0 &&
	       player_pending(static_cast<uint32_t>(GET_PID(character)));
}

namespace
{
#ifndef __NO_MYSQL__
bool preparation_tree_bounded(P_obj root, std::vector<uint8_t> *bytes, std::vector<uint64_t> *uids,
			      size_t max_items, size_t max_bytes, size_t *estimated_bytes)
{
	if (!root)
	{
		bytes->clear();
		return true;
	}
	std::vector<player_item_snapshot> items;
	std::vector<uint8_t> encoded;
	size_t estimated = 0;
	if (player_item_snapshot_tree_capture_literal(root, &items, &estimated) !=
		    player_snapshot_capture_result::ok ||
	    items.empty() || items.size() > max_items ||
	    player_item_snapshot_list_encode(items, &encoded) != player_snapshot_codec_result::ok ||
	    encoded.size() > max_bytes)
		return false;
	if (uids)
		for (const auto &item : items)
			uids->push_back(item.object_uid);
	*bytes = std::move(encoded);
	if (estimated_bytes)
		*estimated_bytes = estimated;
	return true;
}

bool preparation_tree(P_obj root, std::vector<uint8_t> *bytes,
		      std::vector<uint64_t> *uids = nullptr, size_t *estimated_bytes = nullptr)
{
	return preparation_tree_bounded(root, bytes, uids, SHOP_TRADE_MAX_ITEMS,
					SHOP_TRADE_ITEM_BLOB_MAX_BYTES, estimated_bytes);
}

bool preparation_destination_tree(P_obj root, std::vector<uint8_t> *bytes,
				  std::vector<uint64_t> *uids, size_t *estimated_bytes)
{
	return preparation_tree_bounded(root, bytes, uids, PLAYER_SNAPSHOT_MAX_OBJECTS,
					PLAYER_SNAPSHOT_MAX_BYTES - sizeof(uint32_t),
					estimated_bytes);
}

bool same_mapping(const economic_shop_checkpoint_projection &a,
		  const economic_shop_checkpoint_projection &b)
{
	return a.lineage.bytes == b.lineage.bytes && a.epoch.bytes == b.epoch.bytes &&
	       economic_account_key_equal(a.wallet, b.wallet) &&
	       economic_account_key_equal(a.bank, b.bank);
}

bool preparation_body(P_char actor, P_char keeper, uint32_t shop)
{
	return actor && IS_PC(actor) && actor->only.pc && GET_PID(actor) > 0 && actor->runtime_id &&
	       find_character_by_runtime_id(actor->runtime_id) == actor && keeper &&
	       IS_NPC(keeper) && keeper->runtime_id &&
	       find_character_by_runtime_id(keeper->runtime_id) == keeper && shop_index &&
	       number_of_shops > 0 && shop < static_cast<uint32_t>(number_of_shops) &&
	       GET_RNUM(keeper) == shop_index[shop].keeper && GET_VNUM(keeper) > 0;
}
#endif
}

shop_trade_preparation_state
shop_trade_preparation_owner::begin(P_char actor, P_char keeper, P_obj selected, P_obj stock,
				    P_obj destination, uint32_t shop, shop_trade_action action,
				    int64_t price, shop_trade_preparation_token *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)actor;
	(void)keeper;
	(void)selected;
	(void)stock;
	(void)destination;
	(void)shop;
	(void)action;
	(void)price;
	(void)output;
	return shop_trade_preparation_state::refused;
#else
	if (!output || !preparation_body(actor, keeper, shop) || !selected || !selected->obj_uid ||
	    price < 0 || price > INT_MAX || action <= shop_trade_action::unknown ||
	    action > shop_trade_action::sell_destroy ||
	    ((action == shop_trade_action::buy_produced) != (stock != nullptr)) ||
	    (destination && action != shop_trade_action::buy_produced) ||
	    ((action == shop_trade_action::sell_store ||
	      action == shop_trade_action::sell_destroy) &&
	     (!price || !OBJ_CARRIED_BY(selected, actor))) ||
	    (action == shop_trade_action::buy_existing && !OBJ_CARRIED_BY(selected, keeper)) ||
	    (stock && !OBJ_CARRIED_BY(stock, keeper)) ||
	    (destination && !OBJ_CARRIED_BY(destination, actor)) ||
	    pending.size() + notifying >= SHOP_TRADE_PENDING_MAX ||
	    player_pending(static_cast<uint32_t>(GET_PID(actor))) ||
	    shop_trade_transaction_keeper_busy(shop) ||
	    preparation_generation == std::numeric_limits<uint64_t>::max())
		return shop_trade_preparation_state::refused;
	critical_operation_id operation{};
	bool owns_entry = false;
	try
	{
		const char *account = get_account_name_safe(actor);
		if (!account || !strcmp(account, "Unknown") ||
		    strlen(account) > CURRENCY_ACCOUNT_NAME_MAX_BYTES ||
		    GET_RACEWAR(actor) > INT8_MAX)
			return shop_trade_preparation_state::refused;
		const int64_t keeper_cash = static_cast<int64_t>(GET_COPPER(keeper)) +
					    10LL * GET_SILVER(keeper) + 100LL * GET_GOLD(keeper) +
					    1000LL * GET_PLATINUM(keeper);
		if (GET_COPPER(keeper) < 0 || GET_SILVER(keeper) < 0 || GET_GOLD(keeper) < 0 ||
		    GET_PLATINUM(keeper) < 0 || keeper_cash < 0 || keeper_cash > INT_MAX)
			return shop_trade_preparation_state::refused;
		auto prepared = std::make_unique<trade_preparation>();
		if (!economic_gameplay_authority::observe_shop_checkpoint(
			    GET_PID(actor), account, static_cast<uint8_t>(GET_RACEWAR(actor)),
			    &prepared->mapping) ||
		    !preparation_tree(selected, &prepared->selected, &prepared->fenced_uids,
				      &prepared->selected_capture_bytes) ||
		    !preparation_tree(stock, &prepared->stock, &prepared->fenced_uids) ||
		    !preparation_destination_tree(destination, &prepared->destination,
						  &prepared->fenced_uids,
						  &prepared->destination_capture_bytes) ||
		    !shop_item_runtime_capture_keeper_literal(keeper, &prepared->keeper_items) ||
		    player_item_snapshot_list_encode(prepared->keeper_items,
						     &prepared->keeper_blob) !=
			    player_snapshot_codec_result::ok)
			return shop_trade_preparation_state::refused;
		for (const auto &item : prepared->keeper_items)
			prepared->fenced_uids.push_back(item.object_uid);
		std::sort(prepared->fenced_uids.begin(), prepared->fenced_uids.end());
		prepared->fenced_uids.erase(std::unique(prepared->fenced_uids.begin(),
							prepared->fenced_uids.end()),
					    prepared->fenced_uids.end());
		for (const auto uid : prepared->fenced_uids)
			if (shop_trade_transaction_item_busy(uid))
				return shop_trade_preparation_state::refused;
		if (!critical_operation_id_generate(&operation))
			return shop_trade_preparation_state::refused;
		prepared->generation = ++preparation_generation;
		prepared->actor_runtime_id = actor->runtime_id;
		prepared->keeper_runtime_id = keeper->runtime_id;
		const uint64_t generation = prepared->generation;
		pending_trade entry{};
		entry.player_pid = static_cast<uint32_t>(GET_PID(actor));
		entry.payload.player_pid = entry.player_pid;
		entry.payload.shop_id = shop;
		entry.payload.action = action;
		entry.payload.price = price;
		entry.payload.keeper_vnum = GET_VNUM(keeper);
		entry.payload.expected_keeper_cash = keeper_cash;
		entry.payload.keeper_roaming = shop_index[shop].shop_is_roaming != 0;
		entry.payload.selected_item_uid = selected->obj_uid;
		entry.payload.stock_item_uid = stock ? stock->obj_uid : 0;
		entry.payload.target_parent_item_uid = destination ? destination->obj_uid : 0;
		entry.payload.racewar = static_cast<uint8_t>(GET_RACEWAR(actor));
		strcpy(entry.payload.account_name.data(), account);
		entry.preparation = std::move(prepared);
		auto [found, inserted] = pending.emplace(operation.bytes, std::move(entry));
		if (!inserted)
			return shop_trade_preparation_state::refused;
		owns_entry = true;
		shop_trade_preparation_token token;
		token.operation_id_ = operation;
		token.generation_ = generation;
		*output = token;
		// Own every original body/UID before the one native shell probe. An
		// uncertain construction/extraction tail retains this same preparation.
		if (destination)
		{
			auto &owned = *found->second.preparation;
			std::vector<player_item_snapshot> target, source;
			if (player_item_snapshot_list_decode(owned.destination.data(),
							     owned.destination.size(), &target) !=
				    player_snapshot_codec_result::ok ||
			    player_item_snapshot_list_decode(owned.selected.data(),
							     owned.selected.size(), &source) !=
				    player_snapshot_codec_result::ok ||
			    target.empty() || source.empty() ||
			    target.front().type != ITEM_CONTAINER ||
			    source.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - target.size())
			{
				pending.erase(found);
				return shop_trade_preparation_state::refused;
			}
			// Native capture charges structures/dynamic rows in addition to
			// wire bytes. Two separate captures count the envelope twice.
			const size_t envelope = sizeof(player_snapshot);
			if (owned.selected_capture_bytes < envelope ||
			    owned.destination_capture_bytes < envelope ||
			    owned.destination_capture_bytes > PLAYER_SNAPSHOT_MAX_BYTES ||
			    owned.selected_capture_bytes - envelope >
				    PLAYER_SNAPSHOT_MAX_BYTES - owned.destination_capture_bytes)
			{
				pending.erase(found);
				return shop_trade_preparation_state::refused;
			}
			int32_t shell = 0, direct = 0;
			if (target.front().values[4] > 0)
			{
				int64_t sum = 0;
				for (size_t index = 1; index < target.size(); ++index)
					if (target[index].parent_index == 0)
					{
						sum += target[index].weight;
						if (sum < INT32_MIN || sum > INT32_MAX)
						{
							pending.erase(found);
							return shop_trade_preparation_state::refused;
						}
					}
				direct = static_cast<int32_t>(sum);
				owned.destination_shell_started = true;
				const bool captured =
					obj_capture_container_shell_weight(destination, &shell);
				owned.destination_shell_returned = true;
				if (!captured)
				{
					pending.erase(found);
					return shop_trade_preparation_state::refused;
				}
				// Hooks may touch the world. Resolve the original bodies again
				// and compare all literal source values before any checkpoint.
				actor = find_character_by_runtime_id(owned.actor_runtime_id);
				keeper = find_character_by_runtime_id(owned.keeper_runtime_id);
				if (!preparation_body(actor, keeper, shop))
					return shop_trade_preparation_state::pending;
				std::vector<uint8_t> current_target, current_selected;
				// The original UID census is established again by poll before
				// SQL; pointers returned across a probe are never retained.
				P_obj original_destination = nullptr, original_selected = nullptr;
				size_t count = 0;
				for (P_obj object = actor->carrying; object;
				     object = object->next_content)
				{
					if (++count > PLAYER_SNAPSHOT_MAX_OBJECTS ||
					    !OBJ_CARRIED_BY(object, actor))
						return shop_trade_preparation_state::pending;
					if (object->obj_uid ==
					    found->second.payload.target_parent_item_uid)
					{
						if (original_destination)
							return shop_trade_preparation_state::pending;
						original_destination = object;
					}
				}
				// Produced selection is detached, so resolve it from the global
				// object list through the same original UID; never read a stale pointer.
				extern P_obj object_list;
				count = 0;
				for (P_obj object = object_list; object; object = object->next)
				{
					if (++count > 262144)
						return shop_trade_preparation_state::pending;
					if (object->obj_uid ==
					    found->second.payload.selected_item_uid)
					{
						if (original_selected)
							return shop_trade_preparation_state::pending;
						original_selected = object;
					}
				}
				if (!original_destination || !original_selected ||
				    !preparation_destination_tree(original_destination,
								  &current_target) ||
				    !preparation_tree(original_selected, &current_selected) ||
				    current_target != owned.destination ||
				    current_selected != owned.selected)
					return shop_trade_preparation_state::pending;
				destination = original_destination;
				selected = original_selected;
			}
			if (!shop_trade_destination_weight_compute(
				    target.front(), source.front().weight, direct, shell,
				    &owned.destination_weight))
			{
				pending.erase(found);
				return shop_trade_preparation_state::refused;
			}
			// The complete future literal destination must fit the same capture
			// envelope now, before checkpoint/SQL/admission. Selected STORE/key
			// transforms are fixed-width and cannot enlarge this encoding.
			auto combined = target;
			combined.front().weight = owned.destination_weight.after;
			const auto base = static_cast<int32_t>(combined.size());
			for (auto item : source)
			{
				item.parent_index =
					item.parent_index < 0 ? 0 : base + item.parent_index;
				combined.push_back(std::move(item));
			}
			std::vector<uint8_t> encoded;
			if (player_item_snapshot_list_encode(combined, &encoded) !=
				    player_snapshot_codec_result::ok ||
			    encoded.size() > PLAYER_SNAPSHOT_MAX_BYTES - sizeof(uint32_t))
			{
				pending.erase(found);
				return shop_trade_preparation_state::refused;
			}
		}
		found->second.preparation->destination_weight_ready = true;

		// The domain entry already owns the keeper and every frozen UID while
		// the ordinary save request captures/queues the player's checkpoint.
		const bool selling = action == shop_trade_action::sell_store ||
				     action == shop_trade_action::sell_destroy;
		const auto state = player_save_pipeline_shop_checkpoint_begin(
			actor, selling ? selected : nullptr, NOWHERE,
			&found->second.preparation->player_token);
		if (state == player_literal_inventory_state::refused)
		{
			pending.erase(found);
			return shop_trade_preparation_state::refused;
		}
		return shop_trade_preparation_state::pending;
	}
	catch (...)
	{
		// A matching generated ID is not ownership if insertion failed.
		if (!owns_entry)
			return shop_trade_preparation_state::refused;
		auto found = pending.find(operation.bytes);
		if (found == pending.end())
			return shop_trade_preparation_state::refused;
		if (found->second.preparation->destination_shell_started &&
		    !found->second.preparation->destination_shell_returned)
			return shop_trade_preparation_state::pending;
		const auto &player = found->second.preparation->player_token;
		// The pipeline exposes the installed slot before enqueue. A zero
		// token proves this preparation never acquired a player checkpoint;
		// no keeper write or critical admission has been attempted yet.
		if (!player.pid && !player.actor_runtime_id && !player.generation)
		{
			pending.erase(found);
			return shop_trade_preparation_state::refused;
		}
		return shop_trade_preparation_state::pending;
	}
#endif
}

shop_trade_preparation_state
shop_trade_preparation_owner::poll(const shop_trade_preparation_token &token, P_char actor,
				   P_char keeper, P_obj selected, P_obj stock,
				   P_obj destination) noexcept
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)keeper;
	(void)selected;
	(void)stock;
	(void)destination;
	return shop_trade_preparation_state::refused;
#else
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return shop_trade_preparation_state::refused;
	auto &entry = found->second;
	auto &prepared = *entry.preparation;
	if (prepared.submission_started ||
	    (prepared.destination_shell_started && !prepared.destination_shell_returned) ||
	    !prepared.destination_weight_ready)
		return shop_trade_preparation_state::refused;
	try
	{
		if (!preparation_body(actor, keeper, entry.payload.shop_id) ||
		    actor->runtime_id != prepared.actor_runtime_id ||
		    keeper->runtime_id != prepared.keeper_runtime_id ||
		    static_cast<uint32_t>(GET_PID(actor)) != entry.player_pid || !selected ||
		    selected->obj_uid != entry.payload.selected_item_uid ||
		    (stock ? stock->obj_uid : 0) != entry.payload.stock_item_uid ||
		    (destination ? destination->obj_uid : 0) !=
			    entry.payload.target_parent_item_uid ||
		    GET_VNUM(keeper) != entry.payload.keeper_vnum ||
		    (shop_index[entry.payload.shop_id].shop_is_roaming != 0) !=
			    (entry.payload.keeper_roaming != 0) ||
		    GET_RACEWAR(actor) != entry.payload.racewar ||
		    ((entry.payload.action == shop_trade_action::sell_store ||
		      entry.payload.action == shop_trade_action::sell_destroy) &&
		     !OBJ_CARRIED_BY(selected, actor)) ||
		    (entry.payload.action == shop_trade_action::buy_existing &&
		     !OBJ_CARRIED_BY(selected, keeper)) ||
		    (stock && !OBJ_CARRIED_BY(stock, keeper)) ||
		    (destination && !OBJ_CARRIED_BY(destination, actor)))
			return shop_trade_preparation_state::refused;
		const int64_t keeper_cash = static_cast<int64_t>(GET_COPPER(keeper)) +
					    10LL * GET_SILVER(keeper) + 100LL * GET_GOLD(keeper) +
					    1000LL * GET_PLATINUM(keeper);
		if (GET_COPPER(keeper) < 0 || GET_SILVER(keeper) < 0 || GET_GOLD(keeper) < 0 ||
		    GET_PLATINUM(keeper) < 0 || keeper_cash != entry.payload.expected_keeper_cash)
			return shop_trade_preparation_state::refused;
		const char *account = get_account_name_safe(actor);
		economic_shop_checkpoint_projection mapping{};
		std::vector<uint8_t> a, b, c, d;
		std::vector<player_item_snapshot> keeper_items;
		if (!account || strcmp(account, entry.payload.account_name.data()) ||
		    !economic_gameplay_authority::observe_shop_checkpoint(
			    entry.player_pid, account, entry.payload.racewar, &mapping) ||
		    !same_mapping(mapping, prepared.mapping) || !preparation_tree(selected, &a) ||
		    a != prepared.selected || !preparation_tree(stock, &b) || b != prepared.stock ||
		    !preparation_destination_tree(destination, &c) || c != prepared.destination ||
		    !shop_item_runtime_capture_keeper_literal(keeper, &keeper_items) ||
		    player_item_snapshot_list_encode(keeper_items, &d) !=
			    player_snapshot_codec_result::ok ||
		    d != prepared.keeper_blob)
			return shop_trade_preparation_state::refused;
		if (prepared.player_held)
		{
			player_shop_checkpoint_stage held{};
			if (!player_save_shop_checkpoint_owner::observe_held(
				    prepared.player_token, actor, token.operation_id_, &held) ||
			    held.save_revision != prepared.player_stage.save_revision ||
			    held.level != prepared.player_stage.level)
				return shop_trade_preparation_state::refused;
			return shop_trade_preparation_state::ready;
		}
		player_shop_checkpoint_stage stage{};
		const auto state = player_save_pipeline_shop_checkpoint_poll(prepared.player_token,
									     actor, &stage);
		if (state == player_literal_inventory_state::pending)
			return shop_trade_preparation_state::pending;
		if (state != player_literal_inventory_state::database_acknowledged ||
		    !player_save_pipeline_shop_checkpoint_hold(prepared.player_token,
							       token.operation_id_))
			return shop_trade_preparation_state::refused;
		prepared.player_stage = stage;
		prepared.player_held = true;
		return shop_trade_preparation_state::ready;
	}
	catch (...)
	{
		return shop_trade_preparation_state::refused;
	}
#endif
}

bool shop_trade_preparation_owner::build_accounted_command(
	const shop_trade_preparation_token &token, P_char actor, P_char keeper, P_obj selected,
	P_obj stock, P_obj destination, critical_command *output) noexcept
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)keeper;
	(void)selected;
	(void)stock;
	(void)destination;
	(void)output;
	return false;
#else
	if (!output || !nevent_is_game_thread() ||
	    poll(token, actor, keeper, selected, stock, destination) !=
		    shop_trade_preparation_state::ready)
		return false;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return false;
	auto &entry = found->second;
	auto &prepared = *entry.preparation;
	if (!prepared.native_ready || !prepared.native_sealed || !prepared.player_held)
		return false;
	try
	{
		const auto &native = prepared.native_stage;
		const std::array<int64_t, 4> wallet{ GET_COPPER(actor), GET_SILVER(actor),
						     GET_GOLD(actor), GET_PLATINUM(actor) };
		const std::array<int64_t, 4> bank{ GET_BALANCE_COPPER(actor),
						   GET_BALANCE_SILVER(actor),
						   GET_BALANCE_GOLD(actor),
						   GET_BALANCE_PLATINUM(actor) };
		if (wallet != native.wallet.amount || bank != native.bank.amount ||
		    actor->only.pc->wallet_revision != native.wallet_revision ||
		    actor->only.pc->bank_revision != native.bank_revision)
			return false;
		if (prepared.command)
		{
			critical_command copied = *prepared.command;
			*output = std::move(copied);
			return true;
		}
		shop_trade_payload payload{};
		if (shop_trade_runtime_build_accounted_payload(
			    actor, keeper, selected, stock, destination, entry.payload.shop_id,
			    entry.payload.action, entry.payload.price, prepared.player_stage,
			    native.shop_revision_after,
			    &payload) != shop_trade_payload_build_result::ok)
			return false;
		payload.native_destination_weight_recorded = true;
		payload.destination_weight = prepared.destination_weight;
		// Refuse an unpublishable complete player AFTER before journal admission
		// or SQL. Reuse the exact held original body and the final observer rules.
		player_shop_checkpoint_stage held{};
		std::vector<player_item_snapshot> original, prospective;
		if (!player_save_shop_checkpoint_owner::observe_held(
			    prepared.player_token, actor, token.operation_id_, &held, &original) ||
		    held.save_revision != payload.expected_player_save_revision ||
		    held.level != payload.expected_player_level ||
		    !shop_trade_native_publication_owner::expected_player_forest(
			    payload, original, false, &prospective))
			return false;
		if (payload.action == shop_trade_action::buy_existing ||
		    payload.action == shop_trade_action::buy_produced)
		{
			std::vector<player_item_snapshot> ordered;
			if (!shop_trade_native_publication_owner::expected_player_order(
				    payload, actor, selected, destination, prospective, &ordered))
				return false;
			prospective = std::move(ordered);
		}
		if (!shop_trade_world_player_values_supported(prospective))
			return false;

		// Saved-policy inventory omits some physical NORENT bodies; its row
		// count alone cannot prove that the final bounded world census fits.
		std::vector<uint64_t> literal_roots;
		if (std::any_of(original.begin(), original.end(), [&](const auto &item)
				{ return item.object_uid == payload.selected_item_uid; }))
			literal_roots.push_back(payload.selected_item_uid);
		std::vector<player_item_snapshot> detached;
		if (payload.action == shop_trade_action::buy_produced &&
		    player_item_snapshot_list_decode(payload.item_blob.data(),
						     payload.item_blob_size,
						     &detached) != player_snapshot_codec_result::ok)
			return false;
		shop_trade_world_expectation before_world{};
		before_world.actor_pid = payload.player_pid;
		before_world.actor_runtime_id = prepared.actor_runtime_id;
		before_world.keeper_runtime_id = prepared.keeper_runtime_id;
		before_world.shop_id = payload.shop_id;
		before_world.keeper_vnum = payload.keeper_vnum;
		before_world.player_items = original;
		before_world.keeper_items = prepared.keeper_items;
		before_world.detached_items = detached;
		before_world.literal_player_root_uids = literal_roots;
		shop_trade_world_witness before_observed;
		if (!shop_trade_world_witness_observe(before_world, &before_observed) ||
		    ((payload.action == shop_trade_action::buy_existing ||
		      payload.action == shop_trade_action::buy_produced) &&
		     payload.item_count > PLAYER_SNAPSHOT_MAX_OBJECTS -
						  before_observed.player_physical_item_count))
			return false;

		// Journal the original ordered forest bindings before admission. These
		// values are recovery comparison inputs; SQL must authenticate their
		// complete original images under the existing authority/custody locks.
		std::vector<player_item_snapshot> keeper_after;
		if (!shop_trade_native_publication_owner::expected_keeper_forest(
			    payload, keeper, selected, prepared.keeper_items, false, &keeper_after))
			return false;
		const auto freeze_forest = [](const std::vector<player_item_snapshot> &forest,
					      shop_trade_recovery_forest_role role,
					      shop_trade_recovery_forest_binding *binding)
		{
			std::vector<uint8_t> canonical;
			return player_item_snapshot_list_encode(forest, &canonical) ==
				       player_snapshot_codec_result::ok &&
			       shop_trade_recovery_forest_freeze(canonical, role, binding);
		};
		shop_trade_recovery_manifest manifest;
		if (!freeze_forest(original, shop_trade_recovery_forest_role::player_before,
				   &manifest.player_before) ||
		    !freeze_forest(prospective, shop_trade_recovery_forest_role::player_after,
				   &manifest.player_after) ||
		    !freeze_forest(prepared.keeper_items,
				   shop_trade_recovery_forest_role::keeper_before,
				   &manifest.keeper_before) ||
		    !freeze_forest(keeper_after, shop_trade_recovery_forest_role::keeper_after,
				   &manifest.keeper_after))
			return false;
		if (payload.target_parent_item_uid)
		{
			// Full live literal includes omitted non-durable NORENT children.
			// Cold recovery uses the separate saved-policy player binding and
			// must neither demand nor recreate those omitted children.
			std::vector<player_item_snapshot> target_after;
			if (!shop_trade_recovery_forest_freeze(
				    prepared.destination,
				    shop_trade_recovery_forest_role::live_target_before,
				    &manifest.live_target_before) ||
			    !shop_trade_native_publication_owner::expected_destination_forest(
				    payload, actor, selected, destination, prepared.destination,
				    &target_after) ||
			    !freeze_forest(target_after,
					   shop_trade_recovery_forest_role::live_target_after,
					   &manifest.live_target_after))
				return false;
		}
		payload.recovery_manifest = std::move(manifest);
		payload.recovery_manifest_recorded = true;

		auto command = std::make_unique<critical_command>();
		if (!shop_trade_command_build_recovery(command.get(), token.operation_id_, payload,
						       critical_source_site::command,
						       critical_deadline_class::interactive) ||
		    economic_gameplay_authority::prepare_shop_trade(command.get()) !=
			    economic_accounting_error::ok ||
		    command->schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
			return false;
		economic_frozen_intent intent;
		shop_trade_payload verified{};
		economic_account_key verified_wallet, verified_bank, counterparty;
		auto projection = *command;
		projection.accepted_at_usec = 1;
		if (shop_trade_accounting_decode(projection, &intent, &verified, &verified_wallet,
						 &verified_bank,
						 &counterparty) != economic_accounting_error::ok ||
		    intent.admission.metadata.epoch.bytes != prepared.mapping.epoch.bytes ||
		    !economic_account_key_equal(verified_wallet, prepared.mapping.wallet) ||
		    !economic_account_key_equal(verified_bank, prepared.mapping.bank))
			return false;
		const auto now = std::chrono::duration_cast<std::chrono::microseconds>(
					 std::chrono::system_clock::now().time_since_epoch())
					 .count();
		if (now <= 0)
			return false;
		command->accepted_at_usec = static_cast<uint64_t>(now);
		command->publication_required = true;
		if (!critical_command_envelope_valid(*command))
			return false;
		// All allocating copies finish before retaining the immutable decision.
		// Subsequent exact-ID attempts preserve its original real acceptance time.
		critical_command copied = *command;
		prepared.command = std::move(command);
		*output = std::move(copied);
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

critical_submit_result shop_trade_preparation_owner::submit_accounted(
	const shop_trade_preparation_token &token, P_char actor, P_char keeper, P_obj selected,
	P_obj stock, P_obj destination, shop_trade_accounted_publication_fn publication,
	shop_trade_completion_fn completion) noexcept
{
#ifdef __NO_MYSQL__
	(void)token;
	(void)actor;
	(void)keeper;
	(void)selected;
	(void)stock;
	(void)destination;
	(void)publication;
	(void)completion;
	return critical_submit_result::unavailable;
#else
	if (!nevent_is_game_thread() || !publication)
		return critical_submit_result::invalid;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return critical_submit_result::identity_conflict;
	auto &entry = found->second;
	auto &prepared = *entry.preparation;
	try
	{
		critical_command command;
		shop_trade_payload payload{};
		if (prepared.submission_started)
		{
			if (!prepared.command || entry.accounted_publication != publication ||
			    entry.completion != completion)
				return critical_submit_result::identity_conflict;
			command = *prepared.command;
		}
		else if (!build_accounted_command(token, actor, keeper, selected, stock,
						  destination, &command))
			return critical_submit_result::unavailable;
		if (!shop_trade_command_decode_payload(command, &payload) ||
		    command.operation_id.bytes != token.operation_id_.bytes ||
		    (command.payload_version != SHOP_TRADE_NATIVE_PAYLOAD_VERSION &&
		     command.payload_version != SHOP_TRADE_RECOVERY_PAYLOAD_VERSION) ||
		    payload.player_pid != entry.player_pid || !prepared.native_ready ||
		    !prepared.native_sealed || !prepared.player_held)
			return critical_submit_result::invalid;
		// Before admission the selection placeholder stays intact for full poll
		// recapture. Freeze the complete v6 payload only after that poll succeeds.
		const auto previous_payload = entry.payload;
		const auto previous_publication = entry.accounted_publication;
		const auto previous_completion = entry.completion;
		const bool was_started = prepared.submission_started;
		entry.payload = payload;
		entry.accounted_publication = publication;
		entry.completion = completion;
		prepared.submission_started = true;
		bool checkpoint_released = false;
		const auto submitted = player_save_shop_checkpoint_owner::submit_owned(
			prepared.player_token, std::move(command), &checkpoint_released);
		if (critical_submit_result_keeps_operation(submitted))
			return submitted;
		if (checkpoint_released)
		{
			// The pipeline proved synchronous refusal before journal admission and
			// consumed its exact original execution generation. The keeper's durable
			// checkpoint remains: retiring preparation is no economic rollback.

			if (entry.production_owned)
			{
				entry.local_refused = true;
				entry.refusal_error = EAGAIN;
				prepared.player_held = false;
			}
			else
				pending.erase(found);
			return submitted;
		}
		if (was_started)
			return critical_submit_result::journal_uncertain;
		// No execution hold was installed. Preserve the original held checkpoint
		// and selection so a later original-token attempt can revalidate it.
		entry.payload = previous_payload;
		entry.accounted_publication = previous_publication;
		entry.completion = previous_completion;
		prepared.submission_started = false;
		return submitted;
	}
	catch (...)
	{
		return prepared.submission_started ? critical_submit_result::journal_uncertain :
						     critical_submit_result::invalid;
	}
#endif
}

bool shop_trade_preparation_owner::cancel(const shop_trade_preparation_token &token) noexcept
{
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return false;
	auto &prepared = *found->second.preparation;

	if (found->second.local_refused)
		return true;
	if (prepared.destination_shell_started && !prepared.destination_shell_returned)
		return false;
#ifndef __NO_MYSQL__
	// An unresolved native attempt still owns its original rows/revisions/body.
	// Committed preparation also remains owned until guarded admission/publication.
	if (prepared.native_attempted || prepared.native_ready)
		return false;
#endif
	if (prepared.submission_started)
		return false;
	if (!prepared.player_token.pid && !prepared.player_token.actor_runtime_id &&
	    !prepared.player_token.generation && !prepared.player_held)
	{
		// No checkpoint, SQL preparation or command admission exists. A normally
		// returned probe can be abandoned; an uncertain native tail cannot.

		if (found->second.production_owned)
			found->second.local_refused = true;
		else
			pending.erase(found);
		return true;
	}
	const bool released =
		prepared.player_held ?
			player_save_pipeline_shop_checkpoint_release(prepared.player_token,
								     token.operation_id_) :
			player_save_pipeline_shop_checkpoint_cancel(prepared.player_token);

	if (!released)
		return false;
	if (found->second.production_owned)
	{
		found->second.local_refused = true;
		prepared.player_held = false;
	}
	else
		pending.erase(found);
	return true;
}

bool shop_trade_preparation_owner::production_available() noexcept
{
#ifdef __NO_MYSQL__
	return false;
#else
	return nevent_is_game_thread() && economic_gameplay_authority::active_regular_sql() &&
	       economic_shop_trade_admission_available();
#endif
}

bool shop_trade_preparation_owner::start(P_char actor, P_char keeper, P_obj selected, P_obj stock,
					 P_obj destination, uint32_t shop, shop_trade_action action,
					 int64_t price,
					 shop_trade_accounted_publication_fn publication,
					 shop_trade_completion_fn completion,
					 shop_trade_preparation_refusal_fn refusal) noexcept
{
	if (!publication || !completion || !refusal || !production_available())
		return false;
	shop_trade_preparation_token token;
	begin(actor, keeper, selected, stock, destination, shop, action, price, &token);
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_ ||
	    found->second.production_owned)
		return false;
	// begin may retain an uncertain native probe tail. Ownership is the retained
	// original entry, not its status enum; never let the caller free that stage.
	auto &entry = found->second;
	entry.accounted_publication = publication;
	entry.completion = completion;
	entry.refusal = refusal;
	entry.production_owned = true;
	return true;
}

void shop_trade_preparation_owner::notify_refusal(const shop_trade_preparation_token &token) noexcept
{
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return;
	auto &entry = found->second;
	if (!entry.production_owned || !entry.local_refused || entry.blocked || entry.publishing ||
	    entry.refusal_attempted || !entry.refusal)
		return;
	// This is notification after proven cancellation, not rebinding a native
	// checkpoint. A reconnect can receive its original PID/account cancellation.
	P_char actor = find_player_by_pid(entry.player_pid);
	if (!actor || !IS_PC(actor) || !actor->only.pc || GET_PID(actor) <= 0 ||
	    static_cast<uint32_t>(GET_PID(actor)) != entry.player_pid ||
	    GET_RACEWAR(actor) != entry.payload.racewar)
		return;
	const char *account = get_account_name_safe(actor);
	if (!account || strcmp(account, entry.payload.account_name.data()))
		return;
	entry.refusal_attempted = entry.publishing = true;
	++notifying;
	bool finished = false;
	try
	{
		// Keep the entry visible to PID/keeper/UID exclusions during callbacks.
		// A partial/throwing cleanup cannot admit a second overlapping owner.
		finished = entry.refusal(actor, entry.payload, entry.preparation->selected,
					 entry.refusal_error);
	}
	catch (...)
	{
		finished = false;
	}
	--notifying;
	found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return;
	found->second.publishing = false;
	if (finished)
		pending.erase(found);
	else
		found->second.blocked = true;
}

void shop_trade_preparation_owner::drive(const shop_trade_preparation_token &token) noexcept
{
#ifndef __NO_MYSQL__
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_ ||
	    !found->second.production_owned || found->second.driving || found->second.blocked ||
	    found->second.publishing || found->second.completion_ready)
		return;
	if (found->second.local_refused)
	{
		notify_refusal(token);
		return;
	}
	found->second.driving = true;
	struct driving_scope
	{
		std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES> key;
		uint64_t generation;
		~driving_scope()
		{
			auto current = pending.find(key);
			if (current != pending.end() && current->second.preparation &&
			    current->second.preparation->generation == generation)
				current->second.driving = false;
		}
	} scope{ token.operation_id_.bytes, token.generation_ };
	// Submission retries use only the previously frozen command and callbacks.
	// They never reselect a keeper/item, recreate a stage or reroll a copy.
	if (found->second.preparation->submission_started)
	{
		submit_accounted(token, nullptr, nullptr, nullptr, nullptr, nullptr,
				 found->second.accounted_publication, found->second.completion);
		return;
	}
	// Resolve an existing uncertain checkpoint against its retained SQL stage
	// before requiring any original live body. This retry API cannot issue a
	// fresh write; a settled commit still needs full live proof below.
	if (found->second.preparation->native_attempted && !found->second.preparation->native_ready)
	{
		const auto reconciled = shop_trade_native_checkpoint_owner::attempt(
			token, nullptr, nullptr, nullptr, nullptr, nullptr);
		found = pending.find(token.operation_id_.bytes);
		if (found == pending.end() || !found->second.preparation ||
		    found->second.preparation->generation != token.generation_ ||
		    found->second.blocked || found->second.local_refused ||
		    found->second.completion_ready)
			return;
		if (reconciled != shop_trade_preparation_state::ready)
			return;
	}
	if (!production_available())
	{
		cancel(token);
		return;
	}
	auto unique_object = [](uint64_t uid) -> P_obj
	{
		if (!uid)
			return nullptr;
		extern P_obj object_list;
		P_obj result = nullptr;
		size_t count = 0;
		for (P_obj object = object_list; object; object = object->next)
		{
			if (++count > 262144)
				return nullptr;
			if (object->obj_uid != uid)
				continue;
			if (result)
				return nullptr;
			result = object;
		}
		return result;
	};
	// Reobserve all original identities after each native attempt. The helper
	// merely produces transient pointers; poll owns full literal equality proof.
	auto observe = [&](P_char &actor, P_char &keeper, P_obj &selected, P_obj &stock,
			   P_obj &destination)
	{
		auto current = pending.find(token.operation_id_.bytes);
		if (current == pending.end() || !current->second.preparation ||
		    current->second.preparation->generation != token.generation_ ||
		    current->second.blocked || current->second.local_refused ||
		    current->second.completion_ready)
			return false;
		const auto &entry = current->second;
		actor = find_character_by_runtime_id(entry.preparation->actor_runtime_id);
		keeper = find_character_by_runtime_id(entry.preparation->keeper_runtime_id);
		selected = unique_object(entry.payload.selected_item_uid);
		stock = unique_object(entry.payload.stock_item_uid);
		destination = unique_object(entry.payload.target_parent_item_uid);
		return actor && keeper && selected && (!entry.payload.stock_item_uid || stock) &&
		       (!entry.payload.target_parent_item_uid || destination);
	};
	P_char actor = nullptr, keeper = nullptr;
	P_obj selected = nullptr, stock = nullptr, destination = nullptr;
	if (!observe(actor, keeper, selected, stock, destination))
	{
		cancel(token);
		return;
	}
	const auto player = poll(token, actor, keeper, selected, stock, destination);
	if (player == shop_trade_preparation_state::pending)
		return;
	if (player != shop_trade_preparation_state::ready)
	{
		cancel(token);
		return;
	}
	const auto native = shop_trade_native_checkpoint_owner::attempt(
		token, actor, keeper, selected, stock, destination);
	if (native == shop_trade_preparation_state::pending)
		return;
	if (native != shop_trade_preparation_state::ready)
	{
		// cancel refuses any uncertain native tail or committed checkpoint.
		cancel(token);
		return;
	}
	if (!observe(actor, keeper, selected, stock, destination))
		return;
	found = pending.find(token.operation_id_.bytes);
	submit_accounted(token, actor, keeper, selected, stock, destination,
			 found->second.accounted_publication, found->second.completion);
#else
	(void)token;
#endif
}

void shop_trade_preparation_owner::pulse() noexcept
{
	if (!nevent_is_game_thread())
		return;
	std::array<shop_trade_preparation_token, SHOP_TRADE_PENDING_MAX> original{};
	size_t count = 0;
	for (const auto &[key, entry] : pending)
	{
		if (count == original.size())
			break;
		if (!entry.production_owned || !entry.preparation)
			continue;
		original[count].operation_id_.bytes = key;
		original[count].generation_ = entry.preparation->generation;
		++count;
	}
	for (size_t index = 0; index < count; ++index)
		drive(original[index]);
}

bool shop_trade_preparation_owner::checkpoint_context(
	const shop_trade_preparation_token &token, shop_trade_checkpoint_context *output) noexcept
{
	auto found = pending.find(token.operation_id_.bytes);
	if (!output || found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_ ||
	    !found->second.preparation->player_held)
		return false;
	try
	{
		const auto &entry = found->second;
		const auto &prepared = *entry.preparation;
		shop_trade_checkpoint_context stage;
		stage.operation_id = token.operation_id_;
		stage.generation = token.generation_;
		stage.actor_runtime_id = prepared.actor_runtime_id;
		stage.keeper_runtime_id = prepared.keeper_runtime_id;
		stage.player_pid = entry.player_pid;
		stage.shop_id = entry.payload.shop_id;
		stage.account_name = entry.payload.account_name;
		stage.racewar = entry.payload.racewar;
		stage.keeper_roaming = entry.payload.keeper_roaming;
		stage.keeper_cash = entry.payload.expected_keeper_cash;
		stage.keeper_vnum = entry.payload.keeper_vnum;
		stage.mapping = prepared.mapping;
		stage.player = prepared.player_stage;
		stage.keeper_items = prepared.keeper_items;
		*output = std::move(stage);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

#ifndef __NO_MYSQL__
bool shop_trade_preparation_owner::begin_native_checkpoint(
	const shop_trade_preparation_token &token, P_char actor, P_char keeper, P_obj selected,
	P_obj stock, P_obj destination, shop_trade_checkpoint_context *context,
	shop_trade_native_checkpoint_stage *retained, bool *retry) noexcept
{
	if (!nevent_is_game_thread() || !context || !retained || !retry)
		return false;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return false;
	auto &prepared = *found->second.preparation;
	if (!prepared.player_held || prepared.native_ready)
		return false;
	// Existing uncertain writes resolve against original SQL facts even if the
	// actor has disconnected or gameplay recapture changed. Retry grants no write.
	if (!prepared.native_attempted && poll(token, actor, keeper, selected, stock,
					       destination) != shop_trade_preparation_state::ready)
		return false;
	try
	{
		shop_trade_checkpoint_context copied_context;
		if (!checkpoint_context(token, &copied_context))
			return false;
		// Copying for a retry may allocate. Failure cannot discard the original
		// sealed stage or reopen cancellation after an uncertain native mutation.
		shop_trade_native_checkpoint_stage copied_stage = prepared.native_stage;
		const bool existing_attempt = prepared.native_attempted;
		*context = std::move(copied_context);
		*retained = std::move(copied_stage);
		*retry = existing_attempt;
		prepared.native_attempted = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_preparation_owner::completed_native_checkpoint(
	const shop_trade_preparation_token &token,
	shop_trade_native_checkpoint_stage *output) noexcept
{
	if (!nevent_is_game_thread() || !output)
		return false;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return false;
	const auto &prepared = *found->second.preparation;
	if (!prepared.player_held || !prepared.native_attempted || !prepared.native_sealed ||
	    !prepared.native_ready)
		return false;
	try
	{
		shop_trade_native_checkpoint_stage copied = prepared.native_stage;
		*output = std::move(copied);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool shop_trade_preparation_owner::seal_native_checkpoint(
	const shop_trade_preparation_token &token,
	shop_trade_native_checkpoint_stage &&stage) noexcept
{
	if (!nevent_is_game_thread())
		return false;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return false;
	auto &prepared = *found->second.preparation;
	if (!prepared.player_held || !prepared.native_attempted || prepared.native_sealed ||
	    prepared.native_ready || !stage.keeper_id || !stage.bank_id ||
	    stage.shop_revision_before == UINT64_MAX ||
	    stage.shop_revision_after != stage.shop_revision_before + 1 ||
	    (stage.payload_checkpoint_recorded_before ?
		     (!stage.payload_checkpoint_revision_before ||
		      stage.payload_checkpoint_revision_before > stage.shop_revision_before) :
		     stage.payload_checkpoint_revision_before != 0) ||
	    stage.keeper_image.size() != prepared.keeper_items.size())
		return false;
	prepared.native_stage = std::move(stage);
	prepared.native_sealed = true;
	return true;
}

bool shop_trade_preparation_owner::finish_native_checkpoint(
	const shop_trade_preparation_token &token,
	shop_trade_native_checkpoint_disposition disposition) noexcept
{
	if (!nevent_is_game_thread())
		return false;
	auto found = pending.find(token.operation_id_.bytes);
	if (found == pending.end() || !found->second.preparation ||
	    found->second.preparation->generation != token.generation_)
		return false;
	auto &prepared = *found->second.preparation;
	if (!prepared.player_held || !prepared.native_attempted || prepared.native_ready)
		return false;
	switch (disposition)
	{
	case shop_trade_native_checkpoint_disposition::committed:
		if (!prepared.native_sealed)
			return false;
		prepared.native_ready = true;
		return true;
	case shop_trade_native_checkpoint_disposition::never_mutated_retired:
		// Exact original session closure plus absence of a sealed stage proves
		// no native mutation could have been issued. It proves no COMMIT outcome.
		if (prepared.native_sealed)
			return false;
		[[fallthrough]];
	case shop_trade_native_checkpoint_disposition::rolled_back:
		prepared.native_stage = {};
		prepared.native_sealed = prepared.native_attempted = false;
		return true;
	case shop_trade_native_checkpoint_disposition::uncertain:
		return false;
	}
	return false;
}
#endif

bool shop_trade_transaction_keeper_busy(uint32_t shop)
{
	return std::any_of(pending.begin(), pending.end(), [shop](const auto &entry)
			   { return entry.second.payload.shop_id == shop; });
}

bool shop_trade_transaction_item_busy(uint64_t uid)
{
	if (!uid)
		return false;
	return std::any_of(pending.begin(), pending.end(),
			   [uid](const auto &value)
			   {
				   const auto &entry = value.second;
				   if (entry.preparation)
					   return std::binary_search(
						   entry.preparation->fenced_uids.begin(),
						   entry.preparation->fenced_uids.end(), uid);
				   const auto &p = entry.payload;
				   if (p.selected_item_uid == uid || p.stock_item_uid == uid ||
				       p.target_parent_item_uid == uid)
					   return true;
				   for (size_t i = 0; i < p.item_count; ++i)
					   if (p.items[i].item_uid == uid)
						   return true;
				   return false;
			   });
}

void shop_trade_transaction_reset_for_tests(void)
{
	pending.clear();
	notifying = 0;
	preparation_generation = 0;
}
