# Plan 5 SQL original-command reader qualification — 2026-10-05

The independent origin and restore/canonical readers now reject a known
admission-time/full-command-hash disagreement. Genuine native schema61 dumps
establish the defect on both MariaDB and MySQL, and the repaired readers refuse
the same cuts without changing authority. Historical NULL admission times remain
unknown and replay compatible. The original native origin and baseline owners,
291-method Linux batch and genuine56→61 upgrade pass with zero skips in the
selected test owners. Release still refuses missing executable writer evidence.

## Branch, parent and owned files

Worktree: `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Branch and sole publication destination: `codex/accounting-plan5` on
`Community-Duris/Duris`. Parent:
`3015f010eaf9d2e446aee57d7414cb6894b79e3f`.
The separate post-publication delivery receipt records the result and verified
remote SHA. Earlier branch heads remain ancestors; all their follow-ups continue
on this branch.

This issue owns exactly:

- `scripts/economic_sql_audit_origins.py`;
- `scripts/economic_restore_evidence.py`;
- `tests/async/test_economic_sql_audit_origins.py`;
- `tests/async/test_economic_sql_canonical_audit.py`;
- `tests/async/run_economic_sql_audit_snapshot_mysql.py`;
- `docs/persistence/economy_accounting/AUDIT_OPERATIONS.md`;
- `docs/persistence/economy_accounting/PLAN5_SQL_COMMAND_PREIMAGE_HANDOFF_2026-10-05.md`;
- this qualification report.

No production, coordinator, mutation contract, migration, registry/matrix or
activation-owner file changes. The narrow primary request is to add five pure
methods to the two existing central manifest rows' `required_cases` and
`arguments`, preserving all prior methods, row count105, flags and policy.
[The exact field/invariant/consumer/test packet](PLAN5_SQL_COMMAND_PREIMAGE_HANDOFF_2026-10-05.md)
contains the complete request. No new SQL or public JSON field is proposed.

## Exact tested source

Maintained `src`, `migrations`, code/test and public data trees differ from
`1ad6c17a688afe0cdfa4083d67f443283f7db229` only at the five owned Python files.
Native tree remains `bf7a92a728ad9b5b813626462e56533f8ba39c97`; migration tree
remains `2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`, canonical61.
Earlier fresh726-unit SQL/flatfile maintained builds retain their native-source
qualification; no C/C++ changes require a new maintained build for this issue.

The immutable reference archive is the full6173-file public package at
`d1487d1f25b57e4973e27fe424767882a9b6abd0`. The successor replaces only the five
owned Python files and adds nine ignored preservation/observation helpers,
yielding6182 regular files plus two committed help-file links. Reference
documentation stays at that frozen snapshot; later documentation-only branch
parents are identified as publication parents, not substituted into the archive.
All maintained code/test/public data inputs match the final candidate's five
file changes. Actual native recipes retain original cases, flags, deadlines,
cache-off policy, sanitizers and disposable-socket guard.

Final archive SHA-256:
`17a2ca6a18b4d4c21591262277c667f0ee2aab299e50af1a36d8c7115017fadd`.

| Exact owned executable/test file | SHA-256 |
| --- | --- |
| `scripts/economic_sql_audit_origins.py` | `49b8082b2d57a5c25554ccccbff150b5e17498f838a7e2ccc8da504e17334017` |
| `scripts/economic_restore_evidence.py` | `930cf8b8a43dd82ebfa1b23a642ec2f63036718a06d5607ad7f019ea2b69a3bf` |
| `tests/async/test_economic_sql_audit_origins.py` | `1a281b8860228a7e6722466916ff0b5bd66f2677ccc552f3fbc37ed5f5b4ceba` |
| `tests/async/test_economic_sql_canonical_audit.py` | `e21cb7296b22f0406eb4f7f9424204334e4d665a21081fe15b1c25e80107d2b9` |
| `tests/async/run_economic_sql_audit_snapshot_mysql.py` | `04d9b855a87f4b71578b79153a39ac247a1a48722cf0adb5da24cbace9e16d2e` |

The probe at sequence02 shares the two final reader hashes. The native origin
owner at03 additionally shares the final origin-test hash; the later modeled
snapshot adaptation is not a consumer of that owner. Sequence06 tests all five
final hashes together. Source maps before/after and terminal launcher records
retain these exact scopes rather than transferring an earlier whole-candidate
qualification.

## Commands and results

Pinned image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Containers are isolated with `--network none --read-only --cpus 2`, frozen
source extraction, read-only original/historical evidence and fresh private
databases. Final original baseline preservation uses4GiB memory and3GiB artifact
capacity after a retained1GiB preservation failure; this changes no native
recipe, case, assertion or deadline. Other stages retain2560MiB/1GiB limits.
Every exact Docker command is in `docker-command-<stage>.json` beside its source
map and authoritative exited0/non-OOM container state.

Outer command, with `<root>` replaced by its absolute evidence directory:

```text
python -B tmp/plan5/launch-command-preimage-windows.py <root> probe
python -B tmp/plan5/launch-command-preimage-windows.py <root> origins
python -B tmp/plan5/launch-command-preimage-windows.py <root> original
python -B tmp/plan5/launch-command-preimage-windows.py <root> pure
python -B tmp/plan5/launch-command-preimage-windows.py <root> legacy56-green
```

| Stage and root sequence | Actual command/owner | Outcome |
| --- | --- | --- |
| Genuine paired corruption proof,02 | `probe-command-preimage-native.py`, original native replay and separate original/repaired reader processes | exit0,17.284907s; each engine has4 old-reader accepted cuts,4 repaired-reader refusals and2 original native nonzero refusals; SELECT-only/1142/all18 authority tables unchanged |
| Native origins,03 | unchanged `retain-baseline-contract.py` → original `NativeSQLOriginTests` plus two new valid-positive cuts per engine | exit0,146.977555s;2 methods,0 skips, fresh ASan/UBSan fixture84.311s/0 reuse; original7 captures/7 refusals/14 rollbacks, additional2 command cuts and16 separate position projections per engine |
| Original baseline,06 | same retainer → original `NativeBaselineAuditTests.test_native_baseline_claims_both_canonical_engines` | exit0,256.044911s; original method197.321s/0 skips; each engine161 corruption cuts,161 restore refusals,7 constraint refusals and exact cold native replay with18 tables unchanged |
| Full Linux pure,06 | original10-owner `python3 -u -B -m unittest -v` batch | exit0,291 methods/0 skips,45.865110s; normal/matrix/runtime also exit0; release exit1 with `writer has no executable evidence` |
| Genuine historical upgrade,06 | unchanged `qualify-retained-legacy56-current61.py legacy56-green` | exit0,8.961073s; original56 dumps →5 additive steps →61; both engines preserve2 actual EAB1 roots with NULL times, all18 original tables, exact original replay and idempotence |

The Linux pure selectors are `ReconciliationTests`, `ChildIdentityTests`,
`ItemRevisionTests`, `OriginTests`, `BaselineVersionTests`, `CanonicalAuditTests`,
`RestoreProjectionTests`, and the full coverage-contract, audit-invariant and
immutable-migration modules. Normal commands are
`validate_economy_accounting.py`, `generate_economy_writer_coverage.py --check`,
`validate_runtime_compatibility.py` and `validate_economy_accounting.py --release`.
Their complete argv, logs, exit statuses and individual runtimes are retained.
Final focused Windows controls independently pass41+20 methods/0 skips.

Both engines are actual MariaDB10.11.14 and MySQL8.0.46. The historical exporter
truthfully retains `evidence_loss:1`, `missing_native_holding:2` and
`missing_native_item:2`. NULL compatibility never asserts a known command
preimage or complete capture. Original native negative replay invocations retain
their nonzero assertion refusal logs; they are not relabelled as positive passes.

## Evidence and limits

Common root: `D:\CodexEvidence\accounting-plan5\bin`.
Genuine paired proof is `current61-command-preimage-02-20261005/probe`;
original native origins are `current61-command-preimage-03-20261005/processes/origins`.
Final all-five source and qualification outputs are under
`current61-command-preimage-06-20261005`: `processes/original`,
`full-pure-linux`, `legacy56-green`, immutable source/map/helpers, exact commands
and terminal states. Focused final Windows evidence is at05/pure.

Receipt: `tmp/plan5/current61-command-preimage-qualification-evidence.json`,
also copied beside the final archive. The seal checks complete retained regular
files and reads any Linux symlink pointer without following its target. This
artifact set has8264 verified regular files and0 retained pointer records;
the two public help links remain separately verified inside the source package.
The seal uses Windows extended paths for the original long native witness names.
Receipt SHA-256:
`82a74e7a39da6dbbf0db919f646329af40783d9f585b0c8dad9f9d5c8e804eb3`.
The post-publication delivery record binds the exact result/remote and reports.

All failed predecessors remain recorded: helper directory collision; original
socket-prefix refusal; invalid UINT64_MAX+1 fixture cut/1264; the modeled inbox
missing-column1054; Windows291-method POSIX errors;1GiB preservation capacity
failure; and the initial Windows long-path seal error. Fresh successors preserve
the native guards and cases. No failed run is waived or relabelled as passing.

No selected successor check is skipped. The complete project qualification,
updated central registration, primary combined-candidate qualification and
original Plan5/R1–R8 writer/world/player/workload gates remain. The separate
retention/managed results retain their exact earlier source; this reader issue
does not transfer them to a new whole-candidate release. Native mutations,
activation, deployment, production data and inactive spell behavior are unchanged;
wallet-root item exclusions remain. The published handoff/qualification pair is
the notebook curator packet for the primary's locally maintained shared notebook.
