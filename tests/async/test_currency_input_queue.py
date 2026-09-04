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
DEPENDS = extract(INTERP_PATH, "bool cmd_depends_on_currency_transaction(int cmd)")

for command in ("CMD_DROP", "CMD_PUT", "CMD_GIVE", "CMD_DEPOSIT", "CMD_WITHDRAW"):
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

PRELUDE = r'''
#include "core/utils.h"
#include "economy/currency_transaction.h"
#include "sql/sql_player.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>

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

static const char *command[] = {
	"drop", "put", "give", "deposit", "withdraw", "inventory", "score", "look",
	"say", "\n"
};

P_char character_list = NULL;
static critical_command submitted_command = {};
static bool coordinator_fenced = false;
static bool item_pending = false;
static int submission_count = 0;

void logit(const char *, const char *, ...) {}
void __free(void *memory, const char *, int) { free(memory); }
void gmcp_char_vitals(P_char) {}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	abort();
}

const char *get_account_name_safe(P_char)
{
	return "queue_account";
}

P_char find_player_by_pid(int pid)
{
	return character_list && character_list->only.pc &&
		       character_list->only.pc->pid == pid ?
		       character_list :
		       NULL;
}

void publish_account_bank_balances_revision(const char *, int, const AccountBankBalances *balances,
					    uint64_t revision)
{
	assert(character_list && balances);
	GET_BALANCE_COPPER(character_list) = balances->copper;
	GET_BALANCE_SILVER(character_list) = balances->silver;
	GET_BALANCE_GOLD(character_list) = balances->gold;
	GET_BALANCE_PLATINUM(character_list) = balances->platinum;
	character_list->only.pc->bank_revision = revision;
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

int main()
{
	pc_only_data player = {};
	player.pid = 42;
	player.wallet_revision = 1;
	player.bank_revision = 1;
	char_data actor = {};
	actor.only.pc = &player;
	actor.player.racewar = 1;
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

	currency_transaction_reset_for_tests();
	printf("currency input queue runtime: ok\n");
	return 0;
}
'''


def main() -> int:
    """Compile and execute the held-currency queue regression."""
    harness = "\n".join([
        PRELUDE,
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
