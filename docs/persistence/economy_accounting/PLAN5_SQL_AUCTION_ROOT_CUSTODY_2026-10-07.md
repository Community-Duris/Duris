# Plan 5 SQL auction root custody qualification — 2026-10-07

Curator-ready evidence packet. This solves the independent SQL auction root and
claim metadata omission. Plan 5, full R7/R8 and release remain incomplete.
The primary's locally maintained notebook and its acknowledgement are nonblocking.
No notebook application, primary adoption or combined release qualification is claimed.

## Delivery and source

- Branch: local/remote `codex/accounting-plan5`; unchanged throughout this slice.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Branch base: `dab1a0c6b3ed40294f8138c6436c438b39755c97`.
- Code commit: `d01e41fb109c0f576eb5ff5ef61cfa4b927e5af6`. The following documentation-only commit is the publication
  result recorded in `D:/Dev/Tests/Duris/accounting-plan5/sql-auction-20261007/delivery/result.json`.
- Refreshed primary used for tests: `6d2bd242df08fbd74c97f9ae5a2dd1617ad8c1c9`.
- Tested composition tree: `9a0e9e3db7a5fb649d846fe507cfc12d2bd2dfc2`; tar SHA-256
  `5af577d2b18891d7308df9502956153e0bc79bc34c401ebb0325627250358824`. It is the refreshed public primary plus exactly
  the four owned blobs below, not the older native tree on this delivery branch.
- Native tree `833d3085815b396861ad18a77635412212381e4b`; migrations tree `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`.
  Both canonical engines ended at `64 / 0064_auction_custody_history`.
  This is fresh-schema reader qualification, not populated-upgrade qualification.

| Owned source | Tested Git blob |
|---|---|
| `scripts/economic_sql_audit_snapshot.py` | `f7b6cf687447aa868e4d791c49b082266942d405` |
| `scripts/reconcile_economy_accounting.py` | `17bb0a635d58b5b829c6c0d86229249a84ae8d8b` |
| `tests/async/run_economic_sql_audit_snapshot_mysql.py` | `1e885ec1c639c47d6f09990f8574ac9756cacb09` |
| `tests/async/test_sql_auction_custody_audit.py` | `56739f00864bb8058e9741871a1c6fa9ebbddedb` |

Owned documentation: this packet and an additive entry in
`PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`. No shared coordinator, producer, registry,
matrix, migration, maintained shared native recipe or activation file changed.
All seven previously consolidated branch tips are checked as ancestors in the
post-push delivery record. No work was moved to another branch.

## Established defect and complete scoped fix

The predecessor exported current-owner metadata without independently collecting
`auction_item_custody`, `auction_item_pickups` or the item side of `auctions`.
Three authenticated predecessor controls accepted wrong ownership, an unadmitted
retained root, and a completed claim with the wrong beneficiary. The new auditor
reports specific findings for each, preserving every input byte. See
`green04/red-controls.json`; predecessor reconciler SHA-256
`a862ca21352b7e08d5e82e8d68a286f82df317dc11089855d36e92ae7811a7e8`.

The exporter now reads all three sources in its existing repeatable-read,
consistent-snapshot, read-only transaction. It preflights their aggregate row
count before bounded detail queries. It exports lengths and SHA-256 digests of
opaque blobs rather than fetching them into the Python audit process. Both new
source tables must be InnoDB. Empty collections use lists. Current UID metadata
also retains the independently selected `vnum`.

The reconciler validates exact integer/boolean/ID/digest representations before
indexing. It checks listing/slot cardinality, duplicate identities, missing
listings, retained template byte identity, claim beneficiary/operation-field
state, unclaimed UID admission, active state, owner/context, root/parent,
revision, vnum and equipment. A reverse scan detects auction-owned current UIDs
without an unclaimed root. Claimed rows require retained UID admission and are
history: later movement, changed revisions/vnums, tombstones and re-listing do
not inherit an old ownership grant. Legacy listings/pickups remain specific
unknown-identity findings, including retrieved pickups.

Claim-operation presence is checked here; authentic inbox/root/history binding
is still required independently. Matching blob digests establish retained byte
identity only. They do not reconstruct prototype defaults or qualify coin values.
The official exporter still returns `complete=false` and records that gap.

Existing operator views retain their global exception counts and zero/one/100
detail bounds. New details expose only non-personal numeric IDs. No audit path
loads a mutating native object parser or posts a repair.

## Exact execution and results

Evidence root: `D:/Dev/Tests/Duris/accounting-plan5/sql-auction-20261007`.
Build root: `D:/Dev/Builds/Duris/accounting-plan5-sql-auction-20261007`.
Original helpers: `D:/Dev/Temp/accounting-plan5-sql-auction`; sealed copies are
under the evidence root's `helpers/` and per-stage directories.

Actual orchestration: `python -B D:/Dev/Temp/accounting-plan5-sql-auction/freeze.py green04`
then `python -B D:/Dev/Temp/accounting-plan5-sql-auction/run.py green04` from the
worktree. `green04/docker-command.json` records the full command. Docker used
pinned image `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
network disabled, read-only root, two CPUs, 4 GiB RAM, RAM source/test/database
scratch, and a direct D: bind for `bin/`. No production or existing daemon was used.

The exact in-container command was
`python3 -u -B /evidence/green04/observer.py green04`. It used unittest's normal
loader for these complete modules, with `DURIS_RUN_SQL_AUCTION_AUDIT=1`:

| Check | Result |
|---|---|
| `test_sql_auction_custody_audit` | 7 tests, zero skips/failures/errors |
| `test_reconcile_economy_accounting` | 134 loaded, 131 executed, three explicit opt-in skips; no failures/errors |
| `test_economic_sql_audit_origins` | 58 loaded, 55 executed, three explicit opt-in native-origin skips; no failures/errors |
| `python3 -u -B tests/async/run_economic_sql_audit_snapshot_mysql.py`, fresh private MariaDB | complete original fixture PASS, exit 0 |
| Same original fixture, fresh private MySQL | PASS, exit 0 |
| Python AST and `git diff --check` | PASS |

The existing six opt-in skips are the near-limit mapping/price budgets, native
stake SQL, and three native SQL origin/captured-opening tests. They are not
claimed as passing here. Their exact test IDs/reasons are in `green04/terminal.json`.
The new native test compiles the genuine
`src/persistence/economic_sql_source_snapshot.c` with its original headers,
GCC 13.3 C++20, strict warnings as errors, ASan and UBSan; no mutation-provider
stub or production code alteration is used. Full argv are in `green04/commands.json`.
GCC/Python versions and executable hashes are in `toolchain.json`.

Fourteen native comparisons (seven per engine) passed on
MariaDB `10.11.14-MariaDB-0ubuntu0.24.04.1` and MySQL `8.0.46-0ubuntu0.22.04.4`:
nine roots with vnum zero and uint64 UID/revision maxima; wrong current owner;
pending winner claim; wrong claimant; claimed tombstone history; that same UID
re-listed under a new auction after claim; and pending plus retrieved legacy
pickups. All listing/root/claim/template/current-owner/equipment projections
match the genuine native raw capture. Both readers have SELECT-only grants and
an UPDATE denial is checked. Full database inventories remain identical around
every read/audit. Pure cases cover the additional corruption, type/width,
cardinality, legacy, privacy and bounded-output branches.

Raw native and independent JSON cuts, generated native fixture, binary and
observations are in
`D:/Dev/Builds/Duris/accounting-plan5-sql-auction-20261007/green04/bin/tests/sql-auction-custody/`.
Native executable SHA-256: `e22cf002dfbf6b6bfb57bb81c60f5c9bee6ec0d2eaf90f1a94b64ee552f35993`.
Evidence summary SHA-256: `1b5c8cfe3126a454a23dbadf61875de023c8220fb0ff39872e1c690d2c637548`.

The complete original exporter fixture preserves all prior controls and now
checks that auctions, roots and legacy pickups committed after capture begins
remain outside that read view, becoming visible in the next cut. Only the new
vnum field is changed in exact area-coin expectations; all other projections,
money balances, bounds, permission checks and original assertions remain.
The private subset schema in this exporter fixture is modeled; canonical64
native-reader qualification is the separate test above. Neither is a gameplay
or release pass.

The final run completed in `125.952891` seconds, within its unchanged
900-second budget, with `805` recorded commands. Source bodies,
file modes and symbolic-link targets were authenticated before and after.
All four actual private SQL daemons stopped normally; no game server started.
There were no production C/C++ changes requiring another full server build.

Retained failed attempts (not passing qualification):

| Stage | Seconds | Finding |
|---|---|---|
| green | 26.16804994999984 | New exporter returned an empty tuple instead of a JSON-contract list. |
| green02 | 100.62481159800154 | Area-coin exact expectation needed the newly retained vnum. |
| green03 | 95.37474460300109 | Coin nested-row exact expectation also needed the newly retained vnum. |

Each failed run retains its source tar, original log, terminal result and native
evidence. No assertion, native provider, sanitizer, row/byte/time budget or
original test case was removed. Final full results are in `green04/container.log`,
`whole-snapshot-mariadb.log`, `whole-snapshot-mysql.log`, and `terminal.json`.

## Narrow primary interface handoff

No shared interface change is required for this metadata fix. Consume all four
owned source blobs together: new `native.auction_listings`, `auction_roots`,
`auction_legacy_pickups`, count-only `auction_custody_coverage`, and the additive
`native.items[].vnum`. SQL partial snapshots missing the new source coverage
receive `missing_auction_custody_coverage`; other historical backend inputs keep
their existing path. Exact JSON consumers must retain vnum as identity rather
than discard it to preserve earlier whole-snapshot equality.

Full serialized-template/coin-literal reconstruction needs a separate proposed
read-only prototype witness from the shared authority owner. Required facts:

1. A versioned ABI/interpretation descriptor: item wire version, byte order and
   short/int/long widths. The qualified Linux writer uses 2/4/8-byte fields.
2. An immutable prototype-generation digest and vnum binding, sealed after
   special-procedure bindings, tied to the same native source boundary.
3. Full default object facts used when unique bits are absent: four strings,
   serializable extra descriptions, values[8], timers[4], type, wear/extra/anti/
   anti2/extra2 flags, weight, material, cost, bitvectors[5], fixed affects[4],
   spellbook representation, and trap defaults if trap comparison is supported.
4. An explicit clock/normalization witness for dynamic effect duration versus
   timer values. Unknown or unavailable comparisons must remain unknown.

Invariants: prototype defaults cannot be copied from the current custody payload
being checked; generation and special bindings cannot change during capture;
all listing roots share one serialized template; its original UID is overwritten
by each root's durable UID at pickup, so template UID is not compared against
every quantity UID. The consumer is the future independent template/coin reader,
with backup/cold-restore recapture using the same binding.

Required tests: genuine native writer/control round-trips against independently
bound prototypes, omitted-field delta cases, non-default money denominations,
all nine root UIDs, stale prototype/binding generation, exact ABI and malformed
blob boundaries, dynamic clock normalization, both engines/flatfile and cold
restore without any allocator/object-construction mutation. This is a proposal,
not a schema allocation or an implemented/accepted shared contract. The existing
declined inactive spell-path change remains preserved.

## Remaining completion gates

The original Plan5/R1–R8 objective remains active. Required work still includes
complete physical/value/origin census (including serialized auction defaults,
coins and history); native EAB2 encoder/install/recapture; authentic retention,
erasure and cross-restore continuity; populated upgrade/rerun on both engines;
all original major-plan native/producer/player/fault/load journeys with declared
budgets; and the primary's published, tested combined candidate. Prior native
recipe omissions remain in the earlier remote follow-ups for their owner.
Inventory coverage, these synthetic metadata fixtures and isolated passing
readers do not complete release. Accounting stays inactive. No audit auto-corrects,
no production data changes, and no PR merge, deployment or activation occurred.
There is no current execution blocker for continued independent Plan5 work.
