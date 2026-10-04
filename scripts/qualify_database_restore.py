#!/usr/bin/env python3
"""Complete migration-history and aggregate value-domain restore verification."""
import os
import sys
import migration_runner as migrations
import economic_restore_evidence


def require_completed_history(rows):
    """Accept only complete immutable histories supported by the boot gate."""
    manifests = (
        migrations.load_manifest(),
        migrations.load_manifest(
            migrations.ROOT / "migrations/migration_manifest.staging_0045.json"),
        migrations.load_manifest(
            migrations.ROOT / "migrations/migration_manifest.master_0031.json"),
    )
    for manifest in manifests:
        try:
            pending = migrations.validate_applied_prefix(manifest, rows)
        except migrations.MigrationContractError:
            continue
        if not pending:
            return
    raise RuntimeError("restore_migration_history_incomplete_or_unknown")


def require_epic_revision_history(executor):
    """A conserved balance must also have every post-opening native revision."""
    query = (
        "SELECT COUNT(*) FROM player_data p JOIN epic_balance_baseline b ON b.pid=p.pid "
        "LEFT JOIN (SELECT b.pid,COUNT(l.operation_id) event_count,MAX(l.epic_revision) last_revision "
        "FROM epic_balance_baseline b LEFT JOIN epic_ledger l ON l.pid=b.pid "
        "AND l.epic_revision>b.opening_revision GROUP BY b.pid) h ON h.pid=p.pid "
        "WHERE p.epic_revision<>COALESCE(h.last_revision,b.opening_revision) OR "
        "CAST(p.epic_revision AS DECIMAL(20,0))-CAST(b.opening_revision AS DECIMAL(20,0))"
        "<>h.event_count;"
    )
    if executor.sql(query) != "0":
        raise RuntimeError("restore_epic_revision_history_mismatch")


def currency_history_events(ledger_identity, revision, kind_tag, account_kind, values=False):
    """Select immutable native/economic witnesses without inventing money legs."""
    denominations = ("copper", "silver", "gold", "platinum")
    prefix = "wallet" if ledger_identity == "pid" else "bank"
    native_values = economic_values = changed_values = ""
    if values:
        for coin in denominations:
            native_values += (
                f",CAST(l.{prefix}_after_{coin} AS DECIMAL(65,0))-"
                f"CAST(l.{prefix}_delta_{coin} AS DECIMAL(65,0)) before_{coin},"
                f"CAST(l.{prefix}_after_{coin} AS DECIMAL(65,0)) after_{coin}"
            )
            economic_values += (
                f",CAST(e.before_{coin} AS DECIMAL(65,0)),"
                f"CAST(e.after_{coin} AS DECIMAL(65,0))"
            )
            changed_values += f" OR e.before_{coin}<>e.after_{coin}"
    account_key = (
        "CONCAT(m.lineage,UNHEX('" + kind_tag + "'),"
        "REVERSE(UNHEX(LPAD(HEX(m.mapping_id),16,'0'))),"
        "REVERSE(UNHEX(LPAD(HEX(m.context_id),16,'0'))),UNHEX('00000000'))"
    )
    events = (
        f"SELECT l.{ledger_identity} native_id,l.{revision} revision,"
        f"CAST(l.{revision} AS DECIMAL(20,0))-1 before_revision,"
        "COALESCE(c.operation_id,l.operation_id) event_root" + native_values + " "
        "FROM currency_ledger l JOIN critical_operation_inbox i ON i.operation_id=l.operation_id "
        "LEFT JOIN economic_accounting_child c ON c.child_operation_id=l.operation_id "
        "AND c.receipt_operation_id=l.operation_id AND c.relationship=1 "
        "WHERE i.status=1 AND i.result_code=0 AND i.failure_stage=0 AND i.committed_at IS NOT NULL "
        "UNION SELECT m.native_id,e.after_revision revision,e.before_revision,o.operation_id event_root" + economic_values + " "
        "FROM economic_account_mapping m "
        "JOIN economic_accounting_account_effect e ON e.account_key=" + account_key +
        " JOIN economic_accounting_operation o ON o.operation_id=e.operation_id AND o.lineage=m.lineage "
        "JOIN critical_operation_inbox i ON i.operation_id=o.operation_id "
        "WHERE m.backend_kind=1 AND m.account_kind=" + str(account_kind) +
        " AND m.active_native_id=m.native_id AND m.retiring_operation_id IS NULL "
        "AND o.outcome=1 AND o.result_code=0 AND o.reason<>38 "
        "AND (e.after_revision<>e.before_revision" + changed_values + ") "
        "AND i.status=1 AND i.result_code=0 AND i.failure_stage=0 AND i.committed_at IS NOT NULL"
    )
    return events


def require_currency_revision_history(executor):
    """Require every wallet/bank revision from native or accounted effects.

    Accounted owners such as shops can advance native authority without a
    legacy currency row. Only the same root and one-step transition may share
    a native revision; unrelated operations are distinct evidence, not retries.
    """
    for table, identity, baseline, ledger_identity, revision, kind_tag in (
            ("player_data", "pid", "currency_wallet_baseline", "pid", "wallet_revision", "01000100"),
            ("account_banks", "id", "currency_bank_baseline", "bank_id", "bank_revision", "01000200")):
        events = currency_history_events(
            ledger_identity, revision, kind_tag, 1 if table == "player_data" else 2)
        revisions = (
            "SELECT native_id,revision,MAX(before_revision) before_revision,COUNT(*) witnesses "
            f"FROM ({events}) raw_events GROUP BY native_id,revision"
        )
        baseline_identity = ledger_identity
        query = (
            f"SELECT COUNT(*) FROM {table} n JOIN {baseline} b ON b.{baseline_identity}=n.{identity} "
            f"LEFT JOIN (SELECT b.{baseline_identity} native_id,COUNT(e.revision) event_count,"
            "SUM(CASE WHEN CAST(e.revision AS DECIMAL(20,0))-"
            "CAST(e.before_revision AS DECIMAL(20,0))<>1 OR e.witnesses<>1 "
            "THEN 1 ELSE 0 END) invalid_steps,"
            f"MAX(e.revision) last_revision FROM {baseline} b LEFT JOIN ({revisions}) e "
            f"ON e.native_id=b.{baseline_identity} AND e.revision>b.opening_revision "
            f"GROUP BY b.{baseline_identity}) h ON h.native_id=n.{identity} "
            f"WHERE n.{revision}<>COALESCE(h.last_revision,b.opening_revision) OR "
            f"CAST(n.{revision} AS DECIMAL(20,0))-CAST(b.opening_revision AS DECIMAL(20,0))"
            "<>h.event_count OR h.invalid_steps<>0;"
        )
        if executor.sql(query) != "0":
            raise RuntimeError("restore_currency_revision_history_mismatch")


def require_currency_values(executor):
    """Reconcile before/after value chains after complete revision qualification.

    Native and economic bridges share a witness only when root, revisions and
    all denomination vectors agree. Wide decimal arithmetic never substitutes
    an estimated price or a synthetic currency posting for the actual vectors.
    """
    coins = ("copper", "silver", "gold", "platinum")
    maximum = 9223372036854775807
    for table, identity, baseline, ledger_identity, revision, kind_tag, native_prefix in (
            ("player_data", "pid", "currency_wallet_baseline", "pid", "wallet_revision", "01000100", ""),
            ("account_banks", "id", "currency_bank_baseline", "bank_id", "bank_revision", "01000200", "bank_")):
        events = currency_history_events(
            ledger_identity, revision, kind_tag, 1 if table == "player_data" else 2, values=True)
        vectors = ",".join(f"MIN({side}_{coin}) {side}_{coin}"
                           for coin in coins for side in ("before", "after"))
        grouped = ("SELECT native_id,revision,MIN(before_revision) before_revision,"
                   f"COUNT(*) witnesses,{vectors} FROM ({events}) raw_events GROUP BY native_id,revision")
        opening_vectors = ",".join(f"CAST(b.opening_{coin} AS DECIMAL(65,0)) {side}_{coin}"
                                   for coin in coins for side in ("before", "after"))
        event_vectors = ",".join(f"e.{side}_{coin}" for coin in coins for side in ("before", "after"))
        history = (
            f"SELECT b.{ledger_identity} native_id,b.opening_revision revision,"
            f"b.opening_revision before_revision,1 witnesses,0 is_event,{opening_vectors} FROM {baseline} b "
            "UNION ALL SELECT e.native_id,e.revision,e.before_revision,e.witnesses,1 is_event,"
            f"{event_vectors} FROM ({grouped}) e JOIN {baseline} b "
            f"ON b.{ledger_identity}=e.native_id AND e.revision>b.opening_revision"
        )
        prior = ",".join(f"LAG(after_{coin}) OVER(PARTITION BY native_id ORDER BY revision) prior_{coin}"
                         for coin in coins)
        sequenced = f"SELECT h.*,{prior} FROM ({history}) h"
        disagreements = " OR ".join(f"s.before_{coin}<>s.prior_{coin}" for coin in coins)
        closing = " OR ".join(f"s.after_{coin}<>n.{native_prefix}{coin}" for coin in coins)
        invalid = [f"n.{native_prefix}{coin} IS NULL" for coin in coins]
        for side in ("before", "after"):
            invalid += [f"s.{side}_{coin} IS NULL OR s.{side}_{coin}<0" for coin in coins]
            total = "+".join(f"s.{side}_{coin}*{weight}" for coin, weight in zip(coins, (1, 10, 100, 1000)))
            invalid.append(f"({total})>{maximum}")
        query = (
            f"SELECT COUNT(*) FROM {table} n JOIN ({sequenced}) s ON s.native_id=n.{identity} "
            "WHERE s.witnesses<>1 OR (s.is_event=1 AND ("
            "CAST(s.revision AS DECIMAL(20,0))-CAST(s.before_revision AS DECIMAL(20,0))<>1 OR "
            f"{disagreements})) OR (s.revision=n.{revision} AND ({closing})) OR "
            + " OR ".join(invalid) + ";"
        )
        if executor.sql(query) != "0":
            raise RuntimeError("restore_currency_value_mismatch")


def require_economic_coin_effect_integrity(executor):
    """Validate retained coin witnesses without giving system books balances."""
    coins = ("copper", "silver", "gold", "platinum")
    ordinary = "SUBSTRING(e.account_key,19,2) IN (" + ",".join(
        f"UNHEX('{kind:02x}00')" for kind in (1, 2, 3, 4, 5, 6, 11)) + ")"
    zero = lambda size: f"UNHEX(REPEAT('00',{size}))"
    checks = [("SELECT COUNT(*) FROM economic_accounting_account_effect e "
               "JOIN economic_accounting_operation o ON o.operation_id=e.operation_id "
               "WHERE OCTET_LENGTH(e.account_key)<>40 OR SUBSTRING(e.account_key,1,16)<>o.lineage "
               f"OR SUBSTRING(e.account_key,1,16)={zero(16)} "
               "OR SUBSTRING(e.account_key,17,2)<>UNHEX('0100') "
               "OR SUBSTRING(e.account_key,19,2) NOT IN (" + ",".join(
                   f"UNHEX('{kind:02x}00')" for kind in range(1, 12)) + ") "
               f"OR SUBSTRING(e.account_key,21,8)={zero(8)} "
               f"OR SUBSTRING(e.account_key,37,4)<>{zero(4)};",
               "restore_economic_account_key_mismatch")]
    # Canonical keys order by numeric kind, authority ID and context, not the
    # little-endian byte order of their persisted representation.
    def key_order(alias):
        return "(" + ",".join(
            f"CAST(CONV(HEX(REVERSE(SUBSTRING({alias}.account_key,{offset},{size}))),16,10) "
            "AS DECIMAL(20,0))" for offset, size in ((19, 2), (21, 8), (29, 8))) + ")"
    checks.append((
        "SELECT COUNT(*) FROM economic_accounting_account_effect e "
        "JOIN economic_accounting_account_effect prior ON prior.operation_id=e.operation_id "
        "AND prior.account_index+1=e.account_index WHERE " + key_order("prior") + ">=" + key_order("e") + ";",
        "restore_economic_account_key_mismatch"))
    weighted = "+".join(f"CAST(p.delta_{coin} AS DECIMAL(65,0))*{weight}"
                        for coin, weight in zip(coins, (1, 10, 100, 1000)))
    checks.extend((
        ("SELECT COUNT(*) FROM economic_accounting_coin_posting p WHERE p.event_index<>p.line_index OR "
         "(" + " AND ".join(f"p.delta_{coin}=0" for coin in coins) + ") "
         f"OR ({weighted})<>CAST(p.copper_value AS DECIMAL(65,0));",
         "restore_economic_posting_value_mismatch"),
        ("SELECT COUNT(*) FROM (SELECT operation_id,SUM(CAST(copper_value AS DECIMAL(65,0))) total "
         "FROM economic_accounting_coin_posting GROUP BY operation_id) p WHERE p.total<>0;",
         "restore_economic_root_unbalanced"),
    ))
    same = " AND ".join(f"e.before_{coin}=e.after_{coin}" for coin in coins)
    invalid_holding = " OR ".join(f"e.{side}_{coin}<0" for side in ("before", "after") for coin in coins)
    for side in ("before", "after"):
        total = "+".join(f"CAST(e.{side}_{coin} AS DECIMAL(65,0))*{weight}"
                         for coin, weight in zip(coins, (1, 10, 100, 1000)))
        invalid_holding += f" OR ({total})>9223372036854775807"
    system_zero = " AND ".join(f"e.{side}_{coin}=0" for side in ("before", "after") for coin in coins)
    checks.append((
        "SELECT COUNT(*) FROM economic_accounting_account_effect e WHERE "
        f"({ordinary} AND ({invalid_holding} OR e.after_revision<e.before_revision "
        f"OR (NOT ({same}) AND e.after_revision=e.before_revision))) "
        f"OR (NOT ({ordinary}) AND (NOT ({system_zero}) OR e.before_revision<>0 OR e.after_revision<>0));",
        "restore_economic_account_witness_mismatch"))
    summed = ",".join(f"SUM(CAST(delta_{coin} AS DECIMAL(65,0))) delta_{coin}" for coin in coins)
    mismatch = " OR ".join(
        f"CAST(e.after_{coin} AS DECIMAL(65,0))-CAST(e.before_{coin} AS DECIMAL(65,0))"
        f"<>COALESCE(p.delta_{coin},0)" for coin in coins)
    checks.append((
        "SELECT COUNT(*) FROM economic_accounting_account_effect e LEFT JOIN "
        f"(SELECT operation_id,account_index,COUNT(*) n,{summed} FROM economic_accounting_coin_posting "
        "GROUP BY operation_id,account_index) p ON p.operation_id=e.operation_id "
        f"AND p.account_index=e.account_index WHERE ({ordinary} AND ({mismatch})) "
        f"OR (COALESCE(p.n,0)=0 AND (NOT ({ordinary}) OR NOT ({same}) "
        "OR e.after_revision<=e.before_revision));",
        "restore_economic_account_delta_mismatch"))
    for query, code in checks:
        if executor.sql(query) != "0":
            raise RuntimeError(code)


def require_economic_evidence_integrity(executor):
    """Refuse lost or internally inconsistent retained economic evidence.

    This is independent SELECT-only reconciliation across every retained epoch.
    Empty/inactive histories pass. Canonical capsules bind retained SQL evidence;
    locked native custody, publication and allocator authority remain separate.
    """
    checks = []
    for table, count, index, first, family in (
            ("economic_accounting_account_effect", "account_count", "account_index", 0, "account"),
            ("economic_accounting_coin_posting", "posting_count", "line_index", 0, "posting"),
            ("economic_accounting_child", "child_count", "child_index", 1, "child"),
            ("economic_accounting_item_reference", "item_event_count", "line_index", 0, "item")):
        checks.extend((
            (f"SELECT COUNT(*) FROM {table} e LEFT JOIN economic_accounting_operation o "
             "ON o.operation_id=e.operation_id WHERE o.operation_id IS NULL;",
             f"restore_economic_orphan_{family}"),
            ("SELECT COUNT(*) FROM economic_accounting_operation o LEFT JOIN "
             f"(SELECT operation_id,COUNT(*) n FROM {table} GROUP BY operation_id) e "
             f"ON e.operation_id=o.operation_id WHERE o.{count}<>COALESCE(e.n,0);",
             f"restore_economic_{family}_count_mismatch"),
            (f"SELECT COUNT(*) FROM {table} e JOIN economic_accounting_operation o "
             f"ON o.operation_id=e.operation_id WHERE e.{index}<{first} "
             f"OR e.{index}>=o.{count}+{first};",
             f"restore_economic_{family}_index_mismatch"),
        ))
    checks.extend((
        ("SELECT COUNT(*) FROM economic_accounting_operation o LEFT JOIN critical_operation_inbox i "
         "ON i.operation_id=o.operation_id WHERE i.operation_id IS NULL OR i.status<>1 "
         "OR i.result_code<>o.result_code OR i.failure_stage<>0 OR i.committed_at IS NULL "
         "OR NOT ((o.outcome=1 AND o.result_code=0) OR (o.outcome=2 AND o.result_code<>0));",
         "restore_economic_receipt_mismatch"),
        ("SELECT COUNT(*) FROM economic_accounting_operation o LEFT JOIN economic_accounting_source_claim c "
         "ON c.operation_id=o.operation_id AND c.lineage=o.lineage AND c.source_event=o.source_event "
         "AND c.outcome=o.outcome WHERE o.outcome=1 AND o.source_event IS NOT NULL AND c.operation_id IS NULL;",
         "restore_economic_source_claim_missing"),
        ("SELECT COUNT(*) FROM economic_accounting_source_claim c LEFT JOIN economic_accounting_operation o "
         "ON o.operation_id=c.operation_id AND o.lineage=c.lineage AND o.source_event=c.source_event "
         "AND o.outcome=c.outcome WHERE o.operation_id IS NULL OR c.outcome<>1;",
         "restore_economic_source_claim_mismatch"),
        ("SELECT COUNT(*) FROM economic_accounting_coin_posting p LEFT JOIN economic_accounting_account_effect e "
         "ON e.operation_id=p.operation_id AND e.account_index=p.account_index WHERE e.operation_id IS NULL;",
         "restore_economic_posting_account_missing"),
        ("SELECT COUNT(*) FROM economic_accounting_child c LEFT JOIN economic_accounting_child p "
         "ON p.operation_id=c.operation_id AND p.child_index=c.parent_index "
         "LEFT JOIN critical_operation_inbox i ON i.operation_id=c.receipt_operation_id "
         "WHERE c.parent_index>=c.child_index OR (c.parent_index<>0 AND p.operation_id IS NULL) "
         "OR c.child_operation_id=c.operation_id OR c.relationship<>1 OR c.domain_id=0 "
         "OR (c.receipt_operation_id IS NOT NULL AND (c.receipt_operation_id<>c.child_operation_id "
         "OR i.operation_id IS NULL OR i.status<>1 OR i.result_code<>0 OR i.failure_stage<>0 "
         "OR i.committed_at IS NULL));",
         "restore_economic_child_link_mismatch"),
    ))
    for table in ("economic_accounting_coin_posting", "economic_accounting_item_reference"):
        checks.append((
            f"SELECT COUNT(*) FROM {table} e LEFT JOIN economic_accounting_child c "
            "ON c.operation_id=e.operation_id AND c.child_index=e.child_index "
            "WHERE e.child_index<>0 AND c.operation_id IS NULL;",
            "restore_economic_child_evidence_missing"))
    checks.append((
        "SELECT COUNT(*) FROM economic_accounting_item_reference r LEFT JOIN item_ownership_ledger l "
        "ON l.operation_id=r.legacy_operation_id AND l.event_index=r.legacy_event_index "
        "AND l.item_uid=r.item_uid AND l.item_revision=r.after_revision "
        "LEFT JOIN economic_accounting_child c ON c.operation_id=r.operation_id AND c.child_index=r.child_index "
        "LEFT JOIN critical_operation_inbox i ON i.operation_id=r.legacy_operation_id "
        "WHERE l.operation_id IS NULL OR r.item_uid=0 OR r.event_index<>r.line_index "
        "OR CAST(r.after_revision AS DECIMAL(20,0))-CAST(r.before_revision AS DECIMAL(20,0))<>1 "
        "OR r.legacy_operation_id<>IF(r.child_index=0,r.operation_id,c.child_operation_id) "
        "OR i.operation_id IS NULL OR i.status<>1 OR i.result_code<>0 OR i.failure_stage<>0 "
        "OR i.committed_at IS NULL;", "restore_economic_item_link_mismatch"))
    for query, code in checks:
        if executor.sql(query) != "0":
            raise RuntimeError(code)
    require_economic_coin_effect_integrity(executor)
    economic_restore_evidence.require_integrity(executor)


def main():
    if os.environ.get("DB_NAME") != "duris_restore" or not os.environ.get("DB_SOCKET"):
        raise RuntimeError("isolated_restore_connection_required")
    manifest = migrations.load_manifest()
    executor = migrations.MysqlExecutor(manifest)
    try:
        executor.require_baseline(manifest)
        require_completed_history(executor.applied())
        queries = [
            "SELECT COUNT(*) FROM artifact_mana WHERE item_uid=0 OR profile_id=0 OR profile_revision=0 "
            "OR version=0 OR capacity=0 OR capacity>1000000000000 OR regeneration>1000000000 "
            "OR reserve>capacity;",
            "SELECT COUNT(*) FROM account_characters c LEFT JOIN accounts a ON a.account_name=c.account_name "
            "LEFT JOIN player_data p ON p.pid=c.pid WHERE c.deleted_at IS NULL AND "
            "(a.account_name IS NULL OR p.pid IS NULL OR (p.account_name IS NOT NULL AND p.account_name<>c.account_name));",
            "SELECT COUNT(*) FROM player_data p LEFT JOIN currency_wallet_baseline b ON b.pid=p.pid WHERE b.pid IS NULL;",
            "SELECT COUNT(*) FROM account_banks a LEFT JOIN currency_bank_baseline b ON b.bank_id=a.id WHERE b.bank_id IS NULL;",
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
        require_epic_revision_history(executor)
        require_currency_revision_history(executor)
        require_currency_values(executor)
        require_economic_evidence_integrity(executor)
        print('{"history":"ok","reconciliation":"ok"}')
    finally:
        executor.release_lock()


if __name__ == "__main__":
    try:
        main()
    except Exception:
        print("database_restore_qualification_failed", file=sys.stderr)
        raise SystemExit(1)
