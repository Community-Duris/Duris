#!/usr/bin/env python3
"""Fail-closed contracts for every terminal locker sync fallback path."""
from _paths import ROOT, SRC, extract_function
from pathlib import Path
import os
import subprocess
import tempfile

source = (SRC / "locker_async.c").read_text()

helper_start = source.index("static int locker_sync_fallback_durable")
helper_end = source.index("\n}\n", helper_start) + 3
helper = source[helper_start:helper_end]
assert "save_locker_storage(chLocker)" in helper
assert "writeCharacter" not in source

start = source.index("static int start_one_snapshot")
end = source.index("\n}\n", start) + 3
body = source[start:end]

# Snapshot-build failure and job-queue-full must share the same durability gate.
assert body.count("locker_sync_fallback_durable(s, chLocker)") == 2
assert body.count("if (s->terminal && durable_ok)") == 2
assert body.count('"terminal_not_durable"') == 2
assert body.count("s->state = LCHK_DIRTY;") >= 2

# Neither failure branch may discard the recovery fence unconditionally.
snapshot_failed = body[body.index("if (!sql)"):body.index("if (!job_push")]
job_full = body[body.index("if (!job_push"):]
for label, branch in (("snapshot_failed", snapshot_failed), ("job_queue_full", job_full)):
    assert "if (s->terminal && durable_ok)" in branch, label
    assert "else if (s->terminal)" in branch, label
    assert branch.index("if (s->terminal && durable_ok)") < branch.index("extract_char(chLocker)"), label

print("locker terminal fallback fail-closed checks passed")

# Exercise production completion and shutdown logic with explicit persistence
# failures. The SQL boundary sees only the dedicated locker view.
PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "core/files.h"
#include <cassert>
#include <cstring>
#include <string>
#include <ctime>
#define LOCKER_ASYNC_NAME_LEN 128
static char_data carrier{}, player{}, deferred{};
static obj_data item{};
static char key[] = "account.owner.1.locker", player_name[] = "Owner";
static bool sql_ok, prepare_ok, drain_ok, in_room;
static int saves, extracts, player_saves, retries;
static unsigned long g_gen_seq = 10;
static int g_inflight = 1;
P_char character_list;
void logit(const char *, const char *, ...) {}
void persistence_alert(int, const char *, const char *, const char *, const char *, const char *, const char *, ...) {}
void extract_char(P_char ch) { assert(ch == &carrier); ++extracts; }
static int locker_count_carried_objects(P_char ch) { return ch->carrying ? 1 : 0; }
nevent_schedule_result add_event(event_func, int delay, P_char ch, P_char, P_obj, int, const void *, int) {
    assert(delay == 3 && ch == &carrier); ++retries;
    return {nevent_schedule_status::scheduled, {}};
}
static int locker_async_prepare_snapshot(P_char) {
    if (!prepare_ok) return 0;
    assert(in_room);
    carrier.carrying = &item; in_room = false;
    return 1;
}
static void locker_async_restore_snapshot_view(P_char) { carrier.carrying = nullptr; in_room = true; }
static void locker_async_request_resort(P_char, P_char) {}
static P_char find_locker_char_by_name(const char *) { return &carrier; }
static P_char find_char_by_pid(int) { return &player; }
static int locker_async_drain(int) { return drain_ok; }
class StorageLocker {
public:
    bool LockerToPFile() { return locker_async_prepare_snapshot(&player); }
    void PFileToLocker() { locker_async_restore_snapshot_view(&player); }
};
static StorageLocker active;
static StorageLocker *active_locker_for(P_char ch) { return ch == &carrier ? &active : nullptr; }
bool persistence_save_character_terminal(P_char ch, int) {
    assert(ch == &player); ++player_saves; return sql_ok;
}
'''

DRIVER = r'''
static void reset() {
    carrier = {}; player = {}; deferred = {}; item = {};
    carrier.player.name = deferred.player.name = key;
    player.player.name = player_name;
    GET_RACEWAR(&carrier) = GET_RACEWAR(&deferred) = 1;
    carrier.next = &player; player.next = &deferred;
    character_list = &carrier;
    slot = {}; slot.state = LCHK_INFLIGHT; slot.gen = 7;
    sql_ok = prepare_ok = drain_ok = in_room = true;
    saves = extracts = player_saves = retries = 0;
    g_inflight = 1;
}
int main() {
    for (int terminal : {0, 1}) {
        for (bool success : {false, true}) {
            reset(); slot.terminal = terminal; sql_ok = success;
            if (terminal) { carrier.carrying = &item; in_room = false; }
            locker_async_result result{}; result.gen = slot.gen; result.ok = 0;
            apply_result(&result);
            assert(saves == 1);
            assert(extracts == (terminal && success ? 1 : 0));
            assert(slot.state == (success ? LCHK_FREE : LCHK_DIRTY));
            if (!terminal) assert(in_room && !carrier.carrying);
            else assert(carrier.carrying == &item);
        }
    }
    reset(); prepare_ok = false;
    locker_async_result failure{}; failure.gen = slot.gen;
    apply_result(&failure);
    assert(!saves && !extracts && slot.state == LCHK_DIRTY && in_room);
    reset(); drain_ok = false;
    assert(!save_all_lockers() && !saves);
    reset(); prepare_ok = false;
    assert(!save_all_lockers() && saves == 1 && !extracts && in_room);
    for (bool success : {false, true}) {
        reset(); sql_ok = success;
        assert(save_all_lockers() == success);
        assert(saves == 2 && !extracts && in_room);
        assert(persistence_save_all_characters_terminal(RENT_CRASH) == success);
        assert(player_saves == 1);
    }
    for (bool success : {false, true}) {
        reset(); sql_ok = success; carrier.carrying = &item;
        event_deferredTerminalSave(&carrier, nullptr, nullptr, nullptr);
        assert(saves == 1 && extracts == (success ? 1 : 0));
        assert(retries == (success ? 0 : 1));
    }
    puts("PASS locker-only payload, restored-view retry, failed-save fences, open/deferred shutdown saves and player exclusion");
}
'''
slot_type = source[source.index('enum locker_async_state'):source.index('struct locker_async_job')]
result_type = source[source.index('struct locker_async_result\n{'):source.index('static struct locker_async_slot g_slots')]
header = (SRC / 'sql_player.h').read_text()
view = header[header.index('struct locker_save_data\n{'):header.index('bool sql_save_locker(')]
support = r'''
static locker_async_slot slot;
static locker_async_slot *slot_find(const char *) { return &slot; }
static void slot_clear(locker_async_slot *s) { s->state = LCHK_FREE; }
bool sql_save_locker(const locker_save_data &saved) {
    ++saves;
    assert(std::string(saved.name) == key && saved.racewar == 1 && !saved.owner_assoc_id);
    if (saves == 1 && prepare_ok) assert(saved.items == &item);
    return sql_ok;
}
'''
with tempfile.TemporaryDirectory(prefix='locker-save-boundary-') as directory:
    harness, binary = Path(directory) / 'harness.cpp', Path(directory) / 'harness'
    harness.write_text(PRELUDE + slot_type + result_type + view + support +
        extract_function('storage_lockers.c', 'locker_save_data locker_save_data_for(') + '\n' +
        extract_function('storage_lockers.c', 'bool save_locker_storage(') + '\n' +
        extract_function('storage_lockers.c', 'bool save_all_lockers(') + '\n' +
        extract_function('storage_lockers.c', 'static void event_deferredTerminalSave(P_char chLocker, P_char /*ch*/') + '\n' +
        extract_function('locker_async.c', 'static void apply_result(') + '\n' +
        extract_function('actoth.c', 'bool persistence_save_all_characters_terminal(') + '\n' + DRIVER)
    subprocess.run(['g++', '-std=c++20', '-g', '-O1', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie',
                    '-Isrc', str(harness), '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ,
        'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1', 'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'})

files = (SRC / 'files.c').read_text()
lockers = (SRC / 'storage_lockers.c').read_text()
assert 'persistence_write_character_flat_fallback' not in files
assert 'writeCharacter' not in lockers and 'only.pc->pwd' not in lockers
assert 'GET_RACE(locker_ch)' not in (SRC / 'sql_player.c').read_text()
comm = (SRC / 'comm.c').read_text()
shutdown = comm[comm.index('if (!_pwipe && !save_all_lockers())'):comm.index('PROFILES(SAVE);')]
assert 'shutdown_cancelled=1' in shutdown and 'goto resume_game_loop;' in shutdown
assert comm.index('persistence_save_all_characters_terminal(RENT_CRASH)') < comm.index('!save_all_lockers()')
