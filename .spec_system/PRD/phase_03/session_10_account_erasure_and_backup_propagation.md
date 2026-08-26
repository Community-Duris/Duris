# Session 10: Account Erasure and Backup Propagation

**Session ID**: `phase03-session10-account-erasure-and-backup-propagation`
**Status**: Not Started
**Work Window**: One authenticated erasure boundary from stable request and domain
fencing through ordered delete/pseudonymize/retain actions, cache and file cleanup,
durable tombstones, backup-restore propagation, reconciliation, and exact completion.

---

## Objective

Replace the empty account-deletion handlers and partial character deletion with an
idempotent end-to-end workflow that applies the approved lifecycle policy once and
prevents deleted personal identity from reappearing after replay or restore.

---

## Scope

### In Scope (MVP)

- Define password-reauthenticated two-step account erasure with one stable request ID,
  request cooling/status rules, cancellation boundary, redacted audit, and no
  name/account value in operational logs.
- Fence new account and character mutations, resolve online descriptors, and drain,
  reject, or safely retain pending Phase 01 snapshots and Phase 02 domain commands
  before identity-bearing rows change.
- Execute manifest-ordered cascade, delete, pseudonymize, archive, and retain-exception
  actions across accounts, characters, IPs, messages, profiles, ledgers, ownership,
  auctions, lockers, guilds, PvP, moderation, journals, outboxes, caches, pfiles,
  exports, and local logs.
- Use Phase 02 domain commands for item, currency, auction, and other asset disposition;
  never orphan or destroy value through ad hoc erasure SQL.
- Preserve required financial, ownership, moderation, and operation evidence with a
  stable non-reversible pseudonymous subject token and no direct account/name/IP/email
  linkage when policy requires retention.
- Record a durable erasure tombstone and make journal replay, pfile import, cache
  rebuild, migration tools, and backup restore apply it before the identity can be
  loaded or service reopened.
- Verify every store and reconciliation invariant before deleting credentials and
  reporting exact completion; failure remains resumable and never reports a partial
  request as complete.

### Out of Scope

- Deleting records whose approved policy requires retention, or making legal decisions
  about an erasure exception during execution.
- Editing historical backup archives in place; restore-time tombstone application is
  the supported propagation contract.
- Production erasure tests using real accounts or player data.

---

## Prerequisites

- [ ] Sessions 07 through 09 lifecycle, archive, and authenticated access contracts are
      validated.
- [ ] Phase 02 ownership, currency, auction, inbox/outbox, and reconciliation commands
      support safe disposition and pseudonymous retained history.
- [ ] Erasure tests use isolated synthetic accounts and restorable non-production
      backups.

---

## Deliverables

1. Authenticated erasure request/state/tombstone schema, bootstrap synchronization, and
   verification under `migrations/`.
2. Manifest-driven ordered erasure coordinator integrated with account/character menus,
   descriptors, Phase 01/02 commands, recovery, caches, files, exports, and shutdown.
3. Backup-restore preflight that reapplies tombstones before login, replay, import, or
   external publication, plus operator status/retry/reconciliation tooling.
4. Focused reauthentication, cancellation, online/pending-work, asset disposition,
   retained-ledger pseudonymization, crash-point, duplicate, cache/file, backup-restore,
   and no-resurrection regressions.

---

## Success Criteria

- [ ] An erasure request can be created only after account reauthentication and is
      idempotent across duplicate input, retry, crash, and restart.
- [ ] New account/character mutations are fenced and pending persistence reaches one
      documented disposition before identity-bearing deletion begins.
- [ ] Every manifest entry reaches its approved delete, pseudonymize, archive, retain,
      regenerate, or tombstone state and reports exact per-store evidence.
- [ ] Currency, items, auctions, lockers, and other value cannot be duplicated, lost,
      or orphaned by the erasure workflow.
- [ ] Retained history has no direct subject identifiers beyond the approved
      non-reversible token and continues to satisfy ledger/reconciliation constraints.
- [ ] Restoring any tested backup and replaying journals cannot recreate the erased
      account, character, cache identity, pfile, export, or live login.
- [ ] Credentials are removed and completion is reported only after every required
      action and reconciliation check succeeds; failure remains safely resumable.
- [ ] Focused regressions, isolated schema/restore tests, formatting checks, and
      `make -C src` pass.
