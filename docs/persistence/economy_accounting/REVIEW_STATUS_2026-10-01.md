# Accounting review status - 2026-10-01

This checkpoint publishes the completed review fixes directly to
`Community-Duris/Duris:experimental-accounting`. It integrates upstream through
`ffb73e6518a2dc39f3d243d3384ea851b4c3b8a4`, preserving the canonical, staging
0045, and master 0031 migration histories. The master upgrade now appends the
0052 witness index and 0053 craft receipts without rewriting its existing prefix.

## Completed fixes

- Recipe Craft/Forge now use frozen item retirement, output publication, and
  durable progression receipts across SQL and flatfile recovery. Migration 0053
  supplies the SQL progression authority. See commit `bc4aeab0a` and its
  preceding focused repair, crafting, and pouch commits.
- Save admission integrates the upstream creation and sealed-save fences with
  the broader item-creation publication fence (`eeac01351`).
- The sanitizer prompt fixture now compiles without expensive optimization
  within its existing bounded runtime test (`77f95ef9f`).
- Null recipe output pointers are rejected before reading item identity, with
  actual movement/publication regression coverage (`0464613a1`).
- Recovery evidence updates are integrated, and recipe writer coverage is
  refreshed while retaining upstream creation-backend evidence (`72b3a5ab6`).
- Worker admission now handles an older save ACK that narrows the queued
  component mask while a newer capture waits for journal append. The actual
  worker regression fails before the fix and passes afterward; missing
  components and mismatched revisions remain rejected, and the craft receipt
  reaches its exact successful completion.
- Progression publication now keeps an admitted Craft/Forge save pending until
  its exact completion, instead of issuing a new revision every 500ms. A failed
  completion rearms capture without granting XP or rerolling a skill notch.
  The actual owner regression fails before the fix and passes under ASan/UBSan,
  covering delayed queued and coalesced saves, failure retry, and exact ACK.
- Mini-mode startup now installs progression callbacks before any player
  materialization. Both strict production builds passed, and the actual
  flatfile server completed mortal Craft and Forge with exact materials/tool
  retirement, fresh output UIDs, and durable XP.
- The real-server recipe fixture now supports a focused mortal journey. It
  uses the launcher copyover signal after crafting, retaining mortal progression
  semantics without granting a staff shutdown command. Physical and retained-
  pouch Craft/Forge pass on flatfile, MySQL 8.0.46 and MariaDB 10.11.14, including
  exact output UIDs, material/tool retirement, XP, counters, copyover, and two
  cold restarts. This qualifies the fixture's frozen leather recipe; it does
  not qualify every recipe variant or active accounting gameplay.
- Upstream artifact lifetime repair and master migration verifier files are
  integrated. Both production profiles rebuild successfully, and the native
  canonical/staging/master history matrix passes again on both SQL engines.
- Superior enhancement now computes its configured quote with checked 64-bit
  arithmetic and refuses negative or unrepresentable prices before mutation.
  The production payment function reproduces signed overflow before the fix
  and passes under ASan/UBSan afterward, covering physical/pouch materials,
  exact ordinary and boundary debits, free valid quotes, and insufficient funds.
  Both strict production profiles pass. Compound enhancement accounting and
  same-UID durable mutation remain outstanding.
- Superior material planning now widens item-value arithmetic, rejects negative
  values and non-finite or unrepresentable scaled quantities, and checks duplicate
  material totals before addition. The production planning regression reproduces
  signed overflow before the fix and passes with float-cast, undefined-behavior,
  and address sanitizers afterward. Ordinary fractional scaling stays intact.
- Superior stat caps now stop at the persisted signed-byte maximum and refuse
  invalid multipliers. Native boundary tests cover ordinary tiers, large finite
  multipliers, and non-finite inputs. Both production profiles pass for these
  fixes; all 1,210 source files match committed tree `49f76401c`.

- Flatfile `.craft` application receipts and `.craft-obligation` frozen terms
  now have separate protected retention/export inventory entries. The lifecycle
  validator requires both; removing either is rejected. SQL and flatfile recipe
  recovery policies must also remain protected and retained; nine policy-edit
  fault cases that previously passed are now refused. Twenty lifecycle,
  seven erasure, and six export regressions pass. The inventory now counts 49
  non-database stores, 225 SQL tables, and 42 Redis surfaces, with destructive
  rules and shared disclosure still disabled pending controller decisions.

- The administrator-property regression harness now supplies the unrelated
  world-activity reload hook required by upstream property application. The real
  initialize/set/revert/save/reload journey passes on SQL and flatfile again;
  the full regression run remains in progress.

- Four files identified by the full formatting check now match the repository
  style with unchanged C++ tokens and comments. The whole-tree formatter, ten
  runtime-compatibility contracts, and the native read-transaction cleanup fault
  harness pass. Both strict production profiles rebuild successfully.

- Native flatfile player snapshots, account records, identity allocation/name
  catalog, and item-ownership catalog now have separate required lifecycle
  entries, alongside legacy compatibility files. Twelve protection/reset edits
  are refused, and account export rules must exclude both stored password hashes
  and confirmation secrets. Controller retention and disclosure decisions remain
  pending; this adds inventory guards without enabling erasure or disclosure.

- The newborn-item grant lifecycle fixture now models the recipient's sealed
  save fence. Its production movement/publication harness passes under ASan/UBSan,
  including held grants to the actor and to another recipient, release after
  the save clears, and exact once-only publication after duplicate completion.

- Craft/Forge material planning now widens item-value arithmetic, checks finite
  positive scaling and integer limits before conversion, and leaves the caller's
  plan untouched on refusal. A shared wide required-level calculation also
  removes overflow from recipe availability, both command gates, and level
  messages. The production planner reproduces signed overflow before the fix
  and passes under address, undefined-behavior and float-cast sanitizers afterward.
  Both production builds pass for source tree `7360003fa`. Its real flatfile
  mortal physical and pouch Craft/Forge journey preserves exact inputs, output
  UIDs, XP and counters through copyover and two cold restarts.

- The terminal-death entrypoint fixture now compiles the production craft-receipt
  merge and component mask alongside quest XP. Controlled capture/ACK checks
  prove that progression capture refusal releases pins and queue bytes, while
  timeout/resume retains the original operation, discipline and XP even if live
  recovery state changes. Only the exact database ACK releases the request.

- Coupled death disposition/conflict, spell, quest-XP and save-quarantine recovery
  records must also remain protected and retained in lifecycle policy. Thirty
  edits that previously removed protection or selected reset/deactivation now
  fail closed. Twenty lifecycle, seven erasure and six export checks pass;
  destructive rules remain disabled.

- Craft info and make now release the successfully loaded temporary material
  probe when the other prototype is unavailable. The production error path
  fails before the fix and passes afterward under ASan/UBSan for all eight
  availability cases, with exactly one extraction per loaded object. Both
  production builds pass for `9fd3b77b1`; all 1,210 native source files match
  that committed tree. Its actual flatfile mortal physical and retained-pouch
  recipe Craft/Forge journey also passes copyover and two cold restarts.

- Native flatfile player wallet/progression records, account bank records, and
  both current and legacy transaction journals now have separate required
  lifecycle entries. All four omissions fail the inventory regression before
  the fix. The validator requires their exact locators and rejects twelve
  edits that remove protection or select reset/deactivation. Twenty lifecycle,
  seven erasure and six export checks pass; disclosure/controller decisions
  stay pending and destructive rules remain disabled. Other native domain
  catalogs still require inventory review.

- Ordinary and superior enhancement now share a wide configured level limit,
  capped at the maximum representable item value. Both command comparisons
  and messages use that limit. The production gates reproduce signed overflow
  for a level-50 character and a large valid multiplier before the fix; native
  ASan/UBSan checks pass afterward, preserving the ordinary 150/151 boundary
  and refusing positive item values when the level or multiplier is invalid.
  Both production builds pass for `cfb42c8ec`, with all 1,210 native source
  files verified. Existing payment, material, stat-cap and configuration checks
  also pass; compound enhancement durability is still outstanding.

- Core accounting lineage, mappings, baseline/activation records, immutable
  evidence, inbox/outbox dedupe and transaction journals must remain protected
  and retained. The validator previously accepted all 99 protection/reset/
  deactivation edits across 33 stores; it now rejects them. Twenty-one lifecycle,
  seven erasure and six export checks pass. This preserves the current recovery
  contract; disclosure and destructive policy decisions remain pending.

- Missing native flatfile UID authority no longer silently starts reservations
  again at UID 1. A permanent `metadata/item_uid_allocator.initialized` witness
  is durably written after the allocator state and before releasing IDs. Missing
  state with that witness, or surviving legacy custody, is refused; healthy older
  allocator files preserve their high-water mark during upgrade. Native ASan/UBSan
  checks cover four concurrent writers, missing/corrupt state, legacy upgrade,
  and an injected marker-write failure that releases no IDs and burns the range
  before retry. The actual flatfile server reaches healthy boot and clean shutdown,
  then refuses missing initialized authority without recreating it or changing
  the witness. Both production builds pass for `97ffd76d8`; all 1,210 source files
  match the committed tree. The allocator and witness are separate required,
  protected retained stores; SQL allocator protection is also mandatory. All
  34 lifecycle/erasure/export checks pass. The current flatfile binary also
  passes mortal physical and retained-pouch recipe Craft/Forge, preserving exact
  inputs, output UIDs, XP and counters through copyover and two cold restarts.
  Restore must preserve the allocator
  and witness together; this does not qualify older-generation rollback or an
  incomplete legacy root with all custody evidence missing.

- Ordinary and superior enhancement now refuse a rejected wallet debit before
  publishing an output, consuming material, or upgrading the retained item.
  Ordinary enhancement releases its provisional output on refusal and rejects
  negative configured fees; valid free quotes skip the debit API. Actual
  production-function regressions fail before `508912dd2` and pass under
  ASan/UBSan afterward for physical and pouch materials. All 15 affected
  enhancement checks and both strict production builds pass; all 1,210 native
  source files match that committed tree. Wallet admission is not a compound
  durable enhancement receipt; that integration remains outstanding.

- Essence modifier enhancement now validates its planned signed-byte modifier
  and accepts payment before changing the source item or consuming its essence.
  The production-function regression reproduces mutation after a refused debit
  before `a2c560b9d`; afterward ASan/UBSan checks preserve all state on refusal,
  exercise all three fee tiers, retain the ordinary three-step cap and reject
  overflow at modifier 127 even with a large configured step limit. All 16
  affected enhancement checks and both strict production builds pass; all 1,210
  native source files match that tree. This does not supply a compound durable
  wallet/item/material receipt.

- Essence description rebuilding now requires its prototype before charging or
  consuming the material. The prior production function dereferences a null
  template under UBSan; `5017a0e9e` refuses instead, preserves the player/item/
  material, and releases a valid probe if payment is refused. Encrusted items
  retain their separate description path without a probe. All 16 affected
  enhancement checks and both strict production builds pass; all 1,210 native
  source files match that tree. The new temporary cleanup is explicitly mapped.

- Native UID initialization evidence now retains sealed next-UID and allocator
  revision bounds on every reservation. Before `bf6aac22f`, restoring an older
  checksum-valid allocator reused previously issued UIDs. The updated native
  ASan/UBSan fixture refuses that stale file, preserves outputs and authority,
  upgrades the original eight-byte witness, detects damage to either sealed
  bound, and burns ranges after witness-write failures both before and after
  initialization. Four concurrent writers remain disjoint. The actual server
  refuses missing and stale allocator authority without rewriting evidence,
  then boots normally after exact restoration. Both strict production profiles
  pass, with all 1,210 native source files matching that tree. The new
  [UID recovery notes](../ITEM_UID_ALLOCATOR_RECOVERY.md) describe version-2
  evidence and upgrade limits. This protects partial stale-file recovery when
  the newer witness survives; restoring an entire older generation remains
  unqualified. Version-1-only binaries refuse the larger witness. Mortal physical and retained-pouch
  Craft/Forge also pass on that exact flatfile binary, with original inputs,
  fresh output UIDs, XP and generated counters preserved through copyover and
  two cold restarts.

- Ordinary enhancement now computes material floors, configured value gains,
  luck-scaled search budgets, cascade steps and bounds using checked-width
  arithmetic. The production-function regression reproduces signed overflow
  before `e8ca4e448`; afterward ASan/UBSan checks cover an INT_MIN material
  delta, unrepresentable gains, an INT_MAX cap plus roll, a fourfold INT_MAX
  search budget and cascade lookup next to INT_MAX. Out-of-range hash keys are
  skipped before narrowing. All 17 affected enhancement and deferred-finding checks and both strict
  production builds pass; all 1,210 native source files match that tree. Normal
  free/paid quotes and refusal cleanup remain qualified; compound durability
  remains outstanding.

## Verification and its limits

| Check | Current evidence |
| --- | --- |
| Strict SQL and flatfile production builds | Passed for `e8ca4e448`; all 1,210 native source files match its committed tree. Flatfile recipe gameplay now passes at `bf6aac22f`; both SQL engines are separately qualified at `97ffd76d8` and their latest integrated-binary journeys are in progress. Recipe gameplay does not execute the enhancement payment repair. |
| Recipe SQL receipts and fault recovery | Passed on disposable MySQL 8.0 and MariaDB 10.11 before the latest upstream integration. |
| Integrated focused runtime checks | All 16 selected checks passed after the first upstream integration, including the ASan/UBSan prompt fixture. |
| Frozen broad regression | The `8be8b55c0` snapshot completed all 822 scripts in 7,221.31 seconds; the runner reported 818 successes and four failures. Property reload, formatting, newborn grants and terminal-death entrypoints fail on that snapshot and pass their separately recorded focused repairs. Optional checks are included in the runner's success total: the MariaDB combat journey skipped because `TEST_DB_HOST` was not configured. This is not an all-green current-head or SQL gameplay qualification. A fresh 828-script run is in progress on frozen `63309643c`; it predates the enhancement payment fix and has no final result yet. |
| Null-output publication regression | Passed using the actual native movement module after `0464613a1`. |
| Combat continuation and quarantine restore | Passed after integrating the upstream recovery evidence. Quarantine archives retain both original component frames and keep recovery fenced on conflicting evidence. |
| Writer census | Refreshed for `e8ca4e448`: 2,811 lexical occurrences, 2,753 unique sites, 863 routes, zero unmapped sites. The new provisional enhancement-output cleanup is explicitly classified; both moved wallet call sites retain their existing route ownership. The four temporary material-probe cleanup sites are explicitly classified in the recipe route. All 52 writer coverage checks pass on the frozen `508912dd2` source in 236.014 seconds, along with 14 accounting fixtures and matrix freshness; the essence fix reanchors the same route set and passes its 14-fixture validation, matrix freshness and corrected focused enhancement check; enhancement assertions now resolve their unique current source calls. Content-keyed parsing caches at most 32 source strings and refreshes after source edits. |
| Recipe flatfile restore | Passed through the actual native restore decoder: complete recovery, checksum damage, missing obligation/root, future revision, and invalid receipt filename. |
| Mortal recipe gameplay | Physical and retained-pouch Craft/Forge passed on the current `bf6aac22f` flatfile binary with exact XP, output UIDs and counters through copyover and two cold restarts. Both disposable SQL engines now pass the same physical and retained-pouch journey on the verified `97ffd76d8` SQL binary, preserving inputs, output UIDs, XP and counters through copyover and two cold restarts. This remains one frozen leather recipe with accounting inactive. |
| Death/resurrection accounting acceptance | **RED** on disposable MariaDB 10.11.14 and MySQL 8.0.46 using the verified `49f76401c` SQL binary. Real combat/death and resurrection returned all 12 original fixture item UIDs, restored the exact wallet, and retired the new death coin pile. The inactive-accounting journey still reports 27 uncovered item events and two currency operations missing accounting roots/postings. It does not qualify active accounting or subsequent restart. |
| Latest migration integration | Runtime manifest validator and all 34 native boot/migration contract tests passed with head 0053 and 225 tables. All three histories converge on the pinned schema on disposable MySQL 8.0.46 and MariaDB 10.11.14, including preserved master receipts/runtime payloads, append/replay, shell/compiled boot, restore selection, and tamper refusal. The fixture uses the current sealed baseline with the master prefix; a captured production clone remains outside this proof. |

Prior build and component passes do not establish an all-green result at this
publication commit. Detailed synthetic fixtures remain in `tests/async/`; local
logs and disposable binaries are not published as production artifacts.

## Remaining work

The accounting release validator remains **blocked**: 750 runtime/projection
writer routes still lack the required qualification. The generated coverage
matrix describes that gap; a complete census alone cannot close it.

Recipe flatfile restore and mortal Craft/Forge restart qualification are
published, along with mini-mode startup and save-admission fixes. Broader recipe
variants and active accounting journeys still need qualification. Current
strict builds pass. The full 822-test regression run finished on the frozen
`8be8b55c0` snapshot, with the four failures and optional-check limit described
above. Each failed check has a passing focused repair. Subsequent enhancement
and recipe bounds, probe cleanup and lifecycle protections have separate native
sanitizer, production-build or policy-fault evidence. A complete current-head
regression and integrated accounting qualification remain outstanding.

Paid same-UID superior enhancement, remaining pouch writers, day-one quest and
loot paths, audit completeness, activation/recovery/backup/retention gates, and
operator policy decisions remain open under [the completion plan](FINISH_ACCOUNTING_PLAN.md).
The branch is **not yet ready to merge or activate**. This push records current
implementation and review status; it performs no deployment or production action.
