# Plan 5 retained EAB1 upgrade from 56 to 64 — 2026-10-08

Fresh copies of the authenticated, previously retained native fixture EAB1 dumps
pass the actual current 56-to-64 migration path on MariaDB 10.11.14 and MySQL 8.0.46.
Each appends eight steps, preserves all 56 original migration receipts exactly,
and preserves the original columns/values of 18 economic/native tables. Current
native known-retained verification and three independent Python readers accept
the known original values with SELECT-only access. The archived original native
fixture also replays exactly. NULL admission timestamps and origin markers stay
NULL; unseen full command headers stay unobserved. Native holdings/items and
source completeness remain missing, and the audit still reports their precise
exceptions. These are retained native fixture dumps, with originally modeled
source/holding values; they are not historical production captures or real player
journeys. Full Plan 5/R1–R8 and the combined release remain unqualified.

## Branch, source and ownership

Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Local and sole publication branch: `codex/accounting-plan5`.
Base: `15f43cc2488a1668f736720ea850912fba24cc80`. The containing report commit is the result; the
post-push `delivery/result.json` binds its exact local/remote SHA. All seven
previous branch tips remain ancestors, and their work/follow-ups continue here.
The main checkout and existing worktrees remain untouched.

Only this report and the additive `PLAN5_REMOTE_FOLLOWUP_2026-10-06.md` change.
No native/server/audit code, shared original fixture, coordinator, producer,
registry/matrix, activation, schema or migration file changes. The additional
external native probe is a test reference; independent operator audit code still
uses its separate Python decoding/SQL readers. No interface/schema fields change.
The narrow shared original-recipe request is below.

| Executed input | Exact identity |
| --- | --- |
| Refreshed published primary | `aa1613f5b3378cd253a046813e3c0de525206de7` |
| Composed primary plus 19 owned overlays | `01ecb10455e8ccd8b4c7bee7e3bc57b2403d3645` |
| Native tree | `833d3085815b396861ad18a77635412212381e4b` |
| Migration tree | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Source archive SHA256 | `c212e25bb98c6ac4b1590b22757caf17fca72cdb99079c264fb05cdbc4d3adaf` |
| Current canonical head | `64 / 0064_auction_custody_history` |
| Current native probe ELF SHA256 | `c421a1fe3e6202b5fcefe7313e676350473ae4e59d4c42ead576ed0c48e9c9f0` |
| Archived fixture ELF SHA256 | `9ff48b67a9e4cff9b71e5001e9783eaa0f51f82b3efe278248ddc81ad423f7bf` |

`source02.json` names every overlay blob, regular-file hash/mode and link target.
All 6508 regular bodies and four source link targets authenticate to 6512 Git
blobs. Both successful terminal guards verify exact frozen bodies, native modes
and links. The earlier `source01` freeze precedes the primary documentation-only
refresh and was not executed. Native/migration trees are identical across them.
`AI_CONTEXT.md` is absent. Required plans, requirements and latest checkpoint bytes
are retained under `refreshed-primary/`; the primary-local notebook is nonblocking.
The private 128-production-file candidate `bd4d4a56c71cbabe2ecc8b56880e2046a856905f458cfdfb164ea4409a255ffe`
is separate and unexecuted here. Published-source checks do not qualify it.

## Historical input authentication

The original retained inventory is
`D:/CodexEvidence/accounting-plan5/bin/bb259-maintained-20261005/tests/p5-baseline-contract-20261005/original/retention.json`,
SHA256 `260b5330faabcb7da0f47719d693653c2fed2192039bd11adaf0a458e4cb1555`. Each body below matches that
inventory, and the three original replay/dump pins also match the earlier 61
qualification. No old evidence is overwritten or relabeled as current 64.

| Retained body | Bytes | SHA256 |
| --- | ---: | --- |
| `client-free.json` | 213 | `3a7f7f3c13c6f38b01cd5b504a95b0009cc685fdcdb0922115d4518115f8bf19` |
| `fixture-sql` | 21031488 | `9ff48b67a9e4cff9b71e5001e9783eaa0f51f82b3efe278248ddc81ad423f7bf` |
| `mariadb-baseline.sql` | 332977 | `1ed0c5dc945cd867d1751fe4fc5f4139af3975166e79c0779a31d5971f3946e8` |
| `mysql-baseline.sql` | 341737 | `31dca46e57e175e3c51562a090352314a467954b5041bc32b016f9dfc55f4522` |

Both retained dumps begin at actual `56 / 0056_spell_ward_durability`, contain two
native persisted EAB1 roots in separate epochs, and retain their original lineage,
operation IDs, witness/plan/intent bytes, normalized projections, source claims
and inbox receipts. Original batch 1 has one holding/item; batch 2 has two. These
historically persisted test roots do not prove native coverage or authenticate
originally modeled source fingerprints. The archived binary's output identifies
those same original IDs; it is never called a current-native build or service.

## Commands, native boundary and results

Exact host commands from the stated worktree:

```text
python -X utf8 D:/Dev/Temp/accounting-plan5-legacy56-current64/launch.py legacy01
python -X utf8 D:/Dev/Temp/accounting-plan5-legacy56-current64/launch.py legacy02
python -X utf8 D:/Dev/Temp/accounting-plan5-legacy56-current64/launch.py legacy03
python -X utf8 D:/Dev/Temp/accounting-plan5-legacy56-current64/summarize.py
python -X utf8 D:/Dev/Temp/accounting-plan5-legacy56-current64/seal.py
```

Every launch uses pinned image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
2 CPUs/5 GiB, network none, task-private native RAM source/database scratch and
direct D: evidence/build mounts. No existing DB service, socket, environment,
player/account files, volume or production mount is used. Actual tools are
GCC 13.3.0, Python 3.12.3, GNU nm 2.42 and mysql_config 10.11.14; hashes and complete
version strings are in `seal/native-build-inventory.json`.

The external probe compiles actual current providers with the original baseline
recipe's C++20, Wall/Wextra/Wpedantic/Werror, O1/debug, ASan+UBSan, frame-pointer,
non-PIE, mysql_config and crypto flags. Its complete 20 translation-unit argv and
source hashes are in each stage's `native-compile-inputs.json`. It replaces only
the standalone main with a borrowed-transaction read-only caller; no production
provider body is changed. The original 15 providers accompanying the old main
remain, plus four actual providers described below. No link stubs, removed
assertions, changed sanitizer/link flags or new authority owner are introduced.

The probe calls the current exported interface:

```cpp
unsigned int economic_sql_baseline_verify_known_retained_in_transaction(
    MYSQL *, const critical_operation_id &, economic_baseline_batch *) noexcept;
```

It disables reconnect, connects as the SELECT-only reader, starts a real READ
COMMITTED / READ ONLY transaction per original root, verifies known retained
values, and confirms rollback. It also requires ENOTCONN (107) outside a transaction
and ENOENT (2) for an absent nonzero operation, with output unchanged on both
failures. It never constructs the unknown original command/header or invokes
private baseline apply/reconcile. Native reference output matches the original
operation, lineage/epoch, version 1 and holding/item counts. The Python operator
readers are independent consumers and do not call this native mutation owner.

| Attempt | Result | Elapsed | Commands / launches | Scope |
| --- | --- | ---: | ---: | --- |
| `legacy01` | FAIL, exit 1 | 55.443927s | 3 / 3 | Native link failure; zero daemons/imports |
| `legacy02` | PASS, exit 0 | 65.572104s | 35 / 41 | Both real engines; 18-table preservation/readers/native replay |
| `legacy03` | PASS, exit 0 | 68.269714s | 36 / 42 | Same full checks plus explicit exact original 56-receipt preservation |

The final successful stage performs, separately on each real engine:

1. Initialize a fresh empty private database, import the retained engine dump and
   prove its actual head 56. Save original 56 history receipts and original columns
   of 18 named economic/native tables before migration.
2. Run the real current manifest and `migration_runner.MysqlExecutor/run_pending`,
   with its original body/checksum/verifier/lock behavior. Require exactly eight
   appended steps and head 64. Compare all 56 original receipt fields, including
   descriptions, apply/verify checksums, compatibility, runner and timestamps.
3. Compare original 18-table columns/values exactly. Capture all current columns
   of those same 18 tables; require two EAB1 witnesses, unchanged NULL admission
   timestamps/claim-origin markers, and inactive lineage pointers.
4. Require the archived original binary's replay JSON to match `client-free.json`
   exactly. Separately require the current native SELECT-only reference to pass
   the original roots and both failure/output-invariance controls.
5. Run independent `economic_sql_canonical_audit.capture`,
   `economic_sql_audit_snapshot.capture` plus `Reconciler.audit`, and
   `qualify_database_restore.require_economic_evidence_integrity`. The last runs
   in a repeatable-read consistent READ ONLY transaction through a SELECT-only
   cursor adapter. Canonical results retain `release_qualified=false`.
6. Require an UPDATE by the reader to fail with 1142. Confirm original and all
   current 18-table columns stay exact after every replay/reader/control. Run the
   real manifest again, require no steps, unchanged 64-history receipts and
   unchanged checked authority. Stop each private daemon normally.

`10.11.14-MariaDB-0ubuntu0.24.04.1`: PASS, eight steps, original 56 receipts exact;
original 18-table digest `9c1bffa98682c261c545d9e535e66ad176159fcc923b6b42757f61c52886d5e4`;
all-current 18-table digest `a3abe1f573d7441a30519f529ea0cf43104ee87d1d7d1686b2b6d16a1ff0fcc7`;
retained 64-history digest `2dc21c859e7b074345c16b523876ae29fd5ca00ac177a9019827366cddb69ac8`.

`8.0.46-0ubuntu0.22.04.4`: PASS, eight steps, original 56 receipts exact;
original 18-table digest `bb516731f5fc3125d60dbe296b40c5d75e8ff3f2a3a1a02557eaf97cf564c36f`;
all-current 18-table digest `5c1cb7571acfc445ca15d7fd18a92482cda011e68c7b0951d82adb71d1b91559`;
retained 64-history digest `62823aa15527ba8a43583f886bfc849c560496e414013230895920bba05b1560`.

Final totals: two restored databases, 16 appended migrations, four healthy
current-native root verifications, four native refusal/output-invariance controls,
two archived native replays, six independent reader/integrity calls and two
SELECT-only UPDATE refusals. Zero explicit skips, zero full server boots, zero
activation. Four initial daemon-readiness SELECT 1 retries exit 1 before sockets
are ready; all substantive final stage commands pass. The first summarizer
incorrectly treated these expected readiness polls as test failures; its script
and failure output are retained as summarize-initial.py and
summary-initial-failure.json. Corrected classification requires the exact private
socket/SELECT 1 readiness argv and changes no stage result or native evidence.
The unmodified Reconciler still reports, on each engine:

```json
{"evidence_loss": 1, "missing_native_holding": 2, "missing_native_item": 2}
```

The two source claims remain present; no balances/items are invented to clear
these findings. These checks cover the named 18-table authority snapshots and
history, not an exhaustive every-table application-data preservation assertion.
`make -C src`, real player/gameplay journeys and the full current original baseline
fixture suite are not run in this slice; repository C/C++ source is unchanged.

## Narrow shared original-baseline recipe handoff

The first external probe copied the original baseline provider list, replaced its
main, and added the three previously established command-decoder providers.
It failed specifically at these current native calls:

```cpp
unsigned int economic_sql_pending_claim_source_contract(MYSQL *);
unsigned int economic_sql_pending_claim_source_stage(
    MYSQL *, const critical_operation_id &, uint16_t,
    const economic_account_key &, uint32_t, uint64_t);
unsigned int economic_sql_pending_claim_source_verify_baseline(
    MYSQL *, const critical_operation_id &, const economic_baseline_batch &);
```

All three real definitions are in
`src/persistence/economic_sql_pending_claim_source.c`; current baseline
`evidence_values` directly references them. Adding that actual provider to the
external compile preserves every flag and links successfully; both engine checks
then pass. Failed argv/linker output remains in `legacy01`, not overwritten.

Primary-owned consumer `tests/async/test_native_sql_baseline_audit.py` still has
its original 16-entry sources list. It omits this pending-source provider and the
three earlier established providers:
`src/item/lockpick_retirement_continuation.c`,
`src/economy/native_quest_cost.c`, `src/economy/native_quest_coin_give.c`.
The original `tests/async/run_native_sql_baseline_audit.py` also still pins 62.
Request: primary owns completing that actual recipe's four-provider boundary and
current head 64 consumer, then runs the full unchanged original baseline/equipment/
claim/cold-clone suite on both engines with all assertions/faults/flags retained.
This probe establishes the concrete provider dependency and known-retained case;
it does not claim that full original suite was executed or that a primary fix is
applied. No new public field/schema/API is requested. Keep historical NULLs and
unknown headers, current root/plan/witness/projection/source checks, original
transaction/output invariants and exact equipment/version policies intact.
The existing flat auction/provider and room seed/UID owner handoffs also remain.

## Evidence, curator disposition and remaining gates

Raw evidence: `D:/Dev/Tests/Duris/accounting-plan5/legacy56-current64-20261008`.
Builds: `D:/Dev/Builds/Duris/accounting-plan5-legacy56-current64-20261008`.
The final `qualification.json` derives only from terminal `legacy03`; `legacy02`
and failed `legacy01` are preserved separately. Source archives/authentication,
historical input binding, exact commands/launches/compile bodies, original/current
authority snapshots, 56/64 receipts, canonical/export/reconciliation JSON, native
outputs and daemon logs are retained. All current/native source guards pass.

`seal/evidence.json` SHA256 `c09a03523393791e49176a108d63f4eb2c746c5cc8bd7edc4e86be1e43ca4c8c` covers 163 regular files /
666757239 bytes and 8 native build files, with zero
copied links/reparse points. Native lstat/body inventory precedes Windows regular
body verification. All three stage containers and the seal container are stopped
and network-disabled. Post-push `delivery/result.json` rehashes every sealed body,
verifies exact same local/remote result, clean worktree, all 19 owned overlays and
all seven earlier ancestor tips, and records the final primary refresh.

The additive remote follow-up is curator-ready. The shared notebook is maintained
locally by primary and is nonblocking; application/adoption/acknowledgement is not
claimed. No other chat is messaged. Remaining gates include full original current
baseline/flat/room fixture qualification; actual primary private combined native/
compiler/SQL/gameplay/persistence/recovery execution; genuine native holdings/item
coverage and writer/player/source/load journeys; captured historical production
or master upgrade paths; complete backup/restore/retention and full R1–R8 release.
Known NULL command headers are permanently unobserved for these retained roots.
Accounting remains inactive; wallet-root ITEM_MONEY exclusions and the declined
inactive spell-path change stay. No merge, activation, deployment, production
access, audit auto-correction or push to experimental-accounting occurs. Goal active.
