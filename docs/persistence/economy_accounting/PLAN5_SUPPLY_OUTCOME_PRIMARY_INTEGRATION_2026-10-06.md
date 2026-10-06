# Supply outcome fix: primary integration — 2026-10-06

The supply view previously counted system postings belonging to rejected,
unknown or duplicate root operations. Importing the completed peer fix makes
only one uniquely identified committed root contribute to supply totals.
All existing audit findings remain visible. No producer, native source, schema,
accounting contract, activation behavior or detail budget changes.

Five owned files are imported byte-exactly from Plan5 milestone
`ceb9dbbb72502d6485b12df2a530b5a76c1307ff`. Their primary preimages match
the original peer base `7c0d6cf6cf475e717aa1e68d207bcb376e939980`.
The native and migration trees match the qualified peer; before import the
only code/test differences are the three imported Python files. All code/test
inputs now match the qualified composition. The entire central manifest,
including all integration rows, original providers, markers, counts and
deadlines, remains byte-identical. Unrelated SHOP harness edits are preserved.

Primary runs both new supply methods, zero skips, with exit0 in3.320s:

```text
python -B tests/async/test_reconcile_economy_accounting.py ReconciliationTests.test_supply_requires_one_committed_root_and_preserves_findings ReconciliationTests.test_supply_outcome_rule_covers_all_system_accounts -v
```

The cases cover clean/rejected/unknown roots, duplicate/conflicting identities
in both orders, all four system account kinds, detail limits0/1/100, unchanged
inputs, preserved global findings and alias restrictions. All three imported
Python files parse. Normal accounting contract validation, generated matrix
check and current61 runtime metadata pass on the primary composition.
Coverage remains false and release BLOCKED; metadata is no native qualification.

Import receipt `tmp/plan5-supply-outcome-primary-import-20261006.json`:
SHA256 `41c1ae283dad68a5dced195a523ce6ad7ac14bff0fed3771bbe295a82c6a4875`.
Actual primary metadata receipt `tmp/plan5-supply-outcome-primary-metadata-20261006.json`:
SHA256 `2eae645cc8fee621481235b98a448b38f300610c19f9c2dc84b4c4bad4f75bab`.

[The peer qualification](PLAN5_SUPPLY_OUTCOME_VIEW_QUALIFICATION_2026-10-06.md)
reports199 Linux methods and116 Windows reconciler methods, zero skips, plus
the complete original native baseline recipe on both SQL engines with its
original900-second limit. Its new SQL supply probes are explicitly modeled;
existing native opening books and equipment cold-restore checks remain.
Primary checks source/report correspondence but has not rerun native tests
or independently inspected peer-protected logs. These results do not qualify
full native system-root gameplay, complete capture, full backup/service restore,
private birth/recovery work, Plans or release. Accounting remains inactive.
