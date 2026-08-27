# Session Specification

**Session ID**: `phase03-session12-boot-schema-and-lookup-compatibility`
**Phase**: 03 - Load Path, Schema, and Retention
**Base Commit**: `ea8bf1054d3020bf797d5cd5556681ddc5a7d581`
**Created**: 2026-08-27

## Objective

Verify migration, schema, connection, and static lookup compatibility before the first
boot mutation, then publish race/class lookup changes in one checksummed transaction.

## Architecture

- Add one versioned runtime compatibility manifest mirrored by compiled constants and
  validated against the Session 11 baseline and critical metadata contract.
- Extend boot preflight with stable redacted reason IDs and move it ahead of lookup
  writes, item UID allocation, pool/worker startup, replay, and gameplay publication.
- Add lookup dataset state schema with compiled version/checksum/count identity.
- Build canonical race/class rows, compare state, no-op unchanged boot, and transactionally
  upsert/delete/state-advance with rollback on any error.
- Reuse the configured connection helper for every pool/worker connection invariant.

## Success Criteria

- [ ] Migration/schema/session compatibility passes before any boot database mutation.
- [ ] Missing/drifted schema, baseline, engine, collation, index, or connection state aborts.
- [ ] Unchanged lookup version/checksum causes zero table writes.
- [ ] Changed lookup rows and state publish atomically or roll back completely.
- [ ] Boot ordering prevents workers, replay, and gameplay before compatibility success.
- [ ] Standalone/source/isolated MySQL tests and full regressions pass.

## Safety

No production target, automatic migration, gameplay data change, or real operator
database test. Dynamic SQL uses only an isolated disposable database.

## Next Steps

Run `implement`.
