# Plan 5 flatfile page consistency — 2026-10-07

The independent flatfile operator now reports `consistent_page=false` when
the current page retains a semantic finding, including a normally returned
native page with only one of two selected entries verified. Healthy later pages
retain their own `consistent_page=true` while prior findings remain sticky and
the CLI continues to exit 1. This repair does not qualify complete reconciliation.

## Publication and ownership

Branch `codex/accounting-plan5`, worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`, base
`e9d498f655b81b8947709914a0ddcc35a6a4559d`. The protected delivery receipt binds the exact result commit,
remote head, clean worktree and tested source bytes/modes/links. All seven
recorded earlier branch tips and their follow-ups remain ancestors; history is
not rewritten and no independent experimental-accounting push occurs.

Owned files are `scripts/flatfile_economic_audit.py`,
`tests/async/test_flatfile_restore_economic_authority.py`,
`tests/async/test_flatfile_restore_lifecycle_receipts.py`,
`docs/persistence/economy_accounting/AUDIT_OPERATIONS.md`, this report and
`docs/persistence/economy_accounting/PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`.
There is no shared contract, schema, producer, coordinator, writer registry/matrix,
activation owner, manifest or runner change/request. Both strengthened test
entry points are already registered.

## Established defect and complete owned repair

Fresh original source archive SHA256 `d3760f1fb089d3d703dcf465d1524e8b8aecd40c0aa0b9ef166f89fa4e055690` completes both original native entry points.
Actual CLI observations independently reproduce the defect in all three scopes:
`retained_record_page`, `authority_crosslink_page` and
`required_lifecycle_receipt_root_page`. Each corrupted page examines two entries,
verifies only one, retains a specific finding and exits 1, yet reports
`consistent_page=true` and `page_refused=false`.

The operator's only implementation change makes current-page consistency
require both no refusal and no current-page findings. It does not use the
accumulated checkpoint finding count: a healthy later page must preserve its
local result without clearing prior exceptions. Existing native decoders,
mutation separation, cursor/fence admission, refusal/timeout rotation, private
atomic checkpoint/owner locking, cumulative counts and CLI exit policy remain.
The checkpoint remains bound to operator source, so an earlier-source checkpoint
refuses under the changed source and requires its own fresh checkpoint path.

Existing native regressions now assert healthy, semantic-finding, refused,
timed-out and healthy-with-sticky-history reports across all three scopes.
They retain the original source and native corruption controls and their
unchanged-state assertions. No native fixture, codec or production input changes.

## Exact tested source, commands, backends and results

Final component source archive SHA256 `ab84f7d2be7d3a2a98abd01377b6bd0089d559fb492e10295168fefd2a8765a9` has
6,381 regular payloads and four links on the recorded base. Only the operator
and two Python regression files differ from the original source; every other
payload, mode and link is identical. Native tree
`4abb609524a1f1682ea4c190f82d75003c4d679b` and migration tree
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` remain exact (canonical 0062).
Publication/operator documents are excluded from executable qualification.

Both original and repaired qualifications use fresh containers with immutable
image `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
network disabled, two CPUs, 4 GiB memory, 3 GiB workspace/2 GiB temporary tmpfs,
no maintained runtime/database mounts and build cache off. Original native
fixture and independent probe compilation retains its ASAN/UBSAN flags;
ASAN leak/error and UBSAN halt-on-error settings remain enabled.

The exact launcher is `python3 -u -B /loader.py`; it verifies/extracts the source
archive, then runs `python3 -u -B /evidence/observer.py red` or `green`. The observer
executes the following original entry points sequentially with their exact
`sys.argv` through `runpy.run_path(path, run_name="__main__")`. Its wrapper records
actual CLI subprocess responses without changing commands, results or assertions.
The retained preparation, loader, observer and Docker argument arrays make this
execution method explicit; the command labels below are equivalent CLI forms.

| Original entry point / equivalent CLI | Result on repaired source |
| --- | --- |
| `python3 -u -B tests/async/test_flatfile_restore_economic_authority.py` | 20 positive stores / 367 corruption refusals; 28 root-page / 29 authority-page controls; 1,058 metadata / 574 envelope comparisons; exit 0, zero skips |
| `python3 -u -B tests/async/test_flatfile_restore_lifecycle_receipts.py --artifacts /workspace/bin/tests/page-consistency-receipts` | 131 original cases (9 accepted / 122 refused), all 46 lifecycle page controls; exit 0, zero skips |

The observer additionally authenticates actual semantic findings, healthy pages,
refusals and healthy pages with sticky prior findings in every scope. Every
semantic finding now reports false; every healthy sample reports true; every
refused sample reports false. All complete/sweep/release flags remain false.
Both original batches also finish with zero skips; their retained observations
are negative evidence for the old report behavior, not a passing repaired claim.

The real native tests retain evidence/domain bytes and lifecycle file metadata,
all original semantic/physical corruption cases, read-only locks, pending journals,
private checkpoint constraints, interrupted writes and unchanged cursor/fences.
These are native-code component fixtures, not authentic producer/gameplay proof.
SQL/disposable-database reruns and server rebuilds are not applicable to this
Python report/test/documentation-only repair; no SQL, server, provider, schema,
native header or fixture input changes. No new SQL/build qualification is claimed.

Exact source gates:

- `git diff --check`: exit 0.
- `python -B scripts/validate_economy_accounting.py`: exit 0; 14 fixtures,
  920 writer routes, 2,876 candidate sites, `release_ready=False`.
- `python -B scripts/validate_economy_accounting.py --release`: expected exit 1,
  `writer has no executable evidence`.
- `python -B -m unittest discover -s tests/async -p test_economy_writer_coverage_contract.py`:
  all 55 methods pass, zero skips.
- The retained exact regression-inventory command validates all 920 owners,
  exit 0. There are no new registration rows or C/C++ formatting requirements.

## Evidence and curator handoff

Protected evidence is under `D:/CodexEvidence/accounting-plan5/bin/`:

- `flatfile-page-consistency-red-01-20261006`: frozen original source, actual
  misleading CLI reports, both full original entry points and native artifacts.
- `flatfile-page-consistency-green-01-20261006`: final frozen source, both full
  strengthened entry points, actual corrected CLI reports and native artifacts.
- `flatfile-page-consistency-gates-01-20261006`: all five source gate results/logs.
- `flatfile-page-consistency-seal-01-20261006/evidence.json`, SHA256
  `926f6ac59c37ac2dc19ad62f91df9345b838362e771c04f16e16ab5d2ab2bfa8`: 11,734 artifacts / 1,178,693,152 bytes and both terminal
  container states.
- `flatfile-page-consistency-delivery-01-20261006/delivery.json`: exact
  result/remote/source/ancestry binding.

This report, appended remote follow-up and protected seal/delivery are the curator
packet for the primary's nonblocking local notebook. No application,
acknowledgement or direct cross-chat message is claimed.

## Remaining gates

There is no independent blocker for this completed report repair. Full Plan 5,
R7/R8 and release remain incomplete: baseline/reservation/lifecycle/orphan
namespace closure, current native holdings/UID/source/writer reconstruction,
genuine player/fault/restart/replay, current combined backup/restore/retention,
growing-history/release-host budgets and complete executable writer coverage.

Refreshed primary `01c5ebf3698dff6815a643e39c16b81b6345fcd8` has native tree
`01291db15446d94f36e066032aa3a1eea28ef354`. Its combined producer, cold admission/template
prerequisite, private canonical 0063 and pending MariaDB/fault cases require
their own exact published combined qualification. The latest checkpoint's
separate flatfile/SQL component milestones grant no full release proof.
Maintained accounting stays inactive; wallet-root exclusions and the declined
inactive spell-path change remain exact. No activation, deployment, PR merge,
production change or audit auto-correction occurs.
