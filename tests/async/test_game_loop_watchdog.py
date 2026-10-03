#!/usr/bin/env python3
"""Executable observer/launcher regressions using isolated native game doubles."""

from __future__ import annotations

import os
from pathlib import Path
import re
import select
import shutil
import signal
import socket
import subprocess
import sys
import tempfile
import time
import unittest


ROOT = Path(__file__).resolve().parents[2]
OBSERVER = ROOT / "scripts/game_loop_watchdog.py"
HARNESS = ROOT / "bin/tests/game_loop_watchdog"


@unittest.skipUnless(sys.platform == "linux", "Linux procfs, credentials, and parent-death signals")
class GameLoopWatchdogTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        HARNESS.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run([
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pthread", "-Isrc",
            "tests/async/game_loop_watchdog_harness.cpp", "src/core/game_loop_watchdog.c",
            "-o", str(HARNESS),
        ], cwd=ROOT, check=True)

    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix="duris-loop-watchdog-")
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.environment = os.environ.copy()
        for key in list(self.environment):
            if key.startswith("DURIS_WATCHDOG_") or key.startswith("WATCHDOG_TEST_"):
                del self.environment[key]
        for name, value in {"startup": 1.2, "stall": 0.3, "copyover": 0.9,
                            "shutdown": 0.9, "abort": 0.15}.items():
            self.environment[f"DURIS_WATCHDOG_{name.upper()}_SECONDS"] = str(value)

    def run_game(self, mode: str, status: int = 0) -> subprocess.CompletedProcess:
        result = subprocess.run(
            [sys.executable, str(OBSERVER), "--", str(HARNESS), mode],
            cwd=self.directory, env=self.environment, capture_output=True, text=True, timeout=5,
        )
        self.assertEqual(result.returncode, status, result.stdout + result.stderr)
        return result

    def start_game(self, mode: str) -> tuple[subprocess.Popen, int, int]:
        process = subprocess.Popen(
            [sys.executable, str(OBSERVER), "--", str(HARNESS), mode],
            cwd=self.directory, env=self.environment,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
        )
        self.addCleanup(self.cleanup_game, process)
        self.assertTrue(select.select([process.stdout], [], [], 3)[0], "game did not start")
        line = process.stdout.readline()
        match = re.search(r"pid=(\d+).*port=(\d+)", line)
        self.assertIsNotNone(match, line)
        return process, int(match[1]), int(match[2])

    @staticmethod
    def cleanup_game(process: subprocess.Popen) -> None:
        if process.poll() is None:
            process.terminate()
            try:
                process.communicate(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.communicate(timeout=3)
        if process.stdout:
            process.stdout.close()
        if process.stderr:
            process.stderr.close()

    def test_normal_completed_loops_outlast_the_stall_deadline(self) -> None:
        result = self.run_game("normal")
        self.assertNotIn("deadline expired", result.stderr)

    def test_observer_scheduling_pause_reads_queued_completions_before_expiry(self) -> None:
        process, _pid, _port = self.start_game("normal-long")
        time.sleep(0.1)
        os.kill(process.pid, signal.SIGSTOP)
        time.sleep(0.6)  # The world continues completing loops beyond the stall deadline.
        os.kill(process.pid, signal.SIGCONT)
        output, errors = process.communicate(timeout=5)
        self.assertEqual(process.returncode, 0, output + errors)
        self.assertNotIn("deadline expired", errors)

    def test_cpu_bound_callback_is_terminated_with_external_diagnostics(self) -> None:
        result = self.run_game("cpu", 56)
        self.assertIn("phase=running", result.stderr)
        self.assertIn("completed=1", result.stderr)
        reports = list((self.directory / "logs/watchdog").glob("*.txt"))
        self.assertEqual(len(reports), 1)
        self.assertEqual(reports[0].stat().st_mode & 0o777, 0o600)
        report = reports[0].read_text()
        self.assertIn("/task/", report)
        self.assertIn("/wchan", report)
        self.assertIn("/syscall", report)

    def test_blocking_wait_with_live_worker_and_listening_port_is_terminated(self) -> None:
        process, _pid, port = self.start_game("blocking")
        with socket.create_connection(("127.0.0.1", port), timeout=1):
            pass
        output, errors = process.communicate(timeout=5)
        self.assertEqual(process.returncode, 56, output + errors)
        self.assertIn("completed=1", errors)

    def test_worker_activity_cannot_advance_world_heartbeat(self) -> None:
        self.run_game("worker-only", 56)

    def test_same_counter_with_fresh_timestamps_does_not_renew_progress(self) -> None:
        self.run_game("duplicate", 56)

    def test_forked_sender_cannot_impersonate_world_progress(self) -> None:
        self.run_game("foreign", 56)

    def test_startup_grace_allows_boot_longer_than_stall_deadline(self) -> None:
        self.run_game("startup")

    def test_hang_before_first_completed_loop_stops_boot_retries(self) -> None:
        result = self.run_game("startup-hang", 78)
        self.assertIn("phase=startup", result.stderr)

    def test_bounded_copyover_drain_and_failed_copyover_resume(self) -> None:
        self.run_game("drain")

    def test_repeated_lifecycle_records_do_not_extend_drain_deadline(self) -> None:
        result = self.run_game("drain-hang", 56)
        self.assertIn("phase=copyover", result.stderr)

    def test_retrying_drain_without_a_completed_loop_does_not_grant_new_grace(self) -> None:
        self.run_game("drain-retry", 56)

    def test_shutdown_and_reboot_drains_allow_bounded_waits(self) -> None:
        self.run_game("shutdown")
        self.run_game("reboot", 52)

    def test_stalled_intentional_shutdown_does_not_request_restart(self) -> None:
        self.run_game("shutdown-hang")

    def test_copyover_exec_preserves_pid_channel_and_increasing_counter(self) -> None:
        result = self.run_game("copyover")
        pids = re.findall(r"world pid=(\d+)", result.stdout)
        self.assertEqual(len(pids), 2, result.stdout)
        self.assertEqual(pids[0], pids[1])
        self.assertNotIn("deadline expired", result.stderr)

    def test_stalled_replacement_boot_recovers_previously_running_game(self) -> None:
        result = self.run_game("copyover-hang", 56)
        self.assertIn("phase=copyover startup", result.stderr)

    def test_copyover_disarms_inherited_legacy_virtual_timer(self) -> None:
        self.run_game("legacy-copyover")

    def test_ignored_abort_is_escalated_to_kill(self) -> None:
        self.run_game("abort-ignored", 56)

    def test_observer_stop_forwards_signal_and_waits_for_bounded_drain(self) -> None:
        process, _pid, _port = self.start_game("signal-stop")
        time.sleep(0.1)
        process.terminate()
        output, errors = process.communicate(timeout=5)
        self.assertEqual(process.returncode, 0, output + errors)
        self.assertNotIn("deadline expired", errors)

    def test_observer_death_cannot_leave_unsupervised_game(self) -> None:
        process, pid, _port = self.start_game("cpu")
        process.kill()
        process.communicate(timeout=5)
        deadline = time.monotonic() + 2
        while time.monotonic() < deadline:
            stat = Path(f"/proc/{pid}/stat")
            if not stat.exists() or stat.read_text().split(") ", 1)[1].startswith("Z"):
                return
            time.sleep(0.02)
        os.killpg(pid, signal.SIGKILL)
        self.fail("observer death left a live game")

    def launcher_fixture(self, mode: str) -> list[str]:
        for relative in ("scripts", "bin/server", "areas_mini", "lib/misc", "shims"):
            (self.directory / relative).mkdir(parents=True)
        for script in ("cycle_mud.sh", "game_loop_watchdog.py"):
            shutil.copy2(ROOT / "scripts" / script, self.directory / "scripts" / script)
        shutil.copy2(HARNESS, self.directory / "bin/server/dms")
        for name in ("mini.mob", "mini.obj", "mini.qst", "mini.wld", "mini.zon",
                     "world.shp", "world.tab", "world.weather"):
            (self.directory / "areas_mini" / name).write_text("isolated fixture\n")
        # Exercise the real launcher's backoff branch without waiting ten seconds.
        sleep = self.directory / "shims/sleep"
        sleep.write_text('#!/bin/bash\necho "fixture backoff: $*" >&2\nexec /bin/sleep 0.01\n')
        sleep.chmod(0o755)
        self.environment.update({
            "ENVIRONMENT": "local", "PERSISTENCE_MODE": "flatfile-primary",
            "FLATFILE_STATE_DIR": str(self.directory / "state"), "SKIP_PREBOOT_BACKUP": "1",
            "WATCHDOG_TEST_MODE": mode,
            "WATCHDOG_TEST_RESTART_FILE": str(self.directory / "restarts"),
            "PATH": f"{self.directory / 'shims'}:{self.environment['PATH']}",
        })
        return ["bash", str(self.directory / "scripts/cycle_mud.sh"), "--minimal"]

    def test_supervisor_recovers_with_fresh_process_and_channel(self) -> None:
        result = subprocess.run(self.launcher_fixture("recover"), cwd=self.directory,
                                env=self.environment, capture_output=True, text=True, timeout=8)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        pids = (self.directory / "restarts").read_text().splitlines()
        self.assertEqual(len(pids), 2)
        self.assertNotEqual(pids[0], pids[1])
        self.assertIn("mud hung reboot [56]", result.stdout)
        self.assertIn("fixture backoff: 10", result.stderr)

    def test_supervisor_refuses_startup_hang_boot_loop(self) -> None:
        result = subprocess.run(self.launcher_fixture("startup-hang"), cwd=self.directory,
                                env=self.environment, capture_output=True, text=True, timeout=5)
        self.assertEqual(result.returncode, 78, result.stdout + result.stderr)
        self.assertEqual(result.stdout.count("world pid="), 1)
        self.assertIn("refusing a boot loop", result.stderr)

    def test_invalid_deadlines_fail_closed_before_launch(self) -> None:
        for value in ("0", "-1", "nan", "inf", "garbage"):
            with self.subTest(value=value):
                self.environment["DURIS_WATCHDOG_STALL_SECONDS"] = value
                result = subprocess.run([sys.executable, str(OBSERVER), "--check-config"],
                                        env=self.environment, capture_output=True, text=True)
                self.assertEqual(result.returncode, 78)

    def test_integration_credits_only_complete_world_loop(self) -> None:
        source = (ROOT / "src/net/comm.c").read_text()
        loop = source[source.index("resume_game_loop:"):source.index("if (_copyover)", source.index("resume_game_loop:"))]
        self.assertLess(loop.index("run_pulse_reset_phase(context);"),
                        loop.index("game_loop_watchdog_completed();"))
        self.assertIn("if (!run_connection_phase(context))\n\t\t\tcontinue;", loop)
        self.assertNotIn("ITIMER_VIRTUAL", (ROOT / "src/core/signals.c").read_text())
        unit = (ROOT / "deploy/systemd/duris-mud-production.service.in").read_text()
        self.assertIn("RestartPreventExitStatus=78", unit)


if __name__ == "__main__":
    unittest.main()
