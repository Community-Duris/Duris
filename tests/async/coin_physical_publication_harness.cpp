// Production publisher, capture/codec and runtime custody; explicit placement,
// renderer and materializer seams. No handler/backend/restart qualification.
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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string_view>

P_obj object_list = nullptr;
P_index obj_index = nullptr;
P_room world = nullptr;
extern const int top_of_world = 1;
extern const int top_of_objt = 0;
namespace
{
obj_data money = {}, duplicate_money = {};
extra_descr_data detail = {};
char_data actor = {};
pc_only_data pc = {};
room_data rooms[2] = {};
index_data objects[1] = {};
std::string_view scenario;
unsigned int placements = 0, placement_tails = 0, extractions = 0, extraction_tails = 0;
unsigned int materializations = 0, amounts = 0;
bool fault = true;
const coin_transfer_payload *pending_payload = nullptr;
const coin_transfer_result *pending_receipt = nullptr;
critical_operation_id pending_id = {};
bool nested_refused = false;
bool check(bool okay, const char *message)
{
	if (!okay)
		std::fprintf(stderr, "ASSERTION FAILED: %s\n", message);
	return okay;
}
void appearance(P_obj object)
{
	char line[64];
	std::snprintf(line, sizeof(line), "fixture coins %d", object->value[0]);
	if (object->description)
		str_free(object->description);
	if (object->short_description)
		str_free(object->short_description);
	object->description = str_dup(line);
	object->short_description = str_dup(line);
	if (object->ex_description)
	{
		if (object->ex_description->description)
			str_free(object->ex_description->description);
		object->ex_description->description = str_dup(line);
	}
	object->str_mask |= STRUNG_DESC1 | STRUNG_DESC2;
	object->weight = 0;
}
}

bool economic_gameplay_authority::active()
{
	return true;
}
char *str_dup(const char *text)
{
	char *copy = static_cast<char *>(std::malloc(std::strlen(text) + 1));
	if (!copy)
		throw std::bad_alloc();
	std::strcpy(copy, text);
	return copy;
}
void str_free(const char *text)
{
	std::free(const_cast<char *>(text));
}
int real_room(const int vnum)
{
	return vnum == 101 ? 1 : -1;
}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}
void logit(const char *, const char *, ...) {}
void add_coins(P_obj object, int copper, int silver, int gold, int platinum)
{
	object->value[0] += copper;
	object->value[1] += silver;
	object->value[2] += gold;
	object->value[3] += platinum;
	appearance(object);
	if (object == &money)
	{
		++amounts;
		if (fault && scenario == "amount_exception")
			throw std::bad_alloc();
	}
}
void obj_to_room(P_obj object, int room)
{
	++placements;
	object->loc_p = LOC_ROOM;
	object->loc.room = room;
	world[room].contents = object;
	if (scenario == "reentry")
		nested_refused = !coin_physical_publication_publish(
			&actor, pending_id, *pending_payload, *pending_receipt, nullptr, 0);
	if (fault && scenario == "placement_exception")
		throw std::bad_alloc();
	++placement_tails;
}
void extract_obj(P_obj object, int)
{
	++extractions;
	if (object != &money)
		std::abort();
	object_list = nullptr;
	world[1].contents = nullptr;
	if (fault && scenario == "extraction_exception")
		throw std::bad_alloc();
	++extraction_tails;
}
bool player_load_item_graph_materialize_detached(
	const std::vector<player_item_snapshot> &items,
	const std::vector<player_load_item_identity> &identities, const item_owner_identity &,
	uint64_t, bool hydrate, bool complete, std::vector<P_obj> *roots,
	player_load_item_materialize_metrics *)
{
	++materializations;
	if (hydrate || !complete || items.size() != 1 || identities.size() != 1 ||
	    identities[0].item_uid != money.obj_uid ||
	    !(identities[0].override_mask & PLAYER_LOAD_ITEM_OVERRIDE_RUNTIME))
		std::abort();
	object_list = &money;
	if (fault && scenario == "materialize_exception")
		throw std::bad_alloc();
	roots->push_back(&money);
	return true;
}

int main(int argc, char **argv)
{
	if (argc != 2)
		return 2;
	scenario = argv[1];
	const bool pickup = scenario.starts_with("pickup") || scenario == "amount_exception" ||
			    scenario == "extraction_exception";
	const bool consumed = scenario == "pickup_consumed" || scenario == "extraction_exception";
	actor.only.pc = &pc;
	pc.pid = 42;
	actor.in_room = 1;
	world = rooms;
	obj_index = objects;
	rooms[1].number = 101;
	objects[0].virtual_number = 100;
	money.obj_uid = 99001;
	money.type = ITEM_MONEY;
	money.R_num = 0;
	money.name = const_cast<char *>("coin fixture");
	money.action_description = const_cast<char *>("literal action");
	money.str_mask = 0;
	money.ex_description = &detail;
	detail.keyword = const_cast<char *>("coin");
	money.value[0] = 4;
	money.value[4] = 18;
	money.timer[5] = 7;
	money.condition = 9;
	appearance(&money);
	money.loc_p = pickup ? LOC_ROOM : LOC_NOWHERE;
	money.loc.room = pickup ? 1 : -1;
	object_list = &money;
	if (pickup)
		rooms[1].contents = &money;
	const item_owner_identity room_owner{ item_owner_type::room, 101, 0 };
	const item_owner_identity system_owner{ item_owner_type::system, 0, 0 };
	const item_owner_identity destruction{ item_owner_type::destruction, 0, 0 };
	if (!item_ownership_runtime_hydrate_owner(room_owner, 1) ||
	    !item_ownership_runtime_hydrate_owner(system_owner, 1) ||
	    !item_ownership_runtime_hydrate_owner(destruction, 1))
		return 2;
	if (pickup && !item_ownership_runtime_hydrate({ money.obj_uid, money.obj_uid, 0, room_owner,
							1, 1, 100, item_custody_state::active }))
		return 2;
	coin_transfer_payload payload;
	coin_transfer_result result;
	auto &pile_endpoint = pickup ? payload.source : payload.destination;
	auto &wallet_endpoint = pickup ? payload.destination : payload.source;
	pile_endpoint.before = pickup ? std::array<int32_t, 4>{ 4, 0, 0, 0 } :
					std::array<int32_t, 4>{};
	pile_endpoint.after = consumed ? std::array<int32_t, 4>{} :
			      pickup   ? std::array<int32_t, 4>{ 2, 0, 0, 0 } :
					 std::array<int32_t, 4>{ 4, 0, 0, 0 };
	wallet_endpoint.before = pickup ? std::array<int32_t, 4>{} :
					  std::array<int32_t, 4>{ 4, 0, 0, 0 };
	wallet_endpoint.after = pickup ? std::array<int32_t, 4>{ consumed ? 4 : 2, 0, 0, 0 } :
					 std::array<int32_t, 4>{};
	critical_operation_id id = {};
	id.bytes[0] = 9;
	currency_command_payload wallet = {};
	wallet.pid = 42;
	std::strcpy(wallet.account_name.data(), "coin_fixture");
	wallet.racewar = 1;
	wallet.reason = currency_reason_type::coin_transfer;
	wallet.wallet_delta.amount[0] = pickup ? (consumed ? 4 : 2) : -4;
	if (!currency_command_build(&wallet_endpoint.change, id, wallet, 1, 1,
				    critical_source_site::command,
				    critical_deadline_class::interactive))
		return 2;
	item_transfer_payload pile = {};
	pile.from_owner = pickup ? room_owner : system_owner;
	pile.to_owner = consumed ? destruction : room_owner;
	pile.expected_from_revision = pile.expected_to_revision = 1;
	pile.reason = pickup ? consumed ? item_transfer_reason::destruction :
					  item_transfer_reason::player_put :
			       item_transfer_reason::creation;
	pile.selected_item_uid = pile.target_root_item_uid = money.obj_uid;
	pile.item_count = 1;
	pile.items[0] = { money.obj_uid,
			  money.obj_uid,
			  0,
			  pickup ? 1 : ITEM_TRANSFER_ABSENT_REVISION,
			  100,
			  pickup ? item_custody_state::active : item_custody_state::absent };
	std::vector<player_item_snapshot> target;
	if (player_item_snapshot_tree_capture_literal(&money, &target, nullptr) !=
	    player_snapshot_capture_result::ok)
		return 2;
	target[0].equipment_slot = -1;
	if (pickup && !consumed)
	{
		target[0].values[0] = 2;
		target[0].description = target[0].short_description = "fixture coins 2";
		target[0].extra_descriptions[0].description = "fixture coins 2";
	}
	std::vector<uint8_t> bytes;
	if (player_item_snapshot_list_encode(target, &bytes) != player_snapshot_codec_result::ok)
		return 2;
	pile.item_blob_size = bytes.size();
	std::copy(bytes.begin(), bytes.end(), pile.item_blob.begin());
	if (!item_transfer_command_build(&pile_endpoint.change, id, pile,
					 critical_source_site::command,
					 critical_deadline_class::interactive))
		return 2;
	result.piles[pickup ? 0 : 1] = { money.obj_uid, 1, 2, 2, pickup ? 2U : 1U, 0 };
	result.wallets[pickup ? 1 : 0].wallet_revision = 2;
	result.wallets[pickup ? 1 : 0].bank_revision = 2;
	std::copy(wallet_endpoint.after.begin(), wallet_endpoint.after.end(),
		  result.wallets[pickup ? 1 : 0].wallet.amount.begin());
	if (scenario == "missing_uid" || scenario == "materialize_exception" ||
	    scenario == "pickup_missing")
		object_list = nullptr;
	if (scenario == "wrong_actor")
		pc.pid = 43;
	if (scenario == "wrong_placement")
	{
		money.loc_p = LOC_ROOM;
		money.loc.room = 0;
	}
	if (scenario == "stale_item")
		item_ownership_runtime_hydrate({ money.obj_uid, money.obj_uid, 0, room_owner, 3, 3,
						 100, item_custody_state::active });
	if (scenario == "masked_blob")
	{
		target[0].string_mask = STRUNG_DESC1 | STRUNG_DESC2;
		if (player_item_snapshot_list_encode(target, &bytes) !=
		    player_snapshot_codec_result::ok)
			return 2;
		pile.item_blob_size = bytes.size();
		std::copy(bytes.begin(), bytes.end(), pile.item_blob.begin());
		if (!item_transfer_command_build(&pile_endpoint.change, id, pile,
						 critical_source_site::command,
						 critical_deadline_class::interactive))
			return 2;
	}
	if (scenario == "malformed_result")
		result.piles[1].max_item_revision++;
	if (scenario == "literal_conflict")
		money.name = const_cast<char *>("changed literal");
	if (scenario == "pickup_opening_weight")
		++money.weight;
	if (scenario == "object_cycle")
		money.next = &money;
	if (scenario == "duplicate_uid")
	{
		duplicate_money = money;
		money.next = &duplicate_money;
	}
	if (scenario == "conflicting_bytes")
		money.value[4]++;
	if (scenario == "conflicting_custody")
		item_ownership_runtime_hydrate({ money.obj_uid,
						 money.obj_uid,
						 0,
						 { item_owner_type::player, 99, 0 },
						 9,
						 9,
						 100,
						 item_custody_state::active });
	if (scenario == "water")
		rooms[1].sector_type = SECT_UNDERWATER;
	if (scenario == "falling")
		rooms[1].chance_fall = 1;
	P_char publisher_actor = scenario == "offline_actor" ? nullptr : &actor;
	pending_payload = &payload;
	pending_receipt = &result;
	pending_id = id;
	auto publish = [&](const coin_transfer_result &receipt)
	{
		try
		{
			return coin_physical_publication_publish(publisher_actor, id, payload,
								 receipt, nullptr, 0);
		}
		catch (...)
		{
			return false;
		} // Same retention boundary as actual currency owner.
	};
	const bool first = publish(result);
	const bool uncertain =
		scenario == "placement_exception" || scenario == "extraction_exception" ||
		scenario == "amount_exception" || scenario == "materialize_exception";
	const bool refused = uncertain || scenario == "duplicate_uid" ||
			     scenario == "conflicting_bytes" || scenario == "conflicting_custody" ||
			     scenario == "water" || scenario == "falling" ||
			     scenario == "offline_actor" || scenario == "pickup_missing" ||
			     scenario == "wrong_actor" || scenario == "wrong_placement" ||
			     scenario == "stale_item" || scenario == "masked_blob" ||
			     scenario == "malformed_result" || scenario == "literal_conflict" ||
			     scenario == "object_cycle" || scenario == "pickup_opening_weight";
	bool okay = check(first != refused, "first publication matches required retention");
	if (scenario == "reentry")
		okay &= check(
			nested_refused && placements == 1,
			"native callback reentry retains original stage without repeated effects");
	fault = false;
	if (uncertain)
	{
		okay &= check(!publish(result),
			      "matching target cannot discharge unfinished native tail");
		okay &= check(placements <= 1 && extractions <= 1 && materializations <= 1 &&
				      amounts <= 1,
			      "uncertain handler never reruns or creates replacement value");
	}
	else if (!refused)
	{
		okay &= check(publish(result),
			      "same original result retries idempotently before ACK");
		okay &= check(placements == (pickup ? 0U : 1U) &&
				      extractions == (consumed ? 1U : 0U) &&
				      amounts == (pickup && !consumed ? 1U : 0U),
			      "native effect executes exactly once");
		if (scenario == "changed_receipt")
		{
			auto changed = result;
			changed.wallets[0].bank.amount[0]++;
			okay &= check(
				!publish(changed),
				"changed original receipt cannot replace completed physical effect");
		}
	}
	if (refused && !uncertain)
		okay &= check(!placements && !extractions && !materializations && !amounts,
			      "admission proof failure preserves native effects");
	if (scenario == "pickup_opening_weight")
	{
		item_ownership_runtime_entry opening = {};
		okay &= check(
			money.weight == 1 && money.value[0] == 4 &&
				item_ownership_runtime_lookup(money.obj_uid, &opening) &&
				opening.item_revision == 1 && opening.owner_revision == 1 &&
				opening.state == item_custody_state::active &&
				item_owner_identity_equal(opening.owner, room_owner),
			"corrupt opening weight refuses before native denomination or custody mutation");
	}
	coin_physical_publication_release(id);
	str_free(money.description);
	str_free(money.short_description);
	str_free(detail.description);
	item_ownership_runtime_reset();
	std::printf(
		"OBSERVE case=%s placements=%u tails=%u extracts=%u tails=%u materializations=%u amounts=%u\n",
		argv[1], placements, placement_tails, extractions, extraction_tails,
		materializations, amounts);
	return okay ? 0 : 1;
}
