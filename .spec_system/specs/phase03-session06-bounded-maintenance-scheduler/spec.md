# Session Specification

**Session ID**: `phase03-session06-bounded-maintenance-scheduler`
**Phase**: 03 - Load Path, Schema, and Retention
**Base Commit**: `f603203b72b0a66e46f40ec1105896ae24f897f3`
**Created**: 2026-08-27

## Objective

Replace aligned recurring external work with one typed, deterministic, bounded
maintenance scheduler and worker. Preserve pure game-thread activities, split mixed
jobs into immutable worker requests plus game-thread completion, prevent overlap, and
prove cursor/retry/shutdown behavior without adding Session 05-rejected indexes.

## Audited Boundary

- Pure/game-thread: ship, ferry, newcomer announce, random-map spawn, short affects.
- External or mixed: auction due scan, web status file, poll expiry, epic task catalog,
  epic-zone balance/modifiers, zone trophies, level cap, boon scan/mutation, and cargo
  database work.
- Boot-only work remains boot-only unless explicitly registered.

## Architecture

- Stable `maintenance_job_id`, cadence, deterministic instance offset, priority,
  deadline, row/time budget, cursor, and overlap rule.
- One bounded worker using a pooled connection and pointer-free request/result DTOs.
- At most one submission and bounded completions per pulse; exact work identity survives
  retry; no live pointer crosses the worker boundary.
- Mixed jobs snapshot only bounded primitive state and publish results on the game
  thread. File publication uses immutable content and atomic replacement.
- Redacted health exposes queue/run age, cursor, rows, retries, suppressions, failures.

## Success Criteria

- [ ] Stable deterministic offsets remove the aligned 60/120-second wave.
- [ ] Every external scan is row/time bounded with exact continuation.
- [ ] Jobs cannot overlap; retry preserves identity/cursor with bounded backoff.
- [ ] Scheduled game-thread callbacks perform no DB, Redis, filesystem, or large serialization.
- [ ] Shutdown/copyover drains or cancels within a fixed deadline without losing cursor state.
- [ ] Focused common-multiple, outage, retry, cursor, overlap, restart, and shutdown tests pass.
- [ ] Warning-clean build, formatting, security/BQC review, and full regression pass.

## Safety

No production operation or migration. Session 05 approved no new index, so every job
must use current schema paths and tighter budgets or remain disabled/fail-closed.

## Next Steps

Run `implement`.
