# Primary pair-batch and original-opening combined qualification

All work remains on `codex/accounting-plan5`, worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
After refreshing `experimental-accounting`, observed primary is
`5759a4783f7785486e8d1ec5592fbf17d6020d3b`. Plan5 base is `df22d682eca46ece62d2f5a9d38b97e5e8bf7a05`;
completed exact import is `8a9a6f0cf72fbe25f6daeb102924f3e949e77134`.

## Established defect and exact import

The primary's unchanged two-account SQL regression reproduces an intact-cut
failure on both MariaDB and MySQL at the old collector's
`invalid_source_roots == 0` assertion. The Cartesian operation/account predicate
returns a pair from an earlier batch and counts that effect again. Red has two
engine failures, no skips and unchanged frozen sources; original native coin
and independent decoder cases already pass before the SQL failure.

The three-line published filter counts only pairs requested by the current
64-pair batch. Real duplicate rows within that batch still increment the
original counter and refuse. The two affected files are
`scripts/economic_sql_audit_snapshot.py` and
`tests/async/run_restore_accounting_evidence_mysql.py`, both exact primary blobs.
The original two-account control has 68 source rows, 66 extra distinct pairs,
two accounts in the overlap root, correct remaining native cash, unchanged
SELECT-only authority and complete fixture restoration. Its copied metadata
is modeled; it does not authenticate a new capsule or real producer.

Five other affected primary inputs already match on this branch:
`scripts/reconcile_economy_accounting.py`,
`tests/async/run_economic_sql_audit_snapshot_mysql.py`,
`tests/async/run_native_sql_baseline_audit.py`,
`tests/async/test_reconcile_economy_accounting.py`, and
`tests/async/test_restore_economic_coin_effects.py`.
Their earlier work is preserved, including exact integer UID provenance and
partial-allocation/current-claim reconciliation. The four new policy/PID
reader/test files from `6d4f708c98a7019780f8d4262a6a53e49a5f55ab` are unchanged.
This is source integration and combined component proof, not a transfer of
earlier passing results to an untested candidate.

The three shared plan/checkpoint notes and two primary qualification notes are
imported as exact published blobs. No independent coordinator, contract,
producer, writer-registry/matrix, schema or activation-owner edit is made.
No new shared interface is requested by the pair filter. The existing narrow
[source-claim handoff](PLAN5_NATIVE_RECOVERY_SOURCE_CLAIM_HANDOFF_2026-10-06.md)
remains open.

## Frozen source and execution

Source archive SHA256 is `ec69a9d9f1ec66bdd2595d0a6b363e56f706c98f60b75bcd0a1be1753e3c694e`.
All 6364 regular source files match the committed bytes, with four repository
links. Every executed source/test mode matches. Two newly added Markdown notes
were manually frozen with mode 0644 while Git exports mode 0664: the primary
partial-snapshot and UID-provenance qualification notes. Their bytes match.
The first strict seal refuses this packaging difference; its archive, helper
and exact failure are retained under `primary-pair-combined-seal-01-20261006/`.
The final seal explicitly records only these two documentation-mode differences;
it does not claim full file-mode equivalence.

Native tree remains `4abb609524a1f1682ea4c190f82d75003c4d679b`;
migration tree remains `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`, canonical 0062/all 62 receipts.
The pinned private image remains
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
GCC 13.3/Python 3.12, MariaDB 10.11.14 and MySQL 8.0.46.
No network, host port, maintained database or environment file is mounted.
Each container has 2 CPU/4 GiB and private RAM workspaces. The unchanged coin test
requires a temporary directory under `/`, so its container has a writable
private root; the separate original-native container has a read-only root.
Original sanitizer flags, providers, assertions and individual deadlines stay.
Outer observation limit remains 7200s; no run is restarted or OOM-killed.

Complete original commands and actual results:

- `python3 -B tests/async/test_restore_economic_coin_effects.py` — exit0, 1 discovered methods, 0 skips, 439.613250s. Environment: `{"DURIS_PLAN5_CANONICAL_EVIDENCE": "1", "DURIS_RUN_RESTORE_COIN_INTEGRATION": "1"}`. Log SHA256 `9bcefec1f4be802a89970b87e1769b466cb312234f35cfc11ac6590fc761f6b9`.
- `python3 -B tests/async/test_economic_sql_audit_origins.py` — exit0, 46 discovered methods, 0 skips, 78.192865s. Environment: `{"DURIS_RUN_ECONOMIC_ORIGIN_INTEGRATION": "1"}`. Log SHA256 `214b965d3fb1b7041e1e71c3a5768b1a9b82573a564f984464aec668bd283ba7`.
- `python3 -B tests/async/test_economic_sql_canonical_audit.py` — exit0, 35 discovered methods, 0 skips, 130.704379s. Environment: `{"DURIS_PLAN5_CANONICAL_ARTIFACTS": "/workspace/bin/tests/primary-pair-canonical", "DURIS_PLAN5_CANONICAL_MOBILE": "1", "DURIS_PLAN5_CANONICAL_NATIVE": "1", "DURIS_PLAN5_CANONICAL_SOURCE": "1"}`. Log SHA256 `7595ce03c5141315c0081a74efa4d915c161993f70850432ca3d98a844905d50`.
- `python3 -B tests/async/test_reconcile_economy_accounting.py` — exit0, 134 discovered methods, 3 skips, 33.703609s. Environment: `{}`. Log SHA256 `6b2dd261eed8e070b125ec17d59720b445ee87ab33556aa947507b5c0884ecd5`.

The first three suites pass 82 methods with zero skips. The reconciliation file
passes 131 executed pure methods and discovers three opt-in methods skipped by
its original decorators: native SQL stake, near-limit prices, and near-limit
mapping snapshots. Their integration/budget flags were not enabled here.
Total 213 executed methods pass, three methods skip. The observer incorrectly
requires zero skips for every file and therefore exits1 after all four test
processes exit0. The whole failed observer, logs and terminal result are retained;
no test, skip or assertion is removed to turn that result into a global pass.

The canonical suite retains both fresh SQL/flat native probes and all 88 original
SQL cuts, including source/mobile variants on both engines. Each engine's full
coin/restore suite passes 109 cuts, 58 full-entry checks and 90 expected refusals,
plus the pair-batch control. Original wire fixtures retain 32 cases and 3026
independent decoder decisions, with 1054 accepted and native agreement.
No fresh maintained `make` is claimed: C/C++ is unchanged, and the existing
SQL/flatfile 740-object build evidence has this exact native tree.

## Original native references on the combined source

The unchanged original compiler recipe from
`tests/async/run_economic_sql_lifecycle_owner_mysql.sh` is recorded verbatim in
`primary-pair-original-native-01-20261006/native-command.json`, with 43 real
providers exactly once. Its recipe SHA256 remains
`10db51c8b4c64edf13f94b3caac289d141fe38796cd226092028e9119769e995`.
Fresh compilation exits0 in 137.971744s;
binary SHA256 is `c8f2ebc1fa25c0b76eec75bf9d759ad4b91a66c4d4f8aa8222fd7c1e6f32fb1b`.
The prior successful original observers/loaders are copied byte-for-byte.

Original programs `lifecycle-owner`, `lifecycle-owner --money-opening`, and
`lifecycle-owner --money-recovery` run on both fresh canonical engines:
six cases exit0, eight genuine emitted reference vectors are independently
recomputed, the two readers agree, and formerly accepted corruptions now refuse
with zero false accepts. Full-table inventories stay unchanged; reader UPDATE
is denied 1142. This source is freshly exercised, not only compared by inventory.

The first two modes retain inactive SQL controls. The unchanged money-recovery
mode exercises its private synthetic three-route activation/recovery fixture;
that scope is explicitly active. It is not maintained activation or a genuine
complete R6 verifier. Its full canonical audit still refuses the same two
committed roots missing source-claim dedupe rows on each engine. The owned
handoff retains their exact fields, producer ownership question, consumers and
required composed tests. No missing claim is inserted by Plan5.

## Evidence and remaining gates

Protected evidence is `D:/CodexEvidence/accounting-plan5/bin/`:
`primary-pair-combined-red-01-20261006/`,
`primary-pair-combined-green-01-20261006/`,
`primary-pair-original-native-01-20261006/`, and
`primary-pair-release-gates-01-20261006/`.
Final seal is `primary-pair-combined-seal-02-20261006/evidence.json`, SHA256
`ac0635bb57d962a7407eb719bca09d361dedc74af665d2161c8eb4f38f335ce3`. It inventories 3800 regular artifacts,
2805865984 bytes and 0 links, without following links, including
the failed first seal. Frozen inputs, executed helpers, original console output,
all test logs, native binaries and private results remain protected outside Git.

Normal `python -B scripts/validate_economy_accounting.py` passes.
The unchanged `--release` command exits1 with
`writer has no executable evidence`. Three opt-in methods above are not newly
qualified. The shared source-claim and actual player-inspector provider handoffs,
genuine borrowed-connection activation verifier, complete R6 capture,
producer/publication/ACK/lost-reply/cold-world journeys, typed active erasure,
full flatfile parity, managed restore/retention and release-host mixed 1000-root
workload budgets remain open. Private 0063/fee/HRT/quest/birth source is not
installed or qualified by this source. No full Plan/R1–R8 or release completes.

Wallet-root exclusions, maintained inactive behavior and the declined inactive
spell change are preserved. No production data, deployment, PR merge, maintained
activation or audit correction occurs. `AI_CONTEXT.md` is absent from this
separate worktree and the supplied original workspace; available AGENTS/README,
Plan5, remaining requirements and current published checkpoint were read.
This report, protected seals and remote followup are the curator packet. The
primary's locally maintained notebook remains nonblocking as the user directs;
no primary acknowledgement or notebook application is claimed.

The final refresh observes primary `4ad525878f30e3aad2f1bbd86840a8ef31b86511`. Its only changes after the
qualified dependency `5759a4783f7785486e8d1ec5592fbf17d6020d3b` are four published documentation files,
imported exactly into this delivery. Native, migration, script and test trees
are unchanged. Its genuine birth publication report records peer-executed
private 747-unit builds/schema 0063 publication, with native-wallet cold recovery
and authentic world restoration still underway. Those private source and
artifacts are not independently inspected or qualified here. This Plan5 seal
retains its exact canonical 0062 reader/native component scope.
