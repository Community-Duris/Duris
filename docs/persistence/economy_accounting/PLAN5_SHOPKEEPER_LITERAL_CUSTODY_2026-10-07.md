# Plan 5: native shopkeeper literal and custody audit - 2026-10-07

The previous operator had no independent shopkeeper literal comparison command.
Four before/after cuts establish that refusal on real native-readable DURSHOP
V1/V2 inputs and its repaired behavior. The new read-only command compares
durable shopkeeper literals with custody and reports retained cash observations
without claiming holding, history, source-admission or release qualification.

Branch/worktree: `codex/accounting-plan5`,
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base `018cb09f2c5850641d18ed8bcaf2c0c9b0befa16`. The containing commit is the
result; the external delivery records exact local/remote SHAs and checks the
seven earlier work tips remain ancestors. No branch was switched. Only this
requested remote branch receives the slice; experimental-accounting remains
primary-owned. Shared coordinators, contracts, native producers, migrations,
registry/matrix, activation owner and maintained runners remain untouched.

Owned files: scripts/qualify_flatfile_native_shopkeeper.h (new),
scripts/qualify_flatfile_restore.cpp, tests/async/flatfile_custody_audit_fixture.cpp,
tests/async/test_flatfile_custody_audit.py, AUDIT_OPERATIONS.md, this report and the
additive PLAN5_REMOTE_FOLLOWUP_2026-10-06.md. No shared API/schema fields change.

Exact tested owned tree `4b1c2df15419494942fcbcdc464e221c1dd3fcb3`, archive
SHA256 `8cc0908cfc50e44aa6c637cc22289e53163eed55513512674fec3fa40a094beb`.
Owned native tree `4abb609524a1f1682ea4c190f82d75003c4d679b` and migrations
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` are unchanged older source, not the
current combined candidate. Publication differs only in the three documents.

Actual native oracle `d98f67e6c657cd39d24617ef8fb93dc3b058469f`, whole tree
`59afa02396c79572d2599b7f8fef0cd907301f65`, native tree
`833d3085815b396861ad18a77635412212381e4b`, migrations
`7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`, archive SHA256
`51a38242149bbcd17a9ff6c04ea927ebc01efb1016568c99b0a1e3612f032f56`.
A later source/document refresh reached
`d0cc767fae87777b26528e3269aa8bd827e5e5ab`, retaining those native/migration
trees. The later primary documentation and private candidates are not promoted
by this slice. Required AGENTS/README, Plan 5 handoff, finish/remaining/checkpoint,
shared native progress and relevant native bodies were captured/read as raw Git
bytes. Tracked AI_CONTEXT.md is absent. Raw document bytes remain preserved.

The CLI is
`--economic-shopkeeper-custody-audit /absolute/private/state-root [--limit 0..100]`.
It uses the existing protected shared read lock and aggregate 128 MiB/30 s budget,
checks pending journals before and after observation, and creates no lock or
authority file. Unsafe/corrupt files refuse with the fixed error and no partial
JSON. It calls no native storage, recovery, producer, mutation or correction path.
Missing catalogs remain absent and unverified. Complete totals are invariant at
every detail limit; UID details and retained cash totals use decimal strings.
Private item strings are omitted.

The independent reader validates complete DURSHOP V1/V2 envelopes, exact length,
checksum/EOF, nonzero catalog revision and bounded keeper/affect/item counts.
Shop IDs are sorted and unique; zero is valid. The native custody transform is
kind9, uint64(shop_id)+1, context zero, including UINT32_MAX -> 4294967296.
Mobile/room vnums are positive, saved timestamps are nonnegative signed64 and
keeper revisions are nonzero. V2 retains cash -1..INT32_MAX and a boolean roaming
flag. V1 has unknown cash -1 and roaming false; no zero cash observation is
invented. Cash counts distinguish known and legacy values; known totals describe
retained catalog observations and do not establish ledger agreement.

Complete signed affect fields and five uint64 bitvectors retain the native
non-strict tuple order: type/location/modifier/duration/bitvectors. Duplicate
affects are valid. Complete item forests reuse the independent item decoder,
with positive globally unique UIDs/vnums and native keeper slots0..255. Children
require slot zero; positive root slots are unique per keeper. Keeper slots remain
distinct from the custody field, whose shop-owner policy requires zero.

The audit compares admitted UID, active state, owner/context, root/parent, vnum
and custody slot policy. Available retained coin payloads must agree in every
literal byte after detached parent framing, including the native keeper slot.
Negative first-four money denominations produce findings even if both literals
agree. Legacy absent inline payloads stay explicit legacy inputs. Catalog,
keeper, custody owner and item clocks remain independent. Other owners are
counted and uncompared; native holdings, item history, full R7 and release stay
unqualified in every result. These comparisons do not execute source admission.

Pinned image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
GCC13.3/C++20. The expanded existing fixture links ten real providers: the
original item/authority/store/snapshot/transfer/critical/source/world/locker
providers plus flatfile_shopkeeper_repository. Original strict warnings,
ASan/UBSan, no PIE and section/GC flags remain. The unchanged maintained operator
builder keeps its ordinary recipe. Full native repository reads occur only in
private initialized fixture roots. Accepted cases compare every native record
field and reencode complete item fields; actual item_shopkeeper_owner_id is used.

All57 shopkeeper formats pass:17 accepted/40 refused. They cover native V1/V2,
empty/legacy inputs, ID zero and full-width IDs/revisions, signed timestamps,
cash boundaries, non-strict affect ordering and4096 affects,4096 items, per-owner
slots, binary item fields and malformed envelope/metadata/order/forest bounds.
All38 findings pass at limits0/1/100 (114 cuts), including every retained coin
field, native slots, topology, missing/unadmitted UIDs, negative money and legacy
payloads. All14 existing read-only boundary controls pass for this domain.
Original133 custody cases (50 accepted/83 refused),13 custody controls,59 world
formats (13 accepted/46 refused),37 world findings/14 controls and87 locker
formats (14 accepted/73 refused),37 locker findings/14 controls also pass in the
expanded original driver. Zero skips; no assertion, sanitizer or timeout failure.

Original Linux inventories assert authority bodies/stat/nlink/inode/mtime remain
unchanged through operator reads. Controls cover unavailable locks, pending
journals, public modes, symlink/dangling/hardlink and zero files. Frozen source
bodies, canonical Git tar modes0664 and all four tracked link targets remain
unchanged through terminal completion. Copied NTFS modes do not qualify Linux
permission/link observations. The full driver closes0 after200.163233296 s;
log SHA256 `de4f70ebbce2e2965bdd813cbfb32c88ea6dd64bf4b2704d532c66f7f95e3396`.
Two successful -MM probes bind336 tracked inputs:118 actual-primary/218 owned.

Maintained reproduction:
`python3 -u -B tests/async/test_flatfile_custody_audit.py --native-source /absolute/exact/native-checkout`.
The retained observer invokes all the original driver functions, builders and
budgets while preserving artifacts instead of deleting the temporary evidence.
Exact compiler/operator/Docker argv and executed helpers remain in the packet.

Additional native regressions establish primary-owned compile-recipe defects:
the unmodified test_flatfile_shopkeeper_repository.py and
test_flatfile_shopkeeper_ownership.py both fail to link against this exact oracle.
The failure packet closes1 with original source/flags/assertions/budgets intact;
log SHA256 `76af99e2f6a340f744b2581bb1b4e633b6efbf630fa4b049376cd91a346f2690`.
An isolated recipe proposal adds only three real providers to repository:
src/item/lockpick_retirement_continuation.c, src/economy/native_quest_cost.c and
src/economy/native_quest_coin_give.c. Its full original repository test passes.
Ownership additionally needs src/economy/shop_trade_recovery_manifest.c; that
intermediate failure remains. The ownership-only four-provider proposal passes
without repeating the passing repository cases. Original ownership's explicit
load stub remains intact; its pure reconciliation pass is not persistent-load
qualification. No passing native case was repeated for observer repair.

Recipe01 closes1 with one pass/one link failure, log SHA256
`9dd7475f66fe1e98ef795a7432d127ff890fa796c319bf0cb1c4131f7dab9b74`.
Recipe02 closes0 after9.345080276 s, log SHA256
`b3cd41d69f8b37fa90fb1e7e2123df020d86cb0d83cf9f1e5d8956a2fd0fa540`.
Two successful -MM probes across these proposals bind101 distinct inputs,
including the retained generated ownership harness. Original tests execute via
their unchanged Python entry points; only proposed compile argv gain the listed
real providers. No shared test body, flag, source omission or assertion changed.

Narrow primary handoff: add those three providers to the repository recipe and
four to the ownership recipe, preserving all original providers, flags, budgets,
assertions and the ownership stub. Register the expanded original custody driver
additively through the maintained runner. Consumers are those exact tests and
independent Plan5 owner comparison. No API/schema fields or invariants change.
Proposals are qualified; shared application/notification is unclaimed.

Repository format/check passes with clang-format18.1.3 before freezing source.
Incremental command
`make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile BIN_ROOT=/workspace/bin/tests/money-history-build/bin OBJDIR=/workspace/bin/tests/money-history-build/objects DMS_BINARY=/workspace/bin/tests/money-history-build/server`
passes after1,291 native inputs match the prior build. All740 objects/.d and
server remain byte-identical; no fresh compile or combined build is claimed.
Server SHA256 `f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4`.
`python3 -u -B scripts/validate_economy_accounting.py` passes; --release still
returns1 for `writer has no executable evidence`, without waiving that gate.

Evidence root `D:/CodexEvidence/accounting-plan5/bin/`. Green01, red01, Make01,
format01, source-refresh01, originals01 and recipe01/02 are terminal and retained.
Seal shopkeeper-custody-seal-01-20261007/evidence.json SHA256
`59c86c27527a93d8e7a2713d65151d24d15335aee866655b7ddf4f894aef0364`
binds2,957 entries/649,968,014 bytes, including original failures and symlink
targets observed without following them. Build binding
shopkeeper-custody-build-binding-01-20261007/binding.json SHA256
`ac6576483dc072c1bfe09a68febfdd547becf35c9d5da65d47655df0240f33e0`
rehashes all1,491 earlier regular build bodies/1,294,533,318 bytes. Publication
and delivery bind exact tree/archive/result/remote and ancestry separately.

This flatfile component has no applicable SQL check; both-engine release checks
remain required. Remaining gates include original shared recipe application,
private native V2/combined source export and full qualification, all native
holdings/ledger/history/treasury owners, original gameplay/persistence/fault
journeys, current combined builds, canonical migration fresh/upgrade/rerun on
MySQL/MariaDB, authentic backup/cold restore/retention and full R1-R8. Older0055,
catalog inventory, component and synthetic passes do not qualify release.
Accounting remains inactive, wallet-root item exclusions and declined inactive
spell behavior remain. No activation, correction, production change, deployment
or merge occurred. This report/follow-up/seal/delivery is curator-ready; the
primary-local notebook is nonblocking, with application/acknowledgement unclaimed.
Full Plan5 remains active.
