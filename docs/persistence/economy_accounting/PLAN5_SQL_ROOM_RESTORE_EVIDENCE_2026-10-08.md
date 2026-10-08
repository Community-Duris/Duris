# Read-only modern room restore evidence - 2026-10-08

This slice closes established structural room-restore omissions. Full Plan 5,
original-payload/runtime authentication and R1-R8 release qualification remain
open. Accounting remains inactive.

## Branch, source and ownership

- Local/remote branch: `codex/accounting-plan5`.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Owned base: `4ccc4defafb714188ded308ad792bed05534e69a`.
- Solved-issue code commit: `7a7165a964928fbc857a9a8d08940a7a4ca47e2f`.
- Documentation/publication result and verified remote: exact SHA in
  `D:/Dev/Tests/Duris/accounting-plan5/room-restore-20261008/delivery/result.json`.
- Refreshed primary: `df0570c5456d4d747ca1320ce958c1db52bb08fd`.
- Actual tested composition: `51f243ee0a09bedc0fbb2c8ee44c4dff4fbbec7d`.
- Frozen archive: `candidatechecks03.tar`, SHA256 `e0b1ade5fb1bbf94b1dcc90dc166831a1024502947c0f732610a0d955edb9473`.
- Native tree: `833d3085815b396861ad18a77635412212381e4b`.
- Canonical64 migrations tree: `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`.

`sourcechecks03.json` binds all source bodies/modes/link targets and 18 explicit
owned overlays. All overlays, including earlier delivered reader/fixture closure,
match the eventual published branch blobs. The owned branch retains older shared
source; this is an explicit primary-plus-owned composition, not a claim that the
whole branch equals primary. Earlier 0055-only results do not qualify it. The
primary owns integration and publication of the tested combined candidate.
All seven earlier branch tips remain ancestors; every earlier follow-up stays
on this same remote branch. No work was moved to another branch in this slice.

Exactly four owned code/test files change:

| File | Purpose |
| --- | --- |
| `scripts/economic_room_restore_evidence.py` | SELECT-only CLI adapter and independent structural refusals |
| `scripts/economic_sql_audit_snapshot.py` | Reuse original room queries for the real second reader |
| `scripts/qualify_database_restore.py` | Add room checks after the original canonical-root verifier |
| `tests/async/test_sql_room_item_restore.py` | Pure protocol controls, native codecs and both disposable engines |

This report and the additive remote follow-up are the only documentation edits.
No shared coordinator/contracts/producers/registry/matrix/schema/activation or
native production source changes. No original native fixture/recipe is edited.

## Established defect and complete scoped repair

Frozen predecessor `4ccc4defafb714188ded308ad792bed05534e69a` accepts all six
actual counterexamples: missing original accounting root, truncated native room
payload and trailing native payload byte on both canonical engines. `probe03`
retains the exact predecessor, native capture-helper bytes, raw commands, complete
schema inventories and terminal result. Whole database contents remain unchanged;
checks use a SELECT-only account. This establishes a restore diagnostic omission,
not genuine producer/publication or release evidence.

The new adapter reuses the existing exporter queries and pure independent item
codec/graph diagnosis. It does not duplicate mutation logic or implement a second
room parser. All nine source tables must be InnoDB; all 48 selected/join numeric
columns must have integer storage types before JSON normalization. NULLs, raw
binary bytes and every malformed/duplicate binding are preserved. Pages use the
same supplied persistent executor: four payload rows or 256 other rows, with
strict JSON/hex/shape, total wire/packet bounds and the existing preflight before
any LOB capture. Limits remain 100,000 room rows, 16MiB aggregate payload,
131,072 bytes per payload and 32MiB packet/wire. Native graph/codec budgets remain.

The helper borrows a quiescent restore or the caller's RR read-only cut. Pagination
is enumeration; it is not a live commit watermark. It never begins a transaction,
commits, repairs, activates or writes. The existing original canonical intent/plan,
receipt, source, count and projection verifier runs first and retains its exact
refusals. Structurally invalid room payloads/bindings/current graphs then refuse.

For nonempty structurally clean room history, CLI JSON adds
`room_item_diagnostics`, containing the positive counts
`room_item_full_runtime_authority_unqualified` and
`room_item_retained_root_authority_unqualified`. These are limitations, not
certification. Empty-room output remains byte-for-byte
`{"history":"ok","reconciliation":"ok"}` plus newline. Full original room
payload authentication and runtime installation/ACK remain separate gates.

The native fixture uses the unchanged capture helper and native EAI1/EAP1 codecs.
The positive accounting root and receipt rows are deliberately modeled test
history. They are valid codec inputs, not a real producer/coordinator/commit/ACK
journey. Native `sql_room_item_payload_verify_retained` explicitly leaves root
verification to its caller; this slice preserves the existing root verifier and
does not claim the complete retained runtime authority.

## Exact commands, backends and results

Pinned network-none image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Each run uses two CPUs, 5GiB memory, fresh source/tmp RAM directories and direct
D: source-evidence/build mounts. Build caches are off. Actual compiler/Python/nm/
mysql_config executable hashes and versions are in
`seal/native-build-inventory.json`. Raw engine version/init commands, canonical
migration commands and stopped datadir inventories are retained per stage.
Actual engines: MariaDB `10.11.14-MariaDB-0ubuntu0.24.04.1` and MySQL
`8.0.46-0ubuntu0.22.04.4`, from retained server logs/version outputs.
Only local private sockets/`duris_restore` schemas are used. No production access.

Final focused invocation (explicit owned worktree):

```powershell
python -B D:/Dev/Temp/accounting-plan5-room-restore/launch.py checks03 sourcechecks03.json
```

Inside that exact archived source, `python3 -u -B /evidence/checks03/checks.py
checks03 sourcechecks03.json` loads these unchanged/native opt-ins:
`DURIS_RUN_ROOM_RESTORE_SQL=1`, `DURIS_RUN_SQL_ROOM_AUDIT=1`,
`DURIS_ROOM_RESTORE_PREDECESSOR=/evidence/predecessorprobe-qualifier.py`,
`DURIS_REGRESSION_BUILD_CACHE=off`, ASan leak/halt and UBSan halt/stacktrace.
The predeclared complete focused batch budget is 1800 seconds. Original whole
snapshot runners keep their 600-second budgets and all original assertions.

| Suite | Methods | Skips | Result |
| --- | --- | --- | --- |
| test_sql_room_item_restore | 6 | 0 | PASS |
| test_sql_room_item_custody_audit | 7 | 0 | PASS |
| test_persistence_backup | 40 | 0 | PASS |
| test_persistence_restore_socket | 7 | 0 | PASS |
| whole-snapshot-mariadb | manual runner | 0 | PASS |
| whole-snapshot-mysql | manual runner | 0 | PASS |

Focused exit0: 753.630s, 317 recorded subprocess commands.
All original source bodies/modes/links remain unchanged. All monitored service,
version and initialization processes are terminal. Both actual engines apply all
64 canonical migrations; whole synthetic exporters retain their original controls
and 38 missing/MyISAM physical-source refusals each.

The new SQL test records 36 observations: 32 complete read-only cuts plus two
aggregate-LOB preflight refusals and two original canonical-root corruption
refusals. Twelve representative ordinary cuts call full `qualifier.main`; two
additional full calls preserve original canonical-plan refusal. All structural
cuts compare CLI, DBAPI and `exporter.read_native` full SQL snapshots exactly. Fourteen
predecessor calls still accept the modeled counterexamples/positive controls.
Every whole DB inventory remains unchanged. Late writer changes stay hidden in
the borrowed RR cut and appear in the next cut; 129 maximum-size payloads refuse
after three queries, before payload reading. SELECT-only UPDATE is denied.
Historical moved/prior-season rows are not invented current missing payloads.
All-sidecar-loss diagnosis uses the original successful typed drop fallback.

Original canonical regression on the *same* frozen source:

```powershell
python -B D:/Dev/Temp/accounting-plan5-room-restore/launchcanonical.py canonical sourcechecks03.json
```

It loads original `test_restore_economic_coin_effects` with
`DURIS_RUN_RESTORE_COIN_INTEGRATION=1`, `DURIS_PLAN5_CANONICAL_EVIDENCE=1`,
cache off and ASan/UBSan. Original inner 1800-second engine budgets/assertions
stay; outer complete two-engine budget is predeclared 5400 seconds.
Exit0: 645.516s, 71 subprocess commands,
one complete method, zero skips. Native SQL/flatfile strict C++20/Werror sanitizer
builds agree on 37 blocks/32 coin cases, retained histories/claim pairs and all
3026 decoder cases (1054 accepted). Original maintained fixture body is unchanged.
Two already-published owned test-builder files are retained as overlays because
they close the three real provider dependencies previously handed off. No shared
recipe or original flags/assertions/timeouts are changed here.

```text
mariadb PENDING_CLAIM_RESTORE_CUTS {"allocation_pagination_rows": {"consumptions": 257, "sources": 258}, "authority_unchanged": true, "controls": 7, "cuts": 25, "native_fixture_sha256": "2f0c5dc917aef019c60befbd592c9d0e0a1bf6b0c6bbef828996b9ad077bd635", "native_roots": 5, "original_readers": 2, "producer_journey_qualified": false, "schema_head": "0064_auction_custody_history", "snapshot_allocation_reader": true}
mariadb CANONICAL_RESTORE_QUALIFIED {"authority_unchanged": true, "cuts": 109, "full_entry_cuts": 58, "intent_bound": 8192, "page_roots": 259, "plan_bound": 4194304, "production_access": false, "refusals": 90, "schema_head": "0064_auction_custody_history"}
mariadb COIN_RESTORE_QUALIFIED {"audited_cases": 30, "authority_unchanged": true, "canonical_constraint_refusals": 2, "native_cases": 32, "production_access": false, "schema_head": "0064_auction_custody_history"}
mysql PENDING_CLAIM_RESTORE_CUTS {"allocation_pagination_rows": {"consumptions": 257, "sources": 258}, "authority_unchanged": true, "controls": 7, "cuts": 25, "native_fixture_sha256": "2f0c5dc917aef019c60befbd592c9d0e0a1bf6b0c6bbef828996b9ad077bd635", "native_roots": 5, "original_readers": 2, "producer_journey_qualified": false, "schema_head": "0064_auction_custody_history", "snapshot_allocation_reader": true}
mysql CANONICAL_RESTORE_QUALIFIED {"authority_unchanged": true, "cuts": 109, "full_entry_cuts": 58, "intent_bound": 8192, "page_roots": 259, "plan_bound": 4194304, "production_access": false, "refusals": 90, "schema_head": "0064_auction_custody_history"}
mysql COIN_RESTORE_QUALIFIED {"audited_cases": 30, "authority_unchanged": true, "canonical_constraint_refusals": 2, "native_cases": 32, "production_access": false, "schema_head": "0064_auction_custody_history"}
```

Direct Windows final-source checks also pass: five pure new methods and six pure
room audit methods, with one explicit Linux/SQL opt-in skip per module.
`windows-pure/result.json` binds source hashes, Python/version and raw output.
No tracked C/C++ changed, so another full server Make run was not required by
this Python-only slice. Prior 754-unit native evidence is not relabeled as a
new build of the primary private candidate.

## Failed/intermediate attempts retained

| Stage | Exit | Seconds | Commands |
| --- | --- | --- | --- |
| probe | 1 | 24.497 | 7 |
| probe02 | 1 | 53.260 | 71 |
| probe03 | 0 | 205.172 | 148 |
| checks | 1 | 90.472 | 71 |
| checks02 | 0 | 736.190 | 321 |
| checks03 | 0 | 753.630 | 317 |
| canonical | 0 | 645.516 | 71 |

`probe` failed because the harness treated MysqlExecutor as a context manager
and assumed a runpy main helper namespace. Its original stopped MariaDB data was
later copied as an unextracted Docker tar with native entry modes/link metadata,
without restarting the container or following links. `probe02` failed on an
omitted explicit owner_revision column list. `probe03` is the passing unchanged
predecessor reproduction. `checks` compiled/executed the native fixture but its
DBAPI test adapter converted byte-valued JSON to a Python bytes repr. The adapter
now decodes strict UTF-8 and retains raw query responses before interpretation.
`checks02` passes with an intermediate test variable shadow that unnecessarily
executes full qualifiers on all cuts and stores the packet instead of a boolean
flag. `checks03` corrects that flag and is the final exact source qualification.
No failed or terminal stage is restarted or overwritten.

## Evidence, curator packet and narrow primary handoffs

Evidence root: `D:/Dev/Tests/Duris/accounting-plan5/room-restore-20261008/`.
Native outputs: `D:/Dev/Builds/Duris/accounting-plan5-room-restore-20261008/`.
Raw seal: `seal/evidence.json`, SHA256 `d60243eae3efd52fe14be1980c84424a2fa36c3c9b193dcd40ae225f1324542d`;
12109 files/5276833176 bytes, 1240 native
build files, no copied data links/reparse points. Native metadata is retained
separately before Windows copies. Original failed-probe tar preserves its own
link metadata without extraction. Every original stage/container is stopped;
source guards, complete command inputs/outputs and terminal JSON are retained.
Delivery rehashes every sealed file and verifies the exact remote, clean worktree,
18 overlay blobs and seven preserved ancestor tips. Delivery is deliberately
post-seal and does not rewrite sealed evidence.

The additive remote follow-up plus this report/seal/delivery is curator-ready for
the primary's locally maintained shared notebook. User explicitly made that
notebook nonblocking. Application, acknowledgement and adoption are unclaimed;
no cross-chat message is sent without user authorization.

Primary integration requests:

1. Integrate these four owned files and retain previous owned reader/test closure.
   The only additive CLI field is `room_item_diagnostics` with the two exact
   positive integer counters above. Existing `persistence_restore.database_qualify`
   discards this CLI output; its native receipt aggregate JSON is a different
   consumer at lines250/253. Existing empty output stays exact. Consumers that
   inspect nonempty output must preserve these limitations, not infer complete
   runtime/payload authentication. No DB/native schema or contract change requested.
2. Register the new focused test with `DURIS_RUN_ROOM_RESTORE_SQL=1` on a fresh
   canonical MariaDB and MySQL candidate. Include its existing room-audit opt-in;
   central registry/matrix edits remain primary-owned.
3. Continue the original shared room-seed recipe handoff in
   `PLAN5_ROOM_OBSERVER_BUILD_AND_SEED_HANDOFF_2026-10-08.md`: 41 unresolved exact
   definitions mapped to 21 real maintained providers. No repeated known bare
   shared compile failure or substitute seeded cold-boot claim here. The genuine
   original room/recovery journeys require that original seed and two cold boots.

Primary review captured at df0570c5456d4d747ca1320ce958c1db52bb08fd remains
the private 119-production/23-original-fixture/
five-schema candidate SHA256
`55f9bf80794eb274d69d49e7a265627c39f2643abe5e9d557741d0c74c44521b`.
It is reviewed source with compiler/native/gameplay/SQL/persistence/recovery
UNEXECUTED, not this composition. Shared keeper cash classification/current-image
and historical transition, genuine O/P admission/source-CAS/budget/adoption,
producer freeze/commit/publication/ACK remain owned and open. Complete physical/
value/UID/origin/writer census, collector/reverse authority, original-payload
and retained-root runtime authentication, full retention/erasure/backup continuity,
populated upgrades/reruns, all major-plan/player/fault/restart/load budgets and a
published tested combined candidate still gate full Plan5/R1-R8. No activation,
PR merge, deployment, production change or audit autocorrection. Wallet-root
ITEM_MONEY exclusions and declined inactive spell-path change are preserved.
