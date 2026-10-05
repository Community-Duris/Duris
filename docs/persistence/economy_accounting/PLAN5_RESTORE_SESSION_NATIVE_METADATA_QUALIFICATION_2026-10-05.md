# Plan 5: published empty-restore fixture and native metadata qualification

The primary's published empty-restore fixture repair now passes the original
24-method migration runner suite on the combined Plan5 branch. Fresh MariaDB
and MySQL databases independently confirm the production reader's table-count
contract: three readable baseline evidence tables qualify; an unreadable
reservation table refuses with `restore_economic_baseline_source_mismatch`.
Qualification and refusal preserve authority and close the original session.
This is empty-history component evidence. Full accounting release remains open.

## Source and ownership

- Branch: `codex/accounting-plan5`, retaining all prior Plan5 histories and fixes.
- Worktree: `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
- Previous Plan5 head: `4f2810111f704f00f3ae5de2fdf24ae21b6d6cc7`.
- Refreshed primary: `3e93d4cb8c805bb042fefc0f88c5b1cbe84f5539`.
- Merge and tested source: `4c9d29fb5109a0624aaa94536326171c03d5bedb`.
  Its two parents are the previous Plan5 head and the refreshed primary.
- Native tree remains `b00968beadaa72d2e126c11d41a92231017e6d27`;
  migration tree remains `1b0f9a40fef29de409338ba83be015cd3390c9f5`.
- The merge imports only the primary's fixture repair and
  `RESTORE_SESSION_BASELINE_TABLE_FIXTURE_2026-10-05.md`. The published fixture
  Git blob is exactly `3babd4522869f5b20641616265599bfb4aad1000`, raw SHA256
  `de045a6c1fc2d411ef6db5130697e18f57ed7405941587f2b975a302adb8abf0`.
  The primary report's private schema61 fixture checksum is a different input;
  this execution qualifies the actual published canonical0056 fixture.
- This slice owns this report. No independent shared-file edit, new interface,
  accounting contract, schema, writer registration or activation change.

The old fixture answered `0` for all SQL, including the metadata query. The
existing reader requires exactly three InnoDB tables named
`economic_baseline_control`, `economic_baseline_witness` and
`economic_baseline_reservation`. The exact primary repair answers `3` only for
that query and `0` for empty-history checks, observes the metadata call, and
requires SELECT-only calls. Original cases and assertions are retained, including
no SQL on incomplete history and session closure on both original branches.

## Original suite and release gates

Pinned Linux image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
All containers use `--network none`, read-only root and source mounts, private
executable tmpfs directories, and no published ports or project environment
credentials. Qualification artifacts are retained separately on D:.

| Exact executed command | Source and result |
| --- | --- |
| `python3 -u -B tests/async/test_immutable_migration_runner.py -v` | Before merge at4f281: all24 methods, zero skips, exit1 with one complete-history error `restore_economic_baseline_source_mismatch`; 78.777542 process seconds. |
| Same original command | After merge at4c9d29: all24 original methods PASS, zero skips, exit0; 71.005694 process seconds. |
| `python3 -u -B scripts/validate_economy_accounting.py` | PASS, exit0; 10.996121 seconds. |
| `python3 -u -B scripts/generate_economy_writer_coverage.py --check` | PASS, exit0; 12.696820 seconds. |
| `python3 -u -B scripts/validate_economy_accounting.py --release` | FAIL, exit1: `writer has no executable evidence`; 0.091760 seconds. |

Retained dispatcher commands are
`python3 -u -B tmp/plan5/run-restore-session-qualification.py before 4f2810111f704f00f3ae5de2fdf24ae21b6d6cc7`
and
`python3 -u -B tmp/plan5/run-restore-session-qualification.py after 4c9d29fb5109a0624aaa94536326171c03d5bedb`.
The dispatcher invokes the original test file directly; it changes no test body,
manifest, head expectation or transport behavior. Source maps before/after each
execution are exact; the only mapped difference between the two source cuts is
the primary's published test fixture. Final native database inputs match the
post-merge suite map exactly.

## Real disposable database checks

Command:
`python3 -u -B tmp/plan5/qualify-empty-restore-databases.py databases-green`.
The retained helper uses the existing `persistence_restore.private_database`,
sealed bootstrap, `MysqlExecutor.adopt('fresh_bootstrap')`, `run_pending`, and
original `qualify_database_restore.main()` implementations. It observes and
forwards the original reader SQL; no substitute reader or mutation logic is used.

| Engine | Fresh schema and complete history | Complete reader | Unreadable required evidence |
| --- | --- | --- | --- |
| MariaDB10.11.14-MariaDB-0ubuntu0.24.04.1 | 56 newly applied steps;226 tables; head0056_spell_ward_durability | Count3; original qualifier PASS;53 SELECT calls | Count2; original qualifier refuses `restore_economic_baseline_source_mismatch`;44 SELECT calls |
| MySQL8.0.46-0ubuntu0.22.04.4 | 56 newly applied steps;226 tables; head0056_spell_ward_durability | Count3; original qualifier PASS;53 SELECT calls | Count2; same exact refusal;44 SELECT calls |

Both independent principals have SELECT-only privileges. The second principal
cannot read `economic_baseline_reservation`; all physical tables remain present.
Each principal's UPDATE probe refuses with native error1142. All226 tables' rows
are captured before and after each qualification/refusal/probe and remain exact.
The original transport closes its session on every path. No economic operations,
baseline books, epochs or active lineage states are installed in these empty cuts.
These four database observations are distinct from the24 unittest methods.

MariaDB authority snapshot SHA256:
`fb345154e7e070f66d40152490aa71c550573fefd8bcb9637931ab6747bc0ebf`.
MySQL authority snapshot SHA256:
`ca439364e926302c9aa52a26e6459c2a025671d74c5766985c7daf7f4eb4fe88`.
Per-engine before/after files have identical hashes. Engine times are14.674984
and19.754880 seconds, excluding source-map/sealing overhead.

The initial helper attempt completed its migration call, then incorrectly called
`applied()` on that closed migration session. The native guard refused with
`migration SQL session is closed`; zero qualification observations resulted.
Its original helper, source map, bootstrap/database logs, failure record and
dispatch traceback remain retained in `databases/` and `database-dispatch.log`.
The corrected helper opens a separate verification session and runs entirely
fresh instances into `databases-green/`. No production reader or test assertion
was changed to accommodate this helper error.

## Evidence and curator handoff

Physical evidence root:
`D:\CodexEvidence\accounting-plan5\bin\bb259-maintained-20261005\tests\p5-3e93-restore-session-20261005`.
It contains original/final logs, executed helper and production-source copies,
full source maps, SQL transcripts, authority snapshots, results and hashes.
All56 retained artifact hashes are independently verified by
`python -u -B tmp/plan5/seal-restore-session-qualification.py`.
The aggregate is `tmp/plan5/restore-session-qualification-evidence.json`, SHA256
`6838c029b671081648d0847c13a917d3c4140345b20704960c9139fe1e550dce`.
The result commit and verified remote publication are recorded separately in
`tmp/plan5/restore-session-qualification-delivery.json` after publication.

For the primary's notebook curator: record this original24-case published56
qualification and four empty-history SQL observations, the helper failure and
correction, and the still-failing release command. Notebook locality is not a
blocker; this report supplies the concrete curator packet.

## Remaining gates and scope limits

No new C++ build or flatfile journey is run for this test-only primary repair.
Prior native build/audit/retention evidence remains at its recorded source cuts.
This slice establishes neither a populated/player restore nor complete capture,
producer coverage, active erasure, runtime budgets or release acceptance.

Primary still owns the integrated nativeEAB2/schema61 candidate, actual producer
and writer evidence, and the shared flat inspector recipe. The existing narrow
request in `PLAN5_CURRENT_RETENTION_AND_INSPECTOR_HANDOFF_2026-10-05.md` remains:
append the two real baseline adapter/codec providers to the original75-source
recipe and rerun the original inspector and three retention journeys. Its
diagnostic77-source pass does not close that shared recipe gate.

After the integrated candidate is published, Plan5 must update only the
designated current-history consumers, retain historical0056 cases, and qualify
the actual combined native audit, restore, retention, replay and real-player
matrix. Controller retention and shared disclosure decisions remain required.
Release, R1–R8 and activation remain incomplete. Inactive behavior, wallet-root
item exclusions and the declined inactive spell-path change are preserved.
