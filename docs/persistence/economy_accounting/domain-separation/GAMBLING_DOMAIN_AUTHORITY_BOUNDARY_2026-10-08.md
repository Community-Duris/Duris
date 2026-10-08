# Gambling domain, native rounds and accounting authority — 2026-10-08

Published blackjack has a card/hand rules implementation and a generic finite-stake
accounting vocabulary. It has no admitted active-accounting round producer.
Preserve the active refusal. A modular conversion needs an original round identity,
retained participant and table lifetime, frozen card outcomes, compound stake and
wallet authority, and a publisher/recovery owner. None follows from extracting a
denomination calculation or decoding an accounting plan.

The one selected future independent slice is actual-provider acceptance of
`Hand::BlackjackValue` and its genuine card/hand lifecycle. It protects the score
used by hit and periodic settlement, needs no new economic rule and does not
enable gambling. This is a design proposal; no implementation or runtime proof
for that slice is claimed here.

## Frozen inputs, scope and disposition

Source/schema/test findings use published primary
`e3e82a92de9537b2f5c1505fa48403c473fc34d9`. Its source and async-test trees are
unchanged from the accepted Collector candidate `20510d07da21759396bbe8775150f1d9aafc4233`.

| Tree | Git object |
| --- | --- |
| `src` | `833d3085815b396861ad18a77635412212381e4b` |
| `tests/async` | `790f367adf805a69d53aac6460938f5c921f9136` |
| `migrations` | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |

The exact three-tree archive is 45,762,560 bytes, 3,069 files, SHA256
`208bf5ee7413d9d2740fdcbee241243816f8050bd4b17d58440948f36123394b`.
It is privately extracted under `D:\Dev\Temp\gambling-authority-20261008\source`.
Additional published scripts/contract examples and owned prior findings are
separately exported and pinned. Explicit line anchors below refer to these frozen
bodies, not a moving checkout. The appendix supplies Git blobs; private manifests
also retain complete SHA256 hashes.

Owned prior findings use parent `463fd659a1d95f6227b0ed6aad18bb7dfaebfb95`:
[R2 reservation](R2_RESERVATION.md), [completed extraction handoff](HANDOFF.md),
[operation inventory](OPERATION_INVENTORY.md),
[currency authority map](CURRENCY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md),
[post-R14 dependencies](POST_R14_OWNER_DEPENDENCIES_2026-10-07.md), and
[accepted Collector capture delivery](COLLECTOR_COLLECTION_NATIVE_CAPTURE_HANDOFF_2026-10-08.md).
R2's `24fa551ae16900b41e509f79fe4685762e0fb2e9` preparation is closed at its
reported scope. The owned `src/economy/currency_value_plan.h:37` accepts captured
native counts; the public candidate still has the inline wallet calculation at
`src/economy/currency_transaction.c:1037`. Do not infer public adoption, repeat R2
numeric tests or add another currency helper for gambling.

The newer primary report at
`docs/persistence/economy_accounting/FINISH_ACCOUNTING_PLAN.md:84` describes private
candidate SHA256 `a41d13a7a78b0eb7e4ff8e34281d8050fbad2c112433ddaa24a7ac404d0527b4`,
140 production files and 75 selected C providers, with prospective role-aware
NBC4/NMB4 accounting, MBR4 results and retained live publication charging.
This is owner-reported/source-review evidence; its compiler/native/SQL/gameplay/
persistence/recovery qualification is **UNEXECUTED**. Its bodies were not imported
or executed here. Full birth-to-SHOP atomicity, physical publication, cold recovery
and driver/admission work remain unfinished.

This delivery changes **this document only**. No production, test, migration,
registry, coverage, shared driver, Plan 5 or finish-plan edit is made. No build,
regression, database, server, journal, native gameplay or recovery run is made.
Accepted Collector bundles/jobs and all closed work remain preserved. The continuing
Plans 1–5/applicable original R1–R8 native Goal remains **BLOCKED and unfinished**;
this preparation does not resume, complete or change its lifecycle.

## Actual callers, identities and round state

Object prototype 55434 is assigned `blackjack_table` at
`src/specs/specs.assign.c:2640`. Room object command dispatch supplies the native
object and current command actor at `src/cmd/interp.c:2843`; the actual special
invoker checks native object/prototype and player-pet restrictions at
`src/item/objmisc.c:237`. The generic object heartbeat can invoke the same special
with null actor and `CMD_PERIODIC` at `src/world/db.c:4373`, when scheduled.
Ordinary creation schedules it only when initialization returns true
(`src/world/db.c:5289`); this inactive table initialization returns false, so that
return does not request a heartbeat. The table's own `event_dealersturn` calls
`blackjack_table` directly and reschedules after two ticks only while its call
returns true and the table is still in dealer state
(`src/economy/cardgames.c:882`). The direct event is explicitly scheduled by stay.
Do not infer that two callbacks are always scheduled from these two call sites;
neither supplies a durable once-only settlement guarantee. A future owner must
serialize actual state advancement if callbacks become runnable together.

The active guard is the first action after local declarations:
`src/economy/cardgames.c:343`. With active authority, OFFER with an actor emits
the unavailable message and returns true; every other call returns false. It
therefore blocks a pending periodic payout, SAY, initialization and lock/unlock
handling before dereferencing table timers. `active()` tests whether the current
authority projection exists (`src/economy/economic_gameplay_authority.c:263`), not
a caller-selected Boolean. The guard remains authoritative for both active SQL
and flat authority; do not bypass it to build a fixture journey.

Inactive native state is:

| Native storage | Actual meaning and limitation |
| --- | --- |
| `obj->value[0]` | State constants PREBID=0, POSTBID=1, POSTDEAL=2, POSTHIT=3, DEALERSTURN=4 (`src/economy/cardgames.h:111`). No durable sequence or revision. |
| `obj->value[1]`, `[2]` | Bet count and denomination 0..3, stored after successful `SUB_MONEY` return (`src/economy/cardgames.c:624`). No committed stake account or original operation ID. |
| `obj->timer[0..2]` | Cast deck/dealer-hand/player-hand addresses (`src/economy/cardgames.c:373`, `src/economy/cardgames.c:660`). Native timers are `time_t[6]`, not a pointer codec (`src/core/structs.h:506`). Never persist these as round identity or card evidence. |
| `Hand::owner` | Raw `P_char`, set when `deal` constructs `Hand(ch)` (`src/economy/cardgames.h:60`, `src/economy/cardgames.c:664`). OFFER does not bind its actor to that owner. SAY branches do not check actor equality to the existing hand owner. |
| Object/character identity fields | `obj_uid` exists (`src/core/structs.h:493`); character `runtime_id` is process-local and changes on storage reuse (`src/core/structs.h:1564`). This table route authenticates neither a table-UID/round sequence nor a stable participant PID/runtime lifetime. Names in logs are not identity. |
| `static lock_game` | Shared across calls/tables, changed by trusted SAY lock/unlock (`src/economy/cardgames.c:342`, `src/economy/cardgames.c:468`). Periodic settlement occurs before this lock check; locking is not pending-round cancellation. |

At `CMD_SET_PERIODIC`, the inactive table sets PREBID and zeroes all timers
(`src/economy/cardgames.c:355`); it does not settle a stake or delete an old deck
there. No per-round identity is allocated in `blackjack_table`. A future owner
must issue and retain its own original round identity through the existing domain
authority, not derive it from a pointer, timer, name, table VNUM, command timestamp
or a new ID minted on every retry.

## Operation and mutation map

All behaviors in this table are **inactive legacy source behavior**, not an
implemented active economic route or a claim of tested gameplay.

| Operation | Source behavior | Values versus authority |
| --- | --- | --- |
| OFFER | PREBID required; parse with `atoi` and `coin_type`, require positive count/type0..3; compare that denomination with native cash and configured cap. Calls `SUB_MONEY(count * unit, 0)` then stores count/type (`src/economy/cardgames.c:491`, `src/economy/cardgames.c:592`). | Parsed count/type, captured limit and a checked quote can be values. Actor validity, current cash, debit acceptance/completion and stake creation are authority. Insufficient denomination branches emit a message but fall through to SUB_MONEY; do not document them as an unconditional early refusal. |
| deal | POSTBID required. A failed `say` emits a message but does not stop the branch. Allocate deck/hands, shuffle seven times, deal dealer one card then player two, store POSTDEAL (`src/economy/cardgames.c:635`, `src/economy/cardgames.c:660`). | Card IDs/order and resulting scores can be frozen owned values. Allocation, RNG/config acquisition, participant binding and advancing the actual round are game-thread work. The dealer's hidden card is not drawn until stay. |
| hit | POSTDEAL/POSTHIT required; remove one deck card into player hand, set POSTHIT, then reset on score>21 (`src/economy/cardgames.c:751`). | Score/bust classification is deterministic for captured card IDs. Actual card removal, card lifetime, loss settlement and cleanup are retained native/round authority. |
| stay | POSTDEAL/POSTHIT required; draw dealer's second card immediately, set DEALERSTURN and schedule a two-tick table-owned event (`src/economy/cardgames.c:696`, `src/economy/cardgames.c:719`). | A frozen dealer-hand/remaining-deck transition is a value. Scheduling and keeping table/participant alive remain event/native authority. |
| periodic draw | Replace incoming actor with `playerHand->getOwner()`. If dealer score<17 and card count<5, take one card and return true (`src/economy/cardgames.c:378`). | Score/card-count decision can be deterministic. Real pointer validity, current round phase and deck ownership are required before access/mutation. Soft17 stays because the score is17. Five cards stop draws even below17. |
| periodic terminal win | If dealer<=21 and player>dealer, or dealer>21, add `2 * bet` directly to `ch->points.cash[type]`, then reset (`src/economy/cardgames.c:397`, `src/economy/cardgames.c:410`, `src/economy/cardgames.c:460`). | Total return is stake plus one stake of net winnings. This direct int addition has no attached gambling plan, revision, receipt or publication ACK. |
| periodic push/loss | Equal scores return one stake directly; lower player score returns nothing (`src/economy/cardgames.c:419`, `src/economy/cardgames.c:430`). | Push returns held stake; loss consumes it. Source logging and reset alone do not produce durable accounting legs. |
| fold / bust | fold allowed only POSTDEAL/POSTHIT; fold and hit bust go to reset with no refund (`src/economy/cardgames.c:730`, `src/economy/cardgames.c:778`). | Classify these observed outcomes as loss, not an automatically refundable interruption. New interruption policy requires an explicit owner decision. |
| reset / showgame | Reset deletes deck/hands and zeroes state/bet/timers; showgame only renders the current native state (`src/economy/cardgames.c:799`, `src/economy/cardgames.c:870`). | Rendering and deletion are not persistence or terminal receipt proof. showgame is not a recovery interface. |

There is **no double command or doubled-stake transition** in this provider, and
no special natural-blackjack/3:2 payout. A two-card21 is simply a score21, handled
by later ordinary hit/stay/periodic branches. `2 * bet` is the ordinary winning
return, not evidence of a double-down action. Cardgames' complete SAY branches
are deal/stay/fold/hit/showgame plus trusted lock/unlock. Do not invent missing
operations from the word blackjack or change these rules in a modularization.

The adjacent `magic_deck` is a separate legacy special, assigned prototype55433
at `src/specs/specs.assign.c:2633`, with its own early active refusal at
`src/specs/specs.gellz.c:218`. Its stay branch settles synchronously through
`do_win`, including dealer bust (`src/specs/specs.gellz.c:501`); it is not this
table's retained periodic provider. The earlier inventory's
`specs.gellz.c:blackjack_table` wording must not be used to select a copied body.
No other casino special conversion is selected by this map.

## Exact money, randomness and frozen outcomes

Native wallet cash is four signed ints (`src/core/structs.h:1151`), with values
1/10/100/1000. OFFER checks the chosen denomination and a property named
`blackjack.MaxBetInPlatinum`, default100, scaling the limit by1000/100/10/1
(`src/economy/cardgames.c:520`). The property, scaling, count multiplication,
`2 * bet` and payout addition use native int arithmetic; source inspection does
not establish safe bounds for arbitrary configured limits or near-INT_MAX cash.
A future route must freeze the effective configuration and explicitly validate
products/results against the native domain. Do not silently widen or saturate
legacy behavior while claiming an extraction.

`SUB_MONEY` first refuses nonpositive amounts, active accounting, insufficient
total cash and nonzero mode (`src/core/utility.c:3301`). For a real PC/PID it
submits a generic `wallet_spend` with reason_id0 and an ordinary currency callback,
returning0 on accepted submission (`src/core/utility.c:3313`). The generated
currency operation is not returned/stored as the blackjack opening operation.
The callback reports failure but does not roll back table phase or bind a round
(`src/core/utility.c:3270`). Table POSTBID/bet assignment therefore must not be
described as proof of a committed wager. Non-PC fallback mutates denominations
directly, including change (`src/core/utility.c:3321`); it supplies no real player
or round identity and is not a fixture shortcut for a supported route.

The real PC debit takes the total wallet value, canonicalizes the remainder and
subtracts the original counts (`src/economy/currency_transaction.c:1037`,
`src/economy/currency_transaction.c:1515`). This can change denominations other
than the selected wager denomination. For example, owned current counts
`[10,10,0,0]` and a five-silver wager have total110, spend50, canonical remainder
`[0,6,0,0]`, delta`[-10,-4,0,0]`. A one-denomination stake plan cannot pretend this
was wallet delta`[0,-5,0,0]`. Resolve the original debit/denomination contract
before connecting an active round. Reuse R2 and the existing currency authority;
this map does not choose a new conversion policy or ledger.

The public ordinary currency route owns generated operation IDs, expected wallet/
bank revisions, fences, frozen intent and retained submission
(`src/economy/currency_transaction.c:1368`, `src/economy/currency_transaction.c:1420`).
Its save/publication/ACK machinery remains that owner's capability; the blackjack
caller has not joined a compound round participant. Direct payout cash writes
in this table do not invoke that machinery or mark a gambling completion.

`Card` IDs are1..52, with suits in groups of13; the constructor clamps out-of-range
values (`src/economy/cardgames.h:26`). `Hand::BlackjackValue` maps rank2..10 to its
number, faces to10, counts aces as1 and promotes one ace by10 only when
`sum + aces < 12` (`src/economy/cardgames.c:71`). This is the reusable deterministic
rule. `Hand::ReceiveCard` rewires the new card into a linked list and `DealACard`
removes from the deck, so these are ownership mutations, not pure capture
(`src/economy/cardgames.h:86`, `src/economy/cardgames.c:210`).

Deck construction produces a linked1..52 order. Shuffle cuts the current deck and
rebuilds it using `number(1,100)` and the live
`cardgames.shuffle.percentage` property, default50, on each iteration
(`src/economy/cardgames.c:108`, `src/economy/cardgames.c:123`,
`src/economy/cardgames.c:157`). `DealACard` does not decrement `numCards`; do not
derive a trustworthy remaining-card count from that field after draws. Seven
shuffles are hardcoded at deal. Retry must retain the original consumed card IDs,
remaining order/draw position and effective rules/outcome from its original
admission owner; it must not reshuffle, consult current configuration, advance RNG
again, or substitute scores for the original card evidence. No such retained
round image is implemented by this table. Ordinary item snapshot capture copies
timer values at `src/player/player_snapshot_capture.c:422`; those scalar copies
are not a deck/hand codec or a valid way to restore the pointer-valued round.

## Existing accounting capability and its limits

| Existing value/schema capability | What it establishes; missing authority |
| --- | --- |
| `economic_account_kind::gambling_stake=11` (`src/economy/economic_accounting_types.h:34`) | Intended finite account keyed by table UID plus durable sequence; treated as ordinary for balance/history (`src/economy/economic_accounting_types.c:54`). General key validation requires nonzero authority ID, not proof that it is a live table. |
| `economic_source_event` kind6/source/generation/sequence/slot (`src/economy/economic_source_event.h:20`, `src/economy/economic_source_event.h:43`) | Existing48-byte owned codec; nonzero source/generation and valid kind required (`src/economy/economic_source_event.c:52`). It allocates no source and proves no original round. Generic codec permits sequence/slot extremes; the gambling plan adds stricter shape. |
| Gambling reason policy (`src/economy/economic_accounting_plan.h:30`, `src/economy/economic_accounting_plan.h:57`, `src/economy/economic_accounting_plan.c:44`, `src/economy/economic_accounting_plan.c:132`) | Stake18, payout19, loss45, interruption46 require domain actor and round source; terminal reasons require an original operation. Metadata checks require nonzero identities and forbid self-linking (`src/economy/economic_accounting_plan.c:485`). They do not authenticate actor/table/round linkage. |
| `gambling_round_structure` (`src/economy/economic_accounting_plan.c:413`) | Opening slot0; terminals slot1; no child/item effects; exactly one stake with nonzero context matching source sequence. Opening starts stake at zero, terminal empties it. Positive stake in exactly one denomination, all postings in that denomination. Stake/interruption pair wallet+stake; loss pair sink+stake; payout wallet+stake with optional issuance. If issuance exists it is exactly one posting for negative stake. This supports ordinary1:1 net wins and pushes, not natural3:2 or a second stake/double transition. |
| SQL evidence tables (`migrations/economy_accounting.sql:119`, `migrations/economy_accounting.sql:141`, `migrations/economy_accounting.sql:219`) | Existing account effects, exact denomination postings and unique lineage/source claim can record evidence. They do not supply native participant lifetime, current wallet/stake lock, cards or a gambling command producer. |
| Published SQL reason constraint (`migrations/economy_accounting.sql:100`, `migrations/immutable/0031_economy_accounting.sql:99`) | Both restrict operation reason to1..42. Inspection found no later published immutable migration changing that constraint. Thus C++ loss45/interruption46 validation is not proof of SQL compatibility for those reasons. Leave schema unchanged; an admitted-route owner must resolve this precise dependency before qualification. |
| Persistent mapping constraint (`migrations/economy_accounting.sql:51`) | Maps account kinds1..6 only. Kind11 is not a new persistent native mapping category. A round's retained holding/origin/retirement evidence is a separate requirement, not permission to widen mappings. |

Manual accounting expectations, already represented by existing tests, are:
wallet ten silver -> wager five -> wallet five, held five; ordinary win returns
ten, ending wallet fifteen/held zero with issuance negative five; push/interruption
returns five, ending wallet ten/held zero with no issuance; loss empties held five
to a positive-five sink and does not credit wallet. These are exact silver legs,
not just equal copper totals. An interruption refund in the plan is not an
implemented logout/table-removal/activation refund. Terminal original-operation
and source-slot linkage must remain retained through retry; a new terminal ID
does not permit consuming the same held round again.

Reuse the inspected tests, without claiming they ran during this design task:

- `tests/async/economic_accounting_plan_test.cpp:328` covers stake, push, win,
  loss and interruption, missing original link, wrong slot/context, multiple
  stakes, treasury substitution, denomination change and excess issuance.
- `tests/async/test_economy_accounting_contract.py:42` and
  `docs/persistence/economy_accounting/golden.json:1201` check stake return plus
  net winnings and require a committed original operation. Exact replay/conflict
  checks at `tests/async/test_economy_accounting_contract.py:50` are generic
  contract behavior, not round admission/recovery.
- `tests/async/test_economy_writer_coverage_contract.py:328` checks source ordering
  for active wager/pending-payout refusal; its adjacent magic-deck check is a
  different synchronous source contract. Neither executes the real table route.
- `tests/async/test_reconcile_economy_accounting.py:359` explicitly labels the
  finite-round fixture as having no game producer/activation. Tests at
  `tests/async/test_reconcile_economy_accounting.py:1096` cover kind11, opening/
  terminal holdings, native/history/posting disagreement, missing origins,
  retirement and bounded operator views. `native_stake_sql` at
  `tests/async/test_reconcile_economy_accounting.py:3367` uses genuine plan/source
  codecs and disposable SQL evidence, not blackjack gameplay.
- `scripts/reconcile_economy_accounting.py:37` includes kind11 as ordinary while
  keeping mapped kinds1..6. Its receipt/source/original-link checks at
  `scripts/reconcile_economy_accounting.py:664` inspect supplied quiescent evidence;
  they do not create or authorize it. The golden validator's original-receipt
  check at `scripts/validate_economy_accounting.py:135` is another existing
  contract layer, not a new round backend.

## Interruption, cleanup and publication owners

Normal terminal reset destroys deck/hands and clears table fields. Object
extraction disarms table-owned events and calls `free_obj`
(`src/world/handler.c:3505`, `src/world/handler.c:3544`); `free_obj` also disarms
events, frees ordinary object payload and releases object memory
(`src/world/db.c:8214`). Neither inspected function interprets the blackjack timer
pointers, closes a round or refunds a stake. This is a source gap, not a runtime
claim of a measured leak. Character extraction disarms character-linked events
(`src/world/handler.c:5821`), but dealer events were scheduled with null character
and the table as owner. `disarm_char_nevents` walks character events and
`disarm_obj_nevents` walks object events (`src/world/new_events.c:845`). They do
not discover `Hand::owner` hidden behind a timer.

Consequently a future owner must explicitly retain/authenticate participant PID
and runtime lifetime, table UID/lifetime, round generation/sequence and the
original wallet/stake command. It needs an interruption disposition for quit,
death, table removal, reset/reinitialization, activation, shutdown and lost
completion. Current code does not prove those outcomes or stop a stale hand owner
from being read. Never convert the raw owner pointer into a saved token.

Current currency completion, player save exclusion and physical publication/ACK
remain with existing transaction/save/coordinator owners, as mapped in the
currency document. The public table has no join that retains card state and stake
with those owners. A supported route must commit the original compound economic
outcome, publish it once to the authenticated current native round and wallet,
keep its save hold until guarded ACK, and restore the same retained outcome cold.
This states needed ownership, not a proposed new coordinator, ledger or wire
format. No public gambling command/retained-round/publisher/cold driver was found
in the inspected route and tests. Private primary birth/publication reports are
not gambling ownership evidence.

## One proposed independent acceptance slice

**Selected: genuine hand-score/card-ownership acceptance only.** The original
caller connection is direct: hit bust and periodic dealer draw/terminal comparison
call `Hand::BlackjackValue`; its score also appears in deal/stay/showgame output.
Card/ace changes would otherwise change economic outcomes before any accounting
adapter could detect them. Existing gambling tests exercise monetary structure
and source refusal, not this production scoring body. No extraction of currency,
stake arithmetic or the entire blackjack special is useful for this slice.

Proposed owned paths, requiring a separate implementation reservation:

- `tests/async/test_blackjack_hand_native_values.py`: one focused runner with
  explicit candidate/output paths, bounded commands and retained artifacts.
- `tests/async/blackjack_hand_native_values_harness.cpp`: manually specified
  card IDs, score/count expectations and genuine lifecycle operations.

No production path changes are proposed. Compile the **complete** frozen
`src/economy/cardgames.c` as C++20 and include its actual `cardgames.h`/transitive
production headers; call the real `Card` constructors, `Hand::ReceiveCard`,
`BlackjackValue`, `numCards`, `Fold` and destructors. The selected score body has
no outgoing RNG/config/authority call. Standard C++ allocation/deallocation and
sanitizer runtime are the needed live external providers. Use function/data
sections and linker collection to exclude the uncalled table, deck shuffle,
diagnostics, currency, authority and event paths. No copied body, accessor trick,
replacement hand, RNG sequence, accepting money/coordinator stub or synthetic
player is needed. If the whole-TU link exposes a live dependency, retain the
failure and inspect the genuine provider; do not replace it with an accepting
double or quietly broaden the owned paths.

The future proof must pin actual compiler/tools, direct bodies, `-MD` header
closure, linker map, ELF and libraries; verify the real `Hand::BlackjackValue`
symbol and absent `blackjack_table`/shuffle/settlement execution. This is the
required provider recipe, not an already compiled/linked closure. Compiler and
runtime feasibility remain **UNEXECUTED** in this design delivery.

| Independent manual fixture | Expected score/count |
| --- | --- |
| Empty hand; card1 (ace); card13 (king) separately | 0/0; 11/1; 10/1 |
| IDs11,12,13 tested individually in each suit | Each face is10; no suit changes rank. |
| `[1,9]`, then add10 | 20/2, then20/3: soft hand becomes hard. |
| `[1,10]`; `[1,14]`; `[1,14,9]`; `[1,14,10]` | 21/2; 12/2; 21/3; 12/3. No natural payout claim. |
| `[1,14,27,40,9]` | 13/5: four distinct aces plus nine. |
| `[10,23,5]`; `[2,3,4,5,6]` | 25/3 bust score; 20/5, with no automatic five-card win. |
| Every legal ID1..52, with explicit expected rank table; distinct-card insertion permutations | Rank/suit mapping and order-independent score/count; no oracle copied from the implementation. |

Use only genuine allocated cards with IDs1..52 and one owning hand at a time.
Track allocations independently for audit; do not mutate protected card links,
forge a cycle, call `ReceiveCard(nullptr)`, duplicate an owning pointer or pass a
fake participant to make setup work. Repeated score/count calls must leave count,
score and tracked allocation ownership unchanged. Fold transfers the original
tracked cards out and leaves an empty hand; transfer those known card pointers
through public `ReceiveCard` into a second real hand, then let its destructor
release them once. Keep another hand alive as a no-cross-hand-change control.
ASan/leak detection and UBSan cover these legal ownership transitions; allocation
failure and malformed-pointer behavior are outside the selected contract.

The legal schedule is construct -> append one real card -> check manual prefix
score/count -> repeat observation -> append remaining cards -> Fold -> verify
empty -> transfer tracked cards -> verify same manual score/count -> destroy.
There is no event ordering, delayed payout or native process schedule in this
slice. Future strict `-O1` and `-Og` ASan/UBSan builds should bound compiler/tool
probes at20s, each compilation at900s, link at120s and each runtime control at30s,
with a bounded outer runner and retained nonzero/timeout partial logs. Use new
task-specific D: scratch/bin directories and the existing logging pattern from
the accepted Collector runner; do not edit that closed runner or rerun its cases.

This score slice has no durable write at which to inject a receipt/ACK/crash cut.
For the later **dependent** round journey, required cuts are: before/after original
stake admission and commit; after cards/config are frozen but before publication;
between periodic callbacks; after terminal commit but before native wallet/table
publication; during interruption; after native publish but before guarded ACK;
and cold replay with absent/reused participant or removed table. Expectations must
be original-ID replay, unchanged frozen cards, exactly one stake disposition,
no second debit/issuance/refund, preserved save hold on failure and authenticated
current-world publication. Those are dependency requirements, not manufactured
cases in the proposed hand-score harness.

## Bounded design review and remaining dependencies

The source/design review checks the early active guard, complete command branches,
all actual money writes, real caller/scheduling/cleanup edges, accounting shape,
SQL compatibility limits, and existing test scope. All file blobs, explicit
line anchors and document links are checked against frozen inputs. Private
`D:\Dev\Temp\gambling-authority-20261008` retains source/reference pins,
design-review results and a hashed evidence index. The final diff is this one
owned document. No test or compiler execution is implied by these checks.

The bounded private `python D:\Dev\Temp\gambling-authority-20261008\review_driver.py`
review passes under a 120-second deadline: 3,069 original published blobs/files,
12 separate reference files, 94 explicit anchor occurrences across 31 source paths,
38 appendix body pins, six relative links, the complete seven-keyword command
scan, absence of an existing published hand-score test, and the sole staged path
with `git diff --cached --check`. Logs and command/return-code records are retained.

Proceed independently only with the proposed score acceptance after its bounded
reservation. Active conversion still depends on an original round admission and
source issuer; a real retained participant/table/current-state owner; frozen
cards/RNG/configuration; compatible exact debit/stake and terminal policies;
authenticated locked wallet/stake/origin/retirement evidence; compatible SQL/flat
transaction ownership; save exclusion, original retained publication and guarded
ACK; and a genuine cold recovery/driver fixture. The unavailable private primary
and exact439 native inputs stay with their owners. They block their dependent
native journeys, not this source map or the independent score proposal.

Design proposals remain proposals. This delivery neither enables wagering nor
closes any implementation, release or native qualification gate.

## Frozen-body authentication

| Scope | Path | Git blob |
| --- | --- | --- |
| published | `docs/persistence/economy_accounting/FINISH_ACCOUNTING_PLAN.md` | `f56e54c67c86b325031750e4b66289440fc95f18` |
| published | `docs/persistence/economy_accounting/golden.json` | `3bd40d638677447cf302eebaeaec5fd05a3b08b6` |
| published | `migrations/economy_accounting.sql` | `a1d65bc8dc54d4426b6a8e3f7687636fd75fd7db` |
| published | `migrations/immutable/0031_economy_accounting.sql` | `45656ea974b7d9520cff5da60610d9c41abd4f12` |
| published | `scripts/reconcile_economy_accounting.py` | `e8b02432ef2b0cede6f8ad488a8309f05437ad4a` |
| published | `scripts/validate_economy_accounting.py` | `cfca2680bd57cbfe036d1166312b93b72f46c455` |
| published | `src/cmd/interp.c` | `74281b6f20ccd88a588d63cdda143da275258e85` |
| published | `src/core/structs.h` | `5ba85524e8ceee7ae085fc6637b501d528a41fe0` |
| published | `src/core/utility.c` | `92bdffd57f8979d9531251d1b049887505d7715d` |
| published | `src/economy/cardgames.c` | `ecc5178fdbea9e739fbf30ff613ad93e4f0f2bfb` |
| published | `src/economy/cardgames.h` | `0e92f3ee29b04da68ef0757cd6fac75de074183f` |
| published | `src/economy/currency_transaction.c` | `7366935cbf829554395f739f7af7b32ba303558f` |
| owned | `src/economy/currency_value_plan.h` | `4d7a6f8146ffc05cb3db5a61536c7bae268166e3` |
| published | `src/economy/economic_accounting_plan.c` | `8204617fcf1ea5f322f06897f9c5c8848f9aa38b` |
| published | `src/economy/economic_accounting_plan.h` | `9e99a4c42936fc96be163d557c05765f91893783` |
| published | `src/economy/economic_accounting_types.c` | `f1ada31fc487e5649bda982d30482ace1e6ce009` |
| published | `src/economy/economic_accounting_types.h` | `59b66a3ab4ab22fea65bedaae607ec8021d94868` |
| published | `src/economy/economic_gameplay_authority.c` | `4afa41bdb718068e4862f422b701fa5ad6953d56` |
| published | `src/economy/economic_source_event.c` | `f91b9c3eb94dfd2900dfa39e97391cad7966f301` |
| published | `src/economy/economic_source_event.h` | `588d21cd6369404994988e56c96d7ca0f9388c31` |
| published | `src/item/objmisc.c` | `e3b467d6cf223e4c9d28bf86278870e8060650e0` |
| published | `src/player/player_snapshot_capture.c` | `9aacf75b74b45f1d2f0911e7325b9d7a3125921b` |
| published | `src/specs/specs.assign.c` | `a0f1724469d4dd4c72d276ace11e03127fd3738d` |
| published | `src/specs/specs.gellz.c` | `bc298f33d6202e5088c8e9bde574121b993e0e87` |
| published | `src/world/db.c` | `da996337d17da9a0f81016e3f4c077405fd4fdee` |
| published | `src/world/handler.c` | `0054c7db2f7cb990dfde047e6de09250d54670e3` |
| published | `src/world/new_events.c` | `4bab396e1c0f60b080d7ac1f17b0fbb801be3815` |
| published | `tests/async/economic_accounting_plan_test.cpp` | `e46a589d07de085a2e5d9ff08970516e3b67b5ae` |
| published | `tests/async/test_economy_accounting_contract.py` | `441411cefe11b2d9aabea007c88a8845f531f4ce` |
| published | `tests/async/test_economy_writer_coverage_contract.py` | `9e95959569cb6a8a271880db8d84fb4c2fd5b53a` |
| published | `tests/async/test_reconcile_economy_accounting.py` | `ff40f81c3498ad41d173bf4ab45ebde1ff48f449` |
| published | `docs/persistence/economy_accounting/registry.json` | `f3797ec71cd79c7f24fb0f3c015af6ef5ad39754` |
| owned | `docs/persistence/economy_accounting/domain-separation/R2_RESERVATION.md` | `57ffc720ee8ade963017284ece3c0709daa04f82` |
| owned | `docs/persistence/economy_accounting/domain-separation/HANDOFF.md` | `493abc0e785f339acf687b0bb347f79454bac373` |
| owned | `docs/persistence/economy_accounting/domain-separation/OPERATION_INVENTORY.md` | `e1761f0dde821d281a9fd24618d932234d192f90` |
| owned | `docs/persistence/economy_accounting/domain-separation/POST_R14_OWNER_DEPENDENCIES_2026-10-07.md` | `c32aad66519191ab2530b0f2b1c7e14968822753` |
| owned | `docs/persistence/economy_accounting/domain-separation/CURRENCY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md` | `d3d1caab73fe706bdd7fc0e912fd7a2f83ede65e` |
| owned | `docs/persistence/economy_accounting/domain-separation/COLLECTOR_COLLECTION_NATIVE_CAPTURE_HANDOFF_2026-10-08.md` | `4165442aff2a2f70a57a98d5934ac1a8fade508d` |
