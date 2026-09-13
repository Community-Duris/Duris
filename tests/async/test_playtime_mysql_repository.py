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
#include <iostream>

int main() {
    assert(std::string(std::getenv("DB_HOST")) == "127.0.0.1");
    MYSQL *db = mysql_init(nullptr);
    assert(mysql_real_connect(db, "127.0.0.1", std::getenv("DB_USER"), std::getenv("DB_PASSWD"),
                              std::getenv("DB_NAME"), 3306, nullptr, 0));
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
    player_snapshot snapshot;
    snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    snapshot.pid = 1;
    snapshot.revision = 2;
    snapshot.components = PLAYER_COMPONENT_STATUS;
    snapshot.status_integers.push_back({player_status_field::played_time,
                                       player_playtime_total(3600,10000,10600),0,false});
    assert(player_snapshot_repository_apply(db, snapshot).outcome == player_save_apply_outcome::applied);
    assert(total() == 4200);
    assert(player_snapshot_repository_apply(db, snapshot).outcome == player_save_apply_outcome::already_applied);
    assert(total() == 4200);
    snapshot.revision = 1;
    snapshot.status_integers[0].signed_value = 3600;
    assert(player_snapshot_repository_apply(db, snapshot).outcome == player_save_apply_outcome::stale_revision);
    assert(total() == 4200);
    mysql_close(db);
    std::cout << "[PASS] real SQL snapshot apply, duplicate ACK and stale revision preserve 4200 seconds\n";
}
'''
with tempfile.TemporaryDirectory(prefix="duris-playtime-sql-") as temporary:
    source, binary = Path(temporary) / "playtime.cpp", Path(temporary) / "playtime"
    source.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-ffunction-sections", "-fdata-sections", "-Isrc",
                    "-I/usr/include/mysql", str(source), "src/player/player_snapshot_repository.c",
                    "src/player/player_snapshot_codec.c", "src/persistence/persistence_observability.c",
                    "-Wl,--gc-sections", "-lmysqlclient", "-pthread", "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
