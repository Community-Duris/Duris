#!/usr/bin/env python3
"""Source and lightweight runtime contracts for the generic inbox/outbox boundary."""

from _paths import SRC, rel
import subprocess
import tempfile
import shlex
from pathlib import Path
from contract_text import contains


ROOT = Path(__file__).resolve().parents[2]
MIGRATION = (ROOT / "migrations/critical_command_inbox_outbox.sql").read_text()
BOOTSTRAP = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
REPOSITORY = (SRC / "critical_command_repository.c").read_text()
OUTBOX = (SRC / "critical_outbox.c").read_text()
COMM = (SRC / "comm.c").read_text()
COPYOVER = (SRC / "copyover.c").read_text()
DIAGNOSTICS = (SRC / "actinf.c").read_text()
MIGRATION_RUNNER = (ROOT / "migrations/run_migration.sh").read_text()

for table in (
    "critical_operation_inbox",
    "critical_test_state",
    "critical_outbox",
    "critical_outbox_delivery_dedupe",
):
    assert f"CREATE TABLE IF NOT EXISTS {table}" in MIGRATION
    assert f"`{table}`" in BOOTSTRAP
assert "BINARY(16) NOT NULL" in MIGRATION
assert "BINARY(32) NOT NULL" in MIGRATION
assert "payload BLOB NOT NULL" in MIGRATION
assert "UNIQUE KEY uq_critical_outbox_operation_event (operation_id, event_index)" in MIGRATION
assert "KEY idx_critical_outbox_claim (status, next_attempt_at, outbox_id)" in MIGRATION
assert "ENGINE=InnoDB" in MIGRATION
assert "ON UPDATE RESTRICT ON DELETE RESTRICT" in MIGRATION
assert 'run_sql_file "apply critical command inbox and outbox"' in MIGRATION_RUNNER
assert 'run_check "verify critical command inbox and outbox"' in MIGRATION_RUNNER

assert "mysql_stmt_prepare" in REPOSITORY
assert "SHA256(" in REPOSITORY
assert 'execute(connection, "START TRANSACTION")' in REPOSITORY
apply_start = REPOSITORY.index("critical_apply_result critical_command_repository_apply(")
assert REPOSITORY.index("read_operation(connection, command.operation_id, true", apply_start) < REPOSITORY.index(
    "for (const critical_entity_key &key : command.keys)", apply_start
)
assert REPOSITORY.index("insert_inbox(connection") < REPOSITORY.index("execute_entity(connection")
assert REPOSITORY.index("insert_outbox(connection") < REPOSITORY.index('execute(connection, "COMMIT")')
assert REPOSITORY.index("finish_inbox(connection") < REPOSITORY.index('execute(connection, "COMMIT")')
assert "critical_apply_outcome::ambiguous_commit" in REPOSITORY
assert "critical_command_repository_reconcile(connection, command)" in REPOSITORY
# The per-thread setup goes through the shared helper so the client library
# is initialised first; MySQL 8 fails mysql_thread_init() without it.
assert "sql_worker_thread_init()" in REPOSITORY and "mysql_thread_end()" in REPOSITORY
reconcile = REPOSITORY[REPOSITORY.index("critical_apply_result critical_command_repository_reconcile("):
                       REPOSITORY.index("// Public narrow wrappers")]
assert "const bool accounted_bank = accounted_bank_envelope(command);" in reconcile
assert "coin_transfer_accounting_command_supported(command)" in reconcile
assert "item_transfer_accounting_command_supported(command)" in reconcile
assert contains(reconcile, "const bool accounted_root = accounted_bank || accounted_coin || accounted_item || accounted_collector || accounted_shop;")
locked_lookup = "read_operation(connection, command.operation_id, accounted_root"
assert locked_lookup in reconcile
assert reconcile.index('execute(connection, "START TRANSACTION")') < reconcile.index(locked_lookup)
for retained_verifier in (
    "economic_sql_bank_verify_retained(",
    "coin_transfer_accounting_verify_retained(",
    "economic_sql_item_transfer_verify_retained(",
):
    assert locked_lookup in reconcile and retained_verifier in reconcile
    assert reconcile.index(locked_lookup) < reconcile.index(retained_verifier) < reconcile.index(
        "return stored_result("
    )
pooled_apply = REPOSITORY[
    REPOSITORY.index("critical_apply_result critical_command_repository_apply_from_pool("):
    REPOSITORY.index("critical_apply_result critical_command_repository_reconcile(")
]
assert "accounted_coin_envelope(command)" in pooled_apply
assert "coin_transfer_accounting_command_supported(command)" in pooled_apply
assert "item_transfer_accounting_command_supported(command)" in pooled_apply
assert "!accounted_bank_envelope(command) && !accounted_coin && !accounted_item" in pooled_apply
retryable = REPOSITORY[REPOSITORY.index("bool retryable_error(unsigned int error)"):
                       REPOSITORY.index("critical_apply_result failure(unsigned int error)")]
assert all(f"error == {code}" in retryable for code in ("ENOMEM", "1205", "1213"))
assert "EEXIST" in REPOSITORY and "ERANGE" in REPOSITORY
assert "command.payload.data()" not in REPOSITORY
assert "PREPARE " not in REPOSITORY

assert "CRITICAL_OUTBOX_BATCH_MAX = 64" in (SRC / "critical_outbox.h").read_text()
assert "CRITICAL_OUTBOX_RECORD_MAX_BYTES = 65535" in (SRC / "critical_outbox.h").read_text()
assert "CRITICAL_OUTBOX_MAX_ATTEMPTS = 8" in (SRC / "critical_outbox.h").read_text()
assert "ORDER BY next_attempt_at,outbox_id LIMIT 64" in OUTBOX
assert "SELECT outbox_id,operation_id,destination" in OUTBOX
assert "record.operation_id.bytes.data()" in OUTBOX
assert "critical_outbox_delivery_dedupe" in OUTBOX
assert "dead_lettered_at" in OUTBOX
assert "critical_outbox_reconcile" in OUTBOX
assert "critical_outbox_retry_dead_letter" in OUTBOX
assert "sql_pool_replace_connection" in OUTBOX
assert "sql_worker_thread_init()" in OUTBOX and "mysql_thread_end()" in OUTBOX
assert "critical_command_repository_apply_from_pool" in COMM
assert '"integrity_failure"' in COMM and '"operation metadata redacted"' in COMM
assert 'getenv("CRITICAL_COMMAND_JOURNAL_DIR")' in COMM
assert COMM.index("critical_command_coordinator_drain(3000)") < COMM.index(
    "critical_outbox_drain(3000)"
)
assert COPYOVER.index("critical_command_coordinator_drain(3000)") < COPYOVER.index(
    "critical_outbox_drain(3000)"
)
assert "critical_outbox state=%s" in DIAGNOSTICS
assert "committed_without_outbox" in DIAGNOSTICS

HARNESS = r'''
#include "persistence/critical_outbox.h"
#include <cassert>
extern "C" struct st_mysql *sql_pool_acquire(void) { return nullptr; }
extern "C" void sql_pool_release(struct st_mysql *) {}
extern "C" struct st_mysql *sql_pool_replace_connection(struct st_mysql *) { return nullptr; }
int main()
{
    critical_outbox_record valid = {};
    valid.outbox_id = 1;
    valid.operation_id.bytes[0] = 1;
    valid.destination = 1;
    valid.event_type = 1;
    valid.payload_version = 1;
    valid.payload.resize(16);
    assert(critical_outbox_test_destination(valid, nullptr) ==
           critical_outbox_delivery_result::delivered);
    valid.payload.push_back(0);
    assert(critical_outbox_test_destination(valid, nullptr) ==
           critical_outbox_delivery_result::terminal_failure);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="duris-critical-outbox-") as temporary:
    source = Path(temporary) / "test.cpp"
    binary = Path(temporary) / "test"
    source.write_text(HARNESS)
    mysql_flags = shlex.split(
        subprocess.check_output(["mysql_config", "--cflags", "--libs"], text=True)
    )
    subprocess.run(
        [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-pthread", "-Isrc", str(source), rel("critical_outbox.c"),
            rel("corpse_lifecycle_command.c"), rel("item_transfer_command.c"), rel("craft_pouch_mutation.c"), rel("chaos_pouch_ledger.c"), rel("player_snapshot_codec.c"),
            rel("critical_command.c"), "-lcrypto",
            "-o", str(binary),
        ] + mysql_flags,
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("critical transaction schema, repository, outbox, lifecycle, and redaction contracts passed")
