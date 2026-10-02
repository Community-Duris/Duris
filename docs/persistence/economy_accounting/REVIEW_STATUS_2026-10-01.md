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
  seven erasure, and six export regressions pass. The inventory now counts 47
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

## Verification and its limits

| Check | Current evidence |
| --- | --- |
| Strict SQL and flatfile production builds | Passed for `9fd3b77b1`; all 1,210 native source files match its committed tree. Current flatfile recipe gameplay uses that binary; SQL recipe gameplay remains separately qualified at `ad5bc52bf`. |
| Recipe SQL receipts and fault recovery | Passed on disposable MySQL 8.0 and MariaDB 10.11 before the latest upstream integration. |
| Integrated focused runtime checks | All 16 selected checks passed after the first upstream integration, including the ASan/UBSan prompt fixture. |
| Null-output publication regression | Passed using the actual native movement module after `0464613a1`. |
| Combat continuation and quarantine restore | Passed after integrating the upstream recovery evidence. Quarantine archives retain both original component frames and keep recovery fenced on conflicting evidence. |
| Writer census | Refreshed for `9fd3b77b1`: 2,809 lexical occurrences, 2,751 unique sites, 863 routes, zero unmapped sites. The four added temporary material-probe cleanup sites are explicitly classified in the recipe route. Coverage contracts and artifact freshness pass. Content-keyed parsing caches at most 32 source strings and refreshes after source edits. |
| Recipe flatfile restore | Passed through the actual native restore decoder: complete recovery, checksum damage, missing obligation/root, future revision, and invalid receipt filename. |
| Mortal recipe gameplay | Physical and retained-pouch Craft/Forge passed on the current `9fd3b77b1` flatfile binary with exact XP, output UIDs and counters through copyover and two cold restarts. Both SQL engines retain the earlier separately qualified `ad5bc52bf` journey. |
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
strict builds pass. The full 822-test regression run uses a frozen `8be8b55c0`
snapshot and is in progress. The subsequent enhancement price, material, and
stat bounds fixes have separate native sanitizer and production-build evidence. Earlier full-suite failures
have not yet been superseded by a complete passing run.

Paid same-UID superior enhancement, remaining pouch writers, day-one quest and
loot paths, audit completeness, activation/recovery/backup/retention gates, and
operator policy decisions remain open under [the completion plan](FINISH_ACCOUNTING_PLAN.md).
The branch is **not yet ready to merge or activate**. This push records current
implementation and review status; it performs no deployment or production action.
