#!/usr/bin/env python3
"""Native codec regression, plus opt-in disposable MySQL transaction integration.

Default: python3 tests/async/test_epic_stone_transaction.py
SQL: EPIC_STONE_MYSQL_TEST=1 EPIC_TEST_DB_NAME=epic_stone_test_<unique> ...
Supply DB_HOST, DB_PORT, DB_USER, DB_PASSWD from an explicitly disposable service.
The SQL harness creates a new database, refuses an existing database, and leaves
its synthetic contents for inspection. It never reads the repository .env.
Requires Linux g++, OpenSSL; SQL additionally requires mysql_config and MySQL.
"""

import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
CODEC = r'''
#include "world/zone_touch_command.h"
#include "world/epic_command.h"
#include <algorithm>
#include <cassert>
#include <limits>

int main()
{
    critical_operation_id id = {};
    id.bytes[0] = 42;
    zone_touch_payload payload = {};
    payload.zone_number = 77;
    payload.toucher_pid = 100;
    payload.boot_time = 1000;
    payload.touched_at = 1100;
    payload.group_size = 15;
    payload.epic_value = 9;
    payload.alignment_delta = -1;
    payload.reset_requested = 1;
    for (size_t i = 0; i < payload.group_size; ++i)
        payload.participant_pids[i] = 100 + i;

    // A v1 command and its original padded 88-byte receipt remain readable.
    critical_command legacy;
    assert(zone_touch_command_build(&legacy, id, payload));
    assert(legacy.payload_version == 1 && legacy.payload.size() == 85);
    std::array<uint8_t, 88> old_receipt = {};
    std::copy(legacy.payload.begin(), legacy.payload.end(), old_receipt.begin());
    zone_touch_result decoded;
    assert(zone_touch_command_decode_result(old_receipt.data(), old_receipt.size(), &decoded));
    assert(decoded.group_size == 15 && decoded.participant_pids[14] == 114);
    assert(decoded.stone_uid == 0 && decoded.record_zone == 1);
    assert(decoded.alignment_delta == -1 && decoded.epic_value == 9);
    assert(decoded.balances[14] == 0 && !decoded.recovered_claim);
    old_receipt.back() = 1;
    assert(!zone_touch_command_decode_result(old_receipt.data(), old_receipt.size(), &decoded));

    payload.stone_uid = UINT64_C(0xfedcba9876543210);
    payload.stone_level = 56;
    for (size_t i = 0; i < payload.group_size; ++i)
        payload.awards[i] = {static_cast<int32_t>(10 + i), static_cast<int32_t>(i),
                             static_cast<uint8_t>(i % 4)};
    critical_command command;
    assert(zone_touch_command_build(&command, id, payload));
    command.accepted_at_usec = 1;
    assert(command.payload_version == 2 && critical_command_valid(command));
    zone_touch_payload roundtrip;
    assert(zone_touch_command_decode_payload(command, &roundtrip));
    assert(roundtrip.stone_uid == payload.stone_uid && roundtrip.awards[14].amount == 24);
    zone_touch_result receipt(payload);
    receipt.recovered_claim = true;
    for (size_t i = 0; i < payload.group_size; ++i)
    {
        receipt.balances[i] = std::numeric_limits<int64_t>::max() - i;
        receipt.revisions[i] = std::numeric_limits<uint64_t>::max() - i;
        critical_command child, repeated;
        assert(zone_touch_award_command(command, i, &child));
        assert(zone_touch_award_command(command, i, &repeated));
        assert(child.operation_id.bytes == repeated.operation_id.bytes);
        assert(child.operation_id.bytes != command.operation_id.bytes);
        epic_command_payload award;
        assert(epic_command_decode_payload(child, &award));
        assert(award.pid == 100 + i && award.delta == 10 + static_cast<int64_t>(i));
        assert(award.reason == epic_reason_type::zone_award && award.reason_id == 77);
        if (i)
        {
            critical_command previous;
            assert(zone_touch_award_command(command, i - 1, &previous));
            assert(child.operation_id.bytes != previous.operation_id.bytes);
        }
    }
    std::array<uint8_t, ZONE_TOUCH_RESULT_BYTES> bytes;
    assert(zone_touch_command_encode_result(receipt, &bytes));
    assert(bytes.size() == 512);
    assert(zone_touch_command_decode_result(bytes.data(), bytes.size(), &decoded));
    assert(decoded.balances == receipt.balances && decoded.revisions == receipt.revisions);
    assert(decoded.recovered_claim && decoded.stone_level == 56);
    assert(decoded.awards[14].errand == 14 && decoded.awards[14].flags == 2);
    assert(!zone_touch_command_decode_result(bytes.data(), bytes.size() - 1, &decoded));
    bytes.back() = 1;
    assert(!zone_touch_command_decode_result(bytes.data(), bytes.size(), &decoded));
    auto truncated = command;
    truncated.payload.pop_back();
    assert(!zone_touch_command_decode_payload(truncated, &roundtrip));
    auto extra = command;
    extra.payload.push_back(0);
    assert(!zone_touch_command_decode_payload(extra, &roundtrip));
    auto wrong_keys = command;
    wrong_keys.keys.pop_back();
    assert(!zone_touch_command_decode_payload(wrong_keys, &roundtrip));
    assert(!zone_touch_award_command(command, 15, &extra));
    auto invalid = payload;
    invalid.participant_pids[14] = invalid.participant_pids[0];
    assert(!zone_touch_command_build(&extra, id, invalid));
    invalid = payload;
    invalid.awards[14].amount = 0;
    assert(!zone_touch_command_build(&extra, id, invalid));
    invalid = payload;
    invalid.awards[14].flags = 4;
    assert(!zone_touch_command_build(&extra, id, invalid));

    // A stone awarded outside a qualifying zone still carries all payouts.
    payload.zone_number = 0;
    payload.record_zone = 0;
    assert(zone_touch_command_build(&command, id, payload));
    assert(zone_touch_command_decode_payload(command, &roundtrip));
    assert(roundtrip.record_zone == 0 && roundtrip.stone_uid == payload.stone_uid);
    assert(zone_touch_command_encode_result(zone_touch_result(payload), &bytes));
    assert(zone_touch_command_decode_result(bytes.data(), bytes.size(), &decoded));
    assert(decoded.zone_number == 0 && decoded.record_zone == 0);
}
'''

SQL_SOURCES = [
    "persistence/critical_command.c", "world/epic_command.c", "economy/currency_command.c",
    "item/item_transfer_command.c", "item/item_transfer_repository.c",
    "economy/auction_command.c", "economy/auction_repository.c",
    "combat/combat_outcome_command.c", "combat/combat_outcome_repository.c",
    "guild/artifact_guild_command.c", "guild/artifact_guild_repository.c",
    "economy/boon_reward_command.c", "economy/boon_reward_repository.c",
    "world/zone_touch_command.c", "world/zone_touch_repository.c",
    "account/session_audit_command.c", "account/session_audit_repository.c",
    "economy/coin_transfer_command.c", "player/player_snapshot_codec.c",
    "persistence/critical_command_repository.c",
]


class EpicStoneTransactionTests(unittest.TestCase):
    def compile_and_run(self, harness, sources, mysql=False):
        self.assertIsNotNone(shutil.which("g++"), "Linux g++ is required")
        with tempfile.TemporaryDirectory(prefix="epic-stone-test-") as directory:
            binary = str(Path(directory) / "harness")
            if isinstance(harness, str):
                path = Path(directory) / "harness.cpp"
                path.write_text(harness)
            else:
                path = harness
            flags, libraries = [], []
            if mysql:
                self.assertIsNotNone(shutil.which("mysql_config"))
                flags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
                libraries = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
            subprocess.run([
                "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                "-pthread", "-ffunction-sections", "-fdata-sections", "-Isrc", *flags,
                str(path), *(str(ROOT / "src" / source) for source in sources),
                "-Wl,--gc-sections", *libraries, "-lcrypto", "-o", binary,
            ], cwd=ROOT, check=True)
            subprocess.run([binary], cwd=ROOT, check=True)

    def test_codec_legacy_maximum_group_and_invalid_inputs(self):
        self.compile_and_run(CODEC, [
            "persistence/critical_command.c", "world/epic_command.c", "world/zone_touch_command.c",
        ])

    @unittest.skipUnless(os.environ.get("EPIC_STONE_MYSQL_TEST") == "1",
                         "requires explicitly provisioned disposable MySQL service")
    def test_mysql_atomic_awards_replay_and_failure_rollback(self):
        self.compile_and_run(ROOT / "tests/async/epic_stone_mysql_harness.cpp", SQL_SOURCES, mysql=True)


if __name__ == "__main__":
    unittest.main()
