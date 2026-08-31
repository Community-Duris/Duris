#!/usr/bin/env python3
"""Regression test for a character vanishing from its account after death.

A player took a lethal artifact proc, the death completed through the deferred
recovery path ("Your death has been recorded; the world lets go of you."), and
the account menu that followed reported

    Account currently doesn't have any characters (0/16).

display_character_list() prints that whenever d->account->acct_character_list is
NULL, and only two things empty a live account's list: remove_char_from_list()
(deletion, which never ran here - no deletion message was printed) and
read_account(), which frees the list and installs whatever the reload returns.
This pins down the three ways the projection behind that reload can be lost:

1. account_characters is UNIQUE on char_name and the account list is selected by
   account_name, so sql_update_account_character() writing the
   get_account_name_safe() "Unknown" placeholder moves a character off its real
   account permanently while player_data still holds it.
2. read_account() installed a zero-character reload over a populated live list,
   turning any lost or rolled-back projection into an empty account menu for a
   player who is standing in the game.
3. The queued account-cache sync held a raw P_char that sql_commit() dereferenced
   later. A terminal death save runs inside an outer transaction and
   extract_char() returns the character to its pool before that commit, so the
   commit read ch->desc->account through freed memory.
"""

from pathlib import Path
import sys

from contract_text import contains, index

ROOT = Path(__file__).resolve().parents[2]
account = (ROOT / "src" / "account.c").read_text(encoding="utf-8", errors="replace")
sql = (ROOT / "src" / "sql.c").read_text(encoding="utf-8", errors="replace")
sql_player = (ROOT / "src" / "sql_player.c").read_text(encoding="utf-8", errors="replace")


def body(text, signature):
    """Return one function definition from its signature through its closing brace.

    Skips forward declarations: a definition has no ";" between its signature and
    its opening brace.
    """
    start = index(text, signature)
    while ";" in text[start + len(signature):text.index("{", start)]:
        start = index(text, signature, start + len(signature))
    depth = 0
    opening = text.index("{", start)
    for position in range(opening, len(text)):
        if text[position] == "{":
            depth += 1
        elif text[position] == "}":
            depth -= 1
            if depth == 0:
                return text[start:position + 1]
    raise AssertionError(f"unterminated function: {signature}")


checks = []

mapping = body(sql, "void sql_update_account_character(P_char ch)")
checks.append((
    "the account mapping is never rewritten to a placeholder account name",
    contains(mapping, "if (!ch->desc || !ch->desc->account || !ch->desc->account->acct_name ||")
    and contains(mapping, "outcome=skipped_no_account")
    and mapping.index("outcome=skipped_no_account")
    < mapping.index("INSERT INTO account_characters ")
))

reload_body = body(account, "int read_account(P_acct acct)")
checks.append((
    "a zero-character reload cannot empty a populated live account list",
    contains(reload_body, "if (!loaded->acct_character_list && acct->acct_character_list)")
    and contains(reload_body, "character_projection_empty")
    and reload_body.index("if (!loaded->acct_character_list && acct->acct_character_list)")
    < reload_body.index("acct->acct_name = check_and_clear(acct->acct_name);")
))
checks.append((
    "the untransferred reload DTO is released rather than leaked",
    contains(account, "static void free_acct_entry_shallow(struct acct_entry *loaded)")
    and contains(reload_body, "free_acct_entry_shallow(loaded);")
    and contains(reload_body, "flatfile_account_state_release(loaded);")
))

checks.append((
    "the queued account-cache sync survives the character being extracted",
    contains(sql_player, "static int pending_account_cache_pid = 0;")
    and not contains(sql_player, "pending_account_cache_char")
    and contains(sql_player, "static void sql_sync_account_character_cache(int pid, int room);")
))

sync = body(sql_player, "static void sql_sync_account_character_cache(int pid, int room)")
checks.append((
    "the commit-time sync resolves the pid against the live roster",
    contains(sync, "for (P_char candidate = character_list; candidate; candidate = candidate->next)")
    and contains(sync, "if (!ch || !ch->desc || !ch->desc->account || !GET_NAME(ch))")
    and sync.index("if (!ch || !ch->desc || !ch->desc->account || !GET_NAME(ch))")
    < sync.index("ch->desc->account->acct_character_list;")
))

queue = body(sql_player, "static void sql_queue_account_character_cache_sync(P_char ch, int room)")
checks.append((
    "queueing stores a pid, never a character pointer",
    contains(queue, "pending_account_cache_pid = ch ? GET_PID(ch) : 0;")
    and contains(queue, "pending_account_cache_sync = (pending_account_cache_pid > 0);")
))

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

if failed:
    print("\nFailed regression checks:")
    for name in failed:
        print(f"- {name}")
    sys.exit(1)

print("\nAll account character projection checks passed successfully.")
