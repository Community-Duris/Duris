# Session Specification

**Session ID**: `phase03-session11-immutable-migration-ledger-and-runner`
**Phase**: 03 - Load Path, Schema, and Retention
**Base Commit**: `5202c8120e741acdd37185c3572a44b3630a51d5`
**Created**: 2026-08-27

## Objective

Add an honest verified baseline-adoption record and an immutable manifest-driven ledger
for every later migration, without fabricating historical execution evidence.

## Architecture

- Preserve the legacy runner solely as the baseline upgrade path and label its markers
  separately from the new complete migration ledger.
- Commit one required-schema contract and deterministic fingerprint for fresh bootstrap
  and verified legacy adoption.
- Add immutable ledger schema with ordered IDs, content/verify checksums, compatibility,
  baseline kind, status, and redacted failure identity.
- Add a strict runner that validates the entire manifest and every file before target
  access, acquires a database lock, verifies applied prefix immutability, executes one
  pending step, verifies it, and records success last.
- Refuse production targets and expose isolated empty/adoption/failure/resume tests.

## Success Criteria

- [ ] Honest bootstrap/adoption baseline identity is distinct from legacy data markers.
- [ ] All post-baseline migrations have unique ordered IDs and exact SHA-256 hashes.
- [ ] Missing, duplicate, reordered, edited, or failed migrations stop before later work.
- [ ] Success records only after apply and verifier success; exact replay is a no-op.
- [ ] Partial DDL remains unrecorded and a re-runnable migration can resume exactly.
- [ ] Fresh and verified-upgrade schema contracts converge to the same fingerprint.
- [ ] Target/credential checks reject production and tests use disposable databases.

## Safety

No configured database, production migration, rollback automation, or fabricated
history. Dynamic database checks use only isolated containers.

## Next Steps

Run `implement`.
