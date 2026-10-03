#!/usr/bin/env python3
"""Exercise journal replay against real SQL item custody in a disposable DB.

Called by test_mysql_playtime_journey and disposable economic SQL fixtures.
The fixture uses a migrated schema and synthetic rows only; it refuses
non-loopback or unmarked economic test databases and does not read .env.
"""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "player/player_save_journal.h"
#include "player/player_load_repository.h"
#include "player/player_snapshot_repository.h"
#include "persistence/persistence_observability.h"

#include <mysql/mysql.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <iterator>
#include <cerrno>
#include <limits>
#include <string>
#include <vector>

extern "C" void sql_pool_discard_connection(MYSQL *) {}

char *sql_escape_string(const char *text)
{
    const size_t length = std::strlen(text);
    char *copy = static_cast<char *>(std::malloc(length + 1));
    if (copy)
        std::memcpy(copy, text, length + 1);
    return copy;
}

MYSQL *connect_db()
{
    const char *host = std::getenv("DB_HOST");
    const char *user = std::getenv("DB_USER");
    const char *password = std::getenv("DB_PASSWD");
    const char *database = std::getenv("DB_NAME");
    const char *port_text = std::getenv("DB_PORT");
    if (!host || std::strcmp(host, "127.0.0.1") != 0 || !user || !password ||
        !database || !port_text)
    {
        std::cerr << "refusing non-disposable DB target\n";
        std::exit(2);
    }
    MYSQL *db = mysql_init(nullptr);
    const unsigned int port = static_cast<unsigned int>(std::strtoul(port_text, nullptr, 10));
    if (!db || !mysql_real_connect(db, host, user, password, database, port, nullptr, 0))
    {
        std::cerr << "mysql connect failed: " << (db ? mysql_error(db) : "mysql_init") << '\n';
        std::exit(2);
    }
    return db;
}

void exec_sql(MYSQL *db, const std::string &statement)
{
    if (mysql_query(db, statement.c_str()) != 0)
    {
        std::cerr << "SQL failed: " << mysql_error(db) << "\nstatement=" << statement << '\n';
        std::exit(2);
    }
    if (MYSQL_RES *result = mysql_store_result(db))
        mysql_free_result(result);
}

std::vector<std::vector<std::string>> query_rows(MYSQL *db, const std::string &statement)
{
    if (mysql_query(db, statement.c_str()) != 0)
    {
        std::cerr << "SQL failed: " << mysql_error(db) << "\nstatement=" << statement << '\n';
        std::exit(2);
    }
    MYSQL_RES *result = mysql_store_result(db);
    if (!result)
    {
        std::cerr << "mysql_store_result failed: " << mysql_error(db) << '\n';
        std::exit(2);
    }
    std::vector<std::vector<std::string>> rows;
    while (MYSQL_ROW row = mysql_fetch_row(result))
    {
        const unsigned int count = mysql_num_fields(result);
        std::vector<std::string> values;
        values.reserve(count);
        for (unsigned int index = 0; index < count; ++index)
            values.emplace_back(row[index] ? row[index] : "<NULL>");
        rows.push_back(std::move(values));
    }
    mysql_free_result(result);
    return rows;
}

uint64_t scalar(MYSQL *db, const std::string &statement)
{
    const auto rows = query_rows(db, statement);
    if (rows.size() != 1 || rows[0].empty() || rows[0][0] == "<NULL>")
    {
        std::cerr << "expected scalar result: " << statement << '\n';
        std::exit(2);
    }
    return std::strtoull(rows[0][0].c_str(), nullptr, 10);
}

std::string describe(player_save_journal_result result)
{
    switch (result)
    {
    case player_save_journal_result::ok: return "ok";
    case player_save_journal_result::not_initialized: return "not_initialized";
    case player_save_journal_result::invalid_path: return "invalid_path";
    case player_save_journal_result::unsafe_permissions: return "unsafe_permissions";
    case player_save_journal_result::encode_failure: return "encode_failure";
    case player_save_journal_result::io_failure: return "io_failure";
    case player_save_journal_result::quota_exceeded: return "quota_exceeded";
    case player_save_journal_result::corrupt_data: return "corrupt_data";
    case player_save_journal_result::replay_blocked: return "replay_blocked";
    case player_save_journal_result::quarantined_pid: return "quarantined_pid";
    }
    return "unknown";
}

struct replay_context
{
    MYSQL *connection;
};

player_save_apply_result apply_snapshot(const player_snapshot &snapshot, void *raw)
{
    auto *context = static_cast<replay_context *>(raw);
    return player_snapshot_repository_apply(context->connection, snapshot);
}

player_snapshot make_snapshot(int pid, player_revision_t revision,
                              const std::vector<std::pair<uint64_t, int>> &items)
{
    player_snapshot snapshot{};
    snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    snapshot.pid = pid;
    snapshot.revision = revision;
    snapshot.components = PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY;
    snapshot.encoded_size_bound = 8192;
    for (const auto &[uid, vnum] : items)
    {
        player_item_snapshot item{};
        item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
        item.object_uid = uid;
        item.vnum = vnum;
        snapshot.items.push_back(item);
    }
    return snapshot;
}

bool expect(bool condition, const char *message)
{
    if (!condition)
        std::cerr << "ASSERTION FAILED: " << message << '\n';
    return condition;
}

void seed_player(MYSQL *db, int pid, const std::string &name)
{
    exec_sql(db, "INSERT INTO player_data(pid,name,save_revision) VALUES(" +
                     std::to_string(pid) + "," + "'" + name + "',1)");
}

void open_journal(const std::string &directory)
{
    if (!player_save_journal_init(directory.c_str()))
    {
        std::cerr << "player save journal init failed for fixture\n";
        std::exit(2);
    }
}

std::vector<char> journal_bytes(const std::string &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.good()) return {};
    return std::vector<char>(std::istreambuf_iterator<char>(file), {});
}

bool exact_quarantine(int pid, const std::string &directory, const std::vector<char> &original)
{
    const auto archive = journal_bytes(directory + "/player-save.journal.quarantine.archive");
    return !original.empty() && !archive.empty() &&
           std::search(archive.begin(), archive.end(), original.begin(), original.end()) != archive.end() &&
           player_save_journal_pid_quarantined(pid) &&
           player_save_journal_health_copy().records == 0;
}

bool replay_case(MYSQL *db, const std::string &directory, const char *case_name,
                 player_save_journal_result expected)
{
    open_journal(directory);
    replay_context context{db};
    const player_save_journal_result actual = player_save_journal_replay(apply_snapshot, &context);
    const bool matched = actual == expected;
    std::cout << "CASE " << case_name << " replay=" << describe(actual)
              << " expected=" << describe(expected) << '\n';
    return matched;
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    const std::string journal_root = argv[1];
    MYSQL *db = connect_db();
    const uint64_t maximum_pid = scalar(db, "SELECT COALESCE(MAX(pid),0) FROM player_data");
    if (maximum_pid > static_cast<uint64_t>(std::numeric_limits<int>::max()) - 20)
    {
        std::cerr << "no safe synthetic PID range in disposable schema\n";
        return 2;
    }
    const int lag_pid = static_cast<int>(maximum_pid + 5);
    const int conflict_pid = lag_pid + 1;
    const int foreign_pid = lag_pid + 2;
    const int equipment_pid = lag_pid + 3;
    const int legacy_equipment_pid = lag_pid + 4;
    const int retired_pid = lag_pid + 5;
    const int orphan_pid = lag_pid + 6;
    const int orphan_pet_pid = lag_pid + 7;
    constexpr uint64_t lag_root = UINT64_C(9000000000000000100);
    constexpr uint64_t lag_child = UINT64_C(9000000000000000101);
    constexpr uint64_t foreign_item = UINT64_C(9000000000000000102);
    constexpr uint64_t equipment_item = UINT64_C(9000000000000000103);
    constexpr uint64_t legacy_equipment_item = UINT64_C(9000000000000000104);
    constexpr uint64_t retired_root = UINT64_C(9000000000000000105);
    constexpr uint64_t retired_child = UINT64_C(9000000000000000106);
    for (uint64_t uid : {lag_root, lag_child, foreign_item, equipment_item,
                         legacy_equipment_item, retired_root, retired_child})
        if (scalar(db, "SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
                           std::to_string(uid)) != 0)
        {
            std::cerr << "synthetic UID collision\n";
            return 2;
        }
    seed_player(db, lag_pid, "ItemReconLagFixture");
    seed_player(db, conflict_pid, "ItemReconConflictFixture");
    seed_player(db, foreign_pid, "ItemReconForeignFixture");
    seed_player(db, equipment_pid, "ItemReconEquipmentFixture");
    seed_player(db, legacy_equipment_pid, "ItemReconLegacyEquipFixture");
    seed_player(db, retired_pid, "ItemReconRetiredFixture");
    seed_player(db, orphan_pid, "ItemReconOrphanFixture");
    seed_player(db, orphan_pet_pid, "ItemReconOrphanPetFixture");
    exec_sql(db,
             "INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,"
             "owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
                 std::to_string(lag_root) + "," + std::to_string(lag_root) +
                 ",NULL,1," + std::to_string(lag_pid) + ",0,1,15,1),(" +
                 std::to_string(lag_child) + "," + std::to_string(lag_child) +
                 ",NULL,1," + std::to_string(lag_pid) + ",0,1,16,1),(" +
                 std::to_string(foreign_item) + "," + std::to_string(foreign_item) +
                 ",NULL,1," + std::to_string(foreign_pid) + ",0,4,17,1)");
    exec_sql(db, "INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
                 "owner_type,owner_id,owner_context_id,item_revision,vnum,state,"
                 "equipment_slot) VALUES(" + std::to_string(equipment_item) + "," +
                 std::to_string(equipment_item) + ",NULL,1," +
                 std::to_string(equipment_pid) + ",0,2,18,1,5)");
    exec_sql(db, "INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
                 "owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
                 std::to_string(legacy_equipment_item) + "," +
                 std::to_string(legacy_equipment_item) + ",NULL,1," +
                 std::to_string(legacy_equipment_pid) + ",0,1,19,1)");
    exec_sql(db, "INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
                 "owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
                 std::to_string(retired_root) + "," + std::to_string(retired_root) +
                 ",NULL,1," + std::to_string(retired_pid) + ",0,1,20,1),(" +
                 std::to_string(retired_child) + "," + std::to_string(retired_root) +
                 "," + std::to_string(retired_root) + ",1," +
                 std::to_string(retired_pid) + ",0,1,21,1)");
    exec_sql(db,
             "INSERT INTO player_items(pid,vnum,equip_slot,container_id,obj_uid) VALUES(" +
                 std::to_string(lag_pid) + ",15,0,NULL," + std::to_string(lag_root) + "),(" +
                 std::to_string(lag_pid) + ",16,0,NULL," + std::to_string(lag_child) + "),(" +
                 std::to_string(conflict_pid) + ",17,0,NULL," +
                 std::to_string(foreign_item) + ")");
    exec_sql(db, "INSERT INTO player_items(pid,vnum,equip_slot,container_id,obj_uid) VALUES(" +
                 std::to_string(equipment_pid) + ",18,0,NULL," +
                 std::to_string(equipment_item) + ")");
    exec_sql(db, "INSERT INTO player_items(pid,vnum,equip_slot,container_id,obj_uid) VALUES(" +
                 std::to_string(legacy_equipment_pid) + ",19,5,NULL," +
                 std::to_string(legacy_equipment_item) + ")");
    exec_sql(db, "INSERT INTO player_items(pid,vnum,equip_slot,container_id,obj_uid) VALUES(" +
                 std::to_string(retired_pid) + ",20,0,NULL," +
                 std::to_string(retired_root) + ")");
    const uint64_t retired_native_root = scalar(
        db, "SELECT id FROM player_items WHERE obj_uid=" + std::to_string(retired_root));
    exec_sql(db, "INSERT INTO player_items(pid,vnum,equip_slot,container_id,obj_uid) VALUES(" +
                 std::to_string(retired_pid) + ",21,0," +
                 std::to_string(retired_native_root) + "," +
                 std::to_string(retired_child) + ")");

    const std::string lag_directory = journal_root + "/owner-projection-lag";
    player_snapshot lag_snapshot = make_snapshot(lag_pid, 2, {{lag_root, 15}, {lag_child, 16}});
    open_journal(lag_directory);
    if (player_save_journal_append(lag_snapshot) != player_save_journal_result::ok)
    {
        std::cerr << "could not append synthetic player save frame\n";
        return 2;
    }
    player_save_journal_shutdown();

    // The player frame captured both items as roots. Before replay, an accepted
    // same-player move committed the child beneath lag_root; player_items remains
    // a materialized projection from the earlier save.
    exec_sql(db, "UPDATE item_current_owner SET root_item_uid=" + std::to_string(lag_root) +
                     ",parent_item_uid=" + std::to_string(lag_root) +
                     ",item_revision=2 WHERE item_uid=" + std::to_string(lag_child));
    const std::string custody_before_replay =
        "SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,"
        "owner_context_id,item_revision,vnum,state FROM item_current_owner WHERE item_uid IN (" +
        std::to_string(lag_root) + "," + std::to_string(lag_child) + ") ORDER BY item_uid";
    const auto owner_before = query_rows(db, custody_before_replay);
    mysql_close(db);
    db = connect_db();

    bool all_passed = replay_case(db, lag_directory, "recoverable_owner_projection_lag",
                                  player_save_journal_result::ok);
    const uint64_t lag_revision_after =
        scalar(db, "SELECT save_revision FROM player_data WHERE pid=" + std::to_string(lag_pid));
    const auto reloaded_child = query_rows(
        db, "SELECT child.obj_uid,COALESCE(parent.obj_uid,0) FROM player_items child "
            "LEFT JOIN player_items parent ON parent.id=child.container_id WHERE child.pid=" +
            std::to_string(lag_pid) + " AND child.obj_uid=" + std::to_string(lag_child));
    const uint64_t lag_uid_count = scalar(
        db, "SELECT COUNT(*) FROM player_items WHERE pid=" + std::to_string(lag_pid) +
                " AND obj_uid IN (" + std::to_string(lag_root) + "," +
                std::to_string(lag_child) + ")");
    const auto owner_after = query_rows(db, custody_before_replay);
    const bool projection_reloaded =
        reloaded_child.size() == 1 && reloaded_child[0].size() == 2 &&
        reloaded_child[0][0] == std::to_string(lag_child) &&
        reloaded_child[0][1] == std::to_string(lag_root);
    all_passed &= expect(lag_revision_after == 2, "lagged save revision was not committed");
    all_passed &= expect(lag_uid_count == 2, "lagged save lost or duplicated an item UID");
    all_passed &= expect(projection_reloaded, "reconnected projection did not use authoritative parent");
    all_passed &= expect(owner_after == owner_before, "save rewrote authoritative custody or revisions");
    std::cout << "CASE recoverable_owner_projection_lag revision=" << lag_revision_after
              << " projected_uids=" << lag_uid_count
              << " child_parent=" << (projection_reloaded ? "authoritative" : "stale")
              << " custody_unchanged=" << (owner_after == owner_before ? "yes" : "no") << '\n';
    player_save_journal_shutdown();
    mysql_close(db);
    db = connect_db();

    // A process restart opens the same journal directory and database again.
    // The successful replay must have checkpointed the frame, so reconnect/restart
    // cannot insert another copy or reapply a stale graph.
    open_journal(lag_directory);
    replay_context lag_context{db};
    const auto restart_replay = player_save_journal_replay(apply_snapshot, &lag_context);
    const auto restart_health = player_save_journal_health_copy();
    const uint64_t restart_revision =
        scalar(db, "SELECT save_revision FROM player_data WHERE pid=" + std::to_string(lag_pid));
    const uint64_t restart_uid_count = scalar(
        db, "SELECT COUNT(*) FROM player_items WHERE pid=" + std::to_string(lag_pid) +
                " AND obj_uid IN (" + std::to_string(lag_root) + "," +
                std::to_string(lag_child) + ")");
    all_passed &= expect(restart_replay == player_save_journal_result::ok,
                         "restarted journal did not replay cleanly");
    all_passed &= expect(restart_health.records == 0, "acknowledged frame remained after restart");
    all_passed &= expect(restart_revision == 2 && restart_uid_count == 2,
                         "restart changed revision or item multiplicity");
    std::cout << "CASE reconnect_restart replay=" << describe(restart_replay)
              << " pending_frames=" << restart_health.records
              << " revision=" << restart_revision << " projected_uids=" << restart_uid_count << '\n';
    player_save_journal_shutdown();
    mysql_close(db);
    db = connect_db();

    // The queued save captured this item as carried before a committed wear.
    // Its replay and restart must preserve the slot recorded by custody.
    const std::string equipment_directory = journal_root + "/equipment-slot-lag";
    const player_snapshot stale_equipment =
        make_snapshot(equipment_pid, 2, {{equipment_item, 18}});
    open_journal(equipment_directory);
    if (player_save_journal_append(stale_equipment) != player_save_journal_result::ok)
        return 2;
    player_save_journal_shutdown();
    const std::string equipment_owner_query =
        "SELECT item_uid,item_revision,equipment_slot FROM item_current_owner WHERE item_uid=" +
        std::to_string(equipment_item);
    const auto equipment_owner_before = query_rows(db, equipment_owner_query);
    mysql_close(db);
    db = connect_db();
    const bool equipment_replayed = replay_case(db, equipment_directory,
        "equipment_slot_lag", player_save_journal_result::ok);
    all_passed &= expect(equipment_replayed &&
                         scalar(db, "SELECT equip_slot FROM player_items WHERE obj_uid=" +
                                    std::to_string(equipment_item)) == 5 &&
                         query_rows(db, equipment_owner_query) == equipment_owner_before,
                         "stale equipment save overwrote committed custody position");
    player_save_journal_shutdown();
    mysql_close(db);
    db = connect_db();
    open_journal(equipment_directory);
    replay_context equipment_context{db};
    const auto equipment_restart = player_save_journal_replay(apply_snapshot,
                                                               &equipment_context);
    all_passed &= expect(equipment_restart == player_save_journal_result::ok &&
                         player_save_journal_health_copy().records == 0 &&
                         scalar(db, "SELECT equip_slot FROM player_items WHERE obj_uid=" +
                                    std::to_string(equipment_item)) == 5,
                         "equipment slot changed after journal restart");
    player_save_journal_shutdown();
    mysql_close(db);
    db = connect_db();

    // Inactive legacy equipment has no accounting reference. Its native slot
    // must survive a normal save while the new custody column still defaults 0.
    open_journal(journal_root + "/healthy-direct-save");
    player_snapshot legacy_equipment =
        make_snapshot(legacy_equipment_pid, 2, {{legacy_equipment_item, 19}});
    legacy_equipment.items[0].equipment_slot = 5;
    const auto legacy_saved = player_snapshot_repository_apply(db, legacy_equipment);
    all_passed &= expect(legacy_saved.outcome == player_save_apply_outcome::applied &&
                         scalar(db, "SELECT equip_slot FROM player_items WHERE obj_uid=" +
                                    std::to_string(legacy_equipment_item)) == 5,
                         "inactive legacy equipment slot was overwritten");

    exec_sql(db, "UPDATE player_data SET account_name='ItemReconTest' WHERE pid IN (" +
                     std::to_string(equipment_pid) + "," +
                     std::to_string(legacy_equipment_pid) + ")");
    exec_sql(db, "INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,"
                 "revision) VALUES(1," + std::to_string(equipment_pid) + ",0,2),(1," +
                 std::to_string(legacy_equipment_pid) + ",0,1)");
    auto load_slot = [&](int pid, uint64_t uid, uint64_t request_id)
    {
        player_load_request request{};
        request.request_id = request_id;
        request.pid = pid;
        request.account_name = "ItemReconTest";
        request.deadline_usec =
            persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
        const auto loaded = player_load_repository_execute(db, request);
        if (loaded.outcome != player_load_outcome::applied)
            std::cerr << "equipment load outcome=" << static_cast<int>(loaded.outcome)
                      << " stage=" << (loaded.failed_component ? loaded.failed_component : "none")
                      << " error=" << loaded.error_code << '\n';
        const auto item = std::find_if(loaded.snapshot.items.begin(),
                                       loaded.snapshot.items.end(),
                                       [uid](const auto &candidate)
                                       { return candidate.object_uid == uid; });
        return loaded.outcome == player_load_outcome::applied &&
               item != loaded.snapshot.items.end() ? item->equipment_slot : -1;
    };
    all_passed &= expect(load_slot(equipment_pid, equipment_item, 801) == 5,
                         "load did not use committed custody equipment slot");
    all_passed &= expect(load_slot(legacy_equipment_pid, legacy_equipment_item, 802) == 5,
                         "load overwrote inactive legacy wear");

    // A queued save contains a spell component forest that was later retired.
    // Keep the stale frame for diagnosis without restoring either native row.
    player_save_journal_shutdown();
    const std::string retired_directory = journal_root + "/retired-component-save";
    player_snapshot retired_snapshot =
        make_snapshot(retired_pid, 2, {{retired_root, 20}, {retired_child, 21}});
    retired_snapshot.items[1].parent_index = 0;
    open_journal(retired_directory);
    if (player_save_journal_append(retired_snapshot) != player_save_journal_result::ok)
        return 2;
    const auto retired_original = journal_bytes(retired_directory + "/player-save.journal");
    player_save_journal_shutdown();
    exec_sql(db, "UPDATE item_current_owner SET owner_type=8,owner_id=0,state=2,"
                 "item_revision=2 WHERE item_uid IN (" + std::to_string(retired_root) +
                 "," + std::to_string(retired_child) + ")");
    exec_sql(db, "DELETE FROM player_items WHERE id=" +
                 std::to_string(retired_native_root));
    exec_sql(db, "UPDATE player_data SET account_name='ItemReconTest' WHERE pid=" +
                 std::to_string(retired_pid));
    exec_sql(db, "INSERT INTO item_owner_revision(owner_type,owner_id,"
                 "owner_context_id,revision) VALUES(1," +
                 std::to_string(retired_pid) + ",0,2)");
    const std::string retired_owner_query =
        "SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_type,"
        "owner_id,item_revision,state FROM item_current_owner WHERE item_uid IN (" +
        std::to_string(retired_root) + "," + std::to_string(retired_child) +
        ") ORDER BY item_uid";
    const auto retired_owner_before = query_rows(db, retired_owner_query);
    auto load_retired = [&](uint64_t request_id)
    {
        player_load_request request{};
        request.request_id = request_id;
        request.pid = retired_pid;
        request.account_name = "ItemReconTest";
        request.deadline_usec =
            persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
        const auto loaded = player_load_repository_execute(db, request);
        if (loaded.outcome != player_load_outcome::cancelled)
            std::cerr << "retired load outcome=" << static_cast<int>(loaded.outcome)
                      << " stage=" << (loaded.failed_component ? loaded.failed_component : "none")
                      << " error=" << loaded.error_code << '\n';
        return loaded.outcome == player_load_outcome::cancelled &&
               loaded.error_code == EPERM && loaded.snapshot.items.empty();
    };
    mysql_close(db);
    db = connect_db();
    const bool retired_blocked = replay_case(db, retired_directory,
        "retired_component_save", player_save_journal_result::ok);
    const uint64_t retired_native_count = scalar(
        db, "SELECT COUNT(*) FROM player_items WHERE obj_uid IN (" +
            std::to_string(retired_root) + "," + std::to_string(retired_child) + ")");
    all_passed &= expect(retired_blocked &&
                         exact_quarantine(retired_pid, retired_directory, retired_original) &&
                         scalar(db, "SELECT save_revision FROM player_data WHERE pid=" +
                                    std::to_string(retired_pid)) == 1 &&
                         retired_native_count == 0 &&
                         query_rows(db, retired_owner_query) == retired_owner_before &&
                         load_retired(803),
                         "stale save resurrected a retired component");
    player_save_journal_shutdown();
    mysql_close(db);
    db = connect_db();
    open_journal(retired_directory);
    replay_context retired_context{db};
    const auto retired_restart = player_save_journal_replay(apply_snapshot,
                                                            &retired_context);
    all_passed &= expect(retired_restart == player_save_journal_result::ok &&
                         exact_quarantine(retired_pid, retired_directory, retired_original) &&
                         scalar(db, "SELECT COUNT(*) FROM player_items WHERE obj_uid IN (" +
                                    std::to_string(retired_root) + "," +
                                    std::to_string(retired_child) + ")") == 0 &&
                         query_rows(db, retired_owner_query) == retired_owner_before &&
                         load_retired(804),
                         "restart lost a blocked retirement frame or restored its items");
    std::cout << "CASE retired_component_save replay=" << describe(retired_restart)
              << " pending_frames=" << player_save_journal_health_copy().records
              << " projected_uids=" << retired_native_count << '\n';
    player_save_journal_shutdown();

    open_journal(journal_root + "/healthy-direct-refusals");
    player_snapshot incomplete_snapshot =
        make_snapshot(lag_pid, 3, {{lag_root, 15}});
    const player_save_apply_result missing_payload =
        player_snapshot_repository_apply(db, incomplete_snapshot);
    const uint64_t missing_payload_revision =
        scalar(db, "SELECT save_revision FROM player_data WHERE pid=" + std::to_string(lag_pid));
    const uint64_t missing_payload_rows =
        scalar(db, "SELECT COUNT(*) FROM player_items WHERE pid=" + std::to_string(lag_pid));
    all_passed &= expect(
        missing_payload.outcome == player_save_apply_outcome::terminal_failure &&
            missing_payload.error_code == PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH,
        "active item with missing snapshot payload was not refused");
    all_passed &= expect(missing_payload_revision == 2 && missing_payload_rows == 2,
                         "missing-payload refusal changed durable state");
    all_passed &= expect(
        missing_payload.custody_diagnosis ==
            player_save_custody_diagnosis::active_custody_absent_from_snapshot &&
            missing_payload.custody_witness.item_uid == lag_child &&
            !missing_payload.custody_witness.expected_present &&
            missing_payload.custody_witness.observed_present &&
            missing_payload.custody_witness.observed_root == lag_root &&
            missing_payload.custody_witness.observed_parent == lag_root &&
            missing_payload.custody_witness.source_line > 0,
        "missing-payload failure did not retain its precise custody witness");
    std::cout << "CASE active_owner_missing_payload outcome="
              << (missing_payload.outcome == player_save_apply_outcome::terminal_failure
                          ? "terminal_failure"
                          : "unexpected")
              << " revision=" << missing_payload_revision
              << " projected_uids=" << missing_payload_rows << std::endl;

    // Load skips rows with no custody. A later full save or pet replacement must
    // retain that sole payload for the one-time ownership repair.
    exec_sql(db, "INSERT INTO player_items(pid,vnum,equip_slot,obj_uid) VALUES(" +
                     std::to_string(orphan_pid) + ",22,0,9000000000000000110)");
    const auto orphan_before = query_rows(
        db, "SELECT id,obj_uid,vnum FROM player_items WHERE pid=" + std::to_string(orphan_pid));
    const auto orphan_result = player_snapshot_repository_apply(
        db, make_snapshot(orphan_pid, 2, {}));
    all_passed &= expect(
        orphan_result.outcome == player_save_apply_outcome::terminal_failure &&
            orphan_result.error_code == PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH &&
            orphan_result.custody_diagnosis == player_save_custody_diagnosis::orphaned_saved_item &&
            orphan_result.custody_witness.item_uid == 9000000000000000110ULL &&
            orphan_result.custody_witness.expected_present &&
            !orphan_result.custody_witness.observed_present &&
            query_rows(db, "SELECT id,obj_uid,vnum FROM player_items WHERE pid=" +
                               std::to_string(orphan_pid)) == orphan_before &&
            scalar(db, "SELECT save_revision FROM player_data WHERE pid=" +
                           std::to_string(orphan_pid)) == 1,
        "orphaned player item was deleted by a full save");

    exec_sql(db, "INSERT INTO player_pets(owner_pid,mob_vnum,pet_uid) VALUES(" +
                     std::to_string(orphan_pet_pid) + ",23,9000000000000000120)");
    const uint64_t orphan_pet_id = scalar(
        db, "SELECT id FROM player_pets WHERE owner_pid=" + std::to_string(orphan_pet_pid));
    exec_sql(db, "INSERT INTO player_pet_items(pet_id,vnum,obj_uid) VALUES(" +
                     std::to_string(orphan_pet_id) + ",24,9000000000000000121)");
    player_snapshot orphan_pet_snapshot = make_snapshot(orphan_pet_pid, 2, {});
    orphan_pet_snapshot.components = PLAYER_COMPONENT_PETS;
    const auto orphan_pet_result = player_snapshot_repository_apply(db, orphan_pet_snapshot);
    all_passed &= expect(
        orphan_pet_result.outcome == player_save_apply_outcome::terminal_failure &&
            orphan_pet_result.error_code == PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH &&
            orphan_pet_result.custody_diagnosis ==
                player_save_custody_diagnosis::orphaned_saved_pet_item &&
            orphan_pet_result.custody_witness.item_uid == 9000000000000000121ULL &&
            orphan_pet_result.custody_witness.source_line > 0 &&
            scalar(db, "SELECT COUNT(*) FROM player_pet_items WHERE pet_id=" +
                           std::to_string(orphan_pet_id)) == 1 &&
            scalar(db, "SELECT COUNT(*) FROM player_pets WHERE id=" +
                           std::to_string(orphan_pet_id)) == 1,
        "orphaned pet item was deleted by pet replacement");

    const std::string cycle_snapshot_dir = journal_root + "/invalid-owner-cycle";
    player_snapshot cycle_snapshot =
        make_snapshot(lag_pid, 3, {{lag_root, 15}, {lag_child, 16}});
    player_save_journal_shutdown();
    open_journal(cycle_snapshot_dir);
    if (player_save_journal_append(cycle_snapshot) != player_save_journal_result::ok)
    {
        std::cerr << "could not append invalid-topology player save frame\n";
        return 2;
    }
    const auto cycle_original = journal_bytes(cycle_snapshot_dir + "/player-save.journal");
    player_save_journal_shutdown();
    exec_sql(db, "DELETE FROM player_pet_items WHERE pet_id=" + std::to_string(orphan_pet_id));
    exec_sql(db, "DELETE FROM player_pets WHERE id=" + std::to_string(orphan_pet_id));
    exec_sql(db, "DELETE FROM player_items WHERE pid=" + std::to_string(orphan_pid));
    exec_sql(db, "UPDATE item_current_owner SET parent_item_uid=" +
                     std::to_string(lag_child) + ",item_revision=3 WHERE item_uid=" +
                     std::to_string(lag_root));
    exec_sql(db, "UPDATE item_current_owner SET parent_item_uid=" +
                     std::to_string(lag_root) + ",item_revision=3 WHERE item_uid=" +
                     std::to_string(lag_child));
    const auto cycle_owner_before = query_rows(db, custody_before_replay);
    const auto cycle_projection_before = query_rows(
        db, "SELECT obj_uid,vnum,COALESCE(container_id,0) FROM player_items WHERE pid=" +
                std::to_string(lag_pid) + " ORDER BY obj_uid");
    open_journal(cycle_snapshot_dir);
    const auto cycle_result = player_snapshot_repository_apply(db, cycle_snapshot);
    all_passed &= expect(
        cycle_result.outcome == player_save_apply_outcome::terminal_failure &&
            cycle_result.custody_diagnosis == player_save_custody_diagnosis::invalid_custody_topology &&
            cycle_result.custody_witness.item_uid == lag_root &&
            cycle_result.custody_witness.expected_present &&
            cycle_result.custody_witness.observed_present &&
            cycle_result.custody_witness.observed_parent == lag_child &&
            cycle_result.custody_witness.observed_item_revision == 3,
        "invalid cycle failure did not retain original snapshot/native topology evidence");
    player_save_journal_shutdown();
    mysql_close(db);
    db = connect_db();
    const bool cycle_blocked = replay_case(db, cycle_snapshot_dir, "invalid_owner_cycle",
                                           player_save_journal_result::ok);
    const auto cycle_health = player_save_journal_health_copy();
    const uint64_t cycle_revision =
        scalar(db, "SELECT save_revision FROM player_data WHERE pid=" + std::to_string(lag_pid));
    const auto cycle_owner_after = query_rows(db, custody_before_replay);
    const auto cycle_projection_after = query_rows(
        db, "SELECT obj_uid,vnum,COALESCE(container_id,0) FROM player_items WHERE pid=" +
                std::to_string(lag_pid) + " ORDER BY obj_uid");
    all_passed &= expect(cycle_blocked && exact_quarantine(lag_pid, cycle_snapshot_dir, cycle_original),
                         "invalid authoritative cycle was not archived and fenced");
    all_passed &= expect(cycle_revision == 2 && cycle_owner_after == cycle_owner_before &&
                             cycle_projection_after == cycle_projection_before,
                         "cycle refusal changed custody, projection, or save revision");
    std::cout << "CASE invalid_owner_cycle replay="
              << describe(cycle_blocked ? player_save_journal_result::ok
                                        : player_save_journal_result::ok)
              << " pending_frames=" << cycle_health.records
              << " unchanged="
              << (cycle_revision == 2 && cycle_owner_after == cycle_owner_before &&
                          cycle_projection_after == cycle_projection_before
                      ? "yes"
                      : "no")
              << std::endl;
    player_save_journal_shutdown();
    exec_sql(db, "UPDATE item_current_owner SET root_item_uid=" + std::to_string(lag_root) +
                     ",parent_item_uid=NULL,item_revision=4 WHERE item_uid=" +
                     std::to_string(lag_root));
    exec_sql(db, "UPDATE item_current_owner SET root_item_uid=" + std::to_string(lag_root) +
                     ",parent_item_uid=" + std::to_string(lag_root) +
                     ",item_revision=4 WHERE item_uid=" + std::to_string(lag_child));
    mysql_close(db);
    db = connect_db();

    // The conflict case is a stale source-player frame after custody has moved
    // to a different player. Reject it without deleting the old projection,
    // rewriting custody, or acknowledging the journal record.
    const std::string foreign_directory = journal_root + "/foreign-owner-conflict";
    player_snapshot foreign_snapshot = make_snapshot(conflict_pid, 2, {{foreign_item, 17}});
    open_journal(foreign_directory);
    if (player_save_journal_append(foreign_snapshot) != player_save_journal_result::ok)
    {
        std::cerr << "could not append conflicting synthetic player save frame\n";
        return 2;
    }
    const auto foreign_original = journal_bytes(foreign_directory + "/player-save.journal");
    player_save_journal_shutdown();
    const std::string foreign_owner_query =
        "SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,"
        "owner_context_id,item_revision,vnum,state FROM item_current_owner WHERE item_uid=" +
        std::to_string(foreign_item);
    const std::string foreign_projection_query =
        "SELECT pid,obj_uid,vnum,COALESCE(container_id,0) FROM player_items WHERE pid=" +
        std::to_string(conflict_pid) + " AND obj_uid=" + std::to_string(foreign_item);
    const auto foreign_owner_before = query_rows(db, foreign_owner_query);
    const auto foreign_projection_before = query_rows(db, foreign_projection_query);
    mysql_close(db);
    db = connect_db();
    const bool foreign_blocked = replay_case(db, foreign_directory, "conflicting_foreign_owner",
                                             player_save_journal_result::ok);
    const uint64_t conflict_revision = scalar(
        db, "SELECT save_revision FROM player_data WHERE pid=" + std::to_string(conflict_pid));
    const auto foreign_owner_after = query_rows(db, foreign_owner_query);
    const auto foreign_projection_after = query_rows(db, foreign_projection_query);
    all_passed &= expect(foreign_blocked && exact_quarantine(conflict_pid, foreign_directory, foreign_original), "foreign owner conflict was not fail-closed");
    all_passed &= expect(conflict_revision == 1, "conflicting save advanced durable revision");
    all_passed &= expect(foreign_owner_after == foreign_owner_before,
                         "foreign owner's authoritative custody changed");
    all_passed &= expect(foreign_projection_after == foreign_projection_before,
                         "conflicting save rewrote the existing projection");
    player_save_journal_shutdown();
    mysql_close(db);
    db = connect_db();
    open_journal(foreign_directory);
    replay_context foreign_context{db};
    const auto foreign_restart = player_save_journal_replay(apply_snapshot, &foreign_context);
    const auto foreign_health = player_save_journal_health_copy();
    all_passed &= expect(foreign_restart == player_save_journal_result::ok &&
                             exact_quarantine(conflict_pid, foreign_directory, foreign_original),
                         "restart incorrectly discarded a genuine ownership conflict");
    all_passed &= expect(query_rows(db, foreign_owner_query) == foreign_owner_before &&
                             query_rows(db, foreign_projection_query) == foreign_projection_before,
                         "restart changed the foreign item or its stale projection");
    std::cout << "CASE conflicting_foreign_owner replay=" << describe(foreign_restart)
              << " pending_frames=" << foreign_health.records
              << " foreign_custody_unchanged="
              << (query_rows(db, foreign_owner_query) == foreign_owner_before ? "yes" : "no")
              << " projection_unchanged="
              << (query_rows(db, foreign_projection_query) == foreign_projection_before ? "yes" : "no")
              << '\n';

    player_save_journal_shutdown();
    exec_sql(db, "DELETE FROM player_items WHERE pid IN (" + std::to_string(lag_pid) + "," +
                     std::to_string(conflict_pid) + "," +
                     std::to_string(equipment_pid) + "," +
                     std::to_string(legacy_equipment_pid) + "," +
                     std::to_string(retired_pid) + "," + std::to_string(orphan_pid) +
                     "," + std::to_string(orphan_pet_pid) + ")");
    exec_sql(db, "DELETE FROM item_current_owner WHERE item_uid=" +
                 std::to_string(retired_child));
    exec_sql(db, "DELETE FROM item_current_owner WHERE item_uid=" +
                 std::to_string(retired_root));
    exec_sql(db, "DELETE FROM item_current_owner WHERE item_uid=" + std::to_string(lag_child));
    exec_sql(db, "DELETE FROM item_current_owner WHERE item_uid IN (" + std::to_string(lag_root) +
                     "," + std::to_string(foreign_item) + "," +
                     std::to_string(equipment_item) + "," +
                     std::to_string(legacy_equipment_item) + ")");
    exec_sql(db, "DELETE FROM item_owner_revision WHERE owner_type=1 AND owner_id IN (" +
                     std::to_string(equipment_pid) + "," +
                     std::to_string(legacy_equipment_pid) + "," +
                     std::to_string(retired_pid) + ")");
    exec_sql(db, "DELETE FROM player_data WHERE pid IN (" + std::to_string(lag_pid) + "," +
                     std::to_string(conflict_pid) + "," + std::to_string(foreign_pid) +
                     "," + std::to_string(equipment_pid) + "," +
                     std::to_string(legacy_equipment_pid) + "," +
                     std::to_string(retired_pid) + ")");
    mysql_close(db);
    if (!all_passed)
    {
        std::cerr << "RED: save journal could not recover a same-owner topology lag safely\n";
        return 1;
    }
    std::cout << "[PASS] item save journal reconciliation, foreign-owner refusal, reconnect and restart\n";
    return 0;
}
'''


def main() -> None:
    if os.environ.get("DB_HOST") != "127.0.0.1":
        raise SystemExit("refusing non-loopback DB target")
    database = os.environ.get("DB_NAME", "")
    if not (database.startswith("playtime_test_") or
            (database.startswith("economic_schema_test_") and
             os.environ.get("TEST_DB_DISPOSABLE") == "1")):
        raise SystemExit("refusing DB schema outside disposable test journey")
    with tempfile.TemporaryDirectory(prefix="player-item-reconcile-") as temporary:
        source = Path(temporary) / "item_reconcile.cpp"
        binary = Path(temporary) / "item_reconcile"
        journals = Path(temporary) / "journals"
        journals.mkdir(mode=0o700)
        source.write_text(HARNESS)
        subprocess.run(
            [
                "g++", "-std=c++20", "-ffunction-sections", "-fdata-sections", "-Isrc",
                "-I/usr/include/mysql", str(source), "src/player/player_snapshot_repository.c",
                "src/player/player_snapshot_codec.c", "src/player/player_save_journal.c",
                "src/player/player_load_repository.c", "src/player/player_load_topology.c",
                "src/player/player_death_recovery_query.c",
                "src/player/player_death_conflict_repository.c",
                "src/persistence/critical_command.c",
                "src/persistence/player_death_restitution_command.c",
                "src/persistence/quest_reward_obligation_repository.c",
                "src/item/item_transfer_command.c", "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c", "src/economy/currency_command.c",
                "src/sql/item_extra_descr_codec.c", "src/persistence/persistence_observability.c",
                "-Wl,--gc-sections", "-lmysqlclient", "-lcrypto", "-pthread", "-o", str(binary),
            ],
            cwd=ROOT,
            check=True,
        )
        subprocess.run([str(binary), str(journals)], cwd=ROOT, check=True)


if __name__ == "__main__":
    main()
