// Actual shop callback/producer code with explicit native-placement seams.
// This component fixture does not qualify handler physics or durable replay.
#include "../../src/economy/shop.c"
#include "item/item_ownership_runtime.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "world/handler.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string_view>

P_index mob_index = nullptr;
P_index obj_index = nullptr;
P_room world = nullptr;
extern const int top_of_world = 1;
extern const int top_of_objt = 0;
struct str_app_type str_app[1] = {};
P_obj object_list = nullptr;

namespace
{
char_data actor = {}, keeper = {};
pc_only_data pc = {};
npc_only_data npc = {};
room_data rooms[2] = {};
index_data mobs[1] = {}, objects[1] = {};
obj_data item = {}, container = {}, child = {};
shop_data shop = {};
critical_command command = {};
std::string_view case_name;
bool fault = true;
unsigned int submits = 0, placements = 0, destructions = 0, audit_calls = 0;
unsigned int notices = 0, balance_calls = 0, custody_calls = 0;
unsigned int placement_tails = 0, destruction_tails = 0, nesting_tails = 0;
unsigned int detachments = 0, detachment_tails = 0;

bool check(bool condition, const char *message)
{
	if (!condition)
		std::fprintf(stderr, "ASSERTION FAILED: %s\n", message);
	return condition;
}
}

P_char find_player_by_pid(int pid)
{
	return pid == 42 ? &actor : nullptr;
}
int real_room(const int room)
{
	return room == 101 ? 1 : -1;
}
int STAT_INDEX(int)
{
	return 0;
}
int container_total_weight(P_obj)
{
	return 0;
}
P_obj read_object(int, int)
{
	std::abort();
}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}
char *coin_stringv(int, int)
{
	static char text[] = "100 copper";
	return text;
}
void send_to_char(const char *text, P_char)
{
	if (std::strstr(text, "You now have") || std::strstr(text, "gives you") ||
	    std::strstr(text, "removed invalid stock"))
		++notices;
}
void do_tell(P_char, char *, int) {}
void act(const char *, int, P_char, P_obj, void *, int) {}
void statuslog(int, const char *, ...) {}
void wizlog(int, const char *, ...) {}
void logit(const char *, const char *, ...) {}
void persistence_alert(int, const char *, const char *, const char *, const char *, const char *,
		       const char *, ...)
{
}
void ADD_MONEY(P_char ch, int amount, const char *)
{
	GET_COPPER(ch) += amount;
}
int SUB_MONEY(P_char ch, int amount, int)
{
	GET_COPPER(ch) -= amount;
	return 0;
}
int sql_shop_sell(P_char, P_obj, int)
{
	++audit_calls;
	return 1;
}

void obj_from_char(P_obj object)
{
	++detachments;
	if (!OBJ_CARRIED(object))
		std::abort();
	object->loc.carrying->carrying = nullptr;
	if (fault && case_name == "detachment_exception")
		throw std::bad_alloc();
	object->loc_p = LOC_NOWHERE;
	object->loc.carrying = nullptr;
	++detachment_tails;
}
obj_to_char_result obj_to_char_checked(P_obj object, P_char owner)
{
	++placements;
	if (fault && case_name == "placement_refused")
		return obj_to_char_result::rejected;
	object->loc_p = LOC_CARRIED;
	object->loc.carrying = owner;
	owner->carrying = object;
	if (IS_PC(owner) && !object->g_key && GET_LEVEL(owner) < 57 && GET_PID(owner) < 10000000)
		object->g_key = 1;
	if (fault && case_name == "placement_exception")
		throw std::bad_alloc();
	++placement_tails;
	return obj_to_char_result::placed;
}
void obj_to_char(P_obj object, P_char owner)
{
	(void)obj_to_char_checked(object, owner);
}
void obj_to_obj(P_obj object, P_obj destination)
{
	++placements;
	if (fault && case_name == "container_refused")
		return;
	object->loc_p = LOC_INSIDE;
	object->loc.inside = destination;
	destination->contains = object;
	if (fault && case_name == "nesting_exception")
		throw std::bad_alloc();
	++nesting_tails;
}
bool obj_can_nest(P_obj object, P_obj destination)
{
	return object && destination && object != destination &&
	       destination->type == ITEM_CONTAINER;
}
void extract_obj(P_obj object, int)
{
	++destructions;
	if (object != &item)
		std::abort();
	object_list = container.obj_uid ? &container : nullptr;
	keeper.carrying = actor.carrying = nullptr;
	if (fault && case_name == "destruction_exception")
		throw std::bad_alloc();
	++destruction_tails;
}

critical_submit_result critical_command_coordinator_submit(critical_command submitted)
{
	++submits;
	command = std::move(submitted);
	return critical_submit_result::accepted;
}
bool currency_transaction_publish_balances(P_char, const char *, uint8_t, const currency_vector &,
					   const currency_vector &, uint64_t, uint64_t)
{
	++balance_calls;
	return true;
}
bool item_ownership_runtime_apply(const item_transfer_payload &, const item_transfer_result &)
{
	++custody_calls;
	return true;
}
bool item_ownership_runtime_lookup(uint64_t, item_ownership_runtime_entry *)
{
	return false;
}
bool shop_trade_runtime_can_advance(uint32_t, uint64_t, uint64_t)
{
	return true;
}
bool shop_trade_runtime_advance(uint32_t, uint64_t, uint64_t)
{
	return true;
}
shop_trade_payload_build_result shop_trade_runtime_build_payload(P_char, P_char, P_obj, P_obj,
								 P_obj, uint32_t, shop_trade_action,
								 int64_t, shop_trade_payload *)
{
	std::abort();
}
bool shop_trade_runtime_object_matches_payload(P_obj object, const shop_trade_payload &payload)
{
	std::vector<player_item_snapshot> rows;
	std::vector<uint8_t> bytes;
	return object && object->obj_uid == payload.selected_item_uid &&
	       !(fault && case_name == "payload_conflict") &&
	       player_item_snapshot_tree_capture(object, &rows, nullptr) ==
		       player_snapshot_capture_result::ok &&
	       player_item_snapshot_list_encode(rows, &bytes) == player_snapshot_codec_result::ok &&
	       bytes.size() == payload.item_blob_size &&
	       std::equal(bytes.begin(), bytes.end(), payload.item_blob.begin());
}

int main(int argc, char **argv)
{
	if (argc != 2)
		return 2;
	case_name = argv[1];
	actor.only.pc = &pc;
	pc.pid = 42;
	keeper.only.npc = &npc;
	keeper.specials.act = ACT_ISNPC;
	npc.R_num = 0;
	actor.player.name = const_cast<char *>("fixture-player");
	keeper.player.name = const_cast<char *>("fixture-keeper");
	actor.in_room = keeper.in_room = 1;
	GET_COPPER(keeper) = 100;
	world = rooms;
	mob_index = mobs;
	obj_index = objects;
	mobs[0].virtual_number = 700;
	objects[0].virtual_number = 800;
	rooms[1].people = &keeper;
	shop_index = &shop;
	number_of_shops = 1;
	shop.in_room = 101;
	shop.keeper = 0;
	shop.message_buy = shop.message_sell = const_cast<char *>("%s %s");
	item.obj_uid = 300;
	item.R_num = 0;
	item.type = ITEM_OTHER;
	item.short_description = const_cast<char *>("fixture item");
	item.str_mask = STRUNG_DESC2;
	item.loc_p = LOC_CARRIED;
	item.loc.carrying = &keeper;
	keeper.carrying = &item;
	object_list = &item;
	shop_trade_payload payload = {};
	payload.action = shop_trade_action::buy_existing;
	payload.player_pid = 42;
	payload.shop_id = 0;
	payload.keeper_vnum = 700;
	payload.expected_keeper_cash = 100;
	payload.price = 100;
	std::strcpy(payload.account_name.data(), "synthetic-shop-account");
	payload.expected_wallet_revision = 2;
	payload.expected_bank_revision = 3;
	payload.expected_shop_revision = 4;
	payload.selected_item_uid = payload.target_root_item_uid = 300;
	payload.item_count = 1;
	payload.item_blob_size = 1;
	payload.item_blob[0] = 0x5a;
	payload.items[0] = { 300, 300, 0, 6, 800, item_custody_state::active };
	payload.stock_item_uid = 300;
	payload.expected_stock_item_revision = 6;
	payload.stock_vnum = 800;
	if (case_name == "missing_object")
		object_list = nullptr;
	else if (case_name == "missing_keeper")
		rooms[1].people = nullptr;
	else if (case_name == "container_refused" || case_name == "nesting_exception")
	{
		container.obj_uid = 400;
		container.type = ITEM_CONTAINER;
		container.value[0] = -1;
		container.loc_p = LOC_CARRIED;
		container.loc.carrying = &actor;
		item.next = &container;
		payload.target_root_item_uid = payload.target_parent_item_uid = 400;
		payload.expected_target_parent_revision = 9;
		payload.action = shop_trade_action::buy_produced;
		payload.stock_item_uid = 900;
		payload.items[0].expected_item_revision = ITEM_TRANSFER_ABSENT_REVISION;
		payload.items[0].expected_state = item_custody_state::absent;
		item.loc_p = LOC_NOWHERE;
		item.loc.carrying = nullptr;
		keeper.carrying = nullptr;
	}
	else if (case_name == "destruction_exception" || case_name == "destroy_success")
	{
		payload.action = shop_trade_action::sell_destroy;
		payload.stock_item_uid = payload.expected_stock_item_revision = 0;
		payload.stock_vnum = 0;
		item.loc.carrying = &actor;
		actor.carrying = &item;
		keeper.carrying = nullptr;
	}
	else if (case_name == "store_success")
	{
		payload.action = shop_trade_action::sell_store;
		payload.stock_item_uid = payload.expected_stock_item_revision = 0;
		payload.stock_vnum = 0;
		item.loc.carrying = &actor;
		actor.carrying = &item;
		keeper.carrying = nullptr;
	}
	else if (case_name == "cleanup_success")
	{
		payload.action = shop_trade_action::discard_invalid;
		payload.price = 0;
	}
	else if (case_name == "tree_success")
	{
		child.obj_uid = 301;
		child.R_num = 0;
		child.type = ITEM_OTHER;
		child.value[2] = 27;
		child.timer[0] = 13;
		child.loc_p = LOC_INSIDE;
		child.loc.inside = &item;
		item.contains = &child;
		item.next = &child;
		payload.item_count = 2;
		payload.items[1] = { 301, 300, 300, 6, 800, item_custody_state::active };
	}
	else if (case_name == "byte_changed_retry")
		case_name = "placement_refused";
	else if (case_name != "payload_conflict" && case_name != "placement_refused" &&
		 case_name != "placement_exception" && case_name != "success" &&
		 case_name != "restored_success" && case_name != "detachment_exception")
		return 2;
	std::vector<player_item_snapshot> source_rows;
	std::vector<uint8_t> source_bytes;
	if (player_item_snapshot_tree_capture(&item, &source_rows, nullptr) !=
		    player_snapshot_capture_result::ok ||
	    player_item_snapshot_list_encode(source_rows, &source_bytes) !=
		    player_snapshot_codec_result::ok)
		return 2;
	payload.item_blob_size = source_bytes.size();
	std::copy(source_bytes.begin(), source_bytes.end(), payload.item_blob.begin());
	if (case_name == "restored_success")
	{
		item.loc.carrying = &actor;
		actor.carrying = &item;
		keeper.carrying = nullptr;
		item.g_key = 1;
		item.extra2_flags |= ITEM2_STOREITEM;
	}
#ifdef SHOP_TRADE_PUBLICATION_H
	bool admitted = shop_trade_transaction_submit_with_publication(
		&actor, payload, shop_trade_publish_physical, shop_trade_completion);
#else
	bool admitted = shop_trade_transaction_submit(&actor, payload, shop_trade_completion);
#endif
	if (!check(admitted, "actual shop admission"))
		return 1;
	shop_trade_result result = {};
	result.action = payload.action;
	result.wallet_revision = 3;
	result.bank_revision = 4;
	result.shop_revision = 5;
	result.player_owner_revision = 8;
	result.counterparty_owner_revision = 7;
	result.item_count = payload.item_count;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		result.item_uids[index] = payload.items[index].item_uid;
		result.item_revisions[index] =
			payload.action == shop_trade_action::buy_produced ? 1 : 7;
	}
	result.keeper_cash_recorded = true;
	result.keeper_cash = payload.action == shop_trade_action::discard_invalid ? 100 :
			     (payload.action == shop_trade_action::sell_destroy ||
			      payload.action == shop_trade_action::sell_store) ?
										    0 :
										    200;
	std::array<uint8_t, SHOP_TRADE_RESULT_BYTES> encoded = {};
	if (!shop_trade_command_encode_result(result, &encoded))
		return 2;
	critical_completion receipt = {};
	receipt.operation_id = command.operation_id;
	receipt.outcome = critical_apply_outcome::applied;
	receipt.durable_revision = 5;
	receipt.result_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), receipt.result_payload.begin());
	bool escaped = false;
	try
	{
		shop_trade_transaction_handle_completions(&receipt, 1);
	}
	catch (...)
	{
		escaped = true;
	}
	const bool successful = case_name == "success" || case_name == "destroy_success" ||
				case_name == "store_success" || case_name == "cleanup_success" ||
				case_name == "tree_success" || case_name == "restored_success";
	bool passed = check(!escaped, "actual physical callback exception escaped");
	if (successful)
		passed &= check(!shop_trade_transaction_player_busy(&actor) && notices == 1,
				"actual physical success not released once");
	else if (case_name == "placement_exception" || case_name == "destruction_exception" ||
		 case_name == "nesting_exception" || case_name == "detachment_exception")
	{
		passed &= check(shop_trade_transaction_player_busy(&actor) && notices == 0,
				"throwing native handler lost original obligation");
		fault = false;
		shop_trade_transaction_player_ready(&actor);
		passed &= check(shop_trade_transaction_player_busy(&actor) && notices == 0 &&
					submits == 1,
				"matching target bytes hid an incomplete native handler tail");
		if (case_name == "placement_exception")
			passed &= check(placements == 1 && placement_tails == 0,
					"uncertain placement was restarted or claimed complete");
		else if (case_name == "destruction_exception")
			passed &= check(destructions == 1 && destruction_tails == 0 &&
						audit_calls == 1,
					"uncertain destruction was restarted or claimed complete");
		else if (case_name == "nesting_exception")
			passed &= check(placements == 2 && nesting_tails == 0,
					"uncertain nesting was restarted or claimed complete");
		else
			passed &= check(detachments == 1 && detachment_tails == 0,
					"uncertain detachment was restarted or claimed complete");
	}
	else if (!std::strcmp(argv[1], "byte_changed_retry"))
	{
		passed &= check(shop_trade_transaction_player_busy(&actor) && notices == 0,
				"refused placement lost original owner");
		item.value[0] = 77;
		fault = false;
		shop_trade_transaction_player_ready(&actor);
		passed &= check(shop_trade_transaction_player_busy(&actor) && notices == 0 &&
					placements == 1 && OBJ_NOWHERE(&item),
				"changed native bytes were delivered after interrupted placement");
	}
	else
	{
		passed &=
			check(shop_trade_transaction_player_busy(&actor) && notices == 0,
			      "actual physical refusal lost committed owner or notified delivery");
		fault = false;
		object_list = &item;
		rooms[1].people = &keeper;
		if (destructions)
			object_list = nullptr;
		try
		{
			shop_trade_transaction_player_ready(&actor);
		}
		catch (...)
		{
			passed = false;
		}
		passed &= check(!shop_trade_transaction_player_busy(&actor) && notices == 1 &&
					balance_calls == 1 && custody_calls == 1 && submits == 1,
				"actual physical retry repeated debit/projection or lost delivery");
		if (case_name == "placement_exception")
			passed &= check(placements == 1, "already placed retry moved object twice");
		if (case_name == "destruction_exception")
			passed &= check(destructions == 1 && audit_calls == 1,
					"already destroyed retry repeated destruction/audit");
	}
	std::printf(
		"CASE physical_%s result=%s placements=%u destructions=%u notices=%u escaped=%d\n",
		argv[1], passed ? "pass" : "fail", placements, destructions, notices, escaped);
	shop_trade_transaction_reset_for_tests();
	produced_purchase_sequences.clear();
	return passed ? 0 : 1;
}
