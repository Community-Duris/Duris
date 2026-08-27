# Code Review

## Findings Repaired

1. **High - generic player checkpoints could overwrite committed wallet state.** Wallet
   fields are no longer captured, legacy journal rows are explicitly ignored, and the
   legacy SQL update preserves the four wallet columns.
2. **High - initial ATM conversion still risked per-denomination partial deposit.**
   `deposit all` now submits one immutable four-denomination wallet/bank vector and
   publishes only its exact committed result.
3. **High - ordinary `ADD_MONEY`/`SUB_MONEY` and direct coin assignments could bypass the
   ledger.** Player credits/spends now enter the currency boundary centrally; audited
   direct mutation sites use those adapters, and a source inventory rejects regression.
4. **High - slot-machine wager and payout were initially separate commands.** The spin
   now preflights the fence and submits one net wallet result after the outcome, avoiding
   a second command colliding with the accepted wager fence.
5. **Medium - rejected auction credits could lose previously claimed staging value.**
   Auction money is restored on submission or semantic rejection; boon, ship-insurance,
   and generic reward submission failures stage safe auction-pickup fallbacks.
6. **Medium - shared bank publication initially lacked its committed revision.** The
   result now publishes all four balances and `bank_revision` to every playing alternate
   on the same account and racewar.
7. **Medium - boot could accept missing opening baselines.** Exact schema/index probes
   and wallet/bank coverage checks now fail closed; new player and bank creation inserts
   the corresponding baseline in the same transaction.
8. **Low - stale ATM rejection checked the wrong errno.** The callback now recognizes
   repository `ESTALE` and asks the player to retry without publishing success.
9. **Low - the repository harness lacked an explicit upper-bound case.** It now proves
   an `INT_MAX + 1` bank result commits only a stable `ERANGE` inbox result and no ledger
   or outbox row.

No unresolved blocking findings remain. Stable operation IDs, the critical journal,
inbox dedupe, canonical coordinator fences, and operation lookup provide retry/restart
convergence; authoritative state also hydrates reconnecting players if a process restart
outlives in-memory presentation continuations.
