#!/usr/bin/env python3
"""Bounded codec, mutation route, schema, and publication contracts for PvP outcomes."""

from _paths import SRC
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "combat/combat_outcome_command.h"
#include <cassert>
#include <cstring>

combat_outcome_participant participant(uint32_t pid, combat_participant_role role)
{
    combat_outcome_participant value = {};
    value.pid = pid;
    value.role = role;
    value.level = 50;
    value.racewar = 1;
    value.expected_frag_revision = pid;
    value.expected_epic_revision = pid + 1;
    value.expected_wallet_revision = pid + 2;
    value.expected_bank_revision = 9;
    memcpy(value.account_name.data(), "combat-account", 14);
    memcpy(value.description.data(), "bounded player", 14);
    return value;
}

int main()
{
    combat_outcome_payload payload = {};
    payload.victim_pid = 22;
    payload.room_vnum = 101;
    memcpy(payload.room_name.data(), "A bounded room", 14);
    payload.participant_count = 2;
    payload.participants[0] = participant(11, combat_participant_role::killer);
    payload.participants[0].frag_delta = 100;
    payload.participants[0].epic_delta = 500;
    payload.participants[0].wallet_delta_copper = 10000;
    payload.participants[1] = participant(22, combat_participant_role::victim);
    payload.participants[1].frag_delta = -100;

    critical_operation_id id = {};
    assert(critical_operation_id_generate(&id));
    critical_command command = {};
    assert(combat_outcome_command_build(&command, id, payload));
    assert(command.type == critical_command_type::combat_outcome);
    assert(command.keys.size() == 3);
    combat_outcome_payload decoded = {};
    assert(combat_outcome_command_decode_payload(command, &decoded));
    assert(decoded.participant_count == 2);
    assert(decoded.participants[0].epic_delta == 500);

    critical_command tampered = command;
    tampered.keys.pop_back();
    assert(!combat_outcome_command_decode_payload(tampered, &decoded));
    payload.participants[1].pid = 11;
    assert(!combat_outcome_command_build(&command, id, payload));

    combat_outcome_result result = {};
    result.event_id = 77;
    result.participant_count = COMBAT_OUTCOME_MAX_PARTICIPANTS;
    for (size_t i = 0; i < result.participant_count; ++i)
    {
        result.participants[i].pid = static_cast<uint32_t>(i + 1);
        result.participants[i].frags = 10;
        result.participants[i].epics = 20;
        result.participants[i].wallet_value = 30;
        result.participants[i].frag_revision = 1;
        result.participants[i].epic_revision = 2;
        result.participants[i].wallet_revision = 3;
        result.participants[i].bank_revision = 4;
    }
    std::array<uint8_t, COMBAT_OUTCOME_RESULT_BYTES> bytes = {};
    assert(combat_outcome_command_encode_result(result, &bytes));
    combat_outcome_result decoded_result = {};
    assert(combat_outcome_command_decode_result(bytes.data(), bytes.size(), &decoded_result));
    assert(decoded_result.participant_count == COMBAT_OUTCOME_MAX_PARTICIPANTS);
}
'''


class CombatOutcomeCutoverTests(unittest.TestCase):
    def test_bounded_pointer_free_codec(self):
        with tempfile.TemporaryDirectory() as directory:
            harness = Path(directory) / "combat_codec.cpp"
            binary = Path(directory) / "combat_codec"
            harness.write_text(HARNESS)
            subprocess.run([
                "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", f"-I{SRC}",
                str(harness), str(SRC / "critical_command.c"),
                str(SRC / "currency_command.c"), str(SRC / "combat_outcome_command.c"),
                "-lcrypto", "-o", str(binary),
            ], check=True)
            subprocess.run([str(binary)], check=True)

    def test_combat_route_has_no_direct_mutation_io(self):
        fight = (SRC / "fight.c").read_text()
        start = fight.index("void AddFrags(P_char ch, P_char victim)")
        end = fight.index("unsigned int calculate_ch_state", start)
        body = fight[start:end]
        self.assertIn("submit_pvp_outcome", body)
        for forbidden in ("sql_modify_frags", "sql_save_pkill", "ADD_MONEY",
                          "redis_invalidate_fraglist", "epic_frag"):
            self.assertNotIn(forbidden, body)
        capture = fight[fight.index("static bool submit_pvp_outcome"):start]
        self.assertIn("combat_outcome_transaction_submit", capture)
        self.assertNotIn("GET_PLAYER_LOG", capture)
        self.assertNotIn("get_equipment_list", capture)

    def test_repository_composes_all_authorities_before_outbox(self):
        repository = (SRC / "combat_outcome_repository.c").read_text()
        generic = (SRC / "critical_command_repository.c").read_text()
        for token in ("FOR UPDATE", "combat_frag_ledger", "epic_ledger",
                      "currency_ledger", "pkill_event", "pkill_info", "progress"):
            self.assertIn(token, repository)
        branch = generic[generic.index("if (combat_command)"):]
        self.assertLess(branch.index("insert_outbox"), branch.index('execute(connection, "COMMIT")'))
        self.assertLess(branch.index("finish_inbox"), branch.index('execute(connection, "COMMIT")'))

    def test_schema_and_checkpoint_protection(self):
        migration = (ROOT / "migrations/combat_outcome.sql").read_text()
        bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
        runner = (ROOT / "migrations/run_migration.sh").read_text()
        for token in ("frag_revision", "combat_frag_baseline", "combat_outcome",
                      "combat_outcome_participant", "combat_frag_ledger"):
            self.assertIn(token, migration)
            self.assertIn(token, bootstrap)
        self.assertIn("combat_outcome.sql", runner)
        snapshot = (SRC / "player_snapshot_repository.c").read_text()
        self.assertIn("row.field == player_status_field::frags", snapshot)
        legacy = (SRC / "sql_player.c").read_text()
        self.assertIn('"frags=frags, oldfrags=oldfrags', legacy)
        new_player = legacy[legacy.rindex("bool sql_save_player_status"):]
        self.assertIn("INSERT INTO combat_frag_baseline", new_player)
        self.assertLess(
            new_player.index("INSERT INTO combat_frag_baseline"),
            new_player.index("UPDATE player_data SET account_name"),
        )


if __name__ == "__main__":
    unittest.main()
