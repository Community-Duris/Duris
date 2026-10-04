# Building zone story integrations

Builders can add an optional `areas/story/<source_area>.story.json` beside
their area's existing `.qst`, `.zon`, `.mob`, and `.obj` files. The server loads
it at catalog bootstrap. It supplies names, summaries, alternative terminal
contracts, checklist steps, and reviewed exclusions. Native quests and
accounting still execute exchanges and award their original rewards.

Areas without a sidecar retain their native-contract journal and progress.
The sidecar does not execute scripts, move players, consume offerings, or
grant rewards. The [larger plan](../design/ZONE_STORY_INTEGRATION_PLAN.md)
tracks the later event and provenance work.

Player journals and new discovery/encounter/daily eligibility require verified
active economic accounting. Builder source validation can run offline without
activating accounting or daily rewards. See the [ordered zone roadmap](../design/ZONE_STORY_ZONE_PRIORITIES.md)
and [complete evidence inventory](../reference/ZONE_STORY_ZONE_INVENTORY.md) to
choose the next integration.

## Builder workflow

1. Export readable native bindings for the active area's source filename:

   ```bash
   python3 scripts/zone_story_quest_catalog.py --source-root . \
     --story-inventory twin_towers_forest > bin/twin-story-inventory.json
   ```

   Copy each entry's `binding` into a `contracts` array. The canonical key
   identifies exact offering/reward/disappearance terms, not a Q-block ordinal
   or dialogue string. Changed native contracts must be rebound deliberately.
2. Create a sidecar with `coverage: "partial"`; add integrations as their actual
   behavior is verified. Unbound contracts retain their fallback presentation.
3. Trace item sources in resets/prototypes and inspect relevant specials and
   assignments. Mechanical producer/consumer links are candidates, not proof
   of story prerequisites. NPC prose can disagree with executable terms.
4. Group successful alternatives; classify rejected offerings and supporting
   trades/services. A terminal contract belongs to one story or exclusion.
5. Validate and regenerate the checked-in audit snapshot:

   ```bash
   python3 scripts/zone_story_quest_catalog.py --source-root . --check \
     --production-output docs/reference/ZONE_STORY_QUEST_PRODUCTION_CATALOG.json
   ```

6. On an isolated server, visit the area and inspect its journal, achievement
   totals, and original exchanges. Check every alternative and equipment
   condition, then reconnect/restart. Update the integration register.
7. Use `coverage: "complete"` once every active native contract is classified.
   This certifies static contract coverage, **not** complete custom-script or
   historical objective coverage. Deploy the sidecar with matching world data.

Files take effect at bootstrap; no new live reload command is introduced.
Missing files are optional. Present invalid, unreadable, or oversized files
make zone story bootstrap fail closed; they are never silently ignored.

## Current contracts and intended repairs

Keep an intended progression separate from the actual offering/reward matrix.
Ashrumite's [source dossier](../design/zone-stories/ASHRUMITE_VILLAGE.md)
shows why: silverworking dialogue currently uses raw gems, necklace setting
produces gold ore and magical enchantment produces pyrite with a missing disc.
Static extraction can find these terms and producer/source links. It cannot
choose corrected item identities, a replacement source, prices or campaign
entitlement from matching names or prose. Exact quantities and same-name kinds
must remain distinct in current checks; historical receipts do not create stock.

Use `service` for support crafting and paid information. Explain unavailable
or missing-reference contracts without adding nonexistent item checks, guessed
personal-source milestones or achievements for each keyword. A repaired native
contract needs deliberate rebinding, content-fingerprint review and recovery of
frozen original terms. Increment the journal revision for changed projection.
Keep active-accounting guards until payment/source/output qualification passes.

Record proposed native repairs separately from implemented fixes. For an actual
repair, use a clearly named separate fix commit where practical and PR/news
notes naming zone/interaction, player trigger, before/after, proof and limits.
A journal correction or planned recipe change is not a shipped native repair.

The Hall's [source dossier](../design/zone-stories/THE_HALL_OF_THE_ANCIENTS.md)
shows a second class of mismatch: the loader prepends identical elder offerings,
so the consuming refusal shadows the ore reward. Both remain excluded until
eligibility is deliberately repaired; an item graph cannot infer a saved-son
condition. Sixteen belt ingredients exceed the current fourteen-root offering
limit, and one-copy source caps conflict with repeated quantities. Optional
current-material checks must explain these blockers without enabling a recipe.
Keep actual ten-item armor terms, including one potion, separate from the
two-sample dialogue. Review exact source budgets and receipt rebinding before
changing native balance or limits.

Custom death-spawn guardians need atomic actor/item/source-episode lineage.
A spawned potion in current custody does not prove a personal first recovery;
a later player gift is different. Likewise a pre-command GET procedure can
change cathedral doors before pickup, even on a failed request. Select intended
accepted acquisition/coin settlement or an explicit attempt mechanism before
adding objectives. Shared capture and event persistence need qualified runtime
facts, not a new sidecar keyword that manufactures them.

Sin's actual-opponent Freedom of Movement correction is a shipped separate
combat fix with regression and PR/news evidence. The elder, recipe/source and
cathedral changes remain plans; keep that distinction clear in builder notes.

Sarmiz'Duul's [dossier](../design/zone-stories/SARMIZ_DUUL.md) demonstrates a
custom story outside native Q bindings: boot-seeded moonstone fragments,
pirate-chest core, periodic material assembly and alternative ring or paid
crew rewards. Pirate setup is deliberately disabled with accounting active;
adding a sidecar cannot make it available. Qualify durable treasure/key issuance
and atomic material/output/payment/crew settlement before a custom completion
adapter. A finished stone also feeds a foreign consumer; preserve exact custody
and receipt ownership rather than crediting all consumers from assembly history.

Validate an actual custom ask recipient before publishing dialogue, attacking
or settling rewards. Record accepted events after the mutation commits;
pre-command attempts and material possession are insufficient. Root-carried,
bagged, worn, supplied and personally sourced pieces are different states.
Explicitly document partial boot failure, multiple-core TODO behavior, output
allocation failure and replay policy. Sarmiz's potion descriptions and missing
mount stock are pending builder repair decisions, not changes shipped by the
journal. Use separate fix commits and prominent tested PR/news wording for
actual native/world-data changes.

Duke Delwyn's [dossier](../design/zone-stories/DUKE_DELWYN.md) demonstrates
four paid textile services feeding one independent banner delivery. Show exact
fees and current materials, preserve supplied-banner acceptance, and keep all
five paid services outside achievement/daily credit. Optional producer receipts
explain a route without paying fees or replacing consumed inputs. Atomic mixed
settlement must precede enabling the guarded local producer route.

Access hints should account for actual shared mechanics: a trapped closed desk
and midair fall chances can injure or remove an actor before quest acceptance.
Container contents, successful opening, survival, personal acquisition and safe
return each need their own accepted evidence when builders make them objectives.
An exact supplied document or banner remains a valid independent native input.
Cross-check absent-route scans with raw active exits; Delwyn's surface boundary
is reciprocal. Occasion-text, advertised trade/lodging and optional peaceful
material handovers remain builder decisions, with no native repair in this
journal checkpoint. Actual later repairs need separate tested fix/news entries.

Home of the Divine's [dossier](../design/zone-stories/HOME_OF_THE_DIVINE.md)
distinguishes nine item-only cash-reward requests from six guarded mixed fees
and twelve support rows overall. Classify the interaction's purpose and actual
input/output: receiving coins is not a paid service. Multiple recipes for the
same NPC must bind by complete normalized terms, not giver or item name alone.
The two same-named wyvern eggs and flute/weapon variants remain exact kinds.

The missing Relazier bounty reward demonstrates why candidate shape and
carried-material readiness cannot certify payout. Current admission lacks item
loadability preflight; warnings do not add a consumption guard. Builders choose
the intended reward, followed by deliberate pre-consumption/recovery and native
repair work. Keep generic dragon-scale and mixed-payment guards; a sidecar
cannot enable source issuance or safe fees. Two separate Riser/shard UIDs,
real touch/enter/key/container/trap/fall routes and competing/retiring episodes
need qualification. Optional support receipts cannot replace spent stock,
enforce allegiance, free a prisoner or complete a crafting campaign. Wrong
heart aliases and empty prose are pending repairs, with distinct fix/news
entries only after implementation. No native repair ships in this checkpoint.

The Halfcut Hills' [dossier](../design/zone-stories/THE_HALFCUT_HILLS.md)
shows why three same-story rescues, a badge bundle and a note return remain
independent native outcomes. Prior producer receipts are optional when supplied
matching material is accepted. A retiring recipient needs an episode policy:
offer Bartis's badges before his final jar in the same episode. Four actual
jar declarations are adequate source quantity, but closed-container ancestry
and live stock still need qualification. Narrated drinking/disappearance of
type-13 quest props does not establish an actual rescued NPC arrival home.

The six-scalp bundle and four faction requests compete for exact materials;
duplicate names/keywords do not permit type substitution or prove personal
kills. Keep branch/attempt and full campaign policy explicit. Missing drow
reward 25000 needs builder-selected terms and loadability preflight before
consumption/new credit, preserving frozen obligations. Shared grab/say/enter/
pull commands, numbered black stones, key/opening and safe travel need real
events. Inn and table-based epic teacher bindings exist without extra local
literal assignments. The crossbow scheduler/continuation is a separate shipped
fix with focused proof and a news sentence; a second separate fix delivers its
struck-player warning. Unbound hazards, reward and minor
alias/spelling proposals remain pending. Journal additions are not native
quest repairs.

Scorched Valley's [dossier](../design/zone-stories/THE_SCORCHED_VALLEY.md)
shows a five-key route to blood, four exact colored rewards and a separate
necklace delivery. Keys and producer history are optional guidance when
supplied proof fits; possession does not prove opening or first recovery.
The advisor carries the chest key and receives three other requests, so
source and recipient episode policy matters. Foreign holding routes and
foreign trophies do not transfer contract ownership to the encounter room.
Qualify actual visibility, travel, container ancestry and accepted delivery.
Physical-room discovery precedes an encounter; normal runtime arrival records
it. The current immediate hint still uses the physical area's journal, so
an owning-journal referral needs qualification without remote auto-discovery
or changed credit. Keep the actual discovery guard in fixtures and runtime.

Captive sack, resummoned bodyguard, society membership, curse and rod/battle
finale need real supported state/terminal transactions before objective credit.
Two same-named commanders carry different proof; native acceptance outranks
an overly broad clue. Keep all useful dialogue families on a contact card,
selecting valid synonyms within its thirty-two-topic limit; source indices
retain every native alias. Yeenoghu's dispatch/safety/balance and truthful
clue/departure work are pending actual repairs. This journal ships no native
fix; later repairs need identifiable fix commits and clear PR/news evidence.

Court of the Muse's [dossier](../design/zone-stories/COURT_OF_THE_MUSE.md)
shows four seasonal producers and a separate four-token admission delivery.
Producer histories are optional; supplied distinct tokens fit without personal
favors. Twelve koi scales are one counted material check, while two instances
of an imaginary friend share one contract/outcome. Exact source instances
matter: later matching satyrs/sprites do not all inherit carried essence.

The book and dew really key cave doors because shared lookup matches vnum
without requiring key item type; locket prose alone does not override that.
The dew is also consumed by Spring, so explain competing use without making
optional cave access a required campaign. World door types and reset states
are separate. Actual trap charge/damage, key destruction, access/fall/current
and reset return need qualified events. Unsupported trap values and conflicting
pouch/clue text need deliberate separate content fixes; seasonal/audience,
fishing or soul-extraction objectives need real accepted endpoints. No native
repair or new schema path ships with the journal; later fixes need clear news.

Valley of the Snow Ogres' [dossier](../design/zone-stories/VALLEY_OF_THE_SNOW_OGRES.md)
shows three distinct producer rewards and independent shard assembly, with
optional personal histories that never replace current proof. A same-kind
hide refusal is explicitly excluded; the full six-hide/two-weapon/2,500-platinum
armor recipe is a non-credit service, with mixed settlement guarded. Its tinker
has an active foreign dispersal source: distinguish physical discovery, actual
encounter and contract ownership rather than calling a missing local reset an
absent NPC. Missing stalk and limited hide caps need deliberate source plans.

Trace actual command dispatch and assignment replacement. Push rock clears
only one blocked state; secrecy/opening and living guards remain independent.
A specific golem binding overrides block_dir, and the lich's teacher flag
does not install another function. Axe/whip hit 1000 callbacks are connected,
whereas the berserker's command/setup combination lacks its normal route.
Target reversal, helper-count/prototype mismatch, virtual-versus-real room
replacement and unsafe equipment continuation need separate focused repairs.
Do not enable dormant behavior while mapping. Accepted source, control, entity,
effect and reunion transactions are prerequisites for deeper credit; actual
future fixes need clear commits/news. No native repair ships with this journal.

Dawndale's [dossier](../design/zone-stories/THE_MOUNTAIN_VALLEY_OF_DAWNDALE.md)
shows optional local/foreign producer histories, separate captain receipts for
identical consumed recipes and a distinct same-named flute return. Preserve
item-kind identity, duplicate counts, both recipients and owner zone. The city key
survives admission and is consumed by its final delivery; office/alcove keys can
break. A contact reused by discovered journals shares its saved encounter,
while each receipt keeps its contract owner. Coin-only purchase
and mixed crafting must retain accounting guards; an item-only coin payout is
different. Preparation/service/referral classification removes extra credit
without changing native rewards or inventing full-stage requirements.

Trace generation, container custody and content. Sand may come from a death
procedure rather than an item reset; treasury proof may sit inside a closed
sack rather than be ordinary money. Three vial declarations do not establish
four-copy supply. A foreign administrative copy may share a global cap, while
forced resets bypass that cap; qualify renewal/recovery before repairing stock.
Water item identity alone does not verify source, liquid or volume. Secret rock
switches can clear separate directions on an already open passage, so do not
add an opening instruction merely because another zone uses a secret door.

Descriptions of BOOM, new camps, follower escape, worm tunneling, incantations,
curse removal or telescope use need actual accepted endpoints before credit.
The great lens is not a demonstrated broken apparatus. Builder-selected fossil
follow-up and Ender audience-key repair remain pending; actual future repairs
need separate clear commits and prominent PR/news text, with proof and limits.

The Abyss [dossier](../design/zone-stories/THE_222ND_LAYER_OF_THE_ABYSS.md) shows
equivalent giver aliases without duplicate achievements, exact same-named bodies
and heads, optional foreign producer histories and shared recipient retirement.
Multiple completion bindings mean alternatives; they do not enforce a full-stage
AND campaign. Source versus handoff, random consumable reading and current loose
container proof remain separate from a receipt. Four staging-room DOWN links use
ordinary wandering, not F/falling; qualify actual source arrival before promising it.

Trace room lifecycle in custom phase changes: already-linked insertion is refused
by the current handler, so missing unlink can block later key/reward sources even
when all prototypes exist. Journal authoring must not silently activate dormant Ebb,
create lich/rebirth/escort endpoints or fix clue/departure text. These are separately
owned repairs with actual procedure/journey tests and prominent fix/news reporting.

The Surface Minizones [dossier](../design/zone-stories/THE_MINIZONES_OF_THE_SURFACE.md)
shows exact repeated counts and five equivalent cleansing alternatives. Any-one
materials differ from a five-kind recipe; optional producer receipts do not
restore spent wands/spheres/tokens or prove later payment. Classify ongoing
commissions as support services, retain supplied proof and keep foreign producer
and duplicate-host ownership explicit. Three retiring artifact choices and a
single sphere cannot be treated as guaranteed supply for every service.

Trace custom sources as well as resets: death-to-object generates Incarnate and
psychomia materials without O/G/P. Random transformations need preflight and
lineage; summon procedures need failure cleanup, placement/identity continuation,
cooldowns and actual pet endpoints. Decorative springs and griffon narration are
not usable fountains or granted mounts. Missing ordinary inbound links and dormant
igloo prototypes require builder intent before enabling new content. Keep clear
text/cleanup repairs in separate commits and prominent PR/news entries; conditional
branches with identical current eligibility predicates are not demonstrated bugs.

## Schema version 1

Version 1 remains supported. Use version 2 or 3 for new starter and hometown
integrations; it adds the required `introduction`, `orientation`, and `contacts`
fields below. Story bindings and step kinds retain the same meanings.

Every shown field is required. Unknown/duplicate fields are rejected. Files
are bounded to 512 KiB in both the authoring validator and native loader.
Plain-text strings are at most 1,024 UTF-8 bytes, without controls or `$`
substitution tokens. IDs use lowercase letters,
numbers, hyphens, and underscores, at most 64 characters.

```json
{
  "schema_version": 1,
  "revision": 1,
  "source_area": "twin_towers_forest",
  "coverage": "partial",
  "stories": [
    {
      "id": "flowers-for-alvinar",
      "title": "Flowers for Alvinar",
      "category": "story",
      "summary": "Alvinar wants a plant he can grow. A cut flower will not do.",
      "contracts": [
        {"giver_vnum": 13500, "completion_key": "give=I:13553;receive=I:13572;disappear=0"}
      ],
      "steps": [
        {
          "id": "garden-access",
          "text": "Wear the gardener's belt at your waist",
          "kind": "equipped_item",
          "hint": "The gardener wears this belt. Carrying it is insufficient.",
          "item_vnums": [13521], "count": 1, "slot": 13
        },
        {
          "id": "plant", "text": "Carry an orchid sprig for Alvinar",
          "kind": "carried_item", "hint": "Recover a plant from the towers' garden.",
          "item_vnums": [13553], "count": 1
        },
        {
          "id": "turn-in", "text": "Complete Alvinar's original turn-in",
          "kind": "completion", "hint": "Give him the requested plant.",
          "contracts": [
            {"giver_vnum": 13500, "completion_key": "give=I:13553;receive=I:13572;disappear=0"}
          ]
        }
      ]
    }
  ],
  "exclusions": []
}
```

This is a partial example. The production Twin Towers mapping binds all eight
acceptable plants and classifies its remaining native contracts.

| Field | Meaning |
| --- | --- |
| `revision` | Positive mapping revision; increment when changing it. Native receipt IDs and persistence schema are not rewritten. |
| `coverage` | `partial` retains unbound contracts; `complete` rejects unclassified active native contracts. |
| `category` | `story`/`request` contribute one achievement unit. `service` appears in the journal but contributes neither an achievement nor a daily unit. |
| Story `contracts` | Alternative authoritative terminal exchanges: any one recorded exchange completes the story. Terminal ownership is local to the mapped area. |
| `steps` | One to 32 ordered explanations/checks. These add no new gameplay prerequisites. |
| `exclusions` | Objects with `reason` and `contracts`; remove reviewed responses/trades from journals/totals while retaining native execution and receipts. |

### Supported step kinds

Each step requires `id`, `text`, `kind`, and `hint`; `hint` can be empty.

| Kind | Additional fields | Evidence |
| --- | --- | --- |
| `carried_item` | `item_vnums`, `count` | Live top-level carried inventory. Counts sum alternatives and cap at the requested count. Equipped, dropped, nested-container, and stored objects are not carried offerings. |
| `equipped_item` | `item_vnums`, `count`, `slot` | Live equipment; `slot: -1` accepts any slot. Otherwise use the numeric constant from `src/core/defines.h`; `WEAR_WAIST` is 13. |
| `completion` | `contracts` | Existing seasonal native completion records; any listed receipt satisfies this step. Intermediate/cross-area receipts may be referenced without becoming terminal bindings. |

Item alternatives must be distinct positive existing object VNUMs; counts are
1–3,000. Independent required kinds need separate steps: four different feather
kinds must not accept four copies of one feather. The offline tool checks
active object sources; runtime checks the booted object index.

Live checks render `Ready now` or `Missing now`, and `Check inventory` if no
snapshot is supplied. They never prove personal acquisition. Recorded exchange
steps persist. A story remains complete when its materials are consumed or its
belt removed. A gifted plant can still satisfy Alvinar's native turn-in without
personal garden access. The schema does not enforce new narrative prerequisites.

Daily eligibility still requires supported native offerings and the existing
telemetry policy. A mapping cannot make mixed item-and-coin exchanges executable
or bypass evidence checks. Any terminal alternative marks its story done today;
another alternative cannot earn another daily unit. Existing rewards and the
one-renown-per-day cap keep their existing authority.

### Reviewed exclusions

```json
{
  "reason": "Rejected flower: Alvinar returns the submitted cut flower.",
  "contracts": [
    {"giver_vnum": 13500, "completion_key": "give=I:13552;receive=I:13552;disappear=0"}
  ]
}
```

Do not automatically exclude all returned-item exchanges: another zone may use
that shape for a legitimate transformation. Twin Towers exclusions were reviewed
against their actual responses.

## Linked preparations and independent stages

`contracts` on a story means **any one terminal alternative completes the unit**.
Do not put all promotions or all successive exchanges into this array and call
it an entire campaign: the first receipt would prematurely complete it.

The Elven Homestead and Torg journals name terminal stories and separate
preparation services. Their summaries/hints explain one valid material route;
native exchanges still accept valid gifted materials. Krimeneha's rescue
exchanges retain genuine returned-fragment milestones instead of treating every
returned item as rejection feedback. Bastine promotions and Breale's successive
mixtures remain distinct accepted requests until an all-stage family can be
authored. Native receipt checklists are historical evidence; inventory is live.

Schema 3 supports optional preparation, including historical native receipts.
Use it when a reviewed producer route helps explain a request that also accepts
supplied terminal materials. Optional history cannot replace live stock or
impose an earlier exchange. Conditional subrecipes and all-stage family
completion remain planned capabilities; retain separate native exchanges until
their accounting-backed evidence and shared presentation are qualified.

`item_vnums` on a carried-item step means a **combined count across the listed
kinds**. It does not mean the required count of any one matching kind. For
example, a count of two with strength and wisdom scrolls reports two when the
player carries one of each, even if the recipe requires two strength scrolls
or two wisdom scrolls. Author exact single-kind checks, or retain recipe choices
in text without a misleading live check. Likewise, separate item steps cannot
express either one whole recipe or another; all-of/any-of recipe predicates and
exact root allocation require a future schema. Optional status does not change
the counting semantics. See the [Alatorin dossier](../design/zone-stories/ALATORIN.md)
for the two/six/eight-matching-material examples and planned qualification.

## Source mismatches, properties and unfinished lore

The [Newhaven dossier](../design/zone-stories/NEWHAVEN.md) shows why full
source review matters. Dibbly's fishing-line prose accepts a different snorkel
prototype; retain actual bindings until builders deliberately correct content.
The foreign collar switch resets inside the blocked alcove it should expose;
an item-source edge alone cannot establish a usable approach. The lizard tail
has an actual get/put trap, while the beast carrying cloak material disperses
into mainland forests. Read current command IDs and exact targets/parents.

ROOM_INN automatically binds the shared inn procedure at room bootstrap;
literal assignment lists alone omit the living inn. Conversely, literal pool
assignments with missing loaded targets are not active travel routes. Include
property-driven dispatch and active source inventory before presenting access.
Rift confession and prisoner pleas remain narrative without accepted endpoints.
Current possession is preparation; it does not prove a kill, harvest, gift-free
recovery, safe handling or supported payment. Author exact optional checks and
keep future semantic events and deliberate content repair in the shared plan.

## Equivalent recipients and confirmed effects

The [Faerie Realm dossier](../design/zone-stories/FAERIE_REALM.md) groups two
makers' identical five-material/coin recipes as one service with alternative
native bindings. Keep each exact material check separate; five copies of heat
cannot replace the other four kinds. The service supplies Fix; repairing a
selected item is a later device effect requiring its own durable evidence.
Other Fix sources do not prove five-plane recovery or a forge reconstruction.

Finn's retiring key finale accepts a supplied key without prior ring help.
Optional receipt history explains the story; it must not block native admission
or silently complete his signet and Celriya's blade. Recipient retirement is
an accepted outcome, without invented player escort or castle arrival.

Read property-driven access and actual dispatch. Speaking peace unlocks the
garden door but leaves it closed. The placeholder golden gate has no target;
a negative key alone does not establish a playable riddle. A maker's prototype
owner and initial dispersal room do not establish its current physical location.
The tree helper's old callback is not reached by modern combat: qualify the
dispatcher and builder-selected encounter policy before exposing helper goals.

## Producer history, counts and virtual services

The [Verspin dossier](../design/zone-stories/VERSPIN.md) separates Ramous's
apple-for-bone service from the lion's collar story. Show the earlier receipt
as optional history; a supplied final bone still qualifies. Possessing a collar
or another recipient's receipt does not complete the lion. Keep five same-kind
totems/symbols as count-five checks, while three amulet colors remain separate
kind-one checks. Validate reset caps against simultaneous recipe quantities;
four reds cannot be promised from a reviewed shared cap of three without a
deliberate source/recipe decision.

Review the full custom procedure and caller before choosing objective kinds.
Shinjin's listed potions directly change stats; they create no objects. They
need committed wallet/expected-stat/effect/save evidence, not an item-possession
objective. Crew hiring must honor denied payment before any ship mutation;
pending-command serialization alone does not establish accounting admission.
Lozin's unplaced ten-offer sign does not create five missing contracts. Static
persuasion, honor, reunion and tower-cleansing text remains explanatory until
builders add accepted outcomes. XP sharing/caps do not prove personal source
recovery or campaign participation; preserve admitted frozen awards during any
deliberate balance correction.

## Refunded briefings, regional collections and crate alternatives

The [Ship Yards dossier](../design/zone-stories/SHIP_YARDS.md) illustrates an
optional producer route: Pol refunds the briefing fee and supplies a note, but
his final recipe requires only five shells and five fire glands. The earlier
receipt and current note explain history/preparation without becoming mandatory
or additional achievements. Do not award an advertised pole that no contract
outputs. Keep six distinct potion kinds separate, four exact shivs despite a
conflicting six-shiv clue, and source versus gifted materials explicit.

Grimashk's normal and reinforced crates have different prices within one
recovery request. Group their achievement/daily projection while preserving each
native receipt and payout. Chundel is a different recipient with his own price
and independent outcome. Real source review includes dispersal destinations,
worn proof, caps, foreign nested containers, alternate stock and unresolved
exits removed by bootstrap; initial reset rooms are leads, not live locations.

Fishing text/XP before denied ownership does not establish an accepted catch.
Paid ship/crew mutation after denied debit does not establish a service purchase.
Extend accepted issuance and wallet/ship/save continuations before attaching
semantic objectives. Freshness, personal sailing, six vendor visits, allegiance,
poison/antidote/arsenal state and transformations need deliberate predicates and
outcomes. Keep supplied proof valid for the existing terminal while builders
choose accurate clues or balanced content extensions.

## Competing proofs and same-name outputs

The [Ultarium dossier](../design/zone-stories/ULTARIUM.md) maps four separate
soul kinds required simultaneously by the box. A director offering consumes
one of those proofs; its receipt cannot substitute for current proof at another
recipient. Three offerings declare no reward and stay support services pending
builder intent. A future all-stage campaign needs an actual allocation/episode
policy and world endpoint, while the current terminal still accepts exact gifts.

Recovered/delivery studies and raw/wearable living wind share names/aliases
but have different VNUMs. Test incorrect reward substitution and foreign onward
receipt ownership. Current key checks explain access without imposing an already
completed route. For secret boulders, distinguish accepted near-side clearance
from reverse clearance and actual arrival. For equipment, random pool effects
and class development, record the accepted state change rather than the command.

Keep paid identity, pet and epic-lesson guards until settlement is qualified.
Legacy pet claim text without a restoration call does not prove a returned pet;
restore or retire the feature deliberately. Blank responses, unused prototypes
and promised campaign finales need fair content review before declaring new
products, personal kills or whole-zone completion.

## Competing campaign materials and large regions

The [Surface Realm dossier](../design/zone-stories/SURFACE_REALM.md) distinguishes
four different trophies for a book, four consumed instances of one heart kind
for four different lockets, and a five-kind finale. Optional current possession
is guidance; prior receipts and duplicate kinds cannot replace consumed proof.
Recipients killed for one branch can require later appearances for another.
Do not manufacture exclusivity or a full campaign from opposed native requests;
define episode/allocation policy before deeper credit.

Group market price alternatives into support rows, preserving every exact
binding and distinguishing same-name kinds such as Breale versus normal bass.
Keep item-count preparation separate from coin readiness and guarded mixed
settlement. A retiring story recipient can remove its still-independent crafting
service, so explain useful order without inventing mandatory earlier receipts.
Use an actual valid plain NPC alias when its personal-name alias contains an
apostrophe; advertise verified addressed topics rather than ambient speech.

For large terrain areas, retain one discovery identity and use encountered
regional guidance. Verify the actual directional lock beyond an area's boundary
instead of inferring access from a reward's promise. Death-created wood, helper
spawns, race-context travel, disabled identity commands and commented invasions
need accepted evidence or deliberate repair decisions before journal stages.

## The gardener dependency

Alvinar's Q contract does not describe the barrier. `gardener_block` in
`src/specs/specs.twintowers.c` accepts item 13521 only in `equipment[WEAR_WAIST]`
for guarded movement. The reset loads gardener 13504 and equips the belt.
There is no native Q contract that awards it.

The reset exposes a source link; the special supplies the access predicate;
the sidecar relates them to Alvinar's story. A mapped condition can update from
live equipment dynamically. Narrative relationships implemented in arbitrary
C++ specials cannot reliably be discovered by parsing commands or dialogue.

Generic object mechanisms expose additional candidates through their typed
properties. Resolve `ITEM_SWITCH` and `ITEM_TELEPORT` values against the current
command definitions, actual target room/direction, stock placement and D reset
state. Validate applicable reverse edges and whether the actor can interact with
the source. Raw room flags are adjusted at load and reset; a key field alone
does not prove that a door is locked. Source-derived candidates still need an
authored relationship to a particular story and its reveal policy.

The [Alatorin dossier](../design/zone-stories/ALATORIN.md) gives concrete examples:
eight shrine switches listen to `hit` (70), rather than `kneel` (383); a remote
button opens the royal treasury; a rune-wall switch targets a nonexistent exit
in room 0. Explain or repair the actual mechanism before adding guidance. Keep
current access, accepted mechanism use, key consumption, arrival and NPC encounter
as separate facts. Schema 3 can explain these routes but cannot claim durable
historical completion of those new event kinds.

Check the complete usable route, including its return. Alatorin's vine objects
listen to `grab`, and its casket/crate mechanisms open exits from encounter
load rooms. A successful control action is distinct from meeting the occupant.
Some neighboring-area entrances have real reciprocal exits, while one reviewed
descent has no direct up return. Keep equivalent approaches available without
requiring the player to repeat optional history when they already have access.

Inspect the actual prototype type and wear flags for every requested/rewarded
item. An I reward can be `ITEM_MONEY`, as with Alatorin's Vergadain coins and
Abbathor satchel; its name does not establish an equipment grant or a balanced
currency mint. Ordinary C wallet rewards and physical piles need different
settlement evidence. The integration plan requires typed preflight and recovery
qualification before claiming those pile grants are supported with active
accounting. Preserve the exact native I binding until that adapter or a deliberate
content change exists.

Use current item names for display and VNUM/UID for identity, including equipment
renamed by identification. Confirm the actual slot when writing wear guidance;
the Alatorin audit found legplates flagged as arms and sleeves flagged as head.
Treat static X marks, clue notebook pages and copied extra descriptions as
authored text. They are not live journal progress, and `_noquest_` in an item name
does not erase a separately authored native contract.

Read reset eligibility as well as its numeric chance. Ordinary M/O declarations
with a non-100 chance use different admission from forced/initial population;
G/E/P/F/R and artifact checks have their own rules. Alatorin's rare recipients,
chance-2 flowers and chance-10 books illustrate why a source declaration is not
a promise of stock at every reset. Resolve loaded references before proposing
a repair: administrative paper 5 is valid, while stale 93183/57744 declarations
have no prototype. Builders choose an intended replacement or removal.

For nested supplies, record the actual parent. P placement searches a live
container by kind; the neighboring reset row alone does not identify its UID.
Require directly carried turn-in proofs, allow supplied/prepared alternatives,
and keep personal acquisition optional until accepted source events exist.
Fresh controls, keys and equipment also need durable reset-generation issuance
under active accounting; a recovered admitted object is a separate case.

Check the command's outer admission guard before advertising a preparation
route. Alatorin's material review confirms salvage and refining refuse active
accounting; even the transactional downgrade helper is behind salvage's guard.
Keep supplied/admitted ingredients distinct from personally preparing them.
Random quantities, recipe targets, prices and tool use need one accepted,
recoverable outcome; success prose or a detached output alone is not a receipt.
Do not describe an allocation/authority refusal as an intended failed skill roll.
The [material repair plan](../design/zone-stories/ALATORIN.md) covers refining's
released-input reads, catalyst/fee ambiguity and partial-grant qualification.

Administrative areas can supply valid shared prototypes without owning a
discoverable zone. Validate against every active `areas/AREA` prototype
source before calling an item missing. For example, paper 5 comes from
`limbo.obj` and participates in Tharnadia's map service. Mobile, object and
room numbers have separate namespaces: its missing mobile 132677 is not
repaired by the same-numbered map object. Preserve canonical journal ownership.

A P reset names a container prototype, whose shared loader currently selects
a global matching live instance. The preceding O declaration is a source
lead, not guaranteed parent identity. Qualify actual parent UID/location,
caps and moved/carried containers. Search can reveal hidden contents without
collecting them; only accepted pickup establishes custody. Keep source/GET
history separate from optional current material and final delivery. See the
[Tharnadia dossier](../design/zone-stories/THARNADIA.md) for the complete example.

## Later schema capabilities

All three versions reject dialogue milestones, personal recovery, kills, arbitrary scripted events,
coin objectives, and new reward fields. Do not deploy placeholders claiming those
events are tracked. The integration plan specifies the durable adapters and
credit policies needed before those kinds can enter a versioned schema.

## Schema version 3: optional preparation

Version 3 contains the same area/story fields as version 2. Each step can also
contain `"optional": true` or `false`; omission means false. Only a JSON boolean
is accepted. Versions 1 and 2 reject this field and retain their original behavior.
The journal labels optional preparation, still shows its live/recorded status,
and skips it when selecting the first required `Next:` action. Optional steps
never add gameplay prerequisites or change a terminal native receipt.

Twin Towers uses this for the garden belt: wearing it is necessary for guarded
garden movement, but delivering a supplied valid plant does not require it.
Use optional steps for a supported route, not to claim personal sourcing or
learned history. Conditional recipes, branch/reveal rules, all-stage campaign
completion, and scripted event terminals remain planned capabilities. All
native terminal arrays still mean any-of.

Deploy a schema-3 file with the matching server binary. Older binaries reject
schema 3 rather than silently guessing its semantics; rollback requires a
compatible sidecar revision as well. Existing stored receipts need no migration.

The [execution register](../design/ZONE_STORY_ROADMAP_EXECUTION.md) links full
source story dossiers, including unresolved reset/grant dependencies. Export
the evidence for a zone with:

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence twin_towers_forest
```

## Schema version 2: orientation and encountered people

Add these fields to the version 1 root and set `schema_version` to 2:

```json
{
  "introduction": "Explore the forest and the towers to uncover local requests.",
  "orientation": ["Start with look and exits."],
  "contacts": [
    {
      "mob_vnum": 13500,
      "name": "Alvinar",
      "keyword": "alvinar",
      "description": "Ask about his garden, then follow the flower checklist.",
      "topics": ["garden", "flowers"]
    }
  ]
}
```

The introduction and up to 16 orientation lines appear once the area is
discovered. **Do not name unrevealed NPCs in these fields.** Each contact
appears only after a saved physical encounter with that NPC prototype. The
root permits up to 256 distinct contacts. Keywords must be real NPC aliases;
the source audit and native bootstrap verify them. Choose a distinctive alias
so another NPC in the room is unlikely to intercept the suggested command.
Topics are up to 32 distinct command words. Verify each against native `M`
responses or the actual special procedure. A keyword is guidance, not a
learned-topic achievement. Empty topics are valid for item-only request givers.

Native quest givers are tracked even without authored contacts. An area visit
or group quest credit never manufactures an encounter. The player must be
awake and see an identifiable living NPC in the same physical room.
Invisible/undetectable NPCs, anonymous infravision shapes, remote viewing,
temporary placement, ships, arenas, staff, and disconnected characters do not
reveal contacts. Room arrival, visible room/detail inspection, accepted native
dialogue, and an NPC arriving beside a player can record an encounter.

A story becomes visible after meeting a terminal giver. When alternatives
use several givers, one meeting reveals the shared request, while only met
contacts are listed. Keep story prose free of unseen NPC names; use phrases
such as “the request giver” for unresolved people. Introductions, contacts,
and hints are authored prose, so this spoiler discipline is a builder duty.

Encounter records include character, season, NPC VNUM, room, and first time.
They persist in the existing SQL/flat-file bucket authority using domain
header `ZSQF|3` and `M` records. The first write upgrades older headers
atomically; versions 1 and 2 still load without guessed encounters. SQL bucket
schema and flat-file framing are unchanged. Older binaries cannot read the
new domain state; preserve a pre-upgrade snapshot for a planned rollback.

Discovery publishes the zone name and its exact selectable `quest zone`
command after saving succeeds. First meetings point to new guidance; first
recorded completion progress points back to the journal. Repeats do not
repeat those prompts. The ordered checklist marks the first outstanding step
with `Next:`. Item checks remain current possession and completion steps
remain durable receipts; no new gameplay prerequisites are enforced.

Audit required starter/town coverage with:

```bash
python3 scripts/zone_story_quest_home_coverage.py --check
```

See the [coverage register](../reference/ZONE_STORY_STARTER_HOMETOWN_COVERAGE.md)
for mapped areas, exclusions, and the remaining qualification work.

### Peril Peaks: separate linked deliveries, controls and unfinished investigations

Peril Peaks' [source dossier](../design/zone-stories/THE_MOUNTAIN_OF_PERIL_PEAKS.md)
and [zone sidecar](../../areas/story/nexus.story.json) demonstrate ten independent
deliveries with three optional earlier-exchange histories. A stud, parchment or
troll eye supplied by another player fits the same native contract without
creating a personal mining/hunting receipt. Current proof remains distinct from
the accepted producer receipt; consumed proof is not restored by recorded history.
Three reptile kinds and two tentacle kinds require each distinct material, while
an anaconda pair and a pair of draco eyes are each one object.

Builders can map the native questions and explain medallion/emerald/portal
prerequisites now. A carried NPC emerald needs exposed room custody before its
touch control can dispatch. The ghost holders' death path releases their contents
directly into the room; combat, custody and exposure still need a live journey.
Looking at twilight and touching the return sphere
are actual travel commands, not interchangeable enter hints. Discovery, meeting
the source, receiving a key, removing a block and arriving through a gate are
different facts; do not mark them all complete from one delivery.

The injured human's request for his companions' fate has no further native Q.
Their active connected placements and wandering do not supply a report endpoint.
Design whether the finale means investigation, a surviving subset, escort,
keepsake recovery or a returned report before authoring its future objective.
Completion arrays currently express OR alternatives; they cannot enforce an AND
campaign or five-companion requirement. Add deeper fields only alongside their
implemented parser/runtime, committed events, persistence and rejection/replay
tests. Dynamic extraction can suggest sources and links, while reward intent
(such as Gooran's ring promise versus actual book) remains a builder decision.

Alias, clue or dormant-route repairs belong in separate named fix commits where
practical, with explicit PR/news trigger, before/after and validation. Journal
guidance or a proposed companion finale does not constitute a shipped quest repair.

### Crakkaros' Liar: identical badges, mounts and oversized payments

The [source dossier](../design/zone-stories/CRAKKAROS_LIAR.md) and
[sidecar](../../areas/story/crakkaro.story.json) show six stories/five services
across eleven native exchanges. Four identical badge names need four separate
carried checks and helpful source labels. A fifth identically named corpse badge
does not fit. Maial/Malinok/Burnherf each have a paladin alternative; the fourth
kind is in a bookshelf. Do not turn the woman's kill claim into a personal-kill
requirement or let four copies of one kind substitute.

Dynamic extraction must include R-loaded mounts, rider association and actual
post-R E/G ownership. The centaur is the elf's mount; its existing equipment does
not complete a tail exchange. P resets select an actual matching container from
the object list, so shared bookshelf prototypes cannot guarantee a fixed room.
Committed UID/container/mount custody, source recovery and handoff require deeper
events beyond current possession and accepted producer receipts.

Burnhard's shield → bracer → ring → earring uses three optional producer histories,
with exact supplied equipment still valid. Four other part commissions and fur
work are independent services. Seventeen furs pay one hundred platinum but exceed
the fourteen-root durable limit. State that readiness describes possession and
acceptance remains guarded. Larger-batch/versioned recovery support is planned;
do not silently reduce the recipe or expose an unsupported native payment as
playable. Reset mode zero does not itself exclude surviving-recipient candidates.

The ice key, secret-door search, unlocking, opening, successful passage and a
future dragon finale are separate facts. The sculpture's envy switch expects a
block absent from the reset. Existing key access does not prove that switch
worked. Builder-approved fixes for badges, aliases, control/key intent or world
connections need separate clear fix commits and prominent PR/news proof. A source
finding, journal hint or proposed finale does not count as a shipped native repair.

### Rogue Plains: alternative recipes, foreign owners and actual effects

The [source dossier](../design/zone-stories/ROGUE_PLAINS.md) and
[sidecar](../../areas/story/roguerai.story.json) show seven outcomes covering nine
native exchanges, twenty-four contacts and sixteen optional checks. Each giant's
two potion recipes belong to one outcome. Separate cloud/storm producer histories
express optional earlier stages; one completion list would mean OR, not both.
The mediator still needs two different promise kinds. Four buffalo-flesh kinds
also look identical and need independent source labels/readiness; duplicates do
not substitute. Supplied promises or soul fit without personal producer receipts.

Optional key guidance is not a hard native prerequisite. Both obelisk and altar
are locked but pickable; hardpick bit two is not pickproof bit sixteen. Supplied
medal and successful lockpicking remain valid branches. Secret-door discovery,
unlock/pick/open, same-name portal identity, air-plane/mount rules and actual
arrival require successful targeted events, not inference from possession or prose.

Sijona's tokens resolve through Balance in Myrabolus. Keep that contract's owner
and guide the referral without copying its local credit or requiring personal
kills when supplied proof fits. Dynamic scans can find token supply/consumer
links, but full-zone restoration needs builder-defined effects and a scoped AND
campaign. No keyword, climb, actual silence or dracolich ritual terminal exists
merely because the current dialogue or reward mentions it.

R-aware sources must retain rider/mount/actual post-R E/G ownership. Keva and
other staging mobiles are loaded in connected rooms and can wander; their load
origins are not guaranteed current locations. Source-versus-handoff requires
committed operation/item UID and actual custody. Orc retirement/reset stock needs
an accepted episode rather than an unconditional daily promise.

Master sigil has a direct legacy seven-member worn-slot procedure, while a
separate adapter table lists eight and deduplicates. Document the actual assigned
path. One equipped sigil does not prove a full set or durable effect. Builder
membership/threshold intent and equip/actual-effect/cleanup/restart qualification
must precede effect objectives or lifecycle bug claims. Wording/item-type/alias/
set/route findings remain proposed decisions. Future native repairs need separate
clear fix commits and prominent PR/news before/after evidence; this journal ships
no actual native repair.

For the reaper reward, name and item type are separate: its wand type supports
a room-corpse target, whereas channeling staff targeting does not. The spell
requires a same-room level-46-or-higher corpse, a valid caster/form and available
control capacity; its dragon-scale requirement is commented out. Device activation
and accepted durable corpse transformation/control are separate stages. Follow
the committed corpse/pet outcome, including hostile awakening and recovery, if
a future builder defines a raising objective. Never award it from a charge use,
the reward receipt or an unrelated newborn-dracolich reset.

### Desolate: destructive containers, mixed fees and changing world routes

The [source dossier](../design/zone-stories/DESOLATE.md) and
[sidecar](../../areas/story/desolate.story.json) show nine outcomes/two services
covering eleven exchanges, twenty-seven contacts and sixteen optional checks.
The fill-tankard service can supply a separate story. Optional minotaur/smith/
driver and foreign letter histories explain progression without requiring personal
production when supplied proof fits. Distinct badges retain actual source/owner.

Inspect object trees before authoring recovery hints. The monkey is an item
container with another quest's chain inside it. Its accepted destructive turn-in
consumes remaining children. Warn the player to remove that chain; loose-carried
readiness sees neither contained proof nor valuable unrequired descendants.
Plan transaction-backed previews and an explicit reject/preserve/consent policy
for descendants, with nested/reconnect/recovery proof. Do not silently retain
items while accounting says they were destroyed, or invent live-animal rescue.

Rod + broken wheel + five platinum is a mixed recipe currently guarded with
accounting active. Preserve the fee and supplied repaired-wheel branch. Plan one
atomic item/wallet/reward/receipt/recipient operation, not independent debits;
coin-only purchase needs its appropriate commerce route. Native drink matching
uses kind rather than fullness: add real content predicates and accepted state
events before using drink-state readiness or changing the current offering.

Secret switches clear one blocked side; non-secret trial controls clear both.
Closed/secret still requires successful find/open/pass. Named tests do not impose
recorded guardian defeats, and the final button returns to the start. Define
attempt-scoped accepted controls, actual arrival/defeat/order/return and reset
policy before a gauntlet achievement. Equipment loot and real Master set effects
need their own committed equip/effect/cleanup/recovery evidence.

A reset-admitted timed random exit reroutes normal Desolate into the separately
owned invaded zone, changes closed flags and rewrites the target return exit.
Extract actual route generation/episode and accepted world-control transitions;
static prose and normal-zone receipts do not identify phase completion. Preserve
foreign ownership and builder-defined scoped AND campaigns. Dynamic scans find
these dependencies; source and executable lifecycle review still decide meaning.

Native Endurance/Courage direction text is corrected in separate `fix` commit
`b1ff082bc03ae579aceec97b5d3ebd878bf2eea7`, with original-fails/repaired-passes
source proof and explicit PR/news. Three absent shop-stock references and other
presentation/intent decisions are pending proposals. Keep implemented fixes,
guidance and new capability plans clearly distinct when reporting zone changes.

### Rift Valley Jungle: root counts, source instances and recipient meaning

The [dossier](../design/zone-stories/RFTJNGLE.md) and
[journal](../../areas/story/rftjngle.story.json) show 12 outcomes/12 services/three
non-credit referral/refusals over 28 exchanges, 49 contacts/30 optional checks.
Three matching eggs + one guard differ from three unique caches; six skins are
three of each kind; fifteen feathers are five of each color. One optional step
per distinct kind and a count safely explain current material readiness.

The 15-root feather request is guarded by the 14-root durable limit. Extending
the catalog alone would misrepresent acceptance: expand request/continuation,
ownership/tree/serialization/result/recovery bounds together, with whole-batch
conservation and rejection/replay/disconnect/crash proof. Seven mixed-fee crafts
need atomic wallet/item/reward/recipient/receipt settlement, retaining service
classification. Do not remove fees or split one native recipe to make it eligible.

Trace actual reset owners and instance supply. Green/yellow feathers sit on
different bright-quetzel loads; only one young-couatl load has hide. A ranger
can wander from local connected loads into four foreign planes. Secret item
search reveals differ from acquisition and another player's handoff. Add accepted
UID/revision/source-instance/reset/owner/reveal and transfer provenance before
claiming first-person recovery. Possession hints remain independent and read-only.

Inspect both prose and actual contract. Chief/scout ask for heads but accept
amulet/rod; the actual mangled head is rejected. The engineer's plans actually
load in elven guard quarters despite his cave clue. Journal guidance may show
the accepted proofs and real source while recording the wording/placement
questions for builders. Never silently substitute an imagined input or spawn.

Group two equivalent staff recipients as one OR outcome. The dragon prototype
has no active reset/transition, so guide the reachable woodcarver route. Four
elemental exchanges are independent; departures do not certify travel home.
Plan actual transformation/relocation/ghost events and an explicit scoped AND
campaign before making those story milestones. Faction exclusivity is a builder
choice; current native opposite requests coexist. Decorative compartment/axe/
recipe lore supplies no endpoint without new zone logic.

Actual item portal commands and return pairs are source evidence; successful
arrival/return and survival still need accepted runtime policy. Computed teacher
guidance, native M and procedural bartender quests are different roles. Generated
assignments need actor/start/target/type/count/payout/abandonment episodes and
foreign ownership before journal integration, not keyword or static recipe credit.

Five Moonhollow route words are repaired in separate `fix` commit `f5d5b2a5ccfbb9fccc8f5570079dd3394c74ed54`,
with focused original-fails/repaired-passes source proof and explicit PR/news.
Head/token wording, reward names, cave clue, absent dragon transition and stale
historical boundary are pending intent-dependent fixes. Keep implemented native
repairs prominent and separate from guidance and proposed capabilities.

### Transparent Tower: fragile access, common tokens and accepted closure

The [dossier](../design/zone-stories/TRNSPTOW.md) and
[journal](../../areas/story/trnsptow.story.json) use four independent exchanges,
fourteen contacts and eleven optional checks. Gullivier, Devilish and Lisa
return the scepter kind plus the same token kind. The librarian requires
three matching token roots and a loose-carried scepter. Three personal receipts
or three unique token sources are not enforced. Represent each optional history
separately; three contracts in one completion check mean OR, not ALL.

Trace the actual container and key route: illithid warped key → white mist
swirl key → Aceralde marble key → locked pickproof desk scepter. Opening
the desk triggers a room-wide acid trap. Normal use consumes each fragile key;
current carried-key checks must stay optional. Plan accepted lock/controller
transition and destruction settlement before claiming access. Source UID,
container/actor/reset owner and transfer provenance distinguish first recovery
from another player's supplied proof; a returned same-kind scepter is not
evidence of an unchanged UID.

Describe actual command portals and spoken-word doors. STARE, READ, CRY and
SCREAM differ from LOOK; direction portals can override ordinary movement.
SAY illusion/reality unlocks/reveals but leaves doors closed. Learned words,
controller success and actual arrival need separate accepted events. A key
receipt is not escape. The ordinary stair return conflicts with closet-only
prose; builders choose intended policy before a native route repair.

The imported epic rune already settles through durable zone touch. Consume its
committed result with participant/owner/episode/replay policy; do not add a
second payout or credit typed TOUCH. Globe/mace prototypes have foreign active
sources and their custom effects have separate owners. Full companion rescue,
corporeal restoration and world protection require defined ALL/actor/world
transitions, rather than extending receipt narration into fabricated evidence.

Inspect periodic support as well as reward submission. The shared epic
absorption loop accesses an object after extraction unlinks/releases it.
Plan safe identity-aware traversal, one destruction decision and conserved
item/tree/recovery accounting before qualifying absorption as a story effect.
The touch transaction does not prove safe absorption. A separate shared epic
repair with executable failure/recovery cases is pending; no live crash was
reproduced or repaired by this journal addition.

Aceralde pulse registration, illusion wording, unplaced trash stone bindings
and orphan desk/sign/portal remain pending intent/qualification findings.
No native Tower repair ships. Actual future fixes require clear separate fix
commits and PR/news trigger, before/after, validation and remaining limits.

### Tempest Court: exact multi-zone supplies and consumed branches

The [dossier](../design/zone-stories/TEMPEST_COURT.md) and
[journal](../../areas/story/airp.story.json) map all eight contracts into seven
outcomes, with 21 contacts and 28 optional checks. Equivalent Al'Hajib recipients
share an outcome. The native recipe, not rescue narration, defines acceptance.
Zieflia and Cloudseeker are story-only because D1 combines with reset mode zero.

Trace ordered reset ownership: F changes the active NPC to each follower; the
frost key belongs to the second following aerial servant. R changes E/G's
target to the mount, so the six living tempests carry the following swords.
Keep actor/prototype/generation/source custody distinct from reset comments.
Active accounting refuses legacy reset item issuance without admitted generation;
nominal rare loads and cap/percentage declarations do not prove current supply.

Each of Chan's five essences is a different item kind. Cloudseeker consumes nine
roots; Fearfrost consumes four, including the same fragment kind. Model the
fragment as a consumed material and optional producer receipt, not a persistent
campaign key. Permit separate supplies for both rewards. Foreign wisp/katana
consumers compete for actual objects but are not required predecessor stories.
Supplied proofs fit without personal foreign completion or first recovery.

Explain actual flight/mount/perception, hidden-side reset state, ENTER portals
and the palace's downward key door. Zero-break smoke/palace keys differ from the
two-percent frost key. Successful use and later recipe consumption are different
facts. Plan accepted unlock/arrival and UID/transfer events before awarding access
or source achievements. Current loose carried snapshots remain read-only aids.

Preserve addressed MA room-audience semantics. The sanitized index drops two
families with si'ciltron; keep the native apostrophe topic in prose, and expose
valid galzron/ecthius aliases without pretending that typing a word proves learning.
Add lossless auditing and punctuation-capable builder tokens to the future plan.

Define scoped ALL, actor departure/escort/restoration, divine/visibility effects
and episode ownership before claiming full liberation, god restoration or foreign
journeys. Use existing committed epic-touch settlement rather than another payout.
Eyepiece upgrade promises/prototypes, an empty portal, lightning trigger bits and
sealed rare staging require selected builder intent and actual qualification.

The two actual Tempest direction corrections ship in separate fix dc586e34c,
with original-fails/repaired-passes proof and exact two-word scope. News wording
and remaining live LOOK/traversal limits are explicit in the dossier/register.
Do not advertise pending proposals or journal hints as shipped native repairs.

### Caverns of Armageddon: loaded proofs, salvage containers and optional producers

The [dossier](../design/zone-stories/CAVERNS_OF_ARMAGEDDON.md) and
[journal](../../areas/story/hunt.story.json) map eighteen independent outcomes,
40 contacts and 27 optional checks. Native acceptance and actual availability
are different: Roland's bounty is defined but the global reset scan finds no
declared recipient spawn. Preserve the recipe, flag the blocker and choose a
builder location/lifecycle before a separate placement fix and daily admission.

The heads and creature parts are G-loaded objects, not automatically generated
sever-on-death achievements. F changes their owner: the fire elemental carries
the tendril and the following archangel carries the wings. Soldier salvage
uses closed, unlocked object containers. Recover one Myrabolan and one Wild
Card tag; do not present this as healing live NPCs. The prisoner's D0 receipt
does not implement the departure described in its prose.

Keep ordinary shaft/Veannan/general locks, hidden waterfall/grate and one-way
rift/ladder/disembark routes explicit. A key number on a flag-zero exit is not
a locked gate. Three route keys persist on normal unlock; Abbadon's cosmos
key has a 100-percent break chance and fits two pickproof locks. Qualify the
actual committed unlock, key destruction, shared-door state and arrival before
awarding access. Current carried preparation remains optional and read-only.

Archangel wings trigger GET/PUT sleep and reject that attempted pickup. Trap
activation, charge mutation and later successful recovery need separate
committed evidence. An unrelated wyvern wing never substitutes. Active
accounting refuses legacy reset issuance without admitted generation; reset
chance/cap declarations do not promise renewable daily proof or stock.

Blicatch's four exact parts yield two different amulets; the Dragon Queen
consumes both. Her acceptance permits supplied proofs without a personal
Blicatch receipt. Alexis in Myrabolus is an alternate producer of only the hazy
blue kind. Earlier history neither restores consumed objects nor reserves a
current pair. Treasure chests are quest objects rather than containers, and
their Myrabolus receipts, the lost-monkey reward and the foreign demon-heart
exchange remain owned by their real recipient zones.

Campaign restoration, immortal Hunters, exorcism, actor rescue and timed
Sunweaver arrival need builder-selected scoped objectives and accepted events.
Use existing committed epic settlement and verified effect application before
claiming those milestones; questions alone do not award keyword achievements.
Review target routing for the imported rotating spell pool with a native
dispatcher fixture. Do not mistake race/class/size fields for proc IDs.

This checkpoint ships no native repair. Mixed-company salvage, actor narration,
Roland placement, boundary direction and cosmos-key lifetime are documented
pending intent/qualification. Actual repairs need separate fix commits and
prominent PR/news before-after evidence; journal authoring is a distinct addition.

### Tribal Forest: exact ownership, reused keys and supplied independent stages

The [dossier](../design/zone-stories/TRIBAL_FOREST.md) and
[journal](../../areas/story/tribal.story.json) cover all ten exchanges as nine
outcomes and one refusal exclusion, with seventeen contacts/21 optional checks.
Map the two similar meat kinds honestly: larger meat → hunter grain → staff,
small meat → nest, while the bird's larger-meat return is excluded. Native same-
kind reward issuance does not prove that the original physical UID was returned.

Supplied grain fits without your hunter receipt; the five-material crystal
does not require your spotted-deerskin commission. Earlier receipts cannot
replace spent ingredients. The shaman provides his own narrated blood, sulphur
and oil. The bluebird's nest, room egg descriptions and the Queen's exact missing
egg are distinct. Five duplicate limbs cannot replace five different corpse
parts. Preserve all addressed M/MA aliases; ordinary questions award no credit.

Trace the actual source owner and slot: lookout-held crystal; bone inside an
unlocked container; horn on the following devil; root on first following tree,
egg on second. Loading rooms and reset chance/cap declarations do not establish
guaranteed current NPC encounters or admitted item supply. Read the apprentice's
real shop directly: its valid `#42235~` header differs from prototype headers.

The wife consumes the bluish key alongside three clothing kinds, while normal
kitchen unlocking may break it. Separate source possession, successful unlock,
shared-door state and arrival. PUSH/STOMP only clears configured blocking;
remaining secret/closed state matters. Match actual mirror STARE travel versus
orb viewing/selection/TOUCH HP effects and imported spell-pool outcomes before
adding semantic events. Do not turn magic grove or forgiveness lore into a
hidden native prerequisite, escort outcome or global campaign achievement.

The separate three-word direction repair f8090481d has original-fails/repaired-
passes and exact native-byte evidence in the dossier and PR/news register.
Trap codes 9/10, other text mismatches, selected-target safety and builder-selected
lore endpoints remain pending proposals; the shop needs no header-related repair.
Future actual native repairs need separate identifiable fix commits, player
triggers, before-after proof/live limits and ready news wording.

### Ironstar: effective access, guarded fees and distinct commission inputs

The [dossier](../design/zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md) and
[journal](../../areas/story/lornecro.story.json) map four story outcomes and
three equipment services. Wedding ring → Larra ring → Soulcatcher and crown
→ axe → paid key are independent accepted stages. Optional producer history
cannot replenish spent material, prove a current object’s source or become a
hidden prerequisite for supplied input. Native loose-only offerings differ
from VNUM-based key matching that also accepts a held matching object.

Trace effective door state: the loader interprets low-two-bit door kind, and
D resets open the tomb and magic grate despite their configured keys. Do not
invent a mandatory mold-before-crown cycle or Datherlion password step.
SAY only unlocks an actually locked grate and clears secret state; opening and
arrival are separate. The vault really has three forward locks: third-dragon
silver key, gravel’s large key and Dralor’s paid key. A key receipt is not proof
of every door, and descriptive globes are not physical travel controls.

Keep paid routes guarded until item roots plus normalized coin debit/reward
recovery are atomic. Material readiness does not certify wallet readiness.
Mail is independent; dagger and hammer use different molds and outputs but
share scarce scroll/broken-weapon kinds. Maltheas separately carries a broken
weapon and wields an intact blade; do not invent a required player breaking
action. Following guardian skeletons do not own the minion’s scroll, and only
one of the four baby-dragon reset instances declares the first vault key.

Robert’s chance, wandering and departure need admitted source/renewal fixtures;
random-room labels and teaching/letter/reunion/clan lore are not accepted
terminals. Copied object wording and river current direction remain pending
builder-reviewed proposals. The separate two-word direction repair b07b560cc
has original-fails/repaired-passes and exact native-byte evidence, plus ready
news wording. Keep any future actual repair equally clear in the PR/register.

### Brass: collectible coins, duplicate rewards and unfinished branches

The [dossier](../design/zone-stories/PLANE_OF_FIRE_BRASS.md) and
[journal](../../areas/story/brass.story.json) classify four story outcomes,
one equipment service and two exclusions. The collector’s two coins are
exact I item kinds, unlike normal C wallet fees. Herl’s C is a currency
reward. Separate these semantics before deciding whether an offering needs
unsupported coin-input settlement.

Yodono’s three heads yield two same-kind vials under one accepted outcome.
The spy’s six-head request competes for two of those head kinds: allocate
fresh physical objects rather than reuse prior history. Duplicate grants
need separate source ordinals and recoverable roots, without double story
credit. Supplied input does not prove original source or personal combat.

Trace both entrance golems, actual EX_BLOCKED transition, open and surviving
arrival separately. Front/rear door resets and distinct same-name keys can
provide different access routes; a key receipt is not mandatory history for
every route. The outward-only rare-load room may intentionally disperse
non-sentinel actors. Qualify actual admitted placement, wandering/perception
and renewability; do not add an entrance or force spawns from a birth-room
label. Fire-ward labels do not establish survival.

Keep the two-scale/7500-platinum bracer guarded until mixed-fee settlement is
atomic. Its lesser scale/fee return is a refusal, not a prerequisite. The
dying djinn’s empty recipe has no active placement or meaningful endpoint;
exclude named credit until a builder specifies its rescue/second-task intent.
Cold-blue-flame vials are elemental-form potions, not proved healing items.
Yodono’s missing ambient interval and copied direction/body/shop descriptions
are separate pending repairs, as are unverified foreign notes.

Actual fix d18758098 changes one Imix Avenue exit-description word and has
original-fail/repaired-pass, reciprocal-route and exact-byte evidence. Its
news sentence is prominent in the PR/register; any future native fix should
be reported separately from journal additions and these proposals.

### Tower of Darkness: physical campaigns and immutable quest owners

The [dossier](../design/zone-stories/TOWER_OF_DARKNESS.md) and
[journal](../../areas/story/lortower.story.json) map seven owned contracts as
six stories. The giant’s note and guarded cash alternatives share one accepted
key outcome; report support per branch independently of daily eligibility.
The pure-coin D1/reset-zero branch remains guarded even though its daily reason
says Story-only. A supplied note does not need the sergeant’s earlier receipt.

Amelia’s locket and five exact sword roots form one Dorthan bundle. Optional
rescue history explains the story; native acceptance only requires exact items.
Three planar keys yield one stasis key. Neither receipt proves personal source,
first recovery, kills, password speech, container search, key use, opening,
portal/fall survival or arrival. A portal-named object can be a trapped container,
and maze teleport objects can override ordinary directional exits.

Five physically local Tower requests currently belong to Braddistock by giver
range. Preserve those native receipts and the existing owning journal. Plan
explicit physical affiliation/referral/campaign references and discovery policy
separately; do not silently migrate owners based on filenames or birth rooms.
Azlion/Darrin’s D1 repeatability currently uses owner Braddistock mode two,
even though physical Tower mode is zero. Qualify physical spawn/retirement/
renewal separately from receipt ownership before actual daily use. The oak
entrance resets closed but unlocked in both directions, so its inside key
is not a required initial-entry receipt; both stasis doors reset locked.
Use effective reset/current state, including pickability, to explain access.

A future builder-selected owner correction needs versioned history compatibility,
static/runtime parity, focused regression and a separate repair/news record.

Azlion’s rift, Darrin’s sacrifice, Katalia/Isabia’s D0 departures, Sargon’s defeat
and three Earlion actors need specific actual endpoints and attribution before
new objectives. Holding-room roaming, entry topology and supply need qualification.
Hammer Testing proc messages and copied descriptions remain pending proposals.

Actual fix 3f1ecf2be corrects two direction words and four magic-keyword color
suffixes. Its regression runs production exact matching/reciprocal unlock and
retains closed state. Keep its prominent news entry separate from guidance and
future capability/native proposals; live speech/LOOK/traversal is unqualified.

### Mushroom Caverns: proof type, availability and distinct campaign versions

The [dossier](../design/zone-stories/MUSHROOM_CAVERNS.md) and
[journal](../../areas/story/mushroom_caverns.story.json) map three native
outcomes. Preserve two different half kinds despite identical display names;
producer history is optional and current possession does not prove original
source. The platinum in Ozman’s reward is not a fee.

Review actual prototype and dispatch policy before labeling an all-I recipe
supported. Ozman’s half is ITEM_MONEY; active-accounting ordinary item policy
excludes it and Kryz’s two-item finale is currently unavailable. Coin pickup
and drop merging differ from maintaining a quest root. Builder-selected type
repair needs compatibility for existing objects, pending reward grants and
interrupted offering/currency recovery. Keep guards pending qualification.

Source-comprehensive coverage also records missing goblet 1515, two unplaced
actors and Haz’on’wyz’s five absent dispersal destinations. Potential daily
metadata does not certify source/actor availability. Modern Underdark actors,
halves and seal differ from the legacy campaign; Winterhaven requires the modern
seal. Draknah’s actual golden goblet is a distinct investigation lead, not an
automatic substitute. Keep immutable owners and explicit physical discovery
separate; external encounters do not discover Mushroom Caverns.

Current topic schema cannot encode literal apostrophes. Retain valid aliases
from mixed families and explain the other native words in prose pending stable
topic IDs and escaped command tokens. Asking house questions is not allegiance
proof. Actual pool target/perception/arrival/return, closed/blocked state and
falling differ from ladder/lift/lava scenery. The second young aboleth follows
the last young M; a parental story would need actual builder-selected behavior.

No native repair ships in this checkpoint. Missing source/actor/dispersal,
money type, pool/obstacle/parent and unfinished-scene proposals must not appear
as completed player news. Any later repair needs a clear separate fix commit
and prominent PR affected-zone/trigger/before-after/validation/limits/news entry.

Haz’on’wyz also receives the shared teacher from ACT_TEACHER. Its `level`
guidance lists matching-class live runestones, without a quest or lesson
receipt. The shared handler lacks addressed-recipient/visibility resolution
and appends to a 512-byte buffer without bounds; plan actual-function recipient,
class and long-output qualification/hardening. No live crash or teacher repair
is claimed. External encounter publication first requires discovery of the
physical area; it does not automatically discover the Mushroom credit owner.

### Plane of Smoke: two deliveries, four services and actual access state

The [dossier](../design/zone-stories/PARA_ELEMENTAL_PLANE_OF_SMOKE.md) and
[journal](../../areas/story/smoke.story.json) distinguish the two native story
deliveries from Erk's four equipment services. Keep conversion/forging history
without achievements/dailies. A supplied ring, blade or staff needs no earlier
key exchange, personal kill or captive rescue. Two Hate copies do not replace
Hate plus Discontent, and shared staff keywords do not make upgraded139832
valid input139825. The ring is competing material across services.

Trace the actual current mobile, not just the last M declaration: the spectacles
G follows the second F25 mephit. Rare M/F, conditional chains, caps and admitted
item generation determine source availability. Potential daily metadata is
independent of playable renewal. Both keys have break value100; carry/hold,
accepted unlock, durable consumption, open, surviving arrival and reset are
distinct. Historical receipts do not certify today's access or restore supply.

Inspect typed shared behavior even without a literal local special. The forge
room uses Negative Plane terrain and schedules life-force drain. Greatsword
139831 directly resolves a packed permanent Power spell through current weapon
dispatch. Builder-selected terrain/spells/balance, approved effect authority,
existing-instance versions and combat/persistence/recovery qualification are
needed before repairs; do not silently replace spell IDs. Imported epic stone
359 uses committed group claims, distinct from its custody or heart delivery.
The literal default topic is an alias, not a wildcard; audience changes create
no observer credit. Rescue/cure/escort/scenery endpoints require actual design.

Native fix commit 0fff62e70 independently corrects the vault's south return
key reference and adds Discontent's own-name targeting alias, preserving old
aliases. The production access/name regression fails before and passes after;
key settlement/live travel/existing object migration remain unqualified. Ready
news: “The Plane of Smoke's vault key now works from either side of the
portcullis, and Discontent can be selected by its own name.” Keep the pending
forge/proc/earring/prose/scene plans separate from this shipped repair entry.

### Fishermans Wharf: quantities, optional supplies and alternative abilities

The [dossier](../design/zone-stories/FISHERMANS_WHARF.md) and
[journal](../../areas/story/fishermans_wharf.story.json) map five independent
accepted outcomes. One egg, four separate stick bundles and three pelts require
eight different physical roots; a single "bundle" item counts once. Four
bottles and four jellies likewise mean four separate exact items. Optional
cleanup/guide receipts explain bait/line sources, while supplied exact items
skip those histories. Duplicate actors share their native definition; aliases,
current quantities and story narration do not create extra credit.

Use actual actor identity and admitted supply: guarding eagle88910 carries the
egg while same-looking flying eagle88911 does not. Preloaded pelts/jelly/skull
do not prove skinning, spawning, personal kills or first acquisition. A future
source objective needs committed item/actor/source evidence; ordinary deliveries
remain compatible with supplied items. Keep cap/reset availability separate
from static potential-daily classification.

Check actual effects and access semantics. Snorkel must be face/nose worn;
belt waist worn or working spell/innate can provide alternative breathing.
Historical reward/loose/held custody does not establish an active effect. Totem
works as a key by exact vnum; unlock/open/arrival are distinct. Tree F50 starts
chance falling at command time, not immediate grounded arrival. Future effect/
alternative and accepted travel/fall objectives need actual qualification.

Shared fishing selects recognized loose poles, generates global fish, and does
not consume quest bait/line/net/jelly. Its success text/XP precede ownership
submission, whose result is ignored. Qualify accepted/rejected grants and
recovery before a separate publication-order fix or typed catch objectives.
Skull31320 belongs to Qin's foreign five-kind quest; NODROP handling and native
bundle consumption differ. Keep foreign ownership and supplied-route history
explicit. No native repair ships with this map; preserve earlier Newhaven terms.
Keep proposed builder decisions separate from actual fix commits and news.

### Northern Lakes: courier stages, source identity and retiring recipients

The [dossier](../design/zone-stories/NORTHERN_LAKES.md) and
[journal](../../areas/story/nlakes.story.json) keep six accepted stories
independent. Tamara packaging → Aerin delivery → Tamara return note is a useful
chain, but exact supplied bottles/notes skip earlier personal history. An old
receipt does not restore consumed supplies or grant another stage. The current
any-contract semantics cannot express a parent requiring all three stages;
use independent entries and explanatory links until parent display is designed.

Two exact scales are different physical roots, not proven different sources.
The green/old dragons share targeting aliases and scale kind. The green dragon
also accepts a vial and leaves, so it is both supplier and retiring recipient.
Record actor/source conflicts and peaceful/reset alternatives for qualification;
do not invent personal kills, stricter provenance or duplicate materials.
Quest golden vial75274 differs from ordinary potion75225. Maur's heart is a
preloaded material despite blue-dragon/expedition lore; no accepted kill/rescue
or experiment endpoint is established. Tein's qc_action is ambient, not a topic.

Use actual code for travel prerequisites: boat movement does not suppress
currents, arrival/command can both attempt sweeps, warnings precede movement,
and F chances occur at command time. Accepted access, alternatives, current
cause/result and surviving fall need real evidence. Native D acceptance means
recipient retirement; prose alone cannot create recall/flight/cure events.
Artek's visage continues to Aevenyl's foreign request, whose ownership and
prior map remain unchanged. `_noquest_` does not forbid the explicit recipe.

Separate shipped repair5a2b93d6d corrects only two pile-of-bones direction words
and has a precise PR/news entry. Artek's empty bespoke response, Tamara source
narration and inactive Melbh announcement naming remain pending builder intent
or qualification. Keep those proposals distinct from fixed behavior. Active,
ready accounting remains mandatory; frozen persistence/recovery stays separate.

### Kobold Settlement: paid services, source limits and custom guardians

The [dossier](../design/zone-stories/KOBOLD_SETTLEMENT.md) and
[journal](../../areas/story/kobold.story.json) keep smelting, shield crafting and
gem inspection as supporting services, with one guarded spectacles story.
Earlier smelt/inspection receipts are optional; supplied exact materials skip
history and spent materials stay missing. Coin fees are explanation until the
accounting adapter is qualified, not a live readiness guarantee. None of these
four contracts is a daily candidate. Service classification removes three
support exchanges from the zone achievement count without editing native recipes.

Review native selection order when recipes share inputs: boot prepends Q and
G nodes, so spectacles can reach its unsupported coin goal before gem
inspection. Qualify gem-only, gem+frames and full paid routes before changing
selection. Eight nuggets versus a normal five-live-copy reset cap is a supply
qualification lead; saved supplies and forced resets can differ. Do not label
the commission impossible or silently change the cap.

Use the actual placements and shared code for gates: switches clear blocking,
raw translated speech unlocks the bone door, and Jkyl's barrier remains a
separate actor condition. The repaired guardians use altar1481/tomb1482/
pit1484 and ledge1483; the demon follows the room list. Custom death piles,
forced arrivals, combat start and surviving escape need accepted causal events,
not narration. Imported epic touch and ambassador memory keep their own credit
authority. Rod lore without a source/reassembly endpoint and the guard-post
inn binding need explicit builder decisions.

The guardian change is isolated in fix02788c573 with original-fails/repaired-
passes evidence and a prominent news sentence. Fees, caps, legacy forge,
parchment learning, rod, inn and wording remain pending plans. Active, ready
accounting is mandatory; frozen recovery stays separate. Preserve the
[PR checkpoint archive](../design/ZONE_STORY_PR_CHECKPOINT_HISTORY.md) and prior
repair/news records when shortening the current PR description.

## Troll Caves: same names, dynamic handlers and prerequisite mismatches

The [complete example](../design/zone-stories/TROLL_CAVES.md) maps all five
native exchanges to one story and four paid supporting services. Both emerald
kinds share an alias, and base/enhanced mace kinds share the same displayed
name. Bind exact kinds; an enhanced mace does not satisfy the base-mace input.
Use optional producer receipts and current-material checks so legitimately
supplied items skip personal crafting, mining or student-kill history.

Dialogue demanding Farghan's wand does not add that item to Q148. Document
the actual recipe and intended-lore gap, then let the builder select terms
before changing the recipe or awarding wand credit. Likewise, a static egg,
slave followers, periodic forge narration and class/level guidance are not
accepted hatch, rescue, crafting or learning endpoints.

Zero literal assignments can coexist with type/flag/table or generic dispatch.
Here both PUSH switches dynamically bind, and the sign uses PUNCH while the
orb uses TOUCH. For secret exits, shared switches leave reverse blocking and
closed/secret bits. State changes, attempts, successful arrival and surviving
return need distinct accepted evidence; water-flow text does not declare a
current. Keep mixed-fee commissions/forge guarded with accounting active.
All actual content repairs need separate fix commits and clear PR/news
behavior/proof/limits; this example only records pending builder decisions.
