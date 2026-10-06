# Plan 5 current61 audit budget and build dependency qualification — 2026-10-05

Both original near-limit audit budget methods pass on the exact published Plan5
source with zero skips. Twelve fresh CLI measurements preserve exception counts
and input bytes at detail limits 0, 1 and 100. Separately, both maintained server
builds have an unchanged recorded native dependency closure at this source.
These are scoped synthetic budget and dependency checks. Current-reader native
retention and managed restore qualification remain separate pending work.

## Source, branch and ownership

- Worktree: `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
- Branch/publication destination: `Community-Duris/Duris:codex/accounting-plan5`.
  All seven earlier Plan 5 tips remain ancestors; follow-ups stay on this branch.
- Report parent and exact tested public source:
  `f78df1c8a6beee21aca8fc21fa4f3c7ecec931de`.
- Refreshed primary source remains
  `c1dc8960c11447cd5d6d46e7033e0e348ad839bf` and is included in the branch.
- Native build source remains `1ad6c17a688afe0cdfa4083d67f443283f7db229`.
  Current native tree is `bf7a92a728ad9b5b813626462e56533f8ba39c97`;
  migration tree is `2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`, canonical 61.
- This slice owns only this report. It changes no production code, schema,
  coordinator, producer, registry, matrix, activation owner or maintained test.
  It requests no new shared interface change. The existing five-method central
  registration request in the command-preimage handoff remains outstanding.

The source archive is a full tracked snapshot of `src`, `migrations`, `scripts`,
`tests`, `areas_mini`, `areas`, `lib`, `docs`, `README.md`, `AGENTS.md`,
`.clang-format` and `.gitignore` at f78, plus three individually hashed observation
helpers. All 6,178 regular files and both public help links are verified. This
includes the complete original fixture alias targets, uses no live checkout bind,
and copies no local environment, credentials, player data or operational archive.
Archive SHA-256:
`f84afb87df6caeaa7e242776f422234fee565375bb5456273f6a0d2e622c8944`.

## Original budget methods and results

Command from Windows:

```text
wsl -d Ubuntu-22.04 -- python3 -u -B /mnt/d/CodexEvidence/accounting-plan5/bin/f78-operational-qualification-20261005/execution-helper.py
```

The launcher verifies the archive, extracts into a fresh private Linux directory,
verifies hashes/modes/links, then runs the unchanged
`test_reconcile_economy_accounting.AuditBudgetTests` methods:

```text
test_near_limit_price_view_cli_budget_and_limit_invariance
test_near_limit_mapping_snapshot_cli_budget_and_limit_invariance
```

`DURIS_RUN_AUDIT_BUDGET=1` is set before import; the Linux guard is satisfied.
All original cases, fixtures, CLI calls, 30-second/256-MiB assertions and 35-second
process timeouts remain. The observer only records commands and preserves the
original temporary input directory before normal cleanup. Actual environment is
WSL Ubuntu 22.04, Python 3.10.12/GCC 11.4.0, kernel
`6.18.33.2-microsoft-standard-WSL2`, glibc 2.35. No database backend is invoked.

| Original case | Input and observed scope | Result |
| --- | --- | --- |
| Price view | 100,000 price roots; 33,552,384 bytes including original valid trailing whitespace; clean and conflicting inputs at limits 0/1/100. | Six fresh CLI runs; global price/exception counts invariant; clean exit 0, conflict exit 1; input unchanged. |
| Mapping snapshot | 9,948 mapping roots; 33,551,795 bytes of structured original snapshot; clean and corrupt inputs at limits 0/1/100. | Six fresh CLI runs; exact `invalid_mapping_creation_root` count invariant; clean exit 0, corrupt exit 1; input unchanged. |
| Over-limit refusal within the mapping method | Valid JSON with whitespace reaching 33,554,433 bytes; limits 0/100. | Original CLI exit 2, no stdout, original refusal diagnostic and unchanged input. |

Both methods pass: 2 tests, 0 failures, 0 errors, 0 skips; wrapper 14.620214 seconds.
Maximum of the 12 measured CLI calls is 0.937349 seconds and 169,353,216 bytes
(161.51 MiB). Every call remains below its original 30-second/256-MiB limits.
The final temporary inputs and their hashes are retained; the clean/corrupt
measurement identities remain in the original log. This synthetic component
does not qualify a complete mixed native snapshot or the release host. The
price fixture deliberately reaches its row bound before padding to its byte
bound; it is not relabelled as 32 MiB of structured price records.

The raw collector retained 10 JSON records because unittest progress shares the
line with each method's first measurement. The seal extracts all 12 original
JSON objects after their exact marker, verifies the complete case/limit matrix,
and retains the raw collector result unchanged. This is an observation-parser
correction; no test or CLI result changes. Log SHA-256:
`6d4bb0acb8ac0ebf65caad12063fdaae0bf35f7726b0557c6c4ed010b0a9d3fe`.

## Native build dependency proof

Command:

```text
python -u -B tmp/plan5/verify-f78-native-dependencies.py
```

The verifier binds the original source maps, build results, complete build logs,
server hashes, 726 objects and 726 compiler dependency records per backend.
Every original build still has 726 unique compiled units, zero object reuse,
warnings and errors, and an unchanged recorded source. The only differences
among 3,079 recorded broad input fingerprints are the five Python files from the
independent command-preimage slice. None is in either recorded native dependency
closure. Every local dependency and `src/Makefile` matches the frozen f78 snapshot;
all external headers remain associated with the original qualified image.

| Backend | Unchanged local dependency files | Original qualified server SHA-256 |
| --- | --- | --- |
| SQL/MariaDB build | 1,259 | `21755a0be2bbd9f51fb1133167532a6ecf23bd963bda07d29e759199112ca49f` |
| Flatfile build | 1,261 | `2087640a44081883ffffdb5e5a4929c7c8fce5c12f353518c6ecfcd3f82f3daa` |

Original build image remains
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
This justifies reusing those exact original binaries in a subsequent current-reader
journey; it does not claim a new build or current-reader native execution. The
first verifier attempt refused before preparation had finished because its
source-transport file did not yet exist. Its completed successor verifies the
original files and dependencies without changing them.
Dependency receipt SHA-256:
`d2f175f9efe70630dac4c837114c6722b88e412a05a4aa61182ddba9cde0045d`.

## Evidence, gates and curator packet

Protected evidence root:
`D:\CodexEvidence\accounting-plan5\bin\f78-operational-qualification-20261005`.
It contains the immutable source/map, exact observation helpers, budget log,
commands, preserved final inputs, native dependency receipt and engine blocker
readback. The scoped seal is `tmp/plan5/f78-operational-budget-evidence.json`,
also copied beside the archive: 23 verified artifacts at sealing time, all 12
measurements, 0 skipped selected checks. Seal SHA-256:
`3e08de4eddc4dce4f966bc9c358999da9500cca8496a3249120fb3e15d81a644`.
The post-publication delivery receipt binds this report's result and remote SHA.

At this seal Docker is stopped and reports `Docker Desktop is unable to start`.
The CLI returns 0 with that error and no server version, so exit status alone is
not treated as a successful engine readback. The Docker startup log identifies
a full C: disk. Completed, ignored Plan5 build-object relocation is a separate
preservation operation and does not qualify a database or server journey.
It completes after this seal: seven directories, 10,969 regular files and
6,425,717,013 bytes verified through both original junction paths and D: targets.
No evidence is discarded. `relocation-preflight.json` and
`relocation-verified.json` retain the exact paths and hashes. C: free capacity is
6,716,133,376 bytes at verification. The stopped engine remains unavailable;
`docker desktop start` reports the app is already running. No global restart is
performed. The frozen native observers are prepared but have not run.

Native SQL/flatfile retention with the new readers and managed SQL/flatfile
restore remain pending. Earlier native results retain their exact older source.
Primary central registration and a tested combined candidate remain required,
as do complete writer, full-world/player, real workload, complete 32-MiB snapshot,
release-host and R1–R8 acceptance. Accounting stays inactive; wallet-root item
exclusions and the declined inactive spell-path change are preserved. Nothing is
activated, deployed, merged, auto-corrected or written to production.

This report is the notebook curator packet for the primary's locally maintained
shared notebook. Record the scoped budget/dependency passes and retain every
pending gate above; do not promote inventory or synthetic tests to release.
