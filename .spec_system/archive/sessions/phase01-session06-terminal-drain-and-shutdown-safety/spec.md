# Session Specification

**Session ID**: `phase01-session06-terminal-drain-and-shutdown-safety`
**Phase**: 01 - Replace Forked Full Saves
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `28735fde`
**Work Window**: Terminal promotion, bounded durability wait, extraction gates, and process drain.

## Objectives

1. Add fixed-capacity exact-revision terminal fences and typed terminal outcomes.
2. Promote/capture one newest cumulative revision and wait to a monotonic deadline.
3. Permit extraction only after database ACK or explicitly allowed synced journal handoff.
4. Reject new ordinary mutations while the terminal revision remains fenced.
5. Quiesce, drain, or journal-spill every accepted job before copyover/shutdown success.
6. Retire new player flat-fallback writes while preserving all existing files.

## Scope

Terminal player paths, copyover/shutdown integration, coordinator drain/quiescence,
legacy player fallback retirement, redacted health, and focused regressions. World
snapshot shutdown, preexisting pfile deletion, and Phase 02 critical domains are out.

## Success Criteria

- [x] Exact terminal fence cannot be released by an older completion.
- [x] Journal or DB durability is proven before destructive callers proceed.
- [x] Timeout/failure retains live state and a retryable fence.
- [x] Copyover/shutdown quiesce and finish a bounded drain before success.
- [x] New player flat fallback writes are disabled without deleting existing records.
- [x] Focused and full validation gates pass.
