# Plan 5 retained claim allocation qualification — 2026-10-06

Restore and the standalone canonical SQL audit previously ignored retained
pending-claim sources, partial consumptions and declared opening-origin slots.
Two focused regressions establish that well-shaped unattached rows were
incorrectly qualified. Separate owned fix
`bfbc513e8b959a03613da5b3f366575c36398eb1` binds these projections to the original
independently decoded accounting effects. Both original consumers pass native
codec controls and refuse damaged allocations on private canonical-0062
MariaDB and MySQL. This closes that retained-projection omission; source
capture, claim producer journeys and complete release remain unqualified.

## Exact branch, source and ownership

All work and follow-up stay on remote `codex/accounting-plan5`, in worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.

| Role | Exact commit |
| --- | --- |
| Previous Plan 5 report tip | `b470fd6356443d5eab20d4ac06644846e3139069` |
| Primary executable-mode correction | `4e006c3a738d00c19ab3d09f573b57c5115e2033` |
| Exact mode import / owned fix base | `dbc3dd66d6b6d52567653ae9e84f9a0ff4dfff3e` |
| Separate owned fix | `bfbc513e8b959a03613da5b3f366575c36398eb1` |
| Latest fetched primary | `ad2ebe4dbe803f429fa5195ad4671f94499f1f29` |
| Exact two-parent refresh merge | `229515bd5b260d92341c0c28bb995f5e6b49aa37` |

The fix owns seven files:

- `scripts/economic_restore_evidence.py`;
- `scripts/economic_sql_canonical_audit.py`;
- `tests/async/restore_coin_effects_fixture.cpp`;
- `tests/async/run_restore_accounting_evidence_mysql.py`;
- `tests/async/test_economic_sql_canonical_audit.py`;
- `tests/async/test_restore_economic_coin_effects.py`;
- `docs/persistence/economy_accounting/AUDIT_OPERATIONS.md`.

The refresh imports exactly the primary's updated review checkpoint, finish
plan, remaining requirements and SQL native-mobile qualification report.
Four owned conflicts preserve the tested fix byte-for-byte: the incoming
implementation was already in the previous Plan 5 tip, with only three stale
0061 test/output literals differing. The earlier solved 0062 labels are
preserved. Native tree `f0ae5c63273e94035552a75a1b70596d5021e54d` and migration
tree `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` are unchanged by this refresh.
All seven preserved earlier branch tips remain ancestors. No shared native
implementation, producer, schema, registry/matrix, activation owner or contract
is independently edited. The 0062 executable bit comes from the primary's
actual committed correction, with unchanged immutable bytes.

## Complete component behavior and limits

The reader uses SELECT only. It requires InnoDB source, consumption and
mapping tables, bounds each allocation collection to100,000 rows and reads
256-row pages in increasing compound primary-key order. It requires exact
integer IDs/slots/amounts, nonzero original source/spending IDs, canonical SQL
claim mapping/context/backend, matching original mapping/PID/lineage and
successful committed roots/inbox references. Retired mappings need no active
PID. It does not consult mutation/provider logic to determine validity.

Original positive amounts remain immutable. Legacy whole-consumption links
and separate partial rows are mutually exclusive per source. Partial sums
cannot exceed the original amount; retained fully consumed sources still
contribute their original credits. Aggregated credits/debits must exactly
match each original pending-claim account effect, including generic successful
debit reasons. Missing, extra, orphaned, rejected, mismapped, mixed and
overdrawn allocations refuse with fixed diagnostics. Readers never correct
rows, change source amounts or choose a consumption policy.

Declared SQL `claim_origin_version=1` also requires exact canonical holding
index+1 source slots, counts, account keys and original positive copper amounts;
zero openings create no lot. Historical NULL opening metadata retains unknown
allocation coverage. Table consistency alone does not authenticate the
original PID or derive the origin-policy selector from frozen evidence.
[The narrow primary handoff](PLAN5_CLAIM_ORIGIN_SELECTOR_HANDOFF_2026-10-06.md)
names exact fields, invariants, consumers and required genuine reference/
corruption/replay tests. No defect in the unpublished opening producer is
asserted. Snapshot export/reconciliation still lacks partial-consumption
coverage. The additive audit field `retained_pending_claim_allocations` reports
only the retained component check; `source_capture_qualified` and
`release_qualified` remain false.

## Failure and exact native/disposable validation

The retained red transport uses base `b470fd6356443d5eab20d4ac06644846e3139069`
with only the focused test overlay and the original readers. On Windows,
`PYTHONPATH=tests/async`, this command fails with two `RuntimeError not raised`
assertions, exit1 in0.672000s:

```text
python -u -B -m unittest -v test_economic_sql_canonical_audit.RestoreProjectionTests.test_unattached_pending_claim_allocation_is_not_qualified test_economic_sql_canonical_audit.RestoreProjectionTests.test_unattached_pending_claim_consumption_is_not_qualified
```

The passing native/disposable observations authenticate raw Git source bytes
and modes, using the mode-import base plus six owned code/test overlays.
The offline Linux tools image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Each has two CPUs/4 GiB, private workspace/tmp mounts, no network and no checkout
`.env`. Exact environment:

```text
PYTHONPATH=/workspace/tests/async
PYTHONDONTWRITEBYTECODE=1
DURIS_REGRESSION_BUILD_CACHE=off
DURIS_RUN_RESTORE_COIN_INTEGRATION=1
DURIS_PLAN5_CANONICAL_EVIDENCE=1
DURIS_RUN_NATIVE_BASELINE_AUDIT=1
DURIS_PLAN5_CANONICAL_NATIVE=1
DURIS_PLAN5_CANONICAL_ARTIFACTS=/workspace/bin/tests/claim-allocation-original-audit
```

| Original command | Result / observer seconds | Scope |
| --- | --- | --- |
| `python3 -u -B -m unittest -v test_economic_sql_canonical_audit.RestoreProjectionTests test_economic_sql_canonical_audit.CanonicalAuditTests` | Exit0 /0.516385 | All33 pure reader methods; typed, malformed, reference, collection, versioned-origin and historical controls. |
| `python3 -u -B tests/async/test_restore_economic_coin_effects.py` | Exit0 /295.217572 | Original full native/SQL restore method;32 native coin cases,3,026 native/independent decoder decisions, four allocation controls, both fresh canonical0062 engines. Each engine passes104 cuts/88 refusals before the pagination addition. |
| `python3 -u -B -m unittest -v test_economic_sql_canonical_audit.NativeCanonicalAuditTests` | Exit0 /142.955719 | Original full two-engine canonical audit, native SQL/flatfile probes and saved-projection/operator checks. |
| `python3 -u -B tests/async/test_native_sql_baseline_audit.py` | Exit0 /512.612698 | Original SQL/client-free native baseline, both canonical engines,161 cuts/seven constraints per engine, actual dump/import and six equipment/custody phases;229 application-table data remains unchanged. |
| `python3 -u -B tests/async/test_restore_economic_coin_effects.py` after one driver-only pagination addition | Exit0 /322.453408 | Both engines pass105 cuts/89 refusals,55 full-entry cuts and259-root pagination. Each has21 allocation cuts, four valid controls, both readers and a258-source/257-consumption second-page corruption cut. |

All these methods finish within their original outer deadlines:60 seconds for
pure readers,1,800 seconds for restore and2,400 seconds for audit/baseline.
Original providers, compiler/sanitizer flags, assertions and child deadlines
remain. Zero skips, zero reused native objects, no compiler/sanitizer failures.
Connected engines identify MariaDB10.11.14-MariaDB-0ubuntu0.24.04.1 and
MySQL8.0.46-0ubuntu0.22.04.4. The actual setup verifies all62 migration receipts.

New `--claim-history` output uses the actual native EAI1/EAP1 encoder/decoder
for five roots (ten framed capsules) in SQL and flatfile modes, with identical
bytes. It models two immutable credits, two partial debits and an alternative
legacy whole debit. Fixture binary SHA256 in both modes is
`1f1cde33697efe61bfa8a0d79492bddf351e3c6cee980e31493c8382205b14b8`;
new claim fixture SHA256 is
`2f0c5dc917aef019c60befbd592c9d0e0a1bf6b0c6bbef828996b9ad077bd635`.
Original32-case output remains
`0196c2e6489287091dbe742e10dc6883b5208b79f58f267fae66139714ea5b54`;
original SQL history fixture remains
`23cef4c8d3544a383cd9fb252fb2d65deb4924fb6c53d421c9a0f7f5ca57289a`.
These fixtures and binaries are retained. Native codec agreement plus modeled
allocation insertion does not qualify a real opening/claim producer journey.

SQL cuts use the existing runner and SELECT-only audit account; UPDATE is
denied. Both consumers run under read-only repeatable-read cuts and roll back.
The owner damages/repairs only private fixture data; FK checks are disabled
only for deliberate corrupt-import cuts and restored before every reader.
All22 captured table contents remain unchanged by auditing and return exactly
to their original state after repair. This establishes data preservation, not
unchanged AUTO_INCREMENT metadata or full-world authority.

## Source transports, builds and protected evidence

Red source archive SHA256:
`8194cda2ef48256a409e821bec28396852f7e60040a385488cc59490120110bd`.
First full passing transport and both production-build archive SHA256:
`15b99a02d80b515808079da72d9e2814a68735c1b772140a08363b4680300715`.
Final pagination transport SHA256:
`56ab72f84f12950949bb9702f4a874120e7d749acf5b280bc6652599729487f8`.

All3,125 tracked code inputs at the refresh merge match the final transport.
The first full passing transport differs in exactly one code file:
`run_restore_accounting_evidence_mysql.py`, which adds the pagination cut and
its reported row counts. Readers, native fixture, original canonical audit/
baseline methods, migration/native sources and build dependencies are
byte-for-byte unchanged. The original full restore method is repeated on the
final transport. Earlier audit/baseline/build results apply to their exact
unchanged inputs; no identical whole-transport or earlier0055/0061 qualification
is asserted for this final0062 source.

Fresh maintained builds use empty object directories and the same pinned image,
original600-second outer deadline and original production targets:

```text
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=mariadb BIN_ROOT=/evidence/build/bin OBJDIR=/evidence/build/objects DMS_BINARY=/evidence/build/server
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile BIN_ROOT=/evidence/build/bin OBJDIR=/evidence/build/objects DMS_BINARY=/evidence/build/server
```

| Backend | Result | Retained server SHA256 |
| --- | --- | --- |
| SQL / MariaDB | Exit0 /509.218026s | `a76ac6c0a07d6fbddaad09f88634f90d822f8fbefd707e4d2ce625b844afc16d` |
| Flatfile | Exit0 /471.493695s | `6219b71aa50272fe4d5cd67804da8d143f5ab02c896cceac8c97bf525c2fdb0d` |

Each freshly compiles738 objects and738 dependency records, with zero reused
objects, warnings or errors. Server binaries, objects and dependency files
are retained. Compilation does not establish service boot or writer journeys.
Changed-line formatting with existing clang-format18.1.8 and the original
`bash scripts/format.sh --check --file tests/async/restore_coin_effects_fixture.cpp`
passes. The first formatter observation exits127 because the pinned tools image
has no clang-format; the existing formatter is then mounted read-only, without
installing or lowering a check. Python syntax and `git diff --check` pass.

Under `D:/CodexEvidence/accounting-plan5/bin/`, protected evidence directories
are `claim-allocation-reader-red-01-20261006`,
`claim-allocation-reader-green-01-20261006`,
`claim-allocation-pagination-green-01-20261006`,
`claim-allocation-maintained-sql-01-20261006`,
`claim-allocation-maintained-flatfile-01-20261006` and the two
`claim-allocation-format-01/02-20261006` observations. They retain exact raw
source/per-file transports, helpers, original commands/logs/results and
applicable native artifacts. Final source-unchanged receipts pass; all retained
Docker test/build observers exited0 without OOM.

Seal `claim-allocation-final-seal-01-20261006/evidence.json`, SHA256
`aeee544b8a77794fcc87e744439020d8877b44f25e5c688d5701d50e1a396bc7`,
binds3,125 code inputs,4,701 artifacts/3,228,210,945 retained bytes, exact
terminal states, commands/log hashes, native outputs, mode/import receipts,
selector observation and preserved tips. Retained evidence size is not a
release storage-growth measurement. Original literal newline trailers remain
unaltered and are explicitly parsed by the seal.

## Curator handoff and remaining gates

This owned report, origin-selector handoff, sealed evidence and remote branch
are the primary's local notebook curator packet. The shared notebook remains
nonblocking and is not independently rewritten. No cross-chat notification,
shared schema/wire change or activation is implied by this publication.

Saved snapshot partial-consumption export/reconciliation, an authenticated
opening-policy selector and frozen original PID proof remain gates. Complete
R6 native/live-world capture and the primary activation verifier, real financial
writer/ACK/lost-reply/cold-restart journeys, typed erasure, complete managed
backup/restore/retention and release-host mixed-root latency/storage/checkpoint/
reconciliation budgets also remain. The missing published selector/digest
reference is a narrow interface blocker for full new-opening authentication;
independent Plan 5 work can continue. No skips or environment blocker prevented
the completed retained-projection slice.

Accounting stays inactive. Wallet-root item exclusions and the declined
inactive spell-path change are preserved. No production data is accessed,
audit findings corrected, deployment performed, PR merged or direct push to
`experimental-accounting` made. Inventory, synthetic fixtures and passing
component tests do not establish release completion.
