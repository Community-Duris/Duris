# Paid NPC quest cost assertion independent review - 2026-10-08

Disposition: PASS for captured paid-QP02 agreement and modeled sensitivity.
Code `0965b74a8d17ac4690c7039e09640e63031b2ba5` and
[exact handoff `6366b7ee5a354ce74454a5dc70aeddfd155b1566`](https://github.com/Community-Duris/Duris/blob/6366b7ee5a354ce74454a5dc70aeddfd155b1566/docs/persistence/economy_accounting/quest-prep/NATIVE_NPC_COST_ASSERTION_HANDOFF_2026-10-08.md)
are available on the preserved quest-prep branch. Primary import and native
qualification are not established. The helper is not mutation/recovery authority.

The coordinator independently read the complete new helper, test and handoff;
authenticated all 14 declared result/dependency/source blobs and the actual local
policy-header inputs; checked the two new code/test paths plus sole handoff path;
and confirmed existing capture/assertion helpers remain unchanged. Both commits
pass whitespace checks. Actual source comparison includes item-plus-fee capture
in economic_gameplay_authority.c:983-998, the native successor clocks, mapped
wallet/sink policy and denomination posting layout. Birth-operation/generation/
pre-charge stock revision/recipe slot is the correct item-plus-fee source binding;
the fee-only path has different identity fields and is not substituted.

Independently executed on the clean exact published quest worktree under WSL
Ubuntu-22.04 with TMPDIR/TMP/TEMP on `/mnt/d/Dev/Temp`:

```text
python3 -B tests/async/quest_accounting_prep/test_native_quest_cost_checks.py -v
python3 -B tests/async/quest_accounting_prep/test_quest_cut_checks.py -v
```

PASS: all nine new oracle methods (including sensitivity subcases) and all 17
unchanged assertion methods. No database, server, migration or native build ran.
These tests use modeled cuts and existing maintained decoders, not genuine births,
authenticated mapping, native execution or physical publication.

The callable checks the selected paid recipe, exact original recipient/birth,
one-transition clocks, original operation/source/receipt/claims, exact mapped
wallet effects and opposite sink denomination postings. It rejects additional
roots, duplicate/offsetting charges, mismatched denominations even at equal
scalar totals, second player debits, reward/XP/task/ACK effects and damaged input
consumption/residual rows in its narrow funded-before/consumed-after interval.
Existing static_complete and its historical player-fee semantics remain intact.

The documented limit is essential: coherently changed images/effects/postings can
agree without proving the maintained projector chose those denominations. A test
explicitly demonstrates this and the returned requirement for original-owner
proof. Full command/frozen projection, mapping authentication, canonical birth
carrier validation, complete same-cut native forest and physical/held-owner
publication remain external. An opaque available carrier is not authenticated
birth authority. Real paid four-hide setup and active numeric-GIVE refusal remain
unresolved. An unavailable observable cost-only interval must block the dependent
native check rather than be fabricated. Select existing compatible pack inputs
before importing this additive commit; do not import the older production tree.

## Next bounded quest preparation

Quest prep next owns new `native_quest_retirement_checks.py`,
`test_native_quest_retirement_checks.py` under its existing owned tests directory,
and `QP03_TEMPORAL_RETIREMENT_ASSERTION_HANDOFF_2026-10-08.md` under quest-prep docs.
Implement a temporal cut-consistency oracle for original A before/terminal,
later B birth, and stable stale-callback/recovery observations. Reuse maintained
mobile/account decoders and existing history/book patterns. Preserve existing
retired and all other controls. Do not require B to exist alongside A before D,
manufacture a replacement, or treat absence from a selected capture as global
absence/retirement proof. Require explicit observed A terminal evidence, original
stock/cash/source/receipt agreement and historical survival before accepting later
B evidence; B must have its own distinct observed identity/birth/source. A stale
attempt must preserve the original terminal history and the observed B state,
without a second reward/charge/retirement. Genuine chronological reset proof,
complete world census, mapping/physical D, paired holds and ACK remain external
owner requirements; modeled stage labels do not authenticate them.

Execute focused rejection sensitivity and applicable unchanged controls. No
production/driver/schema/authority edit, DB/server/build action, copied owner
codec or D implementation is assigned. Architecture's item-custody boundary map
continues independently. Current public source/migrations remain unchanged and
primary completion unobserved; actual native Goals stay BLOCKED, heartbeat ACTIVE.
