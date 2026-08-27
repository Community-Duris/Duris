# Implementation Summary

Epic balances now have one transactional authority. A guarded opening baseline and
immutable operation-keyed ledger reconcile to `player_data.epics`; every committed
mutation advances `epic_revision`, stores the exact inbox result, and emits one success
outbox event in the same InnoDB transaction.

Awards and guarded spends use typed immutable commands. The game thread publishes the
authoritative balance and dependent effects only after the matching completion, retains
completed effects for disconnected players, and drains completions during copyover and
shutdown. Generic player checkpoints and legacy flat-file replay can no longer overwrite
the transactional balance.

The cutover covers awards, bottles, level purchases, epic stores and skills, resets,
nexus training, spellbind, specialization, ascension, ships, unmulti, refunds, and the
administrator demotion reset. Legacy `epic_gain` evidence remains intact and is included
where historical award queries still need it.

Project version: `1.81.32`
