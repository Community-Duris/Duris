---
name: burnin
description: >-
  Exhaustively qualify a local DurisMUD checkout by stopping its server, running every
  regression and isolated database test, clean-building all maintained code, repairing every
  finding, then booting and smoke-testing with the configured staff character. Use for an
  explicit full burn-in or stability pass, not routine focused validation.
---

# Burn in DurisMUD

Drive the checkout to one uninterrupted clean end-to-end pass after the last fix. Do not stop
merely because one phase passes.

## Boundaries

- Work from the repository root. Read `AGENTS.md`, `README.md`, the root `Makefile`, and the
  starting/stopping section of `docs/operations/RUNBOOK.md`; inspect the worktree first.
- Read only the required `.env` fields without printing their values. Proceed only when
  `ENVIRONMENT=local`; never operate on production/remote services or data. Preserve unrelated
  changes and all player/runtime data.
- Stop only the local instance belonging to this checkout. Prefer the matching user service or
  the in-game immortal `shutdown`; otherwise identify the exact supervisor and child by working
  directory, command line, and listener before a graceful signal. Never use broad `kill`/`pkill`.
  Wait for both processes and their ports to close. Treat another checkout owning a required port
  as a blocker, not as authorization to stop it.
- Do not skip an unavailable gate. Diagnose safe local prerequisites; if Docker, credentials, or
  another external dependency remains unavailable, report the burn-in as incomplete.

## Repair loop

1. With the MUD stopped, run every canonical gate and inspect output even when it exits zero:

   ```bash
   ./scripts/format.sh --check
   make test-all
   make test-db
   ```

   `make test-db` must use its isolated Docker/MySQL fixtures, never the configured game database.

2. Fix every error, warning, crash, hang, flaky result, sanitizer-like symptom, or credible defect
   encountered. Find the root cause, keep fixes narrow, add or update a focused regression for
   changed behavior, and do not weaken checks or hide diagnostics. Run the focused check after each
   repair, then repeat the full affected gate.

3. From a stopped state, prove a completely fresh build:

   ```bash
   make clean-all
   make
   ```

   The root build covers all maintained binaries. Review the entire build output for diagnostics,
   not just its status. After any repair, repeat the clean build and all canonical gates. Continue
   until the complete test, database-test, and clean-build sequence passes without findings.

## Live burn-in

1. Record current log boundaries, start this checkout with `./scripts/start_mud.sh --dev`, and wait
   for `./scripts/healthcheck.sh` to pass. Throughout boot and the smoke test, follow
   `logs/duris-console.log` and every current regular file under `logs/log/`, including files created
   after monitoring begins. Investigate unexpected output as well as obvious warning, error, fatal,
   assertion, crash, and persistence messages.
2. Without exposing secrets, log in through the account menu using `GAME_ACCOUNT_NAME`,
   `GAME_ACCOUNT_PASSWORD`, and `GAME_ACCOUNT_CHARACTER_NAME`. Run a randomized selection of safe,
   non-destructive player and staff inspection commands across several subsystems; record the exact
   commands. Avoid combat, movement, administration, or commands that alter players, world state,
   configuration, or data. Verify sensible responses, a stable connection, a healthy endpoint, and
   a clean logout.
3. Keep monitoring through a short post-logout soak. If anything is wrong, capture evidence, stop
   this exact instance, repair it, and restart the entire repair and live-burn-in loop. Finish only
   after a full clean pass following the final change; leave the healthy development MUD running
   unless the user requested otherwise.

Report fixes, exact commands and results, log files and observation interval, staff smoke commands,
final health/runtime state, and any genuine blocker. Never describe a skipped or partial pass as
clean.
