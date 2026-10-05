#!/usr/bin/env python3
"""Prove spell and quest operation receipts on a disposable SQL save."""

import os
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "player/player_snapshot_repository.h"
#include "player/player_snapshot_codec.h"
#include "player/player_load_repository.h"
#include "persistence/persistence_observability.h"
#include "classes/necromancy.h"
#include "core/files.h"
#include "world/vnum.obj.h"
#include "persistence/quest_reward_obligation_repository.h"
#include "economy/currency_command.h"
#include <mysql/mysql.h>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

extern "C" void sql_pool_discard_connection(MYSQL *) {}
char *sql_escape_string(const char *text) {
    char *copy = static_cast<char *>(std::malloc(std::strlen(text) + 1));
    if (copy) std::strcpy(copy, text);
    return copy;
}

void execute(MYSQL *db, const std::string &statement) {
    if (mysql_query(db, statement.c_str())) {
        std::cerr << "disposable SQL fixture failed: " << mysql_error(db) << '\n';
        std::exit(2);
    }
    if (MYSQL_RES *rows = mysql_store_result(db)) mysql_free_result(rows);
}

uint64_t scalar(MYSQL *db, const std::string &statement) {
    if (mysql_query(db, statement.c_str())) std::exit(2);
    MYSQL_RES *rows = mysql_store_result(db);
    if (!rows || mysql_num_rows(rows) != 1) std::exit(2);
    MYSQL_ROW row = mysql_fetch_row(rows);
    if (!row || !row[0]) std::exit(2);
    const uint64_t value = std::strtoull(row[0], nullptr, 10);
    mysql_free_result(rows);
    return value;
}

std::string hex_bytes(const std::vector<uint8_t> &bytes) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string result;
    for (uint8_t byte : bytes) {
        result.push_back(digits[byte >> 4]);
        result.push_back(digits[byte & 15]);
    }
    return result;
}

std::vector<uint8_t> quest_terms(uint32_t pid, uint32_t peer = 0) {
    std::vector<uint8_t> bytes;
    auto add32 = [&](uint32_t value) {
        for (unsigned int index = 0; index < 4; ++index)
            bytes.push_back(static_cast<uint8_t>(value >> (index * 8)));
    };
    auto add64 = [&](uint64_t value) {
        for (unsigned int index = 0; index < 8; ++index)
            bytes.push_back(static_cast<uint8_t>(value >> (index * 8)));
    };
    add32(5); add32(pid); add32(1); add32(0); add32(77); add32(4200);
    add64(123456789); add32(1); add64(9001); add32(1);
    add32(5); add32(100); add32(0); add32(100);
    add32(1); add32(50); add32(0); add32(peer ? 2 : 1); add32(50); add32(peer ? 2 : 1);
    add32(pid); if (peer) add32(peer);
    add32(1); bytes.push_back('F');
    add32(1); bytes.push_back('d');
    add32(peer ? 2 : 1); add32(pid); add32(0); add32(100);
    if (peer) { add32(peer); add32(0); add32(60); }
    return bytes;
}

void verify_economic_receipts(MYSQL *db, uint32_t pid) {
    std::vector<uint8_t> terms;
    auto add32 = [&](uint32_t v) { for (unsigned i=0;i<4;++i) terms.push_back(v>>(8*i)); };
    auto add64 = [&](uint64_t v) { for (unsigned i=0;i<8;++i) terms.push_back(v>>(8*i)); };
    add32(1); add32(pid); add32(1); add32(0); add32(77); add32(4200);
    add64(123456789); add32(1); add64(9002); add32(2);
    add32(1); add32(22805); add32(3); add32(1234);
    const std::string offering = "aa000000000000000000000000000000";
    const std::string grant = "ab000000000000000000000000000000";
    const auto uid = scalar(db, "SELECT COALESCE(MAX(item_uid),0)+1 FROM item_current_owner");
    const auto source = quest_item_reward_source_id(9002, 22805, 0);
    critical_operation_id parent{}, child{};
    assert(critical_operation_id_from_hex(offering.c_str(), &parent));
    assert(critical_operation_id_derive(parent, QUEST_REWARD_CURRENCY_OPERATION_DOMAIN, 2, &child));
    char child_hex[33]{};
    assert(critical_operation_id_to_hex(child, child_hex, sizeof(child_hex)));
    const std::string cash = child_hex, player = std::to_string(pid), item = std::to_string(uid);
    auto inbox = [&](const std::string &op, unsigned type, const std::vector<uint8_t> &payload) {
        execute(db, "INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
                    "command_type,schema_version,payload_version,status,result_payload) VALUES(UNHEX('"+
                    op+"'),UNHEX(REPEAT('00',32)),UNHEX(REPEAT('00',32)),"+
                    std::to_string(type)+",1,1,1,UNHEX('"+hex_bytes(payload)+"'))");
    };
    inbox(offering, 5, {});
    execute(db, "INSERT INTO quest_reward_obligation(offering_operation_id,player_pid,continuation) "
                "VALUES(UNHEX('"+offering+"'),"+player+",UNHEX('"+hex_bytes(terms)+"'))");
    std::vector<quest_reward_obligation_record> pending;
    unsigned error=0;
    auto read = [&]() { return quest_reward_obligation_repository_pending(db,pid,&pending,&error); };
    assert(read()==quest_reward_obligation_result::ok && pending.size()==1 && !pending[0].economic_applied_mask);
    item_transfer_result native_item{uid,1,8,9,1,0};
    std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> item_result{};
    assert(item_transfer_command_encode_result(native_item, &item_result));
    inbox(grant,5,{item_result.begin(),item_result.end()});
    execute(db,"INSERT INTO item_current_owner(item_uid,root_item_uid,owner_type,owner_id,item_revision,vnum,state) "
               "VALUES("+item+","+item+",1,"+player+",1,22805,1)");
    execute(db,"INSERT INTO item_ownership_ledger(operation_id,event_index,item_uid,root_item_uid,"
               "from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,to_owner_id,to_owner_context_id,"
               "item_revision,from_owner_revision,to_owner_revision,reason_type,reason_id,source_site) "
               "VALUES(UNHEX('"+grant+"'),0,"+item+","+item+",7,0,0,1,"+player+",0,1,8,9,2,"+
               std::to_string(source)+",1)");
    assert(read()==quest_reward_obligation_result::ok && pending[0].economic_applied_mask==1);
    // A later owner or tombstone does not erase the original delivery witness.
    execute(db,"UPDATE item_current_owner SET owner_type=8,owner_id=0,state=2,item_revision=2 WHERE item_uid="+item);
    assert(read()==quest_reward_obligation_result::ok && pending[0].economic_applied_mask==1);
    execute(db,"UPDATE item_ownership_ledger SET to_owner_id="+std::to_string(pid+1)+" WHERE operation_id=UNHEX('"+grant+"')");
    assert(read()==quest_reward_obligation_result::corrupt && pending[0].economic_applied_mask==1);
    execute(db,"UPDATE item_ownership_ledger SET to_owner_id="+player+" WHERE operation_id=UNHEX('"+grant+"')");
    execute(db,"UPDATE item_current_owner SET vnum=22806 WHERE item_uid="+item);
    assert(read()==quest_reward_obligation_result::corrupt);
    execute(db,"UPDATE item_current_owner SET vnum=22805 WHERE item_uid="+item);
    execute(db,"UPDATE critical_operation_inbox SET result_payload=X'' WHERE operation_id=UNHEX('"+grant+"')");
    assert(read()==quest_reward_obligation_result::corrupt);
    execute(db,"UPDATE critical_operation_inbox SET result_payload=UNHEX('"+
               hex_bytes({item_result.begin(),item_result.end()})+"') WHERE operation_id=UNHEX('"+grant+"')");
    currency_command_result native_cash{{{2,3,4,5}},{{0,0,0,0}},7,8};
    std::array<uint8_t,CURRENCY_RESULT_PAYLOAD_BYTES> cash_result{};
    assert(currency_command_encode_result(native_cash,&cash_result));
    inbox(cash,3,{cash_result.begin(),cash_result.end()});
    // An inbox without its matching native ledger cannot prove payment.
    assert(read()==quest_reward_obligation_result::corrupt);
    execute(db,"INSERT INTO currency_ledger(operation_id,pid,bank_id,wallet_delta_copper,wallet_delta_silver,"
               "wallet_delta_gold,wallet_delta_platinum,bank_delta_copper,bank_delta_silver,bank_delta_gold,"
               "bank_delta_platinum,wallet_after_copper,wallet_after_silver,wallet_after_gold,wallet_after_platinum,"
               "bank_after_copper,bank_after_silver,bank_after_gold,bank_after_platinum,wallet_revision,bank_revision,"
               "reason_type,reason_id,source_site) VALUES(UNHEX('"+cash+"'),"+player+",1,4,3,2,1,0,0,0,0,2,3,4,5,0,0,0,0,7,8,5,2,5)");
    assert(read()==quest_reward_obligation_result::ok && pending[0].economic_applied_mask==3);
    execute(db,"UPDATE currency_ledger SET wallet_delta_copper=3 WHERE operation_id=UNHEX('"+cash+"')");
    assert(read()==quest_reward_obligation_result::corrupt && pending[0].economic_applied_mask==3);
    execute(db,"UPDATE currency_ledger SET wallet_delta_copper=4,bank_delta_gold=1 WHERE operation_id=UNHEX('"+cash+"')");
    assert(read()==quest_reward_obligation_result::corrupt);
    execute(db,"UPDATE currency_ledger SET bank_delta_gold=0,wallet_after_gold=9 WHERE operation_id=UNHEX('"+cash+"')");
    assert(read()==quest_reward_obligation_result::corrupt);
    execute(db,"DELETE FROM currency_ledger WHERE operation_id=UNHEX('"+cash+"')");
    execute(db,"DELETE FROM item_ownership_ledger WHERE operation_id=UNHEX('"+grant+"')");
    execute(db,"DELETE FROM item_current_owner WHERE item_uid="+item);
    execute(db,"DELETE FROM quest_reward_obligation WHERE offering_operation_id=UNHEX('"+offering+"')");
    execute(db,"DELETE FROM critical_operation_inbox WHERE operation_id IN (UNHEX('"+offering+"'),UNHEX('"+grant+"'),UNHEX('"+cash+"'))");
}

void verify_craft_progression(MYSQL *db, uint32_t pid) {
    const auto player = std::to_string(pid);
    const std::string operation = "ed000000000000000000000000000000";
    execute(db, "INSERT INTO player_data(pid,name,account_name,save_revision,exp) VALUES("+player+",'CraftProgressionFixture','CraftFixtureAccount',1,100)");
    execute(db, "INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,schema_version,payload_version,status,result_payload) VALUES(UNHEX('"+operation+"'),UNHEX(REPEAT('00',32)),UNHEX(REPEAT('00',32)),5,1,10,1,X'')");
    execute(db, "INSERT INTO player_craft_progression(operation_id,pid,discipline,experience) VALUES(UNHEX('"+operation+"'),"+player+",1,7000)");
    player_snapshot snapshot={}; snapshot.pid=pid; snapshot.revision=2;
    snapshot.schema_version=PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION;
    snapshot.components=PLAYER_COMPONENT_STATUS|PLAYER_COMPONENT_SKILLS|PLAYER_COMPONENT_AFFECTS|PLAYER_COMPONENT_TROPHIES;
    snapshot.encoded_size_bound=8192;
    snapshot.status_integers.push_back({player_status_field::experience,7100,0,false});
    snapshot.skills.push_back({123,51,90}); snapshot.affects.push_back({1234,40,0,0,0,0});
    snapshot.trophies.push_back({42,100});
    player_craft_receipt_snapshot receipt={}; receipt.operation_id.bytes[0]=0xed;
    receipt.discipline=1; receipt.experience=7000; snapshot.craft_receipts.push_back(receipt);
    const std::string row=" FROM player_craft_progression WHERE operation_id=UNHEX('"+operation+"')";
    execute(db,"CREATE TRIGGER craft_progression_save_failure BEFORE UPDATE ON player_craft_progression FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected recipe save failure'");
    const auto failed = player_snapshot_repository_apply(db,snapshot);
    assert(failed.outcome==player_save_apply_outcome::terminal_failure && failed.error_code==1644);
    assert(scalar(db,"SELECT exp FROM player_data WHERE pid="+player)==100);
    assert(scalar(db,"SELECT save_revision FROM player_data WHERE pid="+player)==1);
    assert(scalar(db,"SELECT applied_revision"+row)==0);
    assert(scalar(db,"SELECT COUNT(*) FROM player_skills WHERE pid="+player)==0);
    assert(scalar(db,"SELECT COUNT(*) FROM player_affects WHERE pid="+player)==0);
    execute(db,"DROP TRIGGER craft_progression_save_failure");
    const auto applied = player_snapshot_repository_apply(db,snapshot);
    if (applied.outcome != player_save_apply_outcome::applied)
        std::cerr << "recipe checkpoint outcome=" << static_cast<unsigned>(applied.outcome)
                  << " error=" << applied.error_code << " sql=" << mysql_error(db) << '\n';
    assert(applied.outcome==player_save_apply_outcome::applied);
    assert(scalar(db,"SELECT exp FROM player_data WHERE pid="+player)==7100);
    assert(scalar(db,"SELECT applied_revision"+row)==2);
    assert(player_snapshot_repository_apply(db,snapshot).outcome==player_save_apply_outcome::already_applied);
    execute(db,"INSERT INTO player_spell_effect_receipt(pid,operation_id,effect_id) VALUES("+player+",UNHEX('"+operation+"'),6)");
    player_load_request request={}; request.request_id=1; request.player_name="CraftProgressionFixture";
    request.pending_craft_operations={receipt.operation_id};
    request.pending_spell_effect_operations={receipt.operation_id};
    request.deadline_usec=persistence_observability_now_usec()+PLAYER_LOAD_TIMEOUT_USEC;
    const auto loaded=player_load_repository_execute(db,request);
    if (loaded.outcome!=player_load_outcome::applied)
        std::cerr << "recipe load outcome=" << static_cast<unsigned>(loaded.outcome)
                  << " error=" << loaded.error_code << " component=" << loaded.failed_component
                  << " queries=" << loaded.metrics.query_count << '\n';
    assert(loaded.outcome==player_load_outcome::applied && loaded.snapshot.revision==2);
    assert(loaded.craft_receipts.size()==1 && loaded.craft_receipts[0].experience==7000);
    assert(loaded.spell_effect_receipts.size()==1 && loaded.metrics.query_count==PLAYER_LOAD_QUERY_MAX);
    auto changed=snapshot; changed.craft_receipts[0].experience++;
    assert(player_snapshot_repository_apply(db,changed).outcome==player_save_apply_outcome::terminal_failure);
    execute(db,"UPDATE player_craft_progression SET applied_revision=0 WHERE operation_id=UNHEX('"+operation+"')");
    assert(player_snapshot_repository_apply(db,snapshot).outcome==player_save_apply_outcome::terminal_failure);
    execute(db,"DELETE"+row);
    execute(db,"DELETE FROM critical_operation_inbox WHERE operation_id=UNHEX('"+operation+"')");
    execute(db,"DELETE FROM player_affects WHERE pid="+player);
    execute(db,"DELETE FROM player_skills WHERE pid="+player);
    execute(db,"DELETE FROM player_spell_effect_receipt WHERE pid="+player);
    execute(db,"DELETE FROM zone_trophy WHERE pid="+player);
    execute(db,"DELETE FROM player_data WHERE pid="+player);
}

int main() {
    const char *host = std::getenv("DB_HOST");
    const char *database = std::getenv("DB_NAME");
    const char *user = std::getenv("DB_USER");
    const char *password = std::getenv("DB_PASSWD");
    if (!host || std::strcmp(host, "127.0.0.1") || !database || !user ||
        !password || (std::string(database).rfind("economic_schema_test_", 0) &&
                      std::string(database).rfind("playtime_test_", 0))) return 2;
    MYSQL *db = mysql_init(nullptr);
    const char *port_text = std::getenv("DB_PORT");
    const unsigned int port = port_text ? std::strtoul(port_text, nullptr, 10) : 3306;
    if (!db || !mysql_real_connect(db, host, user, password, database, port,
                                   nullptr, 0)) return 2;
    const int pid = static_cast<int>(scalar(db, "SELECT COALESCE(MAX(pid),0) FROM player_data WHERE pid<2147483646") + 1);
    execute(db, "INSERT INTO player_data(pid,name,save_revision) VALUES(" +
                std::to_string(pid) + ",'SpellReceiptFixture',1)");
    player_snapshot snapshot = {};
    snapshot.schema_version = PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION;
    snapshot.pid = pid;
    snapshot.revision = 2;
    snapshot.components = PLAYER_COMPONENT_AFFECTS;
    snapshot.encoded_size_bound = 1024;
    player_affect_snapshot affect = {};
    affect.type = 1234;
    affect.duration = 20;
    snapshot.affects.push_back(affect);
    player_spell_effect_receipt_snapshot receipt = {};
    receipt.operation_id.bytes[0] = 0xa5;
    receipt.effect_id = 6;
    snapshot.spell_effect_receipts.push_back(receipt);
    const std::string id = std::to_string(pid);
    const std::string receipt_row = " FROM player_spell_effect_receipt WHERE pid=" + id +
                                    " AND operation_id=UNHEX('a5000000000000000000000000000000')";

    auto result = player_snapshot_repository_apply(db, snapshot);
    assert(result.outcome == player_save_apply_outcome::applied);
    assert(scalar(db, "SELECT save_revision FROM player_data WHERE pid=" + id) == 2);
    assert(scalar(db, "SELECT COUNT(*) FROM player_affects WHERE pid=" + id +
                      " AND type=1234 AND duration=20") == 1);
    assert(scalar(db, "SELECT COUNT(*)" + receipt_row + " AND effect_id=6") == 1);
    result = player_snapshot_repository_apply(db, snapshot);
    assert(result.outcome == player_save_apply_outcome::already_applied);

    execute(db, "DELETE" + receipt_row);
    result = player_snapshot_repository_apply(db, snapshot);
    assert(result.outcome == player_save_apply_outcome::terminal_failure &&
           result.error_code == EILSEQ);
    assert(scalar(db, "SELECT save_revision FROM player_data WHERE pid=" + id) == 2);
    execute(db, "INSERT INTO player_spell_effect_receipt(pid,operation_id,effect_id) "
                "VALUES(" + id + ",UNHEX('a5000000000000000000000000000000'),6)");
    snapshot.spell_effect_receipts[0].effect_id = 5;
    result = player_snapshot_repository_apply(db, snapshot);
    assert(result.outcome == player_save_apply_outcome::terminal_failure &&
           result.error_code == EILSEQ);

    snapshot.revision = 3;
    snapshot.affects[0].duration = 30;
    result = player_snapshot_repository_apply(db, snapshot);
    assert(result.outcome == player_save_apply_outcome::terminal_failure);
    assert(scalar(db, "SELECT save_revision FROM player_data WHERE pid=" + id) == 2);
    assert(scalar(db, "SELECT COUNT(*) FROM player_affects WHERE pid=" + id +
                      " AND type=1234 AND duration=20") == 1);
    snapshot.spell_effect_receipts[0].effect_id = 6;
    result = player_snapshot_repository_apply(db, snapshot);
    assert(result.outcome == player_save_apply_outcome::applied);
    assert(scalar(db, "SELECT save_revision FROM player_data WHERE pid=" + id) == 3);
    assert(scalar(db, "SELECT COUNT(*) FROM player_affects WHERE pid=" + id +
                      " AND type=1234 AND duration=30") == 1);

    const std::string quest_id = "b6000000000000000000000000000000";
    execute(db, "INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
                "command_type,schema_version,payload_version,status,result_payload) VALUES("
                "UNHEX('" + quest_id + "'),UNHEX(REPEAT('00',32)),"
                "UNHEX(REPEAT('00',32)),2,2,1,1,X'')");
    execute(db, "INSERT INTO quest_reward_obligation(offering_operation_id,player_pid,"
                "continuation) VALUES(UNHEX('" + quest_id + "')," + id +
                ",UNHEX('" + hex_bytes(quest_terms(pid)) + "'))");
    execute(db, "INSERT INTO quest_reward_xp_entitlement(offering_operation_id,"
                "recipient_pid,reward_index,amount) VALUES(UNHEX('" + quest_id +
                "')," + id + ",0,100)");
    player_snapshot quest = {};
    quest.schema_version = PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION;
    quest.pid = pid;
    quest.revision = 4;
    quest.components = PLAYER_COMPONENT_STATUS;
    quest.encoded_size_bound = 1024;
    quest.status_integers.push_back({player_status_field::experience, 100, 0, false});
    player_quest_xp_receipt_snapshot xp = {};
    xp.offering_operation.bytes[0] = 0xb6;
    xp.reward_index = 0;
    xp.amount = 100;
    quest.quest_xp_receipts.push_back(xp);
    result = player_snapshot_repository_apply(db, quest);
    assert(result.outcome == player_save_apply_outcome::applied);
    const std::string entitlement_row =
        " FROM quest_reward_xp_entitlement WHERE offering_operation_id=UNHEX('" +
        quest_id + "') AND recipient_pid=" + id + " AND reward_index=0";
    assert(scalar(db, "SELECT COUNT(*)" + entitlement_row +
                      " AND applied_at IS NOT NULL") == 1);
    result = player_snapshot_repository_apply(db, quest);
    assert(result.outcome == player_save_apply_outcome::already_applied);
    // A lost recipient ACK may outlive the offering owner's acknowledgment.
    // A later checkpoint must accept the exact existing marker without paying
    // XP again or changing its application timestamp.
    const auto applied_at = scalar(db, "SELECT UNIX_TIMESTAMP(applied_at)*1000000" + entitlement_row);
    execute(db, "UPDATE quest_reward_obligation SET acknowledged_at=CURRENT_TIMESTAMP(6) "
                "WHERE offering_operation_id=UNHEX('" + quest_id + "')");
    quest.revision = 5;
    result = player_snapshot_repository_apply(db, quest);
    assert(result.outcome == player_save_apply_outcome::applied);
    assert(scalar(db, "SELECT UNIX_TIMESTAMP(applied_at)*1000000" + entitlement_row) == applied_at);
    assert(scalar(db, "SELECT exp FROM player_data WHERE pid=" + id) == 100);
    // An obsolete failed attempt can be retired only by operation evidence.
    auto obsolete = quest;
    obsolete.revision = 4;
    auto obsolete_result = player_snapshot_repository_apply(db, obsolete);
    assert(obsolete_result.outcome == player_save_apply_outcome::stale_revision &&
           obsolete_result.durable_revision == 5 && obsolete_result.operation_receipts_verified);
    assert(!player_save_result_matches_exact_request(obsolete, obsolete_result));
    execute(db, "UPDATE quest_reward_xp_entitlement SET applied_at=NULL WHERE "
                "offering_operation_id=UNHEX('" + quest_id + "')");
    assert(player_snapshot_repository_apply(db, obsolete).outcome ==
           player_save_apply_outcome::terminal_failure);
    execute(db, "UPDATE quest_reward_xp_entitlement SET applied_at=CURRENT_TIMESTAMP(6) WHERE "
                "offering_operation_id=UNHEX('" + quest_id + "')");
    obsolete.quest_xp_receipts[0].amount = 99;
    assert(player_snapshot_repository_apply(db, obsolete).outcome ==
           player_save_apply_outcome::terminal_failure);
    assert(scalar(db, "SELECT exp FROM player_data WHERE pid=" + id) == 100);
    obsolete_result = player_snapshot_repository_apply(db, snapshot);
    assert(obsolete_result.outcome == player_save_apply_outcome::stale_revision &&
           obsolete_result.operation_receipts_verified);
    auto conflicting_obsolete = snapshot;
    conflicting_obsolete.spell_effect_receipts[0].effect_id = 5;
    assert(player_snapshot_repository_apply(db, conflicting_obsolete).outcome ==
           player_save_apply_outcome::terminal_failure);
    execute(db, "UPDATE quest_reward_xp_entitlement SET amount=99 WHERE "
                "offering_operation_id=UNHEX('" + quest_id + "')");
    auto invalid_entitlement = quest;
    invalid_entitlement.revision = 6;
    assert(player_snapshot_repository_apply(db, invalid_entitlement).outcome ==
           player_save_apply_outcome::terminal_failure);
    execute(db, "UPDATE quest_reward_xp_entitlement SET amount=100 WHERE "
                "offering_operation_id=UNHEX('" + quest_id + "')");
    execute(db, "UPDATE quest_reward_xp_entitlement SET applied_at=NULL WHERE "
                "offering_operation_id=UNHEX('" + quest_id + "')");
    result = player_snapshot_repository_apply(db, quest);
    assert(result.outcome == player_save_apply_outcome::terminal_failure &&
           result.error_code == EILSEQ);
    execute(db, "UPDATE quest_reward_xp_entitlement SET applied_at=CURRENT_TIMESTAMP(6) "
                "WHERE offering_operation_id=UNHEX('" + quest_id + "')");
    quest.quest_xp_receipts[0].amount = 99;
    result = player_snapshot_repository_apply(db, quest);
    assert(result.outcome == player_save_apply_outcome::terminal_failure &&
           result.error_code == EILSEQ);
    quest.quest_xp_receipts[0].amount = 100;

    execute(db, "DELETE" + entitlement_row);
    result = player_snapshot_repository_apply(db, quest);
    assert(result.outcome == player_save_apply_outcome::already_applied);
    execute(db, "UPDATE quest_reward_obligation SET xp_applied_mask=0 WHERE "
                "offering_operation_id=UNHEX('" + quest_id + "')");
    result = player_snapshot_repository_apply(db, quest);
    assert(result.outcome == player_save_apply_outcome::terminal_failure &&
           result.error_code == EILSEQ);
    execute(db, "DELETE FROM quest_reward_obligation WHERE offering_operation_id=UNHEX('" +
                quest_id + "')");
    execute(db, "DELETE FROM critical_operation_inbox WHERE operation_id=UNHEX('" +
                quest_id + "')");
    // A group peer's exact durable marker must survive the offering owner's
    // ACK and a lost peer completion. A fresh marker under an ACK is refused.
    const int group_owner = pid + 1, group_peer = pid + 2;
    const std::string group_id = "e8000000000000000000000000000000";
    execute(db, "INSERT INTO player_data(pid,name,save_revision) VALUES(" +
                std::to_string(group_owner) + ",'QuestOwnerFixture',1),(" +
                std::to_string(group_peer) + ",'QuestPeerFixture',1)");
    execute(db, "INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
                "command_type,schema_version,payload_version,status,result_payload) VALUES("
                "UNHEX('" + group_id + "'),UNHEX(REPEAT('00',32)),"
                "UNHEX(REPEAT('00',32)),2,2,1,1,X'')");
    execute(db, "INSERT INTO quest_reward_obligation(offering_operation_id,player_pid,"
                "continuation) VALUES(UNHEX('" + group_id + "')," + std::to_string(group_owner) +
                ",UNHEX('" + hex_bytes(quest_terms(group_owner, group_peer)) + "'))");
    execute(db, "INSERT INTO quest_reward_xp_entitlement(offering_operation_id,"
                "recipient_pid,reward_index,amount) VALUES(UNHEX('" + group_id + "')," +
                std::to_string(group_owner) + ",0,100),(UNHEX('" + group_id + "')," +
                std::to_string(group_peer) + ",0,60)");
    auto group_save = quest;
    group_save.revision = 2;
    group_save.quest_xp_receipts[0].offering_operation.bytes[0] = 0xe8;
    group_save.pid = group_owner;
    assert(player_snapshot_repository_apply(db, group_save).outcome == player_save_apply_outcome::applied);
    group_save.pid = group_peer;
    group_save.status_integers[0].signed_value = 60;
    group_save.quest_xp_receipts[0].amount = 60;
    assert(player_snapshot_repository_apply(db, group_save).outcome == player_save_apply_outcome::applied);
    const std::string peer_entitlement =
        " FROM quest_reward_xp_entitlement WHERE offering_operation_id=UNHEX('" +
        group_id + "') AND recipient_pid=" + std::to_string(group_peer) + " AND reward_index=0";
    const auto peer_applied_at = scalar(db, "SELECT UNIX_TIMESTAMP(applied_at)*1000000" + peer_entitlement);
    execute(db, "UPDATE quest_reward_obligation SET acknowledged_at=CURRENT_TIMESTAMP(6) "
                "WHERE offering_operation_id=UNHEX('" + group_id + "')");
    group_save.revision = 3;
    assert(player_snapshot_repository_apply(db, group_save).outcome == player_save_apply_outcome::applied);
    assert(scalar(db, "SELECT UNIX_TIMESTAMP(applied_at)*1000000" + peer_entitlement) == peer_applied_at);
    assert(scalar(db, "SELECT exp FROM player_data WHERE pid=" + std::to_string(group_peer)) == 60);
    execute(db, "UPDATE quest_reward_xp_entitlement SET applied_at=NULL WHERE "
                "offering_operation_id=UNHEX('" + group_id + "') AND recipient_pid=" +
                std::to_string(group_peer));
    group_save.revision = 4;
    assert(player_snapshot_repository_apply(db, group_save).outcome == player_save_apply_outcome::terminal_failure);
    assert(scalar(db, "SELECT save_revision FROM player_data WHERE pid=" + std::to_string(group_peer)) == 3);
    assert(scalar(db, "SELECT COUNT(*)" + peer_entitlement + " AND applied_at IS NULL") == 1);
    execute(db, "DELETE FROM quest_reward_xp_entitlement WHERE offering_operation_id=UNHEX('" + group_id + "')");
    execute(db, "DELETE FROM quest_reward_obligation WHERE offering_operation_id=UNHEX('" + group_id + "')");
    execute(db, "DELETE FROM critical_operation_inbox WHERE operation_id=UNHEX('" + group_id + "')");
    execute(db, "DELETE FROM player_data WHERE pid IN (" + std::to_string(group_owner) + ',' + std::to_string(group_peer) + ')');
    auto death = snapshot;
    death.schema_version = PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION;
    death.revision = 6;
    death.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
    death.save_intent = RENT_DEATH;
    death.encoded_size_bound = 8192;
    death.spell_effect_receipts[0].operation_id.bytes[0] = 0xb6;
    death.affects[0].duration = 40;
    const std::string death_quest_id = "c7000000000000000000000000000000";
    execute(db, "INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,"
                "command_type,schema_version,payload_version,status,result_payload) VALUES("
                "UNHEX('" + death_quest_id + "'),UNHEX(REPEAT('00',32)),"
                "UNHEX(REPEAT('00',32)),2,2,1,1,X'')");
    execute(db, "INSERT INTO quest_reward_obligation(offering_operation_id,player_pid,"
                "continuation) VALUES(UNHEX('" + death_quest_id + "')," + id +
                ",UNHEX('" + hex_bytes(quest_terms(pid)) + "'))");
    execute(db, "INSERT INTO quest_reward_xp_entitlement(offering_operation_id,"
                "recipient_pid,reward_index,amount) VALUES(UNHEX('" + death_quest_id +
                "')," + id + ",0,100)");
    xp.offering_operation.bytes[0] = 0xc7;
    death.quest_xp_receipts.push_back(xp);
    death.status_integers.push_back({player_status_field::experience, 200, 0, false});
    const std::string death_entitlement_row =
        " FROM quest_reward_xp_entitlement WHERE offering_operation_id=UNHEX('" +
        death_quest_id + "') AND recipient_pid=" + id + " AND reward_index=0";
    death.death.emplace();
    death.death->operation_id.bytes[0] = 0xd7;
    death.death->corpse_room_vnum = 1201;
    death.death->wallet_revision = 1;
    player_item_snapshot corpse = {};
    corpse.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
    corpse.object_uid = 900001;
    corpse.vnum = VOBJ_CORPSE;
    corpse.type = ITEM_CORPSE;
    corpse.values[CORPSE_SAVEID] = pid;
    corpse.values[CORPSE_PID] = pid;
    corpse.values[CORPSE_FLAGS] = PC_CORPSE;
    death.death->corpse.push_back(corpse);
    std::vector<uint8_t> death_bytes;
    assert(player_snapshot_encode(death, &death_bytes) == player_snapshot_codec_result::ok);
    execute(db, "UPDATE player_data SET wallet_revision=1 WHERE pid=" + id);
    const std::string death_receipt_row = " FROM player_spell_effect_receipt WHERE pid=" + id +
        " AND operation_id=UNHEX('b6000000000000000000000000000000')";
    execute(db, "CREATE TRIGGER spell_death_fail AFTER INSERT ON player_death_disposition "
        "FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='injected death failure'");
    result = player_snapshot_repository_apply(db, death);
    if (result.error_code != 1644)
        std::cerr << "injected normal death outcome=" << static_cast<unsigned>(result.outcome)
                  << " error=" << result.error_code << '\n';
    assert(result.outcome == player_save_apply_outcome::terminal_failure && result.error_code == 1644);
    assert(scalar(db, "SELECT save_revision FROM player_data WHERE pid=" + id) == 5);
    assert(scalar(db, "SELECT exp FROM player_data WHERE pid=" + id) == 100);
    assert(scalar(db, "SELECT COUNT(*)" + death_entitlement_row + " AND applied_at IS NULL") == 1);
    assert(scalar(db, "SELECT COUNT(*)" + death_receipt_row) == 0);
    assert(scalar(db, "SELECT COUNT(*) FROM player_affects WHERE pid=" + id + " AND duration=40") == 0);
    execute(db, "DROP TRIGGER spell_death_fail");
    result = player_snapshot_repository_apply(db, death);
    if (result.outcome != player_save_apply_outcome::applied)
        std::cerr << "normal death receipt outcome=" << static_cast<unsigned>(result.outcome)
                  << " error=" << result.error_code << '\n';
    assert(result.outcome == player_save_apply_outcome::applied);
    assert(scalar(db, "SELECT exp FROM player_data WHERE pid=" + id) == 200);
    assert(scalar(db, "SELECT COUNT(*)" + death_entitlement_row + " AND applied_at IS NOT NULL") == 1);
    assert(scalar(db, "SELECT COUNT(*)" + death_receipt_row + " AND effect_id=6") == 1);
    assert(scalar(db, "SELECT COUNT(*) FROM player_affects WHERE pid=" + id + " AND duration=40") == 1);
    assert(player_snapshot_repository_apply(db, death).outcome == player_save_apply_outcome::already_applied);
    execute(db, "UPDATE quest_reward_xp_entitlement SET applied_at=NULL WHERE "
                "offering_operation_id=UNHEX('" + death_quest_id + "')");
    assert(player_snapshot_repository_apply(db, death).outcome == player_save_apply_outcome::terminal_failure);
    execute(db, "UPDATE quest_reward_xp_entitlement SET applied_at=CURRENT_TIMESTAMP(6) WHERE "
                "offering_operation_id=UNHEX('" + death_quest_id + "')");
    execute(db, "DELETE" + death_receipt_row);
    assert(player_snapshot_repository_apply(db, death).outcome == player_save_apply_outcome::terminal_failure);
    execute(db, "DELETE FROM player_death_disposition WHERE pid=" + id);
    execute(db, "DELETE" + death_entitlement_row);
    execute(db, "DELETE FROM quest_reward_obligation WHERE offering_operation_id=UNHEX('" + death_quest_id + "')");
    execute(db, "DELETE FROM critical_operation_inbox WHERE operation_id=UNHEX('" + death_quest_id + "')");
    execute(db, "DELETE" + receipt_row);
    execute(db, "DELETE FROM player_affects WHERE pid=" + id);
    verify_economic_receipts(db, pid);
    execute(db, "DELETE FROM player_data WHERE pid=" + id);
    verify_craft_progression(db, pid+1);
    mysql_close(db);
    std::cout << "spell affect and quest XP exact receipt SQL transactions: ok\n";
}
'''


def main() -> None:
    if os.environ.get("DB_HOST") != "127.0.0.1" or not (
        os.environ.get("DB_NAME", "").startswith("playtime_test_")
        or (os.environ.get("DB_NAME", "").startswith("economic_schema_test_")
            and os.environ.get("TEST_DB_DISPOSABLE") == "1")
    ):
        raise SystemExit("refusing non-disposable SQL target")
    with tempfile.TemporaryDirectory(prefix="spell-receipt-sql-") as directory:
        source = Path(directory) / "spell_receipt.cpp"
        binary = Path(directory) / "spell_receipt"
        source.write_text(HARNESS)
        subprocess.run([
            "g++", "-std=c++20", "-ffunction-sections", "-fdata-sections", "-Isrc",
            "-I/usr/include/mysql", str(source), "src/player/player_snapshot_repository.c",
            "src/player/player_snapshot_codec.c", "src/player/player_save_journal.c",
            "src/player/player_load_repository.c", "src/player/player_load_topology.c",
            "src/player/player_death_recovery_query.c",
            "src/player/player_death_conflict_repository.c",
            "src/persistence/critical_command.c",
            "src/persistence/quest_reward_obligation_repository.c",
            "src/item/item_transfer_command.c", "src/world/quest_mobile_native_reference.c", "src/economy/economic_source_event.c", "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c", "src/economy/currency_command.c",
            "src/persistence/player_death_restitution_command.c",
            "src/sql/item_extra_descr_codec.c", "src/persistence/persistence_observability.c",
            "-Wl,--gc-sections", "-lmysqlclient", "-lcrypto", "-pthread", "-o", str(binary),
        ], cwd=ROOT, check=True)
        subprocess.run([str(binary)], cwd=ROOT, check=True)


if __name__ == "__main__":
    main()
