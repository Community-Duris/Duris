# Implementation Summary

Player wallets and shared account banks now have one transactional currency authority.
Every accepted denomination vector locks the player then bank, validates both revisions
and bounds, and commits both materialized vectors, both revisions, an immutable ledger
row, exact inbox result, and success outbox in one InnoDB transaction.

ATM denomination operations and `deposit all`, aggregate bank payments, wallet rewards
and spends, auction pickups/refunds, cash boons, ship insurance, wagers, coin pickups,
and audited direct mutations now cross the typed critical-command boundary. The game
thread publishes the exact wallet ACK to the initiating player and the exact shared bank
ACK to all online same-account/same-racewar characters, with disconnect retention and
lifecycle drain routing.

Opening wallet/bank baselines reconcile exactly to the immutable ledger. Generic player
checkpoints, legacy SQL updates, and flat-file bank slots can no longer overwrite the
transaction-owned state, while SQL hydration and new player/bank initialization remain
authoritative.

Project version: `1.81.33`
