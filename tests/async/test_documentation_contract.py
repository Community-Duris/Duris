#!/usr/bin/env python3
"""Source contracts for maintained persistence and operator documentation."""

from __future__ import annotations

import re
import unittest
from pathlib import Path
from urllib.parse import unquote


ROOT = Path(__file__).resolve().parents[2]
GUIDES = (
    ROOT / "README.md",
    ROOT / "docs/README.md",
    ROOT / "docs/ARCHITECTURE.md",
    ROOT / "docs/DATABASE.md",
    ROOT / "docs/CONFIGURATION.md",
    ROOT / "docs/RUNBOOK.md",
    ROOT / "docs/TESTING.md",
)
DIAGRAMS = (
    ROOT / "docs/diagrams/duris-server-architecture.html",
    ROOT / "docs/diagrams/duris-database-model.html",
)


def github_anchors(markdown: str) -> set[str]:
    anchors: set[str] = set()
    counts: dict[str, int] = {}
    in_fence = False
    for line in markdown.splitlines():
        if re.match(r"^\s*(?:```|~~~)", line):
            in_fence = not in_fence
            continue
        if in_fence:
            continue
        match = re.match(r"^#{1,6}\s+(.+?)\s*#*\s*$", line)
        if not match:
            continue
        heading = re.sub(r"<[^>]+>", "", match.group(1))
        heading = re.sub(r"[`*_~]", "", heading).strip().lower()
        slug = re.sub(r"[^\w\- ]", "", heading, flags=re.UNICODE)
        slug = re.sub(r"\s+", "-", slug)
        suffix = counts.get(slug, 0)
        counts[slug] = suffix + 1
        anchors.add(slug if suffix == 0 else f"{slug}-{suffix}")
    return anchors


class DocumentationContractTest(unittest.TestCase):
    def setUp(self) -> None:
        self.text = {path: path.read_text() for path in GUIDES}

    def test_maintained_markdown_links_and_anchors_resolve(self) -> None:
        failures: list[str] = []
        pattern = re.compile(r"(?<!!)\[[^]]+\]\(([^)]+)\)")
        for source, markdown in self.text.items():
            for raw_target in pattern.findall(markdown):
                raw_target = raw_target.strip().strip("<>")
                if re.match(r"^[a-z][a-z0-9+.-]*:", raw_target, re.I):
                    continue
                target_text, _, fragment = raw_target.partition("#")
                target = source if not target_text else source.parent / unquote(target_text)
                target = target.resolve()
                try:
                    target.relative_to(ROOT)
                except ValueError:
                    failures.append(
                        f"{source.relative_to(ROOT)} -> {raw_target} (outside repository)"
                    )
                    continue
                if not target.exists():
                    failures.append(f"{source.relative_to(ROOT)} -> {raw_target}")
                    continue
                if not fragment:
                    continue
                fragment = unquote(fragment).lower()
                if target.suffix.lower() == ".md":
                    if fragment not in github_anchors(target.read_text()):
                        failures.append(
                            f"{source.relative_to(ROOT)} -> {raw_target} (missing anchor)"
                        )
                elif target.suffix.lower() in {".html", ".svg"}:
                    if not re.search(rf'\bid=["\']{re.escape(fragment)}["\']',
                                     target.read_text(), re.I):
                        failures.append(
                            f"{source.relative_to(ROOT)} -> {raw_target} (missing id)"
                        )
        self.assertEqual(failures, [])

    def test_documented_operator_entry_points_exist(self) -> None:
        paths = (
            "scripts/start_mud.sh",
            "scripts/cycle_mud.sh",
            "scripts/migration_runner.py",
            "scripts/lifecycle_archive.py",
            "scripts/personal_data_export.py",
            "scripts/account_erasure.py",
            "scripts/validate_data_lifecycle.py",
            "migrations/run_migration.sh",
            "migrations/verify_runtime_compatibility.sh",
            "migrations/reconcile_epic_balances.sh",
            "migrations/reconcile_currency_balances.sh",
            "migrations/reconcile_item_ownership.sh",
            "migrations/reconcile_auction_transactions.sh",
            "migrations/reconcile_combat_frags.sh",
            "migrations/reconcile_artifact_guild_outcomes.sh",
            "migrations/reconcile_boon_reward_zone.sh",
            "migrations/reconcile_phase02_domains.sh",
            "tests/async/run_runtime_compatibility_mysql.sh",
            "tests/async/run_lifecycle_archive_schema_mysql.sh",
            "tests/async/run_personal_data_export_schema_mysql.sh",
            "tests/async/run_account_erasure_schema_mysql.sh",
            "tests/async/run_immutable_migration_ledger_mysql.sh",
        )
        runbook_and_testing = self.text[ROOT / "docs/RUNBOOK.md"] + self.text[
            ROOT / "docs/TESTING.md"
        ]
        for relative in paths:
            self.assertTrue((ROOT / relative).is_file(), relative)
            self.assertIn(relative, runbook_and_testing, relative)

        makefile = (ROOT / "Makefile").read_text()
        for target in ("test-all", "test-db", "test-list"):
            self.assertRegex(makefile, rf"(?m)^{re.escape(target)}:")

    def test_configuration_names_match_runtime_surface(self) -> None:
        configuration = self.text[ROOT / "docs/CONFIGURATION.md"]
        example = (ROOT / ".env.example").read_text()
        runtime = (ROOT / "src/sql.c").read_text() + (ROOT / "src/comm.c").read_text()
        required = (
            "ENVIRONMENT",
            "DB_HOST",
            "DB_PORT",
            "DB_USER",
            "DB_PASSWD",
            "DB_NAME",
            "DB_ALLOWED_TARGETS",
            "DB_SSL_CA",
            "PLAYER_SAVE_JOURNAL_DIR",
            "CRITICAL_COMMAND_JOURNAL_DIR",
            "MAINTENANCE_STATE_FILE",
            "REDIS",
            "REDIS_HOST",
            "REDIS_PORT",
            "REDIS_WORLD_STATE",
        )
        for name in required:
            self.assertIn(name, configuration, name)
            self.assertTrue(name in example or name in runtime, name)
        for token in ("10-second", "READ-COMMITTED", "utf8mb4", "250 ms", "100 ms"):
            self.assertIn(token, configuration, token)

    def test_integrated_topology_is_required_and_stale_claims_are_absent(self) -> None:
        maintained = "\n".join(self.text.values())
        required = (
            "consistent player load",
            "revisioned snapshot",
            "critical command",
            "current owner",
            "maintenance",
            "migration",
            "compatibility",
            "MySQL/MariaDB",
            "Redis",
        )
        lower = maintained.lower()
        for token in required:
            self.assertIn(token.lower(), lower, token)

        prohibited = (
            r"redis\s+(?:owns|holds|stores)\s+(?:the\s+)?dirty(?:-player)?\s+saves",
            r"credentials?\s+(?:live|stored|configured|hardened)\s+in\s+`?src/sql\.h`?",
            r"(?:listener|game)\s+port\s+(?:selects|chooses)\s+(?:the\s+)?database",
            r"player/object/ship saves are not written inline.*three worker threads",
            r"(?:is|are|now|proven|certified)\s+200-player\s+(?:ready|qualified)",
        )
        for pattern in prohibited:
            self.assertIsNone(re.search(pattern, maintained, re.I | re.S), pattern)

    def test_mutation_guidance_is_clone_bound_and_fail_closed(self) -> None:
        runbook = self.text[ROOT / "docs/RUNBOOK.md"]
        migration = runbook[runbook.index("### Migration procedure"):
                            runbook.index("### Domain reconciliation")]
        normalized = re.sub(r"\s+", " ", migration).lower()
        for token in (
            "disposable database",
            "backed-up development clone",
            "Stop every writer",
            "no dry-run mode",
            "`--help` is safe",
            "unknown argument is rejected",
            "normal no-argument run begins work immediately",
            "redis-cli FLUSHDB",
            "REDIS_HOST",
            "REDIS_PORT",
            "does not make a shared Redis safe",
            "never use production",
            "MySQL DDL may already have committed",
            "restore the known backup",
        ):
            self.assertIn(token.lower(), normalized, token)

        database = self.text[ROOT / "docs/DATABASE.md"]
        self.assertIn("never use production", database.lower())
        self.assertIn("backed-up development clone", database)
        self.assertIn("Stop the game and every other", database)
        self.assertNotIn("adopt --kind verified_legacy_adoption", migration)
        applying = database[database.index("### Applying schema changes"):
                            database.index("## Tables worth knowing")]
        self.assertNotIn("adopt --kind verified_legacy_adoption", applying)
        self.assertIn("records verified legacy adoption", applying)
        self.assertIn("redis-cli FLUSHDB", applying)
        self.assertIn("configured REDIS_HOST and REDIS_PORT", applying)

    def test_named_authority_tables_exist_in_schema_sources(self) -> None:
        database = self.text[ROOT / "docs/DATABASE.md"]
        schema = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
        immutable = (ROOT / "migrations/immutable/0001_lookup_dataset_state.sql").read_text()
        required = (
            "critical_operation_inbox",
            "critical_outbox",
            "item_current_owner",
            "item_ownership_ledger",
            "mud_schema_baselines",
            "mud_schema_history",
            "mud_schema_migration_state",
            "lookup_dataset_state",
        )
        for table in required:
            self.assertIn(table, database, table)
            self.assertTrue(table in schema or table in immutable, table)
        for nonexistent in (
            "critical_operation_result",
            "`mud_schema_baseline`",
            "`mud_schema_state`",
        ):
            self.assertNotIn(nonexistent, database, nonexistent)

    def test_lifecycle_and_privacy_remain_pending_and_disabled(self) -> None:
        maintained = "\n".join(self.text.values())
        normalized = re.sub(r"\s+", " ", maintained).lower()
        for token in (
            "pending policy",
            "blocked_by_policy",
            "canonical mutation remains disabled",
            "not controller approval",
            "not a claim of legal compliance",
        ):
            self.assertIn(token.lower(), normalized, token)
        self.assertNotRegex(
            maintained.lower(),
            r"canonical\s+(?:archive|export|erasure)\s+(?:is\s+)?enabled",
        )

    def test_diagrams_are_accessible_ascii_and_current(self) -> None:
        for path in DIAGRAMS:
            data = path.read_bytes()
            self.assertNotIn(b"\r", data, path.name)
            text = data.decode("ascii")
            slug = path.stem
            match = re.search(
                r'<svg\b([^>]*)>\s*<title\s+id="([^"]+)">([^<]+)</title>\s*'
                r'<desc\s+id="([^"]+)">([^<]+)</desc>',
                text,
                re.S,
            )
            self.assertIsNotNone(match, path.name)
            attributes, title_id, title, desc_id, desc = match.groups()
            self.assertIn('role="img"', attributes)
            self.assertIn(
                f'aria-labelledby="{title_id} {desc_id}"', attributes, path.name
            )
            self.assertEqual(title_id, f"{slug}-title")
            self.assertEqual(desc_id, f"{slug}-desc")
            self.assertTrue(title.strip())
            self.assertTrue(desc.strip())
            self.assertEqual(text.count("<marker "), 3)
            self.assertNotIn("JetBrains Mono", text)
            self.assertNotIn("dirty saves", text.lower())
            self.assertNotIn("persistence_item_events", text)

        server = DIAGRAMS[0].read_text()
        for token in (
            "Boot Compatibility Gate",
            "Consistent Player Load",
            "Revisioned Snapshots",
            "Critical Commands",
            "MySQL / MariaDB",
            "Optional Redis",
        ):
            self.assertIn(token, server)
        database = DIAGRAMS[1].read_text()
        for token in (
            "Player Revision",
            "Critical Inbox/Outbox",
            "Current Authority",
            "Domain Ledgers",
            "PENDING CONTROLLER DECISIONS",
            "Schema and Dataset Identity",
        ):
            self.assertIn(token, database)

    def test_new_contract_and_diagram_files_are_ascii_lf(self) -> None:
        for path in (*DIAGRAMS, Path(__file__)):
            data = path.read_bytes()
            self.assertNotIn(b"\r", data, str(path))
            data.decode("ascii")


if __name__ == "__main__":
    unittest.main()
