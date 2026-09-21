#!/usr/bin/env python3
"""Issue #259: replay real SQL status snapshots against a connection-private table.

Requires the isolated journey DB environment. Called by test_mysql_playtime_journey;
creates only a TEMPORARY player_data table shadowing the fixture schema's table.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "player/player_snapshot_repository.h"
#include "player/player_playtime.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

// The exercised snapshots have no extra descriptions. Supply the codec's
// normal ownership contract without pulling the monolithic game SQL facade
// into this focused repository harness.
char *sql_escape_string(const char *text) {
    const size_t length = std::strlen(text);
    char *copy = static_cast<char *>(std::malloc(length + 1));
    if (copy) std::memcpy(copy, text, length + 1);
    return copy;
}

int main() {
    assert(std::string(std::getenv("DB_HOST")) == "127.0.0.1");
    const char *port_text = std::getenv("DB_PORT");
    const unsigned int port = port_text ? std::atoi(port_text) : 3306;
    MYSQL *db = mysql_init(nullptr);
    assert(mysql_real_connect(db, "127.0.0.1", std::getenv("DB_USER"), std::getenv("DB_PASSWD"),
                              std::getenv("DB_NAME"), port, nullptr, 0));
    auto sql = [&](const char *text) {
        if (mysql_query(db, text)) { std::cerr << mysql_error(db) << '\n'; std::abort(); }
    };
    sql("CREATE TEMPORARY TABLE playtime_shape LIKE player_data");
    sql("CREATE TEMPORARY TABLE player_data LIKE playtime_shape");
    sql("INSERT INTO player_data(pid,name,played_time,save_revision) VALUES(1,'Playtimefixture',3600,1)");
    auto total = [&]() {
        sql("SELECT played_time FROM player_data WHERE pid=1");
        MYSQL_RES *result = mysql_store_result(db);
        assert(result);
        MYSQL_ROW row = mysql_fetch_row(result);
        assert(row && row[0]);
        int value = std::atoi(row[0]);
        mysql_free_result(result);
        return value;
    };
    player_snapshot snapshot{};
    snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    snapshot.pid = 1;
    snapshot.revision = 2;
    snapshot.components = PLAYER_COMPONENT_STATUS;
    snapshot.status_integers.push_back({player_status_field::played_time,
                                       player_playtime_total(3600,10000,10600),0,false});
    const auto status_applied = player_snapshot_repository_apply(db, snapshot);
    if (status_applied.outcome != player_save_apply_outcome::applied)
        std::cerr << "status apply failed: outcome=" << static_cast<int>(status_applied.outcome)
                  << " error=" << status_applied.error_code << '\n';
    assert(status_applied.outcome == player_save_apply_outcome::applied);
    assert(total() == 4200);
    assert(player_snapshot_repository_apply(db, snapshot).outcome == player_save_apply_outcome::already_applied);
    assert(total() == 4200);
    snapshot.revision = 1;
    snapshot.status_integers[0].signed_value = 3600;
    assert(player_snapshot_repository_apply(db, snapshot).outcome == player_save_apply_outcome::stale_revision);
    assert(total() == 4200);

    // A complete item replacement must prove exact equivalence with active
    // custody before deleting the prior payload projection.
    sql("CREATE TEMPORARY TABLE item_payload_shape LIKE player_items");
    sql("CREATE TEMPORARY TABLE player_items LIKE item_payload_shape");
    sql("CREATE TEMPORARY TABLE custody_shape LIKE item_current_owner");
    sql("CREATE TEMPORARY TABLE item_current_owner LIKE custody_shape");
    sql("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
        "owner_type,owner_id,owner_context_id,item_revision,vnum,state) "
        "VALUES(7001,7001,NULL,1,1,0,1,15,1),"
        "(7002,7001,7001,1,1,0,1,16,1)");
    player_snapshot items{};
    items.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    items.pid = 1;
    items.revision = 3;
    items.components = PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY;
    items.items.push_back({});
    items.items[0].parent_index = PLAYER_SNAPSHOT_NO_PARENT;
    items.items[0].object_uid = 7001;
    items.items[0].vnum = 15;
    items.items.push_back({});
    items.items[1].parent_index = 0;
    items.items[1].object_uid = 7002;
    items.items[1].vnum = 16;
    assert(player_snapshot_repository_apply(db, items).outcome ==
           player_save_apply_outcome::applied);
    sql("SELECT COUNT(*) FROM player_items WHERE pid=1 AND obj_uid IN (7001,7002)");
    MYSQL_RES *payload_rows = mysql_store_result(db);
    assert(payload_rows);
    MYSQL_ROW payload_count = mysql_fetch_row(payload_rows);
    assert(payload_count && std::atoi(payload_count[0]) == 2);
    mysql_free_result(payload_rows);

    items.revision = 4;
    items.items.resize(1);
    const auto rejected = player_snapshot_repository_apply(db, items);
    assert(rejected.outcome == player_save_apply_outcome::terminal_failure);
    assert(rejected.error_code == PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH);
    assert(total() == 4200);
    sql("SELECT save_revision,COUNT(*) FROM player_data JOIN player_items USING(pid) "
        "WHERE pid=1 AND obj_uid IN (7001,7002) GROUP BY save_revision");
    payload_rows = mysql_store_result(db);
    assert(payload_rows);
    payload_count = mysql_fetch_row(payload_rows);
    assert(payload_count && std::atoi(payload_count[0]) == 3 &&
           std::atoi(payload_count[1]) == 2);
    mysql_free_result(payload_rows);

    // Inline coin custody carries its own authoritative payload and is the only
    // active-custody row allowed to omit player_items.
    sql("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,"
        "owner_type,owner_id,owner_context_id,item_revision,vnum,state,coin_payload) "
        "VALUES(7003,7003,NULL,1,1,0,1,1,1,X'00')");
    items.items.push_back({});
    items.items[1].parent_index = 0;
    items.items[1].object_uid = 7002;
    items.items[1].vnum = 16;
    assert(player_snapshot_repository_apply(db, items).outcome ==
           player_save_apply_outcome::applied);
    sql("SELECT COUNT(*) FROM player_items WHERE pid=1 AND obj_uid IN (7001,7002,7003)");
    payload_rows = mysql_store_result(db);
    assert(payload_rows);
    payload_count = mysql_fetch_row(payload_rows);
    assert(payload_count && std::atoi(payload_count[0]) == 2);
    mysql_free_result(payload_rows);
    mysql_close(db);
    std::cout << "[PASS] real SQL snapshot apply, rollback-safe custody guard, inline coin exception, duplicate ACK and stale revision\n";
}
'''
with tempfile.TemporaryDirectory(prefix="duris-playtime-sql-") as temporary:
    source, binary = Path(temporary) / "playtime.cpp", Path(temporary) / "playtime"
    source.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-ffunction-sections", "-fdata-sections", "-Isrc",
                    "-I/usr/include/mysql", str(source), "src/player/player_snapshot_repository.c",
                    "src/player/player_snapshot_codec.c", "src/sql/item_extra_descr_codec.c",
                    "src/persistence/persistence_observability.c",
                    "-Wl,--gc-sections", "-lmysqlclient", "-pthread", "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
