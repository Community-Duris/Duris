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

A handoff the ledger refuses outright (EMSGSIZE from a conflicting custody row)
cannot be drained that way: resubmitting reproduces the refusal, so the death
never completed and the character was never released. corpse_item_completion()
now records the dispute and the retry finalizes the death through
player_save_pipeline_terminal_death(), whose immutable record keeps the corpse
identity and location, the wallet a refused conversion never took, the complete
refused item payload and the disputed custody rows outside active inventory.
"""

from _paths import SRC
from pathlib import Path
import re
import sys
import subprocess

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
busy_retry = retry.split(
    "if (item_movement_transaction_player_busy(ch) || currency_transaction_player_busy(ch))",
    1)[1]
# only the in-flight branch: a refused handoff below it does back off.
busy_retry = busy_retry.split("P_obj corpse = context.corpse_uid", 1)[0]
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
    "a stalled handoff is resubmitted before death can be saved or extracted",
    contains(retry, "P_obj corpse = context.corpse_uid ? corpse_live_item") and
    contains(retry, "if (corpse && ch->carrying)") and
    contains(retry, "submit_next_corpse_item(ch, corpse)") and
    retry.index("if (corpse && ch->carrying)") <
    retry.index("persistence_save_character_terminal(ch, RENT_DEATH)")
))

# EMSGSIZE: the ledger refuses the row outright, so resubmitting only repeats it.
completion = body(fight, "void corpse_item_completion(")
checks.append((
    "a refused handoff is recorded as disputed instead of being resubmitted",
    contains(completion, "note_corpse_transfer_dispute(character);") and
    completion.index("note_corpse_transfer_dispute(character);") <
    completion.index('"rejected_preserved"')
))
checks.append((
    "a new death starts undisputed and an abandoned recovery retires its entry",
    contains(die, "clear_corpse_transfer_dispute(ch);") and
    die.index("clear_corpse_transfer_dispute(ch);") < die.index("make_corpse(ch, loss)") and
    contains(retry, "clear_corpse_transfer_dispute(ch);") and
    retry.index("clear_corpse_transfer_dispute(ch);") <
    retry.index('"death_recovery_abandoned"')
))
checks.append((
    "a disputed death finalizes durably instead of resubmitting the same refusal",
    contains(retry, "if (corpse_transfer_disputed(ch))") and
    retry.index("if (corpse_transfer_disputed(ch))") <
    retry.index("if (corpse && ch->carrying)") <
    retry.index("persistence_save_character_terminal(ch, RENT_DEATH)")
))
checks.append((
    "a missing corpse cannot let an ordinary empty save discard the refused assets",
    contains(retry, "if (!corpse || !save_disputed_death_disposition(ch, context.corpse_uid))")
    and contains(retry, '"death_recovery_corpse_missing"')
    and contains(retry, "schedule_death_extract_retry(ch, context.corpse_uid, previous_delay * 2);")
))
checks.append((
    "the durable record is only released once, and only after it is durable",
    contains(retry, "clear_corpse_transfer_dispute(ch);") and
    contains(retry, 'release_after_terminal_death(ch, "death_disposition_completed");') and
    retry.index("clear_corpse_transfer_dispute(ch);") <
    retry.index('release_after_terminal_death(ch, "death_disposition_completed");')
))

disposition = body(fight,
                   "static bool save_disputed_death_disposition(P_char ch, uint64_t corpse_uid)")
checks.append((
    "the disposition preserves a wallet whose conversion never committed",
    contains(disposition, "if (GET_COPPER(ch) || GET_SILVER(ch) || GET_GOLD(ch) || "
                          "GET_PLATINUM(ch))") and
    contains(disposition, "wallet_pile = create_money(GET_COPPER(ch), GET_SILVER(ch), "
                          "GET_GOLD(ch),") and
    disposition.index("create_money(") <
    disposition.index("player_save_pipeline_terminal_death(")
))
checks.append((
    "the account menu follows the durable record inside the death recovery budget",
    contains(fight, "#define DEATH_DISPOSITION_TIMEOUT_MSEC 2000") and
    contains(disposition, "DEATH_DISPOSITION_TIMEOUT_MSEC")
))
checks.append((
    "a disposition neither backend accepted keeps the live character and its assets",
    contains(disposition, "return durable;") and
    contains(disposition, "player_save_terminal_result::database_acknowledged") and
    contains(disposition, "player_save_terminal_result::journal_durable")
))
checks.append((
    "die() defers to the recovery event while a dispute is outstanding",
    contains(die, "corpse_transfer_disputed(ch)))") and
    die.index("corpse_transfer_disputed(ch)))") <
    die.index("persistence_save_character_terminal(ch, RENT_DEATH)")
))
checks.append((
    "automatic raising cannot consume a PC corpse while its item handoff is pending",
    contains(die, "spawn_raise_skipped_ownership_pending") and
    die.index("item_movement_transaction_player_busy(ch)") <
    die.index("spawn_raise_undead(killer, ch, corpse)")
))
checks.append((
    "the retry still finishes the death once nothing is pending",
    contains(retry, 'release_after_terminal_death(ch, "death_recovery_completed");') and
    contains(body(fight, "static void release_after_terminal_death(P_char ch, const char *outcome)"),
             "extract_char_after_terminal_save(ch);") and
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

# Compile the production dispute tracker and exceed the old fixed capacity.
tracker = fight[fight.index("bool corpse_transfer_disputed(P_char character)"):
                fight.index("bool submit_next_corpse_item(P_char character, P_obj corpse);")]
build = ROOT / "bin/tests/corpse-disputes"
build.mkdir(parents=True, exist_ok=True)
source = build / "regression.cpp"
source.write_text(r"""
#include <set>
#include <cassert>
struct pc_only_data { bool death_custody_disputed = false; };
struct char_data { int pid; bool pc = true; struct { pc_only_data *pc; } only; };
using P_char = char_data *;
#define IS_PC(ch) ((ch)->pc)
#define GET_PID(ch) ((ch)->pid)
""" + tracker + r"""
int main() {
    pc_only_data data[1024];
    char_data players[1024];
    for (int index = 0; index < 1024; ++index) {
        players[index] = {index + 1, true, {&data[index]}};
        note_corpse_transfer_dispute(&players[index]);
        note_corpse_transfer_dispute(&players[index]);
    }
    for (auto &ch : players) {
        assert(corpse_transfer_disputed(&ch));
        clear_corpse_transfer_dispute(&ch);
        assert(!corpse_transfer_disputed(&ch));
    }
}
""")
subprocess.run(["g++", "-std=c++20", str(source), "-o", str(build / "regression")], check=True)
subprocess.run([str(build / "regression")], check=True)
print("[PASS] 1024 simultaneous disputes remain tracked until individually cleared")

# Exercise event admission failure against the production fallback scheduler.
fallback_source = build / "fallback.cpp"
fallback_source.write_text(r"""
#include <cassert>
#include <cstdint>
#include <cstddef>
struct pc_only_data { uint64_t death_retry_corpse_uid = 0, death_retry_due_usec = 0; int death_retry_delay = 0; };
struct char_data { struct { pc_only_data *pc; } only; char_data *next = nullptr; int hit = 0, stat = 0; };
using P_char = char_data *;
using P_obj = void *;
using nevent_schedule_result = bool;
#define IS_NPC(ch) (!(ch)->only.pc)
#define IS_PC(ch) ((ch)->only.pc != nullptr)
#define GET_NAME(ch) "fixture"
#define GET_HIT(ch) ((ch)->hit)
#define GET_POS(ch) 0
#define SET_POS(ch, value) ((ch)->stat = (value))
#define STAT_NORMAL 0
#define STAT_DEAD 1
#define AVATAR 0
#define WAIT_SEC 4
#define DEATH_EXTRACT_RETRY_INITIAL 4
#define DEATH_EXTRACT_RETRY_MAX 60
uint64_t now_usec = 10;
uint64_t persistence_observability_now_usec() { return now_usec; }
bool accept_event = false;
bool add_event(void (*)(P_char,P_char,P_obj,void*), int, P_char, P_char, P_obj, int, const void*, size_t) { return accept_event; }
void persistence_alert(int, const char*, const char*, const char*, const char*, const char*, const char*, ...) {}
P_char character_list = nullptr;
struct death_extract_retry_context { int delay; uint64_t corpse_uid; };
static bool death_retry_fallback_pending = false;
static void event_death_extract_retry(P_char, P_char, P_obj, void*);
static void hold_for_death_extract_retry(P_char);
""" + body(fight, "static void schedule_death_extract_retry(P_char ch, uint64_t corpse_uid, int delay)")
    + body(fight, "void death_extract_retry_pulse(void)")
    + body(fight, "static void hold_for_death_extract_retry(P_char ch)\n{") + r"""
int attempts = 0;
static void event_death_extract_retry(P_char ch, P_char, P_obj, void *data) {
    const auto context = *static_cast<death_extract_retry_context *>(data);
    assert(context.corpse_uid == 999 && ch->stat == STAT_DEAD);
    ++attempts;
    schedule_death_extract_retry(ch, context.corpse_uid, context.delay);
}
int main() {
    pc_only_data pc;
    char_data ch{{&pc}};
    character_list = &ch;
    schedule_death_extract_retry(&ch, 999, 4);
    assert(ch.stat == STAT_DEAD && death_retry_fallback_pending);
    death_extract_retry_pulse();
    assert(attempts == 0);
    now_usec += 1000000;
    death_extract_retry_pulse();
    assert(attempts == 1 && death_retry_fallback_pending);
    accept_event = true;
    now_usec += 1000000;
    death_extract_retry_pulse();
    assert(attempts == 2 && !pc.death_retry_due_usec && !death_retry_fallback_pending);
    assert(ch.stat == STAT_DEAD);
    death_extract_retry_pulse();
    assert(attempts == 2);
}
""")
subprocess.run(["g++", "-std=c++20", str(fallback_source), "-o", str(build / "fallback")], check=True)
subprocess.run([str(build / "fallback")], check=True)
print("[PASS] refused event admission retains the dead character and retries from the game pulse")
