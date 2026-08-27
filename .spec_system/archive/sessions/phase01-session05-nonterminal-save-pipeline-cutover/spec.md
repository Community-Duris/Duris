# Session Specification

**Session ID**: `phase01-session05-nonterminal-save-pipeline-cutover`
**Phase**: 01 - Replace Forked Full Saves
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `896f0aee`
**Work Window**: Ordinary player mutation and checkpoint cutover; terminal extraction remains legacy.

## 1. Session Overview

Sessions 01-04 provide revision state, immutable capture, transactional workers, and a
durable journal, but no live save route uses them. This session adds a bounded dispatch
coordinator so capture is the last simulation-thread operation, then redirects ordinary
dirty, autosave, manual, deferred, and direct checkpoint routes before legacy I/O.

## 2. Objectives

1. Start and stop one bounded journal-dispatch coordinator around the existing worker.
2. Mark component revisions locally and capture only nonempty cumulative work.
3. Journal and submit snapshots off-thread, retaining retry work on overload or outage.
4. Drain exact completions on the game pulse without filesystem or database I/O.
5. Replace Redis dirty membership/fork flush with local revision marks and checkpoints.
6. Branch nonterminal `writeCharacter` and `do_save_silent` before legacy side effects.

## 3. Scope

### In Scope

- Player pipeline lifecycle, bounded dispatch queue, checkpoint results, and health.
- Ordinary online player routes and Redis dirty compatibility API replacement.
- Production startup/pulse integration and focused runtime/source-contract tests.

### Outside This Work Window

- Terminal extraction/drain, locker characters, world recovery, Phase 02 commands, and
  deletion of legacy implementations.

## 4. Technical Approach

The game thread marks monotonic component revisions, seals one cumulative snapshot, and
moves it into a fixed-count/fixed-byte coordinator queue. A dispatcher performs durable
journal append before worker submission and retries retained snapshots with bounded
backoff. Worker ACK checkpointing remains off-thread. The game pulse only drains typed
completions. Redis dirty APIs become compatibility wrappers over local revision state,
and their fork event is no longer scheduled.

## 5. Success Criteria

- [x] Unchanged checkpoints create no capture, journal record, or worker job.
- [x] Main-thread checkpoint and completion paths contain no database, Redis, or file I/O.
- [x] Repeated requests coalesce cumulative revisions without losing later changes.
- [x] Journal/worker outage retains dirty or queued state and reports explicit overload.
- [x] Redis dirty membership and dirty-save fork scheduling are disabled.
- [x] Every nonterminal full-save entry branches to the coordinator before legacy mutation/I/O.
- [x] Focused tests, formatting, warning-as-error build, security scan, and full suite pass.
