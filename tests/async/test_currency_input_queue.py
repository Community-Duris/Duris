#!/usr/bin/env python3
"""Exercise selective input preservation around held currency publication."""

from pathlib import Path
import shlex
import subprocess
import tempfile

from _paths import ROOT, SRC, rel


def extract(source: Path, signature: str) -> str:
    """Return one complete C/C++ function beginning at ``signature``."""
    text = source.read_text(encoding="utf-8", errors="replace")
    start = text.index(signature)
    depth = 0
    for index in range(start, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start:index + 1]
    raise AssertionError(f"unbalanced braces reading {signature}")


COMM_PATH = SRC / "comm.c"
INTERP_PATH = SRC / "interp.c"
CURRENCY_PATH = SRC / "currency_transaction.c"
PROTOTYPES = (SRC / "prototypes.h").read_text(encoding="utf-8", errors="replace")
COMM = COMM_PATH.read_text(encoding="utf-8", errors="replace")
CURRENCY = CURRENCY_PATH.read_text(encoding="utf-8", errors="replace")
BANK_PUBLICATION = extract(SRC / "utility.c", "void publish_account_bank_balances_revision(")
DEPENDS = extract(INTERP_PATH, "bool cmd_depends_on_currency_transaction(int cmd)")

for command in ("CMD_GET", "CMD_DROP", "CMD_PUT", "CMD_GIVE", "CMD_DEPOSIT", "CMD_WITHDRAW"):
    assert command in DEPENDS
assert "currency_transaction_player_busy(character)" in COMM
assert "currency_transaction_player_busy(P_char character)" in CURRENCY
assert "input_allowed_while_item_and_currency_pending" in COMM
assert "int get_pending_transaction_cmd_from_q(struct txt_q *, char *, bool, bool);" in PROTOTYPES

SEARCH = extract(INTERP_PATH, "int old_search_block(const char *argument")
COMMAND_NUMBER = extract(INTERP_PATH, "static int input_command_number(const char *input)")
ALLOWED = extract(INTERP_PATH, "bool input_allowed_while_currency_pending(const char *input)")
COMBINED_ALLOWED = extract(
    INTERP_PATH, "bool input_allowed_while_item_and_currency_pending(const char *input)"
)
GET_FROM_Q = extract(COMM_PATH, "int get_from_q(struct txt_q *queue, char *dest)")
GET_FILTERED = extract(
    COMM_PATH, "static int get_filtered_cmd_from_q(struct txt_q *queue, char *dest,"
)
GET_ITEM = extract(
    COMM_PATH, "int get_item_movement_cmd_from_q(struct txt_q *queue, char *dest)"
)
GET_PENDING = extract(
    COMM_PATH, "int get_pending_transaction_cmd_from_q(struct txt_q *queue, char *dest,"
)
GET_PLAYING = extract(
    COMM_PATH, "static int get_playing_cmd_from_q(P_char character, struct txt_q *queue,"
)
DISPATCH_PLAYING = extract(
    COMM_PATH, "static void dispatch_playing_command(P_char character, char *input)"
)

ACTOBJ = (SRC / "actobj.c").read_text()
COIN_TYPES = ACTOBJ[ACTOBJ.index("enum class coin_debit_action"):
                    ACTOBJ.index("static_assert(sizeof(coin_debit_context)")]
COIN_GIVE = "\n".join([
    COIN_TYPES,
    "static int coin_announcements = 0, coin_errors = 0;",
    "void send_to_char(const char *, P_char) { ++coin_errors; }",
    "void announce_coin_give(P_char, P_char recipient, const coin_give_credit_context &) { if (recipient) ++coin_announcements; }",
    extract(SRC / "actobj.c", "int64_t coin_debit_value("),
    extract(SRC / "actobj.c", "bool coin_give_completion("),
    extract(SRC / "actobj.c", "bool submit_coin_give("),
])

PRELUDE = r'''
#include "core/utils.h"
#include "account/account.h"
#include "economy/currency_transaction.h"
#include "sql/sql_player.h"
#include "item/item_ownership_runtime.h"
#include "item/item_movement_transaction.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "player/player_load_items.h"
#include "world/vnum.obj.h"
#include "classes/necromancy.h"
#include "net/comm.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <unordered_map>
#include "persistence/persistence_checkpoint.h"

#define CMD_NONE 0
#define CMD_DROP 1
#define CMD_PUT 2
#define CMD_GIVE 3
#define CMD_DEPOSIT 4
#define CMD_WITHDRAW 5
#define CMD_INVENTORY 6
#define CMD_SCORE 7
#define CMD_LOOK 8
#define CMD_SAY 9
#define CMD_GET 10

static const char *command[] = {
	"drop", "put", "give", "deposit", "withdraw", "inventory", "score", "look",
	"say", "get", "\n"
};

P_char character_list = NULL;
P_desc descriptor_list = NULL;
static room_data test_rooms[2] = {};
P_room world = test_rooms;
static critical_command submitted_command = {};
static bool coordinator_fenced = false;
static bool item_pending = false;
static int submission_count = 0;

void logit(const char *, const char *, ...) {}
// money_to_inventory() reports a wallet conversion it could not even submit.
void persistence_alert(int, const char *, const char *, const char *, const char *, const char *,
		       const char *, ...)
{
}
void __free(void *memory, const char *, int) { free(memory); }
void gmcp_char_vitals(P_char) {}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	abort();
}

const char *get_account_name_safe(P_char character)
{
	return character && GET_PID(character) == 45 ? "recipient_account" : "queue_account";
}

P_char find_player_by_pid(int pid)
{
	for (P_char character = character_list; character; character = character->next)
		if (character->only.pc && GET_PID(character) == pid)
			return character;
	return nullptr;
}

critical_submit_result critical_command_coordinator_submit(critical_command command_value)
{
	assert(!coordinator_fenced);
	coordinator_fenced = true;
	++submission_count;
	submitted_command = std::move(command_value);
	return critical_submit_result::accepted;
}

bool critical_command_coordinator_is_fenced(const critical_entity_key &,
					     critical_operation_id *)
{
	return coordinator_fenced;
}

bool critical_command_coordinator_get_completed(const critical_operation_id &,
						 critical_completion *)
{
	return false;
}

bool item_movement_transaction_player_busy(P_char)
{
	return item_pending;
}

bool input_allowed_while_item_moving(const char *input)
{
	return input && strcmp(input, "inventory");
}

void command_interpreter(P_char character, char *input);
void process_with_paging(P_char, char *) { abort(); }

int old_search_block(const char *argument, const uint begin, uint length, const char **list,
		     const int mode);
bool input_allowed_while_currency_pending(const char *input);
bool input_allowed_while_item_and_currency_pending(const char *input);
int get_pending_transaction_cmd_from_q(struct txt_q *queue, char *dest, bool item_pending,
				       bool currency_pending);
'''

COIN_PILES = r'''
static std::vector<P_obj> live_items;
P_obj object_list = nullptr;
void obj_to_char(P_obj, P_char);
static uint64_t next_coin_uid = 1000, hidden_container = 0;
static int put_announcements = 0;
const char *coin_names[] = {"copper", "silver", "gold", "platinum"};
void str_free(const char *text) { free(const_cast<char *>(text)); }
char *str_dup(const char *text) { return strdup(text); }
#define APPENDF(buf, ...) snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), __VA_ARGS__)
P_obj find_live_item_uid(uint64_t uid)
{
    if (uid == hidden_container) return nullptr;
    for (P_obj item : live_items) if (item->obj_uid == uid) return item;
    return nullptr;
}
void obj_to_obj(P_obj item, P_obj container)
{
    item->loc_p = LOC_INSIDE;
    item->loc.inside = container;
    item->next_content = container->contains;
    container->contains = item;
}
void extract_obj(P_obj item, bool)
{
    if (OBJ_INSIDE(item)) {
        P_obj *at = &item->loc.inside->contains;
        while (*at && *at != item) at = &(*at)->next_content;
        assert(*at == item);
        *at = item->next_content;
    }
    P_obj *link = &object_list;
    while (*link && *link != item) link = &(*link)->next;
    if (*link) *link = item->next;
    live_items.erase(std::remove(live_items.begin(), live_items.end(), item), live_items.end());
    free(item->description);
    free(item->short_description);
    delete item;
}
player_snapshot_capture_result player_item_snapshot_tree_capture(P_obj item,
    std::vector<player_item_snapshot> *items, size_t *)
{
    player_item_snapshot snapshot = {};
    snapshot.object_uid = item->obj_uid;
    snapshot.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
    snapshot.equipment_slot = -1;
    snapshot.vnum = VOBJ_COINS;
    snapshot.type = item->type;
    snapshot.name = "coins";
    snapshot.string_mask = 1;
    std::copy(std::begin(item->value), std::end(item->value), snapshot.values.begin());
    items->push_back(snapshot);
    return player_snapshot_capture_result::ok;
}
bool locker_owner_for_container(P_char, P_obj, item_owner_identity *) { return false; }
void finish_coin_put_publication(P_char, P_obj, const coin_debit_context &) { ++put_announcements; }
''' + extract(SRC / "handler.c", "void add_coins(") + r'''
P_obj create_money(int copper, int silver, int gold, int platinum)
{
    P_obj item = new obj_data{};
    item->obj_uid = next_coin_uid++;
    item->type = ITEM_MONEY;
    item->loc_p = LOC_NOWHERE;
    add_coins(item, copper, silver, gold, platinum);
    item->next = object_list;
    object_list = item;
    live_items.push_back(item);
    return item;
}
bool player_load_item_graph_materialize_detached(const std::vector<player_item_snapshot> &items,
    const std::vector<player_load_item_identity> &, const item_owner_identity &, uint64_t, bool, bool,
    std::vector<P_obj> *roots, player_load_item_materialize_metrics *)
{
    assert(items.size() == 1);
    P_obj item = create_money(items[0].values[0], items[0].values[1], items[0].values[2], items[0].values[3]);
    item->obj_uid = items[0].object_uid;
    roots->push_back(item);
    return true;
}
''' + "\n".join(extract(SRC / "actobj.c", signature) for signature in [
    "bool coin_put_destination_available(", "bool coin_put_destination_custody(",
    "static bool prepare_coin_pile(", "static bool publish_coin_pile(",
    "bool coin_put_custody_completion(", "static bool submit_coin_put(",
])

COIN_GET = (ACTOBJ[ACTOBJ.index("struct synchronous_get_item"):ACTOBJ.index("struct drop_movement_context")]
            + ACTOBJ[ACTOBJ.index("struct coin_pickup_context"):ACTOBJ.index("static bool coin_get_completion(")]
            + r'''
static std::unordered_map<uint32_t, bulk_get_state> bulk_gets;
static bool item_get_ack_publication = false, item_get_deferred = false, item_get_rejected = false;
static int bulk_total = 0;
static bool submit_coin_get(P_char, P_obj, P_obj, int);
void act(const char *, int, P_char, P_obj, void *, int) {}
void writeCorpse(P_obj) {}
void mark_player_dirty_components(int, player_component_mask_t) {}
bool get_item_source_owner(P_char, P_obj money, P_obj container, item_owner_identity *owner)
{
    item_ownership_runtime_entry row;
    if (!item_ownership_runtime_lookup(money->obj_uid, &row)) {
        if (!container || !item_ownership_runtime_lookup(container->obj_uid, &row)) return false;
    }
    *owner = row.owner;
    return true;
}
P_obj resolve_synchronous_get_item(const synchronous_get_item &item, const bulk_get_state &, P_obj)
{ return find_live_item_uid(item.item_uid); }
bool bulk_get_source_matches(const bulk_get_state &, P_obj container, P_obj money)
{ return money && OBJ_INSIDE(money) && money->loc.inside == container; }
void MakeScrap(P_char, P_obj) { abort(); }
void do_get_finalize_container_success(P_char actor, P_char, P_obj container, P_obj money,
    int &, bool &, bool, const char *)
{
    item_get_deferred = submit_coin_get(actor, money, container, 1);
    item_get_rejected = !item_get_deferred;
}
void do_get_finalize_room_item(P_char, P_obj, bool &, int &) { abort(); }
void finish_bulk_get(P_char, uint32_t pid)
{
    bulk_total = bulk_gets.at(pid).total;
    bulk_gets.erase(pid);
}
'''
            + extract(SRC / "actobj.c", "static bool bulk_get_source_available(")
            + extract(SRC / "actobj.c", "static bool finish_bulk_get_after_commit(")
            + extract(SRC / "actobj.c", "static bool coin_get_completion(")
            + ACTOBJ[ACTOBJ.index("struct coin_admission_context"):
                     ACTOBJ.index("static void coin_admission_completion(")]
            + r'''
static coin_admission_context admission_context;
static item_movement_completion_fn admission_callback = nullptr;
static uint64_t admitted_parent = 0;
static item_owner_identity admitted_owner;
bool item_movement_transaction_submit(P_char, P_obj, P_obj parent,
    const item_owner_identity &from, const item_owner_identity &to,
    item_transfer_reason reason, int64_t, item_movement_completion_fn callback,
    const void *context, size_t size, P_obj, item_movement_reject *)
{
    assert(item_owner_identity_equal(from, to));
    assert(reason == item_transfer_reason::player_get && size == sizeof(admission_context));
    memcpy(&admission_context, context, size);
    admission_callback = callback;
    admitted_parent = parent ? parent->obj_uid : 0;
    admitted_owner = from;
    item_pending = true;
    return true;
}
'''
            + extract(SRC / "actobj.c", "static void coin_admission_completion(")
            + extract(SRC / "actobj.c", "static bool submit_coin_get(P_char actor, P_obj money, P_obj container, int showit)\n{"))

DRIVER = r'''
static void push(struct txt_q *queue, const char *text)
{
	struct txt_block *block = (struct txt_block *)malloc(sizeof(struct txt_block));
	block->text = strdup(text);
	block->next = NULL;
	if (queue->head)
		queue->tail->next = block;
	else
		queue->head = block;
	queue->tail = block;
}

static void drain(struct txt_q *queue)
{
	while (queue->head)
	{
		struct txt_block *next = queue->head->next;
		free(queue->head->text);
		free(queue->head);
		queue->head = next;
	}
	queue->tail = NULL;
}

static void expect_text(const char *got, const char *want)
{
	if (strcmp(got, want))
	{
		fprintf(stderr, "got '%s', want '%s'\n", got, want);
		exit(1);
	}
}

void obj_to_char(P_obj item, P_char ch)
{
    item->loc_p = LOC_CARRIED;
    item->loc.carrying = ch;
    item->next_content = ch->carrying;
    ch->carrying = item;
}
static int completion_count = 0;
static int score_dispatches = 0;
static int put_dispatches = 0;
static int drop_dispatches = 0;
static int give_dispatches = 0;
static int deposit_dispatches = 0;
static int withdraw_dispatches = 0;
static int final_invalid_reasons = 0;
static int transient_rejections = 0;

static void held_reward_completion(P_char actor, bool committed,
				   const currency_command_result &result, unsigned int error_code,
				   const uint8_t *, size_t)
{
	assert(actor && committed && error_code == 0);
	assert(result.wallet.amount[3] == 1 && result.wallet_revision == 2);
	assert(GET_PLATINUM(actor) == 1 && actor->only.pc->wallet_revision == 2);
	++completion_count;
}

void command_interpreter(P_char actor, char *input)
{
	assert(actor && input);
	if (!strcmp(input, "score"))
	{
		++score_dispatches;
		return;
	}
	if (currency_transaction_player_busy(actor))
	{
		++transient_rejections;
		return;
	}
	assert(GET_PLATINUM(actor) == 1 && actor->only.pc->wallet_revision == 2);
	if (!strcmp(input, "put all.coins satchel"))
		++put_dispatches;
	else if (!strcmp(input, "drop all.coins"))
		++drop_dispatches;
	else if (!strcmp(input, "give 1 platinum friend"))
		++give_dispatches;
	else if (!strcmp(input, "deposit all"))
		++deposit_dispatches;
	else if (!strcmp(input, "withdraw 1 platinum"))
		++withdraw_dispatches;
	else if (!strcmp(input, "give 2 platinum friend"))
	{
		assert(GET_PLATINUM(actor) < 2);
		++final_invalid_reasons;
	}
	else
		abort();
}

static int coin_callbacks = 0;
static bool coin_committed = false;
static bool coin_completed(P_char, bool committed, const coin_transfer_payload &,
	const coin_transfer_result &, unsigned int, const uint8_t *, size_t)
{
	++coin_callbacks;
	coin_committed = committed;
	return true;
}

int main()
{
	pc_only_data player = {};
	player.pid = 42;
	player.wallet_revision = 1;
	player.bank_revision = 1;
	char_data actor = {};
	actor.only.pc = &player;
	actor.player.racewar = 1;
	char account_name[] = "queue_account";
	acct_entry account = {};
	account.acct_name = account_name;
	descriptor_data descriptor = {};
	descriptor.account = &account;
	descriptor.character = &actor;
	descriptor.connected = CON_PLAYING;
	descriptor_list = &descriptor;
	GET_COPPER(&actor) = 5;
	character_list = &actor;
	currency_transaction_reset_for_tests();

	assert(!currency_transaction_player_busy(NULL));
	assert(currency_transaction_submit_wallet_value(
		&actor, 1000, currency_reason_type::wallet_reward, 100,
		critical_source_site::command, critical_deadline_class::interactive,
		held_reward_completion, NULL, 0));
	assert(submission_count == 1 && coordinator_fenced);
	assert(currency_transaction_player_busy(&actor));
	assert(!currency_transaction_can_submit(&actor));
	pc_only_data sibling_player = {};
	sibling_player.pid = 43;
	char_data sibling = {};
	sibling.only.pc = &sibling_player;
	sibling.player.racewar = actor.player.racewar;
	assert(currency_transaction_player_busy(&sibling));
	sibling.player.racewar = actor.player.racewar + 1;
	assert(!currency_transaction_player_busy(&sibling));

	/* The direct debit still hits the unchanged player/account fence. */
	assert(!currency_transaction_submit_wallet_value(
		&actor, -1, currency_reason_type::wallet_spend, 0,
		critical_source_site::command, critical_deadline_class::interactive, NULL,
		NULL, 0));
	assert(submission_count == 1);

	assert(!input_allowed_while_currency_pending("get all corpse"));
	assert(!input_allowed_while_currency_pending("put all.coins satchel"));
	assert(!input_allowed_while_currency_pending("  DROP all.coins"));
	assert(!input_allowed_while_currency_pending("gi 1 platinum friend"));
	assert(!input_allowed_while_currency_pending("deposit all"));
	assert(!input_allowed_while_currency_pending("withdraw 1 platinum"));
	assert(input_allowed_while_currency_pending("score"));
	assert(input_allowed_while_currency_pending("look"));
	assert(input_allowed_while_currency_pending("say waiting"));
	assert(!input_allowed_while_currency_pending(NULL));

	char dest[MAX_INPUT_LENGTH];
	struct txt_q queue = {};
	push(&queue, "put all.coins satchel");
	push(&queue, "score");
	push(&queue, "drop all.coins");
	push(&queue, "give 1 platinum friend");
	push(&queue, "deposit all");
	push(&queue, "withdraw 1 platinum");
	push(&queue, "give 2 platinum friend");
	assert(get_playing_cmd_from_q(&actor, &queue, dest));
	expect_text(dest, "score");
	dispatch_playing_command(&actor, dest);
	assert(score_dispatches == 1 && transient_rejections == 0);
	expect_text(queue.head->text, "put all.coins satchel");
	expect_text(queue.tail->text, "give 2 platinum friend");
	strcpy(dest, "untouched");
	assert(!get_playing_cmd_from_q(&actor, &queue, dest));
	expect_text(dest, "untouched");

	/* When both domains are pending, an input must be safe under both gates. */
	struct txt_q combined = {};
	item_pending = true;
	push(&combined, "inventory");
	push(&combined, "deposit all");
	push(&combined, "say waiting");
	assert(get_playing_cmd_from_q(&actor, &combined, dest));
	expect_text(dest, "say waiting");
	assert(!get_playing_cmd_from_q(&actor, &combined, dest));
	expect_text(combined.head->text, "inventory");
	expect_text(combined.tail->text, "deposit all");
	drain(&combined);
	item_pending = false;

	/* Release the held reward and publish the authoritative credited wallet. */
	currency_command_result result = {};
	result.wallet.amount[0] = 5;
	result.wallet.amount[3] = 1;
	result.wallet_revision = 2;
	result.bank_revision = 2;
	std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES> encoded = {};
	assert(currency_command_encode_result(result, &encoded));
	critical_completion completion = {};
	completion.operation_id = submitted_command.operation_id;
	completion.outcome = critical_apply_outcome::applied;
	completion.result_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), completion.result_payload.begin());
	coordinator_fenced = false;
	currency_transaction_handle_completions(&completion, 1);
	assert(completion_count == 1);
	assert(!currency_transaction_player_busy(&actor));
	assert(GET_COPPER(&actor) == 5 && GET_PLATINUM(&actor) == 1);

	const char *expected[] = {
		"put all.coins satchel", "drop all.coins", "give 1 platinum friend",
		"deposit all", "withdraw 1 platinum", "give 2 platinum friend"
	};
	for (const char *input : expected)
	{
		assert(get_playing_cmd_from_q(&actor, &queue, dest));
		expect_text(dest, input);
		dispatch_playing_command(&actor, dest);
	}
	assert(!get_playing_cmd_from_q(&actor, &queue, dest));
	assert(queue.head == NULL && queue.tail == NULL);
	assert(put_dispatches == 1 && drop_dispatches == 1 && give_dispatches == 1);
	assert(deposit_dispatches == 1 && withdraw_dispatches == 1);
	assert(final_invalid_reasons == 1 && transient_rejections == 0);

	/* A disconnected player's retained completion can predate the authoritative
	 * wallet loaded on re-entry. Finishing that callback must not restore money
	 * already spent, or erase money credited by a subsequent committed command. */
	assert(currency_transaction_submit_wallet_value(
		&actor, 10, currency_reason_type::wallet_reward, 101,
		critical_source_site::command, critical_deadline_class::interactive,
		nullptr, nullptr, 0));
	result.wallet.amount = {15, 0, 0, 1};
	result.wallet_revision = 3;
	result.bank_revision = 3;
	assert(currency_command_encode_result(result, &encoded));
	completion.operation_id = submitted_command.operation_id;
	std::copy(encoded.begin(), encoded.end(), completion.result_payload.begin());
	character_list = nullptr;
	coordinator_fenced = false;
	currency_transaction_handle_completions(&completion, 1);
	assert(currency_transaction_health_copy().retained_offline == 1);
	GET_COPPER(&actor) = 7;
	GET_PLATINUM(&actor) = 2;
	player.wallet_revision = 4;
	GET_BALANCE_GOLD(&actor) = 5;
	player.bank_revision = 4;
	character_list = &actor;
	currency_transaction_player_ready(&actor);
	assert(GET_COPPER(&actor) == 7 && GET_PLATINUM(&actor) == 2);
	assert(player.wallet_revision == 4);
	assert(GET_BALANCE_GOLD(&actor) == 5 && player.bank_revision == 4);
	assert(!currency_transaction_player_busy(&actor));
	assert(currency_transaction_health_copy().retained_offline == 0);

	// Real admission and completion use one coin command for both accounts.
	pc_only_data recipient_player = {};
	recipient_player.pid = 45;
	char_data recipient = {};
	recipient.only.pc = &recipient_player;
	recipient.player.racewar = 1;
	for (int scenario = 0; scenario < 4; ++scenario)
	{
		currency_transaction_reset_for_tests();
		coordinator_fenced = false;
		actor.next = &recipient;
		character_list = &actor;
		GET_PLATINUM(&actor) = 100;
		GET_COPPER(&actor) = GET_SILVER(&actor) = GET_GOLD(&actor) = 0;
		GET_PLATINUM(&recipient) = 10;
		player.wallet_revision = player.bank_revision = 1;
		recipient_player.wallet_revision = recipient_player.bank_revision = 1;
		coin_transfer_payload transfer;
		assert(currency_transaction_coin_wallet(&actor, -40000, &transfer.source));
		assert(currency_transaction_coin_wallet(&recipient, 40000, &transfer.destination));
		const int submitted_before = submission_count;
		assert(currency_transaction_submit_coin(&actor, transfer, coin_completed, nullptr, 0));
		assert(submission_count == submitted_before + 1);
		assert(submitted_command.type == critical_command_type::coin_transfer);
		assert(currency_transaction_player_busy(&actor));
		assert(currency_transaction_player_busy(&recipient));
		assert(GET_PLATINUM(&actor) == 100 && GET_PLATINUM(&recipient) == 10);
		coin_transfer_payload admitted;
		assert(coin_transfer_command_decode_payload(submitted_command, &admitted));
		assert(admitted.source.after[3] == 60 && admitted.destination.after[3] == 50);
		coin_transfer_result coin_result;
		coin_result.wallets[0].wallet.amount[3] = 60;
		coin_result.wallets[1].wallet.amount[3] = 50;
		for (auto &wallet : coin_result.wallets)
			wallet.wallet_revision = wallet.bank_revision = 2;
		std::array<uint8_t, COIN_TRANSFER_RESULT_BYTES> coin_bytes;
		assert(coin_transfer_command_encode_result(admitted, coin_result, &coin_bytes));
		critical_completion ack = {};
		ack.operation_id = submitted_command.operation_id;
		ack.outcome = scenario == 0 ? critical_apply_outcome::terminal_failure : critical_apply_outcome::applied;
		ack.error_code = scenario == 0 ? ESTALE : 0;
		ack.result_size = scenario == 0 ? 0 : coin_bytes.size();
		std::copy(coin_bytes.begin(), coin_bytes.end(), ack.result_payload.begin());
		if (scenario == 1)
			character_list = &recipient; // Sender detached before acknowledgement.
		if (scenario == 2)
			actor.next = nullptr; // Recipient detached before acknowledgement.
		if (scenario == 3)
		{
			// Re-entry already loaded a later wallet before this completion arrived.
			GET_PLATINUM(&recipient) = 80;
			recipient_player.wallet_revision = 3;
		}
		coordinator_fenced = false;
		const int callbacks_before = coin_callbacks;
		currency_transaction_handle_completions(&ack, 1);
		assert(coin_callbacks == callbacks_before + 1 && coin_committed == (scenario != 0));
		assert(currency_transaction_health_copy().pending == 0);
		assert(GET_PLATINUM(&actor) == (scenario == 0 || scenario == 1 ? 100 : 60));
		assert(GET_PLATINUM(&recipient) == (scenario == 0 || scenario == 2 ? 10 : scenario == 3 ? 80 : 50));
		currency_transaction_handle_completions(&ack, 1);
		assert(coin_callbacks == callbacks_before + 1);
		assert(submission_count == submitted_before + 1); // No second credit or refund.
	}
	// The actual gameplay helper shares admission and preserves existing change-making.
	currency_transaction_reset_for_tests();
	coordinator_fenced = false;
	actor.next = &recipient;
	character_list = &actor;
	GET_COPPER(&actor) = 100;
	GET_PLATINUM(&actor) = 0;
	GET_COPPER(&recipient) = 10;
	GET_PLATINUM(&recipient) = 0;
	player.wallet_revision = player.bank_revision = 1;
	recipient_player.wallet_revision = recipient_player.bank_revision = 1;
	coin_debit_context give = {};
	give.amount[0] = 40;
	give.action = coin_debit_action::give;
	give.room = recipient.in_room;
	const int admitted_before = submission_count;
	assert(submit_coin_give(&actor, &recipient, give));
	assert(submission_count == admitted_before + 1 && coin_announcements == 0);
	assert(GET_COPPER(&actor) == 100 && GET_COPPER(&recipient) == 10);
	coin_transfer_payload give_payload;
	assert(coin_transfer_command_decode_payload(submitted_command, &give_payload));
	assert((give_payload.source.after == std::array<int32_t, 4>{0, 6, 0, 0}));
	assert((give_payload.destination.after == std::array<int32_t, 4>{10, 4, 0, 0}));
	coin_transfer_result give_result;
	for (size_t i = 0; i < 4; ++i)
	{
		give_result.wallets[0].wallet.amount[i] = give_payload.source.after[i];
		give_result.wallets[1].wallet.amount[i] = give_payload.destination.after[i];
	}
	for (auto &wallet : give_result.wallets)
		wallet.wallet_revision = wallet.bank_revision = 2;
	std::array<uint8_t, COIN_TRANSFER_RESULT_BYTES> give_bytes;
	assert(coin_transfer_command_encode_result(give_payload, give_result, &give_bytes));
	critical_completion give_ack = {};
	give_ack.operation_id = submitted_command.operation_id;
	give_ack.outcome = critical_apply_outcome::applied;
	give_ack.result_size = give_bytes.size();
	std::copy(give_bytes.begin(), give_bytes.end(), give_ack.result_payload.begin());
	coordinator_fenced = false;
	currency_transaction_handle_completions(&give_ack, 1);
	assert(coin_announcements == 1 && coin_errors == 0);
	assert(GET_SILVER(&actor) == 6 && GET_COPPER(&actor) == 0);
	assert(GET_SILVER(&recipient) == 4 && GET_COPPER(&recipient) == 10);
	assert(submission_count == admitted_before + 1);
	actor.next = nullptr;
	character_list = &actor;
	currency_transaction_reset_for_tests();
	item_ownership_runtime_reset();
	GET_COPPER(&actor) = 1000;
	GET_SILVER(&actor) = GET_GOLD(&actor) = GET_PLATINUM(&actor) = 0;
	obj_data bag = {};
	bag.obj_uid = 900;
	bag.type = ITEM_CONTAINER;
	bag.loc_p = LOC_CARRIED;
	bag.loc.carrying = &actor;
	live_items.push_back(&bag);
	const item_owner_identity owner = {item_owner_type::player, 42, 0};
	assert(item_ownership_runtime_hydrate({900, 900, 0, owner, 1, 1, 96443, item_custody_state::active}));
	coin_debit_context put = {};
	put.action = coin_debit_action::put;
	put.container_uid = 900;
	put.room = actor.in_room;
	auto pile_ack = [&](bool accepted) {
		coin_transfer_payload payload;
		assert(coin_transfer_command_decode_payload(submitted_command, &payload));
		coin_transfer_result result;
		const coin_transfer_endpoint *endpoints[] = { &payload.source, &payload.destination };
		for (size_t index = 0; index < 2; ++index)
		{
			const auto &endpoint = *endpoints[index];
			if (endpoint.change.type == critical_command_type::account_bank)
			{
				for (size_t i = 0; i < 4; ++i) result.wallets[index].wallet.amount[i] = endpoint.after[i];
				result.wallets[index].wallet_revision = endpoint.change.expected_revisions[0].revision + 1;
				result.wallets[index].bank_revision = endpoint.change.expected_revisions[1].revision + 1;
				continue;
			}
			item_transfer_payload pile;
			assert(item_transfer_command_decode_payload(endpoint.change, &pile));
			result.piles[index] = {pile.selected_item_uid, 1, pile.expected_from_revision + 1, pile.expected_to_revision + 1,
				pile.from_owner.type == item_owner_type::system ? 1 : pile.items[0].expected_item_revision + 1, 0};
			std::vector<player_item_snapshot> snapshots;
			assert(player_item_snapshot_list_decode(pile.item_blob.data(), pile.item_blob_size, &snapshots) == player_snapshot_codec_result::ok);
			assert(snapshots.size() == 1 && snapshots[0].values[0] ==
				(pile.to_owner.type == item_owner_type::destruction ? endpoint.before[0] : endpoint.after[0]));
		}
		std::array<uint8_t, COIN_TRANSFER_RESULT_BYTES> bytes;
		assert(coin_transfer_command_encode_result(payload, result, &bytes));
		critical_completion ack = {};
		ack.operation_id = submitted_command.operation_id;
		ack.outcome = accepted ? critical_apply_outcome::applied : critical_apply_outcome::terminal_failure;
		ack.error_code = accepted ? 0 : ESTALE;
		ack.result_size = accepted ? bytes.size() : 0;
		std::copy(bytes.begin(), bytes.end(), ack.result_payload.begin());
		coordinator_fenced = false;
		currency_transaction_handle_completions(&ack, 1);
	};
	put.amount[0] = 100;
	assert(submit_coin_put(&actor, put));
	assert(bag.contains == nullptr && GET_COPPER(&actor) == 1000 && put_announcements == 0);
	pile_ack(true);
	assert(bag.contains && bag.contains->value[0] == 100 && put_announcements == 1);
	const uint64_t pile_uid = bag.contains->obj_uid;
	put.amount[0] = 200;
	assert(submit_coin_put(&actor, put));
	assert(bag.contains->value[0] == 100);
	// Lose the live pile while the durable merge is in flight. Restore its exact UID.
	extract_obj(bag.contains, false);
	pile_ack(true);
	assert(bag.contains && bag.contains->obj_uid == pile_uid && bag.contains->value[0] == 300);
	assert(put_announcements == 2);
	put.amount[0] = 50;
	assert(submit_coin_put(&actor, put));
	hidden_container = 900;
	pile_ack(true);
	assert(currency_transaction_health_copy().pending == 1 && put_announcements == 2);
	assert(!currency_transaction_player_busy(&actor) && currency_transaction_can_submit(&actor));
	assert(currency_transaction_coin_item_busy(pile_uid) && currency_transaction_coin_item_busy(900));
	assert(bag.contains->value[0] == 300);
	hidden_container = 0;
	currency_transaction_handle_completions(nullptr, 0);
	assert(currency_transaction_health_copy().pending == 0 && put_announcements == 3);
	assert(bag.contains->value[0] == 350);
	assert(submit_coin_put(&actor, put));
	const int gold_before = GET_GOLD(&actor);
	pile_ack(false);
	assert(bag.contains->value[0] == 350 && GET_GOLD(&actor) == gold_before && put_announcements == 3);
	const int before_pickup = submission_count;
	assert(submit_coin_get(&actor, bag.contains, &bag, 1));
	assert(bag.contains->value[0] == 350 && submission_count == before_pickup + 1);
	pile_ack(true);
	assert(bag.contains == nullptr);
	assert(GET_COPPER(&actor) + 10 * GET_SILVER(&actor) + 100 * GET_GOLD(&actor) + 1000 * GET_PLATINUM(&actor) == 1000);
	item_ownership_runtime_entry retired;
	assert(item_ownership_runtime_lookup(pile_uid, &retired) && retired.state == item_custody_state::destroyed);

	// Exercise the existing bulk continuation through two asynchronous coin commits.
	bulk_get_state batch = {};
	batch.container_uid = bag.obj_uid;
	for (int count : {25, 35})
	{
		P_obj item = create_money(count, 0, 0, 0);
		obj_to_obj(item, &bag);
		uint64_t revision;
		assert(item_ownership_runtime_owner_revision(owner, &revision));
		assert(item_ownership_runtime_hydrate({item->obj_uid, bag.obj_uid, bag.obj_uid,
			owner, 1, revision, VOBJ_COINS, item_custody_state::active}));
		batch.synchronous_items.push_back({item->obj_uid, item, false});
	}
	bulk_gets.emplace(42, batch);
	const int before_bulk = submission_count;
	assert(!finish_bulk_get_after_commit(&actor, bulk_gets.at(42), &bag));
	assert(submission_count == before_bulk + 1);
	pile_ack(true);
	assert(submission_count == before_bulk + 2 && bulk_gets.size() == 1);
	pile_ack(true);
	assert(bulk_gets.empty() && bulk_total == 2 && bag.contains == nullptr);
	assert(GET_COPPER(&actor) + 10 * GET_SILVER(&actor) + 100 * GET_GOLD(&actor) + 1000 * GET_PLATINUM(&actor) == 1060);

	// Partial pickup keeps exact denominations and UID, including at INT32 limits.
	GET_COPPER(&actor) = GET_SILVER(&actor) = GET_GOLD(&actor) = GET_PLATINUM(&actor) = INT32_MAX;
	GET_SILVER(&actor) -= 2;
	P_obj partial = create_money(35, 0, 0, 0);
	const uint64_t partial_uid = partial->obj_uid;
	obj_to_obj(partial, &bag);
	uint64_t owner_revision;
	assert(item_ownership_runtime_owner_revision(owner, &owner_revision));
	assert(item_ownership_runtime_hydrate({partial_uid, bag.obj_uid, bag.obj_uid,
		owner, 1, owner_revision, VOBJ_COINS, item_custody_state::active}));
	assert(submit_coin_get(&actor, partial, &bag, 1));
	coin_transfer_payload partial_payload;
	assert(coin_transfer_command_decode_payload(submitted_command, &partial_payload));
	assert((partial_payload.source.after == std::array<int32_t, 4>{15, 0, 0, 0}));
	assert(partial->value[0] == 35 && GET_SILVER(&actor) == INT32_MAX - 2);
	pile_ack(false);
	assert(partial->value[0] == 35 && GET_SILVER(&actor) == INT32_MAX - 2);
	assert(submit_coin_get(&actor, partial, &bag, 1));
	pile_ack(true);
	assert(bag.contains == partial && partial->obj_uid == partial_uid && partial->value[0] == 15);
	assert(GET_SILVER(&actor) == INT32_MAX && GET_COPPER(&actor) == INT32_MAX);
	assert(item_ownership_runtime_lookup(partial_uid, &retired) && retired.state == item_custody_state::active && retired.item_revision == 2);
	const int at_capacity = submission_count;
	assert(!submit_coin_get(&actor, partial, &bag, 1));
	assert(submission_count == at_capacity && partial->value[0] == 15);
	GET_COPPER(&actor) -= 5;
	GET_SILVER(&actor) -= 1;
	assert(submit_coin_get(&actor, partial, &bag, 1));
	pile_ack(true);
	assert(bag.contains == nullptr && GET_COPPER(&actor) == INT32_MAX && GET_SILVER(&actor) == INT32_MAX);
	assert(item_ownership_runtime_lookup(partial_uid, &retired) && retired.state == item_custody_state::destroyed);

	// A root pile on the ground also retains its exact UID and location on partial pickup.
	world[0].number = 500;
	const item_owner_identity room_owner = {item_owner_type::room, 500, 0};
	P_obj ground = create_money(25, 0, 0, 0);
	ground->loc_p = LOC_ROOM;
	ground->loc.room = 0;
	const uint64_t ground_uid = ground->obj_uid;
	assert(item_ownership_runtime_hydrate({ground_uid, ground_uid, 0, room_owner,
		1, 1, VOBJ_COINS, item_custody_state::active}));
	GET_SILVER(&actor) -= 1;
	assert(submit_coin_get(&actor, ground, nullptr, 1));
	pile_ack(true);
	assert(currency_transaction_health_copy().pending == 0 && ground->value[0] == 15);
	assert(ground->obj_uid == ground_uid && OBJ_ROOM(ground) && ground->loc.room == 0);
	GET_COPPER(&actor) -= 5;
	GET_SILVER(&actor) -= 1;
	assert(submit_coin_get(&actor, ground, nullptr, 1));
	pile_ack(true);
	assert(!find_live_item_uid(ground_uid));

	// Large mixed-denomination piles must not overflow while selecting a credit.
	GET_COPPER(&actor) = GET_SILVER(&actor) = GET_GOLD(&actor) = GET_PLATINUM(&actor) = 0;
	P_obj large = create_money(INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX);
	obj_to_obj(large, &bag);
	assert(item_ownership_runtime_owner_revision(owner, &owner_revision));
	assert(item_ownership_runtime_hydrate({large->obj_uid, bag.obj_uid, bag.obj_uid,
		owner, 1, owner_revision, VOBJ_COINS, item_custody_state::active}));
	assert(submit_coin_get(&actor, large, &bag, 1));
	pile_ack(true);
	assert(large->value[0] == INT32_MAX - 9 && large->value[1] == INT32_MAX - 9 &&
		large->value[2] == INT32_MAX - 9 && large->value[3] == 0);
	assert(GET_PLATINUM(&actor) == INT32_MAX && GET_GOLD(&actor) == 9 &&
		GET_SILVER(&actor) == 9 && GET_COPPER(&actor) == 9);
	int64_t conserved = 0, multiplier = 1;
	for (int i = 0; i < 4; ++i, multiplier *= 10)
		conserved += multiplier * (static_cast<int64_t>(large->value[i]) + actor.points.cash[i]);
	assert(conserved == 1111LL * INT32_MAX);
	extract_obj(large, false); // Synthetic fixture teardown.

	// Leaving the source while a coin commit is in flight stops the next pickup.
	GET_COPPER(&actor) = GET_SILVER(&actor) = GET_GOLD(&actor) = GET_PLATINUM(&actor) = 0;
	batch.synchronous_items.clear();
	bag.loc_p = LOC_ROOM;
	bag.loc.room = actor.in_room = 0;
	for (int count : {25, 35})
	{
		P_obj item = create_money(count, 0, 0, 0);
		obj_to_obj(item, &bag);
		assert(item_ownership_runtime_owner_revision(owner, &owner_revision));
		assert(item_ownership_runtime_hydrate({item->obj_uid, bag.obj_uid, bag.obj_uid,
			owner, 1, owner_revision, VOBJ_COINS, item_custody_state::active}));
		batch.synchronous_items.push_back({item->obj_uid, item, false});
	}
	bulk_gets.emplace(42, batch);
	assert(!finish_bulk_get_after_commit(&actor, bulk_gets.at(42), &bag));
	const int before_leaving = submission_count;
	actor.in_room = 1;
	pile_ack(true);
	assert(submission_count == before_leaving && bulk_gets.empty() && bulk_total == 1);
	assert(bag.contains && bag.contains->value[0] == 35);
	extract_obj(bag.contains, false);
    // Death conversion preserves every denomination and creates custody only on commit.
    // The submit/completion adapter is real; the acknowledgement is an explicit fixture.
    GET_COPPER(&actor) = GET_SILVER(&actor) = GET_GOLD(&actor) = GET_PLATINUM(&actor) = INT32_MAX;
    const auto wallet_revision_before = actor.only.pc->wallet_revision;
    money_to_inventory(&actor);
    assert(currency_transaction_player_busy(&actor));
    assert(actor.carrying == nullptr && GET_COPPER(&actor) == INT32_MAX);
    coin_transfer_payload death_money;
    assert(coin_transfer_command_decode_payload(submitted_command, &death_money));
    item_transfer_payload death_pile;
    assert(item_transfer_command_decode_payload(death_money.destination.change, &death_pile));
    const uint64_t rejected_uid = death_pile.selected_item_uid;
    for (int i = 0; i < 4; ++i) {
        assert(death_money.source.before[i] == INT32_MAX && death_money.source.after[i] == 0);
        assert(death_money.destination.after[i] == INT32_MAX);
    }
    pile_ack(false);
    assert(actor.carrying == nullptr && !find_live_item_uid(rejected_uid));
    assert(GET_PLATINUM(&actor) == INT32_MAX && actor.only.pc->wallet_revision == wallet_revision_before);
    money_to_inventory(&actor);
    assert(coin_transfer_command_decode_payload(submitted_command, &death_money));
    assert(item_transfer_command_decode_payload(death_money.destination.change, &death_pile));
    pile_ack(true);
    assert(!currency_transaction_player_busy(&actor));
    assert(actor.carrying && actor.carrying->obj_uid == death_pile.selected_item_uid);
    item_ownership_runtime_entry death_custody;
    assert(item_ownership_runtime_lookup(actor.carrying->obj_uid, &death_custody));
    assert(death_custody.owner.id == 42 && death_custody.root_item_uid == actor.carrying->obj_uid);
    for (int i = 0; i < 4; ++i)
        assert(actor.points.cash[i] == 0 && actor.carrying->value[i] == INT32_MAX);
    P_obj converted = actor.carrying;
    actor.carrying = nullptr;
    converted->loc_p = LOC_NOWHERE;
    extract_obj(converted, false);
	// Held item admission neither credits coins nor starts a coin transaction.
	actor.in_room = bag.loc.room = 0;
	GET_COPPER(&actor) = GET_SILVER(&actor) = GET_GOLD(&actor) = GET_PLATINUM(&actor) = 0;
	for (bool committed : {false, true}) {
		P_obj untracked = create_money(50, 0, 0, 0);
		obj_to_obj(untracked, &bag);
		const int coin_submissions = submission_count;
		assert(submit_coin_get(&actor, untracked, &bag, 1));
		assert(item_pending && submission_count == coin_submissions && GET_COPPER(&actor) == 0);
		assert(admitted_parent == bag.obj_uid && item_owner_identity_equal(admitted_owner, owner));
		if (committed) {
			assert(item_ownership_runtime_owner_revision(owner, &owner_revision));
			assert(item_ownership_runtime_hydrate({untracked->obj_uid, bag.obj_uid, bag.obj_uid,
				owner, 1, owner_revision, VOBJ_COINS, item_custody_state::active}));
		}
		item_pending = false;
		admission_callback(&actor, committed, {}, committed ? 0 : EEXIST,
			reinterpret_cast<const uint8_t *>(&admission_context), sizeof(admission_context));
		assert(GET_COPPER(&actor) == 0);
		if (committed) {
			assert(submission_count == coin_submissions + 1);
			pile_ack(true);
			assert(GET_SILVER(&actor) == 5 && bag.contains == nullptr);
		} else {
			assert(submission_count == coin_submissions && untracked->value[0] == 50);
			extract_obj(untracked, false);
		}
	}
	// Retired custody is never treated as absence and re-admitted.
	P_obj retired_pile = create_money(1, 0, 0, 0);
	obj_to_obj(retired_pile, &bag);
	assert(item_ownership_runtime_owner_revision(owner, &owner_revision));
	assert(item_ownership_runtime_hydrate({retired_pile->obj_uid, retired_pile->obj_uid, 0,
		owner, 1, owner_revision, VOBJ_COINS, item_custody_state::destroyed}));
	assert(!submit_coin_get(&actor, retired_pile, &bag, 1) && !item_pending);
	extract_obj(retired_pile, false);
	live_items.clear();
	item_ownership_runtime_reset();
	actor.next = nullptr;
	character_list = &actor;
	currency_transaction_reset_for_tests();
	printf("currency input queue runtime: ok\n");
	return 0;
}
'''


def main() -> int:
    """Compile and execute the held-currency queue regression."""
    harness = "\n".join([
        PRELUDE,
        BANK_PUBLICATION,
        COIN_GIVE,
        COIN_PILES,
        COIN_GET,
        extract(SRC / "handler.c", "bool money_inventory_completion("),
        extract(SRC / "handler.c", "bool money_to_inventory("),
        SEARCH,
        COMMAND_NUMBER,
        DEPENDS,
        ALLOWED,
        COMBINED_ALLOWED,
        GET_FROM_Q,
        GET_FILTERED,
        GET_ITEM,
        GET_PENDING,
        GET_PLAYING,
        DISPATCH_PLAYING,
        DRIVER,
    ])
    mysql_cflags = shlex.split(
        subprocess.check_output(["mysql_config", "--cflags"], text=True)
    )
    with tempfile.TemporaryDirectory() as directory:
        source = Path(directory) / "currency_input_queue.cpp"
        binary = Path(directory) / "currency_input_queue"
        source.write_text(harness, encoding="utf-8")
        subprocess.run(
            [
                "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g", "-O1",
                "-ffunction-sections", "-fdata-sections", "-fsanitize=address,undefined",
                "-Isrc", *mysql_cflags, str(source), rel("currency_transaction.c"),
                rel("currency_command.c"), rel("critical_command.c"),
                rel("coin_transfer_command.c"), rel("item_transfer_command.c"),
                rel("player_snapshot_codec.c"), rel("item_ownership_runtime.c"),
                "-Wl,--gc-sections", "-lcrypto", "-o", str(binary),
            ],
            cwd=ROOT,
            check=True,
        )
        subprocess.run([str(binary)], check=True)
    print("All currency input queue checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
