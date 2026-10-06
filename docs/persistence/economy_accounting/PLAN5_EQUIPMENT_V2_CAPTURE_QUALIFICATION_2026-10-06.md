# Plan 5 equipment capture v2 qualification — 2026-10-06

The primary's current equipment capture v2 passes the original native source
recipe and independent read-only exporter recipe on both private engines. Both
maintained production backends build and link cleanly from fresh objects on
this exact source. All selected native/exporter recipe invocations have zero
skips. Full Plan 5, R6/R7/R8, coverage, release and activation remain unfinished.

The separate [published source pin handoff](PLAN5_PUBLISHED_SOURCE_PIN_HANDOFF_2026-10-06.md)
establishes five shared registry pins for CRLF bytes rather than the published
LF blobs. That contract failure remains visible; it is not waived by these
component passes and has not been independently edited by Plan 5.

## Source, branch and ownership

- Worktree: `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
- Branch and sole remote publication destination: `codex/accounting-plan5`.
- Previous publication/import parent: `4b86e892cae1fc9e0eefc29dc52e2549706e4460`.
- Exact imported primary: `8795a0b086a7f581c6c4b8080bc83245646fbf5d`.
- Conflict-free import result: `4b89f6116edeb12f7fe3e324ee9a3e94c1e2d7d3`.
- Tested source and handoff publication: `e1f50802b80d5c788bd4909a169a41348fd7d5b7`.
- Native tree: `03a97173396f720857b1ad58a2ab69b7859a2387`.
- Migration tree: `2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`, canonical 61.
- Resulting report commit and remote ref are bound by the post-commit delivery
  receipt; there is no self-referential commit value in the report.

Every production, migration and script blob equals the pinned primary. Its
shared registry/matrix, lifecycle interface, producer files and coordinator
documents were imported exactly, with no independent shared rewrite. The
earlier separately committed Plan 5 fixes remain, including all nine actual
native-provider additions, the exact historical child-preimage expectation and
the census fixture's abort on an unexpected native quest publication.

All seven earlier branch tips remain ancestors on this same branch. Source
comparison, full raw pin comparison and ancestry proof are retained in
`equipment-v2-current-01-20261006/import-proof.json`. Current source is refreshed
before selection and during execution. Final refresh after the batch observes
primary `11a83b62c1f207e09640a6c282d92328aabb8935`, with the same native and
migration trees. It publishes the earlier Plan 5 provider/child/census fixes
and moves the flatfile provider into its shared source list. This report retains
its exact tested source and does not silently qualify that later test-recipe
composition. The shared-list/restore-helper composition is a separate follow-up.
Older 0055/0056 evidence is not used to qualify this candidate.

This slice independently owns this report and the appended remote follow-up
entry. The sole outstanding shared request is the exact five-pin metadata
repair in the linked handoff. No new accounting interface, schema, migration,
activation caller, producer or audit mutation is introduced.

## Actual results

| Check | Result | Elapsed seconds |
| --- | --- | --- |
| mariadb native recipe | PASS, exit 0, zero skips | 69.754549 |
| mariadb independent recipe | PASS, exit 0, zero skips | 9.836367 |
| mysql native recipe | PASS, exit 0, zero skips | 70.971588 |
| mysql independent recipe | PASS, exit 0, zero skips | 10.944250 |
| Fresh maintained sql production build | PASS, 738 units/objects, zero warnings/errors/reused objects | 428.015981 |
| Fresh maintained flatfile production build | PASS, 738 units/objects, zero warnings/errors/reused objects | 417.624706 |

Each native SQL invocation reports 75 query faults, 105 distributed allocation
faults and 151 normalization allocation faults. The original equipment framing,
v1 compatibility, observed-slot and custody controls pass. The actual SQL path
observes slot 7, restores 0, and includes equipment in the concurrent writer
control. Repeatable-read visibility, DDL locks, unchanged session policy,
capacity limits, disconnect/lost-ACK behavior and refusal without partial output
remain covered. Original client-free capture refusal and malformed normalization
pass independently in both engine invocations.

The independent exporter preserves SELECT-only access and its partial-capture
status. Original source corruption, mismatched-kind, orphan/duplicate evidence,
late lower-ID commit, interrupted cut, checked-copper and uint64 controls pass.
Bounded operation/account/UID views, prior-epoch provenance and global refusal
remain observable without altering source tables. Quarantine interpretation and
the previously repaired equipment audit behavior remain in the original reader.
Neither recipe is an authentic source-complete opening or gameplay journey.

The four recipe terminal records are in `capture/results.json`. Native and
independent logs are retained separately for each engine. Original native
binaries use the runner's temporary directory and are deleted by its unchanged
cleanup; no retained native binary hash is claimed for those four executions.
The original compiler recipe, all consumed source bytes and terminal logs are
bound by the frozen source manifest and seal. Maintained server binaries, all
objects/dependencies and compiler logs are retained in full.

The original 58-method activation/coverage invocation retains one provenance
failure and 57 passes, with zero skips. All 153 raw pins are independently
compared; five mismatches exactly equal LF-to-CRLF conversion. The normal
validator passes 14 golden fixtures / 920 routes. Matrix `--check` passes 2,876
occurrences / 2,818 unique sites, zero unmapped, while coverage and release remain
incomplete. These Windows Python 3.12.10 results are in `contracts/results.json`.

Local WSL clang-format 14 passes the unchanged repository `format.sh --check
--file` command for the four equipment production files, the lifecycle header
and equipment test. A supplemental whole-file invocation also included the
lifecycle implementation and original lifecycle harness; it flags older lambda
formatting outside their imported changes. Primary records changed-line
clang-format 18 evidence. The local supplemental failure is retained as a
version/scope limitation, with no independent formatting edit or claim that the
complete tree passes clang-format 14. Whitespace checks pass.

## Exact commands and environment

The two original recipes execute unchanged once per private engine:

```text
/usr/bin/python3 -u -B /workspace/tests/async/run_economic_sql_source_snapshot_mysql.py
/usr/bin/python3 -u -B /workspace/tests/async/run_economic_sql_audit_snapshot_mysql.py
```

Original native recipe: C++20, `-Wall -Wextra -Wpedantic -Werror -O1 -g`,
ASan/UBSan, `-fno-omit-frame-pointer -fno-pie -no-pie -Isrc`,
`mysql_config --cflags/--libs` in SQL mode, and linker wraps for
`mysql_real_query`, `mysql_errno`, `_Znwm` and `_Znam`. Client-free mode retains
`-D__NO_MYSQL__ -Isrc/no_mysql`. Both retain `-lcrypto -lz -pthread`,
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1`, and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. The original native execution
deadline is 600 seconds; each unmodified outer recipe process has a 1,200-second
observer deadline. No flag, assertion, case or original deadline is removed.

Its exact 12 source inputs are:

```text
tests/async/economic_sql_source_snapshot_test.cpp
src/persistence/economic_sql_source_snapshot.c
src/economy/economic_sql_source_normalize.c
src/economy/economic_accounting_types.c
src/economy/shop_trade_recovery_manifest.c
src/item/item_transfer_command.c
src/world/quest_mobile_native_reference.c
src/economy/economic_source_event.c
src/item/craft_pouch_mutation.c
src/combat/chaos_pouch_ledger.c
src/persistence/critical_command.c
src/player/player_snapshot_codec.c
```

The source runner creates its own random guarded schema on a separately
initialized private engine, then applies the unchanged bootstrap, immutable
0038 equipment migration and immutable 0043 item-condition migration. The
independent exporter runner uses its original minimal disposable DDL and
SELECT-only reader. This is not a new full canonical61 fresh/upgrade run;
canonical61 is the exact archived migration source identity. Original older
fresh/upgrade and restore evidence retains its recorded source scope.

Private engines are MariaDB 10.11.14 and MySQL 8.0.46; Linux Python 3.12.3,
GCC 13.3 and Make 4.3 come from pinned tools image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
The observer reuses `persistence_restore.private_database`, changing only its
owned daemon to bind an unused container-loopback port. Its original temporary
data directory, private authentication, finite start/stop bounds, disabled
native AIO, local-infile and binlog restrictions remain. Separate generated
fixture credentials never enter the checkout or report. Every guarded schema
is verified gone, and both owned daemons stop before the helper completes.

The Docker qualification container has no host network, 2 CPU / 4 GiB limits,
2 GiB RAM-backed `/workspace` and `/tmp`, and only the protected evidence output
and read-only loader mounted. No live checkout, `.env`, private account/player
data or production service is mounted or accessed. Original regression build
cache is off. Public world inputs are copied only from the exact Git archive.

Both maintained commands use fresh protected object directories:

```text
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=mariadb BIN_ROOT=/evidence/builds/sql/bin OBJDIR=/evidence/builds/sql/objects DMS_BINARY=/evidence/builds/sql/server
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile BIN_ROOT=/evidence/builds/flatfile/bin OBJDIR=/evidence/builds/flatfile/objects DMS_BINARY=/evidence/builds/flatfile/server
```

Original warning, hardening, feature, compiler and linker policies remain. The
production profile omits `TEST_MUD`, retains `__NO_TESTS__`, and has no warning
exceptions. Each build compiles 738 distinct units into 738 fresh objects and
links a maintained server. SQL server SHA256:
`9fe1e158d286076438ce9a8b1d85528e2f60215c3e57e46a22f0fd10c73691d0`. Flatfile server SHA256:
`52eb56adfb380d577d013d5b66fd401fb423c869bca612c657b023b5c50e0d96`. Neither server is activated or booted
against production; build success is not a gameplay/recovery result.

## Evidence, curator packet and remaining gates

Protected output:
`D:\CodexEvidence\accounting-plan5\bin\equipment-v2-current-01-20261006`.
Frozen archive SHA256: `70db28e84dc903f4c361ef64685677c93df799aba7867014ee71950ee6e0fd94`.
Full seal:
`D:\CodexEvidence\accounting-plan5\bin\equipment-v2-final-seal-01-20261006\evidence.json`.
Seal SHA256: `b638f8512946eac0c975b4dbaa71aa3c667b950a567c340d7dd7f222d2a887f2`.

The seal verifies all 3,122 tracked code/migration/test inputs,
2,991 retained artifacts and 0 links without following links.
It checks source hashes before/after, both complete builds, four native/exporter
terminal records, the retained shared contract failure and all old-tip ancestry.
The qualification container exits 0 without OOM. Artifact bytes are archives,
debug objects, binaries and test logs; they are not a release storage-growth or
mixed-workload measurement. Command manifests and executed preparation/observer
helpers remain beside their results. Logs, archives and generated outputs are
not committed.

This report, the exact five-pin handoff and the sealed source/evidence receipt
are the Plan 5 packet for primary's locally maintained shared notebook/curator
workflow. The notebook is nonblocking as directed by the user; there is no
independent shared notebook rewrite or wait for external synchronization.

Remaining requirements are explicit: the primary-owned full stopped-maintenance
opening must capture all required holdings and actual physical/item forests,
including escrow and pending claims, and retain authentic current mappings and
source attribution. The Plan 5 independent activation verifier still must bind
the borrowed request/session, installation/baseline identity, current complete
capture, reviewed route manifest, independent audit and tested candidate. The
current `sql_partial` / `complete=False` exporter cannot supply positive authority.

Original actual supported-writer gameplay, linked both-engine lifecycle/replay,
lost-reply and cold publication/ACK journeys, authenticated flat parity,
typed active erasure and complete current-source backup/restore/retention
qualification remain. The published alchemist driver still pins its older
original provider/source manifest; its historical inactive fixture evidence is
not relabeled as a run on this new candidate. Release-host mixed-workload
latency, operation size, storage growth, checkpoint and reconciliation budgets
remain unmeasured here. Component inventory, synthetic corruption fixtures and
isolated native passes do not close those requirements.

No accounting activation, production mutation, deployment, PR merge, audit
autocorrection or push to `experimental-accounting` occurs. Inactive behavior,
wallet-root item exclusions and the declined inactive spell change remain.
There is no blocker to continued independent Plan 5 work; the five shared pin
repairs and complete source-opening interfaces remain primary handoffs, and
full release remains unqualified.
