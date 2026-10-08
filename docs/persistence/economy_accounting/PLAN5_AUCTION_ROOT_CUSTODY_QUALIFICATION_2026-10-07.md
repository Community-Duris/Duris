# Plan 5: retained auction root and claim custody audit - 2026-10-07

The independent operator now reconciles retained auction root metadata and claim
rights with current item custody in both directions. Previously a native-valid
custody catalog could pass its structural audit while an auction root had the
wrong owner, a missing UID, or an invalid claimant. The existing auction money
audit did not compare these root fields. This slice adds a separate read-only
command and preserves explicit template, coin-literal and history limitations.

## Branch, ownership and exact source

- Local/remote branch: `codex/accounting-plan5`.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Branch base: `069156eb82e3b42f45719f22754f57827f52532a`.
- Solved-issue code commit: `88b68ee8a2022493861ff32b4e6dffb571cf134c`.
- Refreshed primary base: `99b2a13a4141e8d36ac0f695e9e31d556139b5d6`.
- Final custody test tree: `2667f69c25807f5f2e552dce0df3e8be3132fcd1`.
- Final custody archive SHA256: `8ac021a32c7ccfaaffb8be41784f989819ac009645b9e6a7ebdd57d8ec53fa8d`.
- Native tree: `833d3085815b396861ad18a77635412212381e4b`.
- Canonical-64 migration tree: `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`.

The frozen primary export receives these exact nine blobs. Seven are the
current code/test slice; the other two are unchanged dependencies of the prior
published player/pet slice, which the refreshed primary has not yet imported.

| Owned file | Tested/result Git blob | Role |
| --- | --- | --- |
| `scripts/qualify_flatfile_native_auction.h` | `adb3b20a18e65d5cf442ee5a8de1de29742a2716` | current slice |
| `scripts/qualify_flatfile_native_auction_custody.h` | `f5cf3a49cc8ee56197bd2abbe979e68bcec96a6f` | current slice |
| `scripts/qualify_flatfile_native_custody.h` | `403cb109bbe10b30901342d93c489d8f3d32001c` | unchanged player dependency |
| `scripts/qualify_flatfile_native_player.h` | `41b9c7ec8c49b012352c174e3f4e04c80b414ac9` | unchanged player dependency |
| `scripts/qualify_flatfile_restore.cpp` | `231903ce93cf82db2b185faf27745cc30309848d` | current slice |
| `tests/async/flatfile_auction_money_cases.py` | `f2e8d66b1e1fc2abbe25adc34cf12f5d9b98410c` | current slice |
| `tests/async/flatfile_auction_money_fixture.cpp` | `80cd27cb93fd064aabb20a1982fe53ff62956906` | current slice |
| `tests/async/flatfile_custody_audit_fixture.cpp` | `3f53d4b6510343fa598c4581280914e4aa117201` | current slice |
| `tests/async/test_flatfile_custody_audit.py` | `c10c1f9f12908f2e12a0462883c9278604669fdc` | current slice |

Primary also needs the predecessor player slice `1691c3e1c7edf314c1cc26d82e6267e1d8af7b14`
and legacy receipt fix `d7c60d3c39da335abf997c49bfff291db0a00177` when integrating.
The branch's older native/migration trees are not the tested combined source.
Bare branch HEAD does not qualify a combined candidate. All seven earlier
remote tips remain ancestors; no branch is switched, history discarded, or
experimental-accounting push performed. Previous slice qualifications retain
their original source scopes and follow-up entries.

## Established omission and implemented behavior

`custody-final/red-controls.json` binds the prior published operator ELF
SHA256 `88a2ff816e9ab32b3801e48ec6f09174f025a81a26a852f037895c7152c640aa`
to its sealed receipt qualification. Its structural custody command accepts
four retained states: healthy, wrong auction owner, missing unclaimed UID and
wrong closed-auction claimant. It has no auction-custody command and refuses
that option generically. On those same states the new command accepts healthy
metadata and reports the three specific semantic defects, with unchanged
complete authority inventories. These controls establish an audit omission;
they do not establish corruption in a real installed authority.

```text
qualify --economic-auction-custody-audit ROOT [--limit 0..100]
```

The new pure header imports independent readers only. It takes the existing
noncreating shared authority read lock, refuses pending authority/currency/
player intents, protects regular private files, and applies the shared byte,
file and deadline bounds. It never calls native mutation/recovery providers.
For each unclaimed root it compares admission, active state, owner
`{type:6,id:listing_id,context:0}`, root=self, parent=0, item revision, vnum and
zero equipment. Duplicate unclaimed UIDs across listings and current auction
custody without an unclaimed root are findings. Closed claims belong to the
winner, or seller when unbid; removed claims belong to the seller. Open roots
must have no claim right or completed claim.

Completed claims grant no current ownership. Their UID must remain admitted,
but its current owner, revision, vnum and topology are not inferred from auction
history. Relisting and tombstoned claimed items are accepted. Custody versions
1-4 lack equipment fields; the command counts that absence and does not assert
verified root metadata. It returns only codes/UIDs and aggregate counts, caps
details at 100 while preserving exact totals, and emits no names or blob bytes.

The existing auction decoder now exposes an optional per-listing observer with
exact id/seller/winner/status/revision, borrowed blob and root fields. Existing
money/receipt consumers keep their original default path and validation. The
native oracle and independent decoder compare every retained root field and
SHA256 of every stored blob. A matching blob hash proves retained byte equality,
not validity or comparison of its serialized pickup template.

## Exact execution and results

Host launch commands from this worktree, with TEMP/TMP on `D:/Dev/Temp`:

```text
python D:/Dev/Temp/accounting-plan5-auction-custody/run.py custody
python D:/Dev/Temp/accounting-plan5-auction-custody/run.py money
python D:/Dev/Temp/accounting-plan5-auction-custody/run-make.py
python D:/Dev/Temp/accounting-plan5-auction-custody/run.py custody-final
python D:/Dev/Temp/accounting-plan5-auction-custody/format.py verify
```

The whole test entry points run through `runpy` without selection/filtering:

```text
python3 tests/async/test_flatfile_custody_audit.py --native-source /workspace
python3 tests/async/flatfile_auction_money_cases.py --native-source /workspace --artifacts /workspace/bin/tests/auction-money-whole
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile BIN_ROOT=/build/native OBJDIR=/build/native/objects DMS_BINARY=/build/native/server
```

Pinned offline/read-only image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
GCC13.3.0/Python3.12.3, 2 CPUs/4 GiB per job. Original flags, ASan/UBSan where
present, assertions, per-command timeouts and one 900-second outer budget
remain. No live job is restarted for an observation timeout. Fresh build outputs
are directly under `D:/Dev/Builds/Duris/accounting-plan5-auction-custody-20261007/`;
RAM links preserve original test paths. Strict native fixtures remain in RAM,
and their metadata are inventoried before copying to D:. Caches are disabled.

- Initial `custody`: all five original family suites pass, then the new healthy
  auction budget test incorrectly expects refusal for zero directory entries.
  The audit performs no directory enumeration and correctly succeeds. Its
  terminal failure, source and native stdout are retained. The final test checks
  that success and preserves byte/file/deadline refusals; no prior assertion or
  case is removed. Two positive claimant cases are added at the same time.
- Final `custody-final`: complete six-family driver passes, zero skips. Existing
  formats: custody133, world59, locker87, shopkeeper57, player/pet95. Existing
  finding scenarios: world37, locker37, shopkeeper38, player/pet45. Existing
  boundaries: custody13, world14, locker14, shopkeeper14, player/pet18. New auction
  coverage: **36 scenarios at limits0/1/100 and 14 boundaries**, native root/blob
  comparisons, sanitizer audit comparisons, and the three used budget refusals.
  It covers full-width identities/clocks, nine roots, exact108-finding totals,
  both auction versions, old equipment absence, duplicate/missing/orphan roots,
  wrong owner/context/state/revision/vnum/topology/equipment, invalid claim rights,
  historical relisting/retirement, malformed catalog and lock/permission/link
  refusals. Observer time **744.476304s**, **2114 commands**.
- `money`: whole existing qualifier passes **117 cases, zero skips**:77 native
  format comparisons (12 accepted/65 refused) plus40 monetary/security/lock/
  budget/inactive observations. All retained inventories remain unchanged.
  Observer time **324.238799s**, **265 commands**.
- `make`: clean flatfile production build passes **754 distinct units**, zero
  reused objects, warnings or errors; server links. Time **849.635047s**.
  Server SHA256: `9462829d443f5e138feac60c30bb27c8ec81b921fadfb2862be4a5386e3b54be`. This build does not start the server.
- Full-file clang-format18.1.3 fixpoint, both Python ASTs and `git diff --check`
  pass. Formatter image is
  `sha256:f87358723e903ec1b3ac9d28d78117fbc10487dccb7f034438591d91d90ca39c`.

Money and Make run on tree `3747b2c57ee408d068b776a41e00fba4e926e3e1`. Its source differs from the
final tree only in `test_flatfile_custody_audit.py`: the new budget assertion and
two claimant controls described above. Every money driver/compiler input,
operator/header, production source, migration and actual Make dependency remains
identical. Separate archives/manifests authenticate these scopes. No SQL database
or game server starts; both-engine database qualification is not claimed here.

All eight component ELFs compile freshly; only the separate prior-operator
omission control reuses its authenticated older ELF.

| Executable | SHA256 |
| --- | --- |
| custody-final / 1-fixture | `3c26826b674e86d7435e227dd3214cd9ad9c5cb7cccc2ebf96cd56510b8e7353` |
| custody-final / 2-qualify | `f0cf27d6d939e68ef523a0d1de91d733575d8732461c0d78e9ff6ef8da26ed29` |
| custody-final / 1802-native | `21b6b7de5be8622f409cc44e4b9e004ae481beb23b0d04554483248fc8d5481a` |
| custody-final / 1803-independent | `77798f152d2fa03438f324ad0046b19c602d748e92eee06185c62022357e1c6c` |
| money / 17-economic-fixture | `a744d50a68ea4c3b7ddcccb65a6cc1a583ce3cedf2a653d713c9690965b588d3` |
| money / 18-qualify | `f0cf27d6d939e68ef523a0d1de91d733575d8732461c0d78e9ff6ef8da26ed29` |
| money / 19-native | `21b6b7de5be8622f409cc44e4b9e004ae481beb23b0d04554483248fc8d5481a` |
| money / 20-independent | `4fa8061442e3702541e32a2f80f2de4b33530fe3d3354e2625e7a3fbbd0fa1ee` |

## Narrow shared adoption and recipe handoff

No shared interface, native contract, schema or migration change is requested.
The primary's exact62-source restore builder is used unchanged (blob
`471ec432f9185d7b9668fced506060e60ebcc30c`). The existing shared native fixture
recipes still omit current real providers. As in the predecessor receipt report:

1. `test_flatfile_accounting_store.py:SOURCES` needs
   `src/item/lockpick_retirement_continuation.c`,
   `src/economy/native_quest_cost.c`, `src/economy/native_quest_coin_give.c`.
   `test_flatfile_restore_economic_authority.build_fixture` additionally needs
   `src/economy/auction_native_command_context.c` and the actual
   `auction_native_expected_player_forest` body. Its non-GC compile uses the
   byte-exact native body via the existing
   `test_auction_retained_seller_fee.py` extraction recipe. Source offsets,
   owner/recipe/body/translation-unit hashes are retained in money's
   `native-forest-source.json`; no stub or rewritten provider is supplied.
2. `flatfile_auction_money_cases.build_native_oracle`, also consumed by the
   expanded custody driver, needs that real command-context source. Original
   GC flags discard unused forest binding, so this oracle adds no forest body.

These are explicit disposable compiler-source proposals, not shared source
edits or claims that unchanged maintained recipes now link. The economic fixture
gets exactly four real sources plus the extracted actual body; each native
oracle gets exactly command-context. `commands.json` preserves original and
executed argv. The seal reverses just these additions and the D: output override
to verify all original arguments/order/flags. The custody fixture itself retains
its original11 native units without provider additions. Primary owns recipe
adoption, registration and original entry reruns with zero proposals. Extend its
existing custody registration to reflect the sixth domain and new header; retain
all original family coverage and budgets.

## Evidence, curator packet and remaining gates

Evidence root: `D:/Dev/Tests/Duris/accounting-plan5/auction-custody-20261007/`.
Each execution retains source archive/manifest, Docker argv/terminal state,
complete compile/test argv, logs and results. The native test stages also retain
original mode/owner/body/link inventories and copied scratch. Final auction
case evidence is under
`custody-final/scratch/tests/duris-custody-audit-yg4p833d/auction-findings.json`
and `auction-boundaries.json`; prior omission controls are
`custody-final/red-controls.json`. Money's whole report is
`money/scratch/tests/auction-money-whole/evidence.json`. Make retains all754
objects/dependencies and the server directly on D:.

Pre/post fences authenticate every frozen body/mode/link; all owned jobs are
terminal with no OOM. POSIX no-follow verification authenticates copied regular
bodies and symbolic link targets. Original metadata remain in native manifests;
NTFS copy attributes are not native authority proof. The raw evidence seal uses
extended Windows paths without resolving or following copied native links.
`seal/evidence.json`, this report, the additive remote follow-up and post-push
`delivery/result.json` form the curator-ready packet. The primary's locally
maintained notebook remains nonblocking; application/acknowledgement, cross-chat
notification and primary adoption are not claimed.

Root metadata cannot establish serialized object-template literals or coin
values omitted from templates and supplied by world prototypes. Those
comparisons remain explicitly false, as do item-history/native-holdings,
full_R7 and release qualification. Complete physical/current-currency/UID/origin
census, NPC/treasury coverage, native V2 install/recapture, genuine producer/
player/fault/load journeys, authentic retention/erasure continuity, both-engine
upgrades and the primary's tested combined full Plans/R1-R8 candidate remain
open. No accounting activation, deployment, merge, production change or audit
correction occurs. Wallet-root exclusions and the declined inactive spell-path
change remain intact. Independent work is not blocked by notebook upkeep.
