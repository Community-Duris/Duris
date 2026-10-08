# Plan 5 original legacy migration wrapper qualification — 2026-10-08

The complete unchanged original `tests/async/run_legacy_migration_mysql.sh` passes
on real MariaDB 10.11.14 and MySQL 8.0.46 at published canonical 64. Both legacy
and fresh profiles reach 64, with the original replay/refusal, imported-history,
persistence metadata, bootstrap equivalence and runtime compatibility controls
passing. The external observers then fail on a case-sensitive missing-container
error assertion after successful original cleanup. Both observer failures remain
recorded as exit 1. Separate read-only checks prove exact owned-container removal,
with a presence control that refuses to call a running owned runner absent.
This qualifies the original wrapper's executed controls, not a passing original
observer pipeline or full release. Its legacy input is the checked-in baseline
fixture, not a captured production dump. Redis is disabled, eligible character
counts are zero, and no full server/player journey occurs. Full Plan 5/R1–R8,
combined-candidate and release qualification remain open.

## Branch, owned files and exact source

Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Sole local/remote publication branch: `codex/accounting-plan5`.
Base: `5a95cf0579b63f2855e2cc4148400b894550c1e4`. The containing report commit is the result, bound to
its exact remote SHA by post-push `delivery/result.json`. All seven earlier branch
tips, their work and follow-ups remain ancestors here. Existing worktrees/main
checkout are untouched.

Only this report and the additive `PLAN5_REMOTE_FOLLOWUP_2026-10-06.md` change.
No original wrapper/helper/assertion, migration/schema, native provider, audit,
coordinator, producer, registry/matrix or activation file changes. No new shared
API/schema/interface fields or application request is needed. The prior full
baseline recipe/head 64, flat auction and room UID/provider handoffs remain open.

| Executed source | Exact identity |
| --- | --- |
| Published primary | `aa1613f5b3378cd253a046813e3c0de525206de7` |
| Complete published tree; zero owned overlays | `a7fe97479e67dd020c76cfb1a560548c030394ca` |
| Native tree | `833d3085815b396861ad18a77635412212381e4b` |
| Migration tree | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Source archive SHA256 | `48ad6cc94cef52da2836048cabe6f1e38f24d98e29f133ffc68b66c70208e6f2` |
| Canonical head | `64 / 0064_auction_custody_history` |

Archive authentication binds 6497 regular bodies and four link targets to 6501
Git blobs and exact archive modes. Each terminal guard rehashes every body,
checks native modes and checks all source links. The 19 owned code blobs retained
on this branch are separately preserved, not overlaid into this execution.
Required plan/requirements/latest checkpoint bytes are retained; AI_CONTEXT.md
is absent. Primary-local notebook maintenance is nonblocking. Primary's private
128-production-file candidate `bd4d4a56c71cbabe2ecc8b56880e2046a856905f458cfdfb164ea4409a255ffe`
is separate and unexecuted here. Current native/schema fingerprints do not
qualify that private implementation.

## Original command and environment

Exact host launches, from the stated worktree:

```text
python -X utf8 D:/Dev/Temp/accounting-plan5-legacy-wrapper/launch.py mariadb01
python -X utf8 D:/Dev/Temp/accounting-plan5-legacy-wrapper/launch.py mysql01
```

Each invokes the complete original wrapper with tracing enabled:

```text
bash -x /workspace/tests/async/run_legacy_migration_mysql.sh
```

No assertion, control, fixture, migration or runtime-contract body is modified;
no stale expected head is replaced, no test is skipped to achieve a pass, and no
alternate migration implementation is used. `bash -x` records expansion/commands;
stdout, xtrace and generated replay/refusal logs are retained. Original commands
include `migrations/run_migration.sh`, the actual immutable `migration_runner.py`,
`verify_item_ownership_schema.sh`, `tests/test_migration_replay_safety.sh`,
`tests/test_run_migration_persistence_schema.sh`,
`tests/compare_bootstrap_mud_schema.sh` and the complete shell runtime verifier.
There is no contract update or schema-only shortcut.

The original `_sql_fixture_network.sh`/`disposable_sql_fixture.private_network`
validates that DURIS_TEST_CONTAINER names the actual runner. Each database then
shares only that runner's network-disabled namespace, listens at its reserved
loopback port and publishes no host port. The runners have 2 CPUs/4 GiB, native RAM
source/scratch and direct D: evidence/artifact mounts. The Docker control socket
is explicitly mounted for this original Docker wrapper; no production database,
service, credentials, data directory or volume is selected. Only newly created,
randomly named legacy test containers are removed by original cleanup. Existing
containers and volumes remain; the original `rm -f` does not remove fixture volumes.

Tools image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
The pinned image lacks Docker CLI, and WSL's installed Docker link is unavailable.
No integration setting changes or tool downloads occur. The existing Desktop
`C:/Program Files/Docker/Docker/resources/wsl/docker-wsl-cli.iso` is authenticated
against its supplied SHA256 `d10f704d03b30773e3d2f53a17a39d0ebfee35b867b052578357d5880b1018bb`. Only its Linux client is
extracted to task storage; executed copy matches SHA256 `e45381109c685311cf84c5e33a1aca7da81d6b55c0f9aed74091fc08c3a94f13`.
The actual client version/body and Python/SQL tools are retained in the seal
inventory. The recorder forwards the original Docker argv to that real client,
validates new fixture name/image/network ownership and preserves metadata,
server logs, two-schema logical dump, history/state/table/readiness and retained
extension/reboot rows before forwarding the original cleanup. Its additional
capture commands are read-only and are not original test assertions.

Both provider images already exist locally and are selected by immutable digest:

- `mysql:8.0.46`: `sha256:7dcddc01f13bab2f15cde676d44d01f61fc9f99fe7785e86196dfc07d358ae2b`.
- `mariadb:10.11.14`: `sha256:dbe56e20372fc6d6b8e0e396866ba89c4c7f128c38c4f59aaa54d957db95790c`.

## Executed controls and distinct verdicts

Each full original driver records 150 steps and zero failures; its Redis-clear
step explicitly says not enabled because the original config sets REDIS=FALSE.
The wrapper requires the legacy driver itself to reach the current manifest head
before any separate replay can conceal an incomplete upgrade. Subsequent controls:

- Drop equipment_slot in the disposable adopted schema; require the item verifier
  and migration replay to refuse. Require the normalized metadata mismatch,
  retained head and missing column; restore the exact known fixture definition.
- Delete baseline evidence, then baseline plus history, in separate damaged
  fixture cuts. Require missing/stale adoption refusal without legacy DDL,
  unchanged item definition and migration state, then restore original fixture rows.
- Preserve imported extension data and both original reboot-history cases,
  including the normalized unfinished runtime row and original archived NULLs.
- Adopt/run the fresh bootstrap, preserve duplicate description metadata in its
  archive, then run original locker replay and persistence-column probes.
- Compare the entire bootstrap-defined MUD structural scope, including columns,
  indexes, engines/collations and foreign keys. The imported extension is checked
  separately, and web-only tables remain outside the original bootstrap allow-list.
- Run the full shell runtime/schema/baseline-readiness verifier. Both profiles
  have zero eligible characters, so this does not prove populated baseline readiness.

| Engine | Original wrapper | Observer terminal | Recorded elapsed | Cleanup follow-up |
| --- | --- | --- | ---: | --- |
| `10.11.14-MariaDB-ubu2204` | PASS / exit 0 | FAIL / exit 1 | 204.135912s | Read-only PASS |
| `8.0.46` | PASS / exit 0 | FAIL / exit 1 | 454.886997s | Read-only PASS |

Both failures occur after original wrapper exit 0, complete diagnostic capture and
original `docker rm -f` exit 0. The observer assumed uppercase `No such object`,
while Docker 29 emits lowercase `error: no such object`. Original fixture source
and before-cleanup data are unchanged; raw terminal/exception bodies stay exact.
The corrected external observer checks a successful exact-ID `docker container ls
--all --no-trunc --filter id=<owned-id>` with empty output, plus failed inspect;
it no longer relies on error-message case. The separate post-terminal checker
also requires inspect's structured empty array, the original successful removal,
exact owned image/network identity, and all diagnostic captures succeeding.
The corrected complete observer is not rerun. These separate checks verify the
observation correction without repeating already passing
migration tests. A known-present, then-running MySQL runner is correctly refused
as absent. Neither original observer is retrospectively relabeled exit 0.

Exact follow-up commands:

```text
python -X utf8 D:/Dev/Temp/accounting-plan5-legacy-wrapper/verify_cleanup.py mariadb01
python -X utf8 D:/Dev/Temp/accounting-plan5-legacy-wrapper/verify_cleanup.py mysql01
python -X utf8 D:/Dev/Temp/accounting-plan5-legacy-wrapper/cleanup-observation-controls.py
python -X utf8 D:/Dev/Temp/accounting-plan5-legacy-wrapper/summarize.py
python -X utf8 D:/Dev/Temp/accounting-plan5-legacy-wrapper/seal.py
```

The original wrappers' final success marker is present, each real Docker call
returns 0, all additional capture commands return 0, source guards pass, and the
separate cleanup check passes. Native database versions are measured from the
actual containers. Complete before-cleanup 64-row receipt/state cuts and logical
dumps are retained. Generated replay/refusal logs stay under each task D: bin.
No repository C/C++ changes, Make build, compiled server boot or real player/
financial producer journey occurs in this slice. No current full baseline fixture,
flat/room recovery or captured historical production upgrade is claimed.

`mariadb01`: original Docker calls 3; logical dump
662001 bytes, SHA256 `add832db665db21ba1b056f8e8fcafef240c1d051bed4c7d4eecc2522e79b4df`.

- `duris_fresh_bootstrap_test`: head 64 / 64 receipts / 230 tables / zero eligible characters; history SHA256 `e259ffc3968ce6d5640af66af60eae114435e6f3a92c6032fb06a61537eaf084`, state `64 / 8DBE4E1771A71D2EFF1D496990D7AFCF0A55E8FAB932C59838ACE4C8EB806FE2`.
- `duris_legacy_migration_test`: head 64 / 64 receipts / 233 tables / zero eligible characters; history SHA256 `49d0d0d7819e96d187a5b3726051272d298b080365465eee747bcf0e0cbb4621`, state `64 / 8DBE4E1771A71D2EFF1D496990D7AFCF0A55E8FAB932C59838ACE4C8EB806FE2`.

`mysql01`: original Docker calls 3; logical dump
674780 bytes, SHA256 `1ae4ec629ff8c92dec74df2bd1c832d6fe0cba9abc690565c0c06881c24437e2`.

- `duris_fresh_bootstrap_test`: head 64 / 64 receipts / 230 tables / zero eligible characters; history SHA256 `77558681bf5b0b7bfd3ebc4fc016f29ad202f8fdee60fece5b52ecc5127e7ec4`, state `64 / 8DBE4E1771A71D2EFF1D496990D7AFCF0A55E8FAB932C59838ACE4C8EB806FE2`.
- `duris_legacy_migration_test`: head 64 / 64 receipts / 233 tables / zero eligible characters; history SHA256 `676f3d77f283b076c7fff51a2bfb77657d8ce534ca671c8edcc452e61e8f7e5c`, state `64 / 8DBE4E1771A71D2EFF1D496990D7AFCF0A55E8FAB932C59838ACE4C8EB806FE2`.

## Evidence, curator handoff and remaining gates

Raw evidence: `D:/Dev/Tests/Duris/accounting-plan5/legacy-wrapper-20261008`.
Artifacts: `D:/Dev/Builds/Duris/accounting-plan5-legacy-wrapper-20261008`.
`qualification.json` explicitly separates original-wrapper PASS, original-observer
FAIL and read-only cleanup PASS. It does not turn raw failure statuses green.
All exact original stdout/xtrace, Docker argv, image/client identities, source
body/mode/link pins, before-cleanup schema dumps/history/state/native rows and
server logs, both raw exceptions and correction controls are retained.

`seal/evidence.json` SHA256 `eeb933813093a24833173f91fb54072ee2333a21bb51be2375829182c0703526` covers 133 files /
307986250 bytes, with 11 native artifact files,
zero copied links/reparse points. Native lstat/hash inventory precedes Windows
regular-body verification. Both runner containers and the seal container are
stopped/network-disabled; original fixture containers are absent and their
pre-removal metadata/logs/dumps retained. Post-push `delivery/result.json` binds
local/remote result, clean worktree, all seven earlier ancestors and 19 preserved
owned code blobs, and rehashes every raw sealed body with the final primary refresh.

The remote follow-up is additive and curator-ready. Primary-local shared notebook
maintenance is nonblocking; adoption/application/acknowledgement is unclaimed.
No other chat is messaged. The legacy wrapper requirement now has current64
published-source execution evidence, with its observed cleanup caveat explicit.
Full original baseline/equipment/claim/cold-clone and flat/room fixture gates,
primary private combined native/SQL/gameplay/persistence/recovery, genuine native
holdings/UID/source coverage, real writer/player/load journeys and complete
backup/restore/retention/upgrade/R1–R8 release qualification remain open.
Accounting stays inactive; wallet-root ITEM_MONEY exclusions and the declined
inactive spell-path change stay. No activation, merge, deployment, production
mutation/access, audit auto-correction or push to experimental-accounting occurs.
Goal active.
