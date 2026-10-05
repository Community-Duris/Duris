#!/usr/bin/env python3
"""Production copyover save -> exec -> recover with the real runtime custody ledger.

The fixture has no clients or NPCs. Worker drains are idle; unrelated player,
network and NPC services must not be called. No database or running game is used.
"""
import ast
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def remove_function(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[:start] + source[end:]


# Reuse only the small in-memory world fixture, without executing its test runner.
module = ast.parse((ROOT / "tests/async/test_world_recovery_pipeline.py").read_text())
support = next(ast.literal_eval(node.value) for node in module.body
               if isinstance(node, ast.Assign)
               and any(isinstance(t, ast.Name) and t.id == "HARNESS" for t in node.targets))
support = support[:support.index("struct publish_gate")]
for signature in (
    "P_char copyover_restore_mob_from_buffer(",
    "bool sql_persistence_reconcile_world_recovery_items(",
    "bool item_ownership_runtime_hydrate_many_atomic(",
    "bool item_ownership_runtime_lookup(",
    "bool item_owner_identity_equal(",
    "bool item_owner_identity_valid(",
    "bool world_recovery_rehydrate_npc_items(",
    "int copyover_write_door_to_buffer(",
    "int copyover_write_zone_age_to_buffer(",
):
    support = remove_function(support, signature)

HARNESS = r'''
#include "persistence/locker_async.h"
#include "persistence/maintenance_scheduler.h"
#include "persistence/critical_command_coordinator.h"
#include "persistence/critical_outbox.h"
#include "player/player_save_pipeline.h"
#include "redis/redis_world_runtime.h"
#include "player/player_load_pipeline.h"
#include "player/player_load_materialize.h"
#include "core/mm.h"
#include "telemetry/telemetry_runtime.h"
#include "persistence/copyover_codec.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <cerrno>
#include <cstdarg>

static int publication_fault = 0;
static int worker_resumes = 0, redis_resumes = 0, queued_notices = 0, exec_attempts = 0;
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd) {
    struct stat status{}; assert(fstat(fd, &status) == 0);
    if ((publication_fault == 1 && S_ISREG(status.st_mode)) ||
        (publication_fault == 2 && S_ISDIR(status.st_mode))) { errno = EIO; return -1; }
    return __real_fsync(fd);
}
extern "C" int __real_rename(const char *, const char *);
extern "C" int __wrap_rename(const char *from, const char *to) {
    if (publication_fault == 3) { errno = EIO; return -1; }
    return __real_rename(from, to);
}
extern "C" int __real_fclose(FILE *);
extern "C" int __wrap_fclose(FILE *file) {
    const int result = __real_fclose(file);
    if (publication_fault == 4) { errno = EIO; return EOF; }
    return result;
}
extern "C" size_t __real_fwrite(const void *, size_t, size_t, FILE *);
extern "C" size_t __wrap_fwrite(const void *data, size_t size, size_t count, FILE *file) {
    if (publication_fault == 5) { errno = ENOSPC; return 0; }
    return __real_fwrite(data, size, count, file);
}

// This no-player custody fixture must never enter gameplay telemetry adapters.
// Its clock is unavailable, so the telemetry durability barrier is not entered.
bool telemetry_runtime_now(telemetry_monotonic_usec *, telemetry_utc_usec *) noexcept
{ return false; }
telemetry_runtime_outcome telemetry_runtime_flush_for_copyover(telemetry_monotonic_usec)
{ std::abort(); }
telemetry_handoff_result telemetry_runtime_game_handoff_copy(P_char) { std::abort(); }
telemetry_capture_result telemetry_runtime_game_session_resume(
    P_char, P_desc, const telemetry_session_handoff *) { std::abort(); }

P_desc descriptor_list = nullptr;
int RUNNING_PORT = 4000, mini_mode = 1, _copyover = 0, used_descs = 0;
int copyover_boot = 0;
struct mm_ds *dead_mob_pool = nullptr, *dead_pconly_pool = nullptr, *dead_desc_pool = nullptr;

bool sql_persistence_reconcile_world_recovery_items(const world_recovery_authority_item *,
    size_t, item_ownership_runtime_entry *, size_t) { std::abort(); }
bool world_recovery_rehydrate_npc_items(P_char const *, size_t) { std::abort(); }
bool item_creation_grant_batches_pending() { return false; }
bool death_extract_retry_pending(P_char) { return false; }
bool death_extract_retry_copy_state(P_char, uint64_t *corpse_uid, int *delay) {
    *corpse_uid = 0; *delay = 0; return true;
}
bool death_extract_retry_restore(P_char, uint64_t, int) { return false; }
void flush_pending_ship_saves() {}
bool drain_pending_ship_saves() { return true; }
int locker_async_drain(int) { return 1; }
void maintenance_scheduler_quiesce() {}
void maintenance_scheduler_resume() { ++worker_resumes; }
bool maintenance_scheduler_drain(uint64_t) { return true; }
void critical_command_coordinator_quiesce() {}
void critical_command_coordinator_resume() { ++worker_resumes; }
bool critical_command_coordinator_drain(uint64_t) { return true; }
void critical_outbox_quiesce() {}
void critical_outbox_resume() { ++worker_resumes; }
bool critical_outbox_drain(uint64_t) { return true; }
void player_save_pipeline_quiesce() {}
void player_save_pipeline_resume() { ++worker_resumes; }
bool player_save_pipeline_drain(uint64_t) { return true; }
bool redis_world_recovery_drain(uint64_t) { return true; }
bool redis_world_recovery_prepare_copyover() { return true; }
void redis_world_recovery_resume_after_copyover() { ++redis_resumes; }
bool persistence_flush_all_character_saves() { return true; }
bool persistence_log_drain(unsigned timeout_ms) { assert(timeout_ms == 3000); return true; }

// Empty-world boundaries: reaching these would make the fixture invalid.
int write_to_descriptor(P_desc, const char *) { std::abort(); }
bool persistence_save_character_terminal(P_char, int) { std::abort(); }
bool persistence_save_character_terminal_database_acknowledged(P_char, int) { std::abort(); }
int websocket_send_text(P_desc, const char *) { std::abort(); }
int compress_end(P_desc, int) { std::abort(); }
void write_to_q(const char *, txt_q *, int) { ++queued_notices; }
uint64_t persistence_observability_now_usec() { std::abort(); }
uint64_t player_load_pipeline_next_request_id() { std::abort(); }
bool player_save_journal_pid_quarantined(int) { return false; }
bool player_load_pipeline_wait(player_load_request, player_load_result *, uint64_t) { std::abort(); }
bool player_load_pipeline_execute_sync(player_load_request, player_load_result *) { std::abort(); }
bool player_load_materialize(P_char, const player_load_result &) { std::abort(); }
void player_load_pets_place(P_char) { std::abort(); }
void *_mm_get(mm_ds *, const char *, int) { std::abort(); }
unsigned mm_find_best_chunk(int, int, int) { std::abort(); }
mm_ds *mm_create(const char *, size_t, size_t, unsigned) { std::abort(); }
void mm_release(mm_ds *, void *) { std::abort(); }
void clear_char(P_char) { std::abort(); }
void register_character_runtime_id(P_char) { std::abort(); }
void free_char(P_char) { std::abort(); }
void nonblock(int) { std::abort(); }
void check_cp437(P_desc) { std::abort(); }
acct_entry *allocate_account() { std::abort(); }
int read_account(acct_entry *) { std::abort(); }
acct_entry *free_account(acct_entry *) { std::abort(); }
bool char_to_room(P_char, int, int) { std::abort(); }
P_char read_mobile(int, int) { std::abort(); }
int setup_pet(P_char, P_char, int, int) { std::abort(); }
void add_follower(P_char, P_char) { std::abort(); }
affected_type *affect_to_char(P_char, affected_type *) { std::abort(); }
char affect_total(P_char, int) { std::abort(); }
void equip_char(P_char, P_obj, int, int) { std::abort(); }
void obj_to_char(P_obj, P_char) { std::abort(); }
int bind_shopkeeper_calls = 0;
void bind_shopkeeper(P_char, int) { ++bind_shopkeeper_calls; }

// Exercise a real process replacement. Only the executable destination changes;
// the production save, publication, counting and recovery functions are intact.
extern "C" int fixture_execl(const char *path, const char *, ...) noexcept
{
    ++exec_attempts;
    if (publication_fault == 6) { errno = ENOENT; return -1; }
    assert(!std::strcmp(path, "bin/server/dms"));
    FILE *file = std::fopen(COPYOVER_FILE, "rb"); assert(file);
    copyover_decoded_state state;
    assert(copyover_codec_read(file, &state, nullptr));
    assert(state.header.version == COPYOVER_VERSION && state.header.num_objects == 1);
    assert(!state.header.num_descriptors && !state.header.num_mobs);
    std::fclose(file);
    char executable[] = "/proc/self/exe", phase[] = "recover";
    char *args[] = {executable, phase, nullptr};
    execv(executable, args);
    std::abort();
}

item_ownership_runtime_entry custody(uint64_t uid)
{
    item_ownership_runtime_entry entry = {};
    entry.item_uid = uid;
    entry.root_item_uid = uid == 900 ? 900 : 901;
    entry.parent_item_uid = uid == 902 ? 901 : 0;
    entry.owner = {item_owner_type::corpse, 77, 12};
    entry.item_revision = uid;
    entry.owner_revision = 31;
    entry.vnum = 1000;
    entry.state = item_custody_state::active;
    return entry;
}

int main(int argc, char **argv)
{
    rooms[0].number = 100; rooms[1].number = 200;
    object_indexes[0].virtual_number = 1000;
    if (argc == 2 && !std::strcmp(argv[1], "publication-failures")) {
        for (int fault : {1,2,3,4,5,6}) {
            FILE *old = std::fopen(COPYOVER_FILE, "wb"); assert(old);
            assert(std::fwrite("previous synthetic snapshot", 1, 27, old) == 27);
            assert(std::fclose(old) == 0);
            int sockets[2]; assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
            assert(fcntl(sockets[0], F_SETFD, FD_CLOEXEC) == 0);
            const int before = worker_resumes;
            publication_fault = fault;
            assert(!copyover_save(sockets[0], -1, -1));
            publication_fault = 0;
            assert(worker_resumes >= before + 4);
            assert(access((std::string(COPYOVER_FILE) + ".tmp").c_str(), F_OK) != 0);
            if (fault != 6) assert(fcntl(sockets[0], F_GETFD) == FD_CLOEXEC);
            FILE *file = std::fopen(COPYOVER_FILE, "rb"); assert(file);
            if (fault == 2 || fault == 6) {
                copyover_decoded_state state;
                assert(copyover_codec_read(file, &state, nullptr));
                assert(state.header.version == 18);
            } else {
                char bytes[27]; assert(std::fread(bytes, 1, 27, file) == 27);
                assert(!std::memcmp(bytes, "previous synthetic snapshot", 27));
            }
            std::fclose(file); close(sockets[0]); close(sockets[1]);
        }
        assert(exec_attempts == 1 && redis_resumes == 1);
        // A stale temporary file or symlink is refused without truncating it.
        const std::string temp = std::string(COPYOVER_FILE) + ".tmp";
        assert(symlink(COPYOVER_FILE, temp.c_str()) == 0);
        assert(!copyover_save(-1,-1,-1)); assert(unlink(temp.c_str()) == 0);
        char_data player{}; descriptor_data descriptor{};
        descriptor.character = &player; descriptor.descriptor = 10;
        descriptor.connected = CON_PLAYING; descriptor_list = &descriptor;
        for (int transport : {0,1,2}) {
            descriptor.websocket = transport == 0;
            descriptor.sslses = transport == 1 ? reinterpret_cast<gnutls_session_t>(&player) : nullptr;
            descriptor.connected = transport == 2 ? CON_MAIN_MENU : CON_PLAYING;
            assert(!copyover_save(-1,-1,-1));
            assert(descriptor.descriptor == 10 && exec_attempts == 1);
        }
        descriptor_list = nullptr;
        assert(queued_notices == 2); // Playing TLS/WebSocket clients receive the notice.
        puts("[PASS] write/fsync/close/rename/directory sync/exec failures, symlink refusal and unsupported transports");
        return 0;
    }
    if (argc == 2 && !std::strcmp(argv[1], "recovery-failures")) {
        P_obj root = read_object(1000, VIRTUAL);
        root->obj_uid = 900; root->type = ITEM_CORPSE;
        obj_to_room(root, 0);
        assert(item_ownership_runtime_hydrate(custody(900)));
        std::vector<char> native(WORLD_RECOVERY_MAX_RECORD_BYTES);
        const int size = copyover_write_obj_to_buffer(root, native.data(), native.size());
        assert(size > 0);
        const int32_t missing_room = 999;
        memcpy(native.data(), &missing_room, 4);
        FILE *file = std::fopen(COPYOVER_FILE, "w+b"); assert(file);
        copyover_header header{}; header.num_objects = 1;
        const int listeners[3] = {-1,-1,-1};
        assert(copyover_codec_begin(file, header, listeners));
        assert(copyover_codec_write_object(file, native.data(), size));
        assert(copyover_codec_finish(file, nullptr)); std::fclose(file);
        extract_obj(root, FALSE); item_ownership_runtime_reset();
        int telnet = 3, tls = 4, websocket = 5;
        assert(!copyover_recover(&telnet, &tls, &websocket));
        assert(telnet == -1 && tls == -1 && websocket == -1);
        assert(!object_list && !character_list && !descriptor_list);
        assert(access(COPYOVER_FILE, F_OK) == 0 && !is_copyover_boot());
        // A checksum failure is rejected before the first gameplay allocation.
        file = std::fopen(COPYOVER_FILE, "r+b"); assert(file);
        assert(fseek(file, 32, SEEK_SET) == 0);
        const int original = fgetc(file); assert(original != EOF);
        assert(fseek(file, 32, SEEK_SET) == 0); assert(fputc(original ^ 1, file) != EOF);
        std::fclose(file);
        assert(!copyover_recover(&telnet, &tls, &websocket));
        assert(!object_list && !character_list && !descriptor_list && used_descs == 0);
        assert(access(COPYOVER_FILE, F_OK) == 0);
        assert(unlink(COPYOVER_FILE) == 0);
        telnet = 3; tls = 4; websocket = 5;
        assert(!copyover_recover(&telnet, &tls, &websocket));
        assert(telnet == -1 && tls == -1 && websocket == -1);
        puts("[PASS] recovery decode/materialization failure retains evidence and resets listeners without world mutation");
        return 0;
    }
    if (argc == 1) {
        P_obj corpse = read_object(1000, VIRTUAL);
        P_obj bag = read_object(1000, VIRTUAL);
        P_obj gloves = read_object(1000, VIRTUAL);
        corpse->obj_uid = 900; corpse->type = ITEM_CORPSE;
        corpse->name = str_dup("corpse snake");
        corpse->action_description = str_dup("a cavern snake");
        corpse->wear_flags = ITEM_TAKE;
        bag->obj_uid = 901; bag->type = ITEM_CONTAINER;
        gloves->obj_uid = 902; gloves->type = ITEM_ARMOR;
        gloves->name = str_dup("boreal hardwood gloves");
        gloves->wear_flags = ITEM_TAKE | ITEM_WEAR_HANDS;
        gloves->material = 3; gloves->weight = 3;
        gloves->affected[0].location = APPLY_HIT;
        gloves->affected[0].modifier = -5;
        obj_to_room(corpse, 0); obj_to_obj(bag, corpse); obj_to_obj(gloves, bag);
        for (uint64_t uid : {900, 901, 902})
            assert(item_ownership_runtime_hydrate(custody(uid)));
        std::vector<char> buffer(WORLD_RECOVERY_MAX_RECORD_BYTES);
        assert(world_recovery_write_object_to_buffer(corpse, 100, buffer.data(), buffer.size()) == 0);
        copyover_save(-1, -1, -1);
        assert(false && "copyover_save failed before process replacement");
    }
    assert(argc == 2 && !std::strcmp(argv[1], "recover"));
    assert(!object_list && item_ownership_runtime_size() == 0);
    // A newer in-memory custody revision must survive a rejected handoff;
    // all newly materialized objects must be rolled back atomically.
    FILE *file = std::fopen(COPYOVER_FILE, "rb"); assert(file);
    copyover_decoded_state decoded;
    assert(copyover_codec_read(file, &decoded, nullptr));
    assert(decoded.objects.size() == 1);
    const auto &buffer = decoded.objects[0];
    std::fclose(file);
    auto newer = custody(902); ++newer.item_revision;
    assert(item_ownership_runtime_hydrate(newer));
    size_t consumed = 99;
    assert(!copyover_restore_obj_from_buffer(buffer.data(), buffer.size(), &consumed));
    assert(!object_list && consumed == 0 && item_ownership_runtime_size() == 1);
    item_ownership_runtime_entry retained = {};
    assert(item_ownership_runtime_lookup(902, &retained));
    assert(retained.item_revision == newer.item_revision);
    item_ownership_runtime_reset();
    int telnet = 0, tls = 0, websocket = 0;
    assert(copyover_recover(&telnet, &tls, &websocket) == 1);
    assert(telnet == -1 && tls == -1 && websocket == -1);
    assert(access(COPYOVER_FILE, F_OK) != 0);
    P_obj corpse = nullptr;
    for (P_obj object = object_list; object; object = object->next)
        if (object->obj_uid == 900) corpse = object;
    assert(corpse && corpse->loc_p == LOC_ROOM && corpse->loc.room == 0);
    assert(!std::strcmp(corpse->action_description, "a cavern snake"));
    assert(corpse->wear_flags == ITEM_TAKE);
    P_obj bag = corpse->contains;
    assert(bag && bag->obj_uid == 901 && !bag->next_content && bag->loc.inside == corpse);
    P_obj gloves = bag->contains;
    assert(gloves && gloves->obj_uid == 902 && !gloves->next_content && gloves->loc.inside == bag);
    assert(!std::strcmp(gloves->name, "boreal hardwood gloves"));
    assert(gloves->wear_flags == (ITEM_TAKE | ITEM_WEAR_HANDS) && gloves->material == 3);
    assert(gloves->affected[0].location == APPLY_HIT && gloves->affected[0].modifier == -5);
    assert(gloves->weight == 3 && bag->weight == 3 && corpse->weight == 3);
    assert(item_ownership_runtime_size() == 3);
    for (uint64_t uid : {900, 901, 902}) {
        item_ownership_runtime_entry restored = {}, expected = custody(uid);
        assert(item_ownership_runtime_lookup(uid, &restored));
        assert(restored.item_uid == expected.item_uid && restored.vnum == expected.vnum);
        assert(restored.root_item_uid == expected.root_item_uid);
        assert(restored.parent_item_uid == expected.parent_item_uid);
        assert(item_owner_identity_equal(restored.owner, expected.owner));
        assert(restored.item_revision == expected.item_revision && restored.owner_revision == 31);
        assert(restored.state == item_custody_state::active);
    }
    extract_obj(corpse, FALSE);
    item_ownership_runtime_reset();
    assert(!object_list);
    assert(bind_shopkeeper_calls == 0);
    std::puts("[PASS] production copyover save/exec/recover preserves corpse, generated loot and live custody without SQL");
}
'''

with tempfile.TemporaryDirectory(prefix="duris-copyover-custody-") as temp:
    temp = Path(temp)
    source = temp / "fixture.cpp"
    source.write_text(support + HARNESS)
    common = ["g++", "-std=c++20", "-g", "-fsanitize=address,undefined",
              "-fno-omit-frame-pointer", "-ffunction-sections", "-fdata-sections", "-Isrc", "-D__NO_MYSQL__"]
    subprocess.run(common + ["-Dexecl=fixture_execl", "-c", "src/persistence/copyover.c",
                            "-o", str(temp / "copyover.o")], cwd=ROOT, check=True)
    subprocess.run(common + [str(source), str(temp / "copyover.o"), "src/core/game_loop_watchdog.c",
                   "src/persistence/copyover_codec.c", "src/world/world_recovery_pipeline.c", "src/world/world_recovery_codec.c", "src/world/generated_npc_state.c", "src/world/generated_npc_runtime.c",
                   "src/player/pet_restore_state.c",
                            "src/item/item_ownership_runtime.c", "src/item/item_transfer_command.c", "src/world/quest_mobile_native_reference.c", "src/economy/economic_source_event.c", "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c", "src/player/player_snapshot_codec.c",
                            "src/redis/redis_command_observability.c", "-Wl,--gc-sections",
                            "-Wl,--wrap=fsync", "-Wl,--wrap=rename", "-Wl,--wrap=fclose", "-Wl,--wrap=fwrite",
                            "-lz", "-pthread", "-lgnutls", "-lbsd", '-lcrypto', "-o", str(temp / "fixture")],
                   cwd=ROOT, check=True)
    subprocess.run([str(temp / "fixture")], cwd=temp, check=True, timeout=60)
    subprocess.run([str(temp / "fixture"), "publication-failures"], cwd=temp, check=True, timeout=60)
    subprocess.run([str(temp / "fixture"), "recovery-failures"], cwd=temp, check=True, timeout=60)
