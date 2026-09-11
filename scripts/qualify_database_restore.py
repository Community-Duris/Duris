#!/usr/bin/env python3
"""Complete migration-history and aggregate value-domain restore verification."""
import os
import sys
import migration_runner as migrations


def main():
    if os.environ.get("DB_NAME") != "duris_restore" or not os.environ.get("DB_SOCKET"):
        raise RuntimeError("isolated_restore_connection_required")
    manifest = migrations.load_manifest()
    executor = migrations.MysqlExecutor(manifest)
    executor.command.insert(1, "--no-defaults")
    executor.require_baseline(manifest)
    if migrations.validate_applied_prefix(manifest, executor.applied()):
        raise RuntimeError("restore_migration_history_incomplete")
    queries = [
        "SELECT COUNT(*) FROM account_characters c LEFT JOIN accounts a ON a.account_name=c.account_name "
        "LEFT JOIN player_data p ON p.pid=c.pid WHERE c.deleted_at IS NULL AND "
        "(a.account_name IS NULL OR p.pid IS NULL OR (p.account_name IS NOT NULL AND p.account_name<>c.account_name));",
        "SELECT COUNT(*) FROM player_data p LEFT JOIN currency_wallet_baseline b ON b.pid=p.pid WHERE b.pid IS NULL;",
        "SELECT COUNT(*) FROM account_banks a LEFT JOIN currency_bank_baseline b ON b.bank_id=a.id WHERE b.bank_id IS NULL;",
        "SELECT COUNT(*) FROM player_data p JOIN currency_wallet_baseline b ON b.pid=p.pid "
        "LEFT JOIN (SELECT pid,SUM(wallet_delta_copper) c,SUM(wallet_delta_silver) s,"
        "SUM(wallet_delta_gold) g,SUM(wallet_delta_platinum) pl FROM currency_ledger GROUP BY pid) l "
        "ON l.pid=p.pid WHERE b.opening_copper+COALESCE(l.c,0)<>p.copper OR "
        "b.opening_silver+COALESCE(l.s,0)<>p.silver OR b.opening_gold+COALESCE(l.g,0)<>p.gold OR "
        "b.opening_platinum+COALESCE(l.pl,0)<>p.platinum;",
        "SELECT COUNT(*) FROM account_banks a JOIN currency_bank_baseline b ON b.bank_id=a.id "
        "LEFT JOIN (SELECT bank_id,SUM(bank_delta_copper) c,SUM(bank_delta_silver) s,"
        "SUM(bank_delta_gold) g,SUM(bank_delta_platinum) pl FROM currency_ledger GROUP BY bank_id) l "
        "ON l.bank_id=a.id WHERE b.opening_copper+COALESCE(l.c,0)<>a.bank_copper OR "
        "b.opening_silver+COALESCE(l.s,0)<>a.bank_silver OR b.opening_gold+COALESCE(l.g,0)<>a.bank_gold OR "
        "b.opening_platinum+COALESCE(l.pl,0)<>a.bank_platinum;",
        "SELECT COUNT(*) FROM player_data p LEFT JOIN epic_balance_baseline b ON b.pid=p.pid WHERE b.pid IS NULL;",
        "SELECT COUNT(*) FROM player_data p JOIN epic_balance_baseline b ON b.pid=p.pid "
        "LEFT JOIN (SELECT pid,SUM(delta) delta FROM epic_ledger GROUP BY pid) l ON l.pid=p.pid "
        "WHERE b.opening_balance+COALESCE(l.delta,0)<>p.epics;",
        "SELECT COUNT(*) FROM epic_ledger l JOIN player_data p ON p.pid=l.pid "
        "WHERE l.epic_revision=p.epic_revision AND l.balance_after<>p.epics;",
        # This is a conservative generation invariant. It is deliberately
        # separate from persistence_restore.tombstone_preflight(), which
        # validates fresh external evidence and its authority.
        "SELECT COUNT(*) FROM account_erasure_tombstones;",
    ]
    for query in queries:
        if executor.sql(query) != "0":
            raise RuntimeError("restore_reconciliation_failed")
    print('{"history":"ok","reconciliation":"ok"}')


if __name__ == "__main__":
    try:
        main()
    except Exception:
        print("database_restore_qualification_failed", file=sys.stderr)
        raise SystemExit(1)

