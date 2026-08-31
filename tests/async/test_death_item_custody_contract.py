#!/usr/bin/env python3
"""Regression test for item custody across a player death.

make_corpse() hands the corpse's items over one asynchronous transaction at a
time: submit_next_corpse_item() submits a single item and corpse_item_completion()
submits the next when that one commits. The completion must publish while the
owner remains in the world even if their descriptor dropped during combat;
otherwise the first committed transfer stalls the chain. Extracting the
character mid-chain also stranded the remaining items as active rows in
item_current_owner while
persistence_save_character_terminal() wrote an empty player_items.

load_items() then compared the two on the next login, saw owned_count !=
payload_count, and refused the character with "Sorry, I couldn't load that
character!" (player_load_outcome::component_failure).

The fix reuses the death-extract retry that already exists for failed terminal
saves: while item_movement_transaction_player_busy() is true, the death is
deferred instead of being saved and extracted, so the chain drains against a
live character and the two sides stay in agreement.
"""

from _paths import SRC
from pathlib import Path
import re
import sys

from contract_text import contains, index

ROOT = Path(__file__).resolve().parents[2]
fight = (SRC / "fight.c").read_text(encoding="utf-8", errors="replace")
actoth = (SRC / "actoth.c").read_text(encoding="utf-8", errors="replace")
movement = (SRC / "item_movement_transaction.c").read_text(
    encoding="utf-8", errors="replace")
repository = (SRC / "player_load_repository.c").read_text(
    encoding="utf-8", errors="replace")


def body(text, signature):
    """Return one function from its signature through its closing brace."""
    start = index(text, signature)
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

# The hazard this guards against: linkdead combat deaths still have a live actor.
checks.append((
    "completions publish to a live owner without requiring a descriptor",
    contains(movement, "P_char find_live_player(uint32_t pid)") and
    contains(movement, "for (P_char character = character_list;") and
    contains(movement, "if (P_char actor = find_live_player(found->second.actor_pid))")
))
checks.append((
    "the corpse handoff is still a one-item-at-a-time chain",
    contains(body(fight, "void corpse_item_completion("),
             "(void)submit_next_corpse_item(character, corpse);")
))
checks.append((
    "the login invariant that rejected the desynchronised character still holds",
    contains(repository, "owned_count == payload_count")
))

die = body(fight, "void die(P_char ch, P_char killer)")
checks.append((
    "die() defers the death while corpse item transfers are in flight",
    contains(die, "item_movement_transaction_player_busy(ch)") and
    contains(die, 'persistence_alert(AVATAR, "player_save", "death", "none", "none",'
                  '"corpse_items_in_flight",') and
    contains(die, "schedule_death_extract_retry(ch, death_corpse_uid,")
))
checks.append((
    "the deferral happens before the terminal save and the extraction",
    die.index("item_movement_transaction_player_busy(ch)") <
    die.index("persistence_save_character_terminal(ch, RENT_DEATH)")
))
checks.append((
    "a deferred death remains dead until recovery completes",
    contains(body(fight, "static void hold_for_death_extract_retry(P_char ch)\n{"),
             "GET_HIT(ch) = 1;") and
    contains(body(fight, "static void hold_for_death_extract_retry(P_char ch)\n{"),
             "SET_POS(ch, GET_POS(ch) + STAT_DEAD);") and
    contains(body(fight, "static void schedule_death_extract_retry(P_char ch, uint64_t "
                         "corpse_uid, int delay)"),
             "hold_for_death_extract_retry(ch);") and
    die.index("persistence_save_character_terminal(ch, RENT_DEATH)") <
    die.index("GET_HIT(ch) = 1;")
))
schedule = body(
    fight,
    "static void schedule_death_extract_retry(P_char ch, uint64_t corpse_uid, int delay)",
)
checks.append((
    "the private death retry can be linked to a dead character safely",
    schedule.index("SET_POS(ch, GET_POS(ch) + STAT_NORMAL);") <
    schedule.index("const nevent_schedule_result scheduled = add_event(") <
    schedule.index("hold_for_death_extract_retry(ch);") and
    contains(schedule, "const death_extract_retry_context context = { delay, corpse_uid };") and
    contains(schedule, "NULL, NULL, 0, &context") and
    contains(schedule, '"death_recovery_schedule_failed"')
))
checks.append((
    "corpse removal cannot cancel or invalidate the character recovery event",
    not contains(schedule, "P_obj corpse") and
    not contains(schedule, "OBJ_VNUM")
))

# the forward declaration shares the name, so match through the opening brace.
retry = body(fight, "static void event_death_extract_retry(P_char ch, P_char victim, "
                    "P_obj obj, void *data)\n{")
checks.append((
    "the retry steadily polls instead of saving over a pending transfer",
    contains(retry, "item_movement_transaction_player_busy(ch)") and
    retry.index("item_movement_transaction_player_busy(ch)") <
    retry.index("persistence_save_character_terminal(ch, RENT_DEATH)") and
    contains(retry, "schedule_death_extract_retry(ch, context.corpse_uid,") and
    contains(retry, "GET_STAT(ch) != STAT_DEAD")
))
busy_retry = retry.split("if (item_movement_transaction_player_busy(ch))", 1)[1]
busy_retry = busy_retry.split("if (!persistence_save_character_terminal", 1)[0]
checks.append((
    "normal corpse handoffs do not exponentially delay the account menu",
    not contains(busy_retry, "previous_delay * 2") and
    contains(busy_retry, "DEATH_EXTRACT_RETRY_INITIAL")
))
checks.append((
    "actual terminal save failures retain bounded exponential backoff",
    contains(retry, "schedule_death_extract_retry(ch, context.corpse_uid, previous_delay * 2);")
))
checks.append((
    "a rejected handoff is resubmitted before death can be saved or extracted",
    contains(retry, "P_obj corpse = context.corpse_uid ? corpse_live_item") and
    contains(retry, "if (corpse && ch->carrying)") and
    contains(retry, "submit_next_corpse_item(ch, corpse)") and
    retry.index("if (corpse && ch->carrying)") <
    retry.index("persistence_save_character_terminal(ch, RENT_DEATH)")
))
checks.append((
    "automatic raising cannot consume a PC corpse while its item handoff is pending",
    contains(die, "spawn_raise_skipped_ownership_pending") and
    die.index("item_movement_transaction_player_busy(ch)") <
    die.index("spawn_raise_undead(killer, ch, corpse)")
))
checks.append((
    "the retry still finishes the death once nothing is pending",
    contains(retry, "extract_char_after_terminal_save(ch);") and
    contains(retry, "persistence_save_character_terminal(ch, RENT_DEATH)")
))

suicide = body(actoth, "void do_suicide(P_char ch, char * /*argument*/, int /*cmd*/)")
checks.append((
    "an already-dead character cannot confirm a second death",
    contains(suicide, "if (GET_STAT(ch) == STAT_DEAD)") and
    contains(suicide, 'send_to_char("You are already dead.\\r\\n", ch);') and
    suicide.index("if (GET_STAT(ch) == STAT_DEAD)") <
    suicide.index("if (!command_confirm)") < suicide.index("die(ch, ch);")
))

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

if failed:
    print("\nFailed regression checks:")
    for name in failed:
        print(f"- {name}")
    sys.exit(1)

print("\nAll death item custody checks passed successfully.")
