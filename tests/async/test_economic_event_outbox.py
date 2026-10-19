#!/usr/bin/env python3
"""Economic audit events and transaction-owned wallet checkpoint regressions."""

from _paths import rel, SRC
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class EconomicEventOutboxTests(unittest.TestCase):
    def test_executable_outbox_behavior(self):
        """Compile and execute the C++ unit test covering outbox drain, fallbacks, and ring-buffer bounds."""
        with tempfile.TemporaryDirectory(prefix="duris-outbox-") as temp_dir:
            binary = Path(temp_dir) / "economic_event_outbox_test"
            command = [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Wpedantic",
                "-Werror",
                "-D__NO_MYSQL__",
                "-Isrc",
                "-I/usr/include/mysql",
                str(ROOT / "tests/async/economic_event_outbox_test.cpp"),
                rel("economic_event_outbox.c"),
                "-o",
                str(binary),
            ]
            compile_res = subprocess.run(
                command, cwd=ROOT, check=True, capture_output=True, text=True
            )
            self.assertEqual(compile_res.returncode, 0)

            run_res = subprocess.run(
                [str(binary)], cwd=ROOT, check=True, capture_output=True, text=True
            )
            self.assertEqual(run_res.returncode, 0)
            self.assertIn("ALL ECONOMIC EVENT OUTBOX TESTS PASSED", run_res.stdout)

    def test_currency_authority_source_contract(self):
        """Wallet mutation enters the same transaction path used by bank and shop commands."""
        utility_c = (SRC / "core/utility.c").read_text()
        for start, end in (("void ADD_MONEY(", "/* TOWARDS BANK MONEY"),
                           ("int SUB_MONEY(", "void strToLower(")):
            body = utility_c[utility_c.index(start):utility_c.index(end)]
            self.assertIn("currency_transaction_submit_wallet_value(", body)
        transaction = (SRC / "economy/currency_transaction.c").read_text()
        self.assertIn('"wallet_commit"', transaction)
        self.assertNotIn("unused_tx_wallet_value_ptr", utility_c)

    def test_persistence_pipeline_integration_contract(self):
        """Verify the economic event outbox is flushed periodically and registered in worker jobs."""
        checkpoint_c = (SRC / "persistence/persistence_checkpoint.c").read_text()
        self.assertIn("economic_event_flush_outbox();", checkpoint_c)

        worker_h = (SRC / "player/player_save_worker.h").read_text()
        worker_c = (SRC / "player/player_save_worker.c").read_text()
        self.assertIn("economy,", worker_h)
        self.assertIn('case persistence_job_kind::economy:\n\t\treturn "economy";', worker_c)

        # Checkpoints cannot race the authoritative currency transaction.
        repo_c = (SRC / "player/player_snapshot_repository.c").read_text()
        apply_status = repo_c[
            repo_c.index("query_result apply_status(") : repo_c.index(
                "template <typename Row, typename Append>"
            )
        ]
        for field in ("copper", "silver", "gold", "platinum"):
            self.assertIn(f"row.field == player_status_field::{field}", apply_status)

    def test_delayed_checkpoint_preserves_committed_wallet(self):
        harness = r'''
#include "player/player_snapshot_repository.c"
#include <cassert>
#include <map>
static std::string applied_sql;
extern "C" int mysql_real_query(MYSQL *, const char *sql, unsigned long length)
{
    applied_sql.assign(sql, length);
    return 0;
}
extern "C" unsigned long mysql_real_escape_string(MYSQL *, char *out, const char *in,
                                                   unsigned long length)
{
    memcpy(out, in, length);
    out[length] = 0;
    return length;
}
int main()
{
    player_snapshot old = {};
    old.pid = 77;
    old.room_vnum = 3001;
    old.status_integers = {{player_status_field::copper, 80, 0, false},
                          {player_status_field::silver, 9, 0, false},
                          {player_status_field::gold, 5, 0, false},
                          {player_status_field::platinum, 4, 0, false},
                          {player_status_field::level, 20, 0, false}};
    // Deposit completes after capture while the live receipt is still pending.
    std::map<std::string, int> wallet = {{"copper", 0}, {"silver", 0},
                                        {"gold", 0}, {"platinum", 0}};
    assert(apply_status(nullptr, old).ok);
    assert(applied_sql.find(",level=20") != std::string::npos);
    for (auto &[field, amount] : wallet)
    {
        const auto start = applied_sql.find("," + field + "=");
        if (start != std::string::npos)
            amount = std::stoi(applied_sql.substr(start + field.size() + 2));
        assert(amount == 0); // Old snapshot must not resurrect deposited coins.
    }
}
'''
        with tempfile.TemporaryDirectory(prefix="wallet-checkpoint-") as directory:
            source = Path(directory) / "regression.cpp"
            binary = Path(directory) / "regression"
            source.write_text(harness)
            subprocess.run([
                "g++", "-std=c++20", "-Isrc", "-I/usr/include/mysql",
                "-ffunction-sections", "-fdata-sections", str(source),
                rel("persistence_observability.c"), "-Wl,--gc-sections",
                "-lmysqlclient", "-pthread", "-o", str(binary),
            ], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True)

    def test_committed_wallet_receipts_are_audited_once(self):
        harness = r'''
#include "core/utils.h"
#include "economy/currency_transaction.h"
#include "economy/economic_event_outbox.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>
static room_data room = {};
P_room world = &room;
extern const int top_of_world = 0;
static std::vector<int64_t> deltas;
void economic_event_record(uint32_t pid, EconomicAction action, int64_t delta,
                           uint32_t target, uint32_t context, const char *reason)
{
    assert(pid == 77 && target == 0 && context == 3001);
    assert(action == (delta > 0 ? EconomicAction::WALLET_ADD : EconomicAction::WALLET_SUB));
    assert(strcmp(reason, "wallet_commit") == 0);
    deltas.push_back(delta);
}
void gmcp_char_vitals(P_char) {}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { abort(); }
int main()
{
    room.number = 3001;
    pc_only_data player = {};
    player.pid = 77;
    player.wallet_revision = 5;
    char_data actor = {};
    actor.only.pc = &player;
    GET_COPPER(&actor) = 100;
    currency_vector wallet = {};
    wallet.amount[0] = 125;
    assert(currency_transaction_publish_wallet(&actor, wallet, 7));
    assert(currency_transaction_publish_wallet(&actor, wallet, 7));
    wallet.amount[0] = 100;
    assert(currency_transaction_publish_wallet(&actor, wallet, 6));
    assert(GET_COPPER(&actor) == 125 && deltas == std::vector<int64_t>{25});
    assert(currency_transaction_publish_wallet(&actor, wallet, 8));
    assert((deltas == std::vector<int64_t>{25, -25}));
}
'''
        with tempfile.TemporaryDirectory(prefix="wallet-audit-") as directory:
            source = Path(directory) / "regression.cpp"
            binary = Path(directory) / "regression"
            source.write_text(harness)
            subprocess.run([
                "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Isrc",
                "-I/usr/include/mysql", "-ffunction-sections", "-fdata-sections",
                str(source), rel("currency_transaction.c"), "-Wl,--gc-sections",
                "-o", str(binary),
            ], cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True)

    def test_open_world_classification_fallback_contract(self):
        """Verify UNCLASSIFIED_INFLOW and UNCLASSIFIED_OUTFLOW exist for novel/unmapped mechanics."""
        outbox_h = (SRC / "economy/economic_event_outbox.h").read_text()
        outbox_c = (SRC / "economy/economic_event_outbox.c").read_text()

        self.assertIn("UNCLASSIFIED_INFLOW", outbox_h)
        self.assertIn("UNCLASSIFIED_OUTFLOW", outbox_h)
        self.assertIn("action == EconomicAction::UNKNOWN", outbox_c)
        self.assertIn("action = EconomicAction::UNCLASSIFIED_INFLOW", outbox_c)
        self.assertIn("action = EconomicAction::UNCLASSIFIED_OUTFLOW", outbox_c)
        self.assertIn("std::atomic<bool> table_initialized", outbox_c)
        self.assertIn("id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY", outbox_c)


if __name__ == "__main__":
    unittest.main()
