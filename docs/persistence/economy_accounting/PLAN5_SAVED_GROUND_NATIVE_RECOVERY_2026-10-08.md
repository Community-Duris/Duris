# Plan 5 saved-ground native recovery qualification — 2026-10-08

The full-world saved-item recovery journey passes all 18 current
invocations on MariaDB and all 18 on MySQL. Its default invocation contains four
fresh-schema cases, so this is 21 current recovery cases per engine. The real
production SQL server executes materialization, publication, handoff acknowledgement,
exact-source retirement, refusal, cold reboot and player inspection. The stale
concurrent-child assertion is repaired to require withholding an untrusted
acknowledged destination; production source remains unchanged. This closes a
test defect and a focused executable evidence gap left by
the raw saved-ground audit slice; it does not qualify Plan 5, R1–R8 or release.

## Branch, exact source and ownership

- Delivery branch and expected remote: `codex/accounting-plan5`.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Owned branch base: `7764b7b91ed8c4a1f56d8fd16eb6c15ae7fddcd6`. Test-fix commit: `3b51d3b2c3aa54ca610f5e329651085de214c935`.
  The recovery journey, this report and additive remote follow-up are the only
  repository changes. Result commit and post-push remote
  verification are in the delivery receipt and delivery message.
- Refreshed, frozen, tested primary: `9360e120f0f966b431b56da6083f3206285690f1`.
- Tested tree: `b9ab0c3a3ea2c6a45cf2cfb286c8eb7473e31f8c`. Only overlay is the owned test file, Git blob
  `01c73781bc5ed4bdb86a3a35d901a031fcf759b3`.
- Unchanged predecessor tree: `e0e98bbb27c5e35dfd59503c2ee598ba3fdbffbd`; source tar SHA-256
  `49fcf5c71e631f67bf7288b6cffc6ae8549d29adea83be028521a484296ad7ad`. Both original-engine RED results are retained.
- Native tree: `833d3085815b396861ad18a77635412212381e4b`.
- Migration tree: `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`; every observed acceptance cut
  has 64 immutable migration-history rows, through canonical 0064.
- Git source tar SHA-256: `cd129b6bed813b8573128e6547e8c0b67a2eb7e1d83ec400ced6e8275fa7acb1`.
- Publication refresh: `7f1b13a8f3655d59efa4655912d6e462621a806a`, six documentation changes only.
  Its native and migration trees are identical; no retest is claimed for the
  newer private implementation described in those documents.
- Seven consolidated branch tips remain ancestors; the seal records each exact SHA.
  No branch switch, rebase, force-push, main-checkout edit or push to the primary.

The complete SQL binary is reused from the prior sealed 754-unit clean production
build, source `62d030746265067638155314b3165bb135cb2c80`, not represented as a new
build. SHA-256 is
`1a7e305e8be3f516ae6778d6eb03fadcb14859e0876df6d279fc1d4bc75d937b`.
All 1,318 actual repository compiler inputs and 28 external header inputs match
the refreshed frozen source/image, byte for byte. The native and migration trees
also match. Its original command was:

```text
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=mariadb
  BIN_ROOT=/workspace/bin/tests/published62-native/sql/bin
  OBJDIR=/workspace/bin/tests/published62-native/sql/objects
  DMS_BINARY=/workspace/bin/tests/published62-native/sql/server
```

The image is pinned to
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
The ordinary FLATFILE production binary from the auction slice is not used here.
There are no C/C++ changes requiring a new whole-server build. The original
journey's printed phrase “sanitizer boot” is historical output; the reused binary
is the production SQL build and this report claims no sanitizer server execution.

## Commands, engines and original cases

The recorded Windows entry points are:

```powershell
$env:TEMP='D:\Dev\Temp'
$env:TMP='D:\Dev\Temp'
python -B D:/Dev/Temp/accounting-plan5-saved-recovery/freeze-candidate.py
python -B D:/Dev/Temp/accounting-plan5-saved-recovery/run.py green-mariadb
python -B D:/Dev/Temp/accounting-plan5-saved-recovery/run.py green-mysql
python -B D:/Dev/Temp/accounting-plan5-saved-recovery/analyze.py
```

Each disposable container executes `make world BIN_ROOT=/workspace/bin` and the
`tests/async/run_saved_item_recovery_journey.py` entry point through Python
`runpy`, with its real production binary argument and original flags. Only the
concurrent-child expectations and proof of a changed SQL ID are edited.
The full command list, inputs, captured outputs, migrations and server processes
are retained. The default invocation proves zero, one and two roots and invalid
room/prototype retention. The other positive flags are:

```text
--nested-replay
--malformed-child
--reject-child
--concurrent-save
--concurrent-child
--fault-stage=before_materialization
--fault-stage=after_materialization
--fault-stage=after_publication
--fault-stage=before_acknowledgment
--fault-stage=after_acknowledgment
--fault-stage=before_retirement
--fault-stage=after_retirement
--fault-stage=after_retirement_commit
--missing-destination
--tamper-destination-payload
--tamper-source-payload
--cyclic-source-topology
```

The nested control preserves UIDs 800000000001/800000000002, root/child placement,
affects and extra descriptions across two cold restarts and real player commands.
Every crash stops the real server with the original injected exit 80. Before ACK,
the original assertions require two source rows, zero destination rows and zero
receipts. After ACK but before retirement commit they require two source rows,
two destination rows and one receipt. After retirement commit they require zero
source rows, two destination rows and one receipt. Restart recovers one graph.
The tamper controls withhold untrusted publication and retain the source/receipt;
concurrent source/child replacement retains the changed generation. Original
timeouts and all other cases' assertions remain unchanged. The changed case
requires no visible backpack, two retained source and destination rows, an
unretired receipt, the source-payload conflict log and deferred-retirement log.
No mutation function is mocked and no assertion or timeout is bypassed.

MariaDB: `10.11.14-MariaDB-0ubuntu0.24.04.1-log`.
MySQL: `8.0.46-0ubuntu0.22.04.4`.
There are zero skipped current cases. Matrix qualification has a declared
1,800-second batch budget, original boot/fault timeouts and 180-second world
generation budget. Two engines run in distinct RAM source/scratch namespaces,
distinct D: build binds and their own loopback-only DB daemons, with external
networking disabled. They use only newly initialized schemas and synthetic
characters. Successful current journeys drop their private schemas after capture;
failed private evidence remains retained.

| Attempt | Exit | Seconds | Commands | Native server stops | SQL cuts |
| --- | ---: | ---: | ---: | ---: | ---: |
| `green-mariadb` | 0 | 779.202949 | 326 | 39 | 60 |
| `green-mysql` | 0 | 871.862276 | 328 | 39 | 60 |
| `matrix-mariadb` | 1 | 187.262360 | 73 | 8 | 14 |
| `matrix-mysql` | 1 | 387.680952 | 130 | 15 | 24 |
| `matrix02-mariadb` | 1 | 194.088737 | 72 | 8 | 12 |
| `probe-mariadb` | 1 | 1.454637 | 1 | 0 | 0 |
| `probe02-mariadb` | 1 | 2.393360 | 1 | 0 | 0 |
| `probe03-mariadb` | 1 | 7.574424 | 8 | 0 | 0 |
| `probe04-mariadb` | 1 | 29.503227 | 25 | 1 | 0 |
| `probe05-mariadb` | 0 | 24.412743 | 25 | 1 | 2 |

Across all retained attempts: 111 native server processes terminate,
8 started private DB daemons stop with exit 0, and 172 complete
read-only SQL cuts are retained. Native fault exits are 80; ordinary completed
server shutdowns are 0. Raw terminal results distinguish setup/client programs
from actual DB/server daemons. Every tracked source body, mode and link is checked
before and after each attempt. No terminal attempt is restarted or overwritten.

## Preserved failures and historical control

`probe-mariadb` fails before DB/server start because the world generator's native
layout is `bin/areas/tools`; the initial helper requested a deeper BIN_ROOT.
`probe02-mariadb` reaches generation but lacks the actual distribution `dos2unix`.
The prerequisite package acquisition initially exits 100 because the image has
no package lists; a separate disposable acquisition container refreshes signed
distribution metadata and downloads Ubuntu Noble `dos2unix` 7.5.1-1. The runtime
containers extract that real package into private scratch, rather than replacing
the utility or editing the generator. Package SHA-256:
`c743df55dfe9c58f211c96b1046a57957583b80febad235d081ce62c569546cc`.
Executable SHA-256:
`e202352c3809a485f55422d002a5f0785af17dcca488eac290b3d06acb84af22`.

`probe03-mariadb` fails before a native boot because the observer root connection
inherits the disposable restore user's MYSQL_PWD; the helper supplies the correct
empty password for that newly initialized daemon. `probe04-mariadb` completes the
native nested assertions but its evidence observer queries two guessed table names;
the attempt fails. Those names were replaced with verified canonical history
tables before the successful `probe05-mariadb` and subsequent matrix captures.
These are failed validation setup/observer attempts, not passing native qualification.

The first MariaDB matrix passes default and nested replay, then runs the historical
`--baseline-loss` control. That original flag intentionally expects the old defect:
publication followed by deletion of every durable source and loss after restart.
It correctly fails on the repaired candidate at “baseline unexpectedly retained
its only durable source.” The raw cut instead contains the retained destination
and retired receipt. This batch exit is 1 and is preserved. The following
`matrix02-mariadb` and `matrix-mysql` attempts both fail the stale concurrent-child
assertion. On restart, native recovery reports an acknowledged source-payload
conflict and withholds the destination; the room has no backpack. Both SQL cuts
retain the changed source, original destination and unretired receipt. Complete
source binding requires the original SQL IDs and payload, so the old assertion
demanding visibility contradicts that invariant. The existing guard was introduced
by primary commit `24dc63dc0777f43a97001478555d5983afcf5742`; migration0037 binds
both full snapshots. The fix verifies a different replacement SQL ID and requires
refusal plus durable evidence retention. Full green-mariadb and green-mysql
batches execute all18 current invocations on the fixed test source. None of these
three failed matrix attempts is relabelled as a passing batch. Historical
`--expect-abort` is not executed or accepted.

## Independent comparison and evidence

The temporary `analyze.py` imports no native writer, mutation plan or recovery
implementation. It independently reconstructs the saved SQL receipt byte grammar:
table ordinal and row count, ordered full rows from saved_items/affects/extra_descr,
then exact NULL or length-prefixed bytes, SHA-256; source IDs are ordered decimal
IDs with trailing commas. This compares the actual native receipts at the observed
SQL cuts. It finds 160 receipt cuts,
30 matching source-ID digests,
12 source-ID conflicts,
22 matching source payloads,
20 conflicting source payloads,
152 matching destination payloads,
4 conflicting destination payloads and
4 missing destinations. Conflicts belong to the
original concurrent/tamper controls; retired source absence is represented as
absence, never as a recomputed matching source. Per-receipt cut hashes and results
are in `independent-receipt-digests.json` (SHA-256 `72d57233e48ab925e99324053f9386adf78c4478e80fa1a7fb1c5ea9f7d63951`).

This comparison is explicitly scoped to the original ASCII fixture keys and
saved SQL timestamp encodings. It does not implement arbitrary SQL-collation
grouping, full cross-provider authority, operator acceptance or automatic repair.
Runtime audit still reports `saved_ground_full_runtime_authority_unqualified` and
`saved_ground_history_authority_unqualified` as appropriate. No earlier unknown
is cleared by passing these selected native cases.

Evidence root: `D:/Dev/Tests/Duris/accounting-plan5/saved-recovery-20261007`.
Build root: `D:/Dev/Builds/Duris/accounting-plan5-saved-recovery-20261007`.
Helpers: `D:/Dev/Temp/accounting-plan5-saved-recovery`; exact copies are sealed.
The package contains the frozen Git tar, source manifest, complete prior compiler
binding, generated full-world bodies/inventories, all original command inputs and
outputs, real native SQL general logs, read-only RR cuts, before-truncation native
logs, stopped private DB copies, original POSIX inventories, all failures, primary
document bodies, curator-ready report, seal and post-push delivery receipt.
Observed links/special entries are recorded but not copied/followed on Windows;
all copied regular bodies are verified against their native hashes. D: copy modes
are not substituted for observed native modes. The seal excludes only its own
directory and post-push delivery; it authenticates all other evidence/build bodies.

## Handoff and remaining gates

No new shared API/schema change is requested and no shared implementation is
edited. The earlier public raw saved-capture/full-handoff authority request remains:
complete saved rows/affects/descriptions, exact SQL key binding, typed modern room
item/coin history and all competing providers are required before native capture
and independent runtime authority can be granted. These focused native receipts
are useful evidence for that primary-owned interface; they do not replace it.

The publication checkpoint describes a private102-production/23-original-fixture/
five-schema cutover/nesting candidate, SHA-256
`5721d293aaa8878723fd4a0b2e9282b205f05c30b976d83cb47bbb55a0aa0682`.
It is not imported or exercised here. Shared coordinator/contracts/producers,
writer registry/matrix, native build recipes and activation stay with the primary.
No combined Plan1–4/Plan5 candidate is certified by this report. Full world/value/
UID/origin/writer census, modern room/collector and cross-provider saved authority,
native EAB2 installation/recapture, authenticated retention/erasure and backup
continuity, both-engine populated upgrades/reruns, original major-plan/native
producer/player/fault/load-budget acceptance and full R1–R8 remain open. No fresh
backup/restore or flatfile qualification is claimed by this SQL saved-recovery run.

Required primary docs and checkpoint were refreshed/read; AI_CONTEXT.md is absent
from tracked source and this owned worktree. The report and additive remote
follow-up are the curator-ready notebook packet. The primary-local notebook is
explicitly nonblocking. Curator acknowledgement/application, primary adoption
and combined-candidate publication are not claimed. Accounting remains inactive:
observed acceptance cuts have empty economic_epoch and economic_lineage_state.
No deployment, PR merge, production data change or audit autocorrection occurs.
Wallet-root ITEM_MONEY exclusions and the declined inactive spell-path change are
preserved. This completed slice is progress; the full Plan 5 goal remains active.
