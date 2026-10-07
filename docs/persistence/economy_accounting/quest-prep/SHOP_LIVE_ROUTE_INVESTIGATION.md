# Shop live-route source-contract investigation — 2026-10-07

Historical pre-approval evidence follows. The exact boundary has since been
approved at primary0f466967 and the one-file maintained repair implemented as
fd4563fab791b33a2905db22baa4167f3567c298. [Current committed import/execution
handoff](SHOP_LIVE_ROUTE_IMPLEMENTATION.md) supersedes the pending reservation
and unimplemented disposition below. Preserve all original private pins/failures;
no private execution is relabeled as maintained or native runtime proof.

Disposition: **private investigation complete; maintained repair unimplemented**.
The complete original `tests/async/test_shop_trade_live_route.py` still FAILS on
both bare primary and bare primary plus R10. Its obsolete syntax/slicing seams
can be replaced by meaningful checks of the actual current publication chain.
No production defect was demonstrated at this source-contract scope. The private
feasibility probe is not a maintained test PASS or native gameplay qualification.
Coordinator boundary/final review and any implementation authorization remain
separate. Actual continuing quest Goal remains **BLOCKED**, not complete/resumed.

Investigation starts from prep `5ec386905c141fd3dade1b74e15d7a8c5157c0e7`.
Published docs-only result is `4ca4016096812b0baad978cacc2b2844f8b5d229` on
origin/codex/accounting-quest-prep, authoring this document and HANDOFF.md only.
Analyzed accounting candidate is `b22d731bc66bd95f09e6cbc2180c99578c7f7205`.
Publication-time fetch observes `503384ca3c7b6bd913010ae3f0922623d503aa40`;
its sole successor commit changes only the R11 review documentation path,
and none of the analyzed source/tests/helpers. Execution keeps its actual b22 pin.
R10 code is `7bbf942e3b7c12dea22e5bf03a592b48aedd690f`. No optional prep or
R0–R9 patch was composed. This investigation does not duplicate R11 valuation.
Final publication fetch `434dc07908b66e2437b4ea0cd72b602f7c3d350b` differs from
b22 only in R11 review documentation. All analyzed source remains unchanged;
the sealed evidence retains its original b22 and earlier observed503 pins.

## Preserved original result and complete predicate census

Executed once on each immutable export, with its original script unchanged:

```text
python3 -B tests/async/test_shop_trade_live_route.py
bare b22: exit 1, 0.03906352422200143 seconds
bare b22 + R10: exit 1, 0.037240812089294195 seconds
stderr, both: live shop trade route is missing object->loc.carrying == keeper
```

Original script Git blob `95803d00847f74c16e1647c80e67dd7c433656a5`, SHA256
`36e072bf56638c8ac1c5eff312d53951a6ff0f82eb9b327bbad5469206df56bc`, is identical
in both exports. The original execution stops at its first failure. Separately,
`original_diagnostic.py` evaluates every original AST predicate independently and
retains assignment/index errors and unavailable dependent guards in DIAGNOSTIC.json.
That census does not bypass guards in the original execution or turn it green.

| Original requirement | Original diagnostic / actual current-chain proof |
|---|---|
| Exact selected UID, keeper lookup and snapshot matcher | Original three global predicates present. Actual physical body calls UID lookup and `shop_trade_source_bytes_match`; its ordinary branch delegates the exact runtime matcher. Matcher checks selected UID, full snapshot encoding length and byte equality. |
| `object->loc.carrying == keeper` and `object->loc.carrying == ch` | Both literal global predicates missing. `OBJ_CARRIED_BY` adds non-null and `LOC_CARRIED` guards before that same union-pointer comparison. Physical source branch requires produced/NOWHERE, buy-or-cleanup/keeper custody, or sale/player custody; destination is player-carried. |
| All five action kinds | Original buy_existing, buy_produced, sell_store, sell_destroy, discard_invalid predicates present; physical/action ownership branches remain actual owners. |
| Original three-argument submission | Literal missing. Four actual callers explicitly bind `shop_trade_publish_physical` and `shop_trade_completion` through `shop_trade_transaction_submit_with_publication`. Pending entry stores this hook before coordinator submission. |
| Failure diagnostic, rejected clone extraction, produced sequence map, container acceptance, produced continuation and cleanup helpers | All seven remaining original global predicates present. No replacement stub is needed. |
| `produced && keeper && OBJ_NOWHERE(object)` | Literal missing. Notification now requires produced + extant exact UID object + NOWHERE + exact source snapshot, and extracts FALSE only in the noncommitted, nondurable branch. A disappearing keeper does not prevent disposal of that authenticated unpublished clone. Committed/durable outcomes retain the alert/refusal boundary. |
| `obj_to_obj(object, destination)` and no fallible `put` | Original positive and negative predicates pass. Current physical owner preflights nesting, checks native placement result and custody readback, then uses the existing direct guarded nesting path. |
| At least nine cleanup-route occurrences | Original census passes. Preserve it and check actual callers: purchase lookup two, buy three, peruse two, listing one, plus helper definition. Authority refusal returns handled before any legacy fallthrough. |
| Trusted flat buy remains available; zero price; price carried to continuations | All three original predicates pass. Preserve the unavailable-message prohibition, exact flat `const int64_t transaction_price = IS_TRUSTED(ch) ? 0 : sale;`, and original count requirement. |
| Buy submit precedes transact and writeShopKeeper | Original submission index unavailable because API changed. Actual four-argument submission in defined shopping_buy precedes both original legacy calls, in their original order. |
| Produced clone staged within flat branch before submit | Original dependent guard unavailable. Actual flat branch after carrying preflight precedes `read_object(temp1->R_num, REAL)` and actual submission. Preserve that branch-specific requirement. |
| Sale submit precedes sql_shop_sell and ADD_MONEY | Original submission index unavailable. Actual submission in defined shopping_sell precedes both original legacy calls, in their original order. |
| Snapshot validation before detachment | Old callback slice cannot find the delegated matcher. Its first `static void shop_trade_completion` is a forward declaration at shop.c:572; defined notification is :1216 and physical owner is earlier at :197. Actual source/custody/full-snapshot validation and source_verified stage precede obj_from_char in that physical definition. |
| Cleanup snapshot validation before TRUE extraction | Same obsolete callback slice. Actual destructive branch rechecks the exact source snapshot immediately before destructive effects and checks absence of every payload UID afterward. Notification does not perform TRUE extraction. |
| Authoritative balances before callback | Old `completion(character, committed && published` spelling absent. Actual publish definition checks receipt/revision, publishes balances, custody and revision, invokes physical hook, marks physical_published, extracts pending, then calls finished.completion. False/throwing publication retains the original pending entry before notification. |
| Target root, target parent and expected target-parent revision | All three original transaction predicates pass. Actual publish_ownership carries selected-root fallback, parent/revision and expected item revisions into item_ownership_runtime_apply. Preserve these original requirements. |

Of the original twenty global predicates, exactly four literal predicates are
obsolete; all sixteen others remain required in the private probe. No original
ordering, authority, trusted pricing, cleanup, exact snapshot, negative put or
target-parent requirement was dropped. Scoped checks use existing `_paths`
`extract_function` to skip declarations and inspect actual defined bodies, and
existing `contract_text` helpers for code whitespace/comments/literal handling.
They do not require a new framework or accepting endpoint.

## Actual ownership, publication and refusal boundaries

All line numbers below refer to immutable b22:

| Owner / bridge | Actual source binding |
|---|---|
| Original keeper lookup / source bytes / published bytes | shop.c:134 / :156 / :163 |
| Physical implementation / ordinary bridge / accounted bridge | shop.c:197 / :457 / :464 |
| Produced continuation / invalid cleanup submit / authority route | shop.c:883 / :1026 / :1039 |
| Notification implementation / ordinary definition / accounted definition | shop.c:1056 / :1216 / :1225 |
| Buy / sell / purchase lookup / peruse / listing | shop.c:1879 / :2316 / :1759 / :2621 / :2698 |
| Payload construction / exact ordinary snapshot matcher | shop_trade_runtime.c:95 / :293 |
| Ownership projection / pending publication / hook registration | shop_trade_transaction.c:233 / :394 / :2825 |

The ordinary pending dispatcher permits a null publication hook generally:
`if (entry.publication && !entry.publication(...))`. A test of that dispatcher
alone cannot prove shop physical publication. The proposed proof must retain all
four concrete non-null production bindings: buy, sale, produced continuation and
invalid cleanup. It must also follow the real wrapper to physical_impl and the
defined notification bridge; existence of a matching name anywhere is insufficient.

Payload construction checks active state, exact UID/root/parent/owner and freezes
item, wallet, bank and shop revisions. Publication carries target parent revision
and item revisions into runtime ownership, checks the original player's runtime
identity, and publishes balances/custody/revision before physical effects and
notification. The physical implementation revalidates source and destination
custody, exact source bytes and final published bytes. Destruction verifies every
selected UID absent. The final published literal comparison preserves the existing
placement/key effects while separately choosing literal accounted capture.

Source/snapshot/location agreement alone does not prove artifact, bookkeeping or
extraction tails completed after an exception. Current stages refuse uncertain
placement, nesting, destruction, room notice or detachment STARTED-without-RETURNED
before new effects. Uncertain audit/cash likewise refuses. Checked placement and
reacquired exact UID/custody guard successful native effects. Dispatcher false or
exception leaves the pending original instead of acknowledging or replacing it.
These are source obligations in the private probe; no faulted runtime journey or
eventual completion guarantee is claimed.

Accounted preparation/restored publication belongs to the separate native owner,
with retained original receipt/identity and guarded ACK before notification. The
ordinary balance/custody/revision/physical projections are gated by
`committed && !entry.preparation`. Accounted physical publication requires the
original keeper runtime identity and player save revision; accounted notification
uses original-keeper lookup, rejecting missing/duplicate/wrong lifetime. An ordinary
chain PASS cannot stand in for this native owner's execution or recovery proof.

## Private execution and sensitivity evidence

The isolated QA container `quest-prep-shop-live-b22` uses unchanged image
`sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`, network
none, two CPUs and 3 GiB. Only source inspection/Python and sleep were used; no
DB, player, game, server, migration, compiler or gameplay operation ran. Container
is stopped after sealing evidence; exports and receipts remain ignored under:

```text
C:\Users\alexa\.codex\worktrees\accounting-quest-prep\NewDuris Max\bin\tests\shop-live-route-b22-20261007
container exports: /candidate and /candidate-r10; receipts/scripts: /evidence
```

Private programs executed:

```text
python3 -B /evidence/original_diagnostic.py
python3 -B /evidence/current_chain_probe.py
python3 -B /evidence/mutation_controls.py
python3 -B /evidence/replay_initial_controls.py
python3 -B /evidence/r10_compare.py
python3 -B /evidence/seal.py
```

The b22 probe PASS covers 106 source predicates in 23 actual function definitions.
Final sensitivity run rejects all 27 counterfactual in-memory source changes:
typed custody, exact snapshot, concrete hook bindings, notification bridge,
false-publication retention, premature detach/destruction/notification, uncertainty
refusal, checked placement, destroyed-UID readback, staged clone disposal, cleanup
authority, trusted flat price, legacy mutation ordering, fallible put, parent
revision, original runtime, accounted lifetime/path separation and receipt guards.
These corruptions are not compiled or gameplay tests; they measure source-oracle
sensitivity. No native economic operation or fabricated authority is involved.

Initial controls and failures are preserved separately. First-removal-only controls
incorrectly accepted three changes: one of two destruction readbacks; an accounted
pricing expression while the original flat const remained; one of multiple native
original-receipt guards. The remaining original safeguards still enforced the
intended requirement. The first actual run stopped on destruction sensitivity,
and a second stopped on pricing sensitivity. Replay INITIAL-CONTROLS.json retains
all three accepts. The final design targets both repeated readbacks/receipt guards
and the specific flat const, rejecting all 27. These design diagnostics are not
three new production defects or extra passing tests.

R10 composition applies its actual three-file code patch to a private b22 Git
index, producing tree `17840a134df38936d198d8923f56c26e76588035`. Only shop.c,
shop_purchase_quote.h and test_shop_purchase_usability.py differ. Complete original
live-route test still FAILS. Derived probe changes only ROOT/OUT; its verify()
function bytes are identical. It also PASSes 106 predicates/23 definitions and
rejects the same 27 controls. Only the buy definition differs among those 23
function bodies; all other 22 are identical. R10 is not the cause of the failure.

COMPARISON.json authenticates all 6,447 b22 and 6,448 b22+R10 file/link entries
against their canonical archives, including body/size/mode or link target, before
and after both probes. Both inventories are unchanged. No source checkout or
maintained test was modified. SHA256 pins:

| Artifact | SHA256 |
|---|---|
| primary-b22.tar | `c923d69b1046904112771c8869eea1a3e4979c76b1a39e2eb0208e9d1be8b301` |
| primary-b22-plus-R10.tar | `2bdcc472b47168e23249d6758e6d7dc75377e2c83cae173fec6767eb702b97de` |
| R10.patch | `d6abdd18bbfd91054d3a50e6c0352c2d952598b412d6d49e0cc2e30a0a074b32` |
| Original shop.c | `de8bdbe95cfa626c6ca869495ccc34afc0097af4c964d021ad2d66cb99d34393` |
| shop_trade_transaction.c | `e50dd031084f2b040c5b8ea65218fa41bc85f8410d6e2f270a9df0ddd9dcf133` |
| shop_trade_runtime.c | `eefaf6fe29c877302a84d8d9cbdc078326b6cf0f0dabb3fe05d9716a039f1c14` |
| core/utils.h | `7b02294677efae5defe16571e999a47f6232b3349bc05933a834a243634bee8d` |
| _paths.py | `bb1ffca804d125835894427e22ef3b286e3555e8ab9af75978a817bef00958e6` |
| contract_text.py | `65202fb525855c647053c98eded3204d332b9e747faa29deeb1ad1759439d9a5` |
| ORIGINAL.json | `7b4e7646696026aff704bcd71ce37b5baa90ca2afa903abf67de76d893c0bb45` |
| DIAGNOSTIC.json | `3662aa25f28859f61d767b8acc7210352a75668cd993161a747913fe060ab7d9` |
| CURRENT-CHAIN.json | `76124b720a578c2652dd43a95ad7887434cb2a9b2955c44567e430fff5cb470c` |
| FUNCTIONS.json | `006b0e7f92f706633851059a2c2b28473fd84109874c8cce5da1e948a570c090` |
| MUTATIONS.json | `5ea18cd6abf21fa74e77219381a3615b86ab4350428a8a2c40bd1cebf4829df8` |
| INITIAL-CONTROLS.json | `b1d3da675f8654ff15c6a0090fad40c3d4e4ec2b8a1394351a9c75e2bac7b7e9` |
| COMPARISON.json | `13b35e4a3152b1e4155046e98e25693f549856d55ba4e30a4c7b60a03d2997ad` |
| current_chain_probe.py | `5ee2be9707dfae8e1422002b896cf82cde049cd50e2fc7b387da40ab0a490f08` |
| mutation_controls.py | `f3c3049ed226561dc51eac4058fe24af6eb064b933a64101bac04b2dad313251` |
| r10/ORIGINAL.json | `a638a10034c84c63e460486c00275b6f65529a6764973f6745f96b97534c386c` |
| r10/CURRENT-CHAIN.json | `3a6df19988e95c73ced0e046afab910635d61b58ee999af73928c1d8a3f810e5` |
| r10/FUNCTIONS.json | `2448cceca0a7b779dbb2d2f425d302da636b29a9f941449ff8a3374374b9d7e4` |
| r10/MUTATIONS.json | `314ef97ccdbaa37091359d49b4f3b1912afe7c8b5eeb706341b30d381b2fcf05` |

SEALED.json retains the complete receipt/program hash index, source hashes,
function occurrence lines and original predicate census. Canonical compact sorted
inventory SHA256 is `c48fc0f755b5229f018b84f94067c4eddddcf3015b29873c2c65446d91dfe753`
for b22 and `3101e6938597e974e3568e63ed37ad59dc4f8207eb4a562f93c7c4422c32c206`
for b22+R10. Raw artifacts are private/ignored and excluded from commits.
SEALED.json SHA256 is
`76922aaf4218f6588950bc9a98fa1d42b06736aeb9a04d47c5661c2ef26c95d4`.

Coordinator separately reports PRIVATE feasibility PASS in
`/tmp/coordinator-live-route`, using the exact probe/controller hashes above and
only relocating output-directory literals. Its unchanged verify AST/106 predicates
and all27 controls pass, with all23 complete extracted bodies, entire shop.c,
custody utils.h and original test authenticated against canonical b22 Git source.
This is an independent source checkpoint; no maintained repair approval or native
runtime is claimed. Publication of boundary/final review remains coordinator-owned.

## Exact prospective reservation and remaining dependencies

Reserve **only `tests/async/test_shop_trade_live_route.py`** if the coordinator
approves implementation. Reuse `_paths.extract_function` and `contract_text`.
Keep all original semantic requirements and sixteen current global predicates;
adapt the four obsolete spellings and old callback/transaction slicing to the
actual owner definitions and chain above. Add concrete four-caller non-null
hook proof, exact source/final literal and revision/custody checks, uncertainty
refusal/retained pending checks and ordinary/native separation. Preserve original
trusted/cleanup census/anti-put/legacy-order/flat-clone/parent-revision guards.
No source rewrite, shared helper change, provider stub, migration, registry,
production path or optional architecture bundle is selected. No native/ACK or
accounting correctness guarantee may be inferred from source tokens alone.

No maintained repair has been authored or applied. Following boundary approval,
the required qualification is the complete maintained entry point on a fresh
bare-primary one-file import, with exact before/after blobs, source inventory,
preserved original failures and proportionate sensitivity evidence. This private
proof supplies a reviewable boundary, not authority to edit shared files.

The codec repair `0b0075724457511f6a7c6599489958cf45cd828a` independently passed
final coordinator review in primary b22; its bounded fixture dependency is closed.
Adoption remains unknown and there is no reason to rerun that unchanged codec.
Native lifecycle/verifier/reset-born source/custody, genuine cost/publication/ACK
capture, supported delayed bartender settlement, durable held-charge/refund and
native pre-ACK move/paired retirement remain owner dependencies. Their absence
still prevents original native/full-project completion. Historical seven-case,
SQL capture, legacy Kord and full-world bartender evidence keep their old pins.
