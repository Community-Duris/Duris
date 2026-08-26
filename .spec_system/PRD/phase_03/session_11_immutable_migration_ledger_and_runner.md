# Session 11: Immutable Migration Ledger and Runner

**Session ID**: `phase03-session11-immutable-migration-ledger-and-runner`
**Status**: Not Started
**Work Window**: One schema-history boundary from verified legacy baseline adoption
through immutable migration discovery, checksum validation, ordered execution, success
recording, rerun, partial-failure diagnosis, and fresh-database equivalence.

---

## Objective

Make migration history complete and tamper-evident so operators can prove which schema
contract is installed without pretending that untracked historical steps were recorded.

---

## Scope

### In Scope (MVP)

- Inventory the final authoritative runner, inline operations, SQL files, scoped
  verifiers, fresh bootstrap, existing `mud_schema_migrations` markers, and all migration
  tests after Phases 00 through 02.
- Define stable ordered migration IDs, canonical content hashing, SHA-256 checksums,
  description, compatibility/version metadata, and a ledger row recorded only after
  exact successful execution and verification.
- Replace manual total counts and anonymous `run_sql`/`run_sql_file` progression for all
  future changes with manifest-driven ordered discovery that rejects duplicate, missing,
  reordered, or modified IDs.
- Upgrade the marker table without losing selected legacy data-copy evidence and keep
  business/data-copy checkpoints distinct from the complete schema migration ledger.
- For existing databases, verify a committed required-schema fingerprint and record one
  explicit baseline-adoption migration; do not fabricate an applied row for each
  historical operation whose execution cannot be proven.
- For fresh databases, record the authoritative bootstrap baseline identity and apply
  only later migrations; verify that fresh bootstrap plus ledger and legacy upgrade plus
  adoption converge to the same required schema.
- Preserve fail-closed target checks, credentials handling, partial-DDL diagnostics,
  resumable exact rerun, isolated-clone safety, and no production automation.

### Out of Scope

- Automatically rolling back arbitrary MySQL/MariaDB DDL.
- Recording unverifiable historical timestamps or checksums as if they were observed.
- Running migrations against production or selecting a deployment promotion system.

---

## Prerequisites

- [ ] Phase 02 and Sessions 05, 08, and 10 final schema changes and verifiers are known.
- [ ] All migration tests run only on empty isolated databases or backed-up development
      clones.
- [ ] Existing data-copy markers are backed up and their semantics are documented.

---

## Deliverables

1. Versioned migration manifest and immutable ledger schema with stable IDs, checksums,
   ordered metadata, bootstrap/adoption identity, and verification fields.
2. Refactored authoritative migration runner with exact discovery, preflight, apply,
   verify, success record, rerun, and partial-failure reporting.
3. Read-only baseline fingerprint/adoption tooling and fresh-versus-upgrade schema
   equivalence verification.
4. Focused empty, legacy-adoption, duplicate-ID, reorder, checksum-tamper, failed-step,
   exact-rerun, data-marker, and MySQL/MariaDB compatibility tests.

---

## Success Criteria

- [ ] Every migration after the explicit baseline has one unique immutable ordered ID
      and SHA-256 checksum recorded only after its apply and verification succeed.
- [ ] Editing, deleting, duplicating, or reordering an applied migration fails closed
      with a redacted diagnostic before any later operation runs.
- [ ] Existing deployments receive an honest verified baseline-adoption record while
      selected legacy data-copy markers remain intact and semantically separate.
- [ ] A partially failed DDL run reports the exact migration and verification state and
      can resume safely without marking incomplete work successful.
- [ ] Empty bootstrap and legacy-upgrade/adoption paths converge to the same required
      schema and current migration identity.
- [ ] Target/credential safety remains fail closed and tests cannot select production.
- [ ] Focused runner, checksum, replay, schema-equivalence, and isolated database tests
      pass.
