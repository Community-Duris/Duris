#!/usr/bin/env python3
"""Codec, source boundary, schema, and publication contracts for artifact/guild outcomes."""

from _paths import SRC
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "guild/artifact_guild_command.h"
#include <cassert>

int main()
{
    critical_operation_id parent = {};
    assert(critical_operation_id_generate(&parent));
    critical_operation_id operation = {}, duplicate = {};
    assert(critical_operation_id_derive(parent, 0x41475431, 42, &operation));
    assert(critical_operation_id_derive(parent, 0x41475431, 42, &duplicate));
    assert(critical_operation_id_equal(operation, duplicate));
    artifact_guild_payload payload = {};
    payload.parent_operation_id = parent;
    payload.actor_pid = 42;
    payload.guild_id = 7;
    payload.expected_guild_revision = 3;
    payload.prestige_delta = 5;
    payload.construction_delta = 1;
    payload.artifact_count = ARTIFACT_GUILD_MAX_ARTIFACTS;
    for (size_t i = 0; i < payload.artifact_count; ++i)
    {
        payload.artifacts[i] = {static_cast<int32_t>(1000 + i), ARTIFACT_DELTA_FEED,
                                i, 100, 200, 42, 42, 0, 0};
    }
    critical_command command = {};
    assert(artifact_guild_command_build(&command, operation, payload));
    assert(command.keys.size() == ARTIFACT_GUILD_MAX_ARTIFACTS + 2);
    assert(command.keys[0].type == critical_entity_type::player);
    assert(command.keys[0].id == payload.actor_pid);
    artifact_guild_payload decoded = {};
    assert(artifact_guild_command_decode_payload(command, &decoded));
    critical_command tampered = command;
    tampered.expected_revisions.pop_back();
    assert(!artifact_guild_command_decode_payload(tampered, &decoded));
    artifact_guild_result result = {};
    result.guild_id = 7;
    result.artifact_count = ARTIFACT_GUILD_MAX_ARTIFACTS;
    std::array<uint8_t, ARTIFACT_GUILD_RESULT_BYTES> bytes = {};
    assert(artifact_guild_command_encode_result(result, &bytes));
    artifact_guild_result decoded_result = {};
    assert(artifact_guild_command_decode_result(bytes.data(), bytes.size(), &decoded_result));
    assert(decoded_result.artifact_count == ARTIFACT_GUILD_MAX_ARTIFACTS);
}
'''


class ArtifactGuildCutoverTests(unittest.TestCase):
    def test_bounded_codec_and_deterministic_child_identity(self):
        with tempfile.TemporaryDirectory() as directory:
            harness = Path(directory) / "artifact_guild_codec.cpp"
            binary = Path(directory) / "artifact_guild_codec"
            harness.write_text(HARNESS)
            subprocess.run([
                "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", f"-I{SRC}",
                str(harness), str(SRC / "critical_command.c"),
                str(SRC / "artifact_guild_command.c"), "-lcrypto", "-o", str(binary),
            ], check=True)
            subprocess.run([str(binary)], check=True)

    def test_epic_and_combat_publication_use_derived_transaction(self):
        epic = (SRC / "epic.c").read_text()
        award_start = epic.index("void epic_award_committed(")
        award_end = epic.index("void epic_level_committed", award_start)
        award = epic[award_start:award_end]
        pvp_start = epic.index("void epic_publish_pvp_award(")
        pvp_end = epic.index("void epic_feed_artifacts(", pvp_start)
        pvp = epic[pvp_start:pvp_end]
        for body in (award, pvp):
            self.assertNotIn("artifact_guild_transaction_submit", body)
            self.assertNotIn("add_points_from_epics", body)
            self.assertNotIn("epic_feed_artifacts", body)
            self.assertNotIn("artifact_feed_sql", body)
        gain_start = epic.index("void gain_epic(")
        gain_end = epic.index("struct affected_type *get_epic_task", gain_start)
        gain = epic[gain_start:gain_end]
        self.assertIn("epic_transaction_submit_identified", gain)
        self.assertIn("artifact_guild_transaction_submit(ch, operation_id", gain)
        self.assertLess(gain.index("epic_transaction_submit_identified"),
                        gain.index("artifact_guild_transaction_submit"))
        combat = (SRC / "fight.c").read_text()
        callback = combat[combat.index("static void combat_outcome_committed"):
                          combat.index("static bool submit_pvp_outcome")]
        self.assertNotIn("artifact_guild_transaction_submit", callback)
        capture = combat[combat.index("static bool submit_pvp_outcome"):
                         combat.index("void AddFrags")]
        self.assertIn("combat_outcome_transaction_submit(payload, combat_outcome_committed, &operation_id)", capture)
        self.assertIn("artifact_guild_transaction_submit(participant, operation_id", capture)
        self.assertLess(capture.index("combat_outcome_transaction_submit"),
                        capture.index("artifact_guild_transaction_submit"))

    def test_repository_commits_ledgers_before_outbox(self):
        repository = (SRC / "artifact_guild_repository.c").read_text()
        generic = (SRC / "critical_command_repository.c").read_text()
        for token in ("FOR UPDATE", "artifact_delta_ledger", "guild_outcome_ledger",
                      "artifact_domain_state", "outcome_revision", "ESTALE"):
            self.assertIn(token, repository)
        branch = generic[generic.index("if (artifact_guild_command)"):]
        self.assertLess(branch.index("insert_outbox"), branch.index('execute(connection, "COMMIT")'))
        self.assertLess(branch.index("finish_inbox"), branch.index('execute(connection, "COMMIT")'))

    def test_hydration_schema_and_generic_save_protection(self):
        state = (SRC / "artifact_guild_state.c").read_text()
        self.assertIn("next_artifacts", state)
        self.assertIn("artifacts.swap(next_artifacts)", state)
        self.assertIn("artifact_guild_state_hydrate", (SRC / "comm.c").read_text())
        migration = (ROOT / "migrations/artifact_guild_outcome.sql").read_text()
        bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
        for token in ("artifact_domain_state", "artifact_guild_outcome",
                      "artifact_delta_ledger", "guild_outcome_ledger", "outcome_revision"):
            self.assertIn(token, migration)
            self.assertIn(token, bootstrap)
        save = (SRC / "sql_player.c").read_text()
        self.assertIn("prestige=prestige", save)
        self.assertIn("construction=construction", save)


if __name__ == "__main__":
    unittest.main()
