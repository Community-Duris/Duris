# Session 03: Save Failure Retry and Terminal Safety

**Session ID**: `phase00-session03-save-failure-retry-and-terminal-safety`
**Status**: Complete
**Work Window**: One durable-save failure contract covering deferred coordination,
flush behavior, fallback truthfulness, and every destructive terminal caller.

---

## Objective

Ensure a failed player save remains scheduled and retryable, and ensure no camp, rent,
death, link-loss, copyover, shutdown, or artifact transition destroys live state before
the required durable save succeeds.

---

## Scope

### In Scope (MVP)

- Replace the stranded deferred-save slot behavior with explicit pending, retry, and
  bounded backoff state that retains the latest requested save semantics.
- Make deferred flush alerts report actual success or failure and avoid duplicate full
  saves after a successful flush.
- Make `writeCharacter()` preserve and re-equip live inventory when durable save fails,
  including failure after a flat fallback attempt.
- Audit every destructive terminal caller and gate extraction or completion on the
  required save result.
- Preserve truthful alerts that distinguish durable database success from a fallback
  record that is not automatically reconciled.
- Extend focused tests for deferred retry, copyover, link loss, camp, death, locker, and
  shutdown failure edges.

### Out of Scope

- Phase 01 monotonic save revisions, component dirty masks, immutable DTO workers, and
  typed local journal.
- Treating the legacy binary pfile as a complete recovery protocol.
- Modernizing unrelated rent or extraction gameplay.

---

## Prerequisites

- [x] Session 01 redacted diagnostics and save-age reporting are validated.

---

## Deliverables

1. Retryable deferred-save state and truthful flush handling in `src/actoth.c` and
   related interfaces.
2. Fail-closed inventory and character handling in `src/files.c` and every terminal
   caller identified by the source review.
3. Focused failure-injection regressions under `tests/async/`, extending the existing
   deferred-save, copyover, locker, and save-guard suites where practical.
4. Operator-facing alerts that never claim durable completion from a failed save.

---

## Success Criteria

- [x] A failed deferred save remains pending and is retried with bounded backoff.
- [x] A later save request for the same player updates the pending state and cannot be
      stranded behind a slot with no scheduled callback.
- [x] Failed terminal saves leave the character and all inventory live and retryable.
- [x] Successful terminal paths do not perform redundant full saves for the same state.
- [x] Alerts distinguish database success, retryable failure, fallback-record success,
      and fallback-record failure accurately.
- [x] Focused regressions, formatting checks, and `make -C src` pass.

---

## Completion Summary

Implemented a bounded deferred-save retry state machine with truthful flush results,
fail-closed inventory restoration, and a shared terminal gate across player, artifact,
locker, copyover, shutdown, and reboot paths. Direct terminal failure queues only a
safe crash-save retry. Copyover and shutdown preserve the live process until all
required saves succeed, and incoherent locker departure now vetoes room release.

Validation passed the C++20 warning-as-error build, safe development runtime smoke,
170/170 Python regressions, signal-handler checks, formatting, whitespace, and
ASCII/LF scans. See the session package for review findings and detailed evidence.
