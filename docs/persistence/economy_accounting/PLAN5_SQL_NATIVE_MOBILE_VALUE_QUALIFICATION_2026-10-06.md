# Plan 5: independent retained SQL native-mobile value qualification — 2026-10-06

## Delivery and exact source

- Branch: `codex/accounting-plan5`, published only to that remote branch.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Base: `5207af7f83c9cdcf2bd0494d0f3aeb7b9a2413ef`.
- Separate solved-issue commit: `51cfdc7b14fc45649e74b5e67e042c86df80f27f`.
- Primary checkpoint included in the base: `8dc0f98eb01bee9e40d227debb3e556d2d0fdbcf`.
- Native `src` tree: `03a97173396f720857b1ad58a2ab69b7859a2387`.
- Canonical migration tree: `2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`; maintained head remains
  `0061_economic_baseline_equipment`.
- All 3122 tracked code/test/schema inputs in the fix are byte-identical
  to the isolated green source. The report publication tip is bound separately
  by `tmp/plan5/sql-native-mobile-value-delivery.json` and remote verification.

The primary branch was refreshed before choosing this base. No runtime,
coordinator, producer, shared accounting contract, migration, registry/matrix
or activation source was edited. All previous branch tips remain ancestors.
The shared notebook remains with its primary curator; this owned report and
sealed receipts are its evidence packet, without rewriting that notebook.

## Established defect and complete owned fix

The original SQL restore/canonical evidence readers never inspected
`quest_mobile_native` values. Migration0059 verifies table shape, not its values.
A candidate could therefore be qualified with a corrupt image or a row whose
mobile identity disagreed with its valid retained reference.

The new regression failed against the frozen original reader with
`RuntimeError not raised`: a damaged native-mobile image was silently ignored.
That original code also falsely qualified the ID/image disagreement on fresh
canonical61 MariaDB and MySQL candidates. Their original SQL driver failed at
`native-mobile-ID binding` with `corrupt canonical evidence was admitted`.
These red failures establish the defect; they are not passing qualifications.

The independent Python evidence reader now parses QMNIMG v1/v2 and QMNREF v1,
without importing native mutation, recovery or image codecs. It binds each SQL
row's `mobile_instance_id`, `mobile_revision`, `stock_revision` and
`lifetime_state` exactly to its image. It checks both checksums, lengths,
reserved bytes, original reference/source grammar, nonzero operation IDs,
nonreserved lifetime/stock UIDs, literal stock, equipment order, contiguous DFS,
shared nested-row budget, strings and depth, cash revision/denominations, and
retired stock/v2 cash. Historical v1 cash remains **unobserved**, including in a
retired image; the reader never creates a zero balance or a money holding.

The existing SELECT-only capsule reader supplies 64 KiB chunks and the 4 MiB
native image bound. The new namespace uses 256-row numeric-ID pagination,
including unsigned lifetime IDs above INT64_MAX. All retained rows are checked;
there is no selected-book filter or repair. The standalone canonical audit now
requires this InnoDB source, bounds its collection at100,000 rows, includes its
image bytes in the32 MiB aggregate capsule budget and returns the additive
`retained_native_mobiles` count. Caller transactions and rollback remain intact.

This verifies retained format and row bindings. A valid capsule does not
establish admission of its birth/source, authenticate a transition receipt,
compare complete live-world custody or supply activation authority.

## Ownership and shared interfaces

The issue commit owns exactly:

1. `scripts/economic_restore_evidence.py` — independent image interpretation and
   bounded retained-row checks, reused by restore and canonical audit.
2. `scripts/economic_sql_canonical_audit.py` — source/collection/byte budgets and
   the native-mobile count in the existing operator result.
3. `tests/async/test_economic_sql_canonical_audit.py` — original-reader regression,
   exact projection types/bindings, controls, stock/topology/depth and budgets.
4. `tests/async/test_flatfile_restore_economic_authority.py` — native oracle
   comparisons added to the original complete restore method.
5. `tests/async/run_restore_accounting_evidence_mysql.py` — original canonical
   disposable-database method, both SELECT-only consumers, image corruptions,
   full table inventories and two-page checks.
6. `docs/persistence/economy_accounting/AUDIT_OPERATIONS.md` — operator scope.

No shared interface or schema change is requested for this fix. Public native
fields, versions, source providers, flags and original test assertions remain.
The existing native qualifier and fixture CPP are unchanged. The extra grammar
fixtures are explicitly modeled; the original native decoder accepts/refuses
those same bytes. They create no born mobile or admitted source authority.

## Exact validation and retained evidence

Pinned Linux tool image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
GCC13.3.0; networking disabled,2 CPUs,4 GiB, private2 GiB workspace and temporary
filesystems, build cache off. No project `.env`, production connection, live
checkout/player-data mount or real account was used. All three containers are
terminal with observer exit0 and no OOM; underlying red method exit1 remains
recorded as failure evidence.

With `PYTHONPATH=tests/async`, original focused regression:

```text
python3 -u -B -m unittest -v test_economic_sql_canonical_audit.RestoreProjectionTests.test_sql_native_mobile_corruption_is_not_qualified
```

- Original reader: exit1, 0.215319s, expected missing refusal.
- Fix: exit0, 0.216318s, zero skips.

All focused original pure classes:

```text
python3 -u -B -m unittest -v test_economic_sql_canonical_audit.RestoreProjectionTests test_economic_sql_canonical_audit.CanonicalAuditTests
```

Passed26 methods, exit0, 0.366267s, zero skips. A preceding local
Windows development check passed the same26 methods; the sealed Linux run is
the delivery qualification. Earlier development fixture errors (source-kind
framing and one mock count sequence) were corrected before that sealed run.

Original full native flatfile restore method:

```text
python3 -u -B tests/async/test_flatfile_restore_economic_authority.py
```

Passed, exit0, 164.095802s, zero skips. There are60 independent/native
image-value decisions,8 valid images and69 refused image/file cases across154
preflight/final native calls. The original20 valid economic stores,367 corrupt
refusals,54 semantic decodes,50 generic semantic cases and1,058 native metadata
comparisons remain. Flags, providers and original deadlines are preserved.
The qualifier/fixture hashes are original printed hashes (their original
TemporaryDirectory cleanup means these two binary files are not retained):

- Qualifier: `d4dada8d8033f9e424590ed7076930477638585e4e8c76876672028b7081325d`.
- Fixture: `78a9f8b006f09e2eccb45c23f59b421d4fcb4b1bbc27003ba1a8cef59c629af0`.

Original canonical native/SQL method, with
`DURIS_RUN_RESTORE_COIN_INTEGRATION=1`,
`DURIS_PLAN5_CANONICAL_EVIDENCE=1` and cache off:

```text
python3 -u -B tests/async/test_restore_economic_coin_effects.py
```

Passed, exit0, 312.715992s, zero skips. Original SQL and flatfile
ASan/UBSan coin fixtures were freshly compiled, with0 reused objects. Their
32 native cases and3,026 independent canonical-decoder decisions agree between
both modes. On each fresh engine, the original maintained bootstrap and
migration runner install canonical61, then SELECT-only roles exercise:

- 45 native-mobile cuts through both original read-only consumers, including
  v1/v2 live/retired controls, literal stock, row-field mismatch, checksum/body/
  length/size errors, topology, maximum lifetime and259-row pagination with a
  second-page corruption.
- 84 total canonical cuts,72 refusals and51 full-entry cuts, retaining the
  original259-root page, intent/plan bounds and surrounding receipt/evidence
  cases.
- 30 original native coin cases and2 canonical SQL constraint refusals.
- Complete captured table rows before/after every check, including native images,
  remain unchanged by both readers.

Engines: `10.11.14-MariaDB-0ubuntu0.24.04.1` and
`8.0.46-0ubuntu0.22.04.4`. These are synthetic format/evidence tests, not player
journeys, native-mobile birth/recovery, a dump/import cycle or managed service
boot. No new complete server Make build was needed: no C/C++ source changed.
The original focused native builds above were run; prior production builds
retain their separately recorded scope.

Frozen-original SQL reproduction uses exact base bytes of both readers and
this same original driver on fresh private daemons:

```text
python3 -u -B tests/async/run_restore_accounting_evidence_mysql.py
```

Both methods exit1 at the genuine false qualification after passing four valid
image controls. MariaDB:33.640089s; MySQL:108.275853s. The retained original
native fixture is reused only here (not in green), with its current source
unchanged and SHA256
`23cef4c8d3544a383cd9fb252fb2d65deb4924fb6c53d421c9a0f7f5ca57289a`.
The source archives, execution/preparation helpers, environment guards, exact
commands and versions are retained. Python syntax and Git whitespace checks
pass; no C/C++ formatting change is present.

Protected evidence root: `D:/CodexEvidence/accounting-plan5/bin/`:

- `sql-native-mobile-red-01-20261006/` — original missing-refusal regression.
- `sql-native-mobile-green-01-20261006/` — complete fixed method batch and retained
  original native SQL/flatfile binaries/corpora/logs.
- `sql-native-mobile-red-engines-01-20261006/` — frozen-original both-engine
  reproduction, authentic reused native fixture and exact source composition.
- `sql-native-mobile-final-seal-01-20261006/evidence.json` — manifest binding all
  3122 code inputs and42 artifacts
  (332,553,994 bytes retained evidence;
  this is evidence storage, not measured workload growth).

Seal SHA256: `c3cf2f810a63cd3f3f1aadd196539aef064634b08eee0b18ec7d37a402b713e8`.

## Remaining gates and curator action

Integrate the separate issue commit and consume this report/receipt in the
primary's notebook. No central route/registry status or release gate is promoted.
This qualified source remains canonical61; the primary's reviewed private0062
claim-allocation candidate is not installed or qualified by these results.
Its immutable source identity, partial consumption, mutually exclusive historical
whole-link path and versioned opening origin must be independently authenticated
once maintained source is available.

The partial SQL snapshot exporter still lacks complete native-mobile/live-world
capture and comparison. Real original producer, birth, item/cash transition,
ACK/replay/cold restart, typed erasure, complete backup/restore/retention and
release-host mixed-workload budgets remain gates, alongside the full activation
verifier and writer census. Current valid images alone cannot close any of those.
No accounting activation, experimental-accounting push, PR merge, deployment,
production data access or audit correction was performed. Wallet-root exclusions
and the declined inactive spell-path change remain unchanged. Plan5 and release
remain incomplete; further owned work is available.
