# Code Review: Account Bank Delta Safety

**Reviewed**: 2026-08-27
**Base commit**: `b64912893cefb1c5f2f3e44c540042585ed8bb1d`
**Result**: RESOLVED

## Scope

Reviewed the complete Session 07 diff: bank SQL APIs and transaction ownership, strict result parsing, denomination and aggregate arithmetic, every direct mutation caller, online publication, failure messaging/compensation, source contracts, isolated MySQL coverage, and session records.

## Findings

### Critical / High

None.

### Medium - resolved

1. Aggregate denomination ceiling calculations initially used `int` addition and could overflow near `INT_MAX`. The additions now widen before division.
2. A guarded denomination update affecting zero rows initially always meant insufficient funds, even if an ignored ensure warning left no usable row. The helper now reads the row under lock and returns database failure when it is absent or malformed.
3. Private-chest creation happens before its legacy payment boundary. A failed bank payment now deletes the newly created chest and reports failure instead of granting it for free.

### Low - resolved

1. Online publication initially skipped a player's original character while its descriptor controlled another body. Publishers now target `original` when present.
2. No-MySQL aggregate helpers now initialize caller-provided results before failing.
3. Deposit-all failure text now states precisely that only failed denominations remain carried.

## Behavioral Review

- Every successful bank result was read after its arithmetic update in the same owned transaction and was exposed only after commit.
- Every command wallet mutation follows bank success; insufficient funds and database failure use different ATM messages.
- Aggregate payments no longer use `GET_BALANCE()` as authority and retain the existing smallest-denomination/change behavior.
- Case-insensitive account identity plus exact racewar identity prevents cross-account or cross-side cache publication.
- The remaining crash window between bank and wallet durability is explicitly reserved for the Phase 02 operation-keyed transaction.

## Verification

- Focused source and isolated MySQL regressions: PASS.
- C++20 warning-as-error build and changed-line formatting: PASS.
- Full suite: PASS, 174/174 plus signal-handler checks.

## Conclusion

All findings are resolved. The implementation is ready for validation.
