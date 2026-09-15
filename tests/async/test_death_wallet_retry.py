#!/usr/bin/env python3
"""A deferred wallet admission must resume normal corpse custody after publication."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
fight = (ROOT / "src/combat/fight.c").read_text()


def function(signature):
    start = fight.index(signature)
    opening = fight.index("{", start)
    depth = 0
    for end in range(opening, len(fight)):
        depth += (fight[end] == "{") - (fight[end] == "}")
        if depth == 0:
            return fight[start:end + 1]
    raise AssertionError(f"unterminated function: {signature}")


corpse = function("P_obj make_corpse(P_char ch, int loss)")
wallet_start = corpse.index("if (!IS_TRUSTED(ch)")
wallet_end = corpse.index("corpse->value[CORPSE_LEVEL]", wallet_start)
admission = corpse[wallet_start:wallet_end]
pending_signature = "static bool death_wallet_pending(P_char ch)"
pending = function(pending_signature) if pending_signature in fight else ""
tracker = fight[fight.index("bool corpse_transfer_disputed(P_char character)"):
                fight.index("bool submit_next_corpse_item(P_char character, P_obj corpse);")]
retry = function("static void event_death_extract_retry(P_char ch, P_char victim, P_obj obj, void *data)\n{")
harness = r'''
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstdio>
struct pc_data {
    bool death_custody_disputed = false;
    uint64_t death_custody_wait_since_usec = 0;
    int death_custody_wait_alerts = 0, death_custody_wait_polls = 0;
};
struct object {};
using P_obj = object *;
struct character {
    struct { pc_data *pc; } only;
    bool npc = false, trusted = false, arena = false;
    int stat = 1, cash[4] = {1, 0, 0, 0};
    const char *name = "Walletfixture";
    P_obj carrying = nullptr;
};
using P_char = character *;
#define IS_PC(ch) (!(ch)->npc)
#define IS_NPC(ch) ((ch)->npc)
#define IS_TRUSTED(ch) ((ch)->trusted)
#define CHAR_IN_ARENA(ch) ((ch)->arena)
#define GET_NAME(ch) ((ch)->name)
#define GET_STAT(ch) ((ch)->stat)
#define GET_COPPER(ch) ((ch)->cash[0])
#define GET_SILVER(ch) ((ch)->cash[1])
#define GET_GOLD(ch) ((ch)->cash[2])
#define GET_PLATINUM(ch) ((ch)->cash[3])
constexpr int STAT_DEAD = 1, AVATAR = 0, RENT_DEATH = 4;
constexpr int DEATH_EXTRACT_RETRY_INITIAL = 4;
enum class persistence_severity { info, alert };
template<class... T> void persistence_report(T...) {}
template<class... T> void persistence_alert(T...) {}
bool items_busy = false, currency_busy = false, wallet_admitted = false;
bool terminal_ok = true, disposition_ok = true;
int wallet_attempts = 0, item_submissions = 0, dispositions = 0;
int terminal_saves = 0, enrollment_ends = 0, releases = 0, schedules = 0, last_delay = 0;
object corpse_object, coin_object;
bool item_movement_transaction_player_busy(P_char) { return items_busy; }
bool currency_transaction_player_busy(P_char) { return currency_busy; }
uint64_t persistence_observability_now_usec() { return 1000000; }
bool death_custody_wait_should_alert(uint64_t, int) { return false; }
void death_custody_wait_reset(P_char) {}
P_obj corpse_live_item(uint64_t) { return &corpse_object; }
bool money_to_inventory(P_char) { ++wallet_attempts; return wallet_admitted; }
bool submit_next_corpse_item(P_char, P_obj) { ++item_submissions; return true; }
bool save_disputed_death_disposition(P_char ch, uint64_t) {
    assert(ch->cash[0] == 0); ++dispositions; return disposition_ok;
}
bool persistence_save_character_terminal(P_char ch, int) {
    assert(ch->cash[0] == 0); ++terminal_saves; return terminal_ok;
}
void collector_death_enrollment_end(P_obj) { ++enrollment_ends; }
void release_after_terminal_death(P_char, const char *) { ++releases; }
struct death_extract_retry_context { int delay; uint64_t corpse_uid; };
void schedule_death_extract_retry(P_char, uint64_t, int delay) { ++schedules; last_delay = delay; }
__TRACKER__
__PENDING__
static void start_wallet_conversion(P_char ch) { __ADMISSION__ }
__RETRY__

int main() {
    pc_data pc;
    character ch;
    ch.only.pc = &pc;
    death_extract_retry_context context = {4, 42};
    start_wallet_conversion(&ch);
    assert(wallet_attempts == 1);
    // A refusal before admission still owns the wallet and may not release it.
    event_death_extract_retry(&ch, nullptr, nullptr, &context);
    assert(wallet_attempts == 2 && !releases && !dispositions && !item_submissions);
    assert(last_delay == 8);
    wallet_admitted = true;
    event_death_extract_retry(&ch, nullptr, nullptr, &context);
    assert(wallet_attempts == 3 && last_delay == 4 && !releases);
    // Accepted is not published: wait while the currency owner remains fenced.
    currency_busy = true;
    event_death_extract_retry(&ch, nullptr, nullptr, &context);
    assert(wallet_attempts == 3 && !releases && !item_submissions);
    currency_busy = false;
    ch.cash[0] = 0;
    ch.carrying = &coin_object;
    event_death_extract_retry(&ch, nullptr, nullptr, &context);
    // The coin pile must enter normal corpse custody, not recovery quarantine.
    assert(item_submissions == 1 && dispositions == 0 && releases == 0);
    items_busy = true;
    event_death_extract_retry(&ch, nullptr, nullptr, &context);
    assert(item_submissions == 1 && !releases);
    items_busy = false;
    ch.carrying = nullptr;
    event_death_extract_retry(&ch, nullptr, nullptr, &context);
    assert(terminal_saves == 1 && enrollment_ends == 1 && releases == 1 && dispositions == 0);
    // An actual refused item handoff must still use the durable disposition.
    note_corpse_transfer_dispute(&ch);
    ch.cash[0] = 6;
    ch.carrying = &coin_object;
    event_death_extract_retry(&ch, nullptr, nullptr, &context);
    assert(wallet_attempts == 4 && dispositions == 0 && releases == 1);
    assert(corpse_transfer_disputed(&ch));
    ch.cash[0] = 0;
    event_death_extract_retry(&ch, nullptr, nullptr, &context);
    assert(dispositions == 1 && enrollment_ends == 2 && releases == 2 && item_submissions == 1);
    assert(!corpse_transfer_disputed(&ch));
    // A conflicting conversion leaves a nonzero wallet after its fence drops.
    // The retry must use the currently published balance; no terminal path may
    // capture the stale attempted amount. Revision enforcement is covered by
    // the real currency repository tests, not by this controlled adapter.
    note_corpse_transfer_dispute(&ch);
    ch.cash[0] = 9;
    currency_busy = true;
    event_death_extract_retry(&ch, nullptr, nullptr, &context);
    assert(wallet_attempts == 4 && dispositions == 1 && releases == 2);
    currency_busy = false;
    wallet_admitted = false;
    event_death_extract_retry(&ch, nullptr, nullptr, &context);
    assert(wallet_attempts == 5 && ch.cash[0] == 9 && dispositions == 1 && releases == 2);
    ch.cash[0] = 12; // New authority publication before the next admission.
    wallet_admitted = true;
    event_death_extract_retry(&ch, nullptr, nullptr, &context);
    assert(wallet_attempts == 6 && ch.cash[0] == 12 && dispositions == 1 && releases == 2);
    currency_busy = true;
    event_death_extract_retry(&ch, nullptr, nullptr, &context);
    assert(wallet_attempts == 6 && dispositions == 1 && releases == 2);
    currency_busy = false;
    ch.cash[0] = 0; // Successful authority acknowledgement published the zero.
    disposition_ok = false;
    event_death_extract_retry(&ch, nullptr, nullptr, &context);
    assert(dispositions == 2 && enrollment_ends == 2 && releases == 2 && corpse_transfer_disputed(&ch));
    assert(last_delay == 8 && wallet_attempts == 6);
    disposition_ok = true;
    event_death_extract_retry(&ch, nullptr, nullptr, &context);
    assert(dispositions == 3 && enrollment_ends == 3 && releases == 3 && !corpse_transfer_disputed(&ch));
    // An ordinary terminal save failure also retains the dead character, and
    // retrying that save does not convert the already-cleared wallet again.
    ch.carrying = nullptr;
    terminal_ok = false;
    event_death_extract_retry(&ch, nullptr, nullptr, &context);
    assert(terminal_saves == 2 && enrollment_ends == 3 && releases == 3 &&
           wallet_attempts == 6 && last_delay == 8);
    terminal_ok = true;
    event_death_extract_retry(&ch, nullptr, nullptr, &context);
    assert(terminal_saves == 3 && enrollment_ends == 4 && releases == 4 && wallet_attempts == 6);
    std::puts("PASS: deferred wallet admission, fenced publication, normal corpse handoff, and true dispute preservation");
    std::puts("PASS: conflicting balance publication, failed disposition, failed terminal save, and retries retain zero-wallet ordering");
}
'''.replace('__TRACKER__', tracker).replace('__PENDING__', pending).replace('__ADMISSION__', admission).replace('__RETRY__', retry)
with tempfile.TemporaryDirectory(prefix="death-wallet-retry-") as temporary:
    source = Path(temporary) / "retry.cpp"
    binary = Path(temporary) / "retry"
    source.write_text(harness)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
