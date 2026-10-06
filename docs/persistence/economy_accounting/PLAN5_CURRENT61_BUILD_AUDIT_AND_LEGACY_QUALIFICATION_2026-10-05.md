# Plan5 current61 maintained builds, native audit and retained EAB1 qualification

Both maintained server builds, the original both-engine native baseline audit,
native-origin checks and genuine retained EAB1/schema56 compatibility checks pass
on the installed schema61/EAB2 source. The original286-method focused batch has
one shared provenance failure, preserved separately. Four original retention and
managed-restore runs remain open during a Docker/WSL infrastructure stall; their
partial observations are not completed qualification. Full capture, producer
coverage, release and activation remain incomplete.

## Exact source, branch and ownership

- Worktree: `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
- Local and remote branch: `codex/accounting-plan5`. No direct push to
  `experimental-accounting`, PR merge, deployment or activation.
- Refreshed primary: `7e7e85146c340c154a2c977225f31ff8fa54c7da`.
- Previous Plan5: `cab37a26145ed901a66cb9a6ced132c0c72e384a`.
- Preserving merge and frozen tested source:
  `1ad6c17a688afe0cdfa4083d67f443283f7db229`.
- Report parent and published shared-pin handoff:
  `75349a0c7012251cdbbe6d72bf0cf7e09fe2dd69`.
- Native tree: `bf7a92a728ad9b5b813626462e56533f8ba39c97`.
- Migration tree: `2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`.

This slice owns this report. Shared production, migration, coordinator, registry,
matrix and activation changes come only from the primary's published integration.
The only merge conflict was in owned
`tests/async/test_economic_sql_audit_origins.py`: the selected version-aware
projection is exactly the incoming primary blob
`f95c89c77ef98752a233bd414fdc66a3ab5ad989`. It expands historical version1
fixtures only and retains actual native version2 equipment positions. Every
strict representation/position control remains. The frozen test directories
remain byte-identical through the report commits.

All six earlier accounting branches are verified ancestors of the tested base:
`codex/accounting-audit-release`, `codex/accounting-plan5-canonical-plans`,
`codex/accounting-plan5-coherent-0056`, `codex/accounting-plan5-published-0056`,
`codex/accounting-qualification-contracts`, and
`codex/accounting-recovery-evidence`. The former coherent remote tip
`42141f6268707787b44d3b95bab2290ae3898b87` is also preserved. Their exact heads
and ancestry assertions are in the aggregate receipt. Subsequent fixes and
follow-ups are published on the same `codex/accounting-plan5` branch.

## Environment and original commands

All native checks use pinned image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
GCC13.3 and Python3.12.3, read-only source, no container network or host ports,
private executable temporary filesystems and disposable database data directories.
Actual engines are MariaDB10.11.14 and MySQL8.0.46. No source `.env`, production
credentials or production database are used. Recorded source maps are rechecked
after execution and again while sealing. Original assertions, cases, sanitizer
flags, cache policy and test deadlines are preserved.

Commands below are executed helper entry points; their exact bodies and all
underlying build/test commands are retained outside Git with the evidence.

| Command | Actual result | Scope |
| --- | --- | --- |
| `python3 -u -B tmp/plan5/run-current61-build.py sql` | exit0;757.491273s | Original `make -C src -j2 PERSISTENCE_BACKEND=mariadb`,726 distinct units/objects,0 warnings/errors/reused objects |
| `python3 -u -B tmp/plan5/run-current61-build.py flatfile` | exit0;772.132940s | Original `make -C src -j2 PERSISTENCE_BACKEND=flatfile`,726 distinct units/objects,0 warnings/errors/reused objects |
| `python3 -u -B tmp/plan5/run-current61-checks.py pure` | original batch exit1;215.188795s | 286 methods,0 skips,285 pass,1 preserved source-pin failure; normal/matrix/runtime exit0; release exit1 |
| `python3 -u -B tmp/plan5/run-current61-checks.py original` | exit0;466.210245s | Original `NativeBaselineAuditTests`,1 method,0 skips; both engines/current61, strict ASan/UBSan and client-free native mode |
| `python3 -u -B tmp/plan5/run-current61-checks.py origins` | exit0;115.955914s | Original2 `NativeSQLOriginTests`,0 skips; both engines/current61/native EAB2 |
| `python3 -u -B tmp/plan5/qualify-retained-legacy56-current61.py legacy56-green` | exit0;both engines | Actual retained native EAB1 dumps/fixture,56→61,read-only audit/native replay |
| `python3 -u -B tmp/plan5/diagnose-current61-provenance.py` | exit0;14.909288s | 71 original coverage/audit methods,0 skips; only two pin values in copied JSON; original metadata remains unchanged |

Both original make commands set `BIN_ROOT`, `OBJDIR` and `DMS_BINARY` to fresh
owned `/evidence/builds/<backend>` paths. Actual maintained server SHA256:

- SQL: `21755a0be2bbd9f51fb1133167532a6ecf23bd963bda07d29e759199112ca49f`,
  193109920 bytes; build-log SHA256
  `033533afa6fd15f6712a2c3dafbc2db0f80699f1c5f9081814769d636f44e200`.
- Flatfile: `2087640a44081883ffffdb5e5a4929c7c8fce5c12f353518c6ecfcd3f82f3daa`,
  171078472 bytes; build-log SHA256
  `495328b2eda099ad07837baf40072af8033cdbbcfe29890bbab3fb15c2bf1b40`.

## Completed native and compatibility evidence

The original current61 audit executes161 corruption cuts,161 restore refusals
and7 SQL constraint refusals per engine. Both cold dump clones run the full
qualifier and exact native receipt replay, preserve all18 compared tables and
retain a null active epoch. Actual native equipment positions5/6 and EAB2
version2 are checked; the original historical EAB1 committed-root diagnostic
remains. Preservation changes only the original temporary-directory root to a
short private executable path, retaining the existing socket-length guard.

The native SQL fixture digest is
`72d6d5027b5ed550921efd6b11d14b120dacc1b42d53b6ba32406f96b3f54f2c`;
client-free fixture digest is
`dd503bd0c1ba66557ff96a9a2004d5007d60525c5c7259bdb5224ced1072ce0d`.
Original audit log SHA256:
`faf85c0090aebc7c0e794b285a615d68e3d1a233cd8e68fcb035a2f1b5dedc35`.
The MariaDB clone dump is337141 bytes,
`db4ff1cc1bf440f8e69fc07d312bf090bafc67e9542dd868c77d6292443769e4`;
MySQL clone dump is345922 bytes,
`a702b2d39c5c21e44efe466f7b58c899480d242e5b8c026c22386e6e55718f9e`.

The origin owner runs7 captures,7 refusals and14 transaction rollbacks per
engine, plus16 controlled EAB2 position-projection refusals per engine. These
are SELECT-only checks with unchanged authority; controlled projections remain
explicitly full-native-capture-unqualified. The origin log SHA256 is
`ea048379c9f444f677c9ad58950089498e46eaf28ff669ba2eaa23b3199cc9c2`.

Historical compatibility imports each engine's actual previously retained
schema56 native dump into a verified empty private database. It applies the5
canonical intervening migrations, reaches61 and reruns idempotently with no
pending steps. Original18 tables/old-column values remain exact. Both retained
witnesses remain actual EAB1/version1; their new
`economic_baseline_witness.command_accepted_at_usec` values remain NULL.
The original archived native fixture runs its unchanged `--reconcile` replay
on the upgraded databases and returns the original operation/epoch/lineage
results. This is archived producer replay on the new database, not a claim
that the current server replays historical commands.

The current independent canonical reader accepts both retained roots. Its
independent exporter/reconciler truthfully returns `evidence_loss:1`,
`missing_native_holding:2`, `missing_native_item:2` for the incomplete historical
fixture. No native world holdings are invented. UPDATE is denied with1142 and
all captured authority remains unchanged. Historical dump/native binary pins
and actual engine results are retained in `legacy56-green`.

The first legacy helper attempt queried the new admission field on the wrong
SQL table and failed with1054. That attempt remains in `legacy56` with its exact
executed helper, inputs, import log and pre-audit authority. The corrected helper
changes only that table lookup and uses entirely fresh databases/evidence paths.

## Evidence, shared request and remaining gates

Physical evidence root:
`D:\CodexEvidence\accounting-plan5\bin\current61-7e7e-20261005`.
Completed stages are `builds/sql`, `builds/flatfile`, `checks/pure`,
`checks/original` (2128 originally inventoried files), `checks/origins`
(1371 files), `legacy56`, `legacy56-green` (20 files), and
`provenance-diagnostic`. Aggregate receipt:
`tmp/plan5/current61-completed-evidence.json`, also copied into the physical
evidence root. It verifies retained bytes/digests, original inventories, source
maps, ancestor heads, exact commands and result records:6468 artifacts and
3079 files in each recorded source map. Aggregate receipt SHA256:
`ec51b2519edd4d200d723bb1d6c1a6234d59ca7fea59f64f708461cd710daf9f`.
Result commit and
verified remote head are recorded after commit in the delivery receipt.

The only new shared request is the existing two lifecycle source-pin replacements
in primary-owned registry/matrix provenance, detailed in
[the published narrow handoff](PLAN5_CURRENT61_SOURCE_PIN_HANDOFF_2026-10-05.md).
The original failure is not waived: proposed-copy71-method success establishes
the repair, while actual shared provenance remains failing until the primary
applies it. Release validation still says `writer has no executable evidence`.
Normal validator/matrix success does not authorize release.

Original retention handles remain73919(SQL) and38397(flatfile); managed restore
handles remain16347(SQL) and99469(flatfile). Exact commands are
`python3 -u -B tmp/plan5/run-current61-retention.py <sql|flatfile>` and
`python3 -u -B tmp/plan5/run-current61-managed.py <sql|flatfile>`.
The SQL log has completed the MariaDB account/character journeys and2 canonical
corruption refusals; the flatfile inspector has completed18 zone-story journal
boundaries. These are partial logs, not completed suite results. Docker status
calls return500 or time out; a read-only WSL observation shows low available
memory and exhausted swap. The original four handles remain open and have not
been restarted, cancelled or reclassified as passing. Timestamped infrastructure
observations are retained separately. Current managed SQL/flatfile restore and
complete both-engine retention qualification remain open for follow-up.

An additional owned follow-up remains for schema58's retained admission time:
the current independent origin and restore witness projections do not select
`command_accepted_at_usec`. Native replay checks a present value against the
original command, while independent root verification deliberately uses the
normalized admission time1. The completed161-cut suite does not establish
independent detection of corruption in this new retained field or reconstruction
of the original inbox command hash from it. Establish that cut on actual new
native rows before claiming admission-time qualification; preserve historical
NULL compatibility and never infer or backfill unknown original timestamps.
This is an owned reader/test follow-up, not an independently applied shared
schema or producer change.

The primary's full original Plan1/producer/real-player/world capture and R1–R8
acceptance remain open. Private quest candidates and SHOP component results
retain their documented source/scope; this report does not install or qualify
them. Inventory, controlled projections, synthetic native fixtures and isolated
passing methods cannot establish complete release. Accounting remains inactive,
wallet-root exclusions and the declined inactive spell-path change stay intact;
no audit finding is auto-corrected. This report is the evidence packet for the
primary's locally maintained notebook curator, as directed by the user.
