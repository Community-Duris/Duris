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
#include "player/player_snapshot_codec.h"
#include "item/item_transfer_command.h"
#include "core/defines.h"
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
#include <new>
#include <string>
#include <vector>

// Observe only the two new partial SQL preparation windows. Allocation sweeps
// never arm during fixture setup, the full legacy writer, or unrelated callers.
enum class partial_allocation_phase { none, delete_preparation, restitution_preparation };
struct partial_allocation_fault
{
    partial_allocation_phase target = partial_allocation_phase::none;
    bool active = false, thrown = false, reached_end = false;
    size_t allocations = 0, fail_at = 0;
    bool transaction_observed = false;
};
thread_local partial_allocation_fault allocation_fault;

[[gnu::noinline]] void *operator new(std::size_t size)
{
    if (allocation_fault.active)
    {
        ++allocation_fault.allocations;
        if (allocation_fault.fail_at && allocation_fault.allocations == allocation_fault.fail_at)
        {
            allocation_fault.active = false;
            allocation_fault.thrown = true;
            throw std::bad_alloc();
        }
    }
    if (void *memory = std::malloc(size ? size : 1)) return memory;
    throw std::bad_alloc();
}
[[gnu::noinline]] void *operator new[](std::size_t size) { return ::operator new(size); }
[[gnu::noinline]] void operator delete(void *memory) noexcept { std::free(memory); }
[[gnu::noinline]] void operator delete[](void *memory) noexcept { std::free(memory); }
[[gnu::noinline]] void operator delete(void *memory, std::size_t) noexcept { std::free(memory); }
[[gnu::noinline]] void operator delete[](void *memory, std::size_t) noexcept { std::free(memory); }

extern "C" int __real_mysql_real_query(MYSQL *, const char *, unsigned long);
extern "C" int __wrap_mysql_real_query(MYSQL *db, const char *sql, unsigned long length)
{
    const auto starts = [sql, length](const char *prefix)
    {
        const size_t size = std::strlen(prefix);
        return length >= size && std::memcmp(sql, prefix, size) == 0;
    };
    const bool end =
        (allocation_fault.target == partial_allocation_phase::delete_preparation &&
         starts("DELETE FROM player_items WHERE pid=")) ||
        (allocation_fault.target == partial_allocation_phase::restitution_preparation &&
         starts("SELECT d.item_uid,ri.vnum,own.item_uid,own.owner_type,"));
    if (end)
    {
        allocation_fault.active = false;
        allocation_fault.reached_end = true;
    }
    const int result = __real_mysql_real_query(db, sql, length);
    if (!result &&
        ((allocation_fault.target == partial_allocation_phase::delete_preparation &&
          starts("SELECT child.obj_uid FROM player_items child JOIN player_items parent ON ")) ||
         (allocation_fault.target == partial_allocation_phase::restitution_preparation &&
          starts("SELECT table_name FROM information_schema.tables WHERE table_schema=DATABASE() "))))
    {
        allocation_fault.transaction_observed = (db->server_status & SERVER_STATUS_IN_TRANS) != 0;
        allocation_fault.active = true;
    }
    return result;
}

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

bool refreshed_transaction_state(MYSQL *db)
{
    // A successful native query refreshes server_status from the actual server.
    // Unlike MariaDB's session variable, this protocol status works on MySQL.
    if (scalar(db, "SELECT 1") != 1)
        std::exit(2);
    return (db->server_status & SERVER_STATUS_IN_TRANS) != 0;
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

std::string partial_hex(const uint8_t *bytes, size_t size)
{
    constexpr char digits[] = "0123456789abcdef";
    std::string encoded;
    encoded.reserve(size * 2);
    for (size_t index = 0; index < size; ++index)
    {
        encoded.push_back(digits[bytes[index] >> 4]);
        encoded.push_back(digits[bytes[index] & 15]);
    }
    return encoded;
}

struct partial_durable_state
{
    std::vector<std::vector<std::string>> players, custody, owner_revisions;
    std::vector<std::vector<std::string>> items, affects, descriptions, runtime;
    std::vector<std::vector<std::string>> room_payloads, inbox, outbox;
    std::vector<std::vector<std::string>> restitution_items, restitution_deliveries, restitution_runtime;
    bool operator==(const partial_durable_state &) const = default;
};

partial_durable_state partial_state(MYSQL *db, int pid, int control_pid,
                                    uint64_t first_uid, uint64_t last_uid,
                                    const std::string &item_filter = "1=1")
{
    const std::string pids = std::to_string(pid) + "," + std::to_string(control_pid);
    const std::string uids = " BETWEEN " + std::to_string(first_uid) + " AND " +
                             std::to_string(last_uid);
    const std::string selected = "pid IN (" + pids + ") AND (" + item_filter + ")";
    const std::string ids = "SELECT id FROM player_items WHERE " + selected;
    partial_durable_state state;
    state.players = query_rows(db, "SELECT pid,save_revision,wimpy FROM player_data WHERE pid IN (" +
                                   pids + ") ORDER BY pid");
    state.custody = query_rows(db, "SELECT item_uid,root_item_uid,parent_item_uid,owner_type,"
        "owner_id,owner_context_id,item_revision,vnum,state,equipment_slot,HEX(coin_payload) "
        "FROM item_current_owner WHERE item_uid" + uids + " ORDER BY item_uid");
    state.owner_revisions = query_rows(db, "SELECT owner_type,owner_id,owner_context_id,revision "
        "FROM item_owner_revision WHERE (owner_type=1 AND owner_id IN (" + pids + ")) OR "
        "(owner_type=3 AND owner_id=" + std::to_string(UINT64_C(700000) + pid) + ") "
        "ORDER BY owner_type,owner_id,owner_context_id");
    state.items = query_rows(db, "SELECT * FROM player_items WHERE " + selected + " ORDER BY id");
    state.affects = query_rows(db, "SELECT * FROM player_item_affects WHERE item_id IN (" +
                                   ids + ") ORDER BY id");
    state.descriptions = query_rows(db, "SELECT id,item_id,keyword,description,"
        "HEX(description_sha256) FROM player_item_extra_descr WHERE item_id IN (" +
        ids + ") ORDER BY id");
    state.runtime = query_rows(db, "SELECT item_id,HEX(payload) FROM player_item_runtime_state "
        "WHERE item_id IN (" + ids + ") ORDER BY item_id");
    state.room_payloads = query_rows(db, "SELECT item_uid,item_revision,payload_version,"
        "HEX(operation_id),season_epoch,HEX(payload) FROM sql_room_item_payload WHERE item_uid" +
        uids + " ORDER BY item_uid,item_revision");
    const std::string operations = "SELECT operation_id FROM sql_room_item_payload WHERE item_uid" + uids;
    state.inbox = query_rows(db, "SELECT HEX(operation_id),HEX(command_hash),HEX(keys_hash),"
        "command_type,schema_version,payload_version,status,result_code,failure_stage,"
        "durable_revision,HEX(result_payload),created_at,committed_at FROM critical_operation_inbox "
        "WHERE operation_id IN (" + operations + ") ORDER BY operation_id");
    state.outbox = query_rows(db, "SELECT outbox_id,HEX(operation_id),event_index,destination,"
        "event_type,payload_version,HEX(payload),status,attempt_count,next_attempt_at,created_at,"
        "delivered_at,dead_lettered_at,last_error_code FROM critical_outbox WHERE operation_id IN (" +
        operations + ") ORDER BY outbox_id");
    state.restitution_items = query_rows(db, "SELECT HEX(restitution_id),item_uid,source_root_item_uid,"
        "source_parent_item_uid,delivered_root_item_uid,delivered_parent_item_uid,source_item_revision,"
        "delivered_item_revision,vnum,artifact_vnum,disposition,classification,HEX(metadata_digest),"
        "HEX(metadata_payload),note,artifact_loss_epoch,artifact_source_timer_epoch,"
        "artifact_usable_lifetime_seconds,artifact_delivered_timer_epoch,artifact_timing_basis,"
        "artifact_compensation_reference FROM player_death_restitution_item WHERE item_uid" + uids +
        " ORDER BY item_uid");
    state.restitution_deliveries = query_rows(db, "SELECT item_uid,HEX(restitution_id),source_pid,"
        "death_revision,recipient_pid,source_item_revision,delivered_item_revision,delivered_item_id,"
        "HEX(metadata_digest),HEX(original_payload),delivered_at FROM player_death_restitution_delivery "
        "WHERE item_uid" + uids + " ORDER BY item_uid");
    state.restitution_runtime = query_rows(db, "SELECT item_uid,recipient_pid,HEX(state_payload),"
        "HEX(state_digest),updated_at FROM player_death_restitution_runtime WHERE item_uid" + uids +
        " ORDER BY item_uid");
    return state;
}

player_snapshot partial_seed_forest(MYSQL *db, int pid, uint64_t first_uid)
{
    seed_player(db, pid, "PartialItemRecon" + std::to_string(pid));
    auto snapshot = make_snapshot(pid, 2, {});
    for (size_t index = 0; index < 5; ++index)
    {
        player_item_snapshot item{};
        item.object_uid = first_uid + index;
        item.vnum = 30 + static_cast<int32_t>(index);
        item.parent_index = index == 0 || index == 3 ? PLAYER_SNAPSHOT_NO_PARENT :
                            static_cast<int32_t>(index - 1);
        item.equipment_slot = index == 0 ? 5 : 0;
        item.generated_key = 100 + static_cast<int64_t>(index);
        item.type = ITEM_CONTAINER;
        item.string_mask = 15;
        item.name = "partial fixture keys";
        item.short_description = "partial fixture description";
        item.description = "complete partial fixture room text";
        item.action_description = "complete partial fixture action text";
        item.weight = 9 + static_cast<int32_t>(index);
        item.cost = 40 + static_cast<int32_t>(index);
        item.condition = 83;
        item.craftsmanship = 11;
        item.values[0] = 100;
        item.timers = {101, 102, 103, 104, 105, 106};
        item.affects[0] = {8, 7};
        item.dynamic_affects.push_back({17, 4, 2});
        item.extra_descriptions.push_back({"fixture", "preserved extra description", false, {}});
        snapshot.items.push_back(item);
        const uint64_t root_uid = first_uid + (index < 3 ? 0 : 3);
        const std::string parent = item.parent_index < 0 ? "NULL" :
                                  std::to_string(first_uid + item.parent_index);
        exec_sql(db, "INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
            "owner_type,owner_id,owner_context_id,item_revision,vnum,state,equipment_slot) VALUES(" +
            std::to_string(item.object_uid) + "," + std::to_string(root_uid) + "," + parent +
            ",1," + std::to_string(pid) + ",0,1," + std::to_string(item.vnum) + ",1," +
            std::to_string(item.equipment_slot) + ")");
    }
    exec_sql(db, "INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
                 "VALUES(1," + std::to_string(pid) + ",0,1)");
    const auto saved = player_snapshot_repository_apply(db, snapshot);
    if (!expect(saved.outcome == player_save_apply_outcome::applied && saved.durable_revision == 2,
                "actual repository could not seed the complete partial-save forest"))
    {
        std::cerr << "PARTIAL_SEED_FAILURE pid=" << pid << " first_uid=" << first_uid
                  << " outcome=" << static_cast<unsigned>(saved.outcome)
                  << " error=" << saved.error_code
                  << " diagnosis=" << player_save_custody_diagnosis_name(saved.custody_diagnosis)
                  << " witness_uid=" << saved.custody_witness.item_uid
                  << " durable_revision=" << saved.durable_revision << '\n';
        std::exit(2);
    }
    return snapshot;
}

player_snapshot partial_select(const player_snapshot &full, bool equipment)
{
    auto snapshot = full;
    snapshot.revision = 3;
    snapshot.components = equipment ? PLAYER_COMPONENT_EQUIPMENT : PLAYER_COMPONENT_INVENTORY;
    snapshot.items.clear();
    const size_t begin = equipment ? 0 : 3;
    const size_t end = equipment ? 3 : 5;
    for (size_t index = begin; index < end; ++index)
    {
        auto item = full.items[index];
        if (item.parent_index >= 0)
            item.parent_index -= static_cast<int32_t>(begin);
        snapshot.items.push_back(std::move(item));
    }
    return snapshot;
}

// Each trigger is restricted to the freshly seeded fixture PID and exact UID.
// Session variables retain ordering evidence after ROLLBACK; no durable fixture
// table or production fault hook is involved. DDL runs only after seed commit.
struct partial_write_fault
{
    MYSQL *&db;
    std::vector<std::string> triggers;
    uint64_t original_root_id = 0;
    unsigned int error_code = 0;
    bool child_fault = false;

    explicit partial_write_fault(MYSQL *&connection) : db(connection) {}
    partial_write_fault(const partial_write_fault &) = delete;
    partial_write_fault &operator=(const partial_write_fault &) = delete;

    void cleanup()
    {
        for (auto trigger = triggers.rbegin(); trigger != triggers.rend(); ++trigger)
            exec_sql(db, "DROP TRIGGER " + *trigger);
        triggers.clear();
    }

    ~partial_write_fault() { cleanup(); }

    void create(const std::string &name, const std::string &definition)
    {
        if (scalar(db, "SELECT COUNT(*) FROM information_schema.TRIGGERS WHERE "
                       "TRIGGER_SCHEMA=DATABASE() AND TRIGGER_NAME='" + name + "'"))
        {
            cleanup();
            std::cerr << "refusing pre-existing partial fixture trigger\n";
            std::exit(2);
        }
        if (mysql_query(db, ("CREATE TRIGGER " + name + definition).c_str()) != 0)
        {
            std::cerr << "partial fixture trigger creation failed: " << mysql_error(db) << '\n';
            cleanup();
            std::exit(2);
        }
        triggers.push_back(name);
    }

    void install(int pid, uint64_t root_uid, uint64_t child_uid, bool at_child)
    {
        child_fault = at_child;
        error_code = at_child ? 45002 : 45001;
        original_root_id = scalar(db, "SELECT id FROM player_items WHERE pid=" +
                                      std::to_string(pid) + " AND obj_uid=" + std::to_string(root_uid));
        const std::string prefix = "partial_item_fault_" + std::to_string(pid);
        const std::string owner = "NEW.pid=" + std::to_string(pid);
        const std::string root = std::to_string(root_uid);
        exec_sql(db, "SET @partial_delete_seen=0,@partial_insert_seen=0,@partial_fault_seen=0");
        create(prefix + "_delete", " AFTER DELETE ON player_items FOR EACH ROW BEGIN IF OLD.pid=" +
            std::to_string(pid) + " AND OLD.obj_uid=" + root +
            " THEN SET @partial_delete_seen=OLD.id; END IF; END");
        if (at_child)
            create(prefix + "_root", " AFTER INSERT ON player_items FOR EACH ROW BEGIN IF " +
                owner + " AND NEW.obj_uid=" + root +
                " THEN SET @partial_insert_seen=NEW.id; END IF; END");
        const std::string preceding = "COALESCE(@partial_delete_seen,0)=" +
            std::to_string(original_root_id) + (at_child ? " AND COALESCE(@partial_insert_seen,0)>0" : "");
        create(prefix + "_fail", " BEFORE INSERT ON player_items FOR EACH ROW BEGIN IF " +
            owner + " AND NEW.obj_uid=" + std::to_string(at_child ? child_uid : root_uid) +
            " THEN IF " + preceding + " THEN SET @partial_fault_seen=" +
            std::to_string(error_code) + "; SIGNAL SQLSTATE '45000' SET MYSQL_ERRNO=" +
            std::to_string(error_code) + ", MESSAGE_TEXT='scoped partial fixture write fault'; "
            "ELSE SIGNAL SQLSTATE '45000' SET MYSQL_ERRNO=45003, "
            "MESSAGE_TEXT='partial fixture write order mismatch'; END IF; END IF; END");
    }

    bool reached()
    {
        const uint64_t deleted = scalar(db, "SELECT COALESCE(@partial_delete_seen,0)");
        const uint64_t inserted = scalar(db, "SELECT COALESCE(@partial_insert_seen,0)");
        const uint64_t signalled = scalar(db, "SELECT COALESCE(@partial_fault_seen,0)");
        const bool ordered = deleted == original_root_id && signalled == error_code &&
                             (!child_fault || inserted > 0);
        std::cout << "PARTIAL_WRITE_FAULT expected_error=" << error_code
                  << " deleted_root_id=" << deleted << " inserted_root_id=" << inserted
                  << " signalled_error=" << signalled << " ordered=" << (ordered ? "yes" : "no") << '\n';
        return ordered;
    }
};

// Direct SQL after-state representation only: this does not execute a drop,
// the critical-command coordinator, an accounting admission, or a pooled owner.
// The real codec supplies complete immutable bytes whose preservation is tested.
void partial_represent_room_transfer(MYSQL *db, const player_snapshot &full)
{
    const uint64_t root_uid = full.items[3].object_uid;
    std::array<uint8_t, 16> operation{};
    operation[0] = 0xa7;
    for (size_t index = 0; index < 8; ++index)
        operation[8 + index] = static_cast<uint8_t>(root_uid >> (index * 8));
    const std::string operation_sql = "X'" + partial_hex(operation.data(), operation.size()) + "'";
    const uint64_t season = scalar(db, "SELECT season_epoch FROM season_reset_state "
                                      "WHERE state_id=1 AND reset_status='active'");
    if (!season)
        std::exit(2);
    item_transfer_result result{};
    result.root_item_uid = root_uid;
    result.item_count = 2;
    result.from_owner_revision = 2;
    result.to_owner_revision = 2;
    result.max_item_revision = 2;
    std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> encoded_result{};
    if (!item_transfer_command_encode_result(result, &encoded_result))
        std::exit(2);
    const std::string result_sql = "X'" + partial_hex(encoded_result.data(), encoded_result.size()) + "'";
    exec_sql(db, "START TRANSACTION");
    exec_sql(db, "INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
        "command_type,schema_version,payload_version,status,durable_revision,result_payload,committed_at) "
        "VALUES(" + operation_sql + ",UNHEX(SHA2('synthetic room after-state',256)),"
        "UNHEX(SHA2('synthetic room keys',256)),5,2,1,1,2," + result_sql + ",NOW(6))");
    for (size_t index = 3; index < 5; ++index)
    {
        auto item = full.items[index];
        item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
        std::vector<uint8_t> encoded;
        if (player_item_snapshot_list_encode({item}, &encoded) != player_snapshot_codec_result::ok)
            std::exit(2);
        exec_sql(db, "INSERT INTO sql_room_item_payload(item_uid,item_revision,payload_version,"
            "operation_id,season_epoch,payload) VALUES(" + std::to_string(item.object_uid) +
            ",2,1," + operation_sql + "," + std::to_string(season) + ",X'" +
            partial_hex(encoded.data(), encoded.size()) + "')");
    }
    exec_sql(db, "INSERT INTO critical_outbox(operation_id,event_index,destination,event_type,"
                 "payload_version,payload) VALUES(" + operation_sql + ",0,1,1,1," + result_sql + ")");
    exec_sql(db, "UPDATE item_current_owner SET owner_type=3,owner_id=" +
        std::to_string(UINT64_C(700000) + full.pid) + ",item_revision=2 WHERE item_uid IN (" +
        std::to_string(root_uid) + "," + std::to_string(full.items[4].object_uid) + ")");
    exec_sql(db, "UPDATE item_owner_revision SET revision=2 WHERE owner_type=1 AND owner_id=" +
                 std::to_string(full.pid) + " AND owner_context_id=0");
    exec_sql(db, "INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
                 "VALUES(3," + std::to_string(UINT64_C(700000) + full.pid) + ",0,2)");
    exec_sql(db, "DELETE FROM player_items WHERE pid=" + std::to_string(full.pid) +
                 " AND obj_uid=" + std::to_string(root_uid));
    exec_sql(db, "COMMIT");
}

bool partial_reconciliation_cases(MYSQL *&db, const std::string &journal_root, int first_pid)
{
    const char *names[] = {"valid_partial_inventory", "valid_partial_equipment",
        "partial_inventory_room_after_state", "partial_inventory_foreign_tree",
        "partial_inventory_destroyed_tree", "partial_inventory_missing_child",
        "partial_equipment_missing_descendants", "partial_inventory_cross_pid_cascade",
        "partial_equipment_cross_pid_cascade", "partial_inventory_after_delete_sql_fault",
        "partial_equipment_child_insert_sql_fault"};
    bool passed = true;
    for (size_t index = 0; index < std::size(names); ++index)
    {
        bool case_passed = true;
        const int pid = first_pid + static_cast<int>(index * 2);
        const int control_pid = pid + 1;
        const uint64_t uid = UINT64_C(9000000000000000200) + index * 20;
        if (scalar(db, "SELECT COUNT(*) FROM item_current_owner WHERE item_uid BETWEEN " +
                      std::to_string(uid) + " AND " + std::to_string(uid + 19)) ||
            scalar(db, "SELECT COUNT(*) FROM player_items WHERE obj_uid BETWEEN " +
                       std::to_string(uid) + " AND " + std::to_string(uid + 19)) ||
            scalar(db, "SELECT COUNT(*) FROM sql_room_item_payload WHERE item_uid BETWEEN " +
                       std::to_string(uid) + " AND " + std::to_string(uid + 19)))
        {
            std::cerr << "synthetic partial-save UID collision\n";
            return false;
        }
        player_save_journal_shutdown();
        const std::string directory = journal_root + "/" + names[index];
        open_journal(directory);
        const auto full = partial_seed_forest(db, pid, uid);
        partial_seed_forest(db, control_pid, uid + 8);
        auto snapshot = partial_select(full, index == 1 || index == 6 || index == 8 || index == 10);
        partial_write_fault fault(db);
        if (index == 5 || index == 6)
            snapshot.items.resize(1);
        if (index < 2)
        {
            snapshot.items[0].cost = 12345;
            snapshot.items[0].short_description = "changed selected partial root";
            snapshot.items[1].timers[5] = 777;
            const std::string untouched = "obj_uid NOT BETWEEN " +
                std::to_string(snapshot.items.front().object_uid) + " AND " +
                std::to_string(snapshot.items.back().object_uid);
            auto before = partial_state(db, pid, control_pid, uid, uid + 19, untouched);
            const auto applied = player_snapshot_repository_apply(db, snapshot);
            auto after = partial_state(db, pid, control_pid, uid, uid + 19, untouched);
            case_passed &= expect(applied.outcome == player_save_apply_outcome::applied &&
                             applied.durable_revision == 3 &&
                             scalar(db, "SELECT save_revision FROM player_data WHERE pid=" +
                                std::to_string(pid)) == 3 &&
                             scalar(db, "SELECT save_revision FROM player_data WHERE pid=" +
                                std::to_string(control_pid)) == 2 &&
                             scalar(db, "SELECT wimpy FROM player_data WHERE pid=" +
                                std::to_string(pid)) == 0 &&
                             scalar(db, "SELECT wimpy FROM player_data WHERE pid=" +
                                std::to_string(control_pid)) == 0 &&
                             scalar(db, "SELECT cost FROM player_items WHERE pid=" +
                                std::to_string(pid) + " AND obj_uid=" +
                                std::to_string(snapshot.items[0].object_uid)) == 12345,
                             "valid partial forest was not actually saved");
            for (const auto &captured : snapshot.items)
            {
                auto standalone = captured;
                standalone.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
                std::vector<uint8_t> encoded;
                const bool encoded_ok = player_item_snapshot_list_encode({standalone}, &encoded) ==
                                        player_snapshot_codec_result::ok;
                const uint64_t parent_uid = captured.parent_index < 0 ? 0 :
                    snapshot.items[captured.parent_index].object_uid;
                const auto projected = query_rows(db,
                    "SELECT COALESCE(parent.obj_uid,0),child.equip_slot,LOWER(HEX(runtime.payload)) "
                    "FROM player_items child LEFT JOIN player_items parent ON parent.id=child.container_id "
                    "LEFT JOIN player_item_runtime_state runtime ON runtime.item_id=child.id "
                    "WHERE child.pid=" + std::to_string(pid) + " AND child.obj_uid=" +
                    std::to_string(captured.object_uid));
                case_passed &= expect(encoded_ok && projected.size() == 1 && projected[0].size() == 3 &&
                                 projected[0][0] == std::to_string(parent_uid) &&
                                 projected[0][1] == std::to_string(captured.equipment_slot) &&
                                 projected[0][2] == partial_hex(encoded.data(), encoded.size()),
                                 "partial save did not write the exact selected payload and topology");
            }
            after.players = before.players;
            case_passed &= expect(after == before,
                             "valid partial save changed the opposite forest or unrelated PID");
            const auto complete_after = partial_state(db, pid, control_pid, uid, uid + 19);
            const auto equal = player_snapshot_repository_apply(db, snapshot);
            snapshot.revision = 2;
            const auto stale = player_snapshot_repository_apply(db, snapshot);
            case_passed &= expect(equal.outcome == player_save_apply_outcome::already_applied &&
                             stale.outcome == player_save_apply_outcome::stale_revision &&
                             partial_state(db, pid, control_pid, uid, uid + 19) == complete_after,
                             "equal/stale partial revisions rewrote durable state");
        }
        else
        {
            snapshot.components |= PLAYER_COMPONENT_STATUS;
            snapshot.status_integers.push_back({player_status_field::wimpy, 99, 0, false});
            if (index >= 9)
                snapshot.items[0].cost = 12345;
            if (player_save_journal_append(snapshot) != player_save_journal_result::ok)
                return false;
            // Keep the victim's exact frame separate from the later healthy one:
            // only the victim belongs in its quarantine archive.
            const auto original = journal_bytes(directory + "/player-save.journal");
            auto control = make_snapshot(control_pid, 3, {});
            control.components = PLAYER_COMPONENT_STATUS;
            control.status_integers.push_back({player_status_field::wimpy, 41, 0, false});
            if (player_save_journal_append(control) != player_save_journal_result::ok)
                return false;
            player_save_journal_shutdown();
            if (index == 2)
                partial_represent_room_transfer(db, full);
            else if (index == 3 || index == 4)
                exec_sql(db, "UPDATE item_current_owner SET owner_type=" +
                    std::string(index == 3 ? "1" : "8") + ",owner_id=" +
                    (index == 3 ? std::to_string(control_pid) : "0") +
                    ",state=" + (index == 3 ? "1" : "2") +
                    ",item_revision=2 WHERE item_uid IN (" + std::to_string(uid + 3) +
                    "," + std::to_string(uid + 4) + ")");
            else if (index == 7 || index == 8)
                exec_sql(db, "UPDATE player_items SET container_id=(SELECT id FROM "
                    "(SELECT id FROM player_items WHERE obj_uid=" +
                    std::to_string(uid + (index == 7 ? 3 : 0)) +
                    ") selected_parent) WHERE pid=" + std::to_string(control_pid) +
                    " AND obj_uid=" + std::to_string(uid + 11));
            else if (index >= 9)
                fault.install(pid, snapshot.items[0].object_uid, snapshot.items[1].object_uid, index == 10);
            const auto before = partial_state(db, pid, control_pid, uid, uid + 19);
            auto after_control = before;
            const auto control_row = std::find_if(after_control.players.begin(), after_control.players.end(),
                [control_pid](const auto &row) { return row[0] == std::to_string(control_pid); });
            if (!expect(control_row != after_control.players.end() && control_row->size() == 3 &&
                        (*control_row)[1] == "2" && (*control_row)[2] == "0" &&
                        scalar(db, "SELECT wimpy FROM player_data WHERE pid=" + std::to_string(pid)) == 0,
                        "partial rollback/progress status precondition was not seeded"))
                std::exit(2);
            (*control_row)[1] = "3";
            (*control_row)[2] = "41";
            open_journal(directory);
            const auto refused = player_snapshot_repository_apply(db, snapshot);
            std::cout << "CASE " << names[index]
                      << " direct_outcome=" << static_cast<unsigned>(refused.outcome)
                      << " error=" << refused.error_code
                      << " diagnosis=" << player_save_custody_diagnosis_name(refused.custody_diagnosis)
                      << " witness_uid=" << refused.custody_witness.item_uid << '\n';
            const auto expected_diagnosis = index <= 4 ?
                player_save_custody_diagnosis::snapshot_item_absent_from_custody : index <= 6 ?
                player_save_custody_diagnosis::active_custody_absent_from_snapshot :
                player_save_custody_diagnosis::invalid_custody_topology;
            const uint64_t missing_uid = uid + (index == 5 ? 4 : index == 6 ? 1 : 3);
            // A closure conflict may name the foreign child or its selected
            // parent. Both are concrete edges of this deliberately corrupt FK.
            const uint64_t conflicting_parent = uid + (index == 7 ? 3 : 0);
            const bool custody_failure = index < 9 &&
                refused.error_code == PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH &&
                refused.custody_diagnosis == expected_diagnosis &&
                (index < 7 ? refused.custody_witness.item_uid == missing_uid :
                 (refused.custody_witness.item_uid == uid + 11 ||
                  refused.custody_witness.item_uid == conflicting_parent)) &&
                refused.custody_witness.source_line > 0;
            const bool injected_failure = index >= 9 && refused.error_code == fault.error_code &&
                                          fault.reached();
            case_passed &= expect(refused.outcome == player_save_apply_outcome::terminal_failure &&
                             (custody_failure || injected_failure) &&
                             partial_state(db, pid, control_pid, uid, uid + 19) == before,
                             "partial refusal bypassed its precise failure or status/payload rollback");
            player_save_journal_shutdown();
            mysql_close(db);
            db = connect_db();
            case_passed &= expect(replay_case(db, directory, names[index], player_save_journal_result::ok) &&
                             exact_quarantine(pid, directory, original) &&
                             !player_save_journal_pid_quarantined(control_pid) &&
                             (index < 9 || fault.reached()) &&
                             partial_state(db, pid, control_pid, uid, uid + 19) == after_control,
                             "partial replay lost victim evidence, rollback, or healthy PID progress");
            player_save_journal_shutdown();
            mysql_close(db);
            db = connect_db();
            open_journal(directory);
            replay_context context{db};
            case_passed &= expect(player_save_journal_replay(apply_snapshot, &context) ==
                                 player_save_journal_result::ok &&
                             exact_quarantine(pid, directory, original) &&
                             !player_save_journal_pid_quarantined(control_pid) &&
                             partial_state(db, pid, control_pid, uid, uid + 19) == after_control,
                             "partial quarantine, healthy PID progress, or payloads changed after restart");
        }
        passed &= case_passed;
        std::cout << "CASE " << names[index] << " result=" << (case_passed ? "pass" : "fail")
                  << " source=actual_repository_partial_save"
                  << (index == 2 ? " transfer=direct_sql_after_state_representation" : "") << '\n';
        player_save_journal_shutdown();
        fault.cleanup();
        const std::string uids = " BETWEEN " + std::to_string(uid) + " AND " + std::to_string(uid + 19);
        exec_sql(db, "DELETE FROM critical_outbox WHERE operation_id IN (SELECT operation_id "
                     "FROM sql_room_item_payload WHERE item_uid" + uids + ")");
        const auto operations = query_rows(db, "SELECT DISTINCT HEX(operation_id) FROM "
                                              "sql_room_item_payload WHERE item_uid" + uids);
        exec_sql(db, "DELETE FROM sql_room_item_payload WHERE item_uid" + uids);
        for (const auto &operation : operations)
            exec_sql(db, "DELETE FROM critical_operation_inbox WHERE operation_id=X'" + operation[0] + "'");
        exec_sql(db, "DELETE FROM player_items WHERE pid IN (" + std::to_string(pid) + "," +
                     std::to_string(control_pid) + ")");
        exec_sql(db, "DELETE FROM item_current_owner WHERE item_uid" + uids +
                     " AND parent_item_uid IS NOT NULL ORDER BY item_uid DESC");
        exec_sql(db, "DELETE FROM item_current_owner WHERE item_uid" + uids);
        exec_sql(db, "DELETE FROM item_owner_revision WHERE (owner_type=1 AND owner_id IN (" +
                     std::to_string(pid) + "," + std::to_string(control_pid) + ")) OR "
                     "(owner_type=3 AND owner_id=" + std::to_string(UINT64_C(700000) + pid) + ")");
        exec_sql(db, "DELETE FROM player_data WHERE pid IN (" + std::to_string(pid) + "," +
                     std::to_string(control_pid) + ")");
    }
    return passed;
}

// Additional repository contracts; synthetic receipts exercise the actual
// runtime companion writer, not operator authorization or restitution delivery.
void partial_seed_restitution(MYSQL *db, int pid, player_item_snapshot item)
{
    const std::string uid = std::to_string(item.object_uid);
    const std::string receipt = "UNHEX(LPAD(HEX(" + uid + "),32,'0'))";
    item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
    item.equipment_slot = -1;
    std::vector<uint8_t> payload;
    if (player_item_snapshot_list_encode({item}, &payload) != player_snapshot_codec_result::ok)
        std::exit(2);
    const std::string bytes = "X'" + partial_hex(payload.data(), payload.size()) + "'";
    const std::string digest = "UNHEX(SHA2(" + bytes + ",256))";
    exec_sql(db, "INSERT INTO player_death_restitution_receipt(restitution_id,source_pid,death_revision,"
        "recipient_pid,death_operation_id,evidence_digest,plan_digest,actor,reason,candidate_count,"
        "delivered_count) VALUES(" + receipt + "," + std::to_string(pid) + ",1," + std::to_string(pid) +
        "," + receipt + "," + digest + "," + digest + ",'fixture','partial opposite preservation',1,1)");
    exec_sql(db, "INSERT INTO player_death_restitution_item(restitution_id,item_uid,vnum,disposition,"
        "classification,metadata_digest,metadata_payload) VALUES(" + receipt + "," + uid + "," +
        std::to_string(item.vnum) + ",1,'fixture'," + digest + "," + bytes + ")");
    exec_sql(db, "INSERT INTO player_death_restitution_delivery(item_uid,restitution_id,source_pid,"
        "death_revision,recipient_pid,source_item_revision,delivered_item_revision,delivered_item_id,"
        "metadata_digest,original_payload) SELECT " + uid + "," + receipt + "," + std::to_string(pid) +
        ",1," + std::to_string(pid) + ",1,1,id," + digest + "," + bytes +
        " FROM player_items WHERE pid=" + std::to_string(pid) + " AND obj_uid=" + uid);
    exec_sql(db, "INSERT INTO player_death_restitution_runtime(item_uid,recipient_pid,state_payload,"
        "state_digest) VALUES(" + uid + "," + std::to_string(pid) + "," + bytes + "," + digest + ")");
}

bool partial_additional_cases(MYSQL *db, const std::string &journal_root, int first_pid)
{
    const char *names[] = {"legacy_equipment_capture", "legacy_equipment_position_change",
        "legacy_inventory_to_equipment", "opposite_restitution_runtime", "independent_inline_coin",
        "captured_missing_native_leaf", "nested_physical_delete_boundary", "ambiguous_omitted_root",
        "selected_to_unselected_cascade", "custody_cycle", "native_cycle", "custody_depth_33",
        "native_depth_33", "custody_rows_4097", "native_rows_4097", "duplicate_native_uid",
        "captured_vnum_mismatch", "captured_rows_4097", "duplicate_captured_uid", "captured_depth_33",
        "omitted_legacy_root_only_nested_physical_position"};
    bool passed = true;
    for (size_t index = 0; index < std::size(names); ++index)
    {
        player_save_journal_shutdown();
        open_journal(journal_root + "/additional_" + names[index]);
        const int pid = first_pid + static_cast<int>(index * 2), control_pid = pid + 1;
        const uint64_t uid = UINT64_C(9000000000100000000) + index * 10000;
        const std::string pids = std::to_string(pid) + "," + std::to_string(control_pid);
        const std::string range = " BETWEEN " + std::to_string(uid) + " AND " + std::to_string(uid + 9999);
        if (scalar(db, "SELECT COUNT(*) FROM item_current_owner WHERE item_uid" + range) ||
            scalar(db, "SELECT COUNT(*) FROM player_items WHERE obj_uid" + range) ||
            scalar(db, "SELECT COUNT(*) FROM sql_room_item_payload WHERE item_uid" + range))
            std::exit(2);
        const auto full = partial_seed_forest(db, pid, uid);
        partial_seed_forest(db, control_pid, uid + 5000);
        auto snapshot = partial_select(full, index < 3);
        if (index == 0 || index == 1)
        {
            exec_sql(db, "UPDATE item_current_owner SET equipment_slot=0 WHERE item_uid=" + std::to_string(uid));
            if (index == 1) snapshot.items[0].equipment_slot = 7;
        }
        else if (index == 2)
        {
            snapshot.items = full.items;
            snapshot.items[3].equipment_slot = 9;
        }
        else if (index == 3)
            partial_seed_restitution(db, pid, full.items[0]);
        else if (index == 4)
        {
            auto coin = full.items[3];
            coin.object_uid = uid + 6;
            coin.vnum = 3;
            coin.type = ITEM_MONEY;
            coin.values = {17, 2, 3, 4, 0, 0, 0, 0};
            std::vector<uint8_t> encoded;
            if (player_item_snapshot_list_encode({coin}, &encoded) != player_snapshot_codec_result::ok)
                std::exit(2);
            exec_sql(db, "INSERT INTO item_current_owner(item_uid,root_item_uid,owner_type,owner_id,"
                "owner_context_id,item_revision,vnum,state,coin_payload) VALUES(" + std::to_string(uid + 6) +
                "," + std::to_string(uid + 6) + ",1," + std::to_string(pid) + ",0,1,3,1,X'" +
                partial_hex(encoded.data(), encoded.size()) + "')");
        }
        else if (index == 5)
            exec_sql(db, "DELETE FROM player_items WHERE pid=" + std::to_string(pid) +
                         " AND obj_uid=" + std::to_string(uid + 4));
        else if (index == 6 || index == 8 || index == 20)
        {
            const uint64_t parent_uid = uid + (index == 6 ? 0 : 3);
            const uint64_t child_uid = uid + (index == 6 ? 3 : 0);
            const uint64_t parent_id = scalar(db, "SELECT id FROM player_items WHERE pid=" +
                std::to_string(pid) + " AND obj_uid=" + std::to_string(parent_uid));
            exec_sql(db, "UPDATE player_items SET equip_slot=0,container_id=" + std::to_string(parent_id) +
                         " WHERE pid=" + std::to_string(pid) + " AND obj_uid=" + std::to_string(child_uid));
            if (index == 20)
                exec_sql(db, "UPDATE item_current_owner SET equipment_slot=0 WHERE item_uid=" + std::to_string(uid));
        }
        else if (index == 7)
        {
            exec_sql(db, "UPDATE item_current_owner SET equipment_slot=0 WHERE item_uid=" + std::to_string(uid));
            exec_sql(db, "DELETE FROM player_items WHERE pid=" + std::to_string(pid) +
                         " AND obj_uid=" + std::to_string(uid));
        }
        else if (index == 9)
            exec_sql(db, "UPDATE item_current_owner SET parent_item_uid=" + std::to_string(uid + 4) +
                         " WHERE item_uid=" + std::to_string(uid + 3));
        else if (index == 10)
        {
            const uint64_t child_id = scalar(db, "SELECT id FROM player_items WHERE obj_uid=" + std::to_string(uid + 4));
            exec_sql(db, "UPDATE player_items SET container_id=" + std::to_string(child_id) +
                         " WHERE obj_uid=" + std::to_string(uid + 3));
        }
        else if (index == 11 || index == 12 || index == 13)
        {
            const size_t count = index == 13 ? PLAYER_SNAPSHOT_MAX_OBJECTS - 4 : PLAYER_SNAPSHOT_MAX_DEPTH + 1;
            for (size_t begin = 0; begin < count; begin += 256)
            {
                std::string rows;
                for (size_t offset = begin; offset < std::min(count, begin + 256); ++offset)
                {
                    const uint64_t item = uid + 100 + offset;
                    rows += (rows.empty() ? "" : ",") + std::string("(") + std::to_string(item) + "," +
                        std::to_string(index == 11 ? uid + 100 : item) + "," +
                        (index == 11 && offset ? std::to_string(item - 1) : "NULL") + ",1," +
                        std::to_string(pid) + ",0,1,30,1)";
                }
                exec_sql(db, "INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
                             "owner_type,owner_id,owner_context_id,item_revision,vnum,state) VALUES" + rows);
            }
            if (index == 12)
            {
                uint64_t parent_id = 0;
                for (size_t offset = 0; offset < count; ++offset)
                {
                    exec_sql(db, "INSERT INTO player_items(pid,vnum,obj_uid,container_id) VALUES(" +
                        std::to_string(pid) + ",30," + std::to_string(uid + 100 + offset) + "," +
                        (parent_id ? std::to_string(parent_id) : "NULL") + ")");
                    parent_id = mysql_insert_id(db);
                }
            }
        }
        else if (index == 14 || index == 15)
        {
            const size_t count = index == 14 ? PLAYER_SNAPSHOT_MAX_OBJECTS - 4 : 1;
            for (size_t begin = 0; begin < count; begin += 256)
            {
                std::string rows;
                for (size_t offset = begin; offset < std::min(count, begin + 256); ++offset)
                    rows += (rows.empty() ? "" : ",") + std::string("(") + std::to_string(pid) +
                            ",33," + std::to_string(uid + 3) + ")";
                exec_sql(db, "INSERT INTO player_items(pid,vnum,obj_uid) VALUES" + rows);
            }
        }
        else if (index == 16)
            snapshot.items[0].vnum = 99;
        else if (index == 17)
            snapshot.items.resize(PLAYER_SNAPSHOT_MAX_OBJECTS + 1, snapshot.items[0]);
        else if (index == 18)
            snapshot.items.push_back(snapshot.items[0]);
        else if (index == 19)
        {
            snapshot.items.clear();
            for (size_t offset = 0; offset <= PLAYER_SNAPSHOT_MAX_DEPTH; ++offset)
            {
                auto item = full.items[3];
                item.object_uid = uid + 100 + offset;
                item.parent_index = offset ? static_cast<int32_t>(offset - 1) : PLAYER_SNAPSHOT_NO_PARENT;
                snapshot.items.push_back(item);
            }
        }
        bool case_passed = true;
        if (index < 7)
        {
            snapshot.items[0].cost = 12345;
            std::string untouched = "obj_uid NOT IN (";
            for (size_t item = 0; item < snapshot.items.size(); ++item)
                untouched += (item ? "," : "") + std::to_string(snapshot.items[item].object_uid);
            untouched += ")";
            auto before = partial_state(db, pid, control_pid, uid, uid + 9999, untouched);
            const auto result = player_snapshot_repository_apply(db, snapshot);
            auto after = partial_state(db, pid, control_pid, uid, uid + 9999, untouched);
            case_passed &= expect(result.outcome == player_save_apply_outcome::applied && result.durable_revision == 3 &&
                                  scalar(db, "SELECT save_revision FROM player_data WHERE pid=" +
                                      std::to_string(pid)) == 3 &&
                                  scalar(db, "SELECT save_revision FROM player_data WHERE pid=" +
                                      std::to_string(control_pid)) == 2 &&
                                  scalar(db, "SELECT wimpy FROM player_data WHERE pid=" + std::to_string(pid)) == 0 &&
                                  scalar(db, "SELECT wimpy FROM player_data WHERE pid=" + std::to_string(control_pid)) == 0 &&
                                  scalar(db, "SELECT cost FROM player_items WHERE pid=" + std::to_string(pid) +
                                       " AND obj_uid=" + std::to_string(snapshot.items[0].object_uid)) == 12345,
                                  "additional valid partial forest was not saved");
            after.players = before.players;
            case_passed &= expect(after == before, "additional partial save changed opposite native/provenance bytes");
            for (const auto &item : snapshot.items)
            {
                const uint64_t parent = item.parent_index < 0 ? 0 : snapshot.items[item.parent_index].object_uid;
                auto standalone = item;
                standalone.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
                std::vector<uint8_t> encoded;
                const bool encoded_ok = player_item_snapshot_list_encode({standalone}, &encoded) ==
                                        player_snapshot_codec_result::ok;
                const auto rows = query_rows(db, "SELECT COALESCE(parent.obj_uid,0),child.equip_slot,"
                    "LOWER(HEX(runtime.payload)) FROM player_items child LEFT JOIN player_items parent ON "
                    "parent.id=child.container_id LEFT JOIN player_item_runtime_state runtime ON "
                    "runtime.item_id=child.id WHERE "
                    "child.pid=" + std::to_string(pid) + " AND child.obj_uid=" + std::to_string(item.object_uid));
                case_passed &= expect(encoded_ok && rows.size() == 1 && rows[0][0] == std::to_string(parent) &&
                                      rows[0][1] == std::to_string(item.equipment_slot) &&
                                      rows[0][2] == partial_hex(encoded.data(), encoded.size()),
                                      "legacy/missing-leaf/boundary partial projection is incorrect");
            }
            if (index == 4)
                case_passed &= expect(scalar(db, "SELECT COUNT(*) FROM player_items WHERE obj_uid=" +
                    std::to_string(uid + 6)) == 0, "partial save manufactured a native inline coin projection");
        }
        else
        {
            snapshot.components |= PLAYER_COMPONENT_STATUS;
            snapshot.status_integers.push_back({player_status_field::wimpy, 99, 0, false});
            const auto before = partial_state(db, pid, control_pid, uid, uid + 9999);
            const auto result = player_snapshot_repository_apply(db, snapshot);
            const bool limit = index == 13 || index == 14 || index == 17;
            const auto diagnosis = index == 16 ? player_save_custody_diagnosis::custody_vnum_mismatch :
                index == 18 ? player_save_custody_diagnosis::duplicate_snapshot_uid :
                index == 19 ? player_save_custody_diagnosis::invalid_snapshot_parent :
                player_save_custody_diagnosis::invalid_custody_topology;
            case_passed &= expect(result.outcome == player_save_apply_outcome::terminal_failure &&
                (limit ? result.error_code == E2BIG : result.error_code == PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH &&
                 result.custody_diagnosis == diagnosis && result.custody_witness.source_line > 0) &&
                (index != 20 || result.custody_witness.item_uid == uid) &&
                partial_state(db, pid, control_pid, uid, uid + 9999) == before,
                "additional partial boundary refusal changed status, custody, or native evidence");
            std::cout << "CASE " << names[index] << " error=" << result.error_code << " diagnosis=" <<
                player_save_custody_diagnosis_name(result.custody_diagnosis) << '\n';
        }
        passed &= case_passed;
        std::cout << "CASE " << names[index] << " result=" << (case_passed ? "pass" : "fail") <<
            " source=actual_repository_partial_boundary\n";
        exec_sql(db, "DELETE FROM player_death_restitution_runtime WHERE item_uid" + range);
        exec_sql(db, "DELETE FROM player_death_restitution_delivery WHERE item_uid" + range);
        exec_sql(db, "DELETE FROM player_death_restitution_item WHERE item_uid" + range);
        exec_sql(db, "DELETE FROM player_death_restitution_receipt WHERE source_pid IN (" + pids + ")");
        // Break deliberate native/custody cycles before FK cleanup, after proofs.
        exec_sql(db, "UPDATE player_items SET container_id=NULL WHERE pid IN (" + pids + ")");
        exec_sql(db, "DELETE FROM player_items WHERE pid IN (" + pids + ")");
        exec_sql(db, "UPDATE item_current_owner SET parent_item_uid=NULL WHERE item_uid" + range);
        exec_sql(db, "DELETE FROM item_current_owner WHERE item_uid" + range);
        exec_sql(db, "DELETE FROM item_owner_revision WHERE owner_type=1 AND owner_id IN (" + pids + ")");
        exec_sql(db, "DELETE FROM player_data WHERE pid IN (" + pids + ")");
    }
    return passed;
}

bool partial_allocation_cases(MYSQL *db, const std::string &journal_root, int first_pid)
{
    const partial_allocation_phase phases[] = {partial_allocation_phase::delete_preparation,
                                               partial_allocation_phase::restitution_preparation};
    const char *names[] = {"partial_delete_preparation_bad_alloc", "partial_restitution_preparation_bad_alloc"};
    bool passed = true;
    for (size_t phase = 0; phase < std::size(phases); ++phase)
    {
        player_save_journal_shutdown();
        open_journal(journal_root + "/allocation_" + names[phase]);
        const int pid = first_pid + static_cast<int>(phase * 2), control_pid = pid + 1;
        const uint64_t uid = UINT64_C(9000000000400000000) + phase * 10000;
        const std::string pids = std::to_string(pid) + "," + std::to_string(control_pid);
        const std::string range = " BETWEEN " + std::to_string(uid) + " AND " + std::to_string(uid + 9999);
        if (scalar(db, "SELECT COUNT(*) FROM item_current_owner WHERE item_uid" + range) ||
            scalar(db, "SELECT COUNT(*) FROM player_items WHERE obj_uid" + range) ||
            scalar(db, "SELECT COUNT(*) FROM sql_room_item_payload WHERE item_uid" + range))
            std::exit(2);
        const auto full = partial_seed_forest(db, pid, uid);
        partial_seed_forest(db, control_pid, uid + 5000);
        partial_seed_restitution(db, pid, full.items[0]);
        const bool initially_active = refreshed_transaction_state(db);
        exec_sql(db, "START TRANSACTION");
        const bool begun_active = refreshed_transaction_state(db);
        exec_sql(db, "ROLLBACK");
        const bool rolled_back_active = refreshed_transaction_state(db);
        std::cout << "TRANSACTION_ORACLE case=" << names[phase]
                  << " initial=" << initially_active << " begun=" << begun_active
                  << " rolled_back=" << rolled_back_active << '\n';
        if (!expect(!initially_active && begun_active && !rolled_back_active,
                    "native transaction status did not track actual begin and rollback"))
            std::exit(2);
        auto snapshot = partial_select(full, false);
        snapshot.components |= PLAYER_COMPONENT_STATUS;
        snapshot.status_integers.push_back({player_status_field::wimpy, 99, 0, false});
        snapshot.items[0].cost = 12345;
        allocation_fault = {phases[phase], false, false, false, 0, 0, false};
        const auto calibrated = player_snapshot_repository_apply(db, snapshot);
        const auto calibration = allocation_fault;
        allocation_fault = {};
        if (!expect(calibrated.outcome == player_save_apply_outcome::applied && calibration.reached_end &&
                    calibration.transaction_observed &&
                    calibration.allocations > 0 && calibration.allocations <= 128,
                    "partial allocation window calibration did not reach actual SQL boundary"))
            std::exit(2);
        bool case_passed = true;
        // Reset only synthetic setup between independent faults. Full repository
        // seeding occurs with the allocator disarmed; every trial snapshots its
        // actual current native IDs, bytes and revisions before applying rev3.
        for (size_t ordinal = 1; ordinal <= calibration.allocations; ++ordinal)
        {
            exec_sql(db, "UPDATE player_data SET save_revision=1,wimpy=0 WHERE pid=" + std::to_string(pid));
            const auto reseeded = player_snapshot_repository_apply(db, full);
            if (!expect(reseeded.outcome == player_save_apply_outcome::applied && reseeded.durable_revision == 2,
                        "allocation trial full repository reseed failed"))
                std::exit(2);
            const auto before = partial_state(db, pid, control_pid, uid, uid + 9999);
            const std::string status_query = "SELECT * FROM player_data WHERE pid IN (" + pids + ") ORDER BY pid";
            const auto status_before = query_rows(db, status_query);
            allocation_fault = {phases[phase], false, false, false, 0, ordinal, false};
            player_save_apply_result result{};
            bool escaped = false;
            try { result = player_snapshot_repository_apply(db, snapshot); }
            catch (const std::bad_alloc &) { escaped = true; }
            const auto fault = allocation_fault;
            allocation_fault = {};
            const bool transaction = refreshed_transaction_state(db);
            const auto status_after = query_rows(db, status_query);
            case_passed &= expect(!escaped && fault.thrown && !fault.reached_end && fault.transaction_observed &&
                result.outcome == player_save_apply_outcome::terminal_failure && result.error_code == ENOMEM &&
                result.durable_revision == 2 && transaction == 0 &&
                status_after == status_before &&
                partial_state(db, pid, control_pid, uid, uid + 9999) == before,
                "partial allocation failure escaped, retained a transaction, or changed protected rows");
            std::cout << "ALLOCATION_FAULT case=" << names[phase] << " ordinal=" << ordinal <<
                " escaped=" << (escaped ? "yes" : "no") << " error=" << result.error_code <<
                " active_at_fault=" << fault.transaction_observed << " transaction=" << transaction << '\n';
            // Cleanup only after recording the failed owner evidence on RED.
            if (transaction) exec_sql(db, "ROLLBACK");
        }
        passed &= case_passed;
        std::cout << "CASE " << names[phase] << " result=" << (case_passed ? "pass" : "fail") <<
            " fault_ordinals=" << calibration.allocations << " source=actual_repository_allocator_sql_window\n";
        exec_sql(db, "DELETE FROM player_death_restitution_runtime WHERE item_uid" + range);
        exec_sql(db, "DELETE FROM player_death_restitution_delivery WHERE item_uid" + range);
        exec_sql(db, "DELETE FROM player_death_restitution_item WHERE item_uid" + range);
        exec_sql(db, "DELETE FROM player_death_restitution_receipt WHERE source_pid IN (" + pids + ")");
        exec_sql(db, "DELETE FROM player_items WHERE pid IN (" + pids + ")");
        exec_sql(db, "UPDATE item_current_owner SET parent_item_uid=NULL WHERE item_uid" + range);
        exec_sql(db, "DELETE FROM item_current_owner WHERE item_uid" + range);
        exec_sql(db, "DELETE FROM item_owner_revision WHERE owner_type=1 AND owner_id IN (" + pids + ")");
        exec_sql(db, "DELETE FROM player_data WHERE pid IN (" + pids + ")");
    }
    return passed;
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    const std::string journal_root = argv[1];
    MYSQL *db = connect_db();
    const uint64_t maximum_pid = scalar(db, "SELECT COALESCE(MAX(pid),0) FROM player_data");
    if (maximum_pid > static_cast<uint64_t>(std::numeric_limits<int>::max()) - 90)
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

    all_passed &= partial_reconciliation_cases(db, journal_root, orphan_pet_pid + 1);
    all_passed &= partial_additional_cases(db, journal_root, orphan_pet_pid + 23);
    all_passed &= partial_allocation_cases(db, journal_root, orphan_pet_pid + 65);
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
        std::cerr << "RED: item reconciliation or partial-save custody/isolation failed\n";
        return 1;
    }
    std::cout << "[PASS] item journal reconciliation, partial-forest isolation/refusal, reconnect and restart\n";
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
                "-Wl,--gc-sections", "-Wl,--wrap=mysql_real_query", "-lmysqlclient", "-lcrypto", "-pthread", "-o", str(binary),
            ],
            cwd=ROOT,
            check=True,
        )
        subprocess.run([str(binary), str(journals)], cwd=ROOT, check=True)


if __name__ == "__main__":
    main()
