#!/usr/bin/env python3
"""Regression coverage for the complete casting input-gate state matrix.

The existing casting queue test proves selective linked-list extraction.  This
probe additionally compiles the production casting-state predicate and drives
it with the four wait/casting combinations that reach the game-loop dequeue:

    wait on/off × casting on/off

Ordinary type-ahead must survive both casting rows, including the row where
PLR2_WAIT has already cleared.  Only the casting whitelist may be removed while
AFF2_CASTING remains set.
"""

from _paths import SRC
from pathlib import Path
import subprocess
import sys
import tempfile



def extract(source: Path, signature: str) -> str:
    """Extract one top-level function selected by its signature prefix."""
    text = source.read_text(encoding="utf-8", errors="replace")
    start = text.index(signature)
    brace = text.index("{", start)
    depth = 0
    for index in range(brace, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start : index + 1]
    raise AssertionError(f"unbalanced braces reading {signature}")


comm_path = SRC / "comm.c"
interp_path = SRC / "interp.c"
comm_text = comm_path.read_text(encoding="utf-8", errors="replace")
interp_text = interp_path.read_text(encoding="utf-8", errors="replace")

PREDICATE_SIGNATURE = "static bool casting_input_for_descriptor(P_desc descriptor, P_char character)"
if PREDICATE_SIGNATURE not in comm_text:
    raise AssertionError(
        "comm.c must expose a casting predicate independent of CAN_ACT before "
        "the queue state-matrix harness can run"
    )

# Source contract: the dispatch decision must use the predicate, and the
# predicate itself must be driven by AFF2_CASTING rather than !CAN_ACT.
assert "casting_input = casting_input_for_descriptor(point, t_ch);" in comm_text
PREDICATE = extract(comm_path, PREDICATE_SIGNATURE)
assert "IS_AFFECTED2(character, AFF2_CASTING)" in PREDICATE
assert "!CAN_ACT" not in PREDICATE
assert "descriptor->connected == CON_PLAYING" in PREDICATE
assert "!descriptor->showstr_count" in PREDICATE
assert "!descriptor->str" in PREDICATE
assert "casting_input ? get_casting_cmd_from_q(t_ch, &point->input, comm)" in comm_text
assert "(CAN_ACT(t_ch) || casting_input)" in comm_text

SEARCH_BLOCK = extract(interp_path, "int old_search_block(const char *argument")
COMMAND_NUMBER = extract(interp_path, "static int input_command_number(const char *input)")
CMD_ALLOWED = extract(interp_path, "bool cmd_allowed_while_casting(P_char ch, int cmd)")
ALLOWED = extract(interp_path, "bool input_allowed_while_casting(P_char ch, const char *input)")
GET_CASTING = extract(comm_path, "int get_casting_cmd_from_q(P_char ch, struct txt_q *queue, char *dest)")
GET_FROM = extract(comm_path, "int get_from_q(struct txt_q *queue, char *dest)")

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
		free(i);      \
		(i) = NULL;   \
	}
#define CON_PLAYING 1
#define AFF2_CASTING 1u

typedef unsigned int uint;

struct char_data
{
	struct
	{
		unsigned int affected_by2;
		unsigned int act3;
	} specials;
};

struct descriptor_data
{
	int connected;
	int showstr_count;
	char *str;
};

typedef struct char_data *P_char;
typedef struct descriptor_data *P_desc;

#define IS_AFFECTED2(ch, flag) (((ch)->specials.affected_by2 & (flag)) != 0)
#define PLR3_ABORT_CASTING 1u
#define PLR3_FLAGGED(ch, flag) (((ch)->specials.act3 & (flag)) != 0)

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
#define CMD_PETITION 1
#define CMD_RETURN 2
#define CMD_ABORT 3
static const char *command[] = {
	"petition", "return", "abort", "rest", "pray", "meditate", "look", "\n"
};

bool cmd_allowed_while_casting(P_char ch, int cmd);

int old_search_block(const char *argument, const uint begin, uint length, const char **list,
		     const int mode);
bool input_allowed_while_casting(P_char, const char *input);
'''

DRIVER = r'''
static void push(struct txt_q *queue, const char *text)
{
	struct txt_block *block = (struct txt_block *)malloc(sizeof(struct txt_block));
	block->text = strdup(text);
	block->next = NULL;
	if (queue->tail)
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

static void expect_text(const char *got, const char *want, const char *label)
{
	if (!got || strcmp(got, want) != 0)
	{
		fprintf(stderr, "%s: got '%s' want '%s'\n", label, got ? got : "<null>", want);
		exit(1);
	}
}

static void expect_queue(struct txt_q *queue, const char *first, const char *second,
			 const char *third, const char *label)
{
	const char *expected[] = {first, second, third};
	struct txt_block *cursor = queue->head;
	struct txt_block *last = NULL;
	for (int i = 0; i < 3; i++)
	{
		if (!expected[i])
			break;
		if (!cursor)
		{
			fprintf(stderr, "%s: queue ended at %d\n", label, i);
			exit(1);
		}
		expect_text(cursor->text, expected[i], label);
		last = cursor;
		cursor = cursor->next;
	}
	if (cursor || queue->tail != last)
	{
		fprintf(stderr, "%s: queue shape or tail mismatch\n", label);
		exit(1);
	}
}

static int select_one(bool can_act, bool casting, bool abort_enabled, struct txt_q *queue, char *dest)
{
	struct descriptor_data descriptor = {CON_PLAYING, 0, NULL};
	struct char_data character = {};
	if (casting)
		character.specials.affected_by2 |= AFF2_CASTING;
	if (abort_enabled)
		character.specials.act3 |= PLR3_ABORT_CASTING;

	const bool casting_input = casting_input_for_descriptor(&descriptor, &character);
	if (!can_act && !casting_input)
		return 0;
	return casting_input ? get_casting_cmd_from_q(&character, queue, dest) : get_from_q(queue, dest);
}

static void ordinary_commands_stay_queued(bool can_act, bool casting, const char *label)
{
	struct txt_q queue = {};
	char dest[MAX_INPUT_LENGTH] = {};
	push(&queue, "rest");
	push(&queue, "pray");
	push(&queue, "meditate");

	if (select_one(can_act, casting, false, &queue, dest) != 0)
	{
		fprintf(stderr, "%s: ordinary input was dequeued\n", label);
		exit(1);
	}
	expect_queue(&queue, "rest", "pray", "meditate", label);
	drain(&queue);
}

int main()
{
	/* The shipped policy disables the interrupt while preserving ordinary
	   type-ahead and the non-interrupting casting whitelist. */
	struct txt_q disabled_abort_queue = {};
	char dest[MAX_INPUT_LENGTH] = {};
	push(&disabled_abort_queue, "rest");
	push(&disabled_abort_queue, "abort");
	push(&disabled_abort_queue, "meditate");
	if (select_one(true, true, false, &disabled_abort_queue, dest) != 0)
		return 1;
	expect_queue(&disabled_abort_queue, "rest", "abort", "meditate", "abort disabled preserves queue");
	drain(&disabled_abort_queue);

	/* The predicate is intentionally independent of PLR2_WAIT: a trusted
	   character can have can_act=true while AFF2_CASTING remains active. */
	ordinary_commands_stay_queued(false, true, "wait-on casting-on");
	ordinary_commands_stay_queued(true, true, "wait-off casting-on");
	ordinary_commands_stay_queued(false, false, "wait-on casting-off");

	/* With neither gate active, normal FIFO dequeue still runs. */
	struct txt_q normal_queue = {};
	push(&normal_queue, "rest");
	push(&normal_queue, "pray");
	push(&normal_queue, "meditate");
	if (select_one(true, false, false, &normal_queue, dest) != 1)
		return 1;
	expect_text(dest, "rest", "normal first command");
	if (select_one(true, false, false, &normal_queue, dest) != 1)
		return 1;
	expect_text(dest, "pray", "normal second command");
	if (select_one(true, false, false, &normal_queue, dest) != 1)
		return 1;
	expect_text(dest, "meditate", "normal third command");
	assert(normal_queue.head == NULL && normal_queue.tail == NULL);

	/* Abort is still selectively executable behind ordinary type-ahead when
	   the player enables the escape, and the ordinary entries remain in
	   their original order for post-abort drain. */
	struct txt_q abort_queue = {};
	push(&abort_queue, "rest");
	push(&abort_queue, "abort");
	push(&abort_queue, "meditate");
	if (select_one(true, true, true, &abort_queue, dest) != 1)
		return 1;
	expect_text(dest, "abort", "abort selective dequeue");
	expect_queue(&abort_queue, "rest", "meditate", NULL, "abort preserves type-ahead");
	if (select_one(true, false, false, &abort_queue, dest) != 1)
		return 1;
	expect_text(dest, "rest", "post-abort first command");
	if (select_one(true, false, false, &abort_queue, dest) != 1)
		return 1;
	expect_text(dest, "meditate", "post-abort second command");
	assert(abort_queue.head == NULL && abort_queue.tail == NULL);

	/* Non-playing/pager/editor descriptors must not enter the casting path. */
	struct descriptor_data non_playing = {CON_PLAYING + 1, 0, NULL};
	struct char_data casting_character = {};
	casting_character.specials.affected_by2 = AFF2_CASTING;
	assert(!casting_input_for_descriptor(&non_playing, &casting_character));
	drain(&normal_queue);
	drain(&abort_queue);
	printf("casting input gate state matrix: ok\n");
	return 0;
}
'''


def main() -> int:
    """Compile and execute the real predicate plus real queue functions."""
    harness = "\n".join([
        PRELUDE,
        SEARCH_BLOCK,
        COMMAND_NUMBER,
        CMD_ALLOWED,
        ALLOWED,
        PREDICATE,
        GET_CASTING,
        GET_FROM,
        DRIVER,
    ])
    with tempfile.TemporaryDirectory() as directory:
        source = Path(directory) / "casting_input_gate.cpp"
        binary = Path(directory) / "casting_input_gate"
        source.write_text(harness, encoding="utf-8")
        subprocess.run(
            ["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g",
             "-fsanitize=address,undefined", str(source), "-o", str(binary)],
            check=True,
        )
        subprocess.run([str(binary)], check=True)
    print("All casting input gate state-matrix checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
