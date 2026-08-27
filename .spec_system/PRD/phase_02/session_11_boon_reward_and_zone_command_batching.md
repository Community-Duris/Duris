# Session 11: Boon, Reward, and Zone Command Batching

**Session ID**: `phase02-session11-boon-reward-and-zone-command-batching`
**Status**: In Progress
**Work Window**: One progress-and-reward boundary covering in-memory eligibility,
immutable group fan-out, typed boon progress, account/gameplay rewards, zone-touch
records, delegation to critical domains, and bounded batch completion.

---

## Objective

Move boon, reward, and zone-touch outcomes out of synchronous lookup/update sequences
and raw scalar messages into typed commands that batch compatible effects and delegate
epic, currency, and item results to their authoritative domains.

---

## Scope

### In Scope (MVP)

- Inventory boon checks/progress/completion, account-bound and gameplay reward claims,
  quest/achievement rewards, zone touches, group fan-out, and related direct or queued
  SQL producers after the preceding sessions.
- Hydrate bounded active boon/reward/zone eligibility data or schedule refreshes so hot
  gameplay callbacks can decide and capture outcomes without synchronous database reads.
- Define immutable typed progress, completion, claim, grant, and zone-touch commands
  with stable IDs, source identity, participant lists, bounds, and schema versions.
- Batch compatible progress and zone rows set-wise; compose epic, currency, item,
  player-revision, and notification effects through Sessions 03 through 05 rather than
  mutating cached balances or ownership independently.
- Make repeatable/non-repeatable progress, cooldown, claim, and account-bound reward
  guards idempotent under duplicate callback, reconnect, restart, and concurrent claim.
- Replace raw scalar zone-touch SQL and direct absolute bank or full-player saves from
  the audited reward paths with typed transaction and outbox results.
- Expose refresh age, command batch size, duplicate, rejection, retry, and fan-out
  metrics without private account, character, or reward payloads.

### Out of Scope

- Phase 03 random-zone read-query rewrite, index selection, or retention policy.
- Boon/reward balance redesign or new gameplay rewards.
- General scheduled-maintenance staggering outside these domain producers.

---

## Prerequisites

- [x] Sessions 03 through 05 authoritative epic, currency, and ownership adapters are
      validated.
- [x] Session 02 outbox and reconciliation contracts are authoritative.
- [x] Account-bound reward snapshot and claim regressions remain passing.

---

## Deliverables

1. In-memory eligibility/refresh state and immutable boon/reward/zone command contracts
   in focused `src/` modules.
2. Set-based progress, completion, claim, grant, and zone-touch repositories integrated
   with authoritative domain transactions.
3. Cutover of audited direct SQL, absolute-bank, raw scalar, and full-save reward paths.
4. Focused repeatable/non-repeatable, group fan-out, concurrent claim, account-bound,
   zone duplicate, outage, restart, and composed-domain regressions under `tests/async/`.

---

## Success Criteria

- [ ] Audited boon, reward, and zone hot paths perform no synchronous database, Redis,
      or filesystem I/O on the simulation thread.
- [ ] One eligibility outcome or claim has one stable operation ID and cannot award or
      advance progress twice under duplicate or restart replay.
- [ ] Epic, currency, and item rewards commit through their authoritative ledgers and
      current rows rather than cached absolute saves.
- [ ] Group fan-out is bounded, immutable after capture, and applied set-wise with
      deterministic participant identities.
- [ ] Repeat, cooldown, completion, and account-bound claim guards remain correct under
      concurrent execution and ambiguous commits.
- [ ] Zone-touch records contain typed bounded fields and no raw SQL durable payload.
- [ ] Focused regressions, formatting checks, and `make -C src` pass.
