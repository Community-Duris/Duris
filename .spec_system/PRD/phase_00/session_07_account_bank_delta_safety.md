# Session 07: Account Bank Delta Safety

**Session ID**: `phase00-session07-account-bank-delta-safety`
**Status**: Not Started
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

- [ ] Session 03 failure semantics prevent destructive completion after failed durable
      work.
- [ ] Database tests use an isolated development schema.

---

## Deliverables

1. Checked delta-only bank helpers in `src/sql_player.c` and their declarations.
2. Correct caller behavior and online-account synchronization in `src/utility.c` and
   other bank mutation paths.
3. Focused source-contract and isolated database regressions under `tests/async/`.

---

## Success Criteria

- [ ] No normal player-save or money path writes a cached absolute account-bank value.
- [ ] Deposit and withdrawal failures are detected and never presented as success.
- [ ] Successful operations publish the database-committed balance to all relevant
      online characters.
- [ ] A stale character cache cannot overwrite a newer shared-bank update.
- [ ] Focused regressions, formatting checks, and `make -C src` pass.
