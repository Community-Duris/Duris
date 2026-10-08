# The Halfcut Hills: comprehensive source story map

**Source-comprehensive, revision one — October 3, 2026. Gameplay qualification
remains open.** The [sidecar](../../../areas/story/halfcut.story.json) explains
all thirteen native deliveries as independent outcomes, with nineteen contacts,
all fifteen addressed dialogue families and twenty-four optional material or
producer-history checks. Thirteen potential daily candidates describe contract
shape, not guaranteed supply or safe payout: the drow names a missing reward.

**Actual native repair shipped:** the bound kobold crossbow ambusher now opts
into periodic scheduling without firing during setup, and safely interrupts
volleys when a target or the ambusher dies, moves or is removed. This is a
pair of separate `fix` commits, described below. The struck-player warning
also uses the correct audience. Native quest recipes and world data
remain unchanged. Missing reward, aliases and broader campaign work are pending.

Active, ready accounting is mandatory for discovery, encounters, journals and
new credit. All thirteen offerings contain items only; cash and experience
rewards are not fees. Optional producer history cannot replace spent material,
restore a retiring recipient, prove personal recovery or complete a campaign.

## Evidence and review boundary

Reviewed all twenty-eight [native blocks](../../../areas/qst/halfcut.qst):
twelve Q, one QA and fifteen M, with no ambient/default family. Every exact
contract is classified. The [reproducible source index](../../reference/zone-story-audits/halfcut.md)
records full bindings, addressed topics, prototypes and the literal assignment.
The active `areas/AREA` entry is `halfcut`; similarly named `halfcut_hills`
files are inactive and do not define this journal's current behavior.

Complete world review covers all 470 [rooms](../../../areas/wld/halfcut.wld),
27001–27470: 208 exact prose groups, 38 numeric headers, 48 non-exit metadata
groups and 242 exit families. Reviewed all 83 [mobiles](../../../areas/mob/halfcut.mob),
sixty [objects](../../../areas/obj/halfcut.obj), one six-stock
[shop](../../../areas/shp/halfcut.shp), and all 413 [resets](../../../areas/zon/halfcut.zon)
across 177 source families. Commands are 309 M, 42 D, 23 E, nineteen G,
thirteen O and seven P; all chances are 100 and reserved arguments zero.
Zone 270 uses mode-two resets. Caps and conditional/container placement still
need live qualification; four jar declarations are not a promise of fresh stock.
The shop header is `#27072~`, which a numeric-header-only scan misses.

The sole literal local binding is [kobold 27009's crossbow](../../../src/specs/specs.assign.c#L300).
Reviewed all three functions in [the Halfcut special unit](../../../src/specs/specs.halfcut.c).
`halfcut_defenders` and `blowgunner` have no assignment; their comments or room
prose do not establish an active level gate or dart hazard. The defender's
low-level random branch would not implement a strict level-44 gate anyway.
The unbound blowgunner has a similar setup/periodic issue, but activating it
would add gameplay and requires a separate placement/balance decision.

Shared review includes native acceptance/reward recovery and retirement,
global item producers, containers, switch/reset semantics, item teleport,
doors, falling, NPC wandering, inn, shop and epic teaching. The inn room's
`ROOM_INN` flag actually assigns [the shared handler](../../../src/world/db.c#L1368).
Slagtooth's invitation does not imply a separate NPC quest. The
[teacher table](../../../src/classes/epic_skills.c#L179) lists A'den for Improved
Listen with Listen 80, and [initialization](../../../src/world/epic.c#L1368)
assigns the shared teacher despite no literal local teacher binding.
Skill/fee eligibility and settlement remain separate from journal credit.

The only ordinary foreign boundary is reciprocal: local 27001 north reaches
surface 629274, the Great Forest of Aravne, whose south exit returns. All local
switch and teleport destinations exist; no invented restoration is needed.

## Exact contract and progression matrix

Bindings include recipient and normalized input, output and retirement terms.
Multiple recipes for Bartis and same-visible-name miners remain distinct.
Every row below is an independent outcome, with no preparation-service row
or inferred all-stage achievement.

| Recipient/native source | Exact offering → declared reward | Treatment |
| --- | --- | --- |
| [Wounded dwarf 27002](../../../areas/qst/halfcut.qst#L13) | Green potion 27037 → C25000 + E25000; stays | Medicine delivery; no checked poison cure |
| [Old miner 27059](../../../areas/qst/halfcut.qst#L61) | Brown jar 27039 → badge part 27040 + E2000; retires | First distinct miner; narrated escape |
| [Old miner 27071](../../../areas/qst/halfcut.qst#L136) | Brown jar → different badge piece 27041 + E20000; retires | Second same-named miner; preserve distinct reward |
| [Young miner 27060](../../../areas/qst/halfcut.qst#L75) | Brown jar → broken badge 27042 + E2500; retires | Well rescue narrative; independent receipt |
| [Bartis 27065: badges](../../../areas/qst/halfcut.qst#L112) | 27040 + 27041 + 27042 → complete badge 27043; stays | One exact three-kind bundle; prior rescue receipts optional |
| [Bartis: final jar](../../../areas/qst/halfcut.qst#L121) | Brown jar → note 27044 + C150000; retires | Offer badges first in the same episode; no required earlier rescue |
| [Dwarf sentry 27005](../../../areas/qst/halfcut.qst#L31) | Bartis note 27044 → earring 27045; retires | Independent supplied-note return |
| [Remi 27033](../../../areas/qst/halfcut.qst#L53) | Raid leader scalp 27051 → thin black staff 27060; stays | Exact trophy, not whole-raid defeat |
| [Raid leader 27078](../../../areas/qst/halfcut.qst#L177) | Orc 27052 + drow 27053 + goblin 27054 + duergar 27055 + Bartis 27057 + Hulkuis 27058 scalps → duergar belt 27059; stays | One six-proof bundle; no six personal kill or takeover endpoint |
| [Head orc 27080](../../../areas/qst/halfcut.qst#L197) | Goblin scalp 27054 → C15000 + E25000; stays | Competes with the six-proof bundle |
| [Head goblin 27081](../../../areas/qst/halfcut.qst#L214) | Orc scalp 27052 → C20000 + E15000; stays | Independent competing request |
| [Drow leader 27082](../../../areas/qst/halfcut.qst#L231) | Duergar scalp 27055 → potion 27056 + **item 25000, absent**; stays | Concrete reward repair/admission blocker; no guessed replacement |
| [Head duergar 27083](../../../areas/qst/halfcut.qst#L246) | Drow scalp 27053 → C20000 + E25000; stays | Independent competing request |

The three miner receipts are optional preparation for Bartis's badge bundle;
his final jar receipt is optional preparation for the sentry's note. Supplied
matching parts or a note satisfy the native offerings without these histories.
Each final accepted-delivery step binds only its own exact contract. A material
readiness label does not consume an item or award progress.

## Sources, scarcity and recipient episodes

The northern wagon 27004 is declared at 27003 with green potion 27037 and the
area map as P contents. Its potion has Remove Poison 43, but the Q exchange
does not cast it on the wounded dwarf or validate an actual cured condition.
The supplier's flaming green potion 27046 is a different kind and cannot
substitute. Carry the recovered potion loose; seeing the wagon or its contents
does not satisfy the offering.

The wandering stone giant 27058 starts at 27396 carrying wooden box 27038.
Four P resets declare brown jars 27039 with cap four inside that exact box.
Its container flags make it closeable and closed, without a key lock. All
three miner deliveries and Bartis consume one jar each; this is adequate
declared quantity, not an established shortage. Global parent selection,
live ancestry/ownership, reset conditions, retained stock and source generation
must be qualified before fresh or personal recovery claims. The jars are
type-13 quest props, not usable potion items. NPC drinking/vanishing text is
implemented as acceptance and retirement, without an NPC teleport or recorded
home arrival; that may be an intentional narrative convention.

Old miners 27059 and 27071 occupy hidden refuges 27415 and 27416. Their
names match, but badge kinds and E2000/E20000 rewards differ. Young miner
27060 occupies well interior 27429; Bartis occupies hidden office 27433.
The bundle needs three distinct kinds, not three interchangeable badge copies.
Complete badge 27043 has no further local Q consumer or automatic campaign
effect. Bartis accepts badges without leaving, while his jar removes him.
Finish the bundle first in one episode; a receipt cannot restore him. This
ordering is guidance, not a newly imposed native prerequisite.

Every scalp source has a one-copy G declaration on its named NPC: raid leader
27465, head orc 27469, drow 27467, goblin 27468, duergar 27466, Bartis 27433,
Hulkuis 27431. Most share the primary keywords `scalp leader`, despite different
visible names. Exact prototype identity remains essential; the journal does
not invent unique command aliases. Four faction proofs are consumed either
by a side request or the six-item bundle until replacements exist. Bartis's
retirement may remove his remaining carried scalp, and Remi's trophy consumes
the raid leader's own scalp. An explicit allegiance/attempt policy must precede
a personal rescue-versus-takeover campaign; current independent deliveries
allow supplied proofs without recorded murders or mine ownership.

Remi, A'den and other outdoor walkers start at dispersal room 27189. Six
surrounding dispersal rooms have real routes into the hills or back to the
source; one room only returns. They are not categorically unreachable, and
their non-sentinel/non-stay-zone flags do not guarantee a particular encounter.
The box-carrying stone giant also wanders. Two kobold holding rooms and boss/
dragon gear are source/combat leads, without additional quest terminals.

## Actual access and hazards

The [command constants](../../../src/cmd/interp.h) matter: 65 is `grab`, 17 is
`say`, 7 is `enter`, and 340 is `pull`. The
[teleport handler](../../../src/magic/spell_travel.c#L932) resolves the named
object through ordinary object lookup; a speech-triggered slab is not an
addressed NPC topic or an inferred password achievement.

- Hidden rope 27020 at 27046 uses **grab rope**, targeting the western
  blocked/secret passage to cave 27374. The reset uses state 13. The
  [switch handler](../../../src/specs/specs.object.c#L309) removes blocked
  state, while ordinary opening/discovery and passage still need qualification.
- Both polished black stones 27024/27025 lie in cave 27374 and use **grab**.
  They lead to spire top 27242 and domed plateau 27380 respectively, with
  unlimited charges. Their same names/keywords require deliberate numbered
  object selection; test actual lookup/reset order rather than invent unique
  selectors or treat either as a two-stage puzzle.
- Well 27029 at 27423 uses **enter well** to reach 27429. The normal up
  exit returns to the gathering cavern; no pulley operation or jar-use
  prerequisite is checked by the native young-miner exchange.
- Slab 27035 at 27110 uses **say windship** (an actual object keyword) to
  reach lich crypt 27453. Its naming differs from the inscription's Winship.
  Crypt doors, Count and vitality vial 27036 are exploration/loot, without
  a local Q return or verified resurrection endpoint. The cemetery's ordinary
  secret down route leads to 27451; the slab jumps directly into the crypt.
- Pull lever 27031 beneath Hulkuis's desk at 27431 removes blocked state
  from the western bedroom door. Reset state 9 is closed and blocked;
  ordinary opening remains separate. Neither source nor destination is missing.
- Hill giant guard 27068 carries LARGE key 27033 for the reciprocal
  27440-east/27450-west cavern door. It is a separate gate from the office
  lever. Custody, unlocking, opening, travel and safe return are distinct facts.

Review retains all falling and room restrictions, including the spire pit,
midair/spire edges, large air maze, cavern drops and crypt. A garbage-pit room
description alone is not proof of a deathtrap. None of these hazards creates
an achievement for first visitation or survival. Accepted post-movement
events need actor identity, destination, source, failure and survival facts.

## Shipped repair and news handoff

**Fix commit:** [b28262d8c](https://github.com/Community-Duris/Duris/commit/b28262d8cc8dfe6156df70f162981399d18be717). Bound crossbow mob 27009 loads in hidden
alcove 27142, firing at 27139, 27137 and 27136. Previously its procedure only
handled `CMD_SET_PERIODIC`, attacked during that setup signal, returned false,
and declined later `CMD_PERIODIC` calls. The loader only schedules procedures
that return true during setup; [the scheduler](../../../src/mob/mobact.c#L10831)
then invokes the periodic command. It therefore never ran the intended pulse.

The fix opts into scheduling without attacks during setup. Each pulse retains
three lanes, player-only targets, four bolts and the original `dice(2,4)+10`
damage. Targets are snapshotted by runtime identity and re-resolved before
each bolt; dead, removed, moved or replaced targets stop their volley. A dead,
removed or moved ambusher ends further firing. A missing lane is skipped
without hiding later lanes. Unbound defender/blowgun behavior, native Q terms,
room/reset/gear data and economic guards are unchanged.

**Warning fix:** [348eccdf3](https://github.com/Community-Duris/Duris/commit/348eccdf3261e62aa8984ac0868b98adfa815a06).
The direct warning previously used `TO_VICT` with the struck player as both
actor and recipient. The shared `act` audience filter excludes its actor
unless the audience is `TO_CHAR`, so the “striking you” text was suppressed.
It now uses `TO_CHAR`, keeping the separate room warning. The regression
models that actual audience rule, fails the preceding crossbow version's
delivered-message count, and passes with four direct/four room warnings per
uninterrupted volley. Damage amount and recipient selection are unchanged.

[The focused regression](../../../tests/async/test_halfcut_crossbow.py) compiles
and executes the actual procedure, covering null/setup actors, all lanes,
unrelated callback actors, four bolts, NPC/dead-target exclusion, missing lane,
target death/movement/removal/storage reuse, ambusher death/movement/removal,
next-occupant removal and a newly inserted occupant. The original procedure
fails during setup; the fixed procedure and maintained server build pass.
This proves callback behavior under controlled damage outcomes, not live
combat balance, reset availability or every combat/accounting side effect.

**Player news:** “The Halfcut Hills kobold crossbow ambusher now fires on its
scheduled pulses, shows each struck player the bolt warning, and stops
interrupted volleys safely.” Announce after merge
and deployment. The separate Hall Shadow of Sin fix remains separately named.

## Pending repairs and expanded capability work

| Finding | Balanced status and next action |
| --- | --- |
| Drow missing reward | `R I 25000` is literally an item reward, absent from all active and inactive object files. A mob with that number does not make it an object. Do not assume coins or XP. Builder chooses intended kind/value or object; follow with a separate tested native recipe fix. |
| Reward admission/loadability | Current supported-kind admission does not preflight reward-item loadability before offering consumption/new credit. The drow's existing potion reward does not cure its missing second output. Static daily eligibility and warning text do not prevent partial/pending payout. Expand reference validation and admission preflight while preserving frozen outstanding obligations; qualify retry, reload and no-consumption failure. No such guard ships here. |
| Rescue representation | Brown jars intentionally function as accepted props; disappearance is not an actual home arrival. Decide whether narrative retirement is sufficient or add atomic custom teleport/escort and committed destination evidence. Preserve independent supplied deliveries; do not impose unproven source or personal-history requirements. |
| Episodes and competing trophies | Three rescue receipts, one badge bundle, final jar and note are independent. Four scalp side requests compete with the larger bundle, and Bartis retirement competes with scalp supply. Select recipient/source generations, branch and attempt policy before enforcing ordering or all-stage/kill/takeover credit. |
| Aliases/prose/usability | Review distinct scalp aliases, same-name stones/miners, Bartis/Baritis and Hulkuis/Halkuis spelling, Windship/Winship clue consistency and minor text errors. Preserve exact item identity and native rewards until builder decisions; no alias or prose repair ships here. The two old miners' different XP is not automatically a defect. |
| Unbound procedures | Similar setup defect exists in unbound blowgunner; no active binding was found. Decide intended placement/balance before enabling it. Defender's comment does not enforce level 44. These are separate proposals, not claims of active repaired hazards. |
| Access and source lineage | Qualify closed box/wagon ancestry, four distinct jar UIDs, one-copy scalp generation, wandering encounters, numbered stone selection, grab/pull/say/enter, keys/doors, falls and safe arrival. Current material/history checks provide guidance without historical ownership or travel credit. |

## Verification boundary

Production fixtures verify all thirteen exact bindings, all fifteen dialogue
families, actual keywords, source/reset/gate/teleport identity, missing reward,
cash-versus-fee distinction, two old miners and exact three-/six-kind bundles.
Native journal fixtures cover encounter visibility, supplied badge/note inputs
without producer history, wrong/worn material, read-only views, history with
spent proof, independent receipts and cold recovery. They cannot instantiate
the missing reward or prove that an NPC reached home.

All 2668 native definitions, revision-two source fingerprint, zone registry
and other 62 journals remain unchanged. Catalog: 63 journals, 1690 achievement
units, 1490 potential daily units and 2225 rows. The original 220-area order
remains intact: first 42 source-comprehensive, 178 pending. Continue with
The Scorched Valley (`scorchvalley`).
