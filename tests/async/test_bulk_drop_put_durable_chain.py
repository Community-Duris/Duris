#!/usr/bin/env python3
"""Contracts for serialized durable `drop all` / `put all`.

The live item ownership cutover made every durable move an asynchronous
transaction with an expected owner revision, so a loop that submitted N moves
for one player at once had N-1 of them rejected at apply time. `do_dropalldot`,
`drop all`, and `put all` / `put all.<name>` were therefore short-circuited for
players with "Durable items must be dropped/put away one at a time.", which
removed three fundamental commands from the game.

They now use the same serialized chain `get all` uses: snapshot the durable item
uids up front, submit one transaction, and let the completion advance the chain.
Currency, transient objects, and PC corpse roots still use their dedicated paths
once the generic ownership chain has drained.
"""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ACTOBJ = (ROOT / "src/actobj.c").read_text(encoding="utf-8", errors="replace")


def function_body(source, signature):
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for pos in range(brace, len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                return source[start : pos + 1]
    raise AssertionError(f"unterminated function: {signature}")


def check(name, condition):
    print(("PASS: " if condition else "FAIL: ") + name)
    return bool(condition)


do_drop = function_body(ACTOBJ, "void do_drop(")
do_dropalldot = function_body(ACTOBJ, "void do_dropalldot(")
do_put = function_body(ACTOBJ, "void do_put(")
put_fn = function_body(ACTOBJ, "bool put(P_char ch, P_obj o_obj, P_obj s_obj, int showit)\n{")
start_drop = function_body(ACTOBJ, "void start_bulk_drop(")
continue_drop = function_body(
    ACTOBJ, "void continue_bulk_drop(P_char actor, bool previous_succeeded)\n{"
)
finish_drop = function_body(ACTOBJ, "void finish_bulk_drop(")
start_put = function_body(ACTOBJ, "void start_bulk_put(")
continue_put = function_body(
    ACTOBJ, "void continue_bulk_put(P_char actor, bool previous_succeeded)\n{"
)
drop_completion = function_body(ACTOBJ, "void item_drop_completion(")
put_completion = function_body(ACTOBJ, "void item_put_completion(")
submit_drop = function_body(ACTOBJ, "bool submit_player_drop(")
defer_put = function_body(ACTOBJ, "bool defer_cross_owner_put(")

ok = True

# --- the blanket rejections are gone -----------------------------------------
ok &= check(
    "no command rejects players with a one-at-a-time message any more",
    "Durable items must be dropped one at a time" not in ACTOBJ
    and "Durable items must be put away one at a time" not in ACTOBJ,
)
ok &= check(
    "drop all enters the serialized bulk path",
    "start_bulk_drop(ch, NULL, false);" in do_drop,
)
ok &= check(
    "drop all.<name> enters the serialized bulk path and keeps its keyword",
    "start_bulk_drop(ch, name, true);" in do_dropalldot,
)
ok &= check(
    "drop all.coins still reaches the dedicated currency path",
    'strcmp(name, "coins")' in do_dropalldot
    and do_dropalldot.index("start_bulk_drop") < do_dropalldot.index('!strcmp(name, "coins")'),
)
ok &= check(
    "put all / put all.<name> enter the serialized bulk path",
    "start_bulk_put(ch, s_obj, type == PUT_ALLDOT ? obj_name : NULL," in do_put,
)

# --- the chains snapshot identities and advance one transaction at a time ----
ok &= check(
    "both chains snapshot durable uids rather than live pointers",
    "durable_items.push_back(object->obj_uid)" in start_drop
    and "durable_items.push_back(object->obj_uid)" in start_put,
)
ok &= check(
    "both chains refuse to start while another movement is in flight",
    "item_movement_transaction_player_busy(actor)" in start_drop
    and "item_movement_transaction_player_busy(actor)" in start_put,
)
ok &= check(
    "a submitted drop suspends the chain until its completion resumes it",
    "const bool submitted = submit_player_drop(actor, object);" in continue_drop
    and "if (submitted)\n\t\t\treturn;" in continue_drop
    and "continue_bulk_drop(actor, true);" in drop_completion,
)
ok &= check(
    "a deferred put suspends the chain until its completion resumes it",
    "if (item_put_deferred)\n\t\t\treturn;" in continue_put
    and "continue_bulk_put(actor, stored);" in put_completion,
)
ok &= check(
    "put() clears the deferral flag on every path so the chain cannot misread it",
    "item_put_deferred = false;" in put_fn,
)
ok &= check(
    "defer_cross_owner_put only reports a deferral it actually submitted",
    defer_put.count("item_put_deferred = true;") == 2
    and "item_put_deferred = false;" in defer_put,
)
ok &= check(
    "the submitters tag each transaction with the bulk owner",
    "bulk_drop_submitter" in submit_drop and "bulk_put_submitter" in defer_put,
)
ok &= check(
    "completions only advance the chain that owns the transaction",
    "context.bulk_actor_pid == static_cast<uint32_t>(GET_PID(actor))" in drop_completion
    and "context.bulk_actor_pid == static_cast<uint32_t>(GET_PID(actor))" in put_completion,
)
ok &= check(
    "a stale live topology cancels the chain instead of stalling it",
    "cancel_bulk_drop(actor);" in drop_completion
    and "cancel_bulk_put(actor);" in put_completion,
)

# --- interrupted chains stop cleanly ----------------------------------------
ok &= check(
    "dropping stops when the player leaves the room it started in",
    "actor->in_room != state.room" in continue_drop,
)
ok &= check(
    "putting stops when the container leaves or closes",
    "container is no longer here" in continue_put
    and "CONT_CLOSED" in continue_put,
)

# --- objects outside generic ownership keep their synchronous path ----------
ok &= check(
    "coins, corpse roots, and unledgered objects still use their lifecycle paths",
    "uses_generic_item_ownership(object)" in continue_drop
    and "uses_generic_item_ownership(object)" in continue_put
    and "PC_CORPSE" in ACTOBJ,
)

# --- preserved semantics ----------------------------------------------------
ok &= check(
    "cursed and soulbound items are still refused",
    "ITEM2_SOULBIND" in ACTOBJ
    and "it must be CURSED!" in ACTOBJ
    and "bulk_drop_permitted(actor, object, state)" in continue_drop,
)
ok &= check(
    "drop all.<name> reports one summary line instead of one line per item",
    "context.quiet" in drop_completion and "You drop %d %s(s)." in finish_drop,
)
ok &= check(
    "durable drops regained the wizard and artifact logging the early return skipped",
    "sql_log(actor, WIZLOG" in drop_completion
    and "dropping artifact %s (%d) in room %d." in drop_completion,
)
ok &= check(
    "a dropped player corpse still rewrites its saved file",
    "writeCorpse(object);" in drop_completion,
)

if not ok:
    raise SystemExit(1)

print("\nAll durable bulk drop/put contracts passed.")
