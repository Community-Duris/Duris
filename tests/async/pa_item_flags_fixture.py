"""Disposable MariaDB runner and production-call harness for the S05 item-flag slice."""
from __future__ import annotations

import os
from pathlib import Path
import secrets
import shutil
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
IMAGE = "mariadb:11.4"
TOOLS_IMAGE = "duris-issue-213-tools:latest"

HARNESS = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/defines.h"
#include "magic/spells.h"
#include "player/player_load_repository.h"
#include "player/player_snapshot_repository.h"
#include "persistence/persistence_observability.h"

#include <mysql/mysql.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

extern struct room_data *world;
extern int top_of_world;
void spell_bless(int, P_char, char *, int, P_char, P_obj);
void spell_continual_light(int, P_char, char *, int, P_char, P_obj);

struct bless_dispatch
{
    int count = 0;
    P_obj target = nullptr;
    int duration = 0;
    sh_int spell = 0;
    sh_int data = 0;
    ulong extra2 = 0;
} last_bless_dispatch;

struct room_data test_rooms[1] = {};
struct room_data *world = test_rooms;
int top_of_world = 0;

void act(const char *, int, P_char, P_obj, void *, int) {}
void send_to_char(const char *, P_char) {}
void send_to_room(const char *, int) {}
int real_room(const int room) { return room; }
int BOUNDED(int lower, int value, int upper) { return value < lower ? lower : value > upper ? upper : value; }
bool require_data(const void *data, const char *, const char *, ...) { return data != nullptr; }
void logit(const char *, const char *, ...) {}
bool affected_by_spell(P_char, int) { return false; }
affected_type *affect_to_char(P_char, affected_type *affect) { return affect; }
void set_obj_affected_extra(P_obj obj, int duration, sh_int spell, sh_int data, ulong extra2)
{
    ++last_bless_dispatch.count;
    last_bless_dispatch.target = obj;
    last_bless_dispatch.duration = duration;
    last_bless_dispatch.spell = spell;
    last_bless_dispatch.data = data;
    last_bless_dispatch.extra2 = extra2;
}
extern "C" void sql_pool_discard_connection(MYSQL *) {}
char *sql_escape_string(const char *text)
{
    if (!text)
        return nullptr;
    const size_t length = std::strlen(text);
    char *copy = static_cast<char *>(std::malloc(length + 1));
    if (copy)
        std::memcpy(copy, text, length + 1);
    return copy;
}
void debug(const char *, ...) {}
nevent_schedule_result add_event(event_func, int, P_char, P_char, P_obj, int,
                                 const void *, int)
{
    return {};
}

namespace
{
void fail(const char *message)
{
    std::cerr << "ASSERTION FAILED: " << message << '\n';
    std::exit(1);
}

const char *required(const char *name)
{
    const char *value = std::getenv(name);
    if (!value || !*value)
        fail("missing fixture environment");
    return value;
}

void exec_sql(MYSQL *db, const std::string &sql)
{
    if (mysql_real_query(db, sql.data(), static_cast<unsigned long>(sql.size())) != 0)
    {
        std::cerr << "fixture SQL error=" << mysql_errno(db) << '\n';
        std::exit(2);
    }
    if (MYSQL_RES *rows = mysql_store_result(db))
        mysql_free_result(rows);
}

std::vector<std::vector<std::string>> query_rows(MYSQL *db, const std::string &sql)
{
    if (mysql_real_query(db, sql.data(), static_cast<unsigned long>(sql.size())) != 0)
    {
        std::cerr << "fixture query error=" << mysql_errno(db) << '\n';
        std::exit(2);
    }
    MYSQL_RES *rows = mysql_store_result(db);
    if (!rows)
        fail("fixture query returned no result set");
    std::vector<std::vector<std::string>> values;
    while (MYSQL_ROW row = mysql_fetch_row(rows))
    {
        std::vector<std::string> fields;
        for (unsigned int index = 0; index < mysql_num_fields(rows); ++index)
            fields.emplace_back(row[index] ? row[index] : "<NULL>");
        values.push_back(std::move(fields));
    }
    mysql_free_result(rows);
    return values;
}

MYSQL *connect_db()
{
    if (std::string(required("DB_HOST")) != "127.0.0.1" ||
        !std::string(required("DB_NAME")).starts_with("s05_item_flags_test_"))
        fail("refusing non-disposable database target");
    MYSQL *db = mysql_init(nullptr);
    if (!db || !mysql_real_connect(db, required("DB_HOST"), required("DB_USER"),
                                  required("DB_PASSWD"), required("DB_NAME"),
                                  static_cast<unsigned int>(std::strtoul(required("DB_PORT"), nullptr, 10)),
                                  nullptr, 0))
    {
        std::cerr << "fixture connection error=" << (db ? mysql_errno(db) : 0) << '\n';
        std::exit(2);
    }
    return db;
}

bool expect(bool ok, const char *message)
{
    if (!ok)
        std::cerr << "ASSERTION FAILED: " << message << '\n';
    return ok;
}

player_save_apply_result save_snapshot(MYSQL *db, const player_snapshot &snapshot)
{
    return player_snapshot_repository_apply(db, snapshot);
}

player_load_result reload_snapshot(MYSQL *db, int pid)
{
    player_load_request request{};
    request.request_id = 501;
    request.pid = pid;
    request.account_name = "s05-flags-account";
    request.include_pets = false;
    request.deadline_usec = persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
    return player_load_repository_execute(db, request);
}
}

int main()
{
    bool passed = true;
    MYSQL *db = connect_db();
    const std::string name = "S05Flags";
    exec_sql(db, "INSERT INTO player_data(name,account_name,save_revision) VALUES('" + name +
                     "','s05-flags-account',1)");
    const int pid = static_cast<int>(mysql_insert_id(db));
    if (pid <= 0)
        fail("synthetic player insert failed");
    const uint64_t root_uid = UINT64_C(900000000005050001);
    const uint64_t child_uid = UINT64_C(900000000005050002);
    exec_sql(db, "INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
                 "VALUES(1," + std::to_string(pid) + ",0,1)");
    exec_sql(db, "INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,"
                 "owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
                     std::to_string(root_uid) + "," + std::to_string(root_uid) +
                     ",NULL,1," + std::to_string(pid) + ",0,1,15001,1),(" +
                     std::to_string(child_uid) + "," + std::to_string(root_uid) + "," +
                     std::to_string(root_uid) + ",1," + std::to_string(pid) + ",0,1,15002,1)");
    exec_sql(db, "INSERT INTO player_items(pid,vnum,equip_slot,container_id,quantity,weight,cost,"
                 "timer,extra_flags,wear_flags,item_type,obj_uid,item_condition) VALUES(" +
                     std::to_string(pid) + ",15001,0,NULL,1,2,0,-1,0,0," +
                     std::to_string(ITEM_WEAPON) + "," + std::to_string(root_uid) + ",100)");
    const uint64_t root_row_id = mysql_insert_id(db);
    exec_sql(db, "INSERT INTO player_items(pid,vnum,equip_slot,container_id,quantity,weight,cost,"
                 "timer,extra_flags,wear_flags,item_type,obj_uid,item_condition) VALUES(" +
                     std::to_string(pid) + ",15002,0," + std::to_string(root_row_id) +
                     ",1,1,0,-1,0,0," + std::to_string(ITEM_OTHER) + "," +
                     std::to_string(child_uid) + ",100)");
    const auto custody_before = query_rows(
        db, "SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,"
            "owner_context_id,item_revision,vnum,state FROM item_current_owner WHERE item_uid IN (" +
            std::to_string(root_uid) + "," + std::to_string(child_uid) + ") ORDER BY item_uid");
    const auto owner_revision_before = query_rows(
        db, "SELECT owner_type,owner_id,owner_context_id,revision FROM item_owner_revision "
            "WHERE owner_type=1 AND owner_id=" + std::to_string(pid) + " AND owner_context_id=0");

    char_data caster{};
    caster.in_room = 0;
    obj_data object{};
    object.obj_uid = static_cast<unsigned long>(root_uid);
    object.type = ITEM_WEAPON;
    object.weight = 2;
    world[0].contents = &object;
    spell_continual_light(10, &caster, nullptr, SPELL_TYPE_SPELL, nullptr, &object);
    const bool live_light = (object.extra_flags & ITEM_LIT) != 0 && room_light(0, REAL) == 1;
    spell_bless(10, &caster, nullptr, SPELL_TYPE_SPELL, nullptr, &object);
    const bool bless_dispatch_ok = last_bless_dispatch.count == 1 &&
                                   last_bless_dispatch.target == &object &&
                                   last_bless_dispatch.duration == -1 &&
                                   last_bless_dispatch.spell == SPELL_BLESS &&
                                   last_bless_dispatch.data == 50 &&
                                   last_bless_dispatch.extra2 == ITEM2_BLESS;
    std::cout << "CASE live_spell_mutation uid=" << root_uid
              << " item_lit=" << (live_light ? "yes" : "no") << '\n';
    std::cout << "CASE spell_bless_dispatch target="
              << (last_bless_dispatch.target == &object ? "same" : "different")
              << " count=" << last_bless_dispatch.count
              << " spell=" << last_bless_dispatch.spell
              << " data=" << last_bless_dispatch.data
              << " extra2=" << last_bless_dispatch.extra2 << '\n';
    passed &= expect(live_light, "continual-light spell did not light the live room");
    passed &= expect(bless_dispatch_ok,
                     "object bless spell did not request its expected item flag mutation");

    player_snapshot snapshot{};
    snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    snapshot.pid = pid;
    snapshot.revision = 2;
    snapshot.components = PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY;
    snapshot.encoded_size_bound = 8192;
    player_item_snapshot blessed{};
    blessed.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
    blessed.object_uid = root_uid;
    blessed.vnum = 15001;
    blessed.type = ITEM_WEAPON;
    blessed.weight = object.weight;
    blessed.extra_flags = object.extra_flags;
    // Model the documented result of the helper request above as the SQL input;
    // this test does not substitute helper behavior for the production SQL path.
    if (bless_dispatch_ok)
    {
        blessed.extra2_flags = ITEM2_BLESS;
        blessed.dynamic_affects.push_back({TAG_ALTERED_EXTRA2, 0, 0});
        blessed.dynamic_affects.push_back({SPELL_BLESS, 50, ITEM2_BLESS});
    }
    player_item_snapshot child{};
    child.parent_index = 0;
    child.object_uid = child_uid;
    child.vnum = 15002;
    child.type = ITEM_OTHER;
    child.weight = 1;
    snapshot.items = {blessed, child};

    const player_save_apply_result saved = save_snapshot(db, snapshot);
    passed &= expect(saved.outcome == player_save_apply_outcome::applied,
                     "same-UID mutated snapshot was not applied");
    player_snapshot stale_attempt = snapshot;
    stale_attempt.revision = 1;
    const player_save_apply_result stale = save_snapshot(db, stale_attempt);
    const auto item_counts_after_stale = query_rows(
        db, "SELECT obj_uid,COUNT(*) FROM player_items WHERE pid=" + std::to_string(pid) +
            " GROUP BY obj_uid ORDER BY obj_uid");
    const auto custody_after_stale = query_rows(
        db, "SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,"
            "owner_context_id,item_revision,vnum,state FROM item_current_owner WHERE item_uid IN (" +
            std::to_string(root_uid) + "," + std::to_string(child_uid) + ") ORDER BY item_uid");
    const bool stale_safe = stale.outcome == player_save_apply_outcome::stale_revision &&
                            item_counts_after_stale.size() == 2 &&
                            item_counts_after_stale[0][1] == "1" &&
                            item_counts_after_stale[1][1] == "1" &&
                            custody_after_stale == custody_before;
    passed &= expect(stale_safe,
                     "stale save erased/duplicated UIDs or changed custody bindings");
    std::cout << "CASE failed_stale_save outcome="
              << (stale.outcome == player_save_apply_outcome::stale_revision ? "refused" : "unexpected")
              << " unique_uids=" << item_counts_after_stale.size()
              << " custody_binding=" << (custody_after_stale == custody_before ? "same" : "changed")
              << '\n';
    const auto projected_flags = query_rows(
        db, "SELECT obj_uid,extra_flags FROM player_items WHERE pid=" + std::to_string(pid) +
                " AND obj_uid=" + std::to_string(root_uid));
    passed &= expect(projected_flags.size() == 1 && projected_flags[0][0] ==
                         std::to_string(root_uid) &&
                         std::strtoull(projected_flags[0][1].c_str(), nullptr, 10) == ITEM_LIT,
                     "same UID or continual-light projection was not saved");
    std::cout << "CASE sql_save uid=" << root_uid
              << " outcome=" << (saved.outcome == player_save_apply_outcome::applied ? "applied" : "failed")
              << " item_lit_projected="
              << (projected_flags.size() == 1 &&
                          std::strtoull(projected_flags[0][1].c_str(), nullptr, 10) == ITEM_LIT
                      ? "yes" : "no")
              << " item2_bless_column=absent\n";

    mysql_close(db);
    db = connect_db();
    player_load_result reloaded = reload_snapshot(db, pid);
    if (reloaded.outcome != player_load_outcome::applied)
        std::cerr << "reload outcome=" << static_cast<int>(reloaded.outcome)
                  << " failed_component=" << (reloaded.failed_component ? reloaded.failed_component : "none")
                  << " error=" << reloaded.error_code << '\n';
    passed &= expect(reloaded.outcome == player_load_outcome::applied,
                     "fresh-connection SQL item reload failed");
    size_t root_index = reloaded.snapshot.items.size();
    size_t child_index = reloaded.snapshot.items.size();
    for (size_t index = 0; index < reloaded.snapshot.items.size(); ++index)
    {
        if (reloaded.snapshot.items[index].object_uid == root_uid)
            root_index = index;
        if (reloaded.snapshot.items[index].object_uid == child_uid)
            child_index = index;
    }
    const bool same_uid = root_index < reloaded.snapshot.items.size() &&
                          reloaded.snapshot.items[root_index].object_uid == root_uid;
    const bool child_binding = child_index < reloaded.snapshot.items.size() &&
                               reloaded.snapshot.items[child_index].object_uid == child_uid &&
                               reloaded.snapshot.items[child_index].parent_index ==
                                   static_cast<int32_t>(root_index);
    const bool restored_light = same_uid &&
                                (reloaded.snapshot.items[root_index].extra_flags & ITEM_LIT);
    const bool restored_bless = same_uid &&
                                (reloaded.snapshot.items[root_index].extra2_flags & ITEM2_BLESS) &&
                                !reloaded.snapshot.items[root_index].dynamic_affects.empty();
    obj_data restored_probe{};
    if (same_uid)
        restored_probe.extra_flags = reloaded.snapshot.items[root_index].extra_flags;
    world[0].contents = same_uid ? &restored_probe : nullptr;
    const bool visible_after_reconnect = room_light(0, REAL) == 1;
    const auto custody_after = query_rows(
        db, "SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,"
            "owner_context_id,item_revision,vnum,state FROM item_current_owner WHERE item_uid IN (" +
            std::to_string(root_uid) + "," + std::to_string(child_uid) + ") ORDER BY item_uid");
    const auto owner_revision_after = query_rows(
        db, "SELECT owner_type,owner_id,owner_context_id,revision FROM item_owner_revision "
            "WHERE owner_type=1 AND owner_id=" + std::to_string(pid) + " AND owner_context_id=0");
    const bool custody_bound = custody_after == custody_before &&
                               owner_revision_after == owner_revision_before &&
                               custody_after.size() == 2 &&
                               custody_after[0][3] == "1" && custody_after[0][4] == std::to_string(pid) &&
                               custody_after[1][3] == "1" && custody_after[1][4] == std::to_string(pid);
    const uint64_t multiplicity = std::strtoull(
        query_rows(db, "SELECT COUNT(*) FROM player_items WHERE pid=" + std::to_string(pid) +
                           " AND obj_uid=" + std::to_string(root_uid))[0][0].c_str(), nullptr, 10);
    passed &= expect(same_uid && multiplicity == 1, "reconnect lost or duplicated the mutated UID");
    passed &= expect(child_binding && custody_bound,
                     "save/reconnect changed parent, player custody, or owner revision binding");
    passed &= expect(visible_after_reconnect && restored_light,
                     "continual-light visibility did not survive SQL reconnect");
    passed &= expect(restored_bless,
                     "bless secondary flag/dynamic affect did not survive SQL reconnect");
    std::cout << "CASE reconnect uid=" << (same_uid ? "same" : "lost")
              << " multiplicity=" << multiplicity
              << " parent_binding=" << (child_binding ? "same" : "changed")
              << " custody_binding=" << (custody_bound ? "same" : "changed")
              << " visible_light=" << (visible_after_reconnect ? "yes" : "no")
              << " item_lit=" << (restored_light ? "yes" : "no")
              << " item2_bless=" << (restored_bless ? "yes" : "no")
              << " dynamic_affects="
              << (same_uid ? reloaded.snapshot.items[root_index].dynamic_affects.size() : 0)
              << '\n';
    mysql_close(db);
    if (!passed)
    {
        std::cerr << "RED: S05 item-flag SQL compatibility is incomplete\n";
        return 1;
    }
    std::cout << "[PASS] S05 continual-light/object-bless SQL persistence and reconnect\n";
    return 0;
}
'''


def _run(command: list[str], *, cwd: Path = ROOT, env: dict[str, str] | None = None,
         input_text: str | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, cwd=cwd, env=env, input=input_text, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)


def _require(result: subprocess.CompletedProcess[str], action: str) -> str:
    if result.returncode:
        raise RuntimeError(f"{action} failed (exit {result.returncode}):\n{result.stdout}")
    return result.stdout


def _sql(container: str, database: str, password: str, *, source: Path | None = None,
         statement: str | None = None) -> str:
    env = os.environ.copy()
    env["MYSQL_PWD"] = password
    command = ["docker", "exec", "-i", "-e", "MYSQL_PWD", container,
               "mariadb", "--protocol=tcp", "-h127.0.0.1", "-P3306", "-uroot",
               "--batch", "--skip-column-names", database]
    if statement is not None:
        command += ["-e", statement]
        source_text = None
    else:
        if source is None:
            raise ValueError("fixture SQL source is required")
        source_text = source.read_text()
    return _require(_run(command, env=env, input_text=source_text), "fixture SQL setup")


def run_item_flags_fixture() -> str:
    if not shutil.which("docker"):
        raise RuntimeError("Docker is required for this disposable fixture")
    if _run(["docker", "image", "inspect", IMAGE]).returncode:
        raise RuntimeError(f"required local test image is unavailable: {IMAGE}")
    if _run(["docker", "image", "inspect", TOOLS_IMAGE]).returncode:
        raise RuntimeError(f"required local test image is unavailable: {TOOLS_IMAGE}")

    token = secrets.token_hex(8)
    name = f"duris-s05-item-flags-{os.getpid()}-{token}"
    database = f"s05_item_flags_test_{token}"
    password = secrets.token_hex(24)
    env = os.environ.copy()
    env["MARIADB_ROOT_PASSWORD"] = password
    env["MARIADB_DATABASE"] = database
    env["MYSQL_PWD"] = password
    container = ""
    tools_container = ""
    output: list[str] = []
    try:
        container = _require(_run([
            "docker", "run", "--pull=never", "--rm", "-d", "--name", name,
            "--cpus=2", "--memory=2g", "--memory-swap=2g",
            "-e", "MARIADB_ROOT_PASSWORD", "-e", "MARIADB_DATABASE", IMAGE,
        ], env=env), "start disposable MariaDB").strip()

        ready = False
        for _ in range(90):
            check = _run(["docker", "exec", "-e", "MYSQL_PWD", container, "mariadb",
                          "--protocol=tcp", "-h127.0.0.1", "-P3306", "-uroot",
                          "--batch", "--skip-column-names", "-e", "SELECT 1"], env=env)
            if check.returncode == 0:
                ready = True
                break
            time.sleep(1)
        if not ready:
            logs = _run(["docker", "logs", container]).stdout
            evidence_dir = Path(tempfile.mkdtemp(prefix="s05-item-flags-db-failure-"))
            evidence = evidence_dir / "mariadb.log"
            evidence.write_text(logs)
            evidence.chmod(0o600)
            raise RuntimeError(f"disposable MariaDB did not become ready; owner-only log: {evidence}")

        env["MYSQL_PWD"] = password
        for migration in (
            ROOT / "migrations/bootstrap_multithread_safe.sql",
            ROOT / "migrations/immutable/0015_output_preferences.sql",
            ROOT / "migrations/immutable/0020_player_death_restitution.sql",
            ROOT / "migrations/immutable/0035_player_item_dynamic_state.sql",
        ):
            output.append(_sql(container, database, password, source=migration))

        verifier = ROOT / "migrations/immutable/0035_player_item_dynamic_state.sh"
        verifier_env = os.environ.copy()
        verifier_env.update({
            "DB_HOST": "127.0.0.1",
            "DB_PORT": "3306",
            "DB_USER": "root",
            "DB_PASSWD": password,
            "DB_NAME": database,
        })
        output.append(_require(
            _run([
                "docker", "exec", "-i", "-e", "DB_HOST", "-e", "DB_PORT", "-e",
                "DB_USER", "-e", "DB_PASSWD", "-e", "DB_NAME", container, "bash", "-s",
            ], env=verifier_env, input_text=verifier.read_text()),
            "verify schema-0035 item dynamic-state metadata",
        ))

        with tempfile.TemporaryDirectory(prefix="s05-item-flags-") as temporary:
            source = Path(temporary) / "item_flags.cpp"
            source.write_text(HARNESS)
            sources = [
                "src/player/player_snapshot_repository.c",
                "src/player/player_snapshot_codec.c",
                "src/player/player_save_journal.c",
                "src/player/player_load_repository.c",
                "src/persistence/quest_reward_obligation_repository.c",
                "src/item/item_transfer_command.c", "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c", "src/economy/currency_command.c",
                "src/player/player_load_topology.c",
                "src/player/player_death_recovery_query.c",
                "src/player/player_death_conflict_repository.c",
                "src/player/player_load_items.c",
                "src/persistence/persistence_observability.c",
                "src/persistence/critical_command.c",
                "src/persistence/player_death_restitution_command.c",
                "src/sql/item_extra_descr_codec.c",
                "src/magic/spell_attribute_buffs.c",
                "src/magic/spell_light_darkness.c",
                "src/world/handler.c",
            ]
            tools_name = name + "-tools"
            tools_container = _require(_run([
                "docker", "create", "--name", tools_name,
                "--network=container:" + container, "--cpus=2", "--memory=2g",
                "--memory-swap=2g", "-w", "/workspace", TOOLS_IMAGE,
                "sleep", "infinity",
            ]), "create disposable compiler container").strip()
            _require(_run(["docker", "start", tools_container]), "start compiler container")
            _require(_run(["docker", "exec", tools_container, "mkdir", "-p",
                           "/workspace/src", "/workspace/tests/async"]), "prepare compiler workspace")
            _require(_run(["docker", "cp", str(ROOT / "src") + "/.",
                           tools_container + ":/workspace/src/"]), "copy source into compiler container")
            _require(_run(["docker", "cp", str(source),
                           tools_container + ":/workspace/tests/async/item_flags.cpp"]),
                     "copy test harness into compiler container")
            source_args = " ".join(sources)
            compile_script = (
                "set -euo pipefail; "
                "read -r -a MYSQL_CFLAGS <<< \"$(mysql_config --cflags)\"; "
                "read -r -a MYSQL_LIBS <<< \"$(mysql_config --libs)\"; "
                "g++ -std=c++20 -pthread -Isrc -ffunction-sections -fdata-sections "
                "\"${MYSQL_CFLAGS[@]}\" tests/async/item_flags.cpp " + source_args +
                " -Wl,--gc-sections \"${MYSQL_LIBS[@]}\" -lcrypto -o /tmp/s05_item_flags"
            )
            compiled = _run(["docker", "exec", tools_container, "bash", "-lc", compile_script])
            output.append(_require(compiled, "compile focused spell and SQL harness"))
            run_env = os.environ.copy()
            run_env.update({
                "DB_HOST": "127.0.0.1",
                "DB_PORT": "3306",
                "DB_USER": "root",
                "DB_PASSWD": password,
                "DB_NAME": database,
            })
            run = _run(["docker", "exec", "-e", "DB_HOST", "-e", "DB_PORT", "-e",
                        "DB_USER", "-e", "DB_PASSWD", "-e", "DB_NAME", tools_container,
                        "/tmp/s05_item_flags"], env=run_env)
            output.append(run.stdout)
            if run.returncode:
                raise RuntimeError(f"S05 disposable SQL journey exit {run.returncode}:\n{run.stdout}")
    finally:
        if tools_container:
            _run(["docker", "rm", "-f", tools_container])
            absent = _run(["docker", "container", "inspect", tools_container])
            if absent.returncode == 0 or not any(
                marker in absent.stdout for marker in ("No such container", "No such object")
            ):
                raise RuntimeError("disposable S05 compiler container cleanup could not be verified")
            print(f"S05_TOOLS_FIXTURE_REMOVED container={name}-tools")
        if container:
            _run(["docker", "rm", "-f", container])
            absent = _run(["docker", "container", "inspect", container])
            if absent.returncode == 0 or not any(
                marker in absent.stdout for marker in ("No such container", "No such object")
            ):
                raise RuntimeError("disposable S05 DB container cleanup could not be verified")
            print(f"S05_FIXTURE_REMOVED container={name}")
    return "\n".join(part.strip() for part in output if part.strip())
