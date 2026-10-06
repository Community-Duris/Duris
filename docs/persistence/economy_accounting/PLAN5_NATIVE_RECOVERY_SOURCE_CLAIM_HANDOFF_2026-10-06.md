# Original native recovery source-claim handoff

The original published `--money-recovery` program passes, while the independent
full canonical audit correctly refuses two committed roots with no matching
source-claim dedupe row. This occurs on both MariaDB10.11.14 and MySQL8.0.46,
canonical0062. Plan5 preserves that gate; it does not insert missing claims,
weaken the reader or independently edit shared producers/coordinator/contracts.

Exact Plan5 source is `6d4f708c98a7019780f8d4262a6a53e49a5f55ab` on `codex/accounting-plan5`, worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`;
native tree `4abb609524a1f1682ea4c190f82d75003c4d679b`. The original recipe/executable/arguments and
disposable database proof are in `PLAN5_ORIGINAL_OPENING_QUALIFICATION_2026-10-06.md`.
Protected evidence:
`D:/CodexEvidence/accounting-plan5/bin/opening-original-reference-green-07-20261006/`.
Each engine's `*-money-recovery-missing-source-claims.json` records these exact rows:

| operation_id | writer_id | reason | source_event |
| --- | --- | --- | --- |
| `15161718191a1b1c1d1e1f2021222324` | 11 | 30 | `0d000100030405060708090a0b0c0d0e0f1011120405060708090a0b0c0d0e0f10111213020000000000000000000000` |
| `161718191a1b1c1d1e1f202122232425` | 10 | 31 | `0d00010015161718191a1b1c1d1e1f20212223240405060708090a0b0c0d0e0f10111213030000000000000000000000` |

Both roots have lineage `4f505152535455565758595a5b5c5d5e`, epoch
`7778797a7b7c7d7e7f80818283848586`, committed outcome1 and result0.
The fixture creates them through actual settlement and item-claim leaf calls in
`tests/async/economic_sql_lifecycle_owner_mysql_harness.cpp:money_recovery_journey`.

## Narrow primary request

Assess the ownership boundary between those direct leaf fixture calls and the
maintained composed caller. For every committed root with a non-NULL source event,
`economic_accounting_source_claim` must retain exactly one row with the original
`lineage`, `source_event`, `operation_id` and `outcome=1`, bound to the committed
root/inbox in the same transaction. No orphan, rejected, changed or missing
claim can qualify. If the composed caller owns dedupe, the authentic fixture must
exercise that caller; if the leaf contract owns it, repair that real provider.
This handoff requests that assessment and original composed proof, not a schema
or wire-format change and not an audit-side correction.

Relevant shared providers are
`src/persistence/economic_sql_auction_settlement_transaction.c` and
`src/persistence/economic_sql_auction_item_claim_transaction.c`, with the shared
coordinator/composed ownership retained by the primary. Consumers are the
database-wide canonical audit, backup/restore evidence checks, snapshot/source
history binding and runtime retention/recovery qualification.

Repeat the unchanged original money-recovery program on both canonical engines
and require the standalone full canonical audit to pass its intact composed cut.
Retain native settlement/item collection/full cashout and cold/recovery cases,
original-ID replay and consumed-history refusals. Then remove/change each source
claim only in a separate disposable owner fault transaction: SELECT-only audit
must refuse, preserve all rows and roll back; restored original controls must
pass. Earlier0055 results or an isolated fixture dedupe insertion cannot qualify
the current composed source. Genuine financial/publication/ACK coverage remains
separate from this synthetic activation fixture.

The shared notebook remains primary-curated and nonblocking. This is a remote
Plan5 curator request; it is not an independently implemented producer repair,
primary acknowledgement or release/activation waiver.
