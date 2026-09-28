#!/usr/bin/env python3
"""Source contracts for retained cutover owners at app teardown boundaries."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
COORDINATOR = (SRC / "persistence/critical_command_coordinator.c").read_text()
COORDINATOR_H = (SRC / "persistence/critical_command_coordinator.h").read_text()
COMM = (SRC / "net/comm.c").read_text()


def section(text, start, end):
    begin = text.index(start)
    finish = text.index(end, begin + len(start))
    return text[begin:finish]


def occurrences(text, token):
    positions = []
    offset = 0
    while (position := text.find(token, offset)) >= 0:
        positions.append(position)
        offset = position + len(token)
    return positions


class CriticalShutdownBoundaryTests(unittest.TestCase):
    def test_lifecycle_guard_atomically_refuses_without_mutating_active_owner(self):
        acquire = section(
            COORDINATOR,
            "bool critical_command_coordinator_try_acquire_lifecycle_guard(void)",
            "void critical_command_coordinator_release_lifecycle_guard(void)",
        )
        self.assertIn("std::lock_guard<std::mutex> lock(coordinator_mutex)", acquire)
        refusal = acquire.index("return false;")
        self.assertLess(refusal, acquire.index("lifecycle_guard_active = true;"))
        self.assertLess(refusal, acquire.index("health.accepting = false;"))
        self.assertIn("active_cutover_phase != cutover_owner_phase::none", acquire)
        self.assertNotIn("stop_requested = true", acquire)
        self.assertNotIn("operations.clear()", acquire)

        release = section(
            COORDINATOR,
            "void critical_command_coordinator_release_lifecycle_guard(void)",
            "bool critical_command_coordinator_shutdown(void)",
        )
        self.assertIn("lifecycle_guard_thread != std::this_thread::get_id()", release)
        self.assertLess(
            release.index("lifecycle_guard_active = false;"),
            release.index("health.accepting = lifecycle_guard_was_accepting;"),
        )
        self.assertIn("bool critical_command_coordinator_try_acquire_lifecycle_guard(void);",
                      COORDINATOR_H)

        shutdown = section(
            COORDINATOR,
            "bool critical_command_coordinator_shutdown(void)",
            "critical_submit_result critical_command_coordinator_submit_internal(",
        )
        refusal = shutdown.index("return false;")
        self.assertLess(refusal, shutdown.index("stop_requested = true;"))
        self.assertLess(refusal, shutdown.index("operations.clear();"))

    def test_owner_issuance_cannot_race_a_claimed_lifecycle_boundary(self):
        acquire_owner = section(
            COORDINATOR,
            "bool critical_command_coordinator_owner::acquire_cutover_lease(",
            "bool critical_command_coordinator_owner::validate_cutover_lease(",
        )
        self.assertIn("lifecycle_guard_active", acquire_owner)
        self.assertIn("active_cutover_phase != cutover_owner_phase::none", acquire_owner)

        init = section(
            COORDINATOR,
            "bool critical_command_coordinator_init(",
            "bool critical_command_coordinator_try_acquire_lifecycle_guard(void)",
        )
        self.assertIn("lifecycle_guard_active", init)

    def test_shutdown_and_copyover_refuse_before_any_dependent_teardown(self):
        game_loop = section(
            COMM,
            "void game_loop(int port, int sslport)",
            "/*\n * ****************************************************************** *\n * general utility stuff",
        )
        copyover = game_loop[game_loop.index("\n\tif (_copyover)") :]
        self.assertLess(
            copyover.index("critical_command_coordinator_try_acquire_lifecycle_guard()"),
            copyover.index("kingdom_flush_persistent_state();"),
        )
        self.assertLess(
            copyover.index("critical_command_coordinator_try_acquire_lifecycle_guard()"),
            copyover.index("copyover_save(s, S, WS)"),
        )
        self.assertIn("refuse_lifecycle_for_active_cutover_owner(\"copyover\")", copyover)
        self.assertIn("critical_command_coordinator_release_lifecycle_guard();", copyover)

        pending = game_loop.index("item_creation_grant_batches_pending()")
        shutdown_guard = game_loop.index(
            "critical_command_coordinator_try_acquire_lifecycle_guard()", pending
        )
        quiesce = game_loop.index("critical_command_coordinator_quiesce();", shutdown_guard)
        self.assertLess(shutdown_guard, quiesce)
        refusal = game_loop[shutdown_guard:quiesce]
        self.assertIn("refuse_lifecycle_for_active_cutover_owner(\"shutdown\")", refusal)
        self.assertIn("goto resume_game_loop;", refusal)
        for teardown in (
            "critical_outbox_quiesce();",
            "critical_command_coordinator_drain(",
            "critical_outbox_drain(",
            "shutdown_ships();",
            "close_sockets(s);",
        ):
            self.assertNotIn(teardown, refusal)

    def test_shutdown_gate_cancellation_releases_after_resuming_dependents(self):
        game_loop = section(
            COMM,
            "void game_loop(int port, int sslport)",
            "/*\n * ****************************************************************** *\n * general utility stuff",
        )
        releases = occurrences(
            game_loop, "critical_command_coordinator_release_lifecycle_guard();"
        )
        coordinator_resumes = occurrences(game_loop, "critical_command_coordinator_resume();")
        outbox_resumes = occurrences(game_loop, "critical_outbox_resume();")
        self.assertEqual(len(releases), 7)
        self.assertEqual(len(coordinator_resumes), 6)
        self.assertEqual(len(outbox_resumes), 6)
        for resume in coordinator_resumes + outbox_resumes:
            self.assertLess(resume, next(release for release in releases if release > resume))
        self.assertIn("player_save_pipeline_resume();", game_loop)
        self.assertIn("retry_after_owner_terminal_cleanup=1", COMM)

    def test_coordinator_shutdown_is_checked_and_inactive_order_is_preserved(self):
        run_game = section(
            COMM,
            "int run_the_game(int port, int sslport)",
            "/* Accept new connects, relay commands, and call 'heartbeat-functs' */",
        )
        teardown = run_game[run_game.index("game_loop(port, sslport);") :]
        order = (
            "kingdom_shutdown();",
            "maintenance_scheduler_shutdown();",
            "player_death_restitution_runtime_shutdown();",
            "const bool critical_coordinator_stopped = critical_command_coordinator_shutdown();",
            "locker_identify_shutdown();",
            "if (critical_coordinator_stopped)",
            "critical_outbox_shutdown();",
            "locker_async_shutdown();",
            "player_save_pipeline_shutdown();",
        )
        positions = [teardown.index(token) for token in order]
        self.assertEqual(positions, sorted(positions))

        startup_failure = run_game[: run_game.index("game_loop(port, sslport);")]
        self.assertIn("if (critical_command_coordinator_shutdown())", startup_failure)
        startup_cleanup = section(
            startup_failure,
            "if (critical_command_coordinator_shutdown())",
            'logit(LOG_STATUS,\n\t\t      "Critical command pipeline unavailable',
        )
        self.assertIn("player_death_restitution_runtime_abort_all();", startup_cleanup)
        self.assertIn("critical_outbox_shutdown();", startup_cleanup)
        self.assertIn("shutdown refused; retaining dependent pipelines", startup_cleanup)

        main = section(COMM, "const int game_exit_status = run_the_game(", "return game_exit_status;")
        self.assertLess(main.index("shutdown_mysql();"),
                        main.index("critical_command_coordinator_release_lifecycle_guard();"))


if __name__ == "__main__":
    unittest.main()
