#!/usr/bin/env python3
"""Regression checks for fail-closed locker result handling."""
from _paths import ROOT, SRC, extract_function
from pathlib import Path
import os
import re
import subprocess
import tempfile

source = (SRC / "storage_lockers.c").read_text()

assert "int name_len = snprintf(name, sizeof(name), \"%s\", esc_locker_name);" in source
assert "(size_t)name_len >= sizeof(name)" in source
assert "locker_require_owner" in source
assert "locker_require_active_user" not in source
assert "Only the locker owner can manage access." in source
assert "bool is_owner = locker_char && esc_locker_name_matches_player" in source
# Non-owner opens must pass the async verification and credential recheck.
opening = source[source.index("static int locker_opencmd(P_char ch, char *arg)\n{"):source.index("static int locker_closecmd(P_char ch, char * /*arg*/)\n{")]
assert "sql_verify_chest_password" not in opening
assert "if (is_owner)" in opening
assert "if (!sql_get_chest_password_hash(chest_id, &hash))" in opening
lookup_failure = opening[opening.index("if (!sql_get_chest_password_hash"):opening.index("if (!hash || !arg2[0])")]
assert "finish(ch->desc, 0, nullptr);" in lookup_failure
assert "return TRUE;" in lookup_failure
assert "finish(ch->desc, !hash && !arg2[0], nullptr);" in opening
assert "password_async_start(" in opening
assert "password_work_submit(arg2, hash, nullptr, 1, 1)" in opening
assert "valid && sql_finish_chest_password(chest_id, expected.c_str()," in opening
assert "sql_get_chest_id(locker_id, name.c_str()) != chest_id" in opening
assert opening.index("if (!valid)") < opening.index("locker->SetCurrentChestId(chest_id)")
failed = opening[opening.index("if (!valid)"):opening.index("locker->SetCurrentChestId(chest_id)")]
assert "CHEST_ACTION_FAIL" in failed and "return;" in failed
assert "if (!submitted)" in opening
assert "Password service is busy; try again later." in opening
assert "strcpy(name, esc_locker_name);" not in source
# locker_access_canAccess now uses db_query() instead of qry() + mysql_store_result
assert "MYSQL_RES *res = db_query(\"%s\", query);" in source
assert "if (!res)" in source
# locker_access_show and locker_access_count still use qry() + mysql_store_result
assert source.count("mysql_store_result(DB)") >= 2
assert "count = mysql_num_rows(res);" in source
assert "if (mysql_num_rows(res) >= 1)" in source
# Personal-locker ownership is a stable PID/racewar decision. It is checked
# before visitor grants for both idle and actively occupied lockers.
assert "static bool locker_access_canEnter(P_char locker, P_char visitor)" in source
assert "sql_locker_owner_can_access(GET_NAME(locker), GET_PID(visitor)" in source
admission = extract_function('storage_lockers.c',
    'static P_char load_locker_char(P_char ch, char *esc_locker_name, int bValidateAccess)\n{')
assert admission.count('locker_access_canEnter(') == 2
sql_player = (SRC / "sql_player.c").read_text()
owner_access = sql_player[sql_player.index("bool sql_locker_owner_can_access") :]
assert "locker->owner_pid != owner_pid" in owner_access
assert "locker->owner_assoc_id" in owner_access
assert "identity.active && !identity.blocked && identity.racewar == racewar" in owner_access
mariadb_owner_access = owner_access[owner_access.index("bool sql_locker_owner_can_access", 1) :]
assert "JOIN account_characters ac ON ac.pid=l.owner_pid" in mariadb_owner_access
assert "l.owner_pid=%d" in mariadb_owner_access
assert "l.owner_assoc_id IS NULL" in mariadb_owner_access
assert "ac.racewar=l.racewar" in mariadb_owner_access
assert "ac.blocked=0" in mariadb_owner_access
assert "ac.deleted_at IS NULL" in mariadb_owner_access
# addAccess and remAccess now return bool
assert "static bool locker_access_remAccess" in source
assert "static bool locker_access_addAccess" in source
# callers check return values
assert "if (locker_access_addAccess(chLocker, arg2))" in source
assert "if (locker_access_remAccess(chLocker, arg2))" in source
# guild locker authorization
assert "locker_is_guild_member" in source
assert "bool is_guild_member = locker_is_guild_member(pLocker, ch);" in source
# PFileToLocker puts items on room floor, not invisible locker char
assert "obj_to_room(tmp_object, m_realRoom);" in source
# UnsortedChest rejects containers (in header)
header = (SRC / "storage_lockers.h").read_text()
assert "obj->type == ITEM_CONTAINER" in header
# Deferred terminal save
assert "event_deferredTerminalSave" in source
# CanAddAccount checks racewar
assert "AND racewar = %d" in source

print("locker result failure checks passed")

# The saved locker character lives in the holding room, while its live wrapper
# belongs to the occupant's private room. Exercise both busy detection and entry
# using production functions; persistence and access decisions are controlled here.
PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "core/mm.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#define PLAYERLOG "player"
static room_data rooms[4]{};
P_room world = rooms;
const int top_of_world = 3;
P_char character_list;
mm_ds *dead_mob_pool, *dead_pconly_pool;
static char_data locker{}, visitor{}, occupant{}, other{}, loaded{};
static extra_descr_data descriptions[8]{};
static char pointers[4][64];
static char locker_name[] = "account.owner.1.locker";
static char request_name[] = "AcCoUnT.OwNeR.1.LoCkEr";
static char visitor_name[] = "Visitor", occupant_name[] = "Occupant";
static bool allowed, deferred, saving;
static int loads, access_checks, frees, moves;
static std::string visitor_output, occupant_output;
class StorageLocker {
public:
    P_char character, user;
    P_char GetLockerChar() { return character; }
    P_char GetLockerUser() { return user; }
};
static StorageLocker active{&locker, &occupant}, unrelated{&other, &occupant};
int storage_locker(int, P_char, int, char *) { return 0; }
void logit(const char *, const char *, ...) {}
void wizlog(int, const char *, ...) {}
void sql_log(P_char, const char *, const char *, ...) {}
int god_check(char *) { return 0; }
bool sql_locker_exists_by_name(const char *name) {
    assert(!str_cmp(name, locker_name));
    return true;
}
P_char sql_load_locker_by_name(const char *) { ++loads; return &loaded; }
static bool locker_access_canEnter(P_char target, P_char who) {
    assert((target == &locker || target == &loaded) && who == &visitor);
    ++access_checks;
    return allowed;
}
static void event_deferredTerminalSave(P_char, P_char, P_obj, void *) {}
P_nevent get_scheduled(P_char ch, event_func func) {
    assert(func == event_deferredTerminalSave);
    // Only the truth value is used by these functions.
    return deferred && ch == &locker ? reinterpret_cast<P_nevent>(&locker) : nullptr;
}
bool locker_async_name_busy(const char *name) {
    assert(!str_cmp(name, locker_name));
    return saving;
}
void send_to_char(const char *text, P_char ch) {
    assert(ch == &visitor || ch == &occupant);
    (ch == &visitor ? visitor_output : occupant_output) += text;
}
void send_to_char_f(P_char ch, const char *fmt, ...) {
    char text[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);
    send_to_char(text, ch);
}
void free_char(P_char ch) { assert(ch == &loaded); ++frees; }
bool char_to_room(P_char ch, int room, int) {
    assert(ch == &loaded && room == 0);
    ch->in_room = room;
    ++moves;
    return true;
}
void *_mm_get(mm_ds *, const char *, int) { assert(false); return nullptr; }
void mm_release(mm_ds *, void *) { assert(false); }
void clear_char(P_char) { assert(false); }
void ensure_pconly_pool() { assert(false); }
static P_char create_locker_char(P_char, P_char, char *) { assert(false); return nullptr; }
static int lockerName_is_inuse(char *);
'''

DRIVER = r'''
static void attach(int room, StorageLocker *wrapper) {
    auto *description = &descriptions[2 * room];
    rooms[room].ex_description = description;
    description->next = description + 1;
    description[1].keyword = pointers[room];
    snprintf(pointers[room], sizeof(pointers[room]), "%p", static_cast<void *>(wrapper));
}
static void reset() {
    locker = {}; visitor = {}; occupant = {}; other = {}; loaded = {};
    for (auto &room : rooms) room = {};
    for (auto &description : descriptions) description = {};
    rooms[0].number = LOCKERS_START;
    rooms[1].number = LOCKERS_START + 1;
    rooms[2].number = LOCKERS_START + 2;
    rooms[3].number = LOCKERS_START + LOCKERS_MAX - 1;
    for (auto &room : rooms) room.room_flags = ROOM_LOCKER;
    locker.player.name = loaded.player.name = locker_name;
    visitor.player.name = visitor_name;
    occupant.player.name = occupant_name;
    visitor.player.level = 20;
    GET_RACEWAR(&visitor) = GET_RACEWAR(&loaded) = GET_RACEWAR(&locker) = 1;
    locker.in_room = 0; visitor.in_room = NOWHERE; occupant.in_room = 3;
    loaded.in_room = NOWHERE;
    character_list = &locker;
    active = {&locker, &occupant};
    attach(1, &active); // stale room metadata: no locker special procedure
    rooms[2].funct = storage_locker;
    attach(2, &unrelated); // active wrapper for a different locker character
    rooms[3].funct = storage_locker;
    attach(3, &active); // last allocatable private room; many VNUMs are absent
    allowed = true; deferred = saving = false;
    loads = access_checks = frees = moves = 0;
    visitor_output.clear(); occupant_output.clear();
}
static void blocked(int validate = 1) {
    assert(load_locker_char(&visitor, request_name, validate) == nullptr);
    assert(loads == 0 && frees == 0 && moves == 0);
    assert(character_list == &locker && locker.next == nullptr);
}
int main(int argc, char **argv) {
    assert(argc == 2);
    reset();
    if (!strcmp(argv[1], "occupied")) {
        assert(lockerName_is_inuse(request_name) == 1);
        blocked();
        assert(access_checks == 1);
        assert(visitor_output.find("Occupant is currently using") != std::string::npos);
        assert(occupant_output.find("Visitor is trying to get in") != std::string::npos);
    } else if (!strcmp(argv[1], "privacy")) {
        allowed = false;
        blocked();
        assert(access_checks == 1);
        assert(visitor_output == "You don't have access to that locker!\r\n");
        assert(occupant_output.empty());
    } else if (!strcmp(argv[1], "opposite-side")) {
        GET_RACEWAR(&visitor) = 2; // even a leftover explicit grant cannot disclose the occupant
        blocked();
        assert(visitor_output == "You don't have access to that locker!\r\n");
        assert(occupant_output.empty());
    } else if (!strcmp(argv[1], "staff-and-own")) {
        allowed = false;
        visitor.player.level = OVERLORD;
        GET_RACEWAR(&visitor) = 1; // staff can bypass grants only on the same side
        blocked();
        assert(access_checks == 0 && !occupant_output.empty());
        reset();
        allowed = false;
        blocked(0); // existing trusted own-locker entry path
        assert(access_checks == 0 && !occupant_output.empty());
        for (int validate : {0, 1}) {
            reset();
            visitor.player.level = OVERLORD;
            GET_RACEWAR(&visitor) = 2;
            blocked(validate);
            assert(occupant_output.empty());
            assert(visitor_output == "You don't have access to that locker!\r\n");
            reset();
            rooms[3].funct = nullptr;
            GET_RACEWAR(&visitor) = 2;
            assert(load_locker_char(&visitor, request_name, validate) == nullptr);
            assert(frees == 1 && moves == 0 && GET_RACEWAR(&loaded) == 1);
        }
    } else if (!strcmp(argv[1], "generic-occupant")) {
        for (P_char who : {static_cast<P_char>(nullptr), &visitor}) {
            reset();
            active.user = who;
            blocked();
            assert(visitor_output == "Someone is currently using that locker.  Please try later.\r\n");
            assert(occupant_output.empty());
        }
    } else if (!strcmp(argv[1], "idle")) {
        for (int state = 0; state < 3; ++state) {
            reset();
            if (state == 0) rooms[3].funct = nullptr;
            if (state == 1) attach(3, nullptr);
            if (state == 2) attach(3, &unrelated);
            assert(lockerName_is_inuse(request_name) == 0);
            assert(load_locker_char(&visitor, request_name, 1) == &loaded);
            assert(loads == 1 && moves == 1 && access_checks == 1 && frees == 0);
            assert(character_list == &loaded && loaded.next == &locker);
            assert(visitor_output.empty() && occupant_output.empty());
        }
    } else if (!strcmp(argv[1], "pending-save")) {
        for (int kind = 0; kind < 2; ++kind) {
            reset();
            rooms[3].funct = nullptr;
            deferred = kind == 0;
            saving = kind == 1;
            assert(lockerName_is_inuse(request_name) == 1);
            blocked();
            assert(visitor_output.find(deferred ? "finishing its save" : "currently using") != std::string::npos);
            assert(occupant_output.empty());
            allowed = false;
            visitor_output.clear();
            blocked();
            assert(visitor_output == "You don't have access to that locker!\r\n");
            assert(occupant_output.empty());
        }
    } else assert(false);
    printf("locker occupancy passed: %s\n", argv[1]);
}
'''

# The conditional lets the same executable regression reproduce the pre-fix bug.
helper = (extract_function("storage_lockers.c", "static StorageLocker *active_locker_for(")
          if "static StorageLocker *active_locker_for(" in source else "")
constants = "\n".join(re.findall(r"^#define LOCKERS_(?:START|MAX) \d+$", source, re.M))
with tempfile.TemporaryDirectory(prefix="locker-occupancy-") as directory:
    harness, binary = Path(directory) / "harness.cpp", Path(directory) / "harness"
    harness.write_text(PRELUDE + "\n" + constants + "\n" +
                       extract_function("db.c", "int real_room(const int virt)") + "\n" +
                       extract_function("db.c", "int real_room0(const int virt)") + "\n" +
                       extract_function("storage_lockers.c", "inline StorageLocker *GetChestList(") + "\n" +
                       helper + "\n" +
                       extract_function("storage_lockers.c", "static int lockerName_is_inuse(char *lockerName)\n{") + "\n" +
                       extract_function("storage_lockers.c", "static P_char load_locker_char(P_char ch, char *esc_locker_name, int bValidateAccess)\n{") + "\n" +
                       DRIVER)
    subprocess.run([
        "g++", "-std=c++20", "-g", "-O1", "-fsanitize=address,undefined",
        "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        "-I" + str(ROOT / "src"), str(harness), "-o", str(binary),
    ], check=True)
    failures = []
    for case in ("occupied", "privacy", "opposite-side", "staff-and-own", "generic-occupant", "idle", "pending-save"):
        result = subprocess.run([str(binary), case], env={
            **os.environ, "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
            "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1",
        })
        if result.returncode:
            failures.append(case)
    assert not failures, failures
