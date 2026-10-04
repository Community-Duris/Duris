#include "economy/coin_physical_publication.h"
#include "economy/economic_gameplay_authority.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "world/handler.h"
#include "item/item_ownership_runtime.h"
#include "player/player_load_items.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <climits>
#include <openssl/sha.h>

extern P_obj object_list;
extern P_room world;
extern const int top_of_world;

namespace
{
// Bound storage independently of allocation and preserve uncertain native
// effects until the owning operation is explicitly retired after ACK.
struct publication
{
	bool used = false;
	bool entered = false;
	critical_operation_id id = {};
	uint64_t uid = 0;
	std::array<unsigned char, SHA256_DIGEST_LENGTH> digest = {};
	bool materialize_started = false, materialize_returned = false;
	bool handler_started = false, handler_returned = false;
	bool amount_started = false, amount_returned = false;
	bool complete = false;
};
std::array<publication, CURRENCY_PENDING_MAX> publications;

P_obj find_unique(uint64_t uid, bool *duplicate)
{
	P_obj found = nullptr;
	*duplicate = false;
	P_obj slow = object_list, fast = object_list;
	for (P_obj object = object_list; object; object = object->next)
	{
		if (fast && fast->next)
		{
			fast = fast->next->next;
			slow = slow->next;
			if (fast == slow)
			{
				*duplicate = true;
				return nullptr;
			}
		}
		else
			fast = nullptr;
		if (object->obj_uid == uid)
		{
			if (found)
				*duplicate = true;
			found = object;
		}
	}
	return found;
}

bool bytes_match(P_obj money, const std::vector<player_item_snapshot> &expected)
{
	std::vector<player_item_snapshot> actual;
	std::vector<uint8_t> actual_bytes, expected_bytes;
	if (player_item_snapshot_tree_capture_literal(money, &actual, nullptr) !=
		    player_snapshot_capture_result::ok ||
	    actual.size() != 1 || expected.size() != 1)
		return false;
	// Tree capture seeds an inventory root slot. The persisted room pile has
	// an explicit floor slot; normalize only that placement representation.
	actual[0].equipment_slot = -1;
	return player_item_snapshot_list_encode(actual, &actual_bytes) ==
		       player_snapshot_codec_result::ok &&
	       player_item_snapshot_list_encode(expected, &expected_bytes) ==
		       player_snapshot_codec_result::ok &&
	       actual_bytes == expected_bytes;
}

// Render the native denomination-dependent fields without touching the live
// source. The remaining captured fields must equal the original target blob.
bool source_matches(P_obj money, const coin_transfer_endpoint &endpoint,
		    const std::vector<player_item_snapshot> &expected)
{
	std::vector<player_item_snapshot> actual;
	if (!money || !std::equal(endpoint.before.begin(), endpoint.before.end(), money->value) ||
	    player_item_snapshot_tree_capture_literal(money, &actual, nullptr) !=
		    player_snapshot_capture_result::ok ||
	    actual.size() != 1)
		return false;
	actual[0].equipment_slot = -1;
	struct rendering
	{
		obj_data object = {};
		extra_descr_data detail = {};
		~rendering()
		{
			if (object.description)
				str_free(object.description);
			if (object.short_description)
				str_free(object.short_description);
			if (detail.description)
				str_free(detail.description);
		}
	} rendered;
	rendered.object.type = ITEM_MONEY;
	rendered.object.ex_description = &rendered.detail;
	std::copy(endpoint.before.begin(), endpoint.before.end(), rendered.object.value);
	add_coins(&rendered.object, 0, 0, 0, 0);
	if (!rendered.object.description || !rendered.object.short_description ||
	    actual[0].extra_descriptions.size() != 1 || !rendered.detail.description ||
	    actual[0].description != rendered.object.description ||
	    actual[0].short_description != rendered.object.short_description ||
	    actual[0].extra_descriptions[0].description != rendered.detail.description)
		return false;
	std::copy(endpoint.after.begin(), endpoint.after.end(), rendered.object.value);
	add_coins(&rendered.object, 0, 0, 0, 0);
	if (!rendered.object.description || !rendered.object.short_description ||
	    !rendered.detail.description)
		return false;
	actual[0].description = rendered.object.description;
	actual[0].short_description = rendered.object.short_description;
	actual[0].extra_descriptions[0].description = rendered.detail.description;
	actual[0].weight = rendered.object.weight;
	std::copy(endpoint.after.begin(), endpoint.after.end(), actual[0].values.begin());
	std::vector<uint8_t> actual_bytes, expected_bytes;
	return player_item_snapshot_list_encode(actual, &actual_bytes) ==
		       player_snapshot_codec_result::ok &&
	       player_item_snapshot_list_encode(expected, &expected_bytes) ==
		       player_snapshot_codec_result::ok &&
	       actual_bytes == expected_bytes;
}

bool custody(const item_transfer_payload &pile, const item_transfer_result &result,
	     bool apply = true)
{
	const bool creation = pile.from_owner.type == item_owner_type::system;
	const bool consumed = pile.to_owner.type == item_owner_type::destruction;
	const auto &item = pile.items[0];
	const uint64_t revision = creation ? 1 : item.expected_item_revision + 1;
	if (!revision || result.item_count != 1 || result.root_item_uid != pile.selected_item_uid ||
	    result.max_item_revision != revision || result.corpse_revision ||
	    result.collector_catalog_changed ||
	    result.from_owner_revision != pile.expected_from_revision + 1 ||
	    result.to_owner_revision != (item_owner_identity_equal(pile.from_owner, pile.to_owner) ?
						 result.from_owner_revision :
						 pile.expected_to_revision + 1))
		return false;
	item_ownership_runtime_entry current = {};
	const bool exists = item_ownership_runtime_lookup(item.item_uid, &current);
	const item_ownership_runtime_entry target = { item.item_uid,
						      pile.target_root_item_uid,
						      0,
						      pile.to_owner,
						      revision,
						      result.to_owner_revision,
						      item.vnum,
						      consumed ? item_custody_state::destroyed :
								 item_custody_state::active };
	const bool exact = exists && current.item_revision == target.item_revision &&
			   current.root_item_uid == target.root_item_uid &&
			   current.parent_item_uid == 0 && current.vnum == target.vnum &&
			   current.state == target.state &&
			   current.owner_revision == target.owner_revision &&
			   item_owner_identity_equal(current.owner, target.owner);
	if (!exact &&
	    (creation ?
		     exists :
		     (!exists || current.item_revision != item.expected_item_revision ||
		      current.root_item_uid != item.root_item_uid || current.parent_item_uid != 0 ||
		      current.vnum != item.vnum || current.state != item_custody_state::active ||
		      !item_owner_identity_equal(current.owner, pile.from_owner))))
		return false;
	uint64_t from = 0, to = 0;
	return item_ownership_runtime_owner_revision(pile.from_owner, &from) &&
	       item_ownership_runtime_owner_revision(pile.to_owner, &to) &&
	       (!apply || ((exact || item_ownership_runtime_hydrate_many_atomic(&target, 1)) &&
			   item_ownership_runtime_hydrate_owner(
				   pile.from_owner, std::max(from, result.from_owner_revision)) &&
			   item_ownership_runtime_hydrate_owner(
				   pile.to_owner, std::max(to, result.to_owner_revision))));
}
}

bool coin_physical_publication_room_safe(int room, P_obj money)
{
	return money && money->type == ITEM_MONEY && !money->contains && !money->affects &&
	       !money->nevents && !money->hitched_to && !money->trap_eff && !money->trap_dam &&
	       !money->trap_charge && !money->trap_level && room >= 0 && room <= top_of_world &&
	       !IS_WATER_ROOM(room) && world[room].chance_fall <= 0 &&
	       world[room].sector_type != SECT_NO_GROUND &&
	       world[room].sector_type != SECT_UNDRWLD_NOGROUND && money->z_cord == 0 &&
	       !IS_ARTIFACT(money) && !IS_SET(money->extra_flags, ITEM_TRANSIENT | ITEM_LIT);
}

bool coin_physical_publication_publish(P_char actor, const critical_operation_id &id,
				       const coin_transfer_payload &payload,
				       const coin_transfer_result &result, const uint8_t *, size_t)
{
	if (!economic_gameplay_authority::active() || !actor || !IS_PC(actor))
		return false;
	const bool drop = payload.source.change.type == critical_command_type::account_bank &&
			  payload.destination.change.type == critical_command_type::item_transfer;
	const bool pickup = payload.destination.change.type ==
				    critical_command_type::account_bank &&
			    payload.source.change.type == critical_command_type::item_transfer;
	if (!drop && !pickup)
		return false;
	const auto &endpoint = drop ? payload.destination : payload.source;
	const auto &pile_result = result.piles[drop ? 1 : 0];
	item_transfer_payload pile = {};
	currency_command_payload wallet = {};
	if (!item_transfer_command_decode_payload(endpoint.change, &pile) || pile.item_count != 1 ||
	    pile.multi_root || pile.target_parent_item_uid || pile.items[0].parent_item_uid ||
	    pile.target_root_item_uid != pile.selected_item_uid ||
	    pile.items[0].root_item_uid != pile.selected_item_uid ||
	    !currency_command_decode_payload(
		    drop ? payload.source.change : payload.destination.change, &wallet) ||
	    wallet.pid != static_cast<uint32_t>(GET_PID(actor)) ||
	    wallet.reason != currency_reason_type::coin_transfer ||
	    (drop ? (pile.from_owner.type != item_owner_type::system ||
		     pile.to_owner.type != item_owner_type::room) :
		    (pile.from_owner.type != item_owner_type::room ||
		     (pile.to_owner.type != item_owner_type::destruction &&
		      !item_owner_identity_equal(pile.from_owner, pile.to_owner)))))
		return false;
	std::vector<player_item_snapshot> expected;
	if (player_item_snapshot_list_decode(pile.item_blob.data(), pile.item_blob_size,
					     &expected) != player_snapshot_codec_result::ok ||
	    expected.size() != 1 || expected[0].object_uid != pile.selected_item_uid ||
	    expected[0].vnum != pile.items[0].vnum || expected[0].type != ITEM_MONEY ||
	    expected[0].equipment_slot != -1 || !expected[0].dynamic_affects.empty() ||
	    IS_SET(expected[0].extra_flags, ITEM_TRANSIENT | ITEM_LIT) ||
	    expected[0].string_mask != (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3))
		return false;
	const uint64_t room_vnum = drop ? pile.to_owner.id : pile.from_owner.id;
	if (!room_vnum || room_vnum > INT_MAX)
		return false;
	const int room = real_room(static_cast<int>(room_vnum));
	if (room < 0 || room > top_of_world)
		return false;
	critical_command canonical;
	std::array<uint8_t, COIN_TRANSFER_RESULT_BYTES> receipt;
	if (!coin_transfer_command_build(&canonical, id, payload, critical_source_site::command,
					 critical_deadline_class::interactive) ||
	    !coin_transfer_command_encode_result(payload, result, &receipt))
		return false;
	coin_transfer_result checked = {};
	if (!coin_transfer_command_decode_result(payload, receipt.data(), receipt.size(), &checked))
		return false;
	const auto &wallet_endpoint = drop ? payload.source : payload.destination;
	const auto &wallet_result = result.wallets[drop ? 0 : 1];
	if (wallet_endpoint.change.expected_revisions.size() != 2 ||
	    wallet_result.wallet_revision !=
		    wallet_endpoint.change.expected_revisions[0].revision + 1 ||
	    wallet_result.bank_revision !=
		    wallet_endpoint.change.expected_revisions[1].revision + 1 ||
	    !std::equal(wallet_endpoint.after.begin(), wallet_endpoint.after.end(),
			wallet_result.wallet.amount.begin()))
		return false;
	canonical.payload.insert(canonical.payload.end(), receipt.begin(), receipt.end());
	std::array<unsigned char, SHA256_DIGEST_LENGTH> digest;
	SHA256(canonical.payload.data(), canonical.payload.size(), digest.data());
	publication *stage = nullptr, *empty = nullptr;
	for (auto &candidate : publications)
	{
		if (candidate.used && candidate.uid == pile.selected_item_uid &&
		    candidate.id.bytes != id.bytes)
			return false;
		if (!candidate.used && !empty)
			empty = &candidate;
		if (candidate.used && candidate.id.bytes == id.bytes)
			stage = &candidate;
	}
	if (!stage)
	{
		if (!empty)
			return false;
		stage = empty;
		stage->used = true;
		stage->id = id;
		stage->uid = pile.selected_item_uid;
		stage->digest = digest;
	}
	if (stage->digest != digest || stage->entered)
		return false;
	struct entry_guard
	{
		publication &stage;
		~entry_guard() { stage.entered = false; }
	} guard{ *stage };
	stage->entered = true;
	if ((stage->handler_started && !stage->handler_returned) ||
	    (stage->amount_started && !stage->amount_returned) ||
	    (stage->materialize_started && !stage->materialize_returned))
		return false;
	bool duplicate = false;
	P_obj money = find_unique(pile.selected_item_uid, &duplicate);
	if (duplicate)
		return false;
	const bool consumed = pile.to_owner.type == item_owner_type::destruction;
	if (!custody(pile, pile_result, false))
		return false;
	if (stage->complete || stage->handler_returned || stage->amount_returned)
	{
		stage->complete = custody(pile, pile_result) &&
				  (consumed ? !money :
					      money && OBJ_IN_ROOM(money, room) &&
						      bytes_match(money, expected));
		return stage->complete;
	}
	if (!money && drop && !stage->handler_started)
	{
		const player_load_item_identity identity = {
			1,
			0,
			1,
			static_cast<uint16_t>(PLAYER_LOAD_ITEM_OVERRIDE_ALL |
					      PLAYER_LOAD_ITEM_OVERRIDE_RUNTIME),
			pile.selected_item_uid,
			pile.selected_item_uid,
			0,
			pile.to_owner,
			pile_result.max_item_revision,
			pile_result.to_owner_revision,
			item_custody_state::active
		};
		const std::vector<player_load_item_identity> identities{ identity };
		std::vector<P_obj> roots;
		player_load_item_materialize_metrics metrics;
		stage->materialize_started = true;
		if (!player_load_item_graph_materialize_detached(
			    expected, identities, pile.to_owner, pile_result.to_owner_revision,
			    false, true, &roots, &metrics) ||
		    roots.size() != 1)
			return false;
		stage->materialize_returned = true;
		money = find_unique(pile.selected_item_uid, &duplicate);
	}
	if (!money || duplicate || !coin_physical_publication_room_safe(room, money) ||
	    (drop ? (!OBJ_NOWHERE(money) || !bytes_match(money, expected)) :
		    (!OBJ_IN_ROOM(money, room) ||
		     !(consumed ? bytes_match(money, expected) :
				  source_matches(money, endpoint, expected)))))
		return false;
	if (!custody(pile, pile_result))
		return false;
	if (!drop && !consumed)
	{
		stage->amount_started = true;
		std::copy(endpoint.after.begin(), endpoint.after.end(), money->value);
		add_coins(money, 0, 0, 0, 0);
		stage->amount_returned = true;
	}
	else
	{
		stage->handler_started = true;
		if (drop)
			obj_to_room(money, room);
		else
			extract_obj(money, FALSE);
		stage->handler_returned = true;
	}
	money = find_unique(pile.selected_item_uid, &duplicate);
	stage->complete = !duplicate && (consumed ? !money :
						    money && OBJ_IN_ROOM(money, room) &&
							    bytes_match(money, expected));
	return stage->complete;
}

void coin_physical_publication_release(const critical_operation_id &id) noexcept
{
	for (auto &entry : publications)
		if (entry.used && entry.id.bytes == id.bytes && !entry.entered)
			entry = {};
}
