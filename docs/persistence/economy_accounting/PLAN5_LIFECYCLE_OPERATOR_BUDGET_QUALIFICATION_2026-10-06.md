# Plan 5 maximum lifecycle operator read budget — 2026-10-06

The independent one-shot flatfile operator no longer refuses the supported
3,071-holding native lifecycle fixture solely because its read-count budget is
below the workload's physical reads. The cap is now 16,384 reads; the existing
128 MiB input, 8,192 directory-entry and 30-second cooperative limits remain.
Repeated physical reads through bounded caches still count. Retained-root and
authority-link pages keep their explicit 64-read/32 MiB bounds.

## Publication and ownership

Branch `codex/accounting-plan5`, worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`, base `21f5896f6a8f7601ac9bea400f842781c962536f`.
The protected delivery receipt binds the exact result commit/remote head, clean
worktree, all tested source payloads/modes/links and seven preserved earlier
branch tips. Existing follow-ups remain ancestors; no history rewrite or
independent experimental-accounting push.

Owned files are `scripts/qualify_flatfile_economic_authority.h`,
`tests/async/test_flatfile_restore_lifecycle_receipts.py`,
`docs/persistence/economy_accounting/AUDIT_OPERATIONS.md`, this report and
`PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`. Native sources, mutations/producers,
coordinator/contracts, schemas/migrations, writer registry/matrix, activation
owner, shared manifests/runners are unchanged. No shared accounting interface or
schema change is requested. The existing registered
`flatfile_restore_lifecycle_receipts` script already runs its maximum case and
now enforces the measured native probe; no shared registration change is needed.

## Defect and complete fix

Frozen original source `0c73e5fda235fb40023ddb489d8f10650a04037c932350d40bde7a8eb8d41cd4` constructs the unchanged native-codec
`maximum` fixture: 3,071 modeled bank holdings, one inactive lifecycle-owned
epoch, its original common baseline command/witness/plan and receipt. A SELECT-
equivalent independent probe runs the original decoder with its read lock.
With 2,048 reads it throws the typed budget refusal after 2,478,480 bytes.
The original operator exits 1 with its fixed diagnostic. With only the read
counter relaxed for diagnosis, the same original bytes verify one lifecycle
receipt using 9,574 reads and 19,639,289 bytes in 0.119708 seconds. Authority
bytes/modes/links/inodes/sizes/mtimes remain exact through all observations.

The native fixture fits the byte/time bounds but cannot fit the prior count.
Its mappings cross the bounded eight-bucket caches, so repeated reads count
individually; the fixture's healthy shape and original decoding are preserved.
The repair changes one default admission cap to 16,384. It adds no cache, codec,
dependency, mutation, semantic waiver or correction. Explicit smaller page
budgets retain their independent caps and refusal behavior. Workloads exceeding
any count/byte/directory/time limit still refuse. This maximum component workload
does not qualify growing history or the release host.

The existing lifecycle regression builds a sanitizer-instrumented independent
probe using the real default budget and read-lock entry point. It checks the
configured caps, successful receipt link, more than 2,048 actual reads, all other
bounds and unchanged full-state metadata. Its protected maximum-case row records
the exact 9,574 reads, 19,639,289 bytes and 2675 directory entries. The
original decoder/restore corruption controls remain mandatory.

## Exact sources, commands, backends and results

Final component archive SHA256 `85c627c4621890d51d54b0190a4439cc0ee37878c810ee1d8e9fc40300e0154e` contains 6,379 regular payloads
and four repository links, based on `21f5896f6a8f7601ac9bea400f842781c962536f` with exactly two code/test overlays.
Protected source transport binds bytes/modes, native tree
`4abb609524a1f1682ea4c190f82d75003c4d679b`, migration tree `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`, canonical 0062.
Operator/publication documents are excluded from executable qualification.
Delivery checks every other published source payload/mode against this archive.

All commands execute in immutable image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
with no network, two CPUs, 4GiB memory, 3GiB `/workspace` and 2GiB `/tmp` tmpfs,
no maintained database/runtime mounts, and build cache disabled. Exact Docker
argv/helpers/observers, source verification, logs, binaries and reports are
retained. Native fixtures use ASAN leak/error and UBSAN halt-on-error settings.

| Command | Result |
| --- | --- |
| `python3 -u -B tests/async/test_flatfile_restore_lifecycle_receipts.py --artifacts /workspace/bin/tests/lifecycle-budget-receipts` | 131 cases, 9 accepted / 122 refused, zero skips, exit 0 |
| `python3 -u -B tests/async/test_flatfile_restore_economic_authority.py` | 20 positive stores, 367 refused corruptions, 28 root-page and 29 authority-page controls, zero skips, unchanged evidence, exit 0 |
| `python3 -u -B tests/async/test_flatfile_restore_baseline_markers.py --native-source /workspace --artifacts /workspace/bin/tests/lifecycle-budget-markers` | 69 cases, zero skips, exit 0 |
| `DURIS_PLAN5_CANONICAL_NATIVE=1 DURIS_PLAN5_CANONICAL_SOURCE=1 DURIS_PLAN5_CANONICAL_MOBILE=1 DURIS_PLAN5_CANONICAL_ARTIFACTS=/workspace/bin/tests/lifecycle-budget-canonical python3 -u -B tests/async/test_economic_sql_canonical_audit.py` | 67 methods, zero skips, fresh MariaDB 10.11.14 and MySQL 8.0.46, canonical 0062, exit 0 |

The SQL suite retains SELECT-only denial 1142, original native plan/source/mobile
checks, database inventory equality and all durable scheduling/CLI refusal
checks. Every native/damaged fixture remains scoped component evidence. Neither
source capture nor production lifecycle installation/activation is executed by
the lifecycle fixture; it does not prove genuine writer/gameplay acceptance.

Both maintained production builds use a separate frozen archive
`4fca1b6603a8bbe9d39d11dca3be4abacc237a78f2d75ea5c160b34f43b71c10`. Only the unused test collector's local integer variable differs
from the final archive: the first metrics collection accidentally shadowed its
fixture dictionary. AST closure proves all other test bodies/provider helper
and generated native probe text remain identical; all 6,378 other source
payloads/modes and four links are exact. All native/build/provider inputs and
the budget header itself are identical. The failed component attempt is
preserved and excluded; its completed independent builds keep their exact scope.

```sh
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=mariadb \
  BIN_ROOT=/workspace/bin/tests/lifecycle-budget-build/bin \
  OBJDIR=/workspace/bin/tests/lifecycle-budget-build/objects \
  DMS_BINARY=/workspace/bin/tests/lifecycle-budget-build/server
# Separate fresh container/workspace for PERSISTENCE_BACKEND=flatfile.
```

Each backend compiles all 740 objects and dependency records, with zero reuse,
warnings or errors, exit 0. SQL: 275.1491628799122 seconds, server SHA256 `5e3e8529dc5701ec52f32a99f481e4fc77b80591561955835490779baba38ac8`.
Flatfile: 251.4534769959282 seconds, server SHA256 `f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4`. All compiled outputs
remain under bin/ and protected evidence; none is committed.

Changed-line formatting via `wsl --cd <worktree> bash ./scripts/format.sh --file
scripts/qualify_flatfile_economic_authority.h --check` and `git diff --check` exit
0. Normal accounting validator exits 0: 14 fixtures, 920 routes, 2,876 candidate
sites, `release_ready=False`. Release mode exits 1 with `writer has no executable
evidence`. Central regression inventory validates 920 owners. These source and
inventory checks do not establish release completion.

Primary refreshed published head `3b7a465af40cd6867d3dcb3c52aaa9200d38661b`, native tree `01291db15446d94f36e066032aa3a1eea28ef354`, shares
the recorded migration tree but has different native inputs. This slice does
not qualify its private fixes or the primary's combined candidate. Earlier
0055/0056 evidence cannot qualify these canonical0062 inputs.

The refreshed review checkpoint records private 753-provider builds, genuine
MySQL warm publication, and unresolved cold admission/template prerequisites;
MariaDB and original fault cases remain pending. The published
[ANF2/ACT2 reader interface](https://github.com/Community-Duris/Duris/blob/3b7a465af40cd6867d3dcb3c52aaa9200d38661b/docs/persistence/economy_accounting/AUCTION_NATIVE_TREE_PLAN5_INTERFACE_2026-10-06.md)
was reviewed. Its 124-byte facts extension, full per-root custody literals,
original-listing forest/revision verification and claim-subset ordering stay
pending until producer, independent reader and original runtime proof meet on
one qualified candidate. Existing version1 evidence is preserved; no new private
format or restored callback/publication capability is claimed here.

## Evidence, curator packet and remaining gates

Evidence under `D:/CodexEvidence/accounting-plan5/bin/`:

- `lifecycle-budget-red-01-20261006`: original-budget/diagnostic measurements and unchanged native state
- `lifecycle-budget-green-02-20261006`: final source, four complete component batches and native artifacts
- `lifecycle-budget-green-01-20261006`: preserved failed metrics collector / exact production build source
- `lifecycle-budget-{sql,flatfile}-build-01-20261006`: both fresh maintained builds
- `lifecycle-budget-gates-02-20261006`: final-source formatting/contract/inventory/whitespace logs
- `lifecycle-budget-seal-01-20261006/evidence.json`, SHA256 `a95a5c6f2aefb0b9b4899d76926b278188fabbe9641076ac1bbc3c1b45d7ce01`
- `lifecycle-budget-delivery-01-20261006/delivery.json`: exact result/remote and source/ancestry closure

The seal hashes 12330 retained artifacts, 6068389641 bytes, final source/report
and closed container states. No retained daemon remains running. This report,
appended remote follow-up and delivery/seal constitute the curator packet for
the primary's nonblocking local notebook. No application, acknowledgement or
direct cross-chat message is claimed. Primary integrates and publishes the
tested combined candidate.

There is no independent blocker for this solved budget issue. The earlier
`lifecycle-page-fairness-red-01-20261006` observations preserve the still-missing
native page interface and operator scope on two healthy current/retained-epoch
fixtures. Durable catalogue-driven lifecycle receipt/root pages remain the next
owned slice. Complete baseline/lifecycle/orphan progress and reconstruction,
R7/R8, native holding/UID/writer closure, genuine player/fault/restart/replay,
current combined backup/restore/retention, primary cold-world/journal/private
producer qualification and measured growing-history/release-host budgets remain.
Maintained accounting is inactive; wallet-root exclusions and the declined
inactive spell change remain exact. No deployment, PR merge, production data
change, activation or auto-correction occurs. Full Plan5/release stays incomplete.
