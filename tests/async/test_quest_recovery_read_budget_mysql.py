#!/usr/bin/env python3
"""Native bounded quest recovery reads against connection-private fixture tables.

Requires an explicitly disposable loopback SQL database. No permanent table is
changed. Both supported SQL engines must run this workload during qualification.
"""
import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "persistence/quest_reward_obligation_repository.h"
#include "player/player_load_repository.h"
#include "economy/currency_command.h"
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

void sql(MYSQL *db, const std::string &statement) {
    if (mysql_query(db, statement.c_str())) {
        std::cerr << "fixture query error=" << mysql_errno(db) << '\n';
        std::abort();
    }
}
uint64_t selects(MYSQL *db) {
    sql(db, "SHOW SESSION STATUS LIKE 'Com_select'");
    MYSQL_RES *rows = mysql_store_result(db);
    assert(rows);
    MYSQL_ROW row = mysql_fetch_row(rows);
    assert(row && row[1]);
    auto count = std::strtoull(row[1], nullptr, 10);
    mysql_free_result(rows);
    return count;
}
uint64_t handler_reads(MYSQL *db) {
    sql(db, "SHOW SESSION STATUS WHERE Variable_name IN "
            "('Handler_read_key','Handler_read_next','Handler_read_rnd_next')");
    MYSQL_RES *rows = mysql_store_result(db);
    assert(rows);
    uint64_t count = 0;
    while (MYSQL_ROW row = mysql_fetch_row(rows)) {
        assert(row[1]);
        count += std::strtoull(row[1], nullptr, 10);
    }
    mysql_free_result(rows);
    return count;
}
std::string hex(const std::vector<uint8_t> &bytes) {
    constexpr char digits[] = "0123456789abcdef";
    std::string out;
    for (uint8_t byte : bytes) {
        out += digits[byte >> 4]; out += digits[byte & 15];
    }
    return out;
}
std::vector<uint8_t> terms(uint64_t root, bool xp = false) {
    std::vector<uint8_t> bytes;
    auto u32 = [&](uint32_t v) { for (unsigned i=0;i<4;++i) bytes.push_back(v>>(i*8)); };
    auto u64 = [&](uint64_t v) { for (unsigned i=0;i<8;++i) bytes.push_back(v>>(i*8)); };
    u32(xp ? 5 : 1); u32(7); u32(1); u32(0); u32(77); u32(4200);
    u64(123456789); u32(1); u64(root); u32(xp ? 1 : 64);
    if (xp) {
        u32(5); u32(100); u32(0); u32(100);
        u32(1); u32(50); u32(0); u32(1); u32(50); u32(1); u32(7);
        u32(1); bytes.push_back('F'); u32(1); bytes.push_back('d');
        u32(1); u32(7); u32(0); u32(100);
    } else {
        for (unsigned i=0;i<64;++i) { u32(i % 2 ? 3 : 1); u32(i % 2 ? 1234 : 22805); }
    }
    return bytes;
}
void inbox(MYSQL *db, const std::string &op, unsigned type, const std::vector<uint8_t> &payload) {
    sql(db, "INSERT INTO critical_operation_inbox VALUES(UNHEX('"+op+"'),1,0,"+
            std::to_string(type)+",UNHEX('"+hex(payload)+"'))");
}
int main() {
    MYSQL *db = mysql_init(nullptr);
    assert(db && mysql_real_connect(db, "127.0.0.1", std::getenv("DB_USER"),
        std::getenv("DB_PASSWD"), std::getenv("DB_NAME"),
        std::strtoul(std::getenv("DB_PORT"),nullptr,10), nullptr, 0));
    sql(db, "CREATE TEMPORARY TABLE critical_operation_inbox(operation_id BINARY(16) PRIMARY KEY,"
            "status INT,result_code INT,command_type INT,result_payload BLOB)");
    sql(db, "CREATE TEMPORARY TABLE quest_reward_obligation(offering_operation_id BINARY(16) PRIMARY KEY,"
            "player_pid INT,continuation BLOB,xp_applied_mask BIGINT UNSIGNED,created_at INT,acknowledged_at INT)");
    sql(db, "CREATE TEMPORARY TABLE quest_reward_xp_entitlement(offering_operation_id BINARY(16),"
            "recipient_pid INT,reward_index INT,amount INT,applied_at INT)");
    sql(db, "CREATE TEMPORARY TABLE item_current_owner(item_uid BIGINT UNSIGNED PRIMARY KEY,vnum INT)");
    sql(db, "CREATE TEMPORARY TABLE item_ownership_ledger(operation_id BINARY(16),reason_type INT,"
            "reason_id BIGINT UNSIGNED,parent_item_uid BIGINT UNSIGNED,from_owner_type INT,from_owner_id INT,"
            "from_owner_context_id INT,to_owner_type INT,to_owner_id INT,to_owner_context_id INT,"
            "item_revision INT,item_uid BIGINT UNSIGNED,root_item_uid BIGINT UNSIGNED,"
            "from_owner_revision BIGINT,to_owner_revision BIGINT,"
            "KEY(from_owner_type,from_owner_id,from_owner_context_id),"
            "KEY(to_owner_type,to_owner_id,to_owner_context_id),"
            "KEY(operation_id),KEY idx_item_ledger_reason_source(reason_type,reason_id))");
    sql(db, "CREATE TEMPORARY TABLE currency_ledger(operation_id BINARY(16) PRIMARY KEY,pid INT,reason_type INT,"
            "reason_id INT,source_site INT,bank_delta_copper INT,bank_delta_silver INT,bank_delta_gold INT,"
            "bank_delta_platinum INT,wallet_delta_copper INT,wallet_delta_silver INT,wallet_delta_gold INT,"
            "wallet_delta_platinum INT,wallet_after_copper INT,wallet_after_silver INT,wallet_after_gold INT,"
            "wallet_after_platinum INT,bank_after_copper INT,bank_after_silver INT,bank_after_gold INT,"
            "bank_after_platinum INT,wallet_revision INT,bank_revision INT)");
    std::vector<quest_reward_obligation_record> pending;
    quest_reward_read_metrics metrics;
    unsigned error = 0;
    auto read = [&]() {
        const auto before = selects(db);
        const auto result = quest_reward_obligation_repository_pending(db,7,&pending,&error,&metrics);
        assert(selects(db) - before == metrics.query_count);
        return result;
    };
    assert(read() == quest_reward_obligation_result::ok && pending.empty());
    assert(metrics.query_count == QUEST_REWARD_PENDING_QUERY_MAX && !metrics.row_count && !metrics.byte_count);
    uint64_t continuation_bytes = 0;
    for (unsigned record=1;record<=64;++record) {
        critical_operation_id op{}; op.bytes[0] = record;
        char op_hex[33]{}; assert(critical_operation_id_to_hex(op,op_hex,sizeof(op_hex)));
        const auto encoded = terms(9000+record);
        continuation_bytes += encoded.size();
        quest_reward_continuation decoded;
        assert(quest_reward_continuation_decode(encoded.data(),encoded.size(),&decoded));
        inbox(db,op_hex,5,{});
        sql(db,"INSERT INTO quest_reward_obligation VALUES(UNHEX('"+std::string(op_hex)+"'),7,UNHEX('"+
            hex(encoded)+"'),0,"+std::to_string(record)+",NULL)");
        for (unsigned index=0;index<64;++index) {
            if (index % 2) {
                critical_operation_id child{}; char child_hex[33]{};
                assert(critical_operation_id_derive(op,QUEST_REWARD_CURRENCY_OPERATION_DOMAIN,index+1,&child));
                assert(critical_operation_id_to_hex(child,child_hex,sizeof(child_hex)));
                currency_command_result result{{{2,3,4,5}},{{0,0,0,0}},7,8};
                std::array<uint8_t,CURRENCY_RESULT_PAYLOAD_BYTES> payload{};
                assert(currency_command_encode_result(result,&payload));
                inbox(db,child_hex,3,{payload.begin(),payload.end()});
                sql(db,"INSERT INTO currency_ledger VALUES(UNHEX('"+std::string(child_hex)+"'),7,5,"+
                    std::to_string(index+1)+",5,0,0,0,0,4,3,2,1,2,3,4,5,0,0,0,0,7,8)");
            } else {
                const uint64_t uid = record*100+index;
                critical_operation_id grant{}; grant.bytes[0] = record; grant.bytes[1] = index+1;
                char grant_hex[33]{}; assert(critical_operation_id_to_hex(grant,grant_hex,sizeof(grant_hex)));
                item_transfer_result result{uid,1,8,9,1,0};
                std::array<uint8_t,ITEM_TRANSFER_RESULT_BYTES> payload{};
                assert(item_transfer_command_encode_result(result,&payload));
                inbox(db,grant_hex,5,{payload.begin(),payload.end()});
                sql(db,"INSERT INTO item_current_owner VALUES("+std::to_string(uid)+",22805)");
                sql(db,"INSERT INTO item_ownership_ledger VALUES(UNHEX('"+std::string(grant_hex)+"'),2,"+
                    std::to_string(quest_item_reward_source_id(decoded,index))+",NULL,7,0,0,1,7,0,1,"+
                    std::to_string(uid)+","+std::to_string(uid)+",8,9)");
            }
        }
    }
    // Same reason and owner as the wanted events: owner lookup alone cannot
    // avoid scanning this unrelated history. Migration 0051 supplies the exact
    // non-unique reason/source lookup while preserving duplicate detection.
    for (unsigned batch=0;batch<100;++batch) {
        std::string values;
        for (unsigned offset=1;offset<=1000;++offset) {
            if (!values.empty()) values += ',';
            const auto source = batch*1000+offset;
            values += "(UNHEX('00000000000000000000000000000000'),2,"+
                std::to_string(source)+",NULL,7,0,0,1,7,0,1,"+
                std::to_string(1000000+source)+","+
                std::to_string(1000000+source)+",8,9)";
        }
        sql(db,"INSERT INTO item_ownership_ledger VALUES"+values);
    }
    const auto reads_before = handler_reads(db);
    const auto started = std::chrono::steady_clock::now();
    assert(read() == quest_reward_obligation_result::ok && pending.size()==64);
    const auto usec = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now()-started).count();
    assert(metrics.query_count==3 && metrics.row_count==64+4096 && metrics.byte_count>continuation_bytes);
    assert(metrics.row_count <= PLAYER_SNAPSHOT_MAX_ROWS && metrics.byte_count <= PLAYER_SNAPSHOT_MAX_BYTES);
    assert(static_cast<uint64_t>(usec) < PLAYER_LOAD_TIMEOUT_USEC);
    const auto native_reads = handler_reads(db)-reads_before;
    assert(native_reads < 25000); // Refuse a full scan of 100,000 old events.
    for (const auto &record : pending) assert(record.economic_applied_mask==UINT64_MAX);
    std::cout << "maximum workload: obligations=64 slots=4096 queries=" << metrics.query_count
              << " rows=" << metrics.row_count << " bytes=" << metrics.byte_count
              << " history=100000 handler_reads=" << native_reads << " usec=" << usec << '\n';
    const auto extra = terms(10000);
    inbox(db,"41000000000000000000000000000000",5,{});
    sql(db,"INSERT INTO quest_reward_obligation VALUES(UNHEX('41000000000000000000000000000000'),7,UNHEX('"+
        hex(extra)+"'),0,65,NULL)");
    assert(read()==quest_reward_obligation_result::limit_exceeded && pending.size()==64);
    assert(metrics.query_count==1 && metrics.row_count==65);
    sql(db,"DELETE FROM quest_reward_obligation WHERE created_at=65");
    // Unpaid slots are retained, not mistaken for paid. Duplicate or conflicting
    // witnesses refuse the entire read and retain the caller's prior output.
    sql(db,"DELETE FROM currency_ledger WHERE operation_id=(SELECT operation_id FROM critical_operation_inbox WHERE command_type=3 LIMIT 1)");
    assert(read()==quest_reward_obligation_result::corrupt && pending.size()==64 && metrics.query_count==3);
    sql(db,"DELETE FROM critical_operation_inbox WHERE command_type=3");
    assert(read()==quest_reward_obligation_result::ok && metrics.query_count==3);
    for (const auto &record : pending) assert(record.economic_applied_mask==UINT64_C(0x5555555555555555));
    // MySQL cannot reopen a TEMPORARY table in INSERT ... SELECT from itself.
    sql(db,"INSERT INTO item_ownership_ledger VALUES(UNHEX('01010000000000000000000000000000'),2,"+
        std::to_string(quest_item_reward_source_id(9001,22805,0))+",NULL,7,0,0,1,7,0,1,100,100,8,9)");
    assert(read()==quest_reward_obligation_result::corrupt && pending.size()==64 && metrics.query_count==2);
    sql(db,"DELETE FROM quest_reward_obligation");
    const auto xp = terms(9999,true);
    inbox(db,"ff000000000000000000000000000000",5,{});
    sql(db,"INSERT INTO quest_reward_obligation VALUES(UNHEX('ff000000000000000000000000000000'),7,UNHEX('"+
        hex(xp)+"'),0,1,NULL)");
    sql(db,"INSERT INTO quest_reward_xp_entitlement VALUES(UNHEX('ff000000000000000000000000000000'),7,0,100,NULL)");
    std::vector<quest_reward_xp_entitlement_record> entitlements;
    auto before = selects(db);
    assert(quest_reward_xp_entitlement_repository_pending(db,7,&entitlements,&error,&metrics)==quest_reward_obligation_result::ok);
    assert(selects(db)-before==metrics.query_count && metrics.query_count==1 && metrics.row_count==1);
    assert(metrics.byte_count==xp.size()+16+4+4);
    sql(db,"UPDATE quest_reward_xp_entitlement SET amount=101");
    assert(quest_reward_xp_entitlement_repository_pending(db,7,&entitlements,&error,&metrics)==quest_reward_obligation_result::corrupt);
    assert(entitlements.size()==1 && entitlements[0].amount==100 && metrics.byte_count==xp.size()+24);
    sql(db,"DROP TEMPORARY TABLE quest_reward_xp_entitlement");
    // Keep shadowing any real migrated table while exercising failed prepare.
    sql(db,"CREATE TEMPORARY TABLE quest_reward_xp_entitlement(invalid_shape INT)");
    assert(quest_reward_xp_entitlement_repository_pending(db,7,&entitlements,&error,&metrics)==quest_reward_obligation_result::database_error);
    assert(entitlements.size()==1 && !metrics.query_count && !metrics.row_count && error);
    mysql_close(db);
    std::cout << "bounded quest recovery native metrics and refusal retention: ok\n";
}
'''


def main() -> None:
    if (os.environ.get("DB_HOST") != "127.0.0.1"
            or os.environ.get("TEST_DB_DISPOSABLE") != "1"
            or not os.environ.get("DB_NAME", "").startswith("economic_schema_test_")):
        raise SystemExit("refusing non-disposable SQL target")
    with tempfile.TemporaryDirectory(prefix="quest-recovery-budget-") as directory:
        source = Path(directory) / "probe.cpp"
        binary = Path(directory) / "probe"
        source.write_text(HARNESS)
        subprocess.run([
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Isrc",
            "-I/usr/include/mysql", str(source), "src/persistence/quest_reward_obligation_repository.c",
            "src/persistence/critical_command.c", "src/item/item_transfer_command.c", "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c",
            "src/economy/currency_command.c", "src/player/player_snapshot_codec.c",
            "-lmysqlclient", "-lcrypto", "-o", str(binary),
        ], cwd=ROOT, check=True)
        subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=60)


if __name__ == "__main__":
    main()
