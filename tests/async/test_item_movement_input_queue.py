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
static critical_command submitted_command = {};

void logit(const char *, const char *, ...) {}
void __free(void *memory, const char *, int) { free(memory); }
void send_to_char(const char *, P_char) {}
void send_to_char(const char *, P_char, int) {}
void extract_obj(P_obj, int) {}
void obj_from_char(P_obj) {}
void obj_to_char(P_obj, P_char) {}
void obj_to_obj(P_obj, P_obj) {}
void obj_to_room(P_obj, int) {}
void mark_player_dirty_components(int, player_component_mask_t) {}

P_char find_player_by_pid(int pid)
{
	return character_list && character_list->only.pc && character_list->only.pc->pid == pid ?
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
	command_submitted = true;
	submitted_command = std::move(queued);
	return critical_submit_result::accepted;
}

void command_interpreter(P_char character, char *input);
void process_with_paging(P_char character, char *input);

int old_search_block(const char *argument, const uint begin, uint length, const char **list,
		     const int mode);
bool input_allowed_while_item_moving(const char *input);
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
	item_movement_transaction_handle_completions(&completion, 1);
	assert(publication_count == 1);
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
