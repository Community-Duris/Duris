# World-quest service authority boundary — 2026-10-08

QP04 failed-service restitution and QP07 original-attempt protection remain
separate requirements. The published bartender path captures a fee and giver,
then invokes mutable world-quest state after settlement. Its generic active-mode
debit is currently refused before admission; its generic refund helper also
refuses active-mode credit. A guarded callback alone cannot establish a durable
service or refund. This document reserves one conditional future acceptance
slice, **original map debit → cross-actor replacement B → original restitution**.
It adds no implementation, executed journey or new accounting authority.

## Pins, scope and reused evidence

| Input | Exact revision / disposition |
| --- | --- |
| Public accounting source inspected | `257190ac149a86af59b1c3c2fe321abb382cd8ef` |
| Previous public pin | `fb641bb56ae719758756d4bd1a575df48fbe5ef7`; intervening changes are documentation only |
| Public source tree | `833d3085815b396861ad18a77635412212381e4b` |
| Public migrations tree | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Preserved prep parent | `be14bedc0627e82fa235025d22636cd94ec5a9f4` |
| Existing QP07 prep correction | `fd997ee4147ba58d835bf4bd61783b51307bc68c`; map/abandon attempt guard, not the public callback below |
| Native producer research | PR678 `55905eac1906cf59405764407f9d22497cccfff3`; current facts checked against the public pin |
| Coordinator-reported private flat candidate | `06f8c714cad027bd34aae137411e14b9dcf15f1a2460b8030504dc2994e75b3e`; reported136 production/73C, bodies unavailable here |

Private-candidate source review does not establish adoption, APIs, build, SQL,
native gameplay, recovery or qualification. Those executions remain deferred to
the agreed major-plan batch. No public production change is inferred from a
documentation advance. Prep source and public source are deliberately distinct.

Reuse [CURRENT_BARTENDER_EXECUTION.md](CURRENT_BARTENDER_EXECUTION.md),
[CREATION_WATERMARK_REVIEW.md](CREATION_WATERMARK_REVIEW.md),
[CONTINUING_RECONCILIATION.md](CONTINUING_RECONCILIATION.md) and
[HANDOFF.md](HANDOFF.md). The earlier full-boot Woodseer legacy journey proved
ordinary creation/abandonment on its own source/binary; its tasks were mapless.
It supplies no map debit, held callback, active epoch, restitution or cold-service
proof. The earlier creation review already covers injected late creation and
configured share/reset reachability. Neither experiment is rerun or promoted
here. Existing modeled fee, capture and mapping joins remain at their recorded
component/reader scopes.

Only this new document is owned by this delivery. Its publication SHA is the
containing commit, obtained with:

```powershell
git log -1 --format=%H -- docs/persistence/economy_accounting/quest-prep/WORLD_QUEST_SERVICE_AUTHORITY_BOUNDARY_2026-10-08.md
```

## Producer, dispatch and full-boot/reset binding

These are dynamic world quests, absent from static recipe catalog output. Their
actual producer is `createQuestForGiverVnum`; the caller is the settled bartender
creation callback, with `createQuest` supplying a wrapper. No independent gameplay
caller of that wrapper was found in the inspected public source. Static recipe
ingredients/rewards do not define these runtime tasks. [Creation producer][create]

Production `areas/AREA` includes Quietus and Woodseer. Real zone reset records
spawn Quietus keeper1709 in room1734 and Woodseer keeper16553 in room16633;
their corresponding shop records identify those keepers/rooms. These VNUMs and
spawn commands are source facts, not authenticated native birth/UID/cash custody.
[Quietus zone][qzone], [Woodseer zone][wzone], [Quietus shop][qshop],
[Woodseer shop][wshop]

Full boot runs `assign_mobiles`, then `assign_objects`, then `assign_rooms` when
specials are enabled. Woodseer's `world_quest` assignment occurs in
`assign_mobiles`, before its final shop wrapping; `assign_the_shopkeepers` saves
the prior special in `SHOP_FUNC` and installs `shop_keeper`. That wrapper calls
the saved special for ASK. **Quietus1709 is assigned `world_quest` inside the later
`assign_objects`**, after wrapping. Thus the source-derived default boot paths
are Woodseer `shop_keeper → SHOP_FUNC(world_quest)` and Quietus's late direct
`world_quest` assignment. This distinction needs observation in a future native
fixture; it is not a newly executed boot. [Boot order][boot],
[Woodseer assignment][wassign], [shop wrapping][wrap], [shop dispatch][shopdispatch],
[Quietus late assignment][qassign]

`reset_zone` first calls the native birth reset gate when accounting is active.
Its M branch uses `quest_mobile_native_birth_owner::prepare_mobile` for active
regular SQL, and `read_mobile` otherwise; reset sealing/finish remain with that
birth owner. The published preparer generates original reset/birth identities,
captures the real constructor/runtime body and marks a staged birth blocked when
its room has a hook or its ACT_SPEC mobile has a special. This is a conditional
source predicate, not an executed bartender birth refusal or a license to strip
the special. Existing birth ownership does not retain this player service's
paid attempt. [Native reset gate][resetgate], [Native birth preparer][birthprepare]

Policy caches are initialized through
`calc_zone_mob_level → world_quest_policy_bootstrap`, after world loading. Neither
a prototype special nor a zone reset authenticates an original native lifetime,
original financial receipt or retirement. [Zone reset][resetzone],
[Policy bootstrap][bootstrap]

## What the current service captures and what remains runtime state

The public `world_quest_payment_context` contains action, int32 fee and int32
giver VNUM: 12-byte layout in the existing ABI, bounded by the currency owner's
64-byte copied context. It contains no original task watermark, actor PID/runtime
body, operation ID, request time, target or selected outcome. The public callback
ignores `currency_command_result`. Currency owns the admitted identity, copied
context, receipt interpretation and balance publication; these must not be
reconstructed from bartender text. [Context/callback][context], [Currency owner][currency]

| Action | Request-time capture | Settlement/runtime work and evidence boundary |
| --- | --- | --- |
| Creation | Nonnegative integer configured cost20×current level, then `difficulty_scale_world_quest_fee`; fee/giver copied before debit | Callback rechecks current active/completed/quota state, then selects zone/type/target against runtime eligibility/population/history. Task level and start use callback-time state. Failure invokes generic refund. |
| Map | Fee10×current level; active task, available map and not-bought request checks | Public callback only checks current active==1 and not-bought; it calls `quest_buy_map` on current task. That function sets bought before finding/rendering the map, updates room/GMCP when found, and creates no item root. |
| Abandonment | Base int(configured0.25×current level³), scaled by remaining fraction of configured24h, bounded1..100 percent with existing float/int truncation; fee captured after quote | Partial-kill progress requires confirmation. Public callback checks active/not-completed; records history only for FIND_AND_KILL with progress>0, then resets/GMCP. No generic history row is required for zero-progress abandonment. |

The configured defaults differ from fallback literals in some getters; preserve
the actual arithmetic and quote. Creation's difficulty scaling is not a blanket
map/abandon multiplier. Abandonment's `quest_started` is both a time input and
generation watermark, so fixture time matters. At level11/default creation
multiplier1, creation is C220 and map C110; retain actual quoted/admitted values
rather than importing a level56 Woodseer fee. [Fees/requests][fees],
[Map request][maprequest], [Creation request][createrequest], [Properties][properties],
[Difficulty fee][difficulty], [Map effect][mapeffect]

Creation uses boot-cached eligible zones/prototypes/rewards, weighted/random
selection, bounded32 candidate/history probes, current population (kill≥2;
ask exactly1), speech/visibility and actor-dependent aggression checks. Above49
it chooses kill only; below41 it rejects mapless zones. History read failure
stops selection. The boot policy excludes static quest inputs/rewards from the
dynamic reward pool. A target and reward VNUM cannot be prescribed as an invented
recipe; record the actual selected zone/type/target/count and later actual reward
VNUM/UID/quantity. [Creation][create], [Policy][policy], [Policy limits][limits]

`world_quest_next_started` strictly advances a valid positive int watermark,
including same-second/clock rollback, and refuses exhaustion. `resetQuest` clears
task/progress/map/shares while preserving that watermark. Sharing advances the
recipient watermark and copies task data; it does not copy map flags. The existing
QP07 prep correction captures/validates this watermark for map/abandon, including
completed/reset/replacement cases; it is not present in the pinned public
12-byte context. It does not supply receipt ownership or durable restitution.
No additional guard is implemented or component claim rerun here. [Attempt/reset][attempt],
[Share installation][shareinstall]

## Busy gate and the conditional cross-actor schedule

`CMD_ASK` is currency-dependent in the input classifier; `CMD_QUEST` is absent
from that classifier. The playing queue uses the player's currency busy state
alongside other owner holds. A second ASK by the same payer therefore cannot be
used as an unconstrained replacement while its first payment is busy. Currency
busy includes same PID or same account/racewar; use genuinely separate test
accounts for other actors. [Input classifier][input], [Playing queue][queue],
[Currency busy][busy]

Default `world.quest.share.max=0.000`, with getter default0 and clamp0..4,
immediately refuses sharing. The proposed schedule is **unavailable in the
unmodified default fresh world**. Configuring a positive limit is an explicit
isolated-fixture condition, not default acceptance. It must precede donor quest
creation, because that installs the donor's shares-left count. [Defaults][properties],
[Share checks][share]

With that configuration, actual donor D must own its receiver PID, have shares
remaining and an unfinished active task. Recipient P must be visible through
`ParseTarget`, inactive, already consenting D, within configured level bounds,
eligible by daily quota and without the donor target in history. No recipient
currency-busy test occurs in this share block. Trusted actor S may execute real
`quest reset P` before the donor's share; that branch precedes the actor's active
task gate. Other actors' commands do not enter P's deferred ASK queue. This is a
source-supported conditional command order, **not proof that a current native
owner can delay and resume the original callback across it**. [Share][share],
[Trusted reset][trustedreset]

## Existing owners and unresolved durability boundaries

| Responsibility | Published owner / behavior | Acceptance boundary |
| --- | --- | --- |
| Admit and settle service money | Bartender submits negative `wallet_spend`, giver reason ID, command/interactive. `currency_transaction` captures wallet deltas, generated operation ID, revisions, copied context and receipt publication. | Active `prepare_currency` only admits its enumerated ATM/chaos or specifically recovery-sourced quest-wallet reward routes; this bartender spend returns `incomplete_coverage` before admission. Static NPC quest-cost reason44 and native wallet context12 do not authorize this player service. [Authority gate][authority] |
| Preserve original payment continuation | Pending map is keyed by original operation; unresolved/ambiguous publication retains continuation. Live account-bank callback resolves a player by PID, without an original runtime-body fence in this service context. | Same PID is not proof of the original character body or task. Disconnection/reconnect must use authenticated owner lifetime evidence. Coin-endpoint runtime arrays cannot be borrowed as bartender authority. [Currency pending/publication][currency] |
| ACK service effects | When publication is required, currency acknowledges publication, extracts its pending entry, then invokes the void business callback. | That ACK does not prove task/history/refund/save completion. A failed business continuation has no retained callback through this call site. [ACK order][ack] |
| Failed/stale restitution | Bartender prints refund prose and calls `ADD_MONEY(fee)`; generic active positive credit refuses. Legacy helper submits a separate `wallet_reward`, reason_id0, with fallback behavior. | No original debit-linked refund identity or durable outstanding obligation is established. A second credit ID or balance coincidence is insufficient; no new refund owner is proposed. [Refund][refund], [Generic credit][credit] |
| Complete task and grant rewards | Real `do_ask → quest_ask`; combat killer/group paths call `quest_kill`. Active item grant carries source `(PID<<32)\|quest_started`, actor/target and actual reward UID; successful callback verifies source/target and published carried item before XP/epic/history/reset. | Target completion and bartender service are different continuations. Active mercenary>24 compound coin+item reward is refused. Legacy random mercenary coins, XP and reward are not active compound proof. [ASK caller][askcaller], [Kill callers][killcaller], [Reward owner][reward] |
| Publish created reward item | `item_creation_grant_submit_to_player_with_completion` and item movement owner retain actual item identity through publication. | Item custody receipt/publication is distinct from quest XP/history/reset and from bartender finance. Do not infer business-context cold serialization or borrow unrelated craft/drop ACK ordering. [Item creation owner][itemowner] |
| History and task save | SQL history records player/time/giver/level/target/reward; flat history has its own persisted records. Snapshot/load preserve fourteen task fields including start/map state. | History has no original service operation linkage. A task checkpoint alone does not retain original fee/receipt/callback. Existing unrelated save holds do not establish this service's hold. [SQL history][history], [Flat history][flathistory], [Task capture][capture], [Task load][load] |
| Replay/cold and retirement | Currency's schema2 replay restoration has specialized coin handling and account-bank restoration restricted to ATM, with null callback/context. | No dynamic bartender service-context reconstruction is provided there. Reset/abandon is player-task cleanup, not native NPC D, stock retirement, original parent/child retirement or a durable service terminal ACK. Private candidates cannot close this gap without published owner contracts and executed evidence. [Replay restoration][replay] |

Creation can fail after a committed debit because eligible targets changed; a
giver can disappear while only its VNUM remains captured. Neither condition
authorizes manufacturing a native birth binding, target, receipt, refund or
successful acknowledgement. QP04 still requires the original financial
obligation to survive until genuine restitution or an explicit durable unresolved
state. This also applies when a stale-attempt guard correctly protects B.

## One future acceptance reservation: stale map restitution after real share

Proposed owned runner, **not added or executable by this delivery**:
`tests/async/quest_accounting_prep/run_world_quest_stale_map_restitution.py`.
Use the maintained dual-backend world-quest setup and command/session patterns,
and existing owned SELECT-only capture/assertions only when their authentic
runtime prerequisites are available. No general service facade, copied codec,
new refund owner or forced quest state is reserved. This slice extends the
previous QP07 component guard into real financial/publication acceptance; it
does not duplicate the earlier late-creation/reset review.

Required primary-owned contracts, **requirements rather than asserted APIs**:

1. A supported genuine service debit/activation route, authentic player and NPC
   lifetime/source setup and original receipt/context observation. Current
   active generic `wallet_spend` cannot provide it. Preserve the real special
   and room hooks; the public birth preparer's conditional ACT_SPEC/hook block
   requires an authentic compatible owner route, not fixture flag removal.
2. An original-operation delay/release export at settlement or notification,
   preserving owner context/receipt while game pulses and other actors run.
   A direct callback call, forced busy flag or sleeping fake debit is invalid.
3. Original service attempt/PID/runtime lifetime and fee retained through task
   effect, failed/stale restitution, save/ACK/replay/cold. Publish compatible
   original-attempt protection; prep-guard existence is not primary adoption.
4. Existing financial owner support for exact debit-linked compensation or an
   explicit durable held-service obligation, with original identity retained
   across lost replies. Export exact money projections, checkpoint/ACK/hold and
   terminal-retirement evidence. An outstanding obligation is a blocked result,
   not a completed refund PASS.

Execute only after those prerequisites become genuine, at the major-candidate
qualification boundary, with distinct isolated SQL/flatfile resources and exact
source/binary/schema/backend pins:

1. Explicitly configure `0 < world.quest.share.max <= 4` before real donor D's
   task creation. Use separate accounts for P, D and trusted S. Establish actual
   consent, matching levels, quota/history eligibility and visibility through
   normal supported commands. Choose production Quietus1709/room1734, P level11,
   recording the actual full-boot dispatch and native lifetime if active. Obtain
   A and donor task via real bartender ASK; observe target/type/count. Require
   A's map to be available and not purchased. If unavailable, record the missing
   fixture precondition; do not set map/target fields to force a run.
2. P issues real `ask <observed bartender name> map`. Capture request level,
   actual fee (default C110), giver VNUM, original watermark A, PID/runtime body,
   original admitted operation, receipt and exact wallet before/after. Hold only
   that genuine original continuation through the owner export. No second P ASK
   is used to manufacture replacement.
3. S issues actual `quest reset <P>` while original money is pending. Capture A's
   inactive cleared task and retained watermark. D issues actual
   `quest share <P>`. Capture success, donor shares decrement, recipient B with
   strictly newer watermark, copied donor receiver/target/type/count and map0.
   Keep B active; a second reset would duplicate the earlier creation scenario.
4. Release the original receipt/continuation through its owner. The guarded
   service must leave **every B task/map/progress field unchanged**, with no
   history, XP, reward item or GMCP map publication attributable to A. Public
   unguarded source permits `quest_buy_map` against B; this is a prediction,
   not a newly reproduced native failure. Observe the original debit exactly
   once and actual original-linked restitution exactly once, or retain an
   explicitly durable unresolved obligation and report BLOCKED.
5. If committed debit is returned, require net copper service value0 and exact
   owner-defined financial vectors/revisions; do not require the original coin
   denominations to reappear if the maintained wallet projection canonicalizes
   them. Original operation/attempt IDs must survive replay/lost reply/cold;
   replacement B stays unchanged and no extra map/debit/refund is created.
   Retirement is eligible only after both financial resolution and required
   service/task checkpoint/ACK evidence are complete.

Fault cuts belong to this single slice: genuine rejection before admission
(no debit, effect or compensating credit); committed original before callback
with B present; restitution prepared/committed with reply lost; and supported
save/ACK/cold boundaries. Exercise each only through authentic exports. A control
with unchanged A must buy its map once; mapless/already-bought request refuses
without charge. Default shares0, active recipient, absent consent, exhausted
shares/quota/history or same-actor queued ASK must remain distinct reachability
controls. Existing same-target/component controls are preserved rather than
rerun here. No arbitrary injection is promoted into native reachability.

Required evidence is the actual command timeline and actor/keeper lifetime;
full A→reset→B images; admitted original operation and retained context/receipt;
literal money/root/custody projections and refund/obligation identity; history/
XP/map deltas; physical publication; save/hold/ACK/replay/cold and retirement
disposition. Existing optional mapping joins report captured agreement only;
they cannot authenticate owner borrow, original source or complete world custody.

## Validation, remaining work and publication

Source/link/design checks only. Retained evidence root:
`D:\Dev\Temp\world-quest-service-boundary-20261008`. `source-manifest.json`
records exact Git blobs, SHA-256 and line counts for40 inspected public files.
`verify.py` authenticates them against Git, verifies public src/migrations tree
equality to the prior pin, validates immutable source anchors and local document
links, retains previous-record pins and checks this delivery's one-file scope.
Its review receipt records the explicit default/configured/active-route limits
and single-reservation nonoverlap; this is a self-review, not independent review.

Commands actually executed in the prep worktree, with Python bytecode disabled
and temporary storage on D:

```powershell
$env:TEMP='D:\Dev\Temp'
$env:TMP='D:\Dev\Temp'
$env:PYTHONDONTWRITEBYTECODE='1'
python -B D:\Dev\Temp\world-quest-service-boundary-20261008\pin.py
python -B D:\Dev\Temp\world-quest-service-boundary-20261008\verify.py
git diff --cached --check
```

Public blob/tree authentication, source anchors, local links and owned-file
scope: **PASS**:40 public bodies,44 immutable anchors, four existing record links
and seven bounded source predicate excerpts;49 retained artifacts are hashed by
`artifact-index.json` (index additional). No component test, compiler/build,
DB/client, server, journal,
native journey or unchanged broad batch was executed. No existing failure is
cleared by this document. The remaining blockers are the four exact owner
contracts above; default share configuration is separately unavailable and must
never be silently changed into default native coverage.

Publish this one-document bundle on `origin/codex/accounting-quest-prep` without
rewriting history. Canonical HANDOFF, shared production/tests/drivers, schema,
registries, finish plan and Plan5 remain untouched under this bounded assignment.
Reassess on published service/held-finance/lifecycle/recovery contracts or a
coherent integrated qualification candidate; private source-review counts and
documentation-only advances do not unlock this journey. Preserve earlier
deliveries and major-batch cadence. **The actual native Goal remains BLOCKED**;
this bounded source/design handoff does not resume or complete it.

[create]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/world/world_quest.c#L982
[qzone]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/areas/zon/quietus.zon#L234
[wzone]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/areas/zon/woodseer.zon#L715
[qshop]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/areas/shp/quietus.shp#L17
[wshop]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/areas/shp/woodseer.shp#L746
[boot]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/world/db.c#L689
[wassign]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/specs/specs.assign.c#L732
[wrap]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/economy/shop.c#L3602
[shopdispatch]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/economy/shop.c#L3033
[qassign]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/specs/specs.assign.c#L2225
[resetzone]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/world/db.c#L7215
[resetgate]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/world/db.c#L6938
[birthprepare]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/world/quest_mobile_native_birth.c#L642
[bootstrap]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/world/world_quest_policy.c#L428
[context]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/specs/specs.world_quest.c#L32
[currency]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/economy/currency_transaction.c#L58
[fees]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/specs/specs.world_quest.c#L241
[maprequest]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/specs/specs.world_quest.c#L321
[createrequest]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/specs/specs.world_quest.c#L405
[properties]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/lib/duris.properties#L2347
[difficulty]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/world/difficulty.c#L213
[mapeffect]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/world/world_quest.c#L1178
[policy]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/world/world_quest_policy.c#L346
[limits]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/world/world_quest_policy_math.h#L11
[attempt]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/world/world_quest.c#L119
[shareinstall]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/world/world_quest.c#L841
[input]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/cmd/interp.c#L1500
[queue]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/net/comm.c#L1520
[busy]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/economy/currency_transaction.c#L1183
[share]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/world/world_quest.c#L740
[trustedreset]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/world/world_quest.c#L663
[authority]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/economy/economic_gameplay_authority.c#L543
[ack]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/economy/currency_transaction.c#L902
[refund]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/specs/specs.world_quest.c#L72
[credit]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/core/utility.c#L3116
[askcaller]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/cmd/actcomm.c#L1407
[killcaller]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/combat/fight.c#L2276
[reward]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/world/world_quest.c#L236
[itemowner]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/item/item_movement_transaction.c#L1338
[history]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/sql/sql.c#L3343
[flathistory]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/flatfile/flatfile_world_quest_history.c#L220
[capture]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/player/player_snapshot_capture.c#L199
[load]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/player/player_load_materialize.c#L588
[replay]: https://github.com/Community-Duris/Duris/blob/257190ac149a86af59b1c3c2fe321abb382cd8ef/src/economy/currency_transaction.c#L1695
