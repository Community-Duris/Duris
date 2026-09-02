#!/usr/bin/env python3
"""Runtime coverage for the casting input queue introduced with 'abort'.

The command-contract test next door proves get_casting_cmd_from_q() and
input_allowed_while_casting() are *written* correctly.  Nothing executed them:
comm.c and interp.c cannot be linked in isolation, so the linked-list surgery
that unlinks an allowed command out of the middle of a descriptor's input queue
had no runtime evidence at all.  A wrong tail pointer there corrupts the next
write_to_q() rather than failing visibly.

This lifts the parser and selective queue functions verbatim out of the tree,
compiles them against the real struct txt_q layout, and runs the extraction
cases.
"""

from _paths import SRC
from pathlib import Path
import re
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
def extract(source: Path, signature: str) -> str:
    """Pull one whole top-level function body out of a source file."""
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


GET_CASTING = extract(SRC / "comm.c", "int get_casting_cmd_from_q(struct txt_q *queue, char *dest)")
GET_FILTERED = extract(
    SRC / "comm.c",
    "static int get_filtered_cmd_from_q(struct txt_q *queue, char *dest,",
)
ALLOWED = extract(SRC / "interp.c", "bool input_allowed_while_casting(const char *input)")
COMMAND_NUMBER = extract(SRC / "interp.c", "static int input_command_number(const char *input)")
SEARCH_BLOCK = extract(SRC / "interp.c", "int old_search_block(const char *argument")

# Mirrors src/structs.h; the layout is what the extracted code walks.
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

/* old_search_block() returns a 1-based index -- the real CMD_* constants are
   defined that way too (CMD_ABORT 857 for "abort" at offset 856), so this
   stand-in table keeps the same convention. */
#define CMD_NONE 0
#define CMD_PETITION 1
#define CMD_RETURN 2
#define CMD_ABORT 3
static const char *command[] = { "petition", "return", "abort", "kill", "look", "\n" };

bool cmd_allowed_while_casting(int cmd)
{
	return (cmd == CMD_PETITION || cmd == CMD_RETURN || cmd == CMD_ABORT);
}

int old_search_block(const char *argument, const uint begin, uint length, const char **list,
		     const int mode);
bool input_allowed_while_casting(const char *input);
'''

# write_to_q() appends through queue->tail, so a stale tail after extraction is
# exactly the corruption this test exists to catch.
DRIVER = r'''
static void push(struct txt_q *q, const char *text)
{
	struct txt_block *block = (struct txt_block *)malloc(sizeof(struct txt_block));
	block->text = strdup(text);
	block->next = NULL;
	if (q->tail)
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

static void check_intact(struct txt_q *q, const char *label)
{
	struct txt_block *walk;
	struct txt_block *last = NULL;
	int seen = 0;

	for (walk = q->head; walk; last = walk, walk = walk->next)
	{
		assert(walk->text != NULL);
		if (++seen > 64)
		{
			fprintf(stderr, "%s: queue is cyclic\n", label);
			exit(1);
		}
	}
	if (q->tail != last)
	{
		fprintf(stderr, "%s: queue->tail is stale\n", label);
		exit(1);
	}
}

static void expect(int got, int want, const char *label)
{
	if (got != want)
	{
		fprintf(stderr, "%s: got %d want %d\n", label, got, want);
		exit(1);
	}
}

static void expect_str(const char *got, const char *want, const char *label)
{
	if (strcmp(got, want) != 0)
	{
		fprintf(stderr, "%s: got '%s' want '%s'\n", label, got, want);
		exit(1);
	}
}

int main()
{
	char dest[MAX_INPUT_LENGTH];
	struct txt_q q;

	/* The word filter itself. */
	assert(input_allowed_while_casting("abort"));
	assert(input_allowed_while_casting("ABORT"));
	assert(input_allowed_while_casting("  abort"));
	assert(input_allowed_while_casting("abort now"));
	assert(input_allowed_while_casting("petition help"));
	assert(input_allowed_while_casting("return"));
	assert(!input_allowed_while_casting("kill orc"));
	assert(!input_allowed_while_casting(""));
	assert(!input_allowed_while_casting("   "));
	assert(!input_allowed_while_casting(NULL));
	/* mode 2 falls back to a left-side match, so abbreviations resolve --
	   and resolve through the same table command_interpreter() uses, which
	   is why the gate and comm.c cannot disagree about a given input. */
	assert(input_allowed_while_casting("ab"));
	assert(!input_allowed_while_casting("l"));
	assert(!input_allowed_while_casting("ki"));

	/* 1. Allowed command sitting at the head. */
	memset(&q, 0, sizeof(q));
	push(&q, "abort");
	push(&q, "kill orc");
	expect(get_casting_cmd_from_q(&q, dest), 1, "head extract");
	expect_str(dest, "abort", "head extract text");
	check_intact(&q, "head extract");
	expect_str(q.head->text, "kill orc", "head extract remainder");
	drain(&q);

	/* 2. Allowed command buried behind type-ahead: the whole point of the
	      out-of-order pull.  The skipped entries must stay queued. */
	memset(&q, 0, sizeof(q));
	push(&q, "kill orc");
	push(&q, "look");
	push(&q, "abort");
	push(&q, "look");
	expect(get_casting_cmd_from_q(&q, dest), 1, "middle extract");
	expect_str(dest, "abort", "middle extract text");
	check_intact(&q, "middle extract");
	expect_str(q.head->text, "kill orc", "middle extract keeps type-ahead");
	drain(&q);

	/* 3. Allowed command at the tail -- queue->tail must fall back to prev. */
	memset(&q, 0, sizeof(q));
	push(&q, "kill orc");
	push(&q, "abort");
	expect(get_casting_cmd_from_q(&q, dest), 1, "tail extract");
	check_intact(&q, "tail extract");
	expect_str(q.tail->text, "kill orc", "tail extract new tail");
	/* A following write_to_q() appends through that tail. */
	push(&q, "look");
	check_intact(&q, "tail extract then append");
	expect_str(q.tail->text, "look", "append after tail extract");
	drain(&q);

	/* 4. Sole entry -- head and tail both have to clear. */
	memset(&q, 0, sizeof(q));
	push(&q, "abort");
	expect(get_casting_cmd_from_q(&q, dest), 1, "sole extract");
	assert(q.head == NULL && q.tail == NULL);
	push(&q, "abort");
	check_intact(&q, "sole extract then append");
	drain(&q);

	/* 5. Nothing allowed: the queue is left completely untouched so the
	      commands run normally once the chant ends. */
	memset(&q, 0, sizeof(q));
	push(&q, "kill orc");
	push(&q, "look");
	strcpy(dest, "sentinel");
	expect(get_casting_cmd_from_q(&q, dest), 0, "no allowed command");
	expect_str(dest, "sentinel", "dest untouched when nothing matches");
	check_intact(&q, "no allowed command");
	expect_str(q.head->text, "kill orc", "queue preserved");
	expect_str(q.tail->text, "look", "queue tail preserved");
	drain(&q);

	/* 6. Empty queue and bogus arguments. */
	memset(&q, 0, sizeof(q));
	expect(get_casting_cmd_from_q(&q, dest), 0, "empty queue");
	expect(get_casting_cmd_from_q(NULL, dest), 0, "null queue");
	expect(get_casting_cmd_from_q(&q, NULL), 0, "null dest");

	/* 7. Repeated extraction drains only the allowed entries. */
	memset(&q, 0, sizeof(q));
	push(&q, "look");
	push(&q, "abort");
	push(&q, "petition help");
	push(&q, "kill orc");
	expect(get_casting_cmd_from_q(&q, dest), 1, "drain 1");
	expect_str(dest, "abort", "drain 1 text");
	check_intact(&q, "drain 1");
	expect(get_casting_cmd_from_q(&q, dest), 1, "drain 2");
	expect_str(dest, "petition help", "drain 2 text");
	check_intact(&q, "drain 2");
	expect(get_casting_cmd_from_q(&q, dest), 0, "drain 3");
	check_intact(&q, "drain 3");
	expect_str(q.head->text, "look", "drain leaves type-ahead");
	expect_str(q.tail->text, "kill orc", "drain leaves tail");
	drain(&q);

	printf("casting input queue runtime: ok\n");
	return 0;
}
'''


def main() -> int:
    """Compile and execute the isolated casting queue regression."""
    harness = "\n".join([
        PRELUDE,
        SEARCH_BLOCK,
        COMMAND_NUMBER,
        ALLOWED,
        GET_FILTERED,
        GET_CASTING,
        DRIVER,
    ])
    with tempfile.TemporaryDirectory() as directory:
        source = Path(directory) / "casting_input_queue.cpp"
        binary = Path(directory) / "casting_input_queue"
        source.write_text(harness, encoding="utf-8")
        subprocess.run(
            ["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g",
             "-fsanitize=address,undefined", str(source), "-o", str(binary)],
            check=True,
        )
        subprocess.run([str(binary)], check=True)
    print("All casting input queue runtime checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
