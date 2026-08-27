# Session Specification

**Session ID**: `phase00-session07-account-bank-delta-safety`
**Phase**: 00 - Correctness and Immediate Lag Removal
**Status**: Not Started
**Created**: 2026-08-27
**Base Commit**: `b64912893cefb1c5f2f3e44c540042585ed8bb1d`
**Work Window**: Remove cached absolute shared-bank writes and make temporary DB-authoritative deltas checked and truthfully published.

---

## 1. Session Overview

Normal deposit callers mutate wallet and cached bank state before ignoring database failure. Withdrawal returns a balance derived from a pre-update read, while `SUB_BALANCE()` and cash boons can overwrite all four shared balances from one stale character cache. Online alternate characters keep independent stale copies.

## 2. Objectives

1. Eliminate normal cached absolute account-bank writes.
2. Apply deposits and guarded withdrawals in checked transactions and return only committed authoritative balances.
3. Mutate carried money and report success only after durable bank success.
4. Publish committed denomination balances to every playing character on the same account and racewar side.
5. Preserve aggregate bank-payment denomination/change behavior without trusting cached balances.

## 3. Scope

### In Scope

- Account-bank SQL helpers and no-MySQL stubs.
- ATM deposit/withdraw commands, `SUB_BALANCE()`, cash boons, and online ship-insurance bank rewards.
- Online alternate-character cache synchronization.
- Focused source contracts and safe isolated/local database validation when available.

### Outside This Work Window

- Atomic bank-plus-wallet transactions, operation IDs, immutable currency ledger, outbox, and asynchronous economy gates reserved for Phase 02.
- Coin rules, schema changes, production writes, or production concurrency tests.

## 4. Technical Approach

Use short owned SQL transactions. Validate the ensure operation, apply arithmetic updates guarded against insufficient funds, verify affected rows, query the resulting row inside the same transaction, strictly parse cache-representable balances, and publish only after commit succeeds. For aggregate copper-value payments, lock and read the authoritative row, calculate the legacy denomination consumption and wallet change, apply only arithmetic deltas, then return the full committed vector. A utility-layer publisher updates all playing characters whose attached account name and racewar side match.

## 5. Deliverables

| File | Change |
|------|--------|
| `src/sql_player.c`, `src/sql_player.h` | Checked transaction helpers, committed results, and removal of cached absolute save API |
| `src/utility.c`, `src/prototypes.h` | Authoritative online-account publication and aggregate bank payment |
| `src/actoth.c`, `src/boon.c`, `src/ships/ship_base.c` | Failure-aware delta callers |
| `tests/async/test_account_bank_delta_safety.py` | Stale-cache, failure, guarded withdrawal, and publication contracts |

## 6. Success Criteria

- [ ] No normal path writes all four bank columns from a character cache.
- [ ] Ensure, update, result query, and commit failures never publish success.
- [ ] Withdrawal returns a post-update value selected in the same successful transaction.
- [ ] Wallet/cache mutation follows durable success at every command caller.
- [ ] All playing same-account/same-side characters receive committed balances.
- [ ] Focused tests, formatting, C++20 build, and full regression suite pass.

## 7. Risks And Resolutions

- **Partial multi-denomination payment**: calculate and apply the full vector in one owned transaction.
- **Commit ambiguity**: report failure and do not publish a claimed balance; Phase 02 adds operation identity and reconciliation.
- **Cache overflow**: strictly reject database values that cannot fit the legacy integer cache.
- **Scope expansion**: retain the existing synchronous temporary boundary and defer bank/wallet atomicity to Phase 02.

## Next Steps

Run the `implement` workflow step.
