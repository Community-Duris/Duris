#!/usr/bin/env python3
"""Exercise production queue/dispatch boundaries around held item publication.

The compiled harness links the real transaction runtime and supplies focused
command fixtures behind the production playing-state dispatcher so inventory
and equipment outcomes can be asserted without linking the full game server.
"""

from pathlib import Path
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
COMM = COMM_PATH.read_text(encoding="utf-8", errors="replace")
PROTOTYPES = (SRC / "prototypes.h").read_text(encoding="utf-8", errors="replace")
DEPENDS = extract(INTERP_PATH, "bool cmd_depends_on_item_movement(int cmd)")

for command in (
    "CMD_GET", "CMD_TAKE", "CMD_DROP", "CMD_PUT", "CMD_GIVE", "CMD_WEAR",
    "CMD_WIELD", "CMD_GRAB", "CMD_HOLD", "CMD_REMOVE", "CMD_EQUIPMENT",
    "CMD_INVENTORY", "CMD_FIRE", "CMD_APPLY", "CMD_BANDAGE", "CMD_DRINK",
    "CMD_EAT", "CMD_FILL", "CMD_POUR", "CMD_QUAFF", "CMD_RECITE",
    "CMD_RELOAD", "CMD_SALVAGE", "CMD_SIP", "CMD_SMOKE", "CMD_TASTE",
    "CMD_THROW", "CMD_THROWPOTION", "CMD_USE",
):
    assert command in DEPENDS

assert "get_playing_cmd_from_q(t_ch, &point->input, comm)" in COMM
assert "int get_item_movement_cmd_from_q(struct txt_q *, char *);" in PROTOTYPES

SEARCH = extract(INTERP_PATH, "int old_search_block(const char *argument")
COMMAND_NUMBER = extract(INTERP_PATH, "static int input_command_number(const char *input)")
ALLOWED = extract(INTERP_PATH, "bool input_allowed_while_item_moving(const char *input)")
GET_FROM_Q = extract(COMM_PATH, "int get_from_q(struct txt_q *queue, char *dest)")
GET_FILTERED = extract(
    COMM_PATH, "static int get_filtered_cmd_from_q(struct txt_q *queue, char *dest,"
)
GET_MOVEMENT = extract(
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

PRELUDE = r'''
#include "core/utils.h"
#include "item/item_movement_transaction.h"
#include "item/item_ownership_runtime.h"
#include "item/item_transfer_command.h"
#include "persistence/persistence_checkpoint.h"
#include "player/player_load_items.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>

#define CMD_NONE 0
#define CMD_GET 1
#define CMD_TAKE 2
#define CMD_DROP 3
#define CMD_PUT 4
#define CMD_GIVE 5
#define CMD_WEAR 6
#define CMD_WIELD 7
#define CMD_GRAB 8
#define CMD_HOLD 9
#define CMD_REMOVE 10
#define CMD_OPEN 11
#define CMD_CLOSE 12
#define CMD_EMPTY 13
#define CMD_JUNK 14
#define CMD_DONATE 15
#define CMD_SACRIFICE 16
#define CMD_BUY 17
#define CMD_SELL 18
#define CMD_LOOK 19
#define CMD_SCORE 20
#define CMD_EQUIPMENT 21
#define CMD_INVENTORY 22
#define CMD_FIRE 23
#define CMD_APPLY 24
#define CMD_BANDAGE 25
#define CMD_DRINK 26
#define CMD_EAT 27
#define CMD_FILL 28
#define CMD_POUR 29
#define CMD_QUAFF 30
#define CMD_RECITE 31
#define CMD_RELOAD 32
#define CMD_SALVAGE 33
#define CMD_SIP 34
#define CMD_SMOKE 35
#define CMD_TASTE 36
#define CMD_THROW 37
#define CMD_THROWPOTION 38
#define CMD_USE 39

static const char *command[] = {
	"get", "take", "drop", "put", "give", "wear", "wield", "grab", "hold",
	"remove", "open", "close", "empty", "junk", "donate", "sacrifice", "buy",
	"sell", "look", "score", "equipment", "inventory", "fire", "apply", "bandage",
	"drink", "eat", "fill", "pour", "quaff", "recite", "reload", "salvage", "sip",
	"smoke", "taste", "throw", "throwpotion", "use", "\n"
};

P_obj object_list = NULL;
P_char character_list = NULL;
static index_data object_indexes[4] = {};
P_index obj_index = object_indexes;
static room_data rooms[1] = {};
P_room world = rooms;
int top_of_objt = 3;
extern const int top_of_world = 0;

static bool command_submitted = false;
static bool reject_submission = false;
static bool hide_player_lookup = false;
static unsigned extracted_count = 0;
static std::vector<uint64_t> extracted_uids;
static std::string grant_messages;
static bool recover_creation_batch = false;
static obj_data recovered_grant_first = {};
static obj_data recovered_grant_second = {};
static critical_command submitted_command = {};
static uint64_t pending_coin_uid = 0;
bool currency_transaction_coin_item_busy(uint64_t uid)
{
	return uid && uid == pending_coin_uid;
}

void logit(const char *, const char *, ...) {}
void statuslog(int, const char *, ...) {}
void persistence_alert(int, const char *, const char *, const char *, const char *, const char *,
                      const char *, ...) {}
bool player_load_item_graph_materialize_creation(const item_transfer_payload &,
                                                 const item_transfer_result &, std::vector<P_obj> *roots)
{
    if (!recover_creation_batch || !roots)
        return false;
    recovered_grant_first.loc_p = LOC_NOWHERE;
    recovered_grant_second.loc_p = LOC_NOWHERE;
    recovered_grant_first.next_content = recovered_grant_second.next_content = nullptr;
    recovered_grant_first.next = &recovered_grant_second;
    recovered_grant_second.next = nullptr;
    object_list = &recovered_grant_first;
    roots->clear();
    roots->push_back(&recovered_grant_first);
    roots->push_back(&recovered_grant_second);
    return true;
}
void __free(void *memory, const char *, int) { free(memory); }
void send_to_char(const char *text, P_char) { grant_messages += text; }
void send_to_char(const char *, P_char, int) {}
void extract_obj(P_obj object, int) { ++extracted_count; extracted_uids.push_back(object->obj_uid); }
void obj_from_char(P_obj) {}
void obj_to_char(P_obj object, P_char actor)
{
    object->loc_p = LOC_CARRIED;
    object->loc.carrying = actor;
    object->next_content = actor->carrying;
    actor->carrying = object;
}
void obj_to_obj(P_obj, P_obj) {}
void obj_to_room(P_obj, int) {}
void mark_player_dirty_components(int, player_component_mask_t) {}

P_char find_player_by_pid(int pid)
{
	return !hide_player_lookup && character_list && character_list->only.pc && character_list->only.pc->pid == pid ?
		       character_list :
		       NULL;
}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	abort();
}

critical_submit_result critical_command_coordinator_submit(critical_command queued)
{
	assert(!command_submitted);
	if (reject_submission) return critical_submit_result::unavailable;
	command_submitted = true;
	submitted_command = std::move(queued);
	return critical_submit_result::accepted;
}

void command_interpreter(P_char character, char *input);
void process_with_paging(P_char character, char *input);
bool currency_transaction_player_busy(P_char) { return false; }
bool input_allowed_while_item_moving(const char *input);
bool input_allowed_while_currency_pending(const char *) { return true; }
bool input_allowed_while_item_and_currency_pending(const char *input)
{
	return input_allowed_while_item_moving(input);
}

int old_search_block(const char *argument, const uint begin, uint length, const char **list,
		     const int mode);
'''

DRIVER = r'''
static void push(struct txt_q *q, const char *text)
{
	struct txt_block *block = (struct txt_block *)malloc(sizeof(struct txt_block));
	block->text = strdup(text);
	block->next = NULL;
	if (q->head)
		q->tail->next = block;
	else
		q->head = block;
	q->tail = block;
}

static void drain(struct txt_q *q)
{
	while (q->head)
	{
		struct txt_block *next = q->head->next;
		free(q->head->text);
		free(q->head);
		q->head = next;
	}
	q->tail = NULL;
}

static void expect_text(const char *got, const char *want, const char *label)
{
	if (strcmp(got, want) != 0)
	{
		fprintf(stderr, "%s: got '%s', want '%s'\n", label, got, want);
		exit(1);
	}
}

static void check_intact(struct txt_q *q)
{
	struct txt_block *last = NULL;
	for (struct txt_block *walk = q->head; walk; walk = walk->next)
		last = walk;
	assert(q->tail == last);
}

static P_obj published_roots[2] = {};
static P_obj published_bow = NULL;
static P_obj published_cloak = NULL;
static int publication_count = 0;
static int bow_publication_count = 0;
static int cloak_publication_count = 0;
static int score_dispatches = 0;
static int put_dispatches = 0;
static int equipment_dispatches = 0;
static int inventory_dispatches = 0;
static int wear_failures = 0;
static int wear_successes = 0;
static int wield_dispatches = 0;
static int fire_dispatches = 0;
static int stale_completion_callbacks = 0;
static P_obj fixture_backpack = NULL;

static void held_bulk_get_completion(P_char actor, bool committed,
				     const item_transfer_result &result, unsigned int error_code,
				     const uint8_t *, size_t)
{
	assert(actor && committed && error_code == 0 && result.item_count == 2);
	item_ownership_runtime_entry ownership = {};
	for (P_obj root : published_roots)
	{
		assert(root);
		assert(item_ownership_runtime_lookup(root->obj_uid, &ownership));
		assert(ownership.owner.type == item_owner_type::player);
		root->loc_p = LOC_CARRIED;
		root->loc.carrying = actor;
	}
	published_roots[0]->next_content = published_roots[1];
	published_roots[1]->next_content = actor->carrying;
	actor->carrying = published_roots[0];
	world[0].contents = NULL;
	++publication_count;
}

static void held_bow_get_completion(P_char actor, bool committed,
				    const item_transfer_result &result, unsigned int error_code,
				    const uint8_t *, size_t)
{
	assert(actor && committed && error_code == 0 && result.item_count == 1);
	item_ownership_runtime_entry ownership = {};
	assert(published_bow);
	assert(item_ownership_runtime_lookup(published_bow->obj_uid, &ownership));
	assert(ownership.owner.type == item_owner_type::player);
	published_bow->loc_p = LOC_CARRIED;
	published_bow->loc.carrying = actor;
	published_bow->next_content = actor->carrying;
	actor->carrying = published_bow;
	world[0].contents = NULL;
	++bow_publication_count;
}

static void held_cloak_get_completion(P_char actor, bool committed,
				      const item_transfer_result &result, unsigned int error_code,
				      const uint8_t *, size_t)
{
	assert(actor && committed && error_code == 0 && result.item_count == 1);
	item_ownership_runtime_entry ownership = {};
	assert(published_cloak);
	assert(item_ownership_runtime_lookup(published_cloak->obj_uid, &ownership));
	assert(ownership.owner.type == item_owner_type::player);
	published_cloak->loc_p = LOC_CARRIED;
	published_cloak->loc.carrying = actor;
	published_cloak->next_content = actor->carrying;
	actor->carrying = published_cloak;
	world[0].contents = NULL;
	++cloak_publication_count;
}

static void stale_registry_completion(P_char, bool, const item_transfer_result &, unsigned int,
				      const uint8_t *, size_t)
{
	++stale_completion_callbacks;
}

static int carried_count(P_char actor)
{
	int count = 0;
	for (P_obj object = actor->carrying; object; object = object->next_content)
		++count;
	return count;
}

void process_with_paging(P_char, char *)
{
	abort();
}

void command_interpreter(P_char actor, char *input)
{
	assert(actor && input && fixture_backpack);
	if (!strcmp(input, "score"))
	{
		++score_dispatches;
		return;
	}
	if (!strcmp(input, "put all.roast bp"))
	{
		assert(actor->carrying == published_roots[0]);
		assert(published_roots[0]->next_content == published_roots[1]);
		assert(published_roots[1]->next_content == fixture_backpack);
		actor->carrying = fixture_backpack;
		fixture_backpack->next_content = NULL;
		fixture_backpack->contains = published_roots[0];
		for (P_obj root : published_roots)
		{
			root->loc_p = LOC_INSIDE;
			root->loc.inside = fixture_backpack;
		}
		published_roots[1]->next_content = NULL;
		++put_dispatches;
		return;
	}
	if (!strcmp(input, "equipment"))
	{
		assert(publication_count == 1 && fixture_backpack->contains == published_roots[0]);
		++equipment_dispatches;
		return;
	}
	if (!strcmp(input, "inventory"))
	{
		assert(actor->carrying == fixture_backpack && fixture_backpack->contains);
		++inventory_dispatches;
		return;
	}
	if (!strcmp(input, "wear roast"))
	{
		assert(actor->carrying == fixture_backpack && fixture_backpack->contains);
		++wear_failures;
		return;
	}
	if (!strcmp(input, "wear cloak"))
	{
		assert(actor->carrying == published_cloak);
		actor->carrying = published_cloak->next_content;
		published_cloak->next_content = NULL;
		published_cloak->loc_p = LOC_WORN;
		published_cloak->loc.wearing = actor;
		actor->equipment[1] = published_cloak;
		++wear_successes;
		return;
	}
	if (!strcmp(input, "wield bow"))
	{
		assert(actor->carrying == published_bow);
		actor->carrying = published_bow->next_content;
		published_bow->next_content = NULL;
		published_bow->loc_p = LOC_WORN;
		published_bow->loc.wearing = actor;
		actor->equipment[0] = published_bow;
		++wield_dispatches;
		return;
	}
	if (!strcmp(input, "fire target"))
	{
		assert(actor->equipment[0] == published_bow);
		++fire_dispatches;
		return;
	}
	abort();
}

int main()
{
	pc_only_data player = {};
	player.pid = 42;
	char_data actor = {};
	actor.only.pc = &player;
	actor.in_room = 0;
	character_list = &actor;

	object_indexes[0].virtual_number = 100;
	object_indexes[1].virtual_number = 101;
	object_indexes[2].virtual_number = 102;
	object_indexes[3].virtual_number = 103;
	obj_data first_roast = {};
	first_roast.obj_uid = 100;
	first_roast.R_num = 0;
	first_roast.loc_p = LOC_ROOM;
	first_roast.loc.room = 0;
	obj_data second_roast = {};
	second_roast.obj_uid = 101;
	second_roast.R_num = 0;
	second_roast.loc_p = LOC_ROOM;
	second_roast.loc.room = 0;
	obj_data bow = {};
	bow.obj_uid = 102;
	bow.R_num = 1;
	bow.loc_p = LOC_ROOM;
	bow.loc.room = 0;
	obj_data cloak = {};
	cloak.obj_uid = 103;
	cloak.R_num = 2;
	cloak.loc_p = LOC_ROOM;
	cloak.loc.room = 0;
	obj_data fault = {};
	fault.obj_uid = 104;
	fault.R_num = 3;
	fault.loc_p = LOC_ROOM;
	fault.loc.room = 0;
	obj_data backpack = {};
	backpack.obj_uid = 200;
	backpack.R_num = 0;
	backpack.loc_p = LOC_CARRIED;
	backpack.loc.carrying = &actor;
	actor.carrying = &backpack;
	first_roast.next = &second_roast;
	second_roast.next = &bow;
	bow.next = &cloak;
	cloak.next = &fault;
	fault.next = &backpack;
	first_roast.next_content = &second_roast;
	object_list = &first_roast;
	world[0].number = 500;
	world[0].contents = &first_roast;
	published_roots[0] = &first_roast;
	published_roots[1] = &second_roast;
	published_bow = &bow;
	published_cloak = &cloak;
	fixture_backpack = &backpack;

	item_ownership_runtime_reset();
	item_movement_transaction_reset_for_tests();
	const item_owner_identity room_owner = { item_owner_type::room, 500, 0 };
	const item_owner_identity player_owner = { item_owner_type::player, 42, 0 };
	const item_ownership_runtime_entry room_items[] = {
		{ 100, 100, 0, room_owner, 1, 3, 100, item_custody_state::active },
		{ 101, 101, 0, room_owner, 1, 3, 100, item_custody_state::active },
	};
	assert(item_ownership_runtime_hydrate_batch(room_items, 2));
	assert(item_ownership_runtime_hydrate_owner(player_owner, 7));
	P_obj roots[] = { &first_roast, &second_roast };
	// A committed coin change may still await live placement. Guard just its
	// affected tree before capturing either a single move or a bulk move.
	item_movement_reject reject = item_movement_reject::none;
	pending_coin_uid = first_roast.obj_uid;
	assert(!item_movement_transaction_submit(
		&actor, &first_roast, NULL, room_owner, player_owner,
		item_transfer_reason::player_get, first_roast.obj_uid,
		held_bulk_get_completion, NULL, 0, NULL, &reject));
	assert(reject == item_movement_reject::pending_conflict && !command_submitted);
	assert(!item_movement_transaction_submit_batch(
		&actor, roots, 2, NULL, room_owner, player_owner,
		item_transfer_reason::player_get, first_roast.obj_uid,
		held_bulk_get_completion, NULL, 0, NULL, &reject));
	assert(reject == item_movement_reject::pending_conflict && !command_submitted);
	pending_coin_uid = backpack.obj_uid;
	assert(!item_movement_transaction_submit(
		&actor, &first_roast, &backpack, room_owner, player_owner,
		item_transfer_reason::player_put, first_roast.obj_uid,
		held_bulk_get_completion, NULL, 0, NULL, &reject));
	assert(reject == item_movement_reject::pending_conflict && !command_submitted);
	assert(!item_movement_transaction_submit_batch(
		&actor, roots, 2, &backpack, room_owner, player_owner,
		item_transfer_reason::player_put, first_roast.obj_uid,
		held_bulk_get_completion, NULL, 0, NULL, &reject));
	assert(reject == item_movement_reject::pending_conflict && !command_submitted);
	// A nested container inherits the pending check from its authoritative root.
	obj_data nested = {};
	nested.obj_uid = 201;
	assert(item_ownership_runtime_hydrate({201, 200, 200, player_owner, 1, 7,
		100, item_custody_state::active}));
	assert(!item_movement_transaction_submit(
		&actor, &nested, NULL, player_owner, room_owner,
		item_transfer_reason::player_drop, nested.obj_uid,
		held_bulk_get_completion, NULL, 0, NULL, &reject));
	assert(reject == item_movement_reject::pending_conflict && !command_submitted);
	assert(!item_movement_transaction_player_busy(&actor));
	pending_coin_uid = 999; // An unrelated pending coin does not block this move.
	assert(item_movement_transaction_submit_batch(
		&actor, roots, 2, NULL, room_owner, player_owner,
		item_transfer_reason::player_get, first_roast.obj_uid,
		held_bulk_get_completion, NULL, 0));
	assert(command_submitted);
	assert(item_movement_transaction_player_busy(&actor));
	assert(actor.carrying == &backpack && publication_count == 0);

	char dest[MAX_INPUT_LENGTH];
	struct txt_q q = {};

	assert(!input_allowed_while_item_moving("put all.roast bp"));
	assert(!input_allowed_while_item_moving("  WEAR roast"));
	assert(!input_allowed_while_item_moving("gi roast friend"));
	assert(!input_allowed_while_item_moving("equipment"));
	assert(!input_allowed_while_item_moving("inventory"));
	assert(!input_allowed_while_item_moving("fire target"));
	assert(!input_allowed_while_item_moving("eat roast"));
	assert(!input_allowed_while_item_moving("quaff potion"));
	assert(!input_allowed_while_item_moving("recite scroll"));
	assert(!input_allowed_while_item_moving("reload bow arrow"));
	assert(!input_allowed_while_item_moving("use wand target"));
	assert(input_allowed_while_item_moving("score"));
	assert(input_allowed_while_item_moving("look"));
	assert(input_allowed_while_item_moving("say still here"));
	assert(!input_allowed_while_item_moving(NULL));

	/* The real movement transaction is pending while its captured coordinator
	   command is held. Dependent commands remain queued while score can run. */
	push(&q, "put all.roast bp");
	push(&q, "score");
	push(&q, "equipment");
	push(&q, "inventory");
	push(&q, "wear roast");
	assert(get_playing_cmd_from_q(&actor, &q, dest));
	expect_text(dest, "score", "safe command during movement");
	dispatch_playing_command(&actor, dest);
	assert(score_dispatches == 1);
	check_intact(&q);
	expect_text(q.head->text, "put all.roast bp", "put remains at head");
	expect_text(q.tail->text, "wear roast", "wear remains at tail");
	strcpy(dest, "sentinel");
	assert(!get_playing_cmd_from_q(&actor, &q, dest));
	expect_text(dest, "sentinel", "dependent-only queue is untouched");

	/* Release the captured production command through the real item-movement
	   completion handler. Registry publication precedes the bulk-get callback. */
	item_transfer_result result = { 100, 2, 4, 8, 2, 0 };
	critical_completion completion = {};
	completion.operation_id = submitted_command.operation_id;
	completion.outcome = critical_apply_outcome::applied;
	std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> encoded = {};
	assert(item_transfer_command_encode_result(result, &encoded));
	completion.result_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), completion.result_payload.begin());
	/* Offline completions retain the operation without invoking a null-actor
	   callback. Re-entry publishes once and releases the dependent queue. */
	character_list = NULL;
	item_movement_transaction_handle_completions(&completion, 1);
	assert(publication_count == 0);
	assert(item_movement_transaction_health_copy().retained_offline == 1);
	assert(item_movement_transaction_player_busy(&actor));
	character_list = &actor;
	item_movement_transaction_player_ready(&actor);
	assert(publication_count == 1);
	item_movement_transaction_player_ready(&actor);
	assert(publication_count == 1);
	assert(item_movement_transaction_health_copy().retained_offline == 0);
	assert(!item_movement_transaction_player_busy(&actor));
	assert(carried_count(&actor) == 3);

	/* The normal command path now observes the published inventory and returns
	   each dependent command exactly once in its original order. */
	const char *expected[] = {
		"put all.roast bp", "equipment", "inventory", "wear roast"
	};
	for (const char *command_text : expected)
	{
		assert(get_playing_cmd_from_q(&actor, &q, dest));
		expect_text(dest, command_text, "dependent command after publication");
		dispatch_playing_command(&actor, dest);
	}
	assert(!get_playing_cmd_from_q(&actor, &q, dest));
	assert(q.head == NULL);
	assert(q.tail == NULL);
	assert(put_dispatches == 1);
	assert(equipment_dispatches == 1);
	assert(inventory_dispatches == 1);
	assert(wear_failures == 1);
	assert(actor.carrying == &backpack && backpack.contains == &first_roast);

	/* A second held get proves fire cannot jump ahead of wield while the bow is
	   unpublished, then executes both through the normal dispatcher in FIFO. */
	const item_ownership_runtime_entry bow_entry = {
		102, 102, 0, room_owner, 1, 4, 101, item_custody_state::active
	};
	assert(item_ownership_runtime_hydrate(bow_entry));
	bow.loc_p = LOC_ROOM;
	bow.loc.room = 0;
	world[0].contents = &bow;
	command_submitted = false;
	submitted_command = {};
	P_obj bow_root[] = { &bow };
	assert(item_movement_transaction_submit_batch(
		&actor, bow_root, 1, NULL, room_owner, player_owner,
		item_transfer_reason::player_get, bow.obj_uid,
		held_bow_get_completion, NULL, 0));
	assert(item_movement_transaction_player_busy(&actor));
	push(&q, "wield bow");
	push(&q, "fire target");
	assert(!get_playing_cmd_from_q(&actor, &q, dest));
	assert(wield_dispatches == 0 && fire_dispatches == 0);

	result = { 102, 1, 5, 9, 2, 0 };
	completion = {};
	completion.operation_id = submitted_command.operation_id;
	completion.outcome = critical_apply_outcome::applied;
	encoded = {};
	assert(item_transfer_command_encode_result(result, &encoded));
	completion.result_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), completion.result_payload.begin());
	item_movement_transaction_handle_completions(&completion, 1);
	assert(bow_publication_count == 1);
	assert(!item_movement_transaction_player_busy(&actor));

	assert(get_playing_cmd_from_q(&actor, &q, dest));
	expect_text(dest, "wield bow", "wield after bow publication");
	dispatch_playing_command(&actor, dest);
	assert(get_playing_cmd_from_q(&actor, &q, dest));
	expect_text(dest, "fire target", "fire after wield");
	dispatch_playing_command(&actor, dest);
	assert(!get_playing_cmd_from_q(&actor, &q, dest));
	assert(q.head == NULL && q.tail == NULL);
	assert(wield_dispatches == 1 && fire_dispatches == 1);
	assert(actor.equipment[0] == &bow);

	/* A direct get-then-wear sequence succeeds after publication, separately
	   from the intentional wear failure after the earlier put command. */
	const item_ownership_runtime_entry cloak_entry = {
		103, 103, 0, room_owner, 1, 5, 102, item_custody_state::active
	};
	assert(item_ownership_runtime_hydrate(cloak_entry));
	cloak.loc_p = LOC_ROOM;
	cloak.loc.room = 0;
	world[0].contents = &cloak;
	command_submitted = false;
	submitted_command = {};
	P_obj cloak_root[] = { &cloak };
	assert(item_movement_transaction_submit_batch(
		&actor, cloak_root, 1, NULL, room_owner, player_owner,
		item_transfer_reason::player_get, cloak.obj_uid,
		held_cloak_get_completion, NULL, 0));
	assert(item_movement_transaction_player_busy(&actor));
	push(&q, "wear cloak");
	assert(!get_playing_cmd_from_q(&actor, &q, dest));
	assert(wear_successes == 0);

	result = { 103, 1, 6, 10, 2, 0 };
	completion = {};
	completion.operation_id = submitted_command.operation_id;
	completion.outcome = critical_apply_outcome::applied;
	encoded = {};
	assert(item_transfer_command_encode_result(result, &encoded));
	completion.result_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), completion.result_payload.begin());
	item_movement_transaction_handle_completions(&completion, 1);
	assert(cloak_publication_count == 1);
	assert(!item_movement_transaction_player_busy(&actor));

	assert(get_playing_cmd_from_q(&actor, &q, dest));
	expect_text(dest, "wear cloak", "wear after cloak publication");
	dispatch_playing_command(&actor, dest);
	assert(!get_playing_cmd_from_q(&actor, &q, dest));
	assert(q.head == NULL && q.tail == NULL);
	assert(wear_successes == 1 && actor.equipment[1] == &cloak);

	/* Pulling a safe tail keeps the queue appendable while a dependent head
	   remains parked. */
	push(&q, "drop all.roast");
	push(&q, "look");
	assert(get_item_movement_cmd_from_q(&q, dest));
	expect_text(dest, "look", "safe tail command");
	check_intact(&q);
	push(&q, "score");
	check_intact(&q);
	assert(get_item_movement_cmd_from_q(&q, dest));
	expect_text(dest, "score", "append after tail extraction");
	check_intact(&q);
	drain(&q);

	/* A committed completion whose live registry revision is stale must retain
	   the transaction and its dependent queue hold instead of calling back. */
	const item_ownership_runtime_entry fault_entry = {
		104, 104, 0, room_owner, 1, 6, 103, item_custody_state::active
	};
	assert(item_ownership_runtime_hydrate(fault_entry));
	fault.loc_p = LOC_ROOM;
	fault.loc.room = 0;
	world[0].contents = &fault;
	command_submitted = false;
	submitted_command = {};
	P_obj fault_root[] = { &fault };
	assert(item_movement_transaction_submit_batch(
		&actor, fault_root, 1, NULL, room_owner, player_owner,
		item_transfer_reason::player_get, fault.obj_uid,
		stale_registry_completion, NULL, 0));
	assert(item_movement_transaction_player_busy(&actor));
	const item_ownership_runtime_entry stale_fault_entry = {
		104, 104, 0, room_owner, 2, 6, 103, item_custody_state::active
	};
	assert(item_ownership_runtime_hydrate(stale_fault_entry));
	result = { 104, 1, 7, 11, 2, 0 };
	completion = {};
	completion.operation_id = submitted_command.operation_id;
	completion.outcome = critical_apply_outcome::applied;
	encoded = {};
	assert(item_transfer_command_encode_result(result, &encoded));
	completion.result_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), completion.result_payload.begin());
	item_movement_transaction_handle_completions(&completion, 1);
	const item_movement_health movement_health = item_movement_transaction_health_copy();
	assert(movement_health.pending == 1 && movement_health.stale_publications == 1);
	assert(stale_completion_callbacks == 0);
	assert(item_movement_transaction_player_busy(&actor));
	item_movement_transaction_player_ready(&actor);
	assert(item_movement_transaction_player_busy(&actor));
	assert(item_movement_transaction_health_copy().stale_publications == 1);
	push(&q, "wear fault");
	assert(!get_playing_cmd_from_q(&actor, &q, dest));
	check_intact(&q);
	expect_text(q.head->text, "wear fault", "stale publication keeps dependent hold");
	drain(&q);
	item_movement_transaction_reset_for_tests();
	assert(!item_movement_transaction_player_busy(&actor));


    // CHAOS pre-entry multi-root admission stages every root before any command.
    // All fixtures below are in-memory; no persistence service is connected.
    item_movement_transaction_reset_for_tests();
    item_ownership_runtime_reset();
    pending_coin_uid = 0;
    command_submitted = false;
    actor.carrying = nullptr;
    descriptor_data grant_descriptor = {};
    grant_descriptor.connected = CON_PLAYING;
    actor.desc = &grant_descriptor;
    obj_data grant_first = {};
    grant_first.obj_uid = 301;
    grant_first.R_num = 0;
    grant_first.loc_p = LOC_NOWHERE;
    obj_data grant_second = {};
    grant_second.obj_uid = 302;
    grant_second.R_num = 1;
    grant_second.loc_p = LOC_NOWHERE;
    grant_first.next = &grant_second;
    object_list = &grant_first;
    P_obj grants[] = {&grant_first, &grant_second};
    P_obj duplicates[] = {&grant_first, &grant_first};
    P_obj invalid_tail[] = {&grant_first, nullptr};
    assert(!item_creation_grant_submit_batch_to_player_before_entry(&actor, nullptr, 2, &actor));
    assert(!item_creation_grant_submit_batch_to_player_before_entry(&actor, grants, 0, &actor));
    assert(!item_creation_grant_submit_batch_to_player_before_entry(&actor, grants, 1025, &actor));
    assert(!item_creation_grant_submit_batch_to_player_before_entry(&actor, duplicates, 2, &actor));
    assert(!item_creation_grant_submit_batch_to_player_before_entry(&actor, invalid_tail, 2, &actor));
    grant_second.loc_p = LOC_ROOM;
    assert(!item_creation_grant_submit_batch_to_player_before_entry(&actor, grants, 2, &actor));
    grant_second.loc_p = LOC_NOWHERE;
    assert(!command_submitted && !item_creation_grant_blocks_commands(&actor));
    assert(!item_creation_grant_batches_pending());
    assert(extracted_count == 0);
    reject_submission = true;
    assert(item_creation_grant_submit_batch_to_player_before_entry(&actor, grants, 2, &actor));
    assert(item_creation_grant_blocks_commands(&actor));
    assert(item_creation_grant_batches_pending());
    reject_submission = false;
    item_movement_transaction_reset_for_tests();
    assert(!item_creation_grant_blocks_commands(&actor));
    assert(!item_creation_grant_batches_pending());
    assert(extracted_count == 0 && OBJ_NOWHERE(&grant_first) && OBJ_NOWHERE(&grant_second));
    assert(item_creation_grant_submit_batch_to_player_before_entry(&actor, grants, 2, &actor));
    assert(command_submitted && item_creation_grant_blocks_commands(&actor));
    assert(item_creation_grant_batches_pending());
    assert(!item_creation_grant_submit_batch_to_player_before_entry(&actor, grants, 2, &actor));
    push(&q, "wear grant");
    assert(!get_playing_cmd_from_q(&actor, &q, dest));
    expect_text(q.head->text, "wear grant", "batch holds immediate wear");
    drain(&q);

    // Commit both roots in one atomic operation. Repeated completion delivery is ignored.
    auto complete_grant = [&](uint64_t root_uid, uint64_t owner_revision)
    {
        item_transfer_result grant_result = {root_uid, 2, owner_revision, owner_revision, 1, 0};
        critical_completion grant_completion = {};
        grant_completion.operation_id = submitted_command.operation_id;
        grant_completion.outcome = critical_apply_outcome::applied;
        std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> payload = {};
        assert(item_transfer_command_encode_result(grant_result, &payload));
        grant_completion.result_size = payload.size();
        std::copy(payload.begin(), payload.end(), grant_completion.result_payload.begin());
        command_submitted = false;
        item_movement_transaction_handle_completions(&grant_completion, 1);
        item_movement_transaction_handle_completions(&grant_completion, 1);
    };
    complete_grant(301, 1);
    assert(OBJ_CARRIED_BY(&grant_first, &actor) && OBJ_CARRIED_BY(&grant_second, &actor));
    assert(!command_submitted && !item_creation_grant_blocks_commands(&actor));
    assert(!item_creation_grant_batches_pending());
    const auto announced = grant_messages.find("Your Chaos Equipment has been prepared!!");
    assert(announced != std::string::npos);
    assert(grant_messages.find("Your Chaos Equipment has been prepared!!", announced + 1) == std::string::npos);

    // A normal gameplay creation must wait behind a system-owned starter
    // transaction instead of being discarded as a terminal failure.
    item_movement_transaction_reset_for_tests();
    item_ownership_runtime_reset();
    command_submitted = false;
    submitted_command = {};
    actor.carrying = nullptr;
    obj_data system_created = {};
    system_created.obj_uid = 350;
    system_created.R_num = 0;
    system_created.loc_p = LOC_NOWHERE;
    obj_data gameplay_created = {};
    gameplay_created.obj_uid = 351;
    gameplay_created.R_num = 1;
    gameplay_created.loc_p = LOC_NOWHERE;
    system_created.next = &gameplay_created;
    gameplay_created.next = nullptr;
    object_list = &system_created;
    const item_owner_identity system_creation_owner = { item_owner_type::system, 0, 0 };
    P_obj system_roots[] = {&system_created};
    item_movement_reject system_reject = item_movement_reject::none;
    assert(item_movement_transaction_submit_batch(
        &actor, system_roots, 1, NULL, system_creation_owner, player_owner,
        item_transfer_reason::creation, 0, NULL, NULL, 0, NULL, &system_reject));
    assert(command_submitted);
    assert(item_creation_grant_submit_to_player(&actor, &gameplay_created, &actor, NULL));
    assert(item_movement_transaction_player_busy(&actor));
    assert(OBJ_NOWHERE(&gameplay_created));

    critical_completion system_creation_completion = {};
    system_creation_completion.operation_id = submitted_command.operation_id;
    system_creation_completion.outcome = critical_apply_outcome::applied;
    item_transfer_result system_creation_result = {350, 1, 1, 1, 1, 0};
    std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> system_creation_encoded = {};
    assert(item_transfer_command_encode_result(system_creation_result, &system_creation_encoded));
    system_creation_completion.result_size = system_creation_encoded.size();
    std::copy(system_creation_encoded.begin(), system_creation_encoded.end(),
              system_creation_completion.result_payload.begin());
    command_submitted = false;
    item_movement_transaction_handle_completions(&system_creation_completion, 1);
    assert(command_submitted && OBJ_NOWHERE(&gameplay_created));

    critical_completion gameplay_creation_completion = {};
    gameplay_creation_completion.operation_id = submitted_command.operation_id;
    gameplay_creation_completion.outcome = critical_apply_outcome::applied;
    item_transfer_result gameplay_creation_result = {351, 1, 1, 1, 1, 0};
    std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> gameplay_creation_encoded = {};
    assert(item_transfer_command_encode_result(gameplay_creation_result,
                                               &gameplay_creation_encoded));
    gameplay_creation_completion.result_size = gameplay_creation_encoded.size();
    std::copy(gameplay_creation_encoded.begin(), gameplay_creation_encoded.end(),
              gameplay_creation_completion.result_payload.begin());
    command_submitted = false;
    item_movement_transaction_handle_completions(&gameplay_creation_completion, 1);
    assert(OBJ_CARRIED_BY(&gameplay_created, &actor));
    assert(!item_movement_transaction_player_busy(&actor));

    // A stale live root retains the committed operation instead of publishing
    // a partial kit. Restoring the live registry lets player_ready() reconcile it.
    item_movement_transaction_reset_for_tests();
    item_ownership_runtime_reset();
    command_submitted = false;
    actor.carrying = nullptr;
    grant_first.loc_p = grant_second.loc_p = LOC_NOWHERE;
    grant_first.next_content = grant_second.next_content = nullptr;
    grant_first.next = &grant_second;
    object_list = &grant_first;
    grant_messages.clear();
    extracted_count = 0;
    assert(item_creation_grant_submit_batch_to_player_before_entry(&actor, grants, 2, &actor));
    critical_completion partial_completion = {};
    partial_completion.operation_id = submitted_command.operation_id;
    partial_completion.outcome = critical_apply_outcome::applied;
    item_transfer_result partial_result = {301, 2, 1, 1, 1, 0};
    std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> partial_encoded = {};
    assert(item_transfer_command_encode_result(partial_result, &partial_encoded));
    partial_completion.result_size = partial_encoded.size();
    std::copy(partial_encoded.begin(), partial_encoded.end(), partial_completion.result_payload.begin());
    grant_first.next = nullptr; // Simulate a stale/missing second live object.
    command_submitted = false;
    item_movement_transaction_handle_completions(&partial_completion, 1);
    assert(OBJ_NOWHERE(&grant_first) && OBJ_NOWHERE(&grant_second));
    assert(item_movement_transaction_health_copy().pending == 1);
    assert(item_movement_transaction_player_busy(&actor));
    assert(item_creation_grant_batches_pending());
    assert(grant_messages.find("publication repair") != std::string::npos);
    assert(grant_messages.find("Your Chaos Equipment has been prepared!!") == std::string::npos);
    grant_first.next = &grant_second;
    item_movement_transaction_player_ready(&actor);
    assert(OBJ_CARRIED_BY(&grant_first, &actor) && OBJ_CARRIED_BY(&grant_second, &actor));
    assert(!item_movement_transaction_player_busy(&actor));
    assert(!item_creation_grant_batches_pending());
    const auto reconciled_success = grant_messages.find("Your Chaos Equipment has been prepared!!");
    assert(reconciled_success != std::string::npos);
    assert(grant_messages.find("Your Chaos Equipment has been prepared!!", reconciled_success + 1) == std::string::npos);

    // A missing root can be rebuilt from the committed transfer snapshot.
    item_movement_transaction_reset_for_tests();
    item_ownership_runtime_reset();
    command_submitted = false;
    submitted_command = {};
    actor.carrying = nullptr;
    grant_first.loc_p = grant_second.loc_p = LOC_NOWHERE;
    grant_first.next_content = grant_second.next_content = nullptr;
    grant_first.next = &grant_second;
    object_list = &grant_first;
    recovered_grant_first = {};
    recovered_grant_first.obj_uid = 301;
    recovered_grant_first.R_num = 0;
    recovered_grant_first.loc_p = LOC_NOWHERE;
    recovered_grant_second = {};
    recovered_grant_second.obj_uid = 302;
    recovered_grant_second.R_num = 1;
    recovered_grant_second.loc_p = LOC_NOWHERE;
    grant_messages.clear();
    extracted_count = 0;
    recover_creation_batch = true;
    assert(item_creation_grant_submit_batch_to_player_before_entry(&actor, grants, 2, &actor));
    critical_completion recovered_completion = {};
    recovered_completion.operation_id = submitted_command.operation_id;
    recovered_completion.outcome = critical_apply_outcome::applied;
    item_transfer_result recovered_result = {301, 2, 1, 1, 1, 0};
    std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> recovered_encoded = {};
    assert(item_transfer_command_encode_result(recovered_result, &recovered_encoded));
    recovered_completion.result_size = recovered_encoded.size();
    std::copy(recovered_encoded.begin(), recovered_encoded.end(),
              recovered_completion.result_payload.begin());
    grant_first.next = nullptr;
    command_submitted = false;
    item_movement_transaction_handle_completions(&recovered_completion, 1);
    assert(OBJ_CARRIED_BY(&recovered_grant_first, &actor) &&
           OBJ_CARRIED_BY(&recovered_grant_second, &actor));
    assert(!item_creation_grant_batches_pending() &&
           !item_movement_transaction_player_busy(&actor));
    assert(grant_messages.find("publication repair") == std::string::npos);
    assert(grant_messages.find("Your Chaos Equipment has been prepared!!") != std::string::npos);
    recover_creation_batch = false;
    // The production extractor frees the displaced grant roots. The harness
    // retains their storage for later scenarios, so restore fresh fixture UIDs.
    grant_first.obj_uid = 301;
    grant_second.obj_uid = 302;
    object_list = &grant_first;
    grant_first.next = &grant_second;

    // A partially carried committed root is reconciled as one complete graph;
    // it must not make the queue permanently refuse repair.
    item_movement_transaction_reset_for_tests();
    item_ownership_runtime_reset();
    command_submitted = false;
    submitted_command = {};
    actor.carrying = nullptr;
    grant_first.loc_p = grant_second.loc_p = LOC_NOWHERE;
    grant_first.next_content = grant_second.next_content = nullptr;
    grant_first.next = &grant_second;
    object_list = &grant_first;
    recovered_grant_first.obj_uid = 301;
    recovered_grant_second.obj_uid = 302;
    grant_messages.clear();
    extracted_count = 0;
    recover_creation_batch = true;
    assert(item_creation_grant_submit_batch_to_player_before_entry(&actor, grants, 2, &actor));
    grant_first.loc_p = LOC_CARRIED;
    grant_first.loc.carrying = &actor;
    actor.carrying = &grant_first;
    grant_first.next = nullptr; // The second root disappeared from the live list.
    critical_completion carried_partial_completion = {};
    carried_partial_completion.operation_id = submitted_command.operation_id;
    carried_partial_completion.outcome = critical_apply_outcome::applied;
    item_transfer_result carried_partial_result = {301, 2, 1, 1, 1, 0};
    std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> carried_partial_encoded = {};
    assert(item_transfer_command_encode_result(carried_partial_result,
                                               &carried_partial_encoded));
    carried_partial_completion.result_size = carried_partial_encoded.size();
    std::copy(carried_partial_encoded.begin(), carried_partial_encoded.end(),
              carried_partial_completion.result_payload.begin());
    command_submitted = false;
    item_movement_transaction_handle_completions(&carried_partial_completion, 1);
    assert(OBJ_CARRIED_BY(&recovered_grant_first, &actor) &&
           OBJ_CARRIED_BY(&recovered_grant_second, &actor));
    assert(!item_creation_grant_batches_pending() &&
           !item_movement_transaction_player_busy(&actor));
    recover_creation_batch = false;
    actor.carrying = nullptr;
    grant_first.obj_uid = 301;
    grant_second.obj_uid = 302;
    grant_first.loc_p = grant_second.loc_p = LOC_NOWHERE;
    grant_first.next_content = grant_second.next_content = nullptr;
    object_list = &grant_first;
    grant_first.next = &grant_second;

    // A terminal failure stops the remaining batch and cannot announce success.
    item_movement_transaction_reset_for_tests();
    item_ownership_runtime_reset();
    actor.carrying = nullptr;
    grant_first.loc_p = grant_second.loc_p = LOC_NOWHERE;
    grant_first.next_content = grant_second.next_content = nullptr;
    grant_messages.clear();
    extracted_count = 0;
    assert(item_creation_grant_submit_batch_to_player_before_entry(&actor, grants, 2, &actor));
    critical_completion failed_grant = {};
    failed_grant.operation_id = submitted_command.operation_id;
    failed_grant.outcome = critical_apply_outcome::terminal_failure;
    failed_grant.error_code = EIO;
    command_submitted = false;
    item_movement_transaction_handle_completions(&failed_grant, 1);
    assert(extracted_count == 2);
    assert(!command_submitted && !item_creation_grant_blocks_commands(&actor));
    assert(!item_creation_grant_batches_pending());
    assert(grant_messages.find("Your Chaos Equipment has been prepared!!") == std::string::npos);
    // A duplicate terminal completion after an atomic batch publication is ignored.
    item_movement_transaction_reset_for_tests();
    item_ownership_runtime_reset();
    actor.carrying = nullptr;
    grant_first.loc_p = grant_second.loc_p = LOC_NOWHERE;
    grant_first.next_content = grant_second.next_content = nullptr;
    grant_messages.clear();
    extracted_count = 0;
    assert(item_creation_grant_submit_batch_to_player_before_entry(&actor, grants, 2, &actor));
    complete_grant(301, 1);
    const unsigned extracted_after_commit = extracted_count;
    failed_grant.operation_id = submitted_command.operation_id;
    item_movement_transaction_handle_completions(&failed_grant, 1);
    assert(extracted_count == extracted_after_commit &&
           OBJ_CARRIED_BY(&grant_first, &actor) && OBJ_CARRIED_BY(&grant_second, &actor));
    assert(!command_submitted && !item_creation_grant_blocks_commands(&actor));
    assert(!item_creation_grant_batches_pending());
    const auto duplicate_success = grant_messages.find("Your Chaos Equipment has been prepared!!");
    assert(duplicate_success != std::string::npos);
    assert(grant_messages.find("Your Chaos Equipment has been prepared!!", duplicate_success + 1) == std::string::npos);
    // Losing the descriptor during a batch must not strand committed items.
    // character_list still contains the linkdead actor used for callback publication.
    item_movement_transaction_reset_for_tests();
    item_ownership_runtime_reset();
    actor.carrying = nullptr;
    grant_first.loc_p = grant_second.loc_p = LOC_NOWHERE;
    grant_first.next_content = grant_second.next_content = nullptr;
    grant_messages.clear();
    extracted_count = 0;
    character_list = &actor;
    assert(item_creation_grant_submit_batch_to_player_before_entry(&actor, grants, 2, &actor));
    hide_player_lookup = true;
    actor.desc = nullptr;
    assert(find_player_by_pid(42) == nullptr && character_list == &actor);
    complete_grant(301, 1);
    assert(OBJ_CARRIED_BY(&grant_first, &actor) && OBJ_CARRIED_BY(&grant_second, &actor));
    assert(extracted_count == 0 && !command_submitted);
    assert(!item_creation_grant_blocks_commands(&actor));
    assert(!item_creation_grant_batches_pending());
    hide_player_lookup = false;
    actor.desc = &grant_descriptor;

    // A socket lost at the pre-entry MOTD cannot drain an active multi-root
    // operation. Retain the complete journaled operation for the next entry.
    item_movement_transaction_reset_for_tests();
    item_ownership_runtime_reset();
    actor.carrying = nullptr;
    actor.desc = &grant_descriptor;
    grant_descriptor.connected = CON_PLAYING;
    grant_first.loc_p = grant_second.loc_p = LOC_NOWHERE;
    grant_first.next_content = grant_second.next_content = nullptr;
    grant_messages.clear();
    extracted_count = 0;
    extracted_uids.clear();
    assert(item_creation_grant_submit_batch_to_player_before_entry(&actor, grants, 2, &actor));
    item_creation_grant_cancel_batch_before_entry(&actor);
    assert(item_creation_grant_batches_pending() && extracted_count == 0);
    grant_descriptor.connected = CON_RMOTD;
    character_list = nullptr;
    item_creation_grant_cancel_batch_before_entry(&actor);
    item_creation_grant_cancel_batch_before_entry(&actor); // idempotent close/retry
    assert(extracted_count == 0 && OBJ_NOWHERE(&grant_first) && OBJ_NOWHERE(&grant_second));
    assert(!item_creation_grant_batches_pending() && !item_creation_grant_blocks_commands(&actor));
    assert(item_movement_transaction_player_busy(&actor));
    complete_grant(301, 1);
    assert(item_movement_transaction_health_copy().retained_offline == 1);
    assert(OBJ_NOWHERE(&grant_first) && OBJ_NOWHERE(&grant_second));
    pc_only_data other_pc{};
    other_pc.pid = 43;
    char_data other_actor{};
    other_actor.only.pc = &other_pc;
    item_movement_transaction_player_ready(&other_actor);
    assert(OBJ_NOWHERE(&grant_first) && OBJ_NOWHERE(&grant_second));
    character_list = &actor;
    grant_descriptor.connected = CON_PLAYING;
    item_movement_transaction_player_ready(&actor);
    item_movement_transaction_player_ready(&actor);
    assert(OBJ_CARRIED_BY(&grant_first, &actor) && OBJ_CARRIED_BY(&grant_second, &actor));
    assert(extracted_count == 0 && actor.carrying == &grant_second &&
           grant_second.next_content == &grant_first);
    assert(!command_submitted && !item_creation_grant_batches_pending());
    assert(!item_movement_transaction_player_busy(&actor));
    assert(grant_messages.find("Your Chaos Equipment has been prepared!!") == std::string::npos);

    // A second pre-entry character waits behind an in-flight system-owned
    // creation without losing its staged roots or returning a false failure.
    item_movement_transaction_reset_for_tests();
    item_ownership_runtime_reset();
    command_submitted = false;
    submitted_command = {};
    grant_messages.clear();
    actor.carrying = nullptr;
    actor.desc = &grant_descriptor;
    grant_descriptor.connected = CON_PLAYING;
    grant_first.loc_p = LOC_NOWHERE;
    grant_first.next_content = nullptr;
    obj_data concurrent_first = {};
    concurrent_first.obj_uid = 401;
    concurrent_first.R_num = 0;
    concurrent_first.loc_p = LOC_NOWHERE;
    concurrent_first.next_content = nullptr;
    obj_data concurrent_second = {};
    concurrent_second.obj_uid = 402;
    concurrent_second.R_num = 1;
    concurrent_second.loc_p = LOC_NOWHERE;
    concurrent_second.next_content = nullptr;
    grant_first.next = &concurrent_first;
    concurrent_first.next = &concurrent_second;
    concurrent_second.next = nullptr;
    object_list = &grant_first;
    pc_only_data concurrent_pc = {};
    concurrent_pc.pid = 43;
    char_data concurrent_actor = {};
    concurrent_actor.only.pc = &concurrent_pc;
    concurrent_actor.in_room = 0;
    descriptor_data concurrent_descriptor = {};
    concurrent_descriptor.connected = CON_PLAYING;
    concurrent_actor.desc = &concurrent_descriptor;
    actor.next = &concurrent_actor;
    concurrent_actor.next = nullptr;
    character_list = &actor;
    const item_owner_identity creation_system_owner = { item_owner_type::system, 0, 0 };
    P_obj first_pending_root[] = { &grant_first };
    item_movement_reject conflict_reject = item_movement_reject::none;
    assert(item_movement_transaction_submit_batch(
        &actor, first_pending_root, 1, NULL, creation_system_owner, player_owner,
        item_transfer_reason::creation, 0, NULL, NULL, 0, NULL, &conflict_reject));
    assert(command_submitted);
    P_obj concurrent_roots[] = { &concurrent_first, &concurrent_second };
    assert(item_creation_grant_submit_batch_to_player_before_entry(
        &concurrent_actor, concurrent_roots, 2, &concurrent_actor));
    assert(command_submitted && item_creation_grant_batches_pending() &&
           item_movement_transaction_player_busy(&concurrent_actor));

    critical_completion first_creation_completion = {};
    first_creation_completion.operation_id = submitted_command.operation_id;
    first_creation_completion.outcome = critical_apply_outcome::applied;
    item_transfer_result first_creation_result = {301, 1, 1, 1, 1, 0};
    std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> first_creation_encoded = {};
    assert(item_transfer_command_encode_result(first_creation_result, &first_creation_encoded));
    first_creation_completion.result_size = first_creation_encoded.size();
    std::copy(first_creation_encoded.begin(), first_creation_encoded.end(),
              first_creation_completion.result_payload.begin());
    command_submitted = false;
    item_movement_transaction_handle_completions(&first_creation_completion, 1);
    assert(command_submitted && item_creation_grant_batches_pending());

    critical_completion concurrent_creation_completion = {};
    concurrent_creation_completion.operation_id = submitted_command.operation_id;
    concurrent_creation_completion.outcome = critical_apply_outcome::applied;
    item_transfer_result concurrent_creation_result = {401, 2, 2, 1, 1, 0};
    std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> concurrent_creation_encoded = {};
    assert(item_transfer_command_encode_result(concurrent_creation_result,
                                               &concurrent_creation_encoded));
    concurrent_creation_completion.result_size = concurrent_creation_encoded.size();
    std::copy(concurrent_creation_encoded.begin(), concurrent_creation_encoded.end(),
              concurrent_creation_completion.result_payload.begin());
    command_submitted = false;
    item_movement_transaction_handle_completions(&concurrent_creation_completion, 1);
    assert(OBJ_CARRIED_BY(&concurrent_first, &concurrent_actor) &&
           OBJ_CARRIED_BY(&concurrent_second, &concurrent_actor));
    assert(!item_creation_grant_batches_pending() &&
           !item_movement_transaction_player_busy(&concurrent_actor));
    item_movement_transaction_reset_for_tests();
    item_ownership_runtime_reset();
    actor.next = nullptr;
    character_list = nullptr;
    actor.desc = nullptr;

	printf("item movement input queue runtime: ok\n");
	return 0;
}
'''


def main() -> int:
    """Compile and execute the item-movement queue regression."""
    harness = "\n".join([
        PRELUDE,
        SEARCH,
        COMMAND_NUMBER,
        DEPENDS,
        ALLOWED,
        GET_FROM_Q,
        GET_FILTERED,
        GET_MOVEMENT,
        GET_PENDING,
        GET_PLAYING,
        DISPATCH_PLAYING,
        DRIVER,
    ])
    with tempfile.TemporaryDirectory() as directory:
        source = Path(directory) / "item_movement_input_queue.cpp"
        binary = Path(directory) / "item_movement_input_queue"
        source.write_text(harness, encoding="utf-8")
        subprocess.run(
            [
                "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g",
                "-O1", "-ffunction-sections", "-fdata-sections",
                "-fsanitize=address,undefined", "-Isrc", str(source),
                rel("item_movement_transaction.c"),
                rel("item_ownership_runtime.c"),
                rel("item_transfer_command.c"),
                rel("critical_command.c"),
                rel("player_snapshot_capture.c"),
                rel("player_snapshot_codec.c"),
                "-Wl,--gc-sections", "-lcrypto", "-o", str(binary),
            ],
            cwd=ROOT,
            check=True,
        )
        subprocess.run([str(binary)], check=True)
    print("All item movement input queue checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
