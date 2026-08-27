# Task Checklist

**Session ID**: `phase01-session04-typed-persistence-journal-and-replay`
**Total Tasks**: 18
**Created**: 2026-08-27

## Inventory And Design

- [x] T001 Confirm clean Session 03 base and validated worker/ACK prerequisites.
- [x] T002 Inventory existing fallback fsync/rotation patterns and runtime path constraints.
- [x] T003 Define fixed record framing, typed payload codec, checksum, identity, and bounds.
- [x] T004 Define append/checkpoint/replay crash boundaries, quota, permissions, and metrics.

## Codec And Journal

- [x] T005 Implement endian-stable bounded snapshot encoder.
- [x] T006 Implement defensive snapshot decoder and full DTO validation.
- [x] T007 Implement CRC32 record framing and unique identity.
- [x] T008 Implement safe directory/file creation, append-all, sync, quota, and locking.
- [x] T009 Implement bounded scanner, corruption classification, and quarantine append.
- [x] T010 Implement exact ACK watermark and atomic compaction.
- [x] T011 Implement duplicate suppression, per-PID replay ordering, and idempotent outcomes.
- [x] T012 Integrate append-before-handoff and ACK checkpoint hooks with the worker.
- [x] T013 Add shutdown/high-water spill and unavailable-worker retained-journal behavior.
- [x] T014 Add redacted journal diagnostics and ignored runtime documentation.

## Tests And Completion

- [x] T015 Add codec/append/permission/quota runtime tests.
- [x] T016 Add truncation/corruption/unsupported/compaction/replay/crash-point tests.
- [x] T017 Run focused tests, format, warning-as-error build, security scan, and full suite.
- [x] T018 Complete review, validation, records/version, commit, and publication.

## Completion Checklist

- [x] All 18 tasks complete
- [x] No outstanding blocker or unresolved failure
- [x] Ready for `creview`
