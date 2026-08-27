# Session 07: Data Processing and Retention Contract

**Session ID**: `phase03-session07-data-processing-and-retention-contract`
**Status**: Complete (destructive rules remain disabled pending controller decisions)
**Work Window**: One governance-to-runtime contract covering the complete data-store
inventory, subject keys, purpose and approval evidence, season/time rules, archive and
erasure actions, exceptions, validation, and fail-closed policy loading.

---

## Objective

Replace implicit indefinite storage with one versioned, reviewable lifecycle manifest
that classifies every durable and recovery record and can safely drive archival,
export, erasure, and restore behavior.

---

## Scope

### In Scope (MVP)

- Inventory every post-Phase-02 table plus Redis keys, typed journals, fallback or
  quarantine records, runtime pfiles/accounts, logs, export spools, and backup classes.
- For each record class, define owner, data category, data-subject key, gameplay or
  operational purpose, controller-approved lawful-basis reference, season ownership,
  active retention, archive duration, terminal action, and audit/reconciliation
  exception.
- Classify Phase 02 epic, currency, ownership, inbox, result, outbox, and operation
  history without allowing ordinary cleanup to break balance/owner reconciliation or
  duplicate-replay horizons.
- Reconcile the lifecycle manifest with the existing season-reset manifest, foreign-key
  cascades, soft-deleted characters, account mapping, logs, messages, PvP rows,
  statistics, trophies, chest activity, and recovery copies.
- Define explicit actions such as retain, archive, purge, pseudonymize, cascade,
  regenerate, and restore-tombstone, including ordering and required pre/post checks.
- Implement strict manifest parsing, schema/table coverage validation, versioning,
  environment/role safety, approval metadata, and fail-closed handling for missing or
  unapproved destructive rules.
- Publish the technical personal-data inventory and privacy/lifecycle guidance without
  claiming legal applicability, lawful basis, or compliance that has not been approved.

### Out of Scope

- Executing archive/purge jobs, owned by Session 08.
- Personal data export or account erasure execution, owned by Sessions 09 and 10.
- Legal advice or invention of controller decisions.

---

## Prerequisites

- [x] Phase 02 final schema and reconciliation contracts are available.
- [ ] The designated data controller or repository owner supplies or approves purpose,
      lawful-basis, retention, archive, and exception decisions before destructive rules
      can be enabled.
- [x] Session 05 schema inventory and Session 06 maintenance identities are stable.

---

## Deliverables

1. Versioned machine-readable lifecycle manifest covering every table and non-database
   store, with strict schema and approval metadata.
2. Coverage, subject-key, dependency-order, policy-version, and fail-closed validation
   tooling under `migrations/`, `scripts/`, or a focused lifecycle module.
3. Reconciled season-reset classifications and technical personal-data inventory with
   purpose, retention, export, erasure, and exception mappings.
4. Focused missing-table, unknown-action, unapproved-rule, dependency-cycle, protected-
   ledger, schema-drift, and non-production safety regressions.

---

## Success Criteria

- [x] Every active schema table and every declared journal, cache, file, log, export,
      and backup class appears exactly once in the lifecycle inventory.
- [x] Each personal-data class has a subject key, purpose, approval reference,
      retention, archive/terminal action, and documented exception state.
- [x] Financial, ownership, moderation, inbox/outbox, and audit records cannot receive a
      purge rule that violates reconciliation, replay, or approved exception constraints.
- [x] An unknown store, missing dependency, invalid action, stale policy version, or
      unapproved destructive rule fails closed before mutation.
- [x] Season reset, retention, export, erasure, and restore code can consume the same
      manifest without maintaining contradictory table lists.
- [x] Documentation distinguishes engineering controls and recorded decisions from a
      legal compliance conclusion.
- [x] Focused policy, schema-coverage, and safety regressions pass.
