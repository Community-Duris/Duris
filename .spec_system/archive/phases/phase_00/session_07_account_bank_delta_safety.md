# Session 07: Account Bank Delta Safety

**Session ID**: `phase00-session07-account-bank-delta-safety`
**Status**: Complete
**Work Window**: The temporary shared-bank command boundary, including all callers,
authoritative committed balances, and multi-character stale-overwrite regressions.

---

## Objective

Stop characters from absolute-saving cached account-bank balances and make each
deposit or withdrawal use a checked database delta whose committed result is published
truthfully while the Phase 02 idempotent bank/wallet ledger is prepared.

---

## Scope

### In Scope (MVP)

- Remove normal calls that write all four account-bank balances from one character's
  cached copy.
- Make deposit and guarded withdrawal helpers check every ensure, update, query, and
  commit result and return the authoritative committed balance.
- Update command callers so failed bank work does not report success or leave a stale
  cached balance as if it were authoritative.
- Publish a successful committed balance to every online character on the same account
  and racewar side.
- Add focused regressions for two online characters, concurrent-looking stale caches,
  insufficient funds, database failure, and result-publication behavior.

### Out of Scope

- The Phase 02 unique operation ID, immutable ledger, wallet delta, outbox, and complete
  exactly-once transaction.
- Redesign of coin denominations or bank gameplay rules.
- Production write or concurrency testing.

---

## Prerequisites

- [x] Session 03 failure semantics prevent destructive completion after failed durable
      work.
- [x] Database tests use an isolated development schema.

---

## Deliverables

1. Checked delta-only bank helpers in `src/sql_player.c` and their declarations.
2. Correct caller behavior and online-account synchronization in `src/utility.c` and
   other bank mutation paths.
3. Focused source-contract and isolated database regressions under `tests/async/`.

---

## Success Criteria

- [x] No normal player-save or money path writes a cached absolute account-bank value.
- [x] Deposit and withdrawal failures are detected and never presented as success.
- [x] Successful operations publish the database-committed balance to all relevant
      online characters.
- [x] A stale character cache cannot overwrite a newer shared-bank update.
- [x] Focused regressions, formatting checks, and `make -C src` pass.

---

## Completion Summary

Completed on 2026-08-27. Cached absolute shared-bank saves were removed. Denomination
and aggregate operations now use checked owned transactions, arithmetic updates,
strict post-update reads, and commit-gated results. ATM, training, locker, boon, and
ship-insurance callers publish committed balances to all online same-account/same-side
characters and do not claim success after bank failure. Validation passed with an
ephemeral MySQL 8 regression and 174/174 repository tests plus signal-handler checks.
