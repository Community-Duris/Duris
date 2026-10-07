#!/usr/bin/env python3
"""Native settlement allocations and claim through retained restricted publication.

Only an outer owned disposable qualification command prepares/removes SQL
servers and schemas. This helper never loads .env or creates a database.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import select
import subprocess
import sys
import time
import tracemalloc
import uuid

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from compile_telemetry_canonical_auction import qualify_native_target_guard
from run_economic_sql_bank_transaction_mysql import target_is_disposable


def qualify(executable, destination, *, sources=2):
    import pymysql
    from scripts.telemetry import canonical_reward_contract as c
    from scripts.telemetry.canonical_reward_auction_source import CanonicalAuctionSnapshot
    from scripts.telemetry.canonical_reward_source import RETAINED_SOURCE_TABLES
    from scripts.telemetry.canonical_reward_retention import CanonicalRewardRetention, RETENTION_TABLES, CUT_TABLE
    from scripts.telemetry.canonical_reward_publication import (
        CanonicalRewardPublisher, CanonicalRewardReporter, PRIVATE_TABLES, PUBLIC_TABLES,
        GENERATION_TABLE, COVERAGE_TABLE, PRIVATE_EVENT_TABLE, EVENT_TABLE,
    )
    from scripts.telemetry.reward_projection import AmbiguousCommit, ProjectionBoundsExceeded
    from scripts.telemetry.db_access import ConnectionSettings, PyMySQLConnectionFactory
    from telemetry_canonical_publication_mysql import MeasuredFactory, qualify_query_plans

    if os.name != "posix" or not target_is_disposable(os.environ) or os.environ.get("TEST_DB_DISPOSABLE") != "1":
        raise RuntimeError("native auction qualification requires explicit disposable Linux loopback")
    if sources not in (2, 128, 129):
        raise ValueError("native auction qualification source count")
    qualify_native_target_guard(executable)
    database = os.environ["DB_NAME"]
    common = dict(host="127.0.0.1", port=int(os.environ["DB_PORT"]), database=database, connect_timeout=3,
        read_timeout=3, write_timeout=3, autocommit=True, charset="utf8mb4", cursorclass=pymysql.cursors.DictCursor)
    admin = pymysql.connect(user=os.environ["DB_USER"], password=os.environ["DB_PASSWD"], **common)
    suffix, password = uuid.uuid4().hex[:12], "synthetic_" + uuid.uuid4().hex
    source_user, project_user, report_user = (prefix + suffix for prefix in ("auction_source_", "auction_project_", "auction_report_"))
    roles, process = [], None
    started = time.monotonic()
    tracemalloc.start()
    try:
        with admin.cursor() as cursor:
            for user in (source_user, project_user, report_user):
                cursor.execute("CREATE USER %s@'%%' IDENTIFIED BY %s", (user, password))
                roles.append(user)
            for table in RETAINED_SOURCE_TABLES:
                cursor.execute("GRANT SELECT ON `" + database + "`.`" + table + "` TO %s@'%%'", (source_user,))
            for table in RETENTION_TABLES:
                cursor.execute("GRANT SELECT,INSERT ON `" + database + "`.`" + table + "` TO %s@'%%'", (source_user,))
                cursor.execute("GRANT SELECT ON `" + database + "`.`" + table + "` TO %s@'%%'", (project_user,))
            cursor.execute("GRANT UPDATE(sealed) ON `" + database + "`.`" + CUT_TABLE + "` TO %s@'%%'", (source_user,))
            for table in (*PRIVATE_TABLES, *PUBLIC_TABLES):
                cursor.execute("GRANT SELECT,INSERT ON `" + database + "`.`" + table + "` TO %s@'%%'", (project_user,))
            cursor.execute("GRANT UPDATE(phase) ON `" + database + "`.`" + GENERATION_TABLE + "` TO %s@'%%'", (project_user,))
            cursor.execute("GRANT UPDATE(complete) ON `" + database + "`.`" + COVERAGE_TABLE + "` TO %s@'%%'", (project_user,))
            for table in PUBLIC_TABLES:
                cursor.execute("GRANT SELECT ON `" + database + "`.`" + table + "` TO %s@'%%'", (report_user,))

        def factory(user):
            return MeasuredFactory(PyMySQLConnectionFactory(ConnectionSettings(host="127.0.0.1", port=common["port"],
                database=database, user=user, password=password, read_timeout_s=3, write_timeout_s=3)))

        source, project, reporter = factory(source_user), factory(project_user), factory(report_user)
        cli_environment = dict(os.environ, DB_HOST="invalid-game-host", DB_NAME="invalid-game-database")
        for prefix, user in (("TELEMETRY_REWARD_SOURCE_DB_", source_user),
                             ("TELEMETRY_REWARD_PROJECT_DB_", project_user),
                             ("TELEMETRY_REWARD_REPORT_DB_", report_user)):
            cli_environment.update({prefix + "HOST": "127.0.0.1", prefix + "PORT": os.environ["DB_PORT"],
                prefix + "USER": user, prefix + "PASSWD": password, prefix + "DATABASE": database})
        env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1",
                   TELEMETRY_AUCTION_CLAIM_SOURCES=str(sources))
        process = subprocess.Popen(["bash", "-c", 'ulimit -s 65536 && exec "$1"', "native-auction", str(executable)],
            cwd=ROOT, env=env, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        output, buffer = bytearray(), bytearray()
        deadline, checkpoint = time.monotonic() + 60, None
        while time.monotonic() < deadline and checkpoint is None:
            ready, _, _ = select.select([process.stdout], [], [], min(1, max(0, deadline - time.monotonic())))
            if not ready:
                if process.poll() is not None:
                    break
                continue
            block = os.read(process.stdout.fileno(), 4096)
            if not block:
                break
            output.extend(block); buffer.extend(block)
            if len(output) > 65536:
                raise AssertionError("native auction output bound")
            while b"\n" in buffer:
                line, _, buffer = buffer.partition(b"\n")
                if line.startswith(b"TELEMETRY_AUCTION_READY "):
                    checkpoint = tuple(bytes.fromhex(part.decode("ascii")) for part in line.split()[1:])
        if checkpoint is None or len(checkpoint) != sources + 1 or \
                len(set(checkpoint)) != sources + 1 or any(len(op) != 16 for op in checkpoint):
            raise AssertionError("native auction checkpoint missing: " + output[-4000:].decode(errors="replace"))
        claim, sale_ids = checkpoint[0], checkpoint[1:]
        changed_sale = sale_ids[-1]
        custody_copper = 2910 * sources + (90 if sources > 2 else 0)
        from scripts.telemetry.canonical_reward_auction import ALLOCATION_COLUMNS
        allocation_plans = qualify_query_plans(source, (("claim_allocations",
            "SELECT " + ",".join(ALLOCATION_COLUMNS) + " FROM economic_pending_claim_source FORCE INDEX (idx_economic_pending_claim_consumed) "
            "WHERE claim_operation_id=%s ORDER BY source_operation_id,source_slot LIMIT %s", (claim, 129)),))
        allocation_access = allocation_plans["claim_allocations"]
        if len(allocation_access) != 1 or allocation_access[0]["selected_key"] != "idx_economic_pending_claim_consumed" or \
                allocation_access[0]["access"] not in ("ref", "range", "const"):
            raise AssertionError("native claim allocation plan did not bound access to its claim index")
        cli_capture = subprocess.run([sys.executable, "-m", "scripts.telemetry.canonical_reward", "capture",
            "--route", "auction", "--operation", claim.hex()], cwd=ROOT, env=cli_environment,
            capture_output=True, text=True, timeout=30)

        def finish_native():
            process.stdin.write(b"continue\n"); process.stdin.flush()
            tail, _ = process.communicate(timeout=30)
            output.extend(tail)
            if process.returncode or b"AddressSanitizer" in output or b"runtime error:" in output:
                raise AssertionError("native auction fixture/sanitizer failed: " + output[-4000:].decode(errors="replace"))

        if sources == 129:
            if cli_capture.returncode != 1 or json.loads(cli_capture.stderr).get("error") != "canonical_auction_allocation_capacity":
                raise AssertionError("actual capture CLI did not refuse oversized native claim")
            snapshot = CanonicalAuctionSnapshot(source)
            try:
                snapshot.capture_money_claims((claim,), preserve_refusals=True)
            except c.EvidenceError as error:
                if str(error) != "canonical_auction_allocation_capacity":
                    raise
            else:
                raise AssertionError("129-source native claim exceeded projection capacity")
            if source.metrics["selects"] != 4 or source.metrics["writes"] or snapshot._sources:
                raise AssertionError("over-capacity claim fetched full receipts or retained partial evidence")
            with admin.cursor() as cursor:
                for table in (*RETENTION_TABLES, *PRIVATE_TABLES, *PUBLIC_TABLES):
                    cursor.execute("SELECT COUNT(*) AS count FROM " + table)
                    if cursor.fetchone()["count"]:
                        raise AssertionError("over-capacity claim published partial telemetry state")
            finish_native()
            _, peak = tracemalloc.get_traced_memory()
            result = dict(status="passed", native_owner="typed_settlement_to_money_claim", selected_claims=1,
                native_upstream_sales=sources, native_custody_copper=custody_copper,
                native_zero_fee_sales=1, projection_capacity_refused=True,
                refusal="canonical_auction_allocation_capacity", preflight_selects=4,
                partial_retention_or_publication=False, native_target_guard_refusals=6,
                actual_capture_cli_refused=True,
                source_query_plans=allocation_plans,
                claim_allocation_index_qualified=True,
                source_metrics=source.metrics, peak_traced_python_bytes=peak,
                elapsed_seconds=time.monotonic() - started,
                native_binary_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
                production_or_staging_access=False)
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
            print(json.dumps(result, sort_keys=True))
            return

        cuts = [CanonicalAuctionSnapshot(source).capture_money_claims((claim,)) for _ in range(2)]
        expected_rows = 9 + sources * 10 - (2 if sources > 2 else 0)
        for cut in cuts:
            if cut.query_counts != (8,) or len(cut.sources) != expected_rows or len(cut.events) != 1 or \
                cut.events[0].disposition != c.Disposition.TRANSFER or cut.events[0].amount != custody_copper or c.earned_totals(cut.events):
                raise AssertionError("native auction canonical source contract")
            CanonicalRewardRetention(source).retain(cut)
            restored = CanonicalRewardRetention(source).load(cut.source_digest)
            if restored.sources != cut.sources or restored.events != cut.events:
                raise AssertionError("native auction exact retained replay")
        if cli_capture.returncode:
            raise AssertionError("actual native capture CLI failed: " + cli_capture.stderr[-2000:])
        cli_cut = json.loads(cli_capture.stdout)
        restored_cli = CanonicalRewardRetention(source).load(bytes.fromhex(cli_cut["cut_id"]))
        if restored_cli.sources != cuts[0].sources or restored_cli.events != cuts[0].events or \
                cli_cut["source_rows"] != expected_rows or cli_cut["source_queries"] != [8]:
            raise AssertionError("actual capture CLI changed native authority or retained source")
        selection = tuple(sorted({cut.source_digest for cut in cuts}))
        reference = cuts[0].sources[0].reference
        from scripts.telemetry.canonical_reward_retention import SOURCE_TABLE
        conflict_plans = qualify_query_plans(project, (("retained_variants",
            "SELECT s.cut_id,s.source_index,s.payload,s.payload_digest FROM " + SOURCE_TABLE +
            " s FORCE INDEX (idx_reward2_source_payloads) JOIN " + CUT_TABLE + " c ON c.cut_id=s.cut_id "
            "WHERE s.source_table=%s AND s.source_key=%s AND s.payload_digest>%s AND s.payload_digest<>%s "
            "AND c.sealed=1 AND c.definition_version=2 ORDER BY s.payload_digest,s.cut_id LIMIT 1",
            (reference.table.encode("ascii"), reference.key, bytes(32), reference.payload_digest)),))
        gid = uuid.uuid4().bytes
        class InterruptedStage(CanonicalRewardPublisher):
            def _bulk(self, table, columns, rows):
                super()._bulk(table, columns, rows)
                if table == PRIVATE_EVENT_TABLE:
                    raise RuntimeError("injected auction staging interruption")

        try:
            InterruptedStage(project).stage(gid, selection)
        except RuntimeError as error:
            if str(error) != "injected auction staging interruption":
                raise
        else:
            raise AssertionError("auction staging interruption not reached")
        with admin.cursor() as cursor:
            cursor.execute("SELECT COUNT(*) AS count FROM " + GENERATION_TABLE + " WHERE generation_id=%s", (gid,))
            if cursor.fetchone()["count"]:
                raise AssertionError("interrupted auction staging left partial generation")

        class LostReplyFactory:
            def connect(self):
                connection = project.connect()
                original = connection.commit
                def commit():
                    original()
                    raise OSError("injected auction lost commit reply")
                connection.commit = commit
                return connection

            def close(self, connection=None):
                project.close(connection)

        try:
            CanonicalRewardPublisher(LostReplyFactory()).stage(gid, selection)
        except AmbiguousCommit:
            pass
        else:
            raise AssertionError("auction lost staging reply did not require reconciliation")
        original_plan = CanonicalRewardPublisher(project).stage(gid, selection)

        class InterruptedPublish(CanonicalRewardPublisher):
            def _bulk(self, table, columns, rows):
                super()._bulk(table, columns, rows)
                if table == EVENT_TABLE:
                    try:
                        CanonicalRewardReporter(reporter).read(gid)
                    except c.EvidenceError:
                        pass
                    else:
                        raise AssertionError("independent reader saw incomplete auction publication")
                    raise RuntimeError("injected auction publication interruption")

        try:
            InterruptedPublish(project).publish(gid)
        except RuntimeError as error:
            if str(error) != "injected auction publication interruption":
                raise
        else:
            raise AssertionError("auction publication interruption not reached")
        try:
            CanonicalRewardPublisher(LostReplyFactory()).publish(gid)
        except AmbiguousCommit:
            pass
        else:
            raise AssertionError("auction lost publication reply did not require reconciliation")
        CanonicalRewardPublisher(project).publish(gid)
        original_report = CanonicalRewardReporter(reporter).read(gid)
        if original_report["coverage"]["transfer_count"] != 1 or original_report["coverage"]["currency_earned_copper"] is not None:
            raise AssertionError("auction overlap inflated earnings or custody")
        with admin.cursor() as cursor:
            cursor.execute("UPDATE economic_pending_claim_source SET amount=amount+1 WHERE source_operation_id=%s AND source_slot=2", (changed_sale,))
        try:
            try:
                CanonicalAuctionSnapshot(source).capture_money_claims((claim,))
            except c.EvidenceError:
                pass
            else:
                raise AssertionError("changed second native auction allocation retained authority")
            refused = CanonicalAuctionSnapshot(source).capture_money_claims((claim,), preserve_refusals=True)
            if refused.events[0].disposition != c.Disposition.UNKNOWN or refused.events[0].amount is not None:
                raise AssertionError("native auction dependency quarantine")
            CanonicalRewardRetention(source).retain(refused)
            conflict_id = uuid.uuid4().bytes
            plan = CanonicalRewardPublisher(project).stage(conflict_id, (cuts[0].source_digest,))
            if len(plan.events) != 1 or plan.events[0].disposition != c.Disposition.CONFLICT or plan.events[0].amount is not None:
                raise AssertionError("global allocation conflict did not reach original claim")
            CanonicalRewardPublisher(project).publish(conflict_id)
            if CanonicalRewardReporter(reporter).read(gid) != original_report:
                raise AssertionError("later auction dependency conflict changed old publication")
        finally:
            with admin.cursor() as cursor:
                cursor.execute("UPDATE economic_pending_claim_source SET amount=amount-1 WHERE source_operation_id=%s AND source_slot=2", (changed_sale,))
        for user, sql in ((source_user, "UPDATE economic_pending_claim_source SET amount=amount WHERE 0"),
                          (project_user, "SELECT operation_id FROM economic_accounting_operation LIMIT 1"),
                          (report_user, "SELECT cut_id FROM " + CUT_TABLE + " LIMIT 1"),
                          (report_user, "UPDATE " + EVENT_TABLE + " SET event_index=event_index WHERE 0")):
            restricted = pymysql.connect(user=user, password=password, **common)
            try:
                with restricted.cursor() as cursor:
                    cursor.execute(sql)
            except pymysql.MySQLError as error:
                if error.args[0] != 1142:
                    raise
            else:
                raise AssertionError("auction principal exceeded its authority")
            finally:
                restricted.close()
        for mode in ("stage", "publish", "report"):
            arguments = [sys.executable, "-m", "scripts.telemetry.canonical_reward", mode, "--generation", gid.hex()]
            if mode == "stage":
                for cut_id in selection:
                    arguments.extend(("--cut", cut_id.hex()))
            subprocess.run(arguments, cwd=ROOT, env=cli_environment, check=True, timeout=30,
                           stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        finish_native()
        # Qualification-only pruning in the guarded owned database. Keep critical
        # inbox fences referenced by native mapping tombstones; never disable FKs.
        native_ids = (claim, *sale_ids)
        markers = ",".join("%s" for _ in native_ids)
        removed_rows = 0
        admin.begin()
        try:
            with admin.cursor() as cursor:
                cursor.execute("DELETE FROM economic_pending_claim_source WHERE claim_operation_id=%s", (claim,))
                removed_rows += cursor.rowcount
                for table in ("economic_accounting_source_claim", "auction_ledger", "currency_ledger",
                              "economic_accounting_coin_posting", "economic_accounting_account_effect",
                              "economic_accounting_operation"):
                    cursor.execute("DELETE FROM " + table + " WHERE operation_id IN (" + markers + ")", native_ids)
                    removed_rows += cursor.rowcount
                cursor.execute("SELECT COUNT(*) AS count FROM economic_accounting_operation "
                               "WHERE operation_id IN (" + markers + ")", native_ids)
                if cursor.fetchone()["count"] != 0 or removed_rows != len(cuts[0].sources) - len(native_ids):
                    raise AssertionError("auction fixture prune lost exact native source scope")
                cursor.execute("SELECT COUNT(*) AS count FROM critical_operation_inbox "
                               "WHERE operation_id IN (" + markers + ")", native_ids)
                if cursor.fetchone()["count"] != len(native_ids):
                    raise AssertionError("auction fixture prune changed protected recovery fences")
            admin.commit()
        except Exception:
            admin.rollback()
            raise
        try:
            CanonicalAuctionSnapshot(source).capture_money_claims((claim,))
        except c.EvidenceError as error:
            if str(error) != "canonical_auction_selected_root_missing":
                raise
        else:
            raise AssertionError("auction source capture invented pruned native roots")
        restored = CanonicalRewardRetention(source).load(cuts[0].source_digest)
        if restored.sources != cuts[0].sources or restored.events != cuts[0].events:
            raise AssertionError("auction exact retained replay depended on pruned native roots")
        after_prune_id = uuid.uuid4().bytes
        after_prune_selection = tuple(sorted((*selection, refused.source_digest)))
        three_cut_budget_refused = False
        if sources == 128:
            try:
                CanonicalRewardPublisher(project).stage(after_prune_id, after_prune_selection)
            except ProjectionBoundsExceeded:
                three_cut_budget_refused = True
            else:
                raise AssertionError("three full maximum-source cuts exceeded generation budget")
            with admin.cursor() as cursor:
                cursor.execute("SELECT COUNT(*) AS count FROM " + GENERATION_TABLE + " WHERE generation_id=%s", (after_prune_id,))
                if cursor.fetchone()["count"]:
                    raise AssertionError("over-budget auction selection left a partial generation")
            # Two full maximum-source cuts fit the unchanged generation budget.
            # The earlier two-cut generation already proves exact overlap.
            after_prune_selection = tuple(sorted((cuts[0].source_digest, refused.source_digest)))
        after_prune = CanonicalRewardPublisher(project).stage(after_prune_id, after_prune_selection)
        if len(after_prune.events) != 1 or after_prune.events[0].disposition != c.Disposition.CONFLICT or after_prune.events[0].amount is not None:
            raise AssertionError("retained auction quarantine duplicated a pruned economic root")
        CanonicalRewardPublisher(project).publish(after_prune_id)
        if CanonicalRewardReporter(reporter).read(after_prune_id)["coverage"]["unknown_count"] != 1 or \
                CanonicalRewardReporter(reporter).read(gid) != original_report:
            raise AssertionError("retained auction readback changed earlier publication after native pruning")
        _, peak = tracemalloc.get_traced_memory()
        result = dict(status="passed", native_owner="typed_settlement_to_money_claim", selected_claims=1, native_upstream_sales=sources,
            custody_copper=custody_copper, native_zero_fee_sales=1 if sources > 2 else 0,
            earned_count=0, snapshot_and_exact_retention=True, global_dependency_conflict=True,
            old_generation_immutable=True, restricted_roles=True, actual_cli=True,
            actual_capture_cli=True,
            source_query_plans=allocation_plans, retained_conflict_query_plans=conflict_plans,
            claim_allocation_index_qualified=True,
            stage_publication_faults_recovered=True, native_target_guard_refusals=6,
            source_queries=list(cuts[0].query_counts), source_rows=len(cuts[0].sources), source_reserved_bytes=cuts[0].reserved_bytes,
            source_capture_seconds=[cut.elapsed_seconds for cut in cuts],
            generation_reserved_bytes=original_plan.header["reserved_bytes"], peak_traced_python_bytes=peak,
            elapsed_seconds=time.monotonic() - started, native_binary_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
            source_metrics=source.metrics, projection_metrics=project.metrics, report_metrics=reporter.metrics,
            historical_backlog="unknown", award_dates="unknown", account_controller_attribution="unknown",
            unsupported_origins="bid refunds,other pending-claim owners", native_source_removed=True,
            native_source_rows_removed=removed_rows, native_accounting_roots_removed=len(native_ids),
            native_critical_recovery_inboxes_retained=len(native_ids), publication_after_native_prune=True,
            selected_quarantine_keeps_one_root=True,
            three_maximum_cut_budget_refused=three_cut_budget_refused,
            production_or_staging_access=False)
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(json.dumps(result, sort_keys=True))
    finally:
        tracemalloc.stop()
        if process is not None and process.poll() is None:
            process.kill(); process.communicate(timeout=10)
        with admin.cursor() as cursor:
            for user in roles:
                cursor.execute("DROP USER %s@'%%'", (user,))
        admin.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiled-auction", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--sources", type=int, choices=(2, 128, 129), default=2)
    args = parser.parse_args()
    for path in (args.compiled_auction, args.report):
        if not path.resolve().is_relative_to((ROOT / "bin/tests").resolve()):
            raise SystemExit("auction qualification files must stay under bin/tests")
    qualify(args.compiled_auction.resolve(), args.report.resolve(), sources=args.sources)
