# Session Specification

**Session ID**: `phase03-session07-data-processing-and-retention-contract`
**Phase**: 03 - Load Path, Schema, and Retention
**Base Commit**: `253801359c1c1887a0ed3b68d77e9c86b6ef9b06`
**Created**: 2026-08-27

## Objective

Create one strict, versioned lifecycle manifest covering every database and declared
non-database durable store. Record technical purpose, subject mapping, season ownership,
retention/archive/terminal behavior, dependencies, reconciliation exceptions, and
approval evidence without inventing legal decisions or enabling unapproved destruction.

## Architecture

- Machine-readable manifest with a strict versioned schema and one canonical store ID.
- Runtime/tool validator that compares live schema inventory and declared file/Redis/
  journal/backup classes, validates dependencies, and fails closed on drift.
- Explicit protected-ledger/replay/reconciliation constraints and destructive-rule
  approval gates.
- One shared classification source for season reset, archive, export, erasure, restore,
  and documentation consumers.

## Success Criteria

- [ ] Every active table and declared non-database store is covered exactly once.
- [ ] Personal-data classes have technical subject/purpose/retention/action/exception mappings.
- [ ] Protected financial, ownership, moderation, replay, outbox, and audit records reject unsafe purge rules.
- [ ] Unknown stores/actions, cycles, schema drift, stale versions, and unapproved destructive rules fail closed.
- [ ] Season, archive, export, erasure, restore, and documentation use one manifest.
- [ ] Engineering controls are distinguished from controller/legal conclusions.
- [ ] Focused tests, build/format where applicable, full regression, review, and security/BQC pass.

## Safety

No archive, purge, erasure, production operation, or invented controller decision. Any
missing approval remains explicit and destructive execution stays disabled.

## Next Steps

Run `implement`.
