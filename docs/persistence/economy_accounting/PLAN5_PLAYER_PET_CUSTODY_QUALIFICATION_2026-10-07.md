# Independent current player and pet custody qualification — 2026-10-07

The independent operator lacked a current player/pet literal comparison. The
published baseline operator refuses the new audit option even for a healthy
native fixture. Plan 5 code commit
`1691c3e1c7edf314c1cc26d82e6267e1d8af7b14` adds that comparison without calling
native storage, recovery, reconciliation or snapshot codecs in the audit.
The complete original custody entry point, extended with this owner family,
passes on the exact canonical-builder composition below. This is component
qualification; full Plan 5, R7/R8 and release remain incomplete.

## Branch, ownership and source pins

Local and remote publication use `codex/accounting-plan5`. The worktree remains
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`; the configured
main checkout is unchanged. Code base is
`05092810201d73342ea4b296550e473a389b54d1`, code result is
`1691c3e1c7edf314c1cc26d82e6267e1d8af7b14`. The following publication commit
adds this report and the additive remote follow-up. The post-push delivery
receipt records its exact result and remote SHA. All seven earlier branch tips
remain ancestors, as recorded by the previous consolidation and the new seal.

Owned code files are exactly:

- `scripts/qualify_flatfile_native_player.h` — pure literal decoder and audit.
- `scripts/qualify_flatfile_native_custody.h` — reuse the item decoder with one
  full-snapshot row/object budget; the standalone-list wrapper preserves its
  original framing, limits and end-of-input check.
- `scripts/qualify_flatfile_restore.cpp` — one additive read-only command.
- `tests/async/flatfile_custody_audit_fixture.cpp` — native file-reader/encoder
  oracles, full literal comparison and direct byte-budget refusal.
- `tests/async/test_flatfile_custody_audit.py` — the existing complete driver,
  with player/pet formats, findings and boundaries appended. Its fixture recipe
  adds the called real `flatfile_player_snapshot_file` unit; original units,
  sanitizers, flags, assertions, tests and timeout policy remain.

No server implementation, migration, accounting contract, coordinator, registry,
writer matrix or activation owner is edited. The Plan 5 checkout still has its
older native/migration trees; bare Plan 5 HEAD is not the qualified combined
candidate. Qualification uses complete immutable exports of published primary
source plus the exact owned blobs and, for the final run, the unmodified
canonical shared-owner builder. No optional-provider fallback is added.

| Source pin | Exact value |
| --- | --- |
| First frozen primary base | `d3ec3b29260fe923059cb9038d1c42e54a00a1f6` |
| Proposal execution tree, green-04 | `3d4cacacb9ccac22b41785290f2487b81e673436` |
| Proposal archive SHA256 | `c8042f9e4db17fcfbca8952d773e698b295fb5deeaace93dcca719e84eac7149` |
| Final refreshed primary base | `71e421d12def1171f5538a30f14bee7c18452974` |
| Canonical shared builder commit | `1793deb80275f1fba2c6d9a73cebf278f13d54c6` |
| Canonical builder blob | `471ec432f9185d7b9668fced506060e60ebcc30c` |
| Final tested tree, green-05 | `0686bb7202afff2df7b9f5c9397be98a9ac459f2` |
| Final complete archive SHA256 | `160a7c3a9d83239a8fc4d37bd07bec861ce3b7fa587362890ef7b3772f9e2fdb` |
| Native tree, both executions | `833d3085815b396861ad18a77635412212381e4b` |
| Migration tree, both executions | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Canonical schema source | `0064_auction_custody_history`, 230 migration files |

The final archive has 6,491 regular files and four source symlinks. Every source
body, archive mode and symlink target is checked before and after execution.
The five code-result blobs exactly match the final tested composition:

| Owned file | Git blob |
| --- | --- |
| `scripts/qualify_flatfile_native_custody.h` | `403cb109bbe10b30901342d93c489d8f3d32001c` |
| `scripts/qualify_flatfile_native_player.h` | `41b9c7ec8c49b012352c174e3f4e04c80b414ac9` |
| `scripts/qualify_flatfile_restore.cpp` | `a7e6246f19f47505544f80533393d73c4cd0258a` |
| `tests/async/flatfile_custody_audit_fixture.cpp` | `235f45c78cd6e6039b0b264755f4627a237e85b0` |
| `tests/async/test_flatfile_custody_audit.py` | `0d4a2c83f4e1c4e4dac450464fdc3a56dbc73981` |

## Resulting audit behavior

The new command is:

```sh
bin/tools/qualify_flatfile_restore --economic-player-custody-audit /absolute/flatfile/root --limit 100
```

It reads canonical `players/<pid>.snapshot` files and `domains/item_ownership`
under the existing non-creating shared authority lock. It refuses pending
authority/currency/player-domain transactions, unsafe ownership or permissions,
linked files, malformed names, checksum/framing errors, unsupported versions
and exhausted original audit budgets. It never recovers a journal or changes
an audit finding. Directory identity is fenced across the scan.

The independent decoder validates the complete `DURPLYR\0` wrapper and all 30
supported wire versions: 1–8, 10–21 and 23–32. This includes complete non-item
framing, pets, ward fields, quest/spell/craft receipts, terminal death custody
and all five conflict-evidence tables. The 4 MiB payload, 8,192 total rows,
4,096 total objects and depth/string limits follow the literal native format.
Death corpse and conflict observations validate the frame and confer no
current player/pet ownership. Retained death sidecars/history are outside this
command's scope and `death_history_compared` remains false.

Current literals compare UID, owner/context, active state, root/parent topology,
vnum, known custody equipment and every retained detached coin byte. Reverse
comparison catches current player/pet custody without a matching literal.
Duplicate item/pet identities, unadmitted items, invalid item identities,
negative nested coin values and mismatches produce bounded fixed-code findings.
Player equipment is already one-based. Legacy zero-UID pets retain the native
player-owner mapping. Wallet-root `ITEM_MONEY` literals are counted as excluded;
they are not admitted to item transfers or reinterpreted as current holdings.

The new `flatfile_player_custody_audit_v1` report exposes aggregate counts and
bounded UID findings. `custody_equipment_fields_absent`, `coin_payloads_absent`
and `coin_payloads_compared` distinguish unavailable legacy fields from actual
comparisons. Scoped owner verification does not establish unknown legacy
attributes, item origin/history or currency balances. Other owner comparisons,
native holdings, item/death history, full R7 and release qualification remain
explicitly false. Character aliases and other private strings are never emitted.

## Exact execution and evidence

Evidence root:
`D:/Dev/Tests/Duris/accounting-plan5/player-20261007/`.
Helpers: `D:/Dev/Temp/accounting-plan5-player/`.
Direct build outputs:
`D:/Dev/Builds/Duris/accounting-plan5-player-20261007/<stage>/bin/`.
Native permission fixtures use RAM `/workspace/bin/tests`; D: lacks the required
POSIX metadata semantics. Compiler output locations alone are redirected to
the separate D: bin directories, with symlinks preserving original test paths.

Native runtime image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`:
GCC 13.3.0 and Python 3.12.3, network disabled, read-only container root, separate
RAM source/scratch mounts. The fixture compiles all 11 real native units plus
its original C++ driver with C++20, strict warnings/Werror, O1, ASan/UBSan,
frame pointers and no PIE. The operator uses the original maintained builder
and all 62 native units. Exact original/actual argv and results are in each
`commands.json`; no test filtering, assertion removal or sanitizer suppression
occurs. The complete original entry point is executed as `__main__` with:

```sh
python3 tests/async/test_flatfile_custody_audit.py --native-source /workspace
```

The original 900-second whole-driver limit and 30-second case limits remain.
Host launches, with `TEMP`/`TMP` explicitly on D:, are:

```powershell
python -X utf8 D:/Dev/Temp/accounting-plan5-player/run.py green-04
python -X utf8 D:/Dev/Temp/accounting-plan5-player/run.py green-05
```

Green-04 passes the whole driver using the previously identified three-provider
compiler proposal; it also executes four original-baseline omission controls
(healthy, wrong owner, missing pet and changed nested coin). All four baseline
invocations refuse the unsupported command with unchanged authority.
Green-05 uses the exact canonical owner builder in the export and passes the
whole driver with **zero compiler source proposals**. Only output paths change.
This does not edit the maintained builder here or claim primary adoption.

| Complete original/extended coverage, each successful run | Result |
| --- | --- |
| Custody formats | 133: 50 accepted, 83 refused |
| World formats | 59: 13 accepted, 46 refused |
| Locker formats | 87: 14 accepted, 73 refused |
| Shopkeeper formats | 57: 17 accepted, 40 refused |
| Player/pet formats | 95: 34 accepted, 61 refused |
| World / locker / shop finding scenarios | 37 / 37 / 38, each at limits 0, 1 and 100 |
| Player/pet finding scenarios | 45, all 135 detail-limit cuts pass |
| Existing custody / world / locker / shop boundaries | 13 / 14 / 14 / 14 |
| Player/pet boundaries | 18 |
| Skipped cases / final errors | 0 / 0 |

The 34 accepted player formats additionally pass the real native complete-file
encoder and independent re-read. Each generated native file is retained and its
SHA256 recorded. Literal native/independent fields match; original before/after
authority inventories remain equal. Cases cover legacy pets and custody,
global row/object exact limits and refusals, complete receipt/death envelopes,
wallet-root exclusions, orphan/duplicate IDs, equipment/topology/context and
complete nested coin bytes. All 18 new boundary controls retain unchanged files.

| Actual output | SHA256 |
| --- | --- |
| Both fresh sanitized fixture binaries | `4974765d840fd310a4f4d403c88bf217eb04dd77a32469eb17500e5b61e264e3` |
| Green-04 proposal operator | `313254455fabe6c5295c9657d96b8f0b662c5e303977da1815f88d1dffee493e` |
| Green-05 canonical-builder operator | `86120167d5a5c096033fdc351b30a56a5d8e4f6fe19d1b282bd4d884c54d9dce` |
| Green-04 complete log | `afddcab7a4e2dcee3a1016f7fac143b90e36d0ee1714c2d0186fc34d86c2695a` |
| Green-05 complete log | `71de61313c169f6e135deb31597c39ea67a8938452987704a998e1400dfab7a6` |

Green-04 records 1,813 commands and 306.454533 seconds; green-05 records 1,801
commands and 309.869196 seconds. These elapsed times include observer evidence
collection and source guards, rather than claiming a standalone workload budget.
Both containers exit zero without OOM. Earlier frozen preparation failures are
retained: duplicated Python entry-point syntax; the fixture encoder name and
catalog namespace ambiguity; and the first collector's absent-scratch error.
They do not qualify any candidate. No observation deadline or live job is reset.

Native inventories precede copying. Separate read-only POSIX re-reads verify
all 3,865/3,845 copied regular bodies and all 13/13 copied symlink targets.
Original native modes/owners stay in `native-artifacts.json`; NTFS copy modes
are not native qualification. Sealing never follows symbolic links. Python
syntax, strict C++ syntax, changed-line clang-format 18.1.3 checks and
`git diff --check` pass. The format image is
`sha256:f87358723e903ec1b3ac9d28d78117fbc10487dccb7f034438591d91d90ca39c`.

No database is started or migrated in this flat-file-only slice. SQL code and
server/migration inputs are unchanged. Earlier both-engine canonical-64 native
and managed results remain bound to their own prior frozen source and do not
qualify this new operator composition. A new full server build is not claimed;
the changed C++ tools themselves are freshly compiled and executed above.

## Shared handoff and remaining gates

No shared accounting API/schema change is requested. Primary integration should
take the five owned code blobs and classify the new independent header in the
existing component closure. The existing custody row remains the full entry
point, with its original policies and the added real snapshot-file dependency.
The canonical builder from owner commit `1793deb8` is consumed unmodified only
in the disposable final export; its adoption remains the primary's decision.
The earlier shared accounting-store recipe request stays with its owner.

Remaining release work includes complete physical/currency/UID and origin
census across all owner families, native NPC/treasury/auction/history closure,
original native V2 install/recapture, authentic erasure/retention continuity,
both-engine upgrades/reruns, genuine producer/player/fault/load journeys and all
original checks on one primary-published combined candidate. These fixture and
component results cannot close those gates. Private producer source and shared
activation interfaces remain with their owners; they do not prevent further
independent Plan 5 work.

Accounting stays inactive. No activation, deployment, PR merge, production
mutation or audit correction occurs. Wallet-root exclusions and the declined
inactive spell-path change are preserved. This report, additive follow-up,
`seal/evidence.json` and post-push `delivery/result.json` form the curator-ready
packet for the nonblocking primary-local notebook. No notebook application,
primary adoption, acknowledgement or cross-chat notification is claimed.
