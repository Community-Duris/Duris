#!/usr/bin/env python3
"""Execute real death recovery with the real bounded collector intake map."""
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


# Reuse the real enrollment test's configuration/catalog adapters and fixture
# constructors; the map, enrollment admission and cleanup remain production code.
harness = r'''
#define main enrollment_policy_main
#include "collector_death_enrollment_harness.cpp"
#undef main
#include "core/files.h"
#include <climits>
#include <cstdio>

constexpr int DEATH_EXTRACT_RETRY_INITIAL = 4;
void persistence_report(persistence_severity, int, const char *, const char *, const char *, const char *, const char *, const char *, ...) {}
void persistence_alert(int, const char *, const char *, const char *, const char *, const char *, const char *, ...) {}
bool items_busy = false, currency_busy = false, terminal_ok = true;
int terminal_saves = 0, releases = 0, schedules = 0, item_submissions = 0;
P_obj live_corpse = nullptr;
room_data fixture_room = {};
room_data *world = &fixture_room;
struct death_extract_retry_context { int delay; uint64_t corpse_uid; };
bool item_movement_transaction_player_busy(P_char) { return items_busy; }
bool currency_transaction_player_busy(P_char) { return currency_busy; }
uint64_t persistence_observability_now_usec() { return 1000000; }
bool death_custody_wait_should_alert(uint64_t, int) { return false; }
void death_custody_wait_reset(P_char) {}
P_obj corpse_live_item(uint64_t uid) {
    return live_corpse && live_corpse->obj_uid == uid ? live_corpse : nullptr;
}
bool corpse_transfer_disputed(P_char) { return false; }
void clear_corpse_transfer_dispute(P_char) {}
bool death_wallet_pending(P_char) { return false; }
bool money_to_inventory(P_char) { std::abort(); }
bool save_disputed_death_disposition(P_char, uint64_t) { std::abort(); }
bool submit_next_corpse_item(P_char, P_obj) { ++item_submissions; return true; }
bool persistence_save_character_terminal(P_char, int) { ++terminal_saves; return terminal_ok; }
void release_after_terminal_death(P_char, const char *) { ++releases; }
void schedule_death_extract_retry(P_char, uint64_t, int) { ++schedules; }
__RETRY__

bool enrolled(P_char character, P_obj corpse) {
    std::vector<player_item_snapshot> snapshots = {item(1)};
    auto payload = transfer(corpse->value[CORPSE_SAVEID], snapshots);
    assert(collector_death_enrollment_attach(character, corpse, operation(1), snapshots, &payload));
    return payload.collector.present;
}

int main() {
    char_data character = {};
    pc_only_data player = {};
    obj_data corpse = {}, carried = {};
    character.only.pc = &player;
    character.player.name = const_cast<char *>("Collectorfixture");
    character.specials.position = STAT_DEAD;
    player.pid = 42;
    corpse.obj_uid = 900;
    corpse.type = ITEM_CORPSE;
    corpse.value[CORPSE_FLAGS] = PC_CORPSE;
    corpse.value[CORPSE_PID] = 42;
    corpse.value[CORPSE_SAVEID] = 1700000000;
    live_corpse = &corpse;
    config.policy.enabled = true;
    config.policy.collection_delay = 10;
    config.policy.sale_delay = 20;
    config.policy.holding_duration = 30;
    config.policy.price_percent = 250;
    config.policy.minimum_value = 7;
    collector_death_enrollment_reset_for_tests();
    death_extract_retry_context context = {4, corpse.obj_uid};

    // Corpse creation captures intake before checking either custody fence.
    // Exercise more distinct naked deaths than the production map's capacity.
    for (int death = 0; death < 1100; ++death) {
        corpse.value[CORPSE_SAVEID] = 1700000000 + death;
        collector_death_enrollment_begin(&character, &corpse);
        assert(enrolled(&character, &corpse));
        items_busy = death % 2 == 0;
        currency_busy = !items_busy;
        const int before_saves = terminal_saves, before_releases = releases;
        event_death_extract_retry(&character, nullptr, nullptr, &context);
        assert(terminal_saves == before_saves && releases == before_releases);
        assert(enrolled(&character, &corpse));
        items_busy = currency_busy = false;
        terminal_ok = false;
        event_death_extract_retry(&character, nullptr, nullptr, &context);
        assert(releases == before_releases && enrolled(&character, &corpse));
        terminal_ok = true;
        event_death_extract_retry(&character, nullptr, nullptr, &context);
        assert(terminal_saves == before_saves + 2 && releases == before_releases + 1);
        assert(!enrolled(&character, &corpse));
        assert(item_submissions == 0);
    }
    // A pending item handoff retains the same intake until its publication
    // removes the inventory and the terminal save succeeds.
    ++corpse.value[CORPSE_SAVEID];
    collector_death_enrollment_begin(&character, &corpse);
    character.carrying = &carried;
    const int before_releases = releases;
    event_death_extract_retry(&character, nullptr, nullptr, &context);
    assert(item_submissions == 1 && enrolled(&character, &corpse));
    assert(releases == before_releases);
    items_busy = true;
    character.carrying = nullptr;
    event_death_extract_retry(&character, nullptr, nullptr, &context);
    assert(enrolled(&character, &corpse) && releases == before_releases);
    items_busy = false;
    event_death_extract_retry(&character, nullptr, nullptr, &context);
    assert(!enrolled(&character, &corpse) && releases == before_releases + 1);
    std::puts("PASS: 1100 deferred naked deaths reclaim intake; fences, failed saves and active handoffs retain it");
}
'''.replace('__RETRY__', function("static void event_death_extract_retry(P_char ch, P_char victim, P_obj obj, void *data)\n{"))

with tempfile.TemporaryDirectory(prefix="collector-death-recovery-") as temporary:
    source = Path(temporary) / "recovery.cpp"
    binary = Path(temporary) / "recovery"
    source.write_text(harness)
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-O1", "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-no-pie",
        "-I", str(ROOT / "src"), "-I", str(ROOT / "tests/async"),
        str(ROOT / "src/persistence/critical_command.c"),
        str(ROOT / "src/item/item_transfer_command.c"),
        str(ROOT / "src/economy/collector_policy.c"),
        str(ROOT / "src/economy/collector_death_enrollment.c"),
        str(source), "-lcrypto", "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
