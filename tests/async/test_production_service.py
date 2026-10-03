#!/usr/bin/env python3
"""Contracts for the production systemd service and launcher guard."""

from __future__ import annotations

import subprocess
import shutil
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


class ProductionServiceTest(unittest.TestCase):
    def setUp(self) -> None:
        self.cycle = (ROOT / "scripts/cycle_mud.sh").read_text()
        self.installer = (ROOT / "scripts/install-production-service.sh").read_text()
        self.template = (
            ROOT / "deploy/systemd/duris-mud-production.service.in"
        ).read_text()
        self.runbook = (ROOT / "docs/operations/RUNBOOK.md").read_text()
        self.readme = (ROOT / "README.md").read_text()

    def test_launcher_has_an_explicit_production_guard(self) -> None:
        self.assertIn("--production)", self.cycle)
        self.assertIn('--production requires ENVIRONMENT=production', self.cycle)
        self.assertIn("PRODUCTION_MODE == 1 && DEV_MODE == 1", self.cycle)
        self.assertIn('!= "mariadb/production"', self.cycle)
        self.assertIn("BUILD_PROFILE=production", self.cycle)

    def test_service_is_boot_enabled_and_restarts_without_rate_limit(self) -> None:
        for token in (
            "ExecStart=\"@DURIS_ROOT@/scripts/cycle_mud.sh\" --production",
            "Restart=always",
            "RestartSec=10s",
            "StartLimitIntervalSec=0",
            "WantedBy=multi-user.target",
        ):
            self.assertIn(token, self.template)

    def test_watchdog_startup_failure_stops_automatic_boot_retries(self) -> None:
        self.assertIn("RestartPreventExitStatus=78", self.template)
        self.assertIn("python3 scripts/game_loop_watchdog.py --check-config || exit 78", self.cycle)
        self.assertIn('python3 scripts/game_loop_watchdog.py -- "$RUNTIME_BINARY"', self.cycle)
        self.assertIn("if (( RESULT == 78 )); then", self.cycle)
        self.assertIn("refusing a boot loop", self.cycle)

    def test_service_uses_a_dedicated_account_and_baseline_hardening(self) -> None:
        for token in (
            "User=@DURIS_USER@",
            "Group=@DURIS_GROUP@",
            "UMask=0077",
            "NoNewPrivileges=true",
            "PrivateTmp=true",
            "PrivateDevices=true",
            "ProtectSystem=full",
            "ProtectProc=invisible",
            "CapabilityBoundingSet=",
            "RestrictNamespaces=true",
            "RestrictAddressFamilies=AF_UNIX AF_INET AF_INET6",
        ):
            self.assertIn(token, self.template)

    def test_installer_preflights_before_enabling(self) -> None:
        preflight = '"$ROOT/scripts/cycle_mud.sh" --production --check-config'
        self.assertLess(
            self.installer.index(preflight), self.installer.index("systemctl enable")
        )
        self.assertIn("systemd-analyze verify", self.installer)
        self.assertIn(
            "stop the prior MUD service before enabling production", self.installer
        )

    def test_rendered_unit_is_valid(self) -> None:
        # Render against a disposable deployment path, including when the
        # development worktree's path contains spaces unsupported by systemd.
        with tempfile.TemporaryDirectory(prefix="duris-production-unit-") as directory:
            fixture = Path(directory)
            (fixture / "scripts").mkdir()
            (fixture / "deploy/systemd").mkdir(parents=True)
            for relative in ("scripts/cycle_mud.sh", "deploy/systemd/duris-mud-production.service.in"):
                shutil.copy2(ROOT / relative, fixture / relative)
            rendered = subprocess.run(
                [
                    str(ROOT / "scripts/install-production-service.sh"),
                    "--render", "--root", str(fixture), "--user",
                    subprocess.check_output(["id", "-un"], text=True).strip(),
                ],
                cwd=ROOT, check=True, capture_output=True, text=True,
            ).stdout
            self.assertNotIn("@DURIS_", rendered)
            self.assertIn(f"WorkingDirectory={fixture}", rendered)
            unit = fixture / "duris-mud-production.service"
            unit.write_text(rendered)
            subprocess.run(
                ["systemd-analyze", "verify", str(unit)],
                check=True,
                capture_output=True,
                text=True,
            )

    def test_operator_docs_cover_installation_and_cutover(self) -> None:
        self.assertIn("#production-systemd-service", self.readme)
        for token in (
            "### Production systemd service",
            "install-production-service.sh",
            "systemctl --user disable --now duris-mud.service",
            "--user DURIS_USER --start",
        ):
            self.assertIn(token, self.runbook)


if __name__ == "__main__":
    unittest.main()
