#!/usr/bin/env python3
"""Runtime regression for type-ahead across item movement publication."""

from pathlib import Path
import subprocess
import tempfile

from _paths import SRC


def extract(source: Path, signature: str) -> str:
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
    "CMD_WIELD", "CMD_GRAB", "CMD_HOLD", "CMD_REMOVE",
):
    assert command in DEPENDS

assert "item_movement_transaction_player_busy(t_ch)" in COMM
assert "get_item_movement_cmd_from_q(&point->input, comm)" in COMM
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

PRELUDE = r'''
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define MAX_INPUT_LENGTH 512
#define LOG_COMM 0
#define FALSE false
#define TRUE true
#define LOWER(c) (((c) >= 'A' && (c) <= 'Z') ? ((c) + ('a' - 'A')) : (c))
#define FREE(i)          \
	{                \
		free(i); \
		(i) = NULL;      \
	}

typedef unsigned int uint;

struct txt_block
{
	char *text;
	struct txt_block *next;
};

struct txt_q
{
	struct txt_block *head;
	struct txt_block *tail;
};

static void logit(int, const char *, ...) {}

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

static const char *command[] = {
	"get", "take", "drop", "put", "give", "wear", "wield", "grab", "hold",
	"remove", "open", "close", "empty", "junk", "donate", "sacrifice", "buy",
	"sell", "look", "score", "\n"
};

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

int main()
{
	char dest[MAX_INPUT_LENGTH];
	struct txt_q q = {};

	assert(!input_allowed_while_item_moving("put all.roast bp"));
	assert(!input_allowed_while_item_moving("  WEAR roast"));
	assert(!input_allowed_while_item_moving("gi roast friend"));
	assert(input_allowed_while_item_moving("score"));
	assert(input_allowed_while_item_moving("look"));
	assert(input_allowed_while_item_moving("say still here"));
	assert(!input_allowed_while_item_moving(NULL));

	/* The bulk get was already dequeued and its completion is held.  Its
	   dependent put and wear remain queued while unrelated input can run. */
	push(&q, "put all.roast bp");
	push(&q, "score");
	push(&q, "wear roast");
	assert(get_item_movement_cmd_from_q(&q, dest));
	expect_text(dest, "score", "safe command during movement");
	check_intact(&q);
	expect_text(q.head->text, "put all.roast bp", "put remains at head");
	expect_text(q.tail->text, "wear roast", "wear remains at tail");
	strcpy(dest, "sentinel");
	assert(!get_item_movement_cmd_from_q(&q, dest));
	expect_text(dest, "sentinel", "dependent-only queue is untouched");

	/* Releasing the held completion returns the command loop to its normal
	   FIFO dequeue path, so both dependent commands execute exactly once. */
	assert(get_from_q(&q, dest));
	expect_text(dest, "put all.roast bp", "put after publication");
	assert(get_from_q(&q, dest));
	expect_text(dest, "wear roast", "wear after publication");
	assert(!get_from_q(&q, dest));
	assert(q.head == NULL);
	q.tail = NULL;

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

	printf("item movement input queue runtime: ok\n");
	return 0;
}
'''


def main() -> int:
    harness = "\n".join([
        PRELUDE,
        SEARCH,
        COMMAND_NUMBER,
        DEPENDS,
        ALLOWED,
        GET_FROM_Q,
        GET_FILTERED,
        GET_MOVEMENT,
        DRIVER,
    ])
    with tempfile.TemporaryDirectory() as directory:
        source = Path(directory) / "item_movement_input_queue.cpp"
        binary = Path(directory) / "item_movement_input_queue"
        source.write_text(harness, encoding="utf-8")
        subprocess.run(
            [
                "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g",
                "-fsanitize=address,undefined", str(source), "-o", str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], check=True)
    print("All item movement input queue checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
