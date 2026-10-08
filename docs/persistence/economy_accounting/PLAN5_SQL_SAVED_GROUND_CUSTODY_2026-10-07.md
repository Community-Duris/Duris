# Plan 5 raw SQL saved-ground custody evidence — 2026-10-07

This slice repairs an omitted raw saved-ground projection. The predecessor
silently ignores malformed saved rows; six authenticated controls produce zero
findings before this fix. Full Plan 5, R1–R8, world/room authority and release
remain incomplete. Primary-local notebook maintenance is nonblocking. This is
a curator-ready evidence handoff; acknowledgement, application and primary
adoption are not claimed.

## Source and delivery

- Local/remote branch `codex/accounting-plan5`; worktree
  `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Branch base `b72e6724bc50d2dd4a6b589357167919a9072f65`; code `a09d5a800ea0673dbcb94ca9a766e2a31ca6241d`.
  Documentation result and verified clean remote equality are recorded after
  publication in external `delivery/result.json`.
- Tested primary `af425e28e27ab5d2c6a3726aa4b814c150b01395`; composed tree `9793f5c8bf5ffc74c63a28dd65bb1b79a61c5f95`.
- Source archive SHA-256 `d0fcd64135cecc08c8a28c1430ba7cfe2220ce3e5441147db99bb5755d8e80f7`.
- Native `833d3085815b396861ad18a77635412212381e4b`; migrations `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`.
  Both canonical fresh engines end at `64 / 0064_auction_custody_history`.
- Four owned Python files overlay the refreshed primary; no production C/C++,
  shared coordinator/contracts/producers/schema/registry/matrix/activation/native
  recipe changes. This packet and additive remote follow-up are the only owned
  documentation changes. All seven earlier consolidated tips remain ancestors.

| Owned file | Tested Git blob |
|---|---|
| `scripts/economic_sql_audit_snapshot.py` | `aae030e4238efd85202fd4b3e56fa2ab62b5c063` |
| `scripts/reconcile_economy_accounting.py` | `1c945b0b234054c345cc3fd73b7215db7b22acc7` |
| `tests/async/run_economic_sql_audit_snapshot_mysql.py` | `05fe2b603e77e9d1fb5953a4692b6fe8bf0f2f06` |
| `tests/async/test_sql_saved_ground_custody_audit.py` | `1968e375239381ad142f66832932464d982ba8c9` |

Six unchanged custody fixture dependencies are frozen alongside those overlays;
their exact blobs, every source body/mode and native link target are in
`source.json`. Shared/native source was selected from the refreshed primary,
including its actual saved-ground coin-history fence, rather than the older
native files retained on this independent branch.

## Defect, repair and authority limits

The exporter now counts saved rows, handoff receipts and immutable room payload
rows before any detail read; the aggregate limit is100,000. Every raw saved row
and receipt survives export. `MIN(id) OVER (PARTITION BY item_key)` preserves the
native SQL collation's case, accent and trailing-space equality without exporting
key text or a reversible key hash. The group anchor is per-cut physical metadata,
never a durable owner identity. Numeric source/destination group anchors use the
same SQL key equality. Raw NULLs, zero/legacy UIDs and parent IDs remain visible.
The main capture requires37 InnoDB source tables, including all three additions,
inside its existing READ ONLY REPEATABLE READ cut.

The independent reconciler checks numeric widths/exact representations, coverage,
one NULL root per key group, actual minimum anchors, foreign-key/room ancestry,
cycles/orphans, duplicate rows/ordinary eligible UIDs, missing or inactive current
custody, room owner `[3,room_vnum,0]`, UID-root/parent correspondence, vnum,
equipment, unsupported quantities, unknown prototype literals and raw coin values.
Its ancestry cache reuses the existing resolver on audit copies and never mutates
input. Wallet-root ITEM_MONEY player rows remain excluded; nested player money
and unclaimed auction custody compete, while claimed auction history does not.
No reverse room-absence claim is made before the remaining source families qualify.

The actual loader calls the child function at depth0, checks `depth>64` before
its SELECT, and recursively asks for children even at an empty leaf. This admits
65 path rows and withholds66; the auditor uses65, not66 or a guessed32. The
current exact schema also withholds saved groups above3000 rows. The selected
loader region and full source SHA are retained in `native-saved-loader-source.json`.
This is source-derived policy; a full native saved-item hydrator was not executed.

Handoffs and modern rows are retained history until stronger proof exists.
Every nonempty source yields `saved_ground_full_runtime_authority_unqualified`.
Any receipt (including old seasons, NULL legacy payload digests and absent retired
sources), associated raw group, immutable item payload, admitted player-drop
history, native coin-payload bytes or typed coin history additionally yields
`saved_ground_history_authority_unqualified`. UINT64_MAX is withheld because the
native coin-history reader refuses that sentinel. These markers suppress current
grant/duplicate assertions for associated groups; structural/literal checks still
run. They do not authenticate a receipt's full payload, original season, source
retirement, destination publication or current runtime authority. Missing source
keys cannot become an inferred acknowledgement or a valid duplicate. Full saved
text/affects/extra-description digests and modern capsule recovery remain gates.

Read-only SQL predicates independently reproduce the two native presence fences.
The runtime reader imports no mutation code. Native production code is used only
as a test oracle: actual `sql_room_item_payload_present`, actual
`sql_room_coin_payload_present`, public current-custody/equipment capture and the
real player snapshot coin codec. There are no fake providers, rewritten native
function bodies, replacement linker policies or schema relaxations on canonical
fixtures. The public capture has no saved_items projection, so selected physical
literals and handoff rows have canonical SQL tests but no public native full-row
capture parity claim. Recovery/hydration/gameplay authority remains unqualified.

## Exact validation

Pinned image `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`;
GCC13.3 SHA-256 `1353e9bdd29a7295c7226bf6c63abccce056d8cac31f112e5cdbecc3f28c2769`;
Python3.12.3 SHA-256 `e50d468e8b0adfb05733f5b87b3cff34829c4a8c1aea50c865aa8bdfe4bb150f`.
`toolchain.json`, `docker-command.json`, every original `commands.json` entry and
the native compiler invocation retain exact flags, env selection and source lists.
Native compilation uses C++20, all original warnings with Werror, ASan/UBSan,
no-PIE, actual mysql_config flags, section GC and libcrypto. No production native
source changed, so no additional complete server Make or boot is claimed.

Commands from the worktree, with Windows TEMP/TMP explicitly on D:

```text
python D:/Dev/Temp/accounting-plan5-sql-saved/toolchain.py
python D:/Dev/Temp/accounting-plan5-sql-saved/freeze.py green05
python D:/Dev/Temp/accounting-plan5-sql-saved/run.py green05
python D:/Dev/Temp/accounting-plan5-sql-saved/precommit.py
```

The frozen observer runs unittest modules (equivalent to `python3 -B -m unittest
MODULE -v`) in this exact order with existing SQL opt-ins plus
`DURIS_RUN_SQL_SAVED_AUDIT=1`: saved-ground, siege, locker, corpse, player, shop,
the complete reconciler suite and the complete SQL-origins suite. It then runs
the original full `tests/async/run_economic_sql_audit_snapshot_mysql.py` against
both private engines, preserving the600-second per-run and900-second whole-batch
limits. Source is in RAM on Linux; private DBs have native0700 paths; output and
evidence bind directly from separate D: directories. Network is disabled and the
container root is read-only; no existing database/container/volume is touched.

Final `green05`: exit0, 486.833688090s /900, 2419 recorded subprocess commands.

| Module | Loaded | Executed | Skips | Result |
|---|---:|---:|---:|---|
| `test_sql_saved_ground_custody_audit` | 8 | 8 | 0 | PASS |
| `test_sql_siege_custody_audit` | 8 | 8 | 0 | PASS |
| `test_sql_locker_custody_audit` | 8 | 8 | 0 | PASS |
| `test_sql_corpse_custody_audit` | 8 | 8 | 0 | PASS |
| `test_sql_player_custody_audit` | 8 | 8 | 0 | PASS |
| `test_sql_shop_custody_audit` | 7 | 7 | 0 | PASS |
| `test_reconcile_economy_accounting` | 134 | 131 | 3 | PASS |
| `test_economic_sql_audit_origins` | 58 | 55 | 3 | PASS |

The new module has7 pure and1 native methods, all mandatory in this batch.
Across both engines:82 canonical raw-cut observations (41 each) plus30 explicitly
synthetic presence-marker observations (15 each),112 total. Canonical cuts cover
actual collation aliases, foreign/cyclic/missing/zero-parent links, NULL room/UID,
wrong current owner/context/topology/vnum/equipment, quantities/literals, native
coin bytes, source depth65/66, handoff source/destination retention and retirement,
modern immutable presence, competing player and auction paths, wallet exclusions
and empty cuts. Full database inventories remain identical across every audit;
SELECT-only users refuse an UPDATE. Output limits0/1/100 preserve counts/input.

The marker matrix reuses the original diagnostic model schema in a separately
named private database. Core metadata-only epoch/lineage tables copy canonical
columns; empty native metadata tables support the unchanged presence oracle.
It tests item payload presence, non-NULL/empty coin bytes, typed item references,
matching pile account-key suffix versus wallet suffix, coin child-domain ledger
links and admitted drops, with wrong writer/UID/domain/failure-stage/room-reason
negative controls. These synthetic rows are explicitly not native canonical
capsules, hydration, installed recovery or release proof. Canonical constraints
remain unchanged; no FK or CHECK weakening is used for this matrix.

Six SHA-authenticated predecessor false clears (unadmitted row, wrong owner,
missing parent, unknown literal, legacy UID and UID-less negative money) replay
before/after, preserve input and remain counted at0/1/100 limits. `red-controls.json`
retains both reports and exact input hashes. The100,000-row reversed ancestry
check retains its measured time in the container log, against the original30s
budget. The original whole SQL runner now also proves saved rows are excluded
from the old RR cut and visible in the later cut (raw UID87), with missing/MyISAM
rejections for15 physical/history source tables,30 negative controls per engine.

Six existing opt-in native tests are skipped inside the two complete pure suites;
their exact case IDs/reasons are in `terminal.json`. They are not reported as
executed by this slice. The original full SQL runner still passes independently
on both engines. Native saved hydration, full handoff verification, nativeEAB2
install/recapture, gameplay, production and full release were not run.

| Original full SQL runner | Exit | Log SHA-256 |
|---|---:|---|
| `whole-snapshot-mariadb` | 0 | `ebd3a1b1e0571a40502ce335f14846c017568503c4f264e0dcd9a64d39b8d250` |
| `whole-snapshot-mysql` | 0 | `e66597c94756c7750b3191891527f2bc784903df0ca500b58192b90213e49a64` |

Native oracle SHA-256 `ec6cf9816b36a8649269cbb78e8d7714c6352dbdf13dd3c6e1159001ceee8f3f`;
observations SHA-256 `57345c448bcdd919c6d1ff8e46ef87952c3310ffba7dd3089eefa2a6436c8606`.
Evidence `D:/Dev/Tests/Duris/accounting-plan5/sql-saved-20261007/`;
build/cut artifacts `D:/Dev/Builds/Duris/accounting-plan5-sql-saved-20261007/green05/bin/tests/sql-saved-ground-custody/`;
helpers `D:/Dev/Temp/accounting-plan5-sql-saved/` (sealed copies under helpers/).
All frozen attempts retain source, commands/logs, stopped private DBs and native
POSIX body/kind/mode inventories. Windows copied modes never substitute for native
mode evidence; no copied link is followed by Windows. `seal/evidence.json` binds
raw evidence/builds/publication and `delivery/result.json` binds the clean push.

Earlier attempts are preserved rather than overwritten:

- `green`: tree `5e3721348ce4d8b62c917880f1dceaa46e54a5dc`, archive `231cf326eb1c2542d64b9067cba6c4bf3546d6b8e84940a679e91613e0ca5114`, exit1, 55.769264092s, 302 commands.
- `green02`: tree `6b06e54ca21fb1da63a416f070000f07856eb0d8`, archive `ac5e55d4c396fe3b49da884e62c0340488cb8c943c5e6c814d009f471821a0f6`, exit1, 51.057607189s, 107 commands.
- `green03`: tree `806d4a3a96b3886792a3d49f0d72175c7ac788c1`, archive `03b22c305f4c8e6506d5ae2299f0485a32561c6a9390a7e57c80dd63ab54ddb7`, exit1, 63.086822221s, 319 commands.
- `green04`: tree `3aa4032be434b83e40f25649a01c7c2fcb062cd4`, archive `c34b014e1dc761dc0ca56d6822b69073c1fb4065d30aba36a7485c191cbfd314`, exit1, 92.403084568s, 661 commands.
- `green05`: tree `9793f5c8bf5ffc74c63a28dd65bb1b79a61c5f95`, archive `d0fcd64135cecc08c8a28c1430ba7cfe2220ce3e5441147db99bb5755d8e80f7`, exit0, 486.833688090s, 2419 commands.

`green` reached canonical MariaDB handoff checks, then rejected the fixture's
nonexistent inbox command_payload column; its insert now uses canonical
payload_version/result_payload. `green02` exposed SQL three-valued logic on a
NULL saved UID; COALESCE preserves a numeric false marker and the raw unknown UID.
`green03` passed all40 canonical MariaDB cuts, then the production presence reader
correctly refused a diagnostic model schema missing core epoch/lineage tables.
Those metadata-only tables now copy canonical definitions. `green04` passed
all80 canonical cuts and30 synthetic markers, then failed a stale aggregate
count assertion (110 observations versus the transcribed104). The final suite
expects112, including two added native sentinel-refusal cuts; per-cut assertions
and original coverage were not removed. The same real reader,
warnings/sanitizers, original policies and budgets remain; no error is waived.
Source review corrected the initial depth66 assumption to65 because empty-tail
recursion runs unconditionally, and added the primary's actual coin fence before
the final run. A transcribed coin-domain constant was corrected before its matrix
run. Pre-freeze local stdout is retained in tool history; no nonexistent raw log
file is claimed. Windows pure-reader run:8 loaded/7 executed/1 native skip;
original reconciler134 loaded/131 executed/3 skips and origins58/55/3 PASS.

## Primary handoff, curator packet and open gates

No shared contract or schema change is needed to integrate this reader fix.
Primary-owned qualification registration should add
`tests/async/test_sql_saved_ground_custody_audit.py`:7 pure methods in
`SavedGroundCustodyAuditTests` and1 native
`NativeSavedGroundCustodyAuditTests.test_both_canonical_engines_current_capture_modern_presence_and_coin_codec`.
Pure: `python3 -B tests/async/test_sql_saved_ground_custody_audit.py SavedGroundCustodyAuditTests`;
full: `DURIS_RUN_SQL_SAVED_AUDIT=1 python3 -B tests/async/test_sql_saved_ground_custody_audit.py`.
Keep existing genuine providers, source registrations, flags, schema and budgets.

For full primary-owned public capture parity, add raw saved item identity/metadata:
`id,item_key grouping,room_vnum,container_id,obj_uid,vnum,quantity,weight,extra_flags,
item_type,value0..value3`; the cut must preserve SQL collation grouping without
exporting identifying key text. Add unfiltered handoff
`season_epoch,source_root_id,source_uid,source_room_vnum,source_row_count,
destination_root_id,retired_at presence,source_id_digest,source_payload_digest,
destination_payload_digest` and independently authenticated key/graph binding.
Full original saved rows, affects and extra-description bytes are required for
the native payload digest, including SQL IDs, NULL markers, field order and exact
lengths. Never treat selected literals or synthetic digest strings as that proof.
Modern item/coin full capsules and original current custody must retain typed
root/reference/effect/child/history provenance and sentinel refusal. Consumers:
independent physical/coin reconciliation and baseline/backup/restore proof.
Existing public framing is versioned; the primary must choose an explicit
compatible capture contract, not silently alter selected wire fields. Tests:
both engines, native full-row parity, collation/erased aliases, nullable legacy
digests, missing/retired/conflicting destinations, stale seasons, real native
hydration/restart and original capsule/history tampering/retention. This is a
narrow future interface request, not an implemented shared schema/contract.

`primary-documents/` preserves the exact tested plan, remaining requirements,
latest review checkpoint and Plan5 interface handoff. The private97 abort/build
candidate includes23 unchanged original fixtures/five private schema files;
source review/formatting only, compiler/native/SQL/migration/gameplay/fault/recovery
unrun. It is not imported/installed or qualified by this reader slice. The real
auction provider/source registration and retained abort proof remain primary
owned. No epoch is selected. Publication refresh metadata authenticates exact
docs-only changes and unchanged native/migration trees.

The curator delta is the defect, four owned blobs, tested source/backends/commands,
raw seals and these explicit open gates. The notebook is locally maintained by
the primary and nonblocking; this packet does not claim curator acknowledgement,
application or primary integration.

Remaining gates: full saved/handoff/modern room native payload and runtime
authority; collector and complete cross-provider/reverse world census; full
prototype/artifact/UID/origin/writer authority; nativeEAB2 install/recapture;
authenticated retention/erasure/restore continuity; both-engine populated upgrade
and rerun; original producer/player/gameplay/fault/restart/load qualification;
primary publication of the tested combined candidate and full R1–R8/Plan closure.
Inventory coverage, synthetic markers and isolated passing tests do not close
release. Accounting remains inactive, wallet-root exclusions and declined inactive
spell behavior remain. No activation, merge, deployment, production mutation or
auto-correction. No independent-work impasse; the goal stays active.

Publication refresh `9360e120f0f966b431b56da6083f3206285690f1` changes only the six
recorded documentation files; native and migration trees remain exact. Its
private99 runtime-return/real-root candidate is source reviewed/format checked
and unrun, not imported/installed or qualified. Actual O/P dispatch is unchanged;
full root chronology/room placement, caller wiring, original terminal/current
authority and major-plan/R1–R8 gates remain. Raw publication documents, blobs,
hashes and diff are preserved separately from the tested private97 checkpoint in
`primary-publication-documents/` and `primary-publication-refresh.*`.

Final reversed100,000-row/65-depth check:1.4933574459973897s /30s.
