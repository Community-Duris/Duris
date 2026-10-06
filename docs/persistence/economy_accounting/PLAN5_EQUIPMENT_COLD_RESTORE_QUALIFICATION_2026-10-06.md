# Plan5 equipment audit through native-book cold restore — 2026-10-06

The maintained original native baseline recipe did not exercise the repaired
live equipment reader through cold restore: its current-owner table was empty.
This qualification gap is closed by adding clean and slot-drift captures before
and after real dump/import into fresh private daemons on both canonical SQL
engines. The original native books, commands, refusal cuts and replay checks are
retained. No production implementation, native contract or schema changes here.

## Branch, source and ownership

- Branch/worktree: `codex/accounting-plan5`,
  `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
- Base: `f0b6bb3e4472e09aa7b52cbc73e75336d00bcda3`, normal merge of refreshed
  primary `8fbf055847ba06f5d0294ee7ec5957b210403e4e` over published Plan5
  `a781ab425d568f82c31fbea59b909999df1448b9`. The transport receipt records the
  authoritative primary SHA; the delivery receipt identifies the separate slice
  commit, subsequent normal primary merge and published result.
- Maintained native tree: `bf7a92a728ad9b5b813626462e56533f8ba39c97`;
  migrations: `2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`, canonical61.
  Earlier0055 evidence does not qualify this candidate.
- Owned code: `tests/async/_plan5_equipment_restore.py`,
  `tests/async/run_native_sql_baseline_audit.py`, and
  `tests/async/test_native_sql_baseline_audit.py`.
- Owned documentation: this report and `AUDIT_OPERATIONS.md`.
  Shared coordinator, producers, contracts, migrations, registry/matrix and
  activation files are untouched independently.

The existing `native_sql_baseline_claim_audit` integration row already invokes
the original single unittest method, requires its final qualification marker,
uses its self-SQL provider and retains its900-second central deadline. The new
helper runs before that marker. There is no additional shared registration
request for this slice. The earlier seven-method equipment-unit registration
was primary-owned; primary `c00f1868174ea3481ec39b4f533f0063a5b2a658` now imports
the previous exact fix and registers that test. Its post-merge verification is
recorded separately rather than rewriting the pre-merge native evidence.

Frozen source archive SHA256:
`dcccc217ebdd82534574881e78d0a3b840db756c868987569ecc77651238eabe`.
All3537 regular source/helper files are verified before and after execution;
there are no links or live source mounts. This component archive includes
src/migrations/scripts/tests and documentation, not complete world inputs.

| Exact qualified input | SHA256 |
| --- | --- |
| New equipment restore helper | `73dfe20b7a7d0de9b55ea1c1a88c5e532f2e059997ca3c70cfd57ae3895d7461` |
| Original runner with appended coverage | `e4951211c54bbb1833a6cf678985086d05efe76a597296d1ad592888189420c5` |
| Original unittest owner with helper binding | `9c2247bee8008579d618053e5231e0e4bb132b72a7b770cd36d6f187d2e47c0f` |
| SQL snapshot reader | `decfb164500bcd92efa673b64efa9e554ec49c17282fb631c8501dde813bba00` |
| Independent reconciler | `7304189a6d0639e737b7e839c7b4f5d6abce81f33c58293d93cd201515a6ca5c` |
| Original native fixture | `b0e6b3915c6251f96cea7635995ad3a2703a9e3f0889107af34471a7f6240df2` |

## Actual recipe, builds and results

Pinned Linux image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`;
Python3.12.3, GCC13.3, MariaDB10.11.14 and MySQL8.0.46.
Container `plan5-equipment-cold-restore-03-20261006` has no network or published
ports,2 CPUs and4GiB memory. Workspace and the original parent database use
private RAM mounts. Original child clone directories require an ephemeral
writable container root; they are not claimed as RAM-only. Only the protected
evidence output is mounted from the host. No local environment or live database
is consumed.

The external preparation/execution wrappers freeze inputs and forward the
original recipe without changing compiler flags, cases, guards or deadlines:

```text
python -B tmp/plan5/prepare-equipment-cold-restore.py
python3 -u -B tmp/plan5/qualify-equipment-cold-restore.py
DURIS_RUN_NATIVE_BASELINE_AUDIT=1 DURIS_PLAN5_BASELINE_AUDIT_ARTIFACTS=/workspace/bin/tests/plan5-baseline-sql-restore/equipment-cold-current61 DURIS_REGRESSION_BUILD_CACHE=off python3 -u -B tests/async/test_native_sql_baseline_audit.py
```

The wrapper preserves the original tests/async import path and adapts only its
private parent directory layout. The original child restore runs unchanged.
`results/process.json` retains every literal compiler/tool/recipe command and
its original timeout. Fresh SQL and client-free native fixture builds retain
C++20, `-Wall -Wextra -Wpedantic -Werror -O1 -g`, the original test define,
ASan/UBSan, frame pointers and non-PIE flags, all15 original fixture/production
units, SQL mysql_config flags/libraries, and client-free `-D__NO_MYSQL__`.
Both report zero reused objects; SQL compiles in39.184s and client-free in35.689s.
Their byte-exact binaries are respectively
`72d6d5027b5ed550921efd6b11d14b120dacc1b42d53b6ba32406f96b3f54f2c` and
`dd503bd0c1ba66557ff96a9a2004d5007d60525c5c7259bdb5224ced1072ce0d`.
The native receipt binds1275 native inputs,246 migration inputs and20 consumers.
No C/C++ changes or new full-server build are claimed.

The complete original unittest method passes both engines in318.902s, zero
skips; wrapper wall time320.4090775460936s. Each engine retains two native
baseline books/epochs,161 corruption/refusal cuts, seven constraint refusals,
ten key-hash cuts, both command-binding cuts, the original cold clone and the
original modeled SQL audit runner. Earlier isolated passing subsets are not
substituted for this completed recipe.

Each engine adds four full SELECT-only captures: source-clean, cold-clean,
source-drift and cold-drift. UID81/player7/revision3/slot5 and
UID82/player8/revision4/slot6 are explicitly modeled live positions. The EAB2
opening slots5/6 and native commands are authentic original native output.
Only the private fixture owner changes UID81's current slot from5 to6; the
audit role cannot UPDATE, demonstrated by native permission error1142.

All eight captures retain64 SQL statements, exactly one rollback and one cursor
close, and identical before/after data hashes/counts for all228 application
tables. Clean reports retain `evidence_loss=1, missing_native_holding=2` because
the original fixture is partial. Drift reports add exactly `stale_native_item=1`.
No all-clear report or complete capture is claimed. All24 real operator CLI
invocations at limits0,1,100 exit1 as expected; totals and source bytes are
invariant, and output detail stays bounded.

Four new cold phases use actual `mysqldump --single-transaction --skip-lock-tables
--hex-blob --routines --triggers --events` and mysql import into separate newly
initialized private daemons. Every imported228-table data inventory matches
its source. Each clone runs the full historical
`scripts/qualify_database_restore.py` and original native `--reconcile`:
four new exact immutable-native replays and retained-evidence qualifiers pass.
These checks prove retained-book/history scope; they do not attest live equipment.
The independent reader still detects the restored drift. No audit finding is
corrected. Private fixture cleanup returns all source application data to its
initial hash and leaves the original books/commands unchanged.

## Evidence, retained failures and gates

Protected artifacts:
`D:\CodexEvidence\accounting-plan5\bin\equipment-cold-restore-{01,02,03}-20261006`.
Successful03 contains source transport, logs, compiler receipts/binaries,
original native dumps, four additional equipment dumps, eight frozen snapshots,
bounded CLI outputs, authority inventories and container states. The seal at
`tmp/plan5/equipment-cold-restore-evidence.json` authenticates149 artifacts:
SHA256 `80491d900fd6752872b8532e354dedb1522dbf9124936a90307799e2797eea5d`.
The delivery receipt is generated only after committing, merging and verifying
the remote Plan5 tip, all exact qualified code bytes and earlier branch ancestry.

Attempt01 failed before compilation/database work because the external runpy
wrapper omitted the original import path. Attempt02 completed fresh builds and
the core cuts, then both original child clone subtests failed because their
root temporary directories required writable ephemeral storage. Neither attempt
qualifies the whole recipe. All failed logs, source transports and exited states
are preserved; attempt03 reran the complete recipe after correcting only wrapper
layout. No source changes or raised limits conceal those failures.

Windows equipment contracts pass seven methods, zero skips. Normal accounting
validation, generated matrix check and current61 runtime metadata pass. Release
validation still exits1 for missing executable writer evidence. Metadata passing
does not establish release completion.

This is an actual native-book cold database restore with modeled live positions,
not a new managed backup generation, service boot, retention drill or flatfile
equipment restore. Complete holdings/UID/world capture, real producer and player
journeys, all backend lifecycle/corruption/retention journeys on the final
combined candidate, erasure qualification, resumable audit sweeps with a reviewed
commit watermark, and original release-host mixed workload/budget measurements
remain open. Earlier managed SQL/flatfile and retention evidence retains its
recorded source and scope; no transfer to private birth candidates is claimed.
Accounting stays inactive; wallet-root exclusions and the declined inactive
spell-path change are preserved. No activation, deployment, production mutation
or shared-file independent edit occurs. There is no blocker to independent work.

This owned report and the delivery/integration receipts form the curator handoff.
The primary agent's locally maintained shared notebook remains authoritative and
nonblocking, per the user. Shared notebook and review checkpoint edits stay with
that owner. All follow-ups and all seven prior branch tips remain on the remote
`codex/accounting-plan5`; no experimental-accounting push is made.
