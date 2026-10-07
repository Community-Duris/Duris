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
substitution tokens. Story and step IDs use lowercase letters,
numbers, hyphens, and underscores, at most 64 characters. `source_area` preserves
the exact registered filename, including existing ASCII capitals such as
`Voluntown`; use `Voluntown.story.json` for that area. NPC keywords and topics
remain lowercase. Differently cased unregistered names and path separators are
rejected. See the [separate loader repair](../design/ZONE_STORY_SOURCE_AREA_CASE_FIX.md).

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

## Centaur Villages: identical halves and optional story history

The [complete example](../design/zone-stories/CENTAUR_VILLAGES.md) maps seven
exchanges to four stories and three supporting inspections/briefings. Two
halves from different story routes are the same kind93313: require count2
distinct loose roots, not two different named kinds or one reused item.
Two final reward items remain one accepted quest outcome. Optional producer
receipts explain the usual route; supplied/reset letters, horn, staff or
halves may skip personal history. Earlier receipts never replenish spent items.

Same-kind inspections do not promise identity preservation. A disappearing
giver's accepted receipt does not prove its retirement, both reward items
currently held, a new reset episode or daily renewal. Qualify those facts
separately before authoring richer actor/ownership/reward stages.

NOSHOW display and actual named selection differ: the shared list lookup
allows hidden controls/containers by name. Audit real GET/PUSH dispatch and
ownership before calling a source inaccessible or changing a flag. Likewise,
actual currents and mountain falls need admitted movement/final survival
evidence, while level guidance, grief, cure and title prose remain narrative.

Six misleading direction clues were repaired separately in e456b3403 with
original-fails/corrected-passes topology checks and a prominent news sentence.
Keep actual native fixes distinct from journals and pending builder plans.

## Opal Phoenix: source reveal differs from a hidden carried reward

The [complete example](../design/zone-stories/OPAL_PHOENIX.md) maps three
independent outcomes, nine contacts/eight aliases and four optional checks.
Quill→sand→quill is a production cycle, without mandatory personal history;
supplied exact inputs work and spent items stay missing. The sand’s container
name does not make it an actual container. Same-named students/bears may have
different contracts and sources, so use actual kinds and runtime identities.

SECRET display/name rejection differs from the shared NOSHOW lookup exception.
The sand reward lacked an ordinary SEARCH path in inventory; separate fix
ac8e2de48 clears only its SECRET flag with production-function original-fails/
corrected-passes proof and a prominent news sentence. Hidden chest/key/remains
retain source SEARCH paths. Do not globally clear hidden flags from quest inputs.

An exact key’s100% break value is an intentional cost, not automatically a bug.
World unlock can precede committed key destruction; successful access and key
spend need separate evidence. Optional mask GET has no native delivery endpoint.
Likewise capped/party XP, each output, D1 removal of recipient/remaining stock,
fresh reset and renewal are distinct from one accepted completion receipt.

Audit computed table/flag bindings as well as literal assignments: Azalea’s
teaching and real inn rooms exist despite absent raw teacher/innkeeper markers.
Trading/epic teaching stay unavailable with active accounting; ordinary rent
has independent admission/save rules. Spell/faction/exam/bridge prose does not
invent an accepted effect. Keep source repairs/news separate from journals,
future spelling/effect choices and saved-instance operational remediation.

## Myrabolus: returns can carry a story, and names do not establish identity

The [complete example](../design/zone-stories/MYRABOLUS.md) maps13 outcomes
and3 support entries,21 contacts/all4 aliases and24 optional checks.
Markam returns the note kind **and adds a new half**, so its receipt is a
story stage. Balance only replaces/refers a letter; its receipt is support.
The native daily returned-input exclusion stays conservative. Classify each
actual result; do not globally turn inspections into achievements or dailies.

Original study76068 and delivery copy76069 share names/aliases but differ in
kinds. Two keystone halves also share a name but need distinct kinds. The
foreign producer receipt can be an optional completion check without changing
ownership; supplied exact inputs skip it. Same-kind returns are new reward
instances, not proof of original UID continuity or fresh inventory.

Successful SEARCH of a hidden/invisible source needs actual perception; a
transient half/key can dissolve on DROP, and100% key break is an intentional
cost. Awake dispatch requires waking Alexis. These are source/access hints,
not supported semantic objectives. A quest item called a chest is not an
OPEN container, a fixed FLOAT life raft is not the requested boat, and an
item monkey is not a living pet. Keep exact type/flag/UID/source roles clear.

Review actual computed and imported bindings: Andryn’s smith, Roland’s epic,
the counter’s guarded locker entry, rotating spell fountain and alarm ward
all exist beyond local literal leads. The Master adapter table’s longbow
membership is not proof that the longbow is bound. Crew payment ordering,
guarded fees and source renewal require accepted continuations and qualification.

Missing treasury kind, unrewarded Roland/load-room intent and wording remain
pending builder decisions. New launch/rescue/healing/fate/mount endings need
explicit accepted effect adapters. **No native repair ships in this checkpoint.**
Later actual fixes must have clear separate commits, concrete before/after
proof and prominent news sentences; keep journal guidance and proposals distinct.

## Building Depths-style corpse, alternate and expiring stories

Use the [Depths dossier](../design/zone-stories/THE_DEPTHS_OF_DURIS.md) and
[authored journal](../../areas/story/surfacekeeps.story.json) for fifteen native
bindings, nine outcomes, three services and source-aware guidance. Accounting
must be active and ready. Discovery is separate; all 109 conversation aliases
are clues rather than individual learned-keyword achievements.

Map ten body parts as ten distinct loose roots of the exact kind. CARVE places
new pieces inside a corpse; GET is a separate step. Nine ordinary part choices
are not ten victims, and arms/legs changing runtime type do not change their
kind. A custom death proof can start on the ground and later become another
kind: distinguish first source recovery, supplied possession and expiry lineage.
The current journal shows materials and accepted offerings; it cannot prove
personal carving, personal kills or a first recovery from command text.

Group Ungalen's four brew recipes as one ANY outcome, retaining each native
receipt and payout. Keep Tok's two identically named feather kinds distinct.
Naltem's foreign five-brew ALL recipe owns its own receipt. Current materials
are optional hints, not credit. Supplied dagger+trophies skip Mystardala's
earlier ball exchange; optional history never restores spent supplies or adds
a wielded-dagger kill requirement that the native recipe does not contain.

Guide SEARCH, hidden tiny key, UNLOCK/OPEN and ball GET for the pickproof chest;
show paths/places to players while keeping numeric evidence in builder docs.
Source cap/chance, map walls, NPC dispersal and a missing destination are distinct
facts. Six dangling edges and wall/stair placement need builder intent; blue
items' pink prose is a possible narrow wording repair. No native repair ships
here. Any later fix needs a separate clear commit and prominent PR/news evidence.

For future custom integrations, declare accepted source/corpse/generation/part/
actor/tool/UID lineage, publication/transfer/expiry, mandatory versus supplied
policy, indexed rewards and actual recipient retirement. Qualify intended failed
carving costs separately from allocation/placement failure, then busy/partial/
replay/restart. Never author unsupported CARVE/kill/rescue/expiry steps as though
schema-three readiness already records them.


## Building IceCrag-style access and transformed-source journals

Use the [IceCrag dossier](../design/zone-stories/ICECRAG_CASTLE.md) and
[authored journal](../../areas/story/icecrag.story.json). Complete coverage can
include an explicit exclusion: Masha's native recipe requires missing object 6551 and
does not agree with the food dialogue. Do not invent a prototype or a replacement
ingredient. Classify the exact binding with a clear reason, show the unfinished
project in orientation/contact guidance, and exclude its achievement/daily unit.
A later builder-selected native repair gets its own fix commit, failing/passing
proof, limits, news-ready sentence and requalified journal revision.

Keep same-name kinds distinct. Three speech pages require one of each numbered
kind; two white and two elven bottles require four physical roots. Masha's juicy
onion cannot replace the Viscount's ordinary onion; the accepted borrowed book
differs from another same-name prototype. Source actor names and dialogue alone
do not identify an item. Use optional current-material steps, loose counts and
clear hint text without changing native acceptance. The earlier artist receipt
is optional for supplied shoes and cannot recreate spent or worn shoes.

Describe access as actions: speak Auril, open the unlocked doors, then travel.
Admitted speech and actual paired lock changes differ from ASK clues, shared
access or arrival. RUB is the pedestal/orb command. Never add a mandatory
self-unlock or source kill merely because it makes a neat story. Masha's GET
interference, NPC bodyguard rescue, wolf conversion and death→vapor need precise
accepted actor/control/custody/lifecycle adapters before historical credit.

Treat helping the freezing sergeant as a meaningful story with a fee blocker;
paid milk/key purchases remain services. Ready garments do not pay coins.
World-cap-one elven bottles, rare/holding NPC routes and D1 recipients need
actual supply and reset qualification before daily availability. Every new
discovery/encounter/journal/achievement/daily update requires active and ready
accounting. Keep broader banquet/shroud/construction/rescue campaigns in builder
plans until their native endings and event evidence exist.


## Cloister pattern: supporting refusal, supplied shortcuts and access hints

Use [Cloister's journal](../../areas/story/cloister.story.json) and
[source dossier](../design/zone-stories/FATHER_TELS_HOLY_CLOISTER.md) when a native
exchange completes mechanically while refusing the player's narrated goal.
Tel consumes and replaces a recommendation note but never admits a student.
Classify that exact binding as a supporting service with no achievement/daily
unit. Receiving the same kind does not mean the original item identity survived.
Keep formal admission or training in builder plans until an accepted endpoint
exists. The disciple's separate experience/clue outcome remains a story.

Current note/ring checks and earlier Mahr/priest receipts are optional. Players
may receive supplies from someone else; do not impose a personal source kill,
prior producer or self-unlock that native acceptance does not require. Receiving
a key differs from retrieving a ring, and an earlier receipt cannot replace a
spent, worn or nested current ingredient. Name exact robes/head/tome/poison
materials and actions without exposing maintainer IDs in player instructions.

Audit type-based automatic bindings as well as literal assignment tables.
Cloister's no-show switches use SAY Khildarak and PUSH statue/boulder/tapestry;
PULL and ASK are different commands. A secret exit still needs successful local
SEARCH after BLOCKED clears, and the other side may need its own reveal. Describe
unlock, open, reveal/GET and travel as separate actions. An acid-trap attempt can
hurt the player and consume a charge while rejecting pickup, so it earns no
first-recovery evidence. Keyword guidance alone earns no learned-topic or quest
achievement.

Keep local and foreign egg consumers independently owned. Source caps, rare
placements, retiring givers and actual reset renewal limit availability even
when seven pure-item outcomes are potential daily candidates. Require active,
ready accounting for new tracking; keep frozen recovery separate. Builder-owned
ALL/ANY campaigns need accepted endpoints and supplied branches before richer
progress cards can present them as completed stages.

The Mahr letter→tablet caption repair is isolated in fix commit 208a56840 with
original-fails/repaired-passes proof and a news-ready sentence. Other admission,
source-clue, deadline and orphan findings remain proposals. Every actual native
repair must stay clearly identifiable in its own fix commit and PR/news ledger.


## Turolopolis pattern: exact colours, source conflicts and foreign givers

Use [Turolopolis's journal](../../areas/story/willem.story.json) and
[complete dossier](../design/zone-stories/RUINS_OF_TUROLOPOLIS.md) for several
independent native offerings with useful optional progression. Five different
badge colours need five count-one carried-item steps, not one ANY-kind list with
countfive. Each badge also unlocks a matching mirror hatch. Receiving, wearing,
using and offering a badge are different facts; a supplied badge need not prove
personal rescue. The white badge does not substitute for a required colour.

Link Lothrell's memorial optionally to Kurtukr's lesser-blade/stinger upgrade.
An exact supplied blade bypasses producer history; earlier acceptance cannot
restore a spent, equipped or handed-away blade. Use actual RUB/ENTER commands
and the plaza's north–south statue row, followed by local SEARCH/UNLOCK/OPEN as
needed. Handled commands, shared access, fall attempts and accepted arrival
remain distinct. Do not add achievement credit for each question or book chapter.

Audit source and recipient survival together. The minotaur carries the emissary's
letter, but departs with remaining stock discarded after ooze acceptance. Killing
for the letter removes the same current recipient. Explain that branch and keep
supplied/earlier-generation proof valid; qualify an actual living-source transfer
before promising it. Do not invent a reward drop or mandatory personal kill.

Lothrell's physical Surface encounter and the home-zone memorial ownership are
independent of actual local discovery. Preserve the real source and owner while
planning visible home-journal links. Holding-room/follower dispersal can reach
public rooms or a sink; initial placement does not guarantee current availability
or justify relocating rare NPCs. Review actual one-based flags and reset episodes.

Narrated memorial, fountain purification, freedom and spell-field collapse need
explicit builder intent and accepted endpoints before richer campaign cards can
claim them. Existing sidecars author meaningful grouping/guidance; static data
alone cannot discover every intended story. Require active, ready accounting,
indexed settlement and qualified renewal for new progress/daily credit. Frozen
recovery remains separate. No native Turolopolis repair ships; later repairs must
be identified in separate fix commits and prominent PR/news proof and wording.


## Ixarkon example: preserve identities and explain unavailable preparation

The [Ixarkon dossier](../design/zone-stories/IXARKON.md) and
[revision-two sidecar](../../areas/story/ixarkon.story.json) preserve all three
existing story IDs while classifying two stories and one supporting service.
An optional banker receipt explains the amulet route; it never requires that
route for supplied items or substitutes for a spent/worn/nested amulet. Give
players all22 native aliases, distinguish the pacing recipient from same-named
elders, and keep red/black/bone-white skullcaps separate.

The cap-plus1000platinum route is refused while active accounting is enabled.
State this availability beside a valid supplied-amulet alternative; current
item readiness is not a live paid-transaction qualification. Paid hireling and
locker services also have their own guards. Source acquisition, sender provenance,
reward custody, personal source defeat and terminal acceptance need separate
evidence. Do not turn each ASK keyword into another achievement.

Room prose is an intent lead. The bridge starts open and its switches only
unblock it. The veil chamber has no ordinary ingress, and a separately assigned
entrance room is absent. Builder should define controller/ceremonial/racial/
return intent, guard assignments and qualify accepted actual movement and
restoration before adding runtime objectives or repairing those mechanics.
The two stories do not implement a slave liberation or diplomatic campaign.

Actual repairs must remain easy to announce: Ixarkon's12 direction words in11
rooms ship in separate fix commit7baa78c3c, with reciprocal-route regression,
exact scope, before/after table and news sentence. Future mechanic repairs need
their own separate fix evidence. All new progress requires active, ready
accounting; frozen reward recovery remains separate.


## Du'Maathe example: exact batches and loader-aware access

The [Du'Maathe dossier](../design/zone-stories/DU_MAATHE_CASTLE.md) and
[schema-three sidecar](../../areas/story/mntcastl.story.json) classify four
lord outcomes and four supporting potion services. Keep sand AND the foreign
recipe separate; the native exchange consumes both for three granular potions.
An earlier batch receipt is optional for a supplied lord potion. Foreign hermit
history is optional for a supplied recipe and retains the foreign owner.
Neither history restores spent current material or proves first source recovery.

Read the loader and D resets before writing access steps: raw high hidden bits
are discarded. The waterfall and prison portal start visible; the northern
prison wall and catacomb gate are secret through D resets. The onyx, decayed
and tooth keys break on ordinary successful unlocking; the golden catacomb key
does not. Actual public/picked/supplied alternatives remain valid. Described
water, bridge controllers and spoken wards are not automatic prerequisites.

Do not invent a sapphire drop for the unresolved blue-tinged horn. The crystal
dragon's white horn, onyx dragon's blackened horn and shop iron wand are distinct
items. The white flower's hiding place requires no gardener belt. Explain paid
foreign clothing and epic-skill unavailability under active accounting, including
valid supplied alternatives. Keep actual potion use, dragon victory, NPC healing,
prison rescue and notebook research out of completion until explicit events exist.

Native repairs need their own scope and news proof: separate fix45bb3c948
corrects one northwest-parapet direction word; source, price, visibility and
campaign findings are still plans. All new progress requires active, ready
accounting; frozen obligations retain their separate recovery path.


## Tundra example: complete sets and a departing giver

The [Tundra dossier](../design/zone-stories/TUNDRA.md) and
[schema-three sidecar](../../areas/story/tundra.story.json) classify six story
outcomes and one paid service. Keep old, green, magenta and black books as
four distinct current rows, and clam, pike and lobster as three distinct rows.
Four copies of one book or rations do not substitute. Bom's foreign painting
receipt explains only pike/lobster supply; hairy crab is not the missing clam.
Keep that receipt foreign-owned and optional for supplied seafood.

Eleadora's boots exchange may use the earlier book reward or supplied exact
boots. Her head exchange is independent and retires the actor. Explain returning
boots first if the player wants both rewards in one encounter; do not enforce
that order as a new prerequisite. D retirement does not by itself exclude an
item-only contract from daily candidacy. Actual giver presence and renewal still
need qualification, particularly with branching loading rooms and traps.

Read automatic bindings and loaded state. The working mirror's type29 installs
PUSH behavior; its forward blocked passage differs from the reverse secret/closed
door. Onyx Stairs and workshop entrances instead start visible/closed through
D1 despite raw high hidden bits. Neither SEARCH nor a mirror key is required
for those visible entrances. The two village keys open different flaps and break
on ordinary unlock. Earlier history never replaces current materials or key.

Validate actual fishing conditions: the named village docks are coded as land,
and the fishmonger has no configured shop. Do not advertise a guaranteed local
catch/vendor. Preserve valid foreign/supplied routes and active-accounting paid
guards. Dedicated fishing origin, personal kill, mirror use, key break, actual
world peace and actor renewal require semantic events before completion credit.

No native Tundra repair ships with this journal. Builder decisions about shops,
routes, sectors, rarity and captions belong in the dossier; implemented native
fixes need a separate clear commit and PR/news proof. All new tracking requires
active, ready accounting; frozen obligations retain separate recovery.


## Fields Between example: shared stock and a roaming chain

The [dossier](../design/zone-stories/FIELDS_BETWEEN.md) and
[sidecar](../../areas/story/fields_between.story.json) show seven exact outcomes.
Timmy and the professor each consume a dark-mithril bar; the professor also
needs a torture box. Explain uncommon shared supply without changing its odds.
Timmy's letter history is optional when an exact supplied letter reaches his
mother. She can be physically in Scorched Valley while her receipt belongs to
Fields Between; foreign seekers keep foreign owners.

Name all five different head kinds even when their native names look alike.
The monkey manuscript is actually worn as a shield; bananas and the prepared
body are miscellaneous quest items. Guide exact loose custody without inventing
reading, eating, corpse creation or personal victory prerequisites.

Trace automatic type25 ENTER behavior alongside resets. Rift71030 is fixed on
the floor with original TAKE=0/weight1,000,000. Earlier portability fix05eeca928
was incorrect and withdrawn by correctiond2a64432d: inventory ENTER would make
this unlimited portal an unintended portable escape tool. Native quest prose
alone does not authorize enabling disabled mechanics. The shaman's delivery
source remains unavailable; builder intent must choose a non-teleport proof,
source-bound interaction or recipe retirement. Do not advertise pickup as fixed.
Existing fixed travel remains separate from any accepted native supplied-item
receipt. Narrated escape/restoration/reunion still needs explicit campaign facts.

Active, ready accounting remains mandatory. Current availability, source versus
gift, portal travel, actor retirement and reset renewal need actual admitted
state. Synthetic receipts establish projection/replay/recovery behavior only.


## Moregeeth example: recipient availability, real keys and trapped containers

The [dossier](../design/zone-stories/THE_TOWN_OF_MOREGEETH.md) and
[sidecar](../../areas/story/goblinht.story.json) retain seven existing IDs and
exact bindings, with five stories/two paid services. Explain Gimbatul’s pouch →
magical key → four planar components as optional preparation; supplied items
and open public routes skip personal history. Moreg wears Glub’s sword, and
Gimbatul leaves after the crown: shared actor availability can affect another
card without enforcing an artificial quest order.

Read actual types and reset parents. Moreg’s key reward is a lockpick, the
component pouch a potion, and Ungal’s letter is nested inside a trapped desk.
The sticky, magical and rusty keys open different doors. Four different planar
kinds are an ALL set; five bat skulls need five separate copies. Paid item/coin
crafts retain their exact accepted receipts without story achievement/daily credit.

Check command constants before writing travel hints: STARE flame139, TOUCH
crystal320, ENTER mirrors7 and SOUTH smoke3 are real routes. Plane names and
narrated magical protection do not supply a new campaign controller. Questions,
opening, reading, pickup, traversal and accepted outcome are distinct facts.

The desk’s actual negative-keyhole repair7b916b887 is separate, one field only,
retains the lock/trap/letter exchange and has original-fails/repaired-passes
source proof and a prominent news sentence. Skill rolls and trap handling still
apply. Copied captions and threatened ghost/assassin campaigns remain pending
builder decisions. Active, ready accounting is required for new tracking;
frozen recovery and source-versus-played qualification remain separate.


## Ceothia example: one choice, independent rewards and real effects

The [dossier](../design/zone-stories/CEOTHIA.md) and
[sidecar](../../areas/story/ceothia.story.json) use six cards for nine recipes.
Put all four retiring guild bargains on one ANY card; retain every native
binding and receipt while counting the shared outcome once. Badge, horn and
thread rewards from Lenbrea are independent; earlier history and key checks
are optional guidance. A narrated surviving-guild or repaired-timeline campaign
needs explicit admitted stage/finale predicates before completion credit.

Check real types, flags and parent context. Four leader badges differ from the
common reward. Two TAKEable OTHER crates are nested in a fixed wagon. Its
compatibility HARDPICK bit2 does not mean PICKPROOF bit16: ordinary PICK/KNOCK
are possible. The tiny iron key may break; Lenbrea’s two access keys break on
successful unlocking. Past and Future Ceothia’s portals and sources keep their
own zone ownership; supplied proof bypasses personal travel.

Pool62 has an actual DRINK agility handler, while the skill-beacon table is
dormant here. Target selection, cooldown admission and a positive committed stat
delta are separate from a key receipt or command text. The existing pool target
ambiguity needs its own repair, regression and news. Captain’s legacy scroll
recipe is separate from guarded epic PRACTICE; missing fresh tablet supply and
unbound scroll learning need builder decisions. qc_action timers are ambient,
not addressed ASK topics or command IDs. LIST/HIRE and ordinary shops/inns remain
support services without automatic story credit.

All new discovery/encounter/journal/achievement/daily credit requires active,
ready accounting. Source review and synthetic receipts do not qualify actual
source/trap/door/travel/effect/learning/crew/offer/settlement or daily renewal.
No native repair ships with this journal checkpoint. Future implemented repairs
need a separate named fix commit and prominent PR news; proposals stay labeled.


## Braddistock example: keep physical location and credit owner explicit

The [zone1350 dossier](../design/zone-stories/BRADDISTOCK_MANSION_1350.md) and
[sidecar](../../areas/story/brad.story.json) preserve five native IDs/bindings
while explaining their actual Tower locations. A giver's current VNUM range
sets credit ownership; its declared area/reset room can differ. Sidecars cannot
currently borrow a foreign binding or discover the owner from an encounter.
Record the mismatch, source/recipient episodes and a physical-referral plan;
do not silently migrate native receipts or imply reliable daily renewal.

Use ten optional preparation checks: nine exact loose inputs and one earlier
Isabia receipt for Danthas. Five different Star Stone pieces are distinct kinds,
not five copies of any piece. The conjurer's held staff must become loose. The
head is fixed carried proof; ring type13 TRASH and bone key type12 OTHER are
valid exact offerings. Supplied ring/materials do not require personal source,
combat, earlier receipt, escort or travel. An old receipt cannot restore supplies.

Local books, extra descriptions, secret cellar, skeletons, alchemy and smuggler
cave can explain an exploration story without inventing native completion.
Validate types, bit positions and actual bindings: skeleton135040 has NOLOCATE,
not invisible/secret; book flags5 are closed/unlocked; world door kind2 is valid
pickable encoding; boat type22 is not teleport; local maul135071 is not older
bound1372. Missing strongbox key source still permits ordinary PICK/KNOCK where
admitted. Builder intent is needed before adding stock, lab conversion or voyage.

Separate fix5b4c4f34a restores the spirit's refusal speech to the blocked player
with a newline. Its threshold/staff bypass and observer messages stay intact;
original-fails/repaired-passes isolated production-function proof is distinct
from a played network journey. Keep native repairs in named fix commits and
prominent PR news; stock, prose, campaign and physical-referral proposals remain
labeled. Active, ready accounting is mandatory; frozen recovery is separate.


For Fields Between, the owner confirmed that the original non-takeable flag
was an intentional hotfix for a game-breaking escape mechanic. Required builder
follow-up ZSQ-FIELDS-BETWEEN-RIFT-HOTFIX-REPLACEMENT must preserve that protection
while designing a proper quest proof or fixed-source interaction. Prefer an
inert non-teleport remnant; version changed native inputs/receipts explicitly
and qualify source/gifts, PvP restrictions and accounting settlement/recovery.
The [full acceptance checklist](../design/zone-stories/FIELDS_BETWEEN.md) keeps
this pending work separate from shipped fixes and news.


## Venan'Trut example: a remote switch and a foreign progression

The [dossier](../design/zone-stories/DESERT_CITY_OF_VENAN_TRUT.md) and
[sidecar](../../areas/story/desert.story.json) use eight independent native
cards. Goranon gives the medallion Eriic requests, but Eriic accepts a supplied
copy without earlier personal history. Put the earlier receipt and loose garb
on optional steps; the exact terminal stays required. Winterhaven's ambassador/
paired fabrics and Shipyard map contracts keep their own owners. A local source
or item name does not make a foreign terminal a local achievement.

Inspect every registration route: fixed boulder49012 is a type29 PUSH switch
without a literal assignment; Eriic/Ruffus are computed epic teachers; ROOM_INN
initialization can be overridden by the taproom's later crew handler. The
Sultan carries a real imported epic stone and foreign memory. Preserve separate
Q/epic/crew/random-world-quest dispatch and authoritative group claims; TOUCH
text, source prose, a retained flag or visiting an inn does not prove success.

Keep mobility and native gates intact. The boulder clears a blocked exit in
another room; SEARCH/OPEN exposes floor contraband; two fixed rivers and tower
TOUCH stones move actors. The white-robed figure's ordinary type11 runestone
is another object. Pickproof gates need their real keys or a shared open route;
old receipts never recreate spent keys/materials. Unplaced tents with absent
rooms require builder intent before retirement, placement or repair. Do not
activate an orphan or enable TAKE simply to match lore; Fields Between's
intentional hotfix is the explicit example.

Eight cards/22 contacts/46 aliases/11 optional checks are source-guided. Future
source/gift/access/travel/learning/group/campaign milestones need committed event
and recovery qualification. Active, ready accounting is mandatory for all new
tracking; frozen recovery is separate. No native repair ships. Proposed clue,
inn/crew or orphan-template changes stay proposals until implemented in a
separate named fix with regression and prominent PR/news wording.


## Past Ceothia example: alternatives and a native mismatch

The [dossier](../design/zone-stories/PAST_CEOTHIA.md) and
[sidecar](../../areas/story/ceopast.story.json) use four cards for six recipes.
Bind Jamael's black/white/brown payments to one outcome and a one-copy material
check accepting any of those kinds. Keep all three canonical branch receipts;
do not require all colors or award three copies of the same story achievement.

The dryad's initial carried hair and blossom reward are two real sources.
Majelle needs hair/shard/feather together but no personal earlier dryad history.
Put the earlier receipt and onward breaking keys on optional checks. Describe
the actual beast/hawk sources; a chipped-stone description does not implement
mining. Present key80815 and Past reward key81105 are different bluestone keys.

Wolfspeed's dialogue asks for skull81119 while the native recipe accepts black
pelt81108. Map the actual accepted pelt; keep the skull a clearly labeled clue
and add **ZSQ-CEOPAST-WOLFSPEED-PROOF** to builder work. Decide intended balance
and version/recovery policy before altering the recipe. The pelt also feeds
Jamael, so one consumed root cannot satisfy both. Any implemented repair must
be isolated in a named fix with concrete before/after/proof and prominent news.

Read staging geometry and actual wandering, not only rare-load room prose.
The winter wolf and Wolfspeed can disperse independently; roughly6% is not a
qualified availability rate. Reset mode0 and cap1 sources also require renewal
policy before making a daily promise. Preserve native scarcity, reset mode,
pickproof gates, breaking keys, fixed portal TAKE/weight/charges and PvP access.
Do not activate an unplaced template or reverse an intentional hotfix to fit
a story; the Fields inert-proof follow-up is the explicit design precedent.

Schema3 supports current materials and historical receipts. First source versus
gift, successful SEARCH/key/door/arrival and timeline restoration still need
committed event qualification. Active, ready accounting is mandatory for all
new tracking, with frozen recovery separate. This checkpoint ships guidance,
not a native proof, rare-spawn, reset or mobility repair.


## Basin Wastes example: an offered item can choose the outcome

The [dossier](../design/zone-stories/THE_BASIN_WASTES.md) and
[sidecar](../../areas/story/basin_wa.story.json) preserve ten native recipes as
seven cards. Keep four different potion rewards independent; group four equal
part payments as ANY, retaining each branch receipt. Classify a keep-it/same-kind
heartstone refusal as service, without achievement or daily.

Read actual shared dispatch before telling a player to give ingredients.
Here Q blocks load in reverse order and the offered kind filters matching:
giving a part chooses cash; giving elixir chooses crafting. Multiple carried
part kinds make crafting ambiguous, so explain the intended single loose part.
Do not silently reorder shared recipes. **ZSQ-BASIN-DISPATCH-CHOICE** records
builder intent and played accounting-active qualification before a repair.

Trace each actual reset instance: only one minotaur carries elixir, only the
ten tunnel fire beetles hold glands, and Aberden's fixed container supplies the
signet. Optional earlier craft history explains a route, never blocks supplied
matching proof. Source/gift and scarce renewal still need admitted events.

Books are clues with distinct parents/E passages. Carrying or READ→LOOK prose
does not yet record durable learning; **ZSQ-BASIN-LEARNED-BOOKS** plans exact
reader/target/passage/revision evidence. Same-kind reward does not mean the
original UID survives. Explicitly false-city cave prose and a real spider
return barrier are not authorization to add an exit or bypass. Treat copy/clue
mismatches fairly and isolate any real repairs in named fixes/news. Preserve
the owner-confirmed Fields PvP hotfix; all new credit requires active, ready
accounting with frozen recovery separate. No native repair ships here.


## Nakral's Crypt example: do not infer identity or holder from appearance

The [dossier](../design/zone-stories/NAKRALS_CRYPT.md) and
[sidecar](../../areas/story/crypt.story.json) show five exact outcomes. Four
sticks plus two branches is not any six wooden items. Five same-name adamantite
chunks are five different prototypes. Original and enhanced collars have the
same name but different identities. Label sources/reward change clearly while
keeping native contract matching exact.

Trace reset execution, not only nearest M: F loads a follower and replaces the
current mob for subsequent G/E. Statue trophies belong to butterfly/imp/devil/
wolf, not their leaders. Qualify live parent/root/custody and reset generation.
Optional earlier trophy receipt explains a token route without blocking valid
supplied token. Statue bracelet/collar both retire it; avoid a promise that one
encounter suffices for both or that static daily eligibility proves renewal.

The three-fragment magic word and four fixed switches have real shared handlers.
Explain those controls now, but carrying notes or seeing an open door is not
personal learned/action credit. **ZSQ-CRYPT-LEARNED-WORDS / CONTROL-ACCESS** plan
reader/passage/revision, selected target, actual before/after and committed
arrival. Distinguish source, gifts, reading, successful unlock and travel.

A stale active exit to an old absent surface room requires builder intent,
not reactivating an obsolete map. Quantity/direction discrepancies are fair
clue-only repair proposals. Isolate actual repairs with before/after proof and
prominent named fix/news. Preserve fixed orb TAKE/charges and the owner-confirmed
Fields PvP hotfix. New credit requires active, ready accounting; frozen recovery
stays separate. No native zone or quest repair ships in this example.


## Valoisian Castle example: decode state and avoid implied prerequisites

The [dossier](../design/zone-stories/THE_VALOISIAN_CASTLE.md) and
[sidecar](../../areas/story/val.story.json) map eight native exchanges. Family
seals differ from prince/princess seals; beautiful roses differ from the white
rose. F replaces the current holder, so guard maces belong to followers rather
than the chamberlain. Exact kinds and live custody matter more than appearance.

Wine→cook→plate→queen is helpful progression, not a mandatory personal history
gate: supplied plate works. An old cook receipt never replaces consumed proof.
The actual keyed cellar and courtyard gates explain source access. Native D
state5 masks to1(closed/unlocked) and adds secret4; it does not mean open.
ROOM_INN registers rent without literal assignment. _spec1_ selects a mob's
class specialization, not a quest gate. Shop prose/inventory alone proves no
BUY transaction. Check shared/flag/table dispatch before declaring functionality.

Two blank rose responses still have exact input/reward outcomes; don't invent
a relationship, reading or sexual scene. King/assassin lore does not prove a
personal kill predicate or political ending. Fifteen unfinished Veralis rooms
are isolated; choose reserve/retirement/completion explicitly before connecting
or populating them. Richer source/learning/access/politics needs authoritative
events, not guessed milestones. Actual repairs need separate named fix/news.
Preserve the owner-confirmed Fields PvP hotfix and required replacement plan.
All new credit requires active, ready accounting; frozen recovery stays separate.


## Harrow example: one proof cannot fund several accepted outcomes

The [dossier](../design/zone-stories/HARROW_THE_GNOME_VILLAGE.md) and
[sidecar](../../areas/story/harrow.story.json) map a ring→token source followed
by four alternative crafts. Each craft consumes a fresh token. Supplied token
works without personal ring history; old history cannot restore a spent token.
Current readiness on several cards is not a material reservation or completion.

Validate exact source holders and IDs: child scrap differs from shop bolt,
bard horn from shop horn, artists' near-finished painting from gallery decor,
and hidden artist doll from attic toy prose. Lomya stocks the completed lucky
sack as finite merchandise too; its purchase or possession is not crafting credit.
Hidden floor silk is not a promised spider drop/carve. Teacher flags provide
shared teaching, not an implied art-learning gate. MA/QA control room echo;
seventeen ambient messages do not create automatic quests or alias achievements.

For portals inspect selected target and actual arrival, retaining original pickup,
charges and shared restrictions. JUMP fishbowl uses an object target. Lucky-star
rooms lack identified ordinary incoming access and three spokes have no ordinary
exits; builder confirms intentional/shared access or missing design before
connecting anything. Non-door paint-set key field is not a locked magic gate.
Native clue fixes must be separate named fixes/news; no repair ships here.
Preserve Fields owner-confirmed PvP hotfix and required replacement follow-up.
All new credit needs active, ready accounting; frozen recovery remains separate.


## Mountain Tracts example: current commands, closed doors and consumed proof

The [dossier](../design/zone-stories/MOUNTAIN_TRACTS_OF_THE_UNTAMED.md) and
[sidecar](../../areas/story/mountaintracks.story.json) map four native outcomes.
Check exact source objects:the wearable hidden miniature differs from the large
stationary statue; the vampire tooth comes from one particular vampire. Two
scales do not replace scale+tooth, and lore about killing an apprentice does not
establish a personal-kill prerequisite. Nineteen aliases group into four responses.

Keep current material distinct from history:Bumble consumes the potion. His
gloves plus another fresh potion and moonstone heart belong to Ohnagra's separate
Winterhaven recipe. Supplied potion fits without personal Futni history; an old
receipt cannot restore a spent/drunk/spilled copy. Journal readiness does not
reserve one item for competing uses or fabricate a crafting/learning receipt.

Decode commands against current headers. Vine value65 is valid GRAB, while
CLIMB556 is separate preparation; do not rename or repair a working dispatch
from an assumed command. PUSH removes EX_BLOCKED, not EX_CLOSED, so tomb/boulder
doors still need OPEN. Record actual selected control/actor and committed arrival
before personal access credit; another player's opening or audience text differs.

Unplaced portals/switches and missing212xx references remain builder design
questions. Boot removes unresolved exits; this does not prove a live dangling
exit crash or authorize new routes. Prefer truthful clues after intent review.
Any actual repair needs a separate named fix, focused before/after proof and
prominent news. Preserve pickup/charges/stock/terrain/PvP, especially the Fields
intentional rift hotfix. All new credit requires active, ready accounting;
frozen recovery remains separate.


## Orcish Slave Camp example: a quest prop is not every item with a food name

The [dossier](../design/zone-stories/THE_ORCISH_SLAVE_CAMP.md) and
[sidecar](../../areas/story/shortc.story.json) explain the exact key→steak and
steak→mace/XP exchanges, including the hero's tragic departure. Do not title
the scene a rescue or turn text about a corpse into combat-death evidence.
Supplied steak fits without personal master history; alternate master stock
means possessing it does not prove earning it.

Key value 1=20 means20-percent break chance, not 20 uses. Key hand-in consumes
the trapdoor tool. Current loose key, native held-key eligibility, earlier
receipt, accepted destruction, reveal/unlock/OPEN and actual arrival differ.
Raw door kind2 adds EX_PICKABLE, not EX_PICKPROOF; preserve native picking
after reveal and other existing admission. No mandatory personal key-only gate.

The steak is TRASH13. A separate G T19 request accepts ordinary FOOD, has no
reward and leaves the hero hungry. Active exact-only durable admission refuses
that type branch, so classify it as an explicit exclusion until qualified typed
root consumption/outcome support exists. Do not make the prop edible to bypass
the limitation. The schema cannot truthfully check arbitrary item-type inventory.

For D retirement inspect selected NPC identity and epoch. Current template/room
reselection needs a delayed commit/reset replacement qualification case before
assuming it preserves the original target. Confirm prop/clue intent before
native repairs, and give actual fixes separate commits/tests/news. Preserve
key chance, locks, stock/PvP and Fields escape hotfix. All new credit requires
active, ready accounting; frozen recovery remains separate.


## Lava Caves example: distinguish a return quest from its blocked producer

The [dossier](../design/zone-stories/THE_UNDERGROUND_LAVA_CAVES.md) and
[sidecar](../../areas/story/lavcav.story.json) classify the exact horns→wrist
chain/XP exchange separately from the coin-only purchase that produces horns.
Active accounting refuses that purchase; no floor/reset/ordinary loot source
places the pair. Supplied matching horns fit the lieutenant without personal
purchase history. An excluded historical purchase can be displayed without
adding an achievement or restoring spent horns. Wearing the horns or owning
the reward differs from a current loose material and accepted return receipt.

Do not infer personal rescue, thief kill, mining or fainting from dialogue.
Six keywords select four responses, not six completed steps. The seller starts
in an isolated room and can wander onto the lake or into an exitless trap.
Preserve that design; expose only actual availability and qualify selected NPC
instance/epoch before retirement. Coin support must debit the actual payer,
issue the exact root and settle atomically with retry/rollback/recovery; legacy
NPC pooled money is not accepted payer evidence. The configured100-platinum
fee and quoted1500-platinum price need intent review before any correction.

Native key eligibility uses VNUM, so the rare WORN onyx key is not automatically
broken by its type. Training gates reset open; value1=500 means a break-roll
threshold, not500 uses. Prison key/picking, reveal/unlock/OPEN/arrival and another
player's opening need separate facts. Fixed PUSH wall clears blocking, not the
closed door, and stays stationary. Fireplane heat can strip protection spells;
prose does not grant safety. Teaching/crafting/rescue lore needs explicit
endpoints before credit. Actual stock/empty-mode1 renewal and source-qualified
daily availability require qualification with active, ready accounting. Keep
builder intent questions distinct from proven repairs; actual fixes get separate
commits/tests/news, preserving Fields hotfix and PvP.


## Nomad Encampment example: a receipt does not supply consumed collateral

The [dossier](../design/zone-stories/THE_CIMMERIAN_NOMAD_ENCAMPMENT.md) and
[sidecar](../../areas/story/nomads.story.json) show two independent accepted
outcomes linked by current material: wood+iron shards give the ring, then the
ring and two distinct heads give the crown. Require the complete exact bundle
loose together for native accounting admission. Both heads use keyword head;
two of one are not both. A worn ring must be removed. Supplied matching ring
fits without personal first-stage history, while earlier history never restores
a ring spent in the final exchange.

MA/QA A means room narration, not an ALL-goals opcode or listener awards. Clues,
Rellius page possession and the crown itself are not accepted completion proof.
The crown also appears in CHAOS starter kits. Heads are G inventory on living
NPCs; do not infer CARVE or personal killer requirements from item names. Keep
SECRET/TRANSIENT/NORENT rules, and qualify actual custody/decay/recovery before
promising source availability. Durable snapshot custody can preserve a no-rent
item, so the flag alone cannot predict logout loss.

For optional investigation facts, native WAKE admission differs from seeing an
awake blacksmith. LOOK/READ page1/page2 differs from owning the journal or another
book with the same page alias. Bind future facts to selected NPC generation or
journal UID/page/content revision and actual accepted output/state. Keep these
as guidance until the capability exists; no invented learned objectives.
The journal is SPELLBOOK33 with108-page capacity, not spell108. Rellius body is
a fixed hidden container, not a normal corpse or pickup/resurrection endpoint.
Teaching/crafting/healing/prophecy needs explicit contracts before credit.

Qualify occupied-mode2 renewal and original Septimus retirement versus a later
replacement before claiming reliable dailies. Keep builder intent questions
separate from verified failures; actual repairs get named fix commits/proof/news.
Preserve sources, flags, doors, travel/PvP and Fields escape hotfix. Active,
ready accounting is mandatory for every new credit; frozen recovery separate.


## Undermountain example: an access key can also be consumed quest proof

The [dossier](../design/zone-stories/THE_RUINS_OF_UNDERMOUNTAIN.md) and
[sidecar](../../areas/story/undermountain.story.json) link two independent
outcomes with current material. Explain key92133's competing uses: keyed UNLOCK
invokes100% break roll, while Tamsil consumes an intact key for note92134.
Opening, destruction settlement, earlier rescue and current note are separate.
A supplied exact note fits Durnan without own earlier rescue; its receipt never
restores consumed material. Do not make own unlocking/kill/reading mandatory
when the native hand-in has no such predicate.

Check actual dispatch: all local NPC special bindings sit inside #if0, although
literal inventories find six named leads. Eight equipment bindings are active.
Do not promise the dormant inn, nine-weapon collection, hired escort or death
transformations; activating them needs a separate design and balance review.
Key is hidden/invisible; SEARCH does not remove invisibility or search loose
inventory. Reward scimitar is hidden. Note is blank/writable and matched by
prototype, not message content. These are explicit builder qualification gaps.

Preserve unplaced lever, rare holding rooms, fixed portals, breakage and PvP.
Universal access/source/learned facts need selected UID/door/actor/state/output,
accounting admission and recovery proof; clues and pre-open routes stay guidance.
Actual repairs need separate named fix commits and prominent before/after news.
All new credit requires active, ready accounting; frozen recovery separate.


## Desolate Under Fire: eight roots do not imply eight own rescues

The [sidecar](../../areas/story/desolateinv.story.json) and [dossier](../design/zone-stories/DESOLATE_UNDER_FIRE.md) explain eight independent captive outcomes and Beregan’s eight-current-bindings requirement. Optional own rescue history never supplies spent items or forces personal history onto an exact hand-in. Gifted matching bindings fit; seven roots do not. Monkey is a container with nested chain: remove chain before consuming monkey, and return monkey before hunter rescue retires that instance. Do not invent escort/CARVE/first-source/own-kill prerequisites from narrative.

Classify guarded coin-only purchases as services and cover every contract. Missing givers/producers and drink/blade mismatch stay explicit builder work. A live invasion needs an accepted phase episode/route generation, not normal-zone discovery or triumphant hand-in text. Verify source choice when legacy worlds disagree. Clearing BLOCKED differs from SECRET/CLOSED/traversal/trial victory. Existing schema3 guidance is enough; new facts need admitted actor/UID/instance/state/output and accounting/recovery proof. Two direction words ship in a separate fix with original-fails/repaired-passes evidence; other mechanics remain builder decisions. Active, ready accounting is mandatory.


## Storm Port Stronghold: a ticket does not prove travel

The [sidecar](../../areas/story/spshold.story.json) and [dossier](../design/zone-stories/STORM_PORT_STRONGHOLD.md) show current coal/valve, captain exchange, current ticket and Decker exchange separately. Matching supplied ticket fits without own repair or travelled ride. Native narration grants no launch controller. Hordine’s sea maps and torn map are different contracts; optional foreign Burgadan referral returns a map/spade, but cannot supply missing current objects. Helmsman, not Mui Pai, is the competing sea-map consumer. Roaming holders are current-source qualification, not guaranteed routes to isolated load hubs.

Check the exact access implementation:spade is a key for secret locked underwater silt; DIG rejects underwater terrain and needs different tool aliases. Both spade and chest key can break after ordinary keyed UNLOCK. Receipt history, door state, settled destruction, replacements and chest loot are separate. Keep native restrictions and source scarcity. An equipment set or helm/avatar loot does not prove a hand-in.

Audit paid services independently:crew_shop_proc ignores refused SUB_MONEY and changes/saves the ship; this requires a separate accounting guard/typed transaction fix before claiming paid hiring is safe. Map the real source defect and acceptance matrix; do not label it already repaired or invent a hire achievement. Existing schema3 guidance is enough; every new credit still requires active, ready accounting.


## Harpy hometown: verify special dispatch before classifying a hand-in

The [sidecar](../../areas/story/harpyht.story.json) and [dossier](../design/zone-stories/THE_MOUNTAIN_SETTLEMENT_OF_THE_HARPIES.md) replace the generic key/shackles/feather requests. The key rescue and queen shackles response are native receipts; supplied proofs fit without own recovery/rescue. The khan feather Q is shadowed by a custom actor mutation and is excluded. Specials run before quest handlers, so verify actual recipient parsing, actor admission and accepted outputs, not only the qst file. Neutral racewar is a dispatch condition; current schema3 does not prove actor-state conversion.

Both custom allegiance procedures accept a feather, ignore the addressed recipient and lack a Harpy/PC guard. Treat atomic actor/item settlement and one-time branch policy as required work. Gargoyle ASK undead counts floor NPC corpses but transformation is commented out; advertised unlife is not a working alias. Keep dormant race/class/exit mechanics until intentional design and separate fixes are qualified. PULL ladder and ENTER cage are real routes; descriptive locked cages are props. No personal source/kill/learned/arrival credit is inferred. Active, ready accounting remains mandatory.


## Behemoth Herders: audit current item receivers and independent outcomes

The [journal](../../areas/story/herders.story.json) and [dossier](../design/zone-stories/THE_BEHEMOTH_HERDERS.md) show6 story requests/6 services, exact counts and19 optional current-material rows. Use named places and commands in player copy; keep prototype IDs and execution internals in builder evidence. Left/right mind halves, five eyes and sword+soul are exact full bundles; supplied proofs fit without personal kills. Reusing a spent diorite scale for another craft is impossible. Two XP rescues remain potential dailies; reward type is not itself an exclusion. Four fee recipes keep the accounting guard.

Read the successful reset receiver: F/R updates current mobile for subsequent G/E even though the follower’s master stays the preceding M. Tiny scales belong to the dragonkin familiar; Zugle’s skull precedes F and stays on Zugle. P selects a globally existing matching container, so static O proximity does not prove actual parent UID. Shared PULL/PUSH/ENTER/UNLOCK/SEARCH changes, NORENT material, native D retirement and healing/transformation prose require separate admitted facts before deeper objectives. Archazel’s epic trainer is assigned through epic initialization, beyond literal assignment scans. Source mismatches, unplaced prototypes and the asymmetric stronghold door are pending builder decisions; no native repair is claimed. Active, ready accounting remains mandatory.


## Jotunheim: distinguish password hints, exact proof and accepted outcomes

The [journal](../../areas/story/jotun.story.json) and [dossier](../design/zone-stories/JOTUNHEIM.md) show11 independent stories/four services,19 optional current-material checks and ten contacts/47 aliases. Use named places/working commands in player copy and keep prototype IDs/execution internals in evidence. Mimir needs five distinct proofs; nightshade does not replace jade, and duplicate sapphire cannot fill quartz. The Balor sword is consumed by Mimir or Quelranor. Rival totem/standard receipts stay distinct despite identical reward. Supplied matching proofs fit without own kills or earlier access receipts. Ordinary/ancient scales are G stock, not CARVE output; tankard acceptance checks identity, not current ale.

SAY kostchtchie or silverwing unlocks a shared reciprocal key−2 route but leaves CLOSED; OPEN then move. Already-open routes do not require personal keyword history. Rare staging, mode1/caps, retiring givers and legitimate stock govern availability. Table-driven Mimir epic assignment overrides the old literal movement gate; blacksmith is also assigned by tradeskill startup. Native response/lore/combat text never supplies separate rescue/healing/keyword/training credit. Preserve mixed-fee/forge/reset accounting guards and classify rejections/crafts as services.

| ID | Source evidence and fair implementation plan |
| --- | --- |
| **ZSQ-JOTUN-SPOKEN-ACCESS** | Gate96197S↔96198N uses key−2/last keyword kostchtchie; wall96215E↔96252W uses key−2/last keyword silverwing. Both reset closed/locked, wall also secret. Successful SAY clears lock/secret on reciprocal sides, leaves CLOSED: OPEN then move. Badge/allegiance prose creates no item/race/faction gate. Define successful actor/room/exit/instance/version/state events only if builders require personal operation. Preserve shared-open routes; qualify rejection/silence/reset/return/accounting publication. Hidden boulder/stair/tear/cobweb doors are ordinary access. |
| **ZSQ-JOTUN-RARE-SOURCE-RENEWAL** | Mode1 stages rare actors at96288–96293; trap96294 has no exits, distributor96295 has six exits to96040/96104/96088/96132/96214/96153. NPC wander uses NUM_EXITS10, no-move draw, CAN_GO/sector/master/sentinel guards and last_direction suppression. Percentages5.2/10.4/20.8/41.6/62.5/83.3 are historical labels, not qualified live probabilities. Preserve caps/positions/chances; audit actual scheduler/lifecycle distribution and recipient renewal before daily activation. Active accounting refuses item reset issuance. Admit legitimate stock generation/UID/custody; no guaranteed-spawn convenience change. |
| **ZSQ-JOTUN-COMPETING-BRANCHES** | Sword96038 is consumed by Mimir’s five-item bundle or Quelranor. Standard96060 and totem96061 differ although rivals share reward96062. Brunnhilde’s requests are independent; cloak retires her. Sarimar/trolls/Olaf/Quelranor also retire. Supplied proofs fit without own kills. Define deliberate branch/recipient/campaign policy before exclusive/aligned/full-clear objectives. Preserve independent historical receipts; shared reward/spent root cannot complete another request. |
| **ZSQ-JOTUN-MIXED-FEE-SERVICES** | Ordinary96069+C250000→96070 and ancient96071+C250000→96072 are services; durable offering refuses mixed inputs under accounting. Smith96058 is also assigned by initialize_tradeskills after boot_db, menu103–112/119; full smith selection/ore/payment/output/rollback flow has its own accounting refusal. Preserve both guards/materials/fees. Future admitted root+wallet settlement must be atomic, count-correct, owned and recoverable. Fee readiness/unrelated FORGE results earn no story credit. |
| **ZSQ-JOTUN-MIMIR-OVERRIDE-LORE** | assign_mobiles binds jotun_mimer; later normal startup epic_initialization/epic_points assigns epic_teacher96013. Old WEST level51/IS_GIANT/return-to-birth procedure is not normal active execution. PRACTICE has full summon-blizzard name/class/level/max/payment/accounting guards and falls through for ASK/GIVE/movement. Greeting/Well wisdom is lore; holy water17 heals good/harms evil, no wisdom grant. Choose combined trainer/access intent before restoring/changing a gate; test ordering/periodic registration/fallthrough/return/race-level policy.34 ASK families/47 aliases are hints; two qc_action50 timers are ambient, never player topics. |
| **ZSQ-JOTUN-NARRATIVE-RESCUES** | Silverwing96036 is Telshanar, wears cloak96042 at96286; Brunnhilde accepts proof and mourns, without freeing/healing/escort/replacement. Olaf’s mask receipt does not liberate slaves/change faction. Mimir’s bundle does not verify every mage dead/permanent extinction. Prisoners/frozen figures/illusion king96043/kitchen slaves are lore/stock. New episodes need chosen builder semantics, real actor identity/state, admitted transitions and restart/replay/party policy. |
| **ZSQ-JOTUN-PROOF-IDENTITY-CUSTODY** | Distinct jade96036/sword96038/whip96039/quartz96037/sapphire96081 are required; sapphire is ground item, not secret/container stock. Scales are G stock, not CARVE; generic CARVE creates8. Tankard96023 hand-in checks identity only, not ale/type/origin. Rejections96035/96046 create same-prototype outputs, no original-instance restoration guarantee. Fireweed96076 is NORENT. Preserve loose exact ownership/supplied routes; admit origin/transfer/liquid/content evidence before personal collection/brewing guarantees. Qualify cold save/reconnect/reset issuance. |
| **ZSQ-JOTUN-COMBAT-PROC-REVIEW** | Four mobile/six object procedures and callers reviewed. Balor death uses caster infravision for target blindness, sends some victim warnings to caster, subtracts nonfatal HP without update_pos and can call nested die; ACT_SPEC_DIE bypasses make_corpse, ordinary extraction drops surviving gear. Loki returns inside first eligible non-giant loop, overlaps fear branches and uses null TO_VICT audience. Faith checks retained original opponent trust for room-target effects across kill-capable spells; null-group targeting needs intent review. Deva cloak has similar null-group question. Build isolated original-fails/repaired-passes audience/lifetime/group/death matrices; preserve proc chances/damage150-250/durations/target policy/PvP balance. Actual repair must be a separate named fix/news commit. Proc text creates no quest receipt. |
| **ZSQ-JOTUN-KEY-RETURN-INTENT** | Thrym gate96189N uses Loki’s glowing ice key96007, reverse96190S key0/reset closed-unlocked; relocking can expose mismatch. Storeroom key96006, cells96014; all key break fields0. Determine return/trap intent before symmetric-key changes; qualify reset/return/pick/lock/scarcity. Cell96276 prose says east, actual return south. Guide correctly now; any caption fix needs separate narrow fix/news commit and exact-byte proof. |
| **ZSQ-JOTUN-UNFINISHED-ORPHANS** | Muspelheim is under construction;96073 only DOWN. Destroyed ferry96111 only SOUTH, older sign still advertises service. Registry95925–96295 contains actual96000–96295/staging;96004 DOWN−1 is NOWHERE. Mobile96066/boots96026/chest96080 have no active reset/quest/literal special producer in closure; chest key96007 cannot make it available. Imported fountain72 is TRASH scenery; node359 separate epic stone. Establish scenic/unused/unfinished intent before adding travel/stock/actors/rewards; actual fixes separate named fix/news commits. |
| **ZSQ-JOTUN-FOREIGN-OWNERSHIP** | Astral19701/O19733 ENTER→96004; local96005/O96004 ENTER→19733. Mundorno83342/M83655 native QA5707:83377+83191+C100000→leggings96021. Fearfrost131637/M131772/chance50 Q240:131647+96000+96012+96055→131648. Local Frostbite E chance40 retained. Shabo Jabulanth32829/M32866 carries outputs96077/96078; owning rewards cannot prove local hand-ins. Preserve foreign contracts/discovery and admitted source history; no guaranteed supply/ownership reassignment. |

These are explicit builder follow-ups, with no native repair in this checkpoint. Active, ready accounting is mandatory for new progress. Any later actual repair needs a separate named fix/news commit; documented intent and isolated original/repaired evidence precede changes to access, lifecycle, combat targets or balance.


## Temple of Flames: explain a material path without inventing prerequisites

The [journal](../../areas/story/temple.story.json) and [dossier](../design/zone-stories/TEMPLE_OF_FLAMES.md) show six independent outcomes, nine contacts/42 aliases and eight optional material rows. Use actual item names:two yellow daggers/green token/blue wooden sword, golden locket, wedding band, soul of Illyn and well-crafted dagger. King and Ommsh empty-reward responses remain stories;500-platinum and duplicate-vial rewards do not change receipt identity. Supplied exact proof fits without personal kills, passwords or prior master/king completion.

Locket identity alone is accepted; actual container children may be consumed, so never promise photo preservation or make it mandatory. Chest starts OPEN+LOCKED; do not silently change the bitvector. Key breaks on successful UNLOCK. Mirrors have real STARE/backside routes. A disconnected walk graph does not prove hammer unavailable:eligible native same-zone teleport can enter melting rooms. Intended “Drink drop” route and shared-control return behavior remain builder review.

| ID | Source evidence and fair implementation plan |
| --- | --- |
| **ZSQ-TEMPLE-SPOKEN-ACCESS** | Sepulchre18362S→18363 and throne18561E→18564 use key−2/last keywords sepulchre/illesarus. SAY clears reciprocal LOCKED/SECRET but leaves CLOSED; OPEN then move. Outer18426E→18427 uses bdkn while reverse says bkdn/key0. Shared open routes require no personal earlier receipt. Define successful actor/exit/instance/state/version events only if builders require personal operation; qualify rejection/reset/recovery/party accounting before adding credit. |
| **ZSQ-TEMPLE-MASTER-CHEST-INTENT** | Master18336 Q294 exchanges locket18307 for steel key18316, retires and points toward the shack. Ring18336 is P stock in chest18315/O18375. Chest values500/9/18316/500 mean OPEN+LOCKED:CONT_CLOSED4 is absent; GET tests CLOSED. Key has100-percent break on successful UNLOCK. Establish intended initial/return/relock/scarcity policy before a closed flag or caption repair. Preserve supplied ring access and separate master/angel receipts. Any actual repair requires a separate named fix/news commit. |
| **ZSQ-TEMPLE-LOCKET-SUBTREE** | Locket18307 has flags5(CLOSEABLE1+CLOSED4), unlocked, SECRET/NORENT; E3/cap1 on first maiden18307/M18337. Photo18312 P/cap1 uses a globally matching locket. Native input checks only locket identity. Batch offering recursively captures actual children and snapshot topology and consumes selected root; photo can be consumed without an independent quest requirement. Qualify legitimate root/child UID issuance, global P target, child removal, extra contents, NORENT save/reconnect and restart/replay refusal. Never silently require photo or promise it is preserved. |
| **ZSQ-TEMPLE-HAMMER-ROUTE** | Hallucination18339/M18578 carries stonecrusher18339/cap1. Melting18576–78 has only incoming18583DOWN from a disconnected staff-labelled hub; hidden pool18578E→18569 is outgoing. Both ordinary TELEPORT and GROUP TELEPORT select eligible same-zone rooms, independent of walk graph; melting room flags33554432/sector0 pass destination filters. Thus no walking route is not proof of unavailability. Hallucination wander initially rejects SECRET. “Drink drop” has no custom local producer; basin18326 is finite unholywater28 and angel potion18337 is vitality56, not teleport. Builder must specify intended puzzle/travel/source-renewal policy, then qualify admitted successful travel/custody without weakening PvP/raid/room guards. |
| **ZSQ-TEMPLE-SHARED-CONTROLS** | PUSH table18314 operates basement18371D→18373. PULL latch18319 operates18405U→18406; SECRET target means item_switch clears forward BLOCKED only, not blocked reciprocal DOWN. TOUCH statue18331 at18503 clears BLOCKED on18506E→18507 but leaves SECRET/CLOSED handling. Mirror18327 STARE→18432 and backside18329 STARE→18431 are real unlimited-value portals. Preserve shared/already-open paths, alternate drain return, exact command selection and hazards; define personal successful-control evidence deliberately. |
| **ZSQ-TEMPLE-CLUE-INTENT** | TRUTH bridge alphabet xokqgbzswumifdharyjlevptnb repeats b/misses c; Illyn periodic “Fire is the key” is ambient, not a SAY trigger. Forward bdkn/reverse bkdn needs an intended puzzle walkthrough. Well18305 is TRASH; basin door permits SEARCH/OPEN/DOWN while room PUSH prose has no bound switch. Determine intended clues before caption/alphabet/password repairs. Add original-fails/repaired-passes command/return matrices and exact-byte scope; no inferred cipher correction, new quest item or disabled procedure activation. |
| **ZSQ-TEMPLE-NARRATIVE-EPISODES** | Four D outcomes retire Illyn/king/angel/master. King response narrates human form; Illyn peace/master reunion/angel aid do not instantiate replacement actors, escort, permanent cure, released cohort or whole-zone liberation. Samael/attendant/chef lore and33 ASK families are hints. Builders must choose actual episode actors/transitions/end states, group/reset/recovery policy and admitted accounting events before achievements for rescuing or curing. |
| **ZSQ-TEMPLE-BUNDLES-REWARD-RECOVERY** | Sage233 requires two distinct yellow-dagger18322 roots plus token18324+sword18325; givesC500000 and stays. Matching loose supplied proofs fit without own kills. Angel273 returns two separate18337 rewards for one accepted ring receipt; duplicate reward ordinals have separate source identities. King87/Ommsh325 have empty reward lists, still real story outcomes. Qualify four-root atomic refusal/consumption, wallet and duplicate-item continuation recovery; reward count/possession must never multiply or identify completions. |
| **ZSQ-TEMPLE-SOURCE-DAILY-RENEWAL** | Mode1/cap1 sources and four retiring recipients govern current availability. Source matching does not guarantee reset issuance, available giver or daily replenishment. Active accounting item-reset refusal remains. Soul18300/key18316/locket18307/ring18336/child photo18312 NORENT and custody require played qualification. Keep finite holy liquids/current stock, hazards and native chances. Admit intended lifecycle/generation identity before daily source promises. |
| **ZSQ-TEMPLE-FOREIGN-OWNERSHIP** | Knife18309 also goes to Hall child77742/Q223 at77911 for letter77743, feeding that zone’s own subsequent chain. Consumed copy cannot complete both; foreign receipts/discovery remain foreign. Imported358 is a separately guarded epic node. Unplaced statue18342 and reward-only18301/18302/18316/18337 are not extra local quests/sources. Ambient temple_illyn and commented sword binding supply no accepted outcomes. Preserve registry/boundary and service/unused intent. |

Active, ready accounting is mandatory for new progress. These follow-ups are plans; no native repair ships in this checkpoint. Any actual clue/access/lifecycle correction requires separate named fix/news commit and exact original/repaired evidence.


## Pharr Valley Swamp: explain paid materials without fabricating payment

The [journal](../../areas/story/pods.story.json) and [complete dossier](../design/zone-stories/PHARR_VALLEY_SWAMP.md) show one shard story and three guarded paid services. Use optional carried-item rows for six actual feathers/one hide and three actual hides; current preparation does not prove payment, craft, own source recovery or any earlier receipt. Native fees differ from spoken craft prices; show both and record builder intent before a separate fee/caption repair.

The mummy contains a tome, not the shard. Its key can be supplied and its lock picked; do not require a personal purchase. Follow F to identify the real equipment receiver. Missing stock29555/38555 sits beside valid hides; guessing replacements could change scarcity. SHAKE/PULL/PUSH are exact switch commands, while shared already-open routes need no personal action. Keera/fresco/stranded traveler prose needs explicit episodes before rescue/restoration credit. Foreign hermit receipts remain foreign.

The dossier records ten ZSQ-PODS follow-ups for payments, price intent, missing reset prototypes, shared controls, material custody, key/container lifecycle, source renewal, narrative episodes, scenery/altitude and foreign ownership. All new progress requires active, ready accounting; native guards remain. Any actual repair needs a separate named fix/news commit with original/repaired evidence.


## The Citadel: separate delivery, puzzle and narrative evidence

The [journal](../../areas/story/citadel.story.json) and [dossier](../design/zone-stories/THE_CITADEL.md) show optional loose key/notes preparation and independent accepted outcomes. Notes custody does not prove a chest opening; a key reward does not prove rescue. Dialogue aliases and generic magic passwords are guidance until qualified personal events exist.

| Follow-up | Evidence and intended next step |
| --- | --- |
| ZSQ-CITADEL-REQUEST-INTENT | Lost apprentice asks for notes plus twelve vault items, while Q260 accepts only I13033. Decide whether to correct dialogue or deliberately create a thirteen-item recipe. Thirteen fits the current fourteen-item durable limit; name all exact prototypes and qualify custody, source availability and consumed keys before a separate fix/news commit. |
| ZSQ-CITADEL-EMPTY-CONTRACT | Q267 has I0, empty response and no reward; no object-zero declaration exists. Excluded from new completion credit without deleting native identity or historical evidence. Builder should confirm intended removal or replacement; an actual native repair needs its own commit and news wording. |
| ZSQ-CITADEL-MASTER-CLUE | Room13185 translates the magic word as master, but both door keywords end in shalafi. Decide intended clue/word, then make a narrow separate caption or keyword repair with before/after speech qualification. Journal explains current behavior. |
| ZSQ-CITADEL-SPOKEN-ACCESS | Generic check_magic_doors matches the last keyword, clears locked/secret bits and matching reciprocal state, then leaves CLOSED intact. It records no personal quest event. Add qualified actor/door generation and successful transition evidence for optional password episodes; already-open travel is not personal puzzle proof. |
| ZSQ-CITADEL-KEY-LIFECYCLE | Coat→bluish key→notes chest competes with spending that key for coins; notes→small key→pickproof desk chest is a separate access chain. Qualify current loose/held/worn/nested custody, supplied/picked/shared-open alternatives, breaks, durable destruction/recovery and cap renewal before promising live accounting journeys. |
| ZSQ-CITADEL-TREASURE-SOURCES | Twelve guardians’ keys have twelve destinations; four vault contents key further store rooms. Current item possession, player supply, container extraction and first native-source acquisition are different evidence. Register exact source/container generations only if builder chooses deeper personal episodes. |
| ZSQ-CITADEL-NARRATIVE-EPISODES | Amberyl’s sorrow, the lost apprentice’s plea and glyph research have no committed release/rescue/study outcome. Define deliberate noncombat/escort/performance endpoints and explain their distinction from delivery receipts before adding story credit. |
| ZSQ-CITADEL-SOURCE-RENEWAL | Mode1 resets, capped coat/key/chest/notes, retiring wandering givers and two chance35 chest children govern real availability. Qualify accounting-backed reset issuance and item renewal; a potential daily is not a guarantee of a fresh key, notes or loot. |
| ZSQ-CITADEL-FOREIGN-SYSTEMS | Diamond13036 has Caer Tannad floor and Alatorin chance15 stock. Beholder imports358/67239/55187; epic stone, worn church-door RUB glass spells and memory trophy have separate rules. Keep foreign discovery, ordinary gear effects and epic accounting distinct from local deliveries. |

All new credit requires active, ready accounting. Preserve native contracts, caps, hazard/access rules and separate foreign/epic ownership. Actual repairs require separate named fix/news commits; none ships in this checkpoint.


## The Elemental Groves: explain competing access keys and separate outcomes

The [journal](../../areas/story/element.story.json) and [dossier](../design/zone-stories/THE_ELEMENTAL_GROVES.md) show three optional loose-material checks and exact accepted recipes. Spending a key removes current access stock; possession or a reward receipt cannot prove unlocking, original recovery, a puzzle solution or grove restoration.

| Follow-up | Evidence and intended next step |
| --- | --- |
| ZSQ-ELEMENT-SOURCE-PROVENANCE | Floor keys3808/3809 and wraith equipment3831 are different source paths; supplied proof is valid for current hand-ins. Qualify original reset/container/mobile generation, exact UID and first successful native-source custody before adding optional own-recovery episodes. A gift, worn item, transferred trophy or receipt must not manufacture source history. |
| ZSQ-ELEMENT-COMPETING-KEYS | Cloudy3808 and black3809 are both offering inputs and access keys. They have native break setting0; consuming a hand-in can still remove access stock. Qualify supplied keys/shared-open doors and durable consumption/recovery independently. Do not make key offerings prerequisites for the cube or plaque puzzle. |
| ZSQ-ELEMENT-PEDESTAL-PLACEMENT | Five O3818 pedestals alternate with five distinct P children. Shared reset P uses get_obj_num, a global matching-container lookup, rather than the immediately preceding O instance. Confirm intended room ownership and retained stock behavior, then make any generation-aware placement correction in a separate fix/news commit. Do not promise a plaque in each intended sanctum until played qualification. |
| ZSQ-ELEMENT-PUZZLE-EVENTS | Six actual plaque-key gates precede key−2/SAY nothing. The forrestal’s earth/water/air/fire verse is lore; the actual gate order is stone/silver/gold/crystal/cloudy/black. Qualified actor/door generation and successful read/speech/unlock/open transitions are needed for personal episodes. Already-open access and clue possession do not prove solving. |
| ZSQ-ELEMENT-NARRATIVE-ENDPOINTS | Cube acceptance narrates possible healing and a visit to the dryad but retires the giver without changing the grove. Sprites discuss going home; kirin refuses access in dialogue; the valley lady has no Q contract. Builder must choose deliberate restoration/escort/permission endpoints and fair recipient policy before recording those outcomes. |
| ZSQ-ELEMENT-HAZARDS | Air route uses sector8 NO_GROUND, water includes sector7/10 and UNDERWATER flags, fire uses sector11, and baobab rooms have F2/4/10/6/8. Qualify movement/mount/fall/breath/equipped effects and accepted destination evidence. Fairy dust grants no flight; blue shell needs its real worn effect; rewarded earring is protection from fire, not universal immunity. |
| ZSQ-ELEMENT-RENEWAL-AND-XP | Mode1, cap1 proof/giver, wandering and D1 retirement constrain availability. Accounting-active item resets are currently refused without durable generation identity. Qualify guarded issuance, renewal, accepted destruction, frozen solo/group capped XP and reward persistence before promising live daily supply. Listed20k/40k/200k are base terms, not guaranteed payout. |
| ZSQ-ELEMENT-IMPORTED-SYSTEMS | Silverleaf78455 O resets have chance60 and can be disabled by the legacy nonforced O branch; these are ordinary foreign stock, not current offering goals. Confirm intended renewal without changing scarcity. Epic358, memory55450, DRINK spring750 and dagger3833 effects are separate systems; spring prototype level−1 and the dagger’s null-object guard need bounded dispatcher tests before any proposed correction, with separate repair/news evidence. |
| ZSQ-ELEMENT-ROUTE-INTENT | Earth room3892’s east exit loops to itself; the valley archway3974N actually returns to local glade path3812. Confirm whether each route and its wording is intentional, retain native destinations now, and qualify any builder-selected topology/caption repair separately. Ordinary exits described as portals use native movement; confirmed personal travel evidence would need a qualified adapter. |

All new credit requires active, ready accounting. Preserve native contracts, caps, hazard/access rules and separate special/foreign ownership. Actual repairs require separate named fix/news commits; none ships in this checkpoint.


## Temple of the Earth: source evidence and shared puzzles

The [journal](../../areas/story/earth.story.json) and [dossier](../design/zone-stories/TEMPLE_OF_THE_EARTH.md) distinguish two exact hand-ins from larger missions. Open-container band recovery is a real source; the badge's hydra-lair narrative is not a working source declaration.

| Follow-up | Evidence and intended next step |
| --- | --- |
| ZSQ-EARTH-BADGE-SUPPLY | Q27 consumes43525, whose full prototype exists; global active reset/recipe and maintained-code scans find no world producer. Hydra-lair dialogue and corpse scenery are not a load path. Builder must choose the intended brother/remains location and a scarce legitimate source; qualify generation/UID, admission, reset and supplied proof in a separate fix/news commit before promising availability. Do not add free or unlimited stock. |
| ZSQ-EARTH-CHAMBER-SELECTION | Hidden43697 has five independent O selectors43585–89 with20/40/60/80/100 chances and cap1, while visible43590–94 use materialbutton names. Ordinary nonforced O refuses and can disable chance<100 rows; a forced initial reset can produce several selectors. The custom scan uses the last recognized object in room-list order. Specify intended one-choice-per-generation distribution and renewal, then qualify all choices, retained stock and empty/multiple selector cases before a separate repair. |
| ZSQ-EARTH-CHAMBER-ACTOR | Custom43584 listens to PUSH270 although its generic values say PULL340; room dispatch calls its custom procedure without matching the selected scenery object. Correct choice clears the actor room's north BLOCKED bit, wrong recognized choice creates one of four mobs without a puzzle-specific cap. Confirm exact room/receiver/argument semantics, idempotent shared-open behavior and bounded trap policy; add admitted actor/exit/generation success evidence, not keyword, repeat-spawn or walk-through credit. |
| ZSQ-EARTH-NATIVE-DATA-INTENT | earth.zon248 refers to undefined object43595; mobile43595 exists but is a different namespace. Renumbering disables the missing object reset. Exit43571E has destination−1. Establish the intended missing prop and route, or their intentional removal, before a narrow separate repair/news commit. Preserve maze43589N/S self-loops unless builder intent demonstrates an error; do not guess replacements. |
| ZSQ-EARTH-CUSTODY-AND-KEYS | P43539 belongs to open fixed body43538 at43628; held/worn/nested proof is not current loose preparation. P uses global matching parent, but the successful fresh O places the newest parent first. Charinth G43542/type13 matches a keyed door by exact vnum: has_key accepts loose/HOLD without requiring ITEM_KEY. Sun43534, dial43509, scale43565 and real keys have distinct controls/custody. Qualify exact source/parent UID and successful removal/unlock independently, with supplied/picked/shared-open alternatives. |
| ZSQ-EARTH-RUNES-AND-DEATH | Six type25 routes use actual command IDs TOUCH320 or ENTER7; value2=−1 is unlimited uses, not a permission flag. Bloodrune43515@43611→43652 and43583@43652→43611; death-created43580→43502; two mirrors and two TOUCH runes differ. Existing factory guards can refuse death creation. Qualify actor/source/generation, actual creation and successful arrival separately; no arbitrary rune decipher/closure, boss kill or portal possession credit. |
| ZSQ-EARTH-NARRATIVE-ENDPOINTS | Gromdishar's outpost/escort, captain's survivor/rune missions, Lithibar's rescue, gnome safety and shaman's dead-priest claims have no personal terminal producer. Both native D1 deliveries retire the giver; D text does not call recall or move the player. Builders must specify real endpoint/state/recipient/group/reset policy and optional versus mandatory episodes before enabling these broader stories. |
| ZSQ-EARTH-RENEWAL-ACCOUNTING | Mode0 makes both native outcomes story-only in the current catalog. Any future daily eligibility needs an explicit builder policy; cap1 proof/givers, wandering contacts and D1 retirement constrain repetition. Accounting-active item resets lack durable generation identity and are refused; death read_object and destructive access have separate guards. Qualify admitted stock, settlement, reward and cold recovery under the accounting branch before promising operational daily supply. Map inclusion does not issue items or renew a giver. |
| ZSQ-EARTH-REWARD-WORDING | Captain Q104 says Torm-blessed shield, while reward43562 is named and described as Melkivar; its extra-description alias still says torm. Decide the intended deity and correct only wording or deliberately redesign the reward separately. Preserve accepted identity/history and item power; show the actual reward now. |
| ZSQ-EARTH-FOREIGN-OWNERSHIP | Literal room43341 patrol_shops is physically in aopal, with stock43342 and dispatch43343, not an Earth mission. Range-only inventory assignment is an inspection lead. Future ownership inventory should resolve physical kind/source before claiming local procedures. Preserve the accounting-active hiring refusal. Brass separately spawns magma43540 at five rooms/cap5; Eligoth's imported monolith360/epic and mace23805/zion combat effects remain separately owned. |
| ZSQ-EARTH-HAZARDS-AND-CLUES | Full room flags/exits, underwater areas, traps and keyed/secret/blocked routes are actual evidence; floating-rock and heavy-door prose alone are not flight/strength gates. Lithibar's verse and tapestry are clues, not accepted knowledge. Qualify successful read/control/access/travel and native equipped/breath effects without adding automatic rescue or source credit. |

New progress requires active, ready accounting. Qualify native supply, custody, shared controls and actor endpoints; preserve scarcity/access/PvP policy and separate native repair/news commits. No native repair ships in this checkpoint.


## Githzerai Stronghold: repeated ingredients and floor controls

The [journal](../../areas/story/githzer.story.json) and [dossier](../design/zone-stories/GITHZERAI_STRONGHOLD.md) demonstrate exact quantities, independent alternatives, a consume-only information outcome, a pending currency route and completion-only contacts without invented topics.

| Follow-up | Evidence and intended next step |
| --- | --- |
| ZSQ-GITHZER-CURRENCY-KEY | Q55 C500000 means500 platinum, not an item44509 payment. Accounting-active GIVE refuses legacy currency offering; durable quest submission currently selects item roots only. Integrate an authorized coin debit, frozen request/recipient/reward and idempotent recovery before enabling this route. Preserve fee, scarcity and guarded refusal; supplied master key and independent item requests are separate. |
| ZSQ-GITHZER-WHOLE-BUNDLES | Hunter requires one large hide plus two distinct small scraps; prophet requires five distinct44563 signets. Durable submit selects different loose object roots for each occurrence, maximum14, and requires the whole bundle together. Qualify incomplete counts, duplicate UID prevention, mixed colors/wrong ring, retained stock, all-or-nothing consumption and cold replay. Do not fabricate an incremental NPC-held collection state. |
| ZSQ-GITHZER-SOURCE-CUSTODY | Resolve exact item UID/generation/source/parent and admitted recovery separately from current loose preparation. P44509 uses locker44403; P44563 uses sarcophagus44561 at44752/53/54/55/58. Fresh successful O makes the normal P parent; global parent lookup and caps still need retained-world qualification. Sarcophagus flags5 mean closeable+closed, not locked. Remove worn44550 before offering; exact supplied proof fits without personal combat/tomb history. |
| ZSQ-GITHZER-ALTERNATIVES | Parchment44401 is consumed by sergeant or Zangzk; trophies and potion/map/poison are independent master-key requests. Item44509 is consumed by Zangzk information or foreign Alatorin83236/QA3427; no local reward item is issued. Preserve separate request identities, empty-reward settlement and competing inventory uses. Information is not a mandatory predecessor. |
| ZSQ-GITHZER-SPEECH-ACCESS | Four native key−2 doors at44626N/44678S/44679S/44715N use final keyword rrakkma/kraange/raylen/tayr-dryn. SAY clears LOCKED+SECRET only, with native speech guards. Qualify actual actor success, remaining CLOSED/BLOCKED, reciprocal state, supplied passwords and already-shared-open alternatives before personal clue/solve credit. |
| ZSQ-GITHZER-FLOOR-CONTROLS | Four fixed levers44568–71 are G on undead guardians44517@44759. Native no-corpse death can drop their inventory onto the floor; generic floor controls can remotely clear44760–63DOWN BLOCKED. Carried/worn controls instead require actor at target room. Do not change TAKE flags based on inventory appearance alone. Qualify death/factory publication, actual floor placement, exact receiver, remaining SECRET/CLOSED, repeated/shared-open state and actor evidence before any repair or achievement. |
| ZSQ-GITHZER-TRAVEL-FALLS | All22 local type25 declarations use exact commands/destinations/unlimited charges: BOW statue, STARE mirror pair, PRAY idol, TOUCH skull/spell, ENTER portals and cardinal floor routes differ. Objects44517/22/29 have no identified current O/P/G/E source or maintained literal producer; confirm intended unused prototypes before proposing supply. F80 at44586/87 andF35 at44624 go through flight/climb/mount/native fall guards. Qualify successful arrival independently from command acceptance. |
| ZSQ-GITHZER-ROYAL-ENDPOINTS | King44461 heartstone receipt retires him; arcane44462@44590 and followers already have independent resets. Prophet D text explains father/forcefield/rogue but does not grant personal clearance. Supply builder-authored actor/state/generation endpoints for feud, transform, awakened Rrakkma, father interaction, forcefield and king overthrow; do not infer them from reward or shared passage. |
| ZSQ-GITHZER-NARRATIVE-WORDING | Zangzk promises a sheath but Q41 rewards only katana44431+key44516; stranded44494 has empty greeting/completion reply and a narrated departure, not an escort. BLEED179, not SACRIFICE409, invokes shadow switch44502 despite the sacrifice inscription. Decide intended prose/command/endpoint changes with builders; any selected native repair belongs in a named fix/news commit. Current journal explains actual behavior. |
| ZSQ-GITHZER-RENEWAL-ACCOUNTING | Mode1 does not guarantee available daily stock: caps, consumed map/coins/hides/rings, wandering contacts and three retiring givers constrain renewal. Accounting-active O/P/G/E refusal lacks durable reset generation identity; guarded no-corpse and other source factories require admission. Qualify legitimate stock, bundle settlement, empty/multiple rewards, reset renewal and cold recovery without activating unsafe issuance or granting free materials. |
| ZSQ-GITHZER-FOREIGN-OWNERSHIP | Four foreign Alatorin recipes and16 foreign reset groups use local equipment/materials; incoming realm14210/wh55634/surface538611 and ravenloft2 portal59079 have separate ownership. Imported360 monolith and follower91053 remain distinct systems. R44488 mounts a mobile; object44488 is a different portal. Keep namespaces, source area/discovery and local accepted identity distinct; `_spec2_` is class specialization metadata, not a custom quest procedure. |

New progress requires active, ready accounting. Qualify legitimate supply, all-or-nothing custody/settlement, shared controls and actor endpoints. Native repairs require separate named fix/news commits. No native repair ships.

The paid master key and adamantite information are support services, outside achievement/daily credit. Eleven independent stories remain; all thirteen native accepted identities and historical receipts are preserved.


## The Caverns of the Worms: service recipes and competing materials

The [journal](../../areas/story/worms.story.json) and [dossier](../design/zone-stories/THE_CAVERNS_OF_THE_WORMS.md) demonstrate sixteen support commissions, exact repeated counts and supplied proof. ASK is descriptive, not order selection. Current have/count already works; source credit must use the roots actually consumed.

| Follow-up | Evidence and intended next step |
| --- | --- |
| ZSQ-WORMS-SELECTED-COMMISSION | ASK is descriptive; Q loading prepends requests, and durable GIVE chooses the first complete supported bundle sharing the offered kind. Shield/backpack/ring can precede overlapping boots/helmet/ear-clasp bundles. Define an explicit selected-contract invocation with frozen exact native binding, actor, distinct consumed roots/quantities and reward; preserve deliberate precedence and historical receipts. A presentation-only selection cannot change settlement. |
| ZSQ-WORMS-CONSUMED-UID-PROVENANCE | Native bundle search chooses distinct loose roots but does not force a surplus same-kind trigger pointer into the consumed set. Qualify intended bulk-trigger/indexed-item semantics before calling this a bug or proposing a separate fix. First-acquisition/source milestones must inspect actual frozen consumed roots and their admitted source/custody, rather than assume that the offered pointer was consumed. |
| ZSQ-WORMS-CURRENT-BUNDLES | All sixteen requests need their full bundle together, with repeated red hides, brown/glowing pieces and three red strips. Shield requires five roots, below durable maximum14. Renderer already shows current have/count. Qualify wrong colors/sizes, incomplete counts, distinct UID selection, competing allocation, loose versus worn/held/nested/spent stock and all-or-nothing settlement; do not invent NPC-held incremental collections. |
| ZSQ-WORMS-SOURCE-AND-SUPPLIED | Fifteen exact material kinds are G on corresponding small/middle/large worms;100 source declarations. Personal kill, death/corpse extraction and first-source acquisition need actor/UID/generation endpoints. Current supplied proof is valid without earlier combat/tunnel/control history. Reward possession and readiness do not prove accepted service or source. |
| ZSQ-WORMS-ACCOUNTING-RENEWAL | Mode2, cap1 Wilms and capped worm stocks do not promise repeatable availability. Accounting-active O/G issuance remains guarded until durable reset generation and legitimate factory admission are qualified. Test accepted source, bundle/output custody, cap/renewal, rejection and cold recovery without free materials or relaxed issuance guards. |
| ZSQ-WORMS-SHARED-GARBAGE-GATE | Fixed type29 object6931 uses PUSH270→6937SOUTH with value3=1. Raw door/reset state8 becomes runtime EX_BLOCKED128; item_switch clears BLOCKED, not a personal quest endpoint. Qualify exact actor/control generation and successful exit transition, remaining bits, shared-open passage and replaced control before personal solve credit. Preserve fixed placement and native command. |
| ZSQ-WORMS-EXPLORATION-ENDPOINTS | Grey/glowing/brown/red/purple burrows, duergar alcove and deeper connector are coherent guidance, not native clearance or boss quests. Darkness and actual sectors/flags govern movement. Builder-selected exploration/combat milestones need successful actor outcomes and renewal scope before a new achievement-bearing campaign. |
| ZSQ-WORMS-ENTRANCE-WORDING | Entry6900 says the Underdark is west, but its reciprocal Underdark811708 route is SOUTH; east into6901 is correct. Proposed repair: replace only west with south, retain all exits/flags/source policy, add original-fails/repaired-passes source-caption coverage and a separate named fix/news commit. No wording repair is selected or applied in this checkpoint. |
| ZSQ-WORMS-FOREIGN-VEIL | Ixarkon object96402@96524 owns illithid_teleport_veil; exact ENTER argument routes randomly among25 valid targets including6900, restores and waits. Qualify successful arrival/restore/accounting and existing travel/PvP policy under the owning zone. It is not required Worms progression or an accepted Wilms contract. No local literal procedure, ordinary incoming type25 declaration or foreign material recipe/reset group was found. |
| ZSQ-WORMS-SERVICE-HISTORY | All sixteen Q are independent supporting commissions. Preserve all native accepted IDs and receipts while raw16 achievement/daily candidates project to authored0; discovery remains separate. Belt's empty written completion reply does not remove its accepted receipt. Any future campaign terminal or crafting achievement requires deliberate builder semantics, rather than deriving it from a full equipment set. |

New progress requires active, ready accounting. Preserve native selection, scarcity, custody and travel policy. Native repairs require separate named fix/news commits; no native repair ships here.


## Castle Ravenloft: independent hand-ins and an unrecorded puzzle

The [journal](../../areas/story/ravenloft.story.json) and [dossier](../design/zone-stories/CASTLE_RAVENLOFT.md) distinguish accepted exact hand-ins, optional materials, supplied proof, same-item prop and shared custom puzzle. Use orientation for unsupported stages; do not invent an accepted receipt from an action or remote world state.

| Follow-up | Evidence and intended next step |
| --- | --- |
| ZSQ-RAVENLOFT-BELL-TRANSACTION | `ravenloft_bell` requires the exact wielded mallet58427, CMD_HIT and bell argument; resolves the first case58430 in remote58564, narrates, destroys the mallet synchronously, unlocks and changes descriptions. Add one admitted atomic transaction for exact actor/tool UID/custody/generation and remote case UID/generation/state transition, with rejection, replay and cold recovery. Do not award a solve from a command, shared-open case or supplied sword. |
| ZSQ-RAVENLOFT-BELL-REPEAT-INTENT | Current code checks the case prototype, not its locked state. Because unlocking leaves prototype58430, a repeated strike can destroy another mallet after the case is already unlocked. Builder should decide intended repeat behavior; proposed repair is a guarded initial-lock/state predicate in a separately named fix/news commit, qualified together with accounting consumption. No repair applied here. Case starts29, becomes21: still closed/pickproof; do not replace it with unused bright-case prototypes. |
| ZSQ-RAVENLOFT-TEMPORAL-SOURCE | Past Obox-Ob58364@58424 gives58393; future Obox-Ob58379@58560 gives58407. Track successful first acquisition from the admitted source/corpse root UID/generation separately from player-supplied custody. Independent appearances and supplied essences require no personal time-travel/kill order. Qualify two-source bundles, transfer, nested/worn/spent stock and scarce reset admission. |
| ZSQ-RAVENLOFT-ACCESS-CONTROLS | Outer black key58826 is distinct from holy symbol58825; courtyard58408, count58411, jail58315, desk58362, false treasury58421 and true treasury58415 affect different approaches. Native key−2 speech door uses obox-ob. Fixed command switches and remote/shared exits need successful exact actor/control/door-generation transitions before personal solve credit; ordinary use of an open route remains valid. |
| ZSQ-RAVENLOFT-RECIPIENT-RENEWAL | Mode0; ghost/wizard/inner Megosh D1 retire, while Perganan D0 remains. Wizard M has50-percent initial chance. Only Perganan is a potential local daily; availability is not promised. Preserve contacts, caps and accounting-active source/factory guards until admitted stock/recipient renewal and recovery are qualified. |
| ZSQ-RAVENLOFT-CROSS-ZONE-RECEIPTS | Perganan note→Urik/Barovia Continued; Gertruda→Mad Mary/Barovia; dracolich skull→Rahadin and old key→Gevin/Catacombs. Local carrying/delivery, onward acceptance and returned-key custody are separate; freeze recipient/source identity and attempt ownership. Outer and inner Megosh are distinct independently placed actors; do not infer a transformation/spawn chain. |
| ZSQ-RAVENLOFT-KILL-ACHIEVEMENT | Existing update_achievements/kill_gain advances Doru91031→Chernovog58835→Strahd58383 and grants EPIC_STRAHDME1000. Solo and eligible in-room group credit differ. Define settled actor/participant death endpoints and idempotent epic/progress publication before adding journal stages; retain native ordering, accounting and party policy. No new reward or duplicate kill achievement here. |
| ZSQ-RAVENLOFT-TRAVEL-AND-ORPHANS | Placed past/future type25 controls, paired imported handprint59326→59065 and foreign59325→58456, plus prison portal7371→7497, need accepted arrival/custody qualification. Ferrik book58300 READ→58325 and return control58304 WEST→58426 have prototypes but no active local/foreign reset or touching Q placement found; other ordinary routes exist. Builder should decide intended placement/retirement before introducing stock or personal objectives. |
| ZSQ-RAVENLOFT-NARRATIVE-AND-HAZARDS | Megosh's transformation, Perganan's escape, Dayheart's sunlight lore, lost servants, garden, adventurers, Ferrik/Mustakrakish books and Vistani combat help are not automatic quest endpoints. Preserve actual traps, underwater/space sectors, falling65/90/100, combat helpers, item effects and supplied alternatives. Perganan exceeds the32-alias presentation limit: advertise one verified alias per response before synonyms, retaining native recognition; plan topic groups/pagination if builders need every synonym displayed. Define deliberate lore/exploration outcomes after played qualification. |
| ZSQ-RAVENLOFT-CAPTION-INTENT | Catacomb plaque58343 still says coming soon despite connected Catacombs; case loot25745 is named Sunblade/longsword while its long/extra descriptions describe Sunlash/whip. Proposed separate builder-approved caption/lore fixes require checking shared Bahamut ownership and intended weapon, original-fails/repaired-passes fixtures and prominent fix/news commits. No content repair selected or applied. |
| ZSQ-RAVENLOFT-DISABLED-DESCENT | do_descend returns a disabled message before its old tome58424/book500032/five-orb400231 consumption and race/class/home transformation. Do not present it as an available quest or restore it during mapping. Any intentional revival is a separate accounting-qualified character/item transaction with explicit builder scope; tome's current equipped shadow-shield effect remains separate. |
| ZSQ-RAVENLOFT-PROP-HISTORY | Exclude the same-mallet lizard prop from four story units while retaining all five native IDs/accepted receipts. It was already not daily-eligible. Qualify raw5→authored4 history, replay/cold recovery and separate discovery. The prop neither supplies an initial mallet nor proves a bell solve. |

New progress requires active, ready accounting. Preserve native access, custody, stock and PvP policy. No native repair ships in this checkpoint.


## Barovia Continued: two Eva requests and alternative outer keys

The [journal](../../areas/story/barovia2.story.json) and [dossier](../design/zone-stories/THE_REALM_OF_BAROVIA_CONTINUED.md) keep five accepted hand-ins distinct from source, faction, puzzle, travel and kill evidence. Orientation describes unsupported prerequisites without manufacturing personal credit.

| Follow-up | Evidence and intended next step |
| --- | --- |
| ZSQ-BAROVIA2-SOURCE-PROOF | Zelenna58815@58941 carries locket58817; Drowned Lady58800@58832 wears eye58824; red dragon58821@58891 carries egg58809; Chernovog58835@58984 carries heart58834 and shard58830. Overseer58825@58848 wears badge58845; saboteur58819@58894 wears mark58844. Add admitted actor/source-root/item UID/generation/custody first-acquisition evidence, distinguishing theft/loot/reset issuance from player transfer. The accepted exact proof needs no earlier personal kill or acquisition. |
| ZSQ-BAROVIA2-MEMBERSHIP-INTENT | Eva's badge+mark→bandana+returned mark is a real accepted exchange, but native code has no stealth test or faction membership transition. Builder should decide whether the welcome is narrative or a real affiliation; any stealth/faction implementation needs explicit actor/action/outcome/participant policy, replay and recovery in a separate feature/fix commit. Do not infer stealth from a badge or faction from the reply. |
| ZSQ-BAROVIA2-ACCESS-TRANSACTIONS | Books58818 PUSH→58941S/blocked8 opens the hidden tower path; scroll58823 TOUCH→58975E/blocked9 opens Kashtrakis; same-named58822 TOUCH→58989D/blocked8 opens a different remote marilith route. item_switch checks exact control, command, target and blocked state; carried controls require the actor in the target room, while a floor control may act remotely. Value3 chooses captions. Add actor/control/door generation and settled successful transition evidence; shared-open use remains independent. |
| ZSQ-BAROVIA2-DEVIL-GATE-INTENT | Gate prose mentions true-blooded devils and a devil's touch, but the generic scroll opener has no race/blood check and accepts any otherwise valid actor. Clarify intended puzzle/race policy before any separately named native change; do not add a new restriction or loosen access during mapping. Qualify wrong scroll, same-name selection, floor/carried use, blocked/unblocked, reset and shared access. |
| ZSQ-BAROVIA2-RENEWAL | Mode0; Urik, Mirkodesiuska and outer Megosh retire on accepted D1. Eva's relic exchange D0 is the only potential local daily. The membership exchange returns an offered mark and was already daily-ineligible. Proof stock has caps; grinning imp scroll sources include a50-percent placement and wandering load routes. Qualify admitted accounting-active source/recipient renewal before promising availability; preserve all native caps, chances and departures. |
| ZSQ-BAROVIA2-CROSS-ZONE-OWNERSHIP | Perganan58347/Castle Lenience→note58416 and Urik58804 note→Moonfriend58849 are separate accepted receipts. Outer Megosh58846 is defined/owned by Barovia Continued but placed in Castle58567; journal contact visibility and owning-zone progress require played encounter/receipt qualification. Runtime arrived/encountered hints currently pass the physical zone to encounter_hint, so this contact can point at the Castle journal instead of his owning Barovia journal. Proposed separate universal hint fix should route to discovered owning quest zones while retaining physical discovery/encounter requirements and handling multi-zone contacts; qualify player-facing links and recovery before shipping. Inner Megosh58381 is independent. Crystal ball58803→Rahadin59070/Catacombs fortune59202, and the foreign Catacombs portal59091@59169→58964, have separate receipts/arrival; do not duplicate ownership or discovery. |
| ZSQ-BAROVIA2-EFFECTS-AND-ORDERED-KILLS | Symbol58825 is a reactive CMD_GOTHIT combat proc:1-in20, live-undead target, randomized destroy-undead/holy damage/fear; it is not a guaranteed Strahd kill, commanded ritual or quest receipt. Existing Doru→Chernovog→Strahd achievement/1000epic preserves order and eligible solo/in-room group attribution. Add settled death/participant/progress/epic and deliberately selected effect endpoints before journal milestones; no duplicate rewards. |
| ZSQ-BAROVIA2-TRAVEL-AND-COLLATERAL | Active portals58800@58897→58854,58819@58963→58964,58820@58985–58987→58988 and58831@58995→58963 are independent of turn-ins. Prototype58812→58897 has no active reset/recipe placement found. Tiny key58833 carried by Eva fits private chamber58894W/58895E and chest58832@58895; Varikov/Farkash key58854 fits drain58959U/58960D; Chernovog shard58830 fits58984U/58995D. Add admitted arrival/key/custody/state provenance only after actual source/access journeys; dormant portal is a builder placement/retirement decision, not automatically a bug. |
| ZSQ-BAROVIA2-CAPTIONS-AND-NARRATIVE | Eva's stealth topic requests two trophies, but her reply says keep this badge while native R returns the mark58844 and consumes badge58845. Badge58845 extra-description keyword also says mark saboteur. Proposed separate builder-approved caption correction should name the actual returned mark and correct the badge's examination alias without changing recipe/flags/stats; original-fails/repaired-passes fixtures and prominent fix/news commit required. Red-dragon location wording, unmapped private Eva chamber, scout/escort/arrival promises and wilderness corruption are narrative/source leads until intent and played qualification establish real endpoints. No native repair applied. |

New progress requires active, ready accounting. No native repair applied; proposed fixes need separate clear fix/news commits.


## Werrun: four requests and two kinds of quest system

The [journal](../../areas/story/werrun.story.json) and [dossier](../design/zone-stories/VILLAGE_OF_WERRUN.md) show books/braid as current preparation, mage debt as an explained blocked story and bartender missions as separate orientation. Preserve source caps, trap rooms, mirrors and native access; truthful scroll copy needs its own fix/news commit.

| Follow-up | Evidence and intended next step |
| --- | --- |
| ZSQ-WERRUN-SOURCE-PROOF | Library38380 has cap1 floor books38311/Revan and38326/Vewon; Lady Death38317@38381 wears braid38312/E13. Add admitted actor/source-root/item UID/generation/custody first-acquisition evidence, distinguishing theft/loot/reset recovery from gifts. Supplied exact materials remain valid independent deliveries; neither book custody nor braid proves reading, personal travel or a kill. |
| ZSQ-WERRUN-MAGE-PAYMENT | Q144 consumes350000 copper-value coins for sack38304/D0. Active quest.c refuses coin offerings, and durable item admission requires exact item roots. Add a separately qualified coin-only quest payment/acceptance transaction with frozen actor/recipient/price/output/receipt, failure preservation and replay/restart recovery before enabling. No fake coin-item step or shop-reopening claim; retains native receipt and current blocked card. |
| ZSQ-WERRUN-ACCESS-AND-TRAVEL | Bush38316@38300 PUSH270 clears38300W→38340/BLOCKED8. Broken mirror38317@38342 STARE139→38344; perfect38318@38344 STARE139→38342, both charges−1. Add settled actor/control/door generation and actual teleport/arrival facts; LOOK/examination, shared-open use and entering the other half of the same zone are distinct. Preserve native travel/PvP/access policy. |
| ZSQ-WERRUN-RECIPIENT-EPISODES | Traveller38308 starts38339 with north→bar38337 and E/S/W→exitless38341. Native wandering can strand him; D1 retires him after acceptance. Reset M probability is100 despite rareload prose, with cap1 and mode2. Measure actual availability and recipient instance/epoch before promising recurrence. Builder should decide whether the trap is deliberate rarity; no exit, placement, mobility or chance repair applied. |
| ZSQ-WERRUN-COLLATERAL-ALLOCATION | Malfun's Revan reward key38308 fits desk38307@38333 (500,29,38308,500), with flight38309 and venom38310 scrolls. Traveller carries metal key38336 for crates38337@38336/38364 (50,15,38336,0); only the ordinary crate has vigor38305 P stock. Official38319@38365 carries skullkey38322 for nightmare doors38365S/38366N. Qualify actual key/custody/container/lock results and allocation separately from quest receipt; alternative supplied keys remain usable. |
| ZSQ-WERRUN-NARRATIVE-ENDPOINTS | Malfun asks for a keepsake and mourns Lady Sklera, while Lady Death wears the braid; no compiled identity/transformation or rescue state was found. His four-color/healing recollection has only flight and venom scrolls actively placed in the desk. Proposed separate caption correction should accurately describe available scrolls after builder intent review; adding green/red stock or a wife finale is content design. Mage payment does not reopen his shop. Do not fabricate these outcomes. |
| ZSQ-WERRUN-GENERATED-MISSIONS | Bartender38309@38337 uses world_quest, minimumlevel11, generated target/history/fee/map/abandon/share rules; cached policy withholds all static Q offering/reward prototypes. It has no fifth static Q endpoint or local story completion hook. Future adapter needs stable source-system/mission-attempt/giver/target/participant/payment/reward identity and settled success, distinct from native Q and local daily units. Preserve current mercenary coin guard and no eligible-target failure/refund behavior. |
| ZSQ-WERRUN-ONWARD-OWNERSHIP | Cigars38314@38338→Frull43181@43292/Shipyard Q822→66666coins/D1 is foreign-owned. Lady Death carries imported memory55416/_noquest_→ambassador55136/WH Q2491→enchantment scroll55362+1000000coins+token55033/D0; starts55005/55400. Rune358 uses the separate settled epic-stone system, with native zone/level/peace/participant and once-per-zone rules. Neither is a local static-Q completion. Five Alatorin reset groups reuse local cigars/horse/beef at rooms declared by nexus56; three boundary edges include surface highway and foreign Incarnate dispersal ingress. No automatic campaign/source/discovery linkage. Qualify actual foreign ownership/arrivals and preserve earlier dossiers. |
| ZSQ-WERRUN-REWARD-AND-RENEWAL | All4 native bindings retained: Malfun twoD0 item requests are potentialdailies; mage coin-only has no daily item offering; travellerD1 retires but mode2 makes his book request a third potentialdaily. This eligibility rule does not guarantee actual recipient renewal. Native35kXP plus sandals has frozen recipient/cap/recovery handling, unlike unsupported payment input. Mode2/cap1 proof/recipient stock and nested access do not guarantee daily renewal. Qualify accounting-active source/consumption/reward/recipient/arrival journeys; exact synthetic receipts test projection only. |


## New Hope: service cards, actual recipe IDs and two reward sources

The [journal](../../areas/story/newhope.story.json) and [dossier](../design/zone-stories/THE_VILLAGE_OF_NEW_HOPE.md) explain four independent paid services, exact material readiness and current mixed-offering refusal. A vault item can be reset stock or death-created stock; first recovery and personal kills need their own event lineage. Preserve native access and supplied-key alternatives.

| Follow-up | Evidence and intended next step |
| --- | --- |
| ZSQ-NEWHOPE-SOURCE-PROOF | Mithral89117 Ocap2 at89098/89162; court guards89170/1/2 at89201/2/3 wield ordinary weapons89140/2/1 respectively, cap3/E16. Add settled actor/source-root/item UID/generation/custody first-acquisition evidence distinguishing recovery, theft/loot and gifts. Loose supplied materials remain valid preparation; no forced personal kill or source chain. Chaos equipment arrays also include local89169/89136/89143; avoid exclusive-source claims. |
| ZSQ-NEWHOPE-MIXED-PAYMENT | All4 Q consume mithral plus an exact ordinary weapon plus45000/100000/250000/10000 copper-value coins. Active quest.c rejects coin offerings and durable item admission rejects non-item goals. Qualify atomic multi-root plus coin payment/accepted output/receipt, actor/recipient epoch, frozen price, failure preservation and replay/restart recovery before enabling. No coin-possession readiness or current payment claim. |
| ZSQ-NEWHOPE-RECIPE-CAPTIONS | Q35 actually requires89141 shortsword although its M asks a dagger; Q43 requires89142 longsword although M asks a shortsword; Q60 requires89140 dagger omitted by M. Q51 correctly requires89142. Ordinary dagger89140 ground caption says shining; shortsword89141 inspection says longsword. Proposed separate fix/news commit should reconcile text to accepted IDs after builder recipe-intent review; do not silently change recipes or invent an upgrade chain. |
| ZSQ-NEWHOPE-COLLATERAL-ALLOCATION | Shining89118/89119 are not ordinary inputs89141/89142. Longsword and two-handed recipes compete for separate copies of89142 and fresh mithral/fees. Qualify material reservation and consume-once allocation; one carried copy may prepare alternatives but cannot settle both. Vault keys, shining outputs and shop black-handled weapons cannot substitute. |
| ZSQ-NEWHOPE-ACCESS-AND-RETURN | Hammerhead89156@89189 carries heart89019;89188D→89218 uses it (file6/reset6, secret/closed/locked but pickable). Return89218U file5/reset5 is secret/closed/unlocked. Lair89220N→89227 file3/reset2 uses eye89188; reverse file3/reset2 uses cracked dagger89152 (two Ocap2 vault placements). File3 is EX_PICKPROOF after setup_dir, not a runtime closed bit. Key breaks0/10/1 percent respectively; has_key accepts HOLD/loose inventory, not nested. Capture successful settled search/unlock/open and matched reverse-door generation separately from shared-open access; preserve breathing/movement/PvP restrictions. Tunnel89219 is native swamp26 despite submerged prose; lair89220 is underwater9. Changing sectors or return-key design needs builder intent, no repair inferred. |
| ZSQ-NEWHOPE-CUSTOM-DEATH-EPISODES | Sole literal mob89181→tentacler_death, native ACT_SPEC_DIE plus read_mobile ACT_SPEC binding. CMD_DEATH attempts one random89145–49 read_object then obj_to_room89227; reset has five independent Ocap1/20% floor rolls. Custom die path bypasses make_corpse; ordinary extract_char drops Geye89188/E16lash89144 in lair89220 when non-transient. Add committed actor/participant/kill attempt, recipient/source epoch and exact created-object lineage; do not treat reset stock, floor pickup, key ownership or proc invocation alone as personal kill/first-recovery evidence. No Q receipt emitted here. |
| ZSQ-NEWHOPE-RECIPIENT-EPISODES | Vitrius89102 starts89083, Hammerhead89156@89189 and tentacler89181@89220; cap1/probability100/mode2, no local Q retirement. Keys, mithral, guard weapons and vault stock have separate caps/chances. Qualify actual availability, NPC/source generation and renewal before promising repeat visits or daily stock; all4 authored crafts remain services with no daily credit. |
| ZSQ-NEWHOPE-NARRATIVE-ENDPOINTS | Janitor89165@89179 and scout89166@89195 explain lord/vault/dead/keep lore; no accepted scout report, prisoner release, Hammerhead soul liberation, village transformation or necromancer repair endpoint. Observer89083@89036 uses a descriptive mirror, with no local type25 control. Future builder-authored campaign needs explicit accepted tasks, branches and settled world-state facts; distinguish prose, training, conversation and actual outcomes. |
| ZSQ-NEWHOPE-ONWARD-OWNERSHIP | All4 local recipes only; imports203/204/424/835 have one touching foreign recipe, Newbie Red29233@29234 Q529 produces835+5000XP+heftybag29310 from bullfrog eyes29241/herb826/blood825/dust824/root822. Imported mindstone424 has guarded lucrot_mindstone journey and worn anti-banishment behavior. Four Alatorin O groups reuse table89008 at nexus56-declared83513/14/16/18. Two active surface boundary rooms618434/618833 reciprocate89217; no incoming type25 target. No automatic campaign, source or discovery linkage. Preserve foreign journal ownership and existing travel guards. |
| ZSQ-NEWHOPE-REWARD-AND-RENEWAL | Preserve native4 receipt identities through raw4-achievement→authored4-service projection; earlier acceptance remains evidence while authored achievement total becomes0. Qualify actual mixed payment, consumed roots, selected reward/recipient custody and accounting recovery independently from projection. Custom vault production has no current receipt or frozen recipient attribution. Source proof, service acceptance and discovery remain distinct; new credit needs active, ready accounting. |


## Yerdonia: explain bundles without inventing a linear campaign

The [journal](../../areas/story/raxthan.story.json) and [dossier](../design/zone-stories/DRUSTLS_YERDONIA_ENSLAVED.md) preserve ten exact requests, optional materials and verified contacts. Drustl retires after either request; hearts compete across recipients. Follow F-owned stock and actual door/reset state rather than leaders or descriptive captions.

| Follow-up | Evidence and intended next step |
| --- | --- |
| ZSQ-YERDONIA-SOURCE-PROOF | Strange shrooms42926 have nine Pcap9 placements inside backpack42925/E27 on merchant42913@42924; cave shrooms42927 have seven Ocap7 placements. Other proof sources: Raxthan42900/Ghead42931@42949; follower Noctule42934/Gheart42940@42949; follower slug42933/Garrow42934@42987; Agama42950/Gtongue42957@42992; Graskal42902/Ghead42939@42994; Morklar42940/Gscales42937@42983; Elytron42935/Ghead42942@42970; zvorgast42942/Gfang42949@42997; Bloodbeast42943/Gheart42948@42966; Darthus42949/E16spine42956@42984. Capture settled actor/source-root/object UID/generation/custody first acquisition; gifts, loot, theft, disarm and nested recovery need distinct provenance. Do not make personal killing mandatory for item acceptance. F loads a new follower and changes the current G/E recipient: Noctule and the smaller slug, not their leaders, own the subsequent proofs. |
| ZSQ-YERDONIA-CONSUME-ONCE-ALLOCATION | Noctule heart42940 serves Dravkult QA194 and T'rin QA329; Bloodbeast heart42948 also serves Alatorin QA6574. Qualify independent item-root reservation/consume-once allocation, competing recipients and failure preservation. A single copy may show both alternatives ready but cannot settle both. Preserve exact bundle quantities, especially three42927 for QA398; one/two copies and rewards are not accepted evidence. |
| ZSQ-YERDONIA-RECIPIENT-EPISODES | All10 Q/QA have D1; Drustl42912@42922 owns two independent requests and retires after either. Other contacts start42929/42905/42933/42934/42956/42950/42971/42981. Each Mcap1/prob100, zone mode1. Native classifier marks10 potential dailies via reset renewal; this is not measured availability. Qualify recipient UID/epoch, pending offering, committed retirement, fresh reset and restart/replay policy without merging Drustl's two receipt identities or guaranteeing two same-visit acceptances. |
| ZSQ-YERDONIA-SETTLED-CONTROLS | Stationary type29 symbols42954@42943/42958@42944 use CMD_TOUCH320 and shared item_switch, targeting42943N/42944S. Near reset8 is blocked/open, reverse reset1 closed/unlocked/unblocked. The switch clears EX_BLOCKED only; no actor achievement emitted. A second-side touch has nothing to clear in ordinary reset state. Capture successful changed-state actor/door generation separately from invocation/shared-open access; preserve ordinary OPEN on return. Review builder intent before changing the redundant return control or rock state. Both declared reverse exits exist; no missing reciprocal fix selected. |
| ZSQ-YERDONIA-FALL-AND-ACCESS | File door masks keep only low two bits. Slab42940S↔42945N file5/reset1 is closed/unlocked, not initially secret; prose cannot create SEARCH prerequisites. Room F fields are fall chances69/83/53/81/9/18/33 at42969/82/86/93/95/96/97, not destination IDs. Shared falling_start/step applies floating/climbing/door/event/injury/relocation guards and checked char_to_room. Capture settled arrival and personal control solve separately; keep travel/breathing/combat/PvP policy. Race-guarded prime-shift may also select42950; physical route steps cannot be universal prerequisites. |
| ZSQ-YERDONIA-CAPTION-REVIEW | Dravkult's raxthan/graskal M says two heads, while noctule M and actual QA194 require Graskal head42939+Noctule heart42940. Propose a separate named fix/news commit reconciling misleading text to approved native terms. T'rin's first/second-heart prose is not an enforced sequential quest in the atomic bundle. Clarify that wording after builder intent review rather than inventing deposit stages or changing G IDs. |
| ZSQ-YERDONIA-SPARED-LIFE-AND-NARRATION | Volgk's M says Agama may be spared, but tongue42957 is native G stock; no custom cutting/spared-life endpoint is assigned. Epolon dragon morph, T'rin hellfire, Grobklarn ritual and Drustl/Dravkult freedom text run as D act followed by extraction, without a persisted new creature or liberated-world state. Future builder-authored branches need explicit accepted actor/outcome/participant/world-state evidence; do not derive violence, rescue, transformation or liberation achievements from item custody or captions. |
| ZSQ-YERDONIA-SHOP-AND-ORPHANS | One complete dealer shop42963@42912 and all28 reset imports reviewed. Azure potion42953 is also shop stock; purchasing/owning it does not prove QA398. Imported6122 has no active prototype and its G reset is disabled by renum_zone; no local quest requires it. Propose builder stock-intent review then a separate fix/news commit removing the stale reference or supplying an approved replacement, without arbitrary item creation. Local descriptive rune/Bozk/Baal/Baphomet imagery, imported node stone358 and _noquest_ memory55453 create no additional accepted local Q endpoint. |
| ZSQ-YERDONIA-ONWARD-OWNERSHIP | Only one touching foreign recipe: Alatorin Ravi83406@83762 QA6574 consumes Tentabeast heart20253+Bloodbeast heart42948→signet83447+C100000+E85000/D1. Full recipe/dialogue/recipient/proof/reward/recipient-room/reset reviewed; keep its journal/receipt ownership foreign. Zero touching foreign reset groups; six boundary edges/three full foreign rooms595852/596251(surface) and749737(underdark), all713 active type25 declarations/no incoming targets. Prime-shift route42950 and chaos-kit bow42903 are additional compiled references, not new local requests or exclusive-source proof. |
| ZSQ-YERDONIA-REWARD-AND-RENEWAL | Preserve10 exact bindings/10 achievements/10 potentialdailies through raw→authored projection. Item-only offerings are supported structurally; qualify real committed consume/reward roots, native XP caps/frozen participant split, Trosat coins, retirement, failure/replay/cold recovery and accounting persistence before claiming a played daily journey. Discovery remains separate; new credit requires active, ready accounting, frozen recovery separate. No guaranteed stock or daily reward claims. |


## Arcaneum: distinguish same-name requirements and source limitations

The [journal](../../areas/story/library.story.json) and [dossier](../design/zone-stories/THE_ARCANEUM_OF_LSRILLIZZIN.md) give eight fragments separate exact checks with native color names and numbered labels without inventing an order. Document unsupported paid service and actual native availability. Staged source qualification and collector routes need builder review, never player instructions to enter staging rooms.

| Follow-up | Evidence and intended next step |
| --- | --- |
| ZSQ-ARCANEUM-PAID-ADMISSION | QA10 consumes C10000→key402003/D0. Active accounting refuses numeric coin offerings in execute_quest_routine; no supported item root or coin-readiness surrogate exists. Design guarded, durable coin payment plus exact reward/receipt/replay and a settled access episode. Keep this paid service separate from five story achievements and discovery. Do not enable legacy payment or relax access. |
| ZSQ-ARCANEUM-SOURCE-CUSTODY | Six essences have Gcap1 on initial golems402001–6 at402009/10; additional summoning-room copies have no essence G. Seven proofs have Gcap1 on specific undead402019–25 at402057/58/60/61/62. Ten book Pcap1 records target shared bookcase402010 (Ocap11); reset P uses get_obj_num, not an explicit immediate-parent pointer. Solbeep journal402029 Pcap1 targets desk402030@402021. Qualify actual settled object UID/generation/root/container custody, gifts/theft/loot and availability; nominal room grouping is not guaranteed parent identity or personal first recovery. |
| ZSQ-ARCANEUM-DREAM-SOURCE | All eight fragments402041–48 have file cost0/weight0, shared plain-text name/keywords/values, distinct native color codes and Ocap1 placements in402101–108. instantiate_object_template calls convertObj: GLOW raises runtime cost to100 before ±2% randomization; no IGNORE, NODROP or TRANSIENT flag prevents this, and RateObject returns0 for treasure. Ordinary scavenger cost/rating selection can therefore accept them, subject to actual custody and actor guards. Demon402035/Mcap1@402109 chooses one fragment room, whose only DOWN exits to treasury402100; it cannot ordinarily return to tour the other seven. A fresh available demon episode can select another kind, with native caps affecting stock; one encounter is not an eight-item source guarantee. Qualify actual loader value, settled pickup/arrival, reset generations and accumulation across episodes. No source repair established merely by file value0 or a one-way staged approach; review builder intent before proposing any supply or mobility change. |
| ZSQ-ARCANEUM-SAME-NAME-IDENTITY | QA85 requires each exact402041–48, not any eight fragments. Eight optional cards retain exact identity with native color names and numbered editorial labels and no offering order. Consider additional color-independent keywords/descriptions only after builder naming intent review; qualify duplicate-kind, partial-set, spent-stock and command-target ambiguity. Receipt/source adapter must preserve exact prototype plus UID rather than infer distinct souls from captions or count. |
| ZSQ-ARCANEUM-COLLECTOR-ARRIVAL | Razeline402061/Mcap1@402148 has SCAVENGER, no SENTINEL and no TEACHER; native wander can select nine exits into no-exit402149 or SOUTH→402147→DOWN402002. Neither staging room has ROOM_NO_MOB or private flags; sector0 and shared CAN_GO permit ordinary selection subject to actor state. Thus public arrival is possible, not guaranteed; trap has no ordinary departure. Builder review should choose a deliberate route/recipient availability policy, followed by a separate named fix/news commit and reset/wander/restart qualification. Do not direct players to staging rooms or claim measured failure frequency. |
| ZSQ-ARCANEUM-ACCESS-EPISODES | Portico402007E↔402008W file3/reset2 is closed/locked/pickproof (near key402003, reverse key0). Treasury402073N↔402100S file7/reset6 is secret/closed/locked/pickproof with key402039, Gcap1 on dream-memory402036@402093. Both keys' break chance100 enters guarded durable key destruction; unlock state and key-destruction acknowledgment can diverge on failure. Capture successful search/unlock/open and actual arrival separately from attempts/shared-open access; supplied key may work without personal paid receipt. Preserve PvP/travel/access policy. |
| ZSQ-ARCANEUM-DESK-AND-PICKS | Desk402030 values300/13/0/300 is closeable/closed/locked, not pickproof; PICK requires a held ITEM_PICK plus skill, visibility, timer and success. Shop stock402031 has wear_flags0, while Gringobbi's job speech offers it. Investigate HOLD/shop policy and intended external picks before a separate named text/equipment fix; do not enable take/hold flags as part of journal work. Staff KNOCK is not an ordinary player solution. Qualified lock/open/container withdrawal or gifts can satisfy current item preparation without personal solve history. |
| ZSQ-ARCANEUM-PORTAL-AND-FALL | Type25 portals402037@402072→402073 and402038@402099→402072 use value1=CMD_ENTER7, value2=-1 (unlimited), not a level range or flag set. Full check_item_teleport and teleport_to reviewed; invocation/caption does not prove char_to_room success. Room F percentages60@402051/116,70@402052,50@402071 are fall chances. Crypt402059N file5/reset1 is closed/unlocked/non-secret; inn402115D file4/noD masks to ordinary exit. Add settled arrival/control adapters without mandatory false SEARCH or source-path prerequisites. |
| ZSQ-ARCANEUM-LORE-SERVICES | Fourteen addressed M/MA across11 contacts/31 aliases, no ambient. Ne'kalsai's curse has no local Q/cure. ACT_TEACHER auto-binding and epic_points binding make class/epic service behavior separate; Shezeera402029 teaches natures ruin but active accounting refuses epic purchase. Imported3097 counter has storage-locker hook and bank caption; fountain402000@402002 is drink type17/liquid28 unholy water, not a travel trigger. Shops, teacher specialization, soul-mending D narration, fountains, equipment and chaos-kit quill402052 do not add local accepted endpoints. Future training/cure/world-state stories need explicit settled contracts. |
| ZSQ-ARCANEUM-RECEIPT-RENEWAL | Preserve six exact receipt identities through raw→authored projection; paid admission changes local achievements6→5, five potential dailies unchanged. Four D0 recipients remain after acceptance; Gazdiel/Razeline D1 retire. Native stock caps/resetmode1 and source episodes/collector route limit availability, despite potential daily classification. Active accounting refuses reset item issuance before read_object pending durable reset-generation identity; M/D remain separate. Qualify guarded reset producers, atomic six/seven/eight/ten-item roots, reward/XP caps and frozen participant split, retirement/reset, failure/replay/cold recovery/persistence with accounting active and ready. Synthetic journal events prove projection, not completed normal-play supply or settlement. |


## Mistywood: verify native source behavior before requiring a method

The [journal](../../areas/story/mistywood.story.json) and [dossier](../design/zone-stories/MISTYWOOD.md) show why floor reset stock and scavenger equipment are both possible source states. Give actionable exact-item guidance, keep lore referrals optional, and distinguish NPC retirement from relocation. Never repair missing-looking E stock or connect old perimeter targets solely from narrative guesses.

| Follow-up | Evidence, required capability and qualification |
| --- | --- |
| ZSQ-MISTYWOOD-ACCOUNTED-SUPPLY | Necklace Ecap1/prob100 on blackbear95005@95108; club Ocap1/prob90@95075; mushroom Ocap1/prob100@95080. Shared reset_zone refuses O/P/G/E before read_object during active accounting, pending durable reset-generation identity. These declarations are possible historical sources, not fresh active-epoch guarantees. Qualify guarded provenance/production and recovered stock; never re-enable legacy issuance to make a journal completable. |
| ZSQ-MISTYWOOD-SOURCE-CUSTODY | Three optional loose-carried checks accept exact gifts without invented killing or first-recovery requirements. Add/qualify settled source, mobile claim, corpse recovery and player-transfer evidence with operation/item/actor/encounter IDs before a builder requires personal retrieval. Necklace recovery is not a kill receipt; possession of rewards is not acceptance. |
| ZSQ-MISTYWOOD-GIANT-SCAVENGING | Giant95002@95075 is SCAVENGER+SENTINEL; club is floor O stock, with no matching E/G. Real convertObj/RateObject regression establishes positive selection inputs: runtime costs5096/5200/5304, weight21, positive rating, TAKE/WIELD, two-handed/FLOAT flags. Shared pickup→CheckEqWorthUsing can claim/equip after actual custody succeeds. Qualify native timing/claim/equipment/race/actor/accounting guards before a repair; missing E alone is not a broken source. Preserve stock chance/cap and do not require personal combat. |
| ZSQ-MISTYWOOD-RESCUE-RETIREMENT | Jarnes Q113/D1 consumes95003, rewards95002+E10000, prints escape then extracts NPC; no relocation beside Daumis, escort or player teleport. A genuine reunion/escort requires an approved destination/attempt contract and settled adapter. Confirm builder intent before any separate native relocation/text fix; do not invent a fourth receipt. |
| ZSQ-MISTYWOOD-REFERRAL-CHOICES | Daumis has only M referrals; hunter/druid rivalry is lore without mutual exclusion or allegiance state. Preserve independent three receipts and optional prior dialogue. Required learned topics, referral history, ethical branches or forest restoration need explicit personal/party/world scope and settled contracts rather than inferred mandatory chains. |
| ZSQ-MISTYWOOD-ACTUAL-ACCESS | Lower tower95044W↔95045E and cabin95169E↔95170W are closed/unlocked at reset1. Laboratory95047UP reset2, roof95048DOWN reset1, file3/key95022; Kromor Gcap1 key has break chance0. Vegetation95072E reset5 versus reverse1; foliage95101S reset1 versus reverse5. Qualify actual reveal/unlock/open/arrival and shared versus personal scope; no false mandatory key/search history or repair of asymmetry without intent review. |
| ZSQ-MISTYWOOD-LORE-ORPHANS | Korred/Kromor unwanted immortality, laboratory recipe/book/runes, dead horse/silver arrows, lizard remains, skeleton containers and animal trophies have no accepted cure/delivery/combination contracts or local assigned custom proc. Room extra descriptions are not carried recipe objects or actionable verb declarations. Record proposed builder authored expansions explicitly, after inventory/event/transaction design, instead of advertising finished quests. |
| ZSQ-MISTYWOOD-NATIVE-HAZARDS | Speckledmushroom95003 is food19/value3=1; native EAT applies sickness/negative hit-regeneration and spends food, not teleport or rescue. Pit95059 and road95060 have ROOM_NO_HEAL; river/no-ground branches retain native movement/fall rules without local F/C records. Ascuren has neither SENTINEL nor ACT_TEACHER, so shared native wandering is not stopped by the teacher/player-presence rule. Qualify hazard/recipient presence independently of accepted completion; preserve food type and native flags. |
| ZSQ-MISTYWOOD-PERIMETER | 42 touching boundary edges: three incoming from fully read pineholl16164/krimman16486 (NO_MOB/NO_SUMMON staging) and underworld3 30162, reciprocal95045DOWN→30162, plus38 outgoing old-map declarations to36 absent targets228xxx–233xxx. make_wld concatenates active AREA files; real_room0 performs exact lookup; renum_world drops missing exits. No alias conversion rescues them. Exclude those directions from player guidance; review intended current-map reconnection or deliberate obsolete-exit cleanup as separate fix/news work, preserving valid Underdark ingress and never opening staging. |
| ZSQ-MISTYWOOD-RECEIPT-RENEWAL | Keep three exact raw→authored receipt identities/three achievements/three potential dailies; discovery separate. Riliatar/Ascuren D0, Jarnes D1; stock caps/resetmode2 and other players constrain renewal. Qualify atomic item consumption, coin/XP/item reward continuation, XP limits/frozen participant split, NPC equipment retirement/reset, failure/replay/cold recovery/persistence with accounting active and ready. Synthetic receipts prove journal projection, not these played journeys. |


## Pharr Valley: verify each input, reward and runtime source

The [journal](../../areas/story/pharrvly.story.json) and [dossier](../design/zone-stories/PHARR_VALLEY.md) show three current material rows feeding one accepted recipe, with a second independent receipt and retiring recipient. Validate promised prose against actual inputs/rewards and reset/compiled producers. Use optional clues and actual PUSH/open/pick routes; sharing prototype files does not place ship objectives in the valley.

| Follow-up | Evidence, required capability and qualification |
| --- | --- |
| ZSQ-PHARRVLY-ACCOUNTED-SUPPLY | Apple40208 has10 Ocap10 placements; orange40209 has11 Ocap11; nest40210 has4 Ocap5 and1 Pcap5 placement in scarecrow40201; carapace40213 Gcap1 belongs to leader40215@40381. Active reset_zone refuses O/P/G/E before read_object until durable reset-generation issuance is qualified. Preserve these guards/caps and qualify admitted or recovered stock; static declarations do not promise fresh active-epoch supply. |
| ZSQ-PHARRVLY-CURRENT-BUNDLE | Q87 consumes three distinct loose roots40208/09/10 in one durable batch; Q100 consumes40213 independently. A nest is a container: qualify parent withdrawal, owned children, complete-tree destruction/publication and refusal/recovery before accepting a filled root. P finds an extant matching scarecrow, not guaranteed preceding-O ownership. Gifts fit without personal source history; add settled source/mobile/corpse/player-transfer IDs before a builder requires retrieval. |
| ZSQ-PHARRVLY-REWARD-TEXT | Q87 declares I40208+I40209+I40210→E2500/D1, while its prose mentions book/carapace and hands over a parchment scroll. No item reward is declared. Journal follows the actual recipe without promising a scroll. Builder intent must choose corrected prose or an explicitly designed produced scroll/reward; any actual repair needs separate fix/news, exact before/after tests and accounting continuation coverage. |
| ZSQ-PHARRVLY-CARAPACE-SOURCE | Farmer M dialogue points to soldier Gartham, but only leader40215@40381 Gcap1/prob100 declares40213; ordinary40216/17 soldiers do not. Preserve source scarcity and exact identity. Proposed separate fix/news after intent review is source-guidance alignment or a deliberately chosen stock design, rather than adding copies to every soldier. Acceptance checks the shell without a personal kill. |
| ZSQ-PHARRVLY-CLUE-ACCESS | Case40204 closed/unlocked holds Pcap1 unknownkey40203/break0; chest40202 Ocap1@40229 is closed/locked/HARDPICK with key40203 and Pcap1 paperwad40211. HARDPICK2 does not implement PICKPROOF16. OPEN/PICK/KNOCK/key use follow native ownership/access rules; supplied key/already open routes remain valid. Reading E descriptions/curator referral does not gate the farmer or award a learned-topic receipt. |
| ZSQ-PHARRVLY-SHARED-PASSAGE | Rock40214 type29 values270/40223/EAST1/mode0, Ocap1@40223; D40223E state8 blocks40384, reverse40384W state0. Shared item_switch auto-binding requires selected object and PUSH270, then clears actual forward/reciprocal blocked state. Qualify actual operation/actor/world-attempt/reveal/arrival identity and scope before a personal puzzle objective; preserve shared shortcuts and native fences/doors/F10 cave falls. |
| ZSQ-PHARRVLY-RECIPIENT-LIFECYCLE | Farmer40201 M cap1@40281 is SENTINEL; Q87/D1 retires him, Q100/D0 leaves him. Carapace-first can preserve a current farmer; bundle-first requires a fresh available recipient for another request. Curator40200 M cap1@40250 is neither SENTINEL nor TEACHER and may wander. Qualify disappearance, inventory retirement, reset generations, presence and concurrent attempts without imposing a false sequential campaign. |
| ZSQ-PHARRVLY-SHIP-SCOPE | Five npcShipCrewData tiers own55 mobile prototypes40220–74 and chest/key pairs40215–19/40220–24. Runtime ship locations/zone600, crew cleanup, actual ship attempt and custody are separate from Pharr physical discovery. load_treasure_chest refuses active accounting before creation; Cyric's Revenge/crew also refuse, with generated hatch40225/core12029 elsewhere. Settled ship issuance/crew encounter/loot/hold/world scope needs its own adapters. Capacity's >40200<40300 exemption also includes local farmers/Gartham if aboard: review intended classification separately without claiming a demonstrated exploit or applying an unreviewed balance fix. |
| ZSQ-PHARRVLY-UNFINISHED-LORE | Curator's lost farmer's-wife chest, paper referral, Skexis history, farmer's promised scroll, peaceful peak and cave-war lore have no additional accepted rescue/history/cure contract. Imported40072 book is ordinary literature,67255 sleeves separate loot, generic herbs/food ordinary supplies. Rock passage and ship/Soldon435→40248 pets do not invent local outcomes. Explicit builder campaign/learned-topic/recovery/branch contracts and settlement must precede deeper story credit. |
| ZSQ-PHARRVLY-RECEIPT-RENEWAL | Preserve two raw→authored identities/two achievements/two potential dailies; discovery separate. Qualify exact three-root versus one-root destruction, XP limits/participant/frozen split, NPC disappearance/stock reset, failure/replay/cold recovery and accounting persistence with active, ready accounting. Native hazard/access outcomes and normal-play completion remain pending; synthetic receipts prove projection only. |


## Tribal Oasis: recipes, access and narrative claims

The [journal](../../areas/story/oasis.story.json) and [dossier](../design/zone-stories/TRIBAL_OASIS.md) show independent shared-ingredient services and one two-proof mayor receipt. Map an exact carried rescue object separately from a mobile escort. Preserve supplied keys/gifts and actual shared SAY/unlock/OPEN routes; ambient remarks, remote scroll prose and an old training table cannot prove available powers.

| Follow-up | Evidence, required capability and qualification |
| --- | --- |
| ZSQ-OASIS-ACCOUNTED-SUPPLY | Six garden/burrow/mine ingredient kinds, floor girl78057, captain emblem78026, queen heart78030 and ambassador wing78063 have exact O/G/E caps/chances. All381 resets and147 expanded families reviewed. Active reset_zone refuses O/P/G/E before creation until a durable reset-generation issuer is qualified; preserve caps/refusal and qualify admitted or recovered stock rather than inventing guaranteed daily copies. |
| ZSQ-OASIS-SHARED-INGREDIENTS | Five independent alchemy services share rose/orchid/weed/moss. One exact current root can prepare several cards, but one accepted recipe spends its own full distinct bundle and does not complete the others. Qualify batch destruction/output issuance, hand-away/held/nested/spent refresh, replay/cold recovery and provenance before personal gathering requirements. Potion possession or quaffing is not another accepted recipe. |
| ZSQ-OASIS-PAID-TOWER | Q103 C100000→I78007/D0 is100 platinum, a service rather than victory. Native static-Q active accounting refuses legacy coin offerings; qualify exact denomination/debit, payer/recipient/attempt, key issuance, failure/recovery and replay before enabling. Five later gladiator keys78006/08/09/10/11 differ from initial78007; preserve current shared doors, supplied matching keys and permitted native alternatives. No synthetic fee, personal kill or treasury receipt. |
| ZSQ-OASIS-SHARED-ACCESS | Secret village down78075→78105, southwest78138→78140 and queen-map west78200→78201 are actual concealed exits, not dialogue achievements. Royal wall78194E↔78195W uses key78026/break100. Magic78295N↔78303S key-2/keywords tomb mortazoth starts locked; successful permitted SAY matches the final keyword and clears LOCKED/SECRET with reciprocal matching but leaves CLOSED. Qualify reveal/unlock/open/arrival actor and world generation separately; preserve supplied keys, already-open shortcuts and native PICK/KNOCK rules. |
| ZSQ-OASIS-PROOFS-AND-RESCUE | Mayor QA62 consumes queen heart78030+ambassador wing78063 together for armor78066+cloak78067. Q192 accepts floor object78057, a65-weight treasure/quest item, for ring78068. Different source/scope from mobile78057. Neither contract enforces personal kills, referrals, escort, miners saved or prevention of future raids. Add explicit builder campaign/actor/escort/outcome contracts and settled evidence before those objectives. |
| ZSQ-OASIS-REMOTE-SCROLL | Q11 requires Bel heart32490, unmaking orb26614 and tablet402→scroll407. Ten touching foreign bindings and two foreign source groups fully read. Bel Gcap1 and Dark Ecap1 retain remote availability; tablet402 has no active declared reset supply, and no compiled producer found. Tablet prose names an extra artifact absent from actual Q11. Scroll407 is trash13 with zero values and no bound stat power; generic READ delegates LOOK, RECITE requires ITEM_SCROLL. Preserve current terms; builder chooses source, prose alignment or explicit accounted stat-use design after intent review. Any native repair separate fix/news. |
| ZSQ-OASIS-TRAINING-INTENT | Bargor's two qc_action45 messages are ambient. Epic table has78006→ENCHANT, but ENCHANT reward row is commented, only46 is explicitly bound epic_teacher, Bargor flags2122 lack ACT_TEACHER, default teacher binding cannot help, and epic_teacher requires a live reward row before purchase. Generic FindTeacher requires ACT_TEACHER or consenting trusted teacher. This is incomplete/legacy training intent, not a journal reward; decide intended availability and a settled teaching contract separately. Do not activate training as a mapping repair. |
| ZSQ-OASIS-PROSE-AND-STAGING | Gnome describes fungus by the southern wall while Ocap3 actual stock is78119/78137/78161 underground. Full room/exit/body review distinguishes narrative island/shops/falls from actual Surface entrance, no local shop file or matching shopkeeper declaration, and no physical room F/C records. Illithid embassy78202 Mcap1/Fcap3/prob100 plus equipment and staging78203/no exits are not proof of a completed raid or a guaranteed rare portal event. Builder reviews intended supply directions/shop/staging behavior before any repair or new encounter objective. |
| ZSQ-OASIS-CONSUMPTION-AND-RENEWAL | Native potion values resolve actual spell IDs: adrenaline vigorize-critic×2/pantherspeed with10 damage; green concealment/wellness/inertial barrier with10; swirling blindness/cure-blind/protection with100; elixir heal/strength/dexterity with15; sweet haste/infravision/dexterity with0. Shared QUAFF applies timer/combat spill/no-magic/damage/spells then extracts; qualify durable consumption/effects/refusal/recovery before promising safety or cure history. Three achievements/three potential dailies and six services preserve nine distinct native identities. All D0; actual repeat stock, consumption, reward and persistence journeys pending. |


## The Great Realm of Duris: one choice, foreign ownership and dynamic dispatch

The [journal](../../areas/story/connectorzones.story.json) and [dossier](../design/zone-stories/THE_GREAT_REALM_OF_DURIS.md) show three alternative five-item receipts in one story, two services and current quantity4 material preparation. Explain union rows clearly until selected-branch grouping exists. Keep remote finishing/mystic/plane receipts in their owning zones. Review normal boot/table bindings as well as literal special assignments; dialogue and reward prose do not prove class/kill/learned-name/stat effects.

| Follow-up | Evidence, capability and qualification |
| --- | --- |
| ZSQ-CONNECTOR-ACCOUNTED-STOCK | All492 resets/full203 parent/location families: ember Pcap1 in campfire Ocap1@53934; four eagle M/G stock cap4; princess/prince Gcap1 heads; reviewed imported input sources and local/foreign stock retain exact caps/chances. Active reset-zone O/P/G/E admission remains guarded. Qualify durable generation, scarcity, repeat availability and recovered stock; never invent guaranteed daily copies. |
| ZSQ-CONNECTOR-FOUR-ROOT-FEE | Q31 needs4 distinct I53622+C10000→I53623/D0. Current schema quantity4 represents preparation, not payment. Active legacy mixed coin offering refuses. Qualify atomic exact-root destruction, denomination debit, reward issuance, refusal/replay/cold recovery and actor/giver/attempt context before enabling. Service remains outside achievement/daily credit. |
| ZSQ-CONNECTOR-BRANCH-SELECTION | Three D1 adventurer recipes consume different five-item bundles and give component+ring, then retire one giver. One story with three ANY terminal receipts fits current projection. Add builder-declared branch groups and selected-branch material rendering so the union of nine optional materials can show only its five chosen roots; don't imply nine or fifteen are mandatory. Preserve all three raw identities in history. |
| ZSQ-CONNECTOR-REMOTE-FINISHER | Surfmini97907 owns four D0 crafts: component53650/51/52/46+sphere97921+3 large mithril400280→53661/62/63/64. Native separate ownership is retained. A future campaign needs selected-branch/output-root lineage and foreign-owner receipt links before declaring the matching artifact finished; a different branch's gift or local ring isn't evidence. Qualify mining/smelting/furnace provenance and exact three-root custody separately. |
| ZSQ-CONNECTOR-SHARED-ACCESS | Six PUSH switches53612/13/29/30/53/54 have exact cmd270/room/direction/mode; item_switch clears actual shared BLOCKED routes/reciprocals. Five local keys have different break0/100 semantics; fixed sunwell/crack/tent/moonwell portals declare exact destinations and value7/-1. Qualify successful actor action, generation and arrival separately; supplied keys, already-open routes and native permitted alternatives fit. No synthetic key/dialogue/press achievements. |
| ZSQ-CONNECTOR-MYSTIC-AND-PLANES | SurfaceQA71 giver500023 consumes four lockets500028–31+book500032→sash500033+key500034/D1. Shadow's optional lead isn't that receipt. Local imported lockets54616/17 differ from those four. SurfminiQA341 giver97917 consumes six distinct55198–55203→sphere97921+wand97923/D1. Foreign journals own both chains. Explicit campaign/custody/gift policies and six-source lineage are needed before personal first-recovery prerequisites. |
| ZSQ-CONNECTOR-SCROLL-INTENT | ChauseisQ52:Bel heart32490+orb26614+tablet402→Wisdom scroll410/D0; eight other stat-scroll outcomes and separate consumers share sources. Tablet has no active declared supply/compiled producer; scroll410 is trash13/zero values with no bound stat-use power; READ delegates LOOK. Builder chooses supply and intended scroll terms/effect with an accounted issuance/use contract. Preserve current native recipe while pending. |
| ZSQ-CONNECTOR-FISHING-COOKING | Q241 consumes crab330+shrimp332→2×bisque53667/D0; same-name53668 differs. Full fish loop/pole/water/skill/events and grant_tradeskill_item reviewed; accounting issuance uses crafting source without species/catch-attempt journal lineage. Supplied fish fit acceptance. Add source-attempt/actor/species receipts before first-catch milestones, and qualified consumption before meal effects; no credit from prose or output possession. |
| ZSQ-CONNECTOR-TRAINING-DISPATCH | Chauseis53658→EPIC_WISDOM has a live reward row. Normal full boot epic_initialization and epic_points dynamically bind table teachers even without ACT_TEACHER/literal assignments. Handler checks level/caps/classes/prerequisites, scales epics/coin tuition and refuses active purchases. Qualify explicit durable teaching before credit/enabling; ambient prayer and scroll receipt remain separate. Review dynamic initialization, not just static assignment scans. |
| ZSQ-CONNECTOR-WORLD-MISSIONS | Feliusius53670 is world_quest. Full generation/policy/ASK/kill/share/reward/log/recovery source reviewed; target zone536 explicitly denied. ASK dispatch matches assigned NPC/type without keyword equality; kill dispatch includes eligible group members present. PID+quest_started reward source and committed ownership callback do not register one of eight static Q receipts. Add an explicit instance/version/objective/group/accepted-outcome bridge, preserving current native eligibility and SQL/flat-file history. |
| ZSQ-CONNECTOR-WORLD-REFUND | Confirmed stale/create-failure payment callback invokes world_quest_refund_payment→ADD_MONEY, which refuses active credits after a committed debit. Propose durable compensation tied to original debit/actor/giver/operation/quest_started identity; no promised refund until committed. Test unavailable target, stale or replaced quest, map/abandon timing, disconnect, refusal, replay and cold recovery. Separate named fix/news commit if selected; not repaired by this map. |
| ZSQ-CONNECTOR-CREW-SETTLEMENT | Complete crew_shop_proc54240:both eligible HIRE branches ignore SUB_MONEY return then change crew/chief and queue ship save. SUB_MONEY returns-1 while active; legacy precheck can pass. Propose durable debit with exact actor/owned ship/crew-or-chief attempt and publish only after accepted continuation. Preserve side/frags/skill/duplicate/native terms; qualify refusal/replay/disconnect/save recovery. No journal credit attached; selected repair separate named fix/news commit. |
| ZSQ-CONNECTOR-LEGACY-BOUNDARIES | All171 boundary edges/full101 foreign room records reviewed;32 targets218196–282265 lack active registry prototypes. setup_dir/renum_world resolves exact VNUMs and removes missing exits, with no legacy remap. Record incomplete-looking source paths fairly; builder verifies intended geography before relocation/removal/prose changes. No guessed destination or new travel route ships. |


Training evidence clarification: normal full boot dynamically binds epic teacher table rows. The preceding Oasis training proposal remains unavailable because ENCHANT has no live reward row; the updated dossier corrects the earlier dispatch reasoning. Historical descriptions remain archived verbatim.


## The Battlefield: returning an item is not reviving a pet

The [journal](../../areas/story/battlefi.story.json) and [dossier](../design/zone-stories/THE_BATTLEFIELD.md) distinguish a dead FOOD item return, two-root armor service, four-object spirit offering and independent trophy exchanges. Use actual follower runtime custody and TOUCH versus ENTER rules. Normal startup can bind custom services beyond literal assignment scans. Recipe-specific dynamic outputs need explicit committed evidence before service milestones.

| Follow-up | Evidence, capability and qualification |
| --- | --- |
| ZSQ-BATTLEFIELD-ACCOUNTED-STOCK | All 260 resets and complete parent/location families retain exact caps and chances. Rat, four offerings, scalp and trophies are scarce; skins share cap two. Active O/P/G/E admission refuses without a durable reset generation. Qualify accepted issuance, actor ownership, stock recovery and repeat availability; no guaranteed daily copies. |
| ZSQ-BATTLEFIELD-PET-RETURN | Q7 accepts exact FOOD 66400 for quilt 66462; not NPC rescue, corpse revival or escort. Eating spends the offering. Current supplied custody fits. A personal first-recovery variant needs actor/source/generation/root evidence distinguishing a gift from source acquisition, plus committed consumption and refusal/replay recovery. Keep the current accepted-return meaning. |
| ZSQ-BATTLEFIELD-BUNDLE-AND-TROPHIES | Q22 needs two distinct skins; Q67 needs four distinct types and retires the spirit. Q114/119/124 are independent D0 Justunian requests, unlike a retiring choice. Existing quantity and separate terminal receipts fit. Add builder-declared optional campaign goals only with explicit ALL/ANY and personal-versus-supplied policy; no invented kills or mandatory three-trophy chain. |
| ZSQ-BATTLEFIELD-SHARED-ACCESS | Cross P66416 is inside O66413 at66456, flags13/key66414; cleric zombie at66457 carries the key. PICKPROOF16 is absent, so native PICK/KNOCK attempts remain possible. Gate key66408 belongs to the last F sentry66403, not preceding M captain66471, and breaks100; key66414 breaks0. Qualify current reset/custody/unlock/open/get and permitted shared alternatives without requiring personal key history. |
| ZSQ-BATTLEFIELD-TRAVEL-AND-CLUES | Eight fixed portals distinguish TOUCH320 from ENTER7 and unlimited -1 charges. Local spoken-word prose has no key-2 lock; source contains no corresponding magic-word controller. Qualify actual destination arrival, arena/entry rules and shared generation before any travel milestone. Builder review can align clues with intended routes; no guessed password, exit or new travel mechanic. |
| ZSQ-BATTLEFIELD-FORGE-BOUNDS | Normal boot dynamically binds smith to mobile66410. Its eight options are127/128/129/130/88/89/90/91, but argument-bearing choice validation uses smith_array index10. Choice9 passes the check and selects -1. Active accounting refuses before this path. Proposed isolated native fix: compute bounded menu length, validate numeric choice and positive recipe index before any lookup; test empty,1,8,9,10,0,negative/large/non-numeric choices and all smith menus. No observed gameplay exploit or fix claimed. |
| ZSQ-BATTLEFIELD-FORGE-CUSTODY | Actual special dispatcher calls smith(NPC,player,...), yet smith scans NPC ch->carrying for ore; failure paths return selected NPC ore to player. Mobile66410 resets with leggings and no ore. Proposed isolated native fix must select explicitly actor-owned distinct materials, preserve ownership on every failure and consume only after committed reward grant. Keep active refusal until a qualified durable service adapter exists. |
| ZSQ-BATTLEFIELD-FORGE-RECEIPTS | Eight recipes consume exact legacy ore plus count-based fees and construct dynamically described armor1255. Generic item1255 possession cannot identify recipe. Add stable recipe/version/actor/giver/attempt, exact material roots, durable debit/reward/consumption and compensation outcome before service journal credit; cover refusal, disconnect, replay, duplicate ore and cold recovery. Existing SUB_MONEY return is checked and grant precedes consumption in the legacy path; retain those protections. |
| ZSQ-BATTLEFIELD-ORE-IDENTITY | Forge constants use legacy194/220/221/223/224/226/231/232, while current random_ore returns400260–400283. Cosmic/Thri limited stock and Vulm's foreign amethyst exchange supply some exact legacy gold/silver/platinum; not all recipe materials have declared reset supply. Builder must choose intended legacy/material-family policy and adapter, preserving foreign receipt ownership. Do not silently substitute modern ore or claim a personal mining receipt. |
| ZSQ-BATTLEFIELD-INN-BINDING | Literal room66355→undead_inn lacks an active room prototype. real_room0 returns0 on missing lookup, so the assignment writes world[0].funct; later assignments determine final boot state. Proposed separate fix should validate the target and resolve intended obsolete/relocated inn before binding. Handler is race-limited RENT with terminal-save rollback, not quest acceptance. No played impact or guessed inn destination claimed. |
| ZSQ-BATTLEFIELD-REWARD-PROSE | Righteous66419 is a holy longsword with righteous_blade, but its ground text/E keywords describe a skull-handled flail. Forge90/91 keywords and visible metal names also disagree. Builder should choose and align intended prose in a separate named content fix. Preserve weapon effects/balance: virtue conditional precedes a 3/4 life-bolt versus 1/4 alternate branch; an old chance comment alone is not a balance defect. |
| ZSQ-BATTLEFIELD-RENEWAL-AND-HISTORY | Six stories/six potential dailies under reset mode1 require fresh qualified materials and eligible giver returns; D1 spirit is limited per lifespan. Starter kit holy medallions and generic forged rewards are not accepted quest receipts. Qualify native acceptance/reward/retirement, current generation, replay and cold persistence separately from synthetic projection journeys. Discovery, contact visibility and ALL materials do not require ASK keyword history. |

## The Forgotten Mansion: clues explain a route without becoming hidden gates

The [journal](../../areas/story/mansion.story.json) and [dossier](../design/zone-stories/THE_FORGOTTEN_MANSION.md) distinguish learning a family name,SAY unlocking a shared library door,opening it,and accepted material requests. Model intermediate world changes explicitly with personal/shared scope and reset policy. A foreign treasure entrance prevents inferring every local key/kill step from arrival. Preserve three independent endpoints and foreign receipt ownership.

| Follow-up | Evidence, capability and qualification |
| --- | --- |
| ZSQ-MANSION-ACCOUNTED-STOCK | All247 resets retain exact source parents, caps and chances. Paper/rattle/head/horn and access keys have cap-one declarations. Active O/P/G/E reset admission refuses without a durable generation. Qualify committed issuance, recovery, actor ownership and renewed stock before promising a played daily; source declarations are not supply receipts. |
| ZSQ-MANSION-SPOKEN-GATE | Library3515 east↔3540 west has key-2, final keyword englehardt and D state6. SAY matches the final keyword, clears lock/secret on both matching sides and leaves opening to the door command. Current schema cannot record clue-learning or spoken-unlock outcomes. A builder-selected milestone needs actor/attempt/exit-pair/generation/pre-post state and committed success, including failed words, already-open shared access and replay. Personal learning/SAY history is not a native Q prerequisite. |
| ZSQ-MANSION-PROOF-ORIGIN | Q126 checks exact head3529+horn3539 together; both are type8 objects declared by G resets, not bespoke death-born body parts. Current supplied custody fits. An optional personal-defeat/recovery variant needs NPC generation, actor/party outcome, exact root lineage and source-versus-player transfer policy. Keep the accepted-bundle meaning until that evidence exists. |
| ZSQ-MANSION-NURSERY-AND-CURSE | Q70 response quiets a child in prose, but3576 room/E still describes crying. Lord dialogue describes undoing a curse; Q126 grants key/XP without changing all ghosts/rooms. Plan a builder-declared per-actor or shared-world resolution with generation, affected entities, committed transition and reset policy before rescue/quiet/curse achievements. Clarify text or integrate the intended state separately; no universal ghost removal or live child is inferred. |
| ZSQ-MANSION-VAULT-ACCESS | First vault key3540 fits3664S↔3671N; red3539@3673 supplies3548 and gold3538@3674 supplies3549 for later gates. WH55634 NORTHWEST(D6) reaches3675 outside this chain; no native local portal exists. Route milestones need actual arrivals/exit outcomes and alternative-access policy. Do not require local boss/key history merely because a player reached treasure; retain the existing foreign route until builder intent is reviewed. |
| ZSQ-MANSION-FOREIGN-MEMORY | Memory55415 is G stock on dragon3541@3675, then exact WH55135 Q2480 offering→scroll55362+coin1000000+token55033. The foreign ambassador has cap-two placements55005/55400. Preserve Winterhaven receipt ownership; a future cross-zone lead can link the native definition and disclose supply versus accepted return without duplicate local achievements. Stone359 uses its independent eligible-group committed touch process, not any of these Q identities. |
| ZSQ-MANSION-REWARD-EFFECTS | Vapor67203 has an actual shield/curse/autoequip handler; those effects do not prove local acceptance or house resolution. Foreign reward scroll55362 has generic RECITE matching and retire-before-unchecked-random-publication, already documented by Winterhaven. Select any repair separately with exact input UID/actor/attempt, frozen output, durable retirement/grant and replay/refusal coverage. No observed exploit or effect repair claimed here. |
| ZSQ-MANSION-CLUES-AND-PROSE | Gate3500 prose says wilderness south, while actual exit is WEST to Surface588620. Proposed isolated content fix should align that direction after builder review. Musical paper names Rise of the Phoenix; diary reveals experiments; gate/front plaque names Englehardt. These are readable clues, not completion receipts. Handmaid's bracelet language matches actual circlet3513 wristwear; no equipment-slot defect is claimed. Tyrlos's old highdrop list entry has a commented-out bonus loop, so it is not an active extra-drop promise. |
| ZSQ-MANSION-OPTIONAL-CAMPAIGN | Three D0 native requests remain independent. Builders may later define a family-history campaign with explicit ALL/ANY, optional clue/access/stone/foreign-return branches, spoiler policy and personal/shared scope. Existing exact accepted contracts support each terminal card; custody or narrative chronology cannot supply missing intermediate world-state events. Do not silently require pianist or handmaid completion before Q126. |
| ZSQ-MANSION-RENEWAL-AND-HISTORY | Resetmode1 and D0 make three potential repeatable story endpoints. Fresh cap-one supplies and eligible giver availability still require qualification. Preserve exact identity, read-only current material rows, discovery/contact visibility, independent terminal history and frozen recovery. New discovery/encounter/journal/achievement/daily credit requires active,ready accounting; cold receipts do not manufacture present items or new world-state outcomes. |

## Woodseer: independent returns with precise source guidance

The [journal](../../areas/story/woodseer.story.json) and [dossier](../design/zone-stories/WOODSEER.md) show declared source placements without requiring personal hunts. Honey suggests wax without enforcing order. Supplied exact materials fit; consumable preparation and accepted history differ. Shops,pets and generated world quests retain their own event/receipt ownership.

| Follow-up | Source, required capability and qualification |
| --- | --- |
| ZSQ-WOODSEER-ACCOUNTED-STOCK | All1186 resets preserve exact parents/caps/chances; selected sources have chance100 but global counts and durable issuance admission still apply. Mode2 resets at lifespan without requiring an empty zone. Bard and woodworker D1 retire after acceptance. Qualify committed giver/item renewal and cold recovery before promising played dailies; a reset declaration is not personal acquisition evidence. |
| ZSQ-WOODSEER-SOURCE-OR-TRANSFER | Honey16545 is G on warrior16581; wax16546 on queen16582; egg16547 is O@16586; feather16544 on tiny hummingbird16572; pigment16561 on dragonfly16615; meat16543 on only two snapping-turtle16571 placements; mandolin16526 is O@16587. Native acceptance checks exact supplied custody. A personal-source variant needs root UID, reset generation, source NPC/container/room, actor/party event and transfer policy; do not infer kills, skinning, egg birth or extraction from item prose. |
| ZSQ-WOODSEER-HIVE-CAMPAIGN | Honey response suggests wax; all7Q have no native prerequisite. Map the three hive returns as independent cards. Optional ALL/ANY campaigns need explicit terminal identities and builder-selected rewards/reset/visibility policy. Queen defeat, honey-first and a cleared hive cannot be inferred from one accepted item. |
| ZSQ-WOODSEER-CLUES-AND-REPAIR | Twenty-two addressed aliases explain the seven requests; ambient qc_action60 is not a player achievement. Mandolin16526 is TRASH13; playable shop16649 is instrument32. Native return says the bard intends to repair it without a persisted repair/play event. Add an explicit committed effect/actor/item lineage and repeat policy if builders select that milestone. |
| ZSQ-WOODSEER-CONSUMPTION | Honey16545/meat16543/soup16610/salmon16635 are FOOD19; eat retires an item. Potion16579 has its normal quaff/spell/retirement behavior. Bought soup or consumed reward does not prove its quest receipt. A use milestone needs exact input UID, committed retirement, effect outcome, failure and replay handling; current journal only records accepted return and current loose supplies. |
| ZSQ-WOODSEER-WORLD-QUEST-SERVICE | Bartender16553@16633 has five same-function assignments, captured as the shop secondary special. world_quest uses its own generated target/type/count/start/giver state, fee settlement and reward continuation. Do not create five identities or an eighth local Q story. Cross-system presentation needs explicit source ownership, actor/start identity and reward settlement; active mercenary coin/item combinations retain existing refusal. |
| ZSQ-WOODSEER-PETS-AND-INN | pet_shops16886 uses next loaded room as stock; pony16700 also follows a visiting dwarf in surfacekeeps. Pet buy/rent/ticket redemption already refuse under active accounting; inn16558 requires ordinary eligibility and terminal persistence. Optional service milestones need durable price/ownership/claim-ticket/actor outcomes before achievement integration. No pet or inn transaction is a native local request. |
| ZSQ-WOODSEER-STALE-BINDINGS | artifact_invisible binds object16904, but it is a bleached corpse container15 with no wear slot while the handler requires OBJ_WORN. guild_guard16501 only handles matching current/birth room16501; all17 maintained local guard births are elsewhere, so that case is dormant. Its expected insignia9316 is a ruby weapon in tower.obj. Builder audit must select intended prototype/placement/handler; do not claim an active city gate defect or enable equipment/access from guessed intent. |
| ZSQ-WOODSEER-CONTENT-INTENT | Woodworker Q81 grants only ring16576 although its response mentions coins; decide whether to correct prose or change intended rewards in a separate repair. DeLoran boots16600 have a return/reward E hint but no matching maintained local Q or literal special binding; determine whether this is lore or an unfinished request. No extra reward, giver or achievement is invented. |
| ZSQ-WOODSEER-ROUTE-AND-STOCK | Approach16500 UP points to154581, absent from registered AREA source but present as ocean in unregistered Duris3.wld. Native renum_world removes unresolved exits; other maintained routes still enter town. Luc16509 shop/reset stock references6070/6109/6110 with no object prototypes; real_object returns-1 and disables those G resets, while shop producing entries remain-1 and cannot match an item. Builder review must reconcile intended approach and obsolete/restorable stock with deployed generated data before a separately named repair. |
| ZSQ-WOODSEER-FOREIGN-RECEIPTS | Fish merchant16538 sells salmon16635. Surface minizones97909 QA302 needs that steak AND firebreather3003→XP15000+coin16000/D1; existing foreign journal owns the bundle. Imported/foreign stock includes mailman16695, Brek's two daggers16599 and pony16700, not new local requests. Cross-zone leads need explicit target/receipt ownership and availability; native keys and similarly numbered mobile/item IDs remain distinct. |
| ZSQ-WOODSEER-PLAYED-QUALIFICATION | Source/schema and compiled receipts qualify guidance, seven independent endpoints, exact current materials and history projection. Normal-play arrival/ASK/access/issuance/turn-in/reward/accounting/persistence journeys remain pending. New discovery/encounter/journal/achievement/daily progress requires active,ready accounting; frozen historical recovery remains separate. |

## Village of Refugees: explicit fees and real rescue outcomes

The [journal](../../areas/story/ruins.story.json) and [dossier](../design/zone-stories/VILLAGE_OF_REFUGEES.md) distinguish a supported eyestalk return from a blocked paid craft. Four different feathers prepare materials only. A cage key,dialogue chain or eyestalk return cannot substitute for prisoner release and safe return. Keep foreign collar settlement in Newhaven.

| Follow-up | Evidence, capability and qualification |
| --- | --- |
| ZSQ-RUINS-MIXED-FEE-SETTLEMENT | Anguinel Q16 requires four exact item roots AND C1000. Active quester refuses numeric/legacy offerings; submit_durable_quest_offering rejects non-ITEM give goals. Keep the paid craft a currently unavailable service. Design atomic item+coin admission, frozen fee/payer/NPC/attempt/generation, failure preservation, exact output and committed receipt, replay/restart recovery. Do not relax the guard. |
| ZSQ-RUINS-WALLET-READINESS | Current schema3 exposes completion/carried_item/equipped_item, not a settled-wallet row. The four feather checks only show loose custody. Add a separately selected read-only settled spendable balance/fee status with busy/unsupported states, active-ready authority and epoch/revision validation; no implicit debit, synthetic receipt or payment promise. |
| ZSQ-RUINS-PRICING-AND-BIRD-INTENT | Anguinel M asks10 platinum, Q16 lists C1000=1 platinum under coin_stringv. M names Snowy Owl while actual mobile98603 is screech owl. Raven feather98601 is P inside broken gate98600@98604, not G on raven98604. Ask builder which fee/labels/source are intended; any selected prose/price/source repair is a separate named fix/news commit. |
| ZSQ-RUINS-SOURCE-OR-TRANSFER | Five exact offerings use gate P, three bird G and beholder G. Native supported return accepts supplied exact custody; it does not prove actor-specific hunts, extraction or bird collection. Personal-source milestones need root UID/reset generation/source parent/room/container, actor/party and transfer policy, confirmed publication and recovery. Raven feather11 is wearable; eyestalk12 can be held; loose checks do not accept worn/held/nested proxies. |
| ZSQ-RUINS-ACCESS-ACTIONS | Table98607/98667 ITEM_SWITCH29 values340/CMD_PULL open blocked98607W↔98610E. Well98631 ITEM_TELEPORT25 uses98629/CMD_ENTER7/-1 unlimited charges. Generic specials/teleport execute before the ordinary command. Track successful actor/root/generation/exit pre-post/actual arrival events before adding access milestones; failed/already-open actions and shared prior opening do not prove personal progress. Preserve native hazards/door rules. |
| ZSQ-RUINS-WEDDING-CLUES | Moaning98611@98614 carries key98610 for small chest98612@98615 with veil98611/note98614. Mourning98616@98628 has M moan/tragedy and points to cedar98626 with cummerbund98625/letter98627. Add builder-selected learned/examined/read outcome identities and spoiler/repeat policy if a grief/wedding story is wanted; ASK encounter, possession and lore are not acceptance or reconciliation. |
| ZSQ-RUINS-CAPTIVE-RESCUE | Q113 accepts eyestalk98642, not proof of release. Beholder98637@98643 declares key98635 and eyestalk; three cage containers hold clothing, while prisoner98638 is a separate NPC with shackles. Design actual release, prisoner episode/reset identity, ownership/party/escort, survival/safe destination and committed rescue outcomes. Do not infer permanent village restoration or all-monster defeat from the farmer receipt. |
| ZSQ-RUINS-STOCK-AND-FOLLOWERS | All202 reset commands/caps/chances and full115 parent/location families retained. Mode1 waits for eligible empty-zone reset; global caps and active issuance admission matter. Farmer D1; Anguinel D0. F uses last root M as follow target but replaces current mob, so subsequent E belongs to the latest successful follower (98658 after F98641), not automatically the root adolescent. Qualify actual source/giver renewal and lineage; no daily-created stock. |
| ZSQ-RUINS-BREAD-TYPE-INTENT | Shop98615/room98619 stocks bread98620 as type24 ITEM_CORPSE; defines.h marks that type internal and not for builder assignment, while other local food is19. Confirm intended food/decay/corpse behavior and assess a separate native-data fix with eat/ownership/persistence/shop qualification. Source mismatch is verified; no played failure or repair is claimed. |
| ZSQ-RUINS-FOREIGN-COLLAR | Collar98606 E on dog98607@98610 is input to Newhaven35286 Q331 with Mixt heart13221 and C100000→collar35224/D0. Existing Newhaven service owns the accepted bundle and remains payment-blocked. Mixt source13207@13228/G13221, Vulgaris35286@35201/sign and full foreign actors/stock/boundaries reviewed. Synchronize the older Newhaven collar hint with the two maintained table switches after the existing native repair; preserve all receipt IDs and classify this as guidance only. Plan linked cross-zone leads with receipt ownership, availability and source/gift policy; no local duplicate. |
| ZSQ-RUINS-QUALIFICATION | Source/schema/compiled synthetic journeys qualify exact identities, service classification, visibility, five material rows, supplied/equipped/reward-only custody, read-only rendering, independent receipts, replay/cold and raw-to-authored history. Played active source/reset/access/fee/reward/retirement/rescue/effect/persistence qualification is pending. New discovery/encounter/journal/achievement/daily credit requires active, ready accounting; frozen recovery separate. |

## Twin Towers: ANY alternatives versus simultaneous bundles

The [journal](../../areas/story/ttowers.story.json) and [dossier](../design/zone-stories/TWIN_TOWERS.md) distinguish five requests/seven native Q. Group equivalent priest alternatives; keep distinct recipients and Lyena/Talfyn simultaneous bundles independent. A holy symbol cannot prove resurrection, and a PUSH attempt cannot prove successful access.

| Follow-up | Evidence, capability and qualification |
| --- | --- |
| ZSQ-TTOWERS-COMPETING-HEARTS | Five authored requests cover seven native Q: separate Blaevyna/Mixt recipients, Lyena's two-root bundle, priest's three equivalent alternatives as one ANY entry, and Talfyn's three-root bundle. Preserve every native binding/history; do not infer branch exclusivity, a required dialogue sequence or mandatory personal hunting. Future builder-selected allegiance needs explicit selection, competing-root consumption and replay/reset policy. |
| ZSQ-TTOWERS-SOURCE-OR-TRANSFER | Hearts13221/22/23 are predeclared Gcap1/chance100 on Mixt13207@13228, Blaevyna13206@13229 and Lyena13213@13258. They share heart keyword and TAKE+HOLD type13. Supplied exact loose roots are valid. Personal-source milestones need UID/reset generation/actor or party/source parent and published acquisition, with explicit transfer/loot policy; death alone or held/nested custody is insufficient. |
| ZSQ-TTOWERS-REVIVAL-OUTCOME | Priest M narrates dispatch to a mentor in Tharnadia, but Q80/85/90 only grant symbol13224. Keep accepted return distinct from send, resurrect, living subject, delivery or permanent resolution. A larger episode needs chosen target and actual durable dispatch/revival/arrival states, accounting ownership and failure/recovery semantics, before added achievements. |
| ZSQ-TTOWERS-SUCCESSFUL-ACCESS | Seven ITEM_SWITCH objects use PUSH270 or PULL340 to clear reciprocal BLOCKED. Closed/locked/search/movement remain distinct. Add actual successful pre/post access events with actor,object UID,room/edge/world generation,shared-state and replay rules; an attempted or already-open control is not a personal unlock. Key13203 comes from Mixt; altar13228 is scenery over a hidden descent, not a switch. |
| ZSQ-TTOWERS-RUBBLE-RESET | Physical13231S begins blocked/closed9; reset D1 sets CLOSED and clears LOCKED without clearing existing BLOCKED. First world load needs PUSH rubble13204; after successful unblocking, D1 closes without reblocking until restart. Other D8 controls explicitly restore BLOCKED. Qualify initial/reset/restart and reverse-side outcomes; ask builder whether this lifetime is intended before any separate reset repair. Do not diagnose D1 as unblocked from the number alone. |
| ZSQ-TTOWERS-LEVER-NARRATIVE | Guard13214 and room13255 describe a broken lever, while lever13208 O@13248 targets13248S. Full automatic switch binding exists. Preserve current access; compare played control/door behavior and builder narrative intent before choosing a separate prose or mechanics fix. Conflicting accounts of living/dead lovers are viewpoints until confirmed otherwise. |
| ZSQ-TTOWERS-CLUES-AND-HAZARDS | All9 addressed M blocks/17 aliases support context, not nine awards. Learned-clue milestones need actual reply delivery/actor ownership and builder-selected meaning. Trapchest13206@13244 contains collar13213, F30 room13239 can drop the player, and temple/tunnels have hidden doors. Carrying collar/key or surviving elsewhere does not record safe retrieval, trap disarm or traversal. |
| ZSQ-TTOWERS-DUERGAR-OUTCOME | Captured miner13220 and councilor/mercenary/tunneling accounts suggest rescue or negotiation, but no local Q dispatches release, escort or safe arrival. Add a selected target/state/escort/survival/arrival outcome with actor/party,reset generation and recovery before calling an achievement a rescue. |
| ZSQ-TTOWERS-FOREIGN-OWNERSHIP | Newhaven35286Q331 consumes Mixt heart+collar98606+C100000; WH55101Q916 consumes Lyena heart+earstud9375+ring82405; Alatorin83337QA5526 consumes Mixt+Lyena hearts. Foreign recipients own exact receipts. Preserve conflicting consumption and supplied routes. WH companions originate in tower9321QA15 (three Labyrinth items) and sun82407Q48 (demon's rock82406); no synthetic local crafting or duplicate completion. |
| ZSQ-TTOWERS-JIN-TOUCH-SETTLEMENT | Oldmonk13219M@13277 is a computed epic_teacher binding, not an unbound mobile or an eighth Q. Normal boot epic_initialization binds teachers; mini skips optional subsystems. Jin Touch row100/75/750000, CLASS_MONK,max100,property/class/level/current-skill/cost/save rules apply. Active accounting refuses epic purchases. A future lesson needs owned quote,epic+coin debits,committed skill+save outcome,epoch/replay/recovery; optional context only today. |
| ZSQ-TTOWERS-AVAILABILITY-AND-QUALIFICATION | Talfyn13229 cap1@13231 has chance20 and disappears after Q112. Others remain D0; hearts have cap1 and foreign competition. Qualify actual active publication,stock admission,successful item-only offering,coin/XP/item rewards,departure,replay/cold persistence and reset/restart/access paths. Daily selection guarantees neither stock nor a present giver; synthetic journal receipts qualify projection only. |

## Deep Ravine of Passage: staged access is not an inferred dependency

The [journal](../../areas/story/minopass.story.json) and [dossier](../design/zone-stories/DEEP_RAVINE_OF_PASSAGE.md) distinguish accepted exchanges,optional supplies and actual movement/outcomes. Declare builder-selected prerequisites explicitly. Timed NPC messages do not award rescue/healing; paid service has no wallet-readiness row.

| Follow-up | Evidence, capability and qualification |
| --- | --- |
| ZSQ-MINOPASS-CAMPAIGN | Oesh's spore/key, contractor ring/key/potions and dwarf's paired relics support a larger traversal story. Native hand-ins do not enforce prior personal discovery, a conversation sequence or earlier receipt. Future campaign gates need selected episode dependencies, alternate or supplied-key routes, accounting-owned transitions and replay/reset policy. |
| ZSQ-MINOPASS-SOURCE-OR-TRANSFER | Spore94723 is hidden Ocap26:13 declarations at each94855/94862; signet94715 hidden Ocap1@94843. Relics88807/4402 are foreign Gcap1 on Kitan88816@88860/Zorta4401@4610. Exact loose supplied roots fit. Personal acquisition needs UID/source parent/reset generation/actor or party and committed custody, with explicit transfer/loot policy. |
| ZSQ-MINOPASS-SUCCESSFUL-ACCESS | Five automatic switches include stationary lever94709@94790 targeting94780E. Remote stationary targets are supported; secret targets clear only the addressed side, while non-secret targets clear reverse BLOCKED too. Track actual before/after edge state, actor, controller UID, target, generation and successful traversal; attempts or key possession are insufficient. |
| ZSQ-MINOPASS-PAID-KEY | Nahasp94757 Q174 C10000→I94729 is ten platinum, matching dialogue, but active accounting refuses non-ITEM offerings. Preserve raw history while service contributes no achievement/daily. Future service requires atomic wallet debit/key issuance/acceptance and rejected/replayed/recovered outcomes under active,ready authority; no PvP-kill prerequisite or invented fee repair. |
| ZSQ-MINOPASS-RESCUE | Borthur94707@94846 congratulates visitors via qc_action20. Shared timed action only broadcasts prose. Ring acceptance retires contractor94705 but implements no Borthur release/escort/safe arrival. Builders should select what rescue means and its actual states before a rescue achievement. |
| ZSQ-MINOPASS-SPIRIT-OUTCOME | QA130 narrates coalescence and then spectral departure, granting symbol94726+XP250000/D1. This may intentionally mean release, rather than lasting resurrection. Preserve that reading; future release/revival requires selected subject, actual durable transition and failure/recovery evidence, not symbol possession or timed healing text. |
| ZSQ-MINOPASS-HEALING | Rooms94872/94877 carry ROOM_HEAL bit131072, and shared recovery uses CHAR_IN_HEAL_ROOM. Spirit/holyman timed messages independently broadcast. Any healing episode needs actual recovered amount/subject/window and relevant conditions; hearing text or entering a shrine cannot substitute for recovery. |
| ZSQ-MINOPASS-TRAVEL-LORE | All8 ITEM_TELEPORT25 objects are reset-backed paired routes with ENTER7/unlimited charges. Blazing94719@94846→94847 and94743@94847→94846 qualify a declared return despite no-turning-back prose. No alignment/virtue predicate is established by the generic portal. Qualify played traversal and ask builders whether warnings are rhetorical before any separate prose/mechanic repair. |
| ZSQ-MINOPASS-FOREIGN-OWNERSHIP | Zorta Q18 unholy88807→S113/D1, Kitan Q19 holy4402→C100000/D1 and Alatorin Berronar Q7425 shard83626→replacement83626+symbol94726+400234/D1 retain their own recipient-zone receipts. Underworld/Menden lack authored journals at this checkpoint. Do not duplicate their acceptance locally or infer spirit completion from alternate symbol stock/chaos equipment. |
| ZSQ-MINOPASS-STOCK-AND-QUALIFICATION | Mode1, caps, departing recipients and current accounting issuance govern availability. Dispersal94876 is a reset staging room, not a player starting point. Qualify normal played source/search/loot/access/hand-in/reward/effect and persistence with active,ready accounting, failure/replay/restart before advertising complete gameplay coverage. |

## Neverwind Valley: same-kind counts are different from distinct story sources

The [journal](../../areas/story/pyramid.story.json) and [dossier](../design/zone-stories/NEVERWIND_VALLEY.md) use count3 for one rod kind and four rows for four different eggs. Explain native access and readable clues;declare any personal provenance,campaign or reunion prerequisite explicitly instead of inferring it from prose.

| Follow-up | Evidence, capability and qualification |
| --- | --- |
| ZSQ-PYRAMID-CAMPAIGN | Egg/ring, letter/spike and rod/staff returns are independent. Dialogue suggests preparation for the pyramid, without enforced egg receipt, worn ring, prior conversation or password use as an offering prerequisite. Future episodes need builder-selected dependencies, alternate/supplied routes and accounting-owned transitions with replay/reset rules. |
| ZSQ-PYRAMID-THREE-TOMBS | Q30 consumes three I20400, all the same prototype; Pcap3 declarations follow sarcophagus20401 Ocap3@20477/20587/20590. Count3 is accurate today. One-piece-from-each-king requires source-parent UID/room/generation and selected policy for transfers, parties and previously acquired pieces; do not silently reject valid supplied roots. |
| ZSQ-PYRAMID-CONTAINER-PARENT | Four O20402 nests@20498–20501 each precede a different P egg; three O20401 sarcophagi precede same-kind wood roots; chest20420@20489 precedes letter20419. Shared P calls global get_obj_num, not a retained O-instance parent. Fresh intended placement is plausible, but partial resets/caps/pre-existing duplicate parents need played qualification. If a misplaced-root defect is reproduced, select a scoped parent-resolution repair in its own fix/news commit before promising provenance. |
| ZSQ-PYRAMID-SOURCE-OR-TRANSFER | Eggs20403/04/05/06 are a simultaneous four-kind AND; rod20400 requires count3; letter20419 is a single root. Supplied loose roots fit; held/worn/nested roots do not prepare material rows. A first personal recovery episode needs committed UID/source/custody and explicit gift/loot/party policies, independently of current readiness and accepted history. |
| ZSQ-PYRAMID-PASSWORD-ACCESS | 20491N→20478 flags3/key-2,keyword double dunan,resetD2; reverseS flags1/key0,resetD1. Successful SAY uses last keyword and clears LOCKED/SECRET on the addressed and matching reverse edge; it does not OPEN or traverse. Future proof requires successful actor/edge/before-after/generation events and actual movement, including prior/shared unlocks and resets. |
| ZSQ-PYRAMID-MAZE-AND-CLUES | Papers at20437 say N,N,E,S,E,N. Full fixed555-edge graph includes self loops,one-way floor links and secret doors. Trial starts20561/20580 reach20573/20576; this does not choose the intended start or validate a universal route. Builders should verify the intended clue/start under normal traversal before changing prose or adding a navigation sequence achievement. |
| ZSQ-PYRAMID-NPC-AVAILABILITY | Goar20409 Mcap1/chance100@20596 disperses via20598 toward20469,or into no-exit20597 jail. Elven adventurer20411@20592 uses three staging steps toward20591,or no-exit20594. Room prose25%/12.5% is not reset chance. Current mundane AI samples directions and live conditions. Qualify normal dispersal/availability; select probability or staging repairs only from reproduced behavior and builder intent. |
| ZSQ-PYRAMID-FAMILY-OUTCOME | Krodn20410 offers dialogue,not a Q. GoarQ47 letter→spike/D1 says he leaves to find his father; NPC extraction does not record travel,escort or reunion. Future episode needs actual subject/destination/outcome and failure/recovery policy; readable Eletter proves authored explanation,not delivery or reunion. |
| ZSQ-PYRAMID-REWARD-AND-HEALING | Ring20417 has protection/+HP properties; staff20418 is type4,level40,one charge,SPELL_HEAL28;20591 has ROOM_HEAL131072,while20509 is ROOM_NO_HEAL262144. Actual wear/device authority/configuration and measured recovery are separate from receipt/possession. Track subject,committed effect/charge/recovered amount before adding protection or healing milestones. |
| ZSQ-PYRAMID-ARRIVAL-AND-QUALIFICATION | Githzerai level1 shift_prime can choose Maern's room20484 among14 destinations when leaving Astral under cooldown/non-combat rules; ordinary boundaries include incoming-only53725S→20466. Neither becomes a mandatory quest route. Mode2/caps/D1 departure and current accounting admission determine availability. Qualify played source/container/travel/hand-in/reward/effect/persistence with rejection,replay/restart; no daily-driven stock recreation. |

## Grumbar’s Domain: explain essence, count exact material and preserve hazards

The [journal](../../areas/story/earthp.story.json) and [dossier](../design/zone-stories/GRUMBARS_DOMAIN.md) distinguish ten exact shards from general granite and identify the lash behind essence prose. Context, current readiness, accepted history, actual access and liberation have separate evidence.

| Follow-up | Evidence, capability and qualification |
| --- | --- |
| ZSQ-EARTHP-CAMPAIGNS | The two independent QA receipts cover ten shards→key and lash→pick/experience. Build a selected all-stage campaign with distinct preparation, accepted history, access and world outcomes before adding a larger prison or pech-liberation completion. Supplied items and either story order remain valid. |
| ZSQ-EARTHP-COUNT-AND-PROVENANCE | Sunnis consumes ten copies of I400104, without checking ten different carriers or personal kills. Thulum consumes I131227, despite essence wording. Current loose custody is not first acquisition history. Add committed UID/source/actor/custody with explicit gifts, loot, party sharing and original-carrier policies only for builder-selected episodes. |
| ZSQ-EARTHP-ADMITTED-SUPPLY | Eighteen G400104/cap18 declarations on six each xorn/xaren/galber dher and E131227/cap1 on Vashur are source intent. Active accounting refuses reset item issuance before live placement; it also gates mining, salvage and refining. Implement generation-bound admitted issuance and qualified production before promising renewed stock. Keep accounting required; use existing authoritative stock, without fabricating daily supply. |
| ZSQ-EARTHP-RESET-PARENTS | All543 resets include104 F followers. F changes the current equipment recipient while retaining the last M as master. Preserve actual current M/F instance, leader, cap, slot, chance and generation in reset provenance. Gear after F belongs to that follower, not automatically to the leader. Qualification needs partial loads, rejected item issuance, death, wandering and restart. |
| ZSQ-EARTHP-WALL-AND-TRAP | Living wall131213 G131210 exposes a non-takeable PUSH270 control targeting131362 DOWN→131366. Its T261/4/1/32767 is a single-charge, room-wide downward acid trap; trap level32767 means327d67 in the current damage formula. A triggered trap aborts that move. Preserve the hazard while builders qualify intended severity, disarm/survival and admitted ground custody. Any selected adjustment needs a separate fix/news commit; do not describe PUSH as guaranteed safe arrival. |
| ZSQ-EARTHP-KEY-AND-BARRIER | Key131211 matches131366E↔131364W; reset closes/locks the Sunnis-side edge but only closes the lair-side edge. Reward receipt, unlocking, opening, traversal and personal/shared prior access differ. Narrated recoil/reformation and six golems do not execute a selected barrier/spawn/liberation transition. Add successful actor/edge/generation and actual spawn/outcome evidence before those achievements. |
| ZSQ-EARTHP-MINES-AND-CREVICE | Six O434 gem nodes and seven G193 ore-node declarations are separate from exact shard400104. Gem-Seeker is a weapon named pick and qualifies when wielded, but accounting currently gates mining. Crysmal131223 G131223 is non-takeable ENTER7→131396, unlimited charges, with ordinary DOWN→131379 return. Both carriers use EE race/no-corpse ground spill. Track real ground controller, mine depletion, admitted output/source, skill/tool and actual travel before adding mine or crevice milestones. |
| ZSQ-EARTHP-NPC-AVAILABILITY | Sunnis131214@131366, Thulum131236 and Vashur131230@131376 all declare Mcap1/chance100. Thulum/Vashur can wander; both recipients depart after acceptance. Normal reset caller0 admits ordinary M only at chance100; rare Zarqath50, Kharzun25, Ungoro25 and crysmal/shardling50 rows are candidates at forced/initial reset, not guaranteed ordinary renewal. Qualify intended encounter cadence before selecting a scoped repair. Mode1 waits for an empty zone. |
| ZSQ-EARTHP-LIBERATION-AND-AVATARS | Pech/slave/colossus mining and Thulum's songs are timed broadcasts. His accepted lash does not change slave followers or establish a freed settlement. Entemoch holding room131398 actually loads23801 plus ten bedrock sentinels; keyword _spec1_ is class specialization. Select the intended actor, freed subjects, destination/state and failure/recovery rules before any avatar or liberation objective. |
| ZSQ-EARTHP-WORM-CORPSE-CUSTODY | Ungoro131232 binds purple_worm: hunting, lethal swallow, corpse possession, delayed release and death spill. It is a hazardous combat/corpse mechanic. Future recovery or defeat episodes need actual death, corpse UID/owner, credited actor, release/custody, expiry and restart policies; the husk or an ambient encounter alone cannot prove a rescue or safe transit. |
| ZSQ-EARTHP-SHARED-OWNERSHIP | Alatorin Miboli83140 QA583 needs paper5 plus two identical400104 for lithic arcanum83686; Dweefniggle83150 QA2114 accepts one400104 for collecting-token piece83245. Local shard availability is shared, but foreign receipts stay owned by those recipients. Preserve exact loaded paper and native identities; another granite quality or any unrefined gem is not an alternative for Sunnis. |
| ZSQ-EARTHP-EDITORIAL-AND-PLAYED-QUALIFICATION | Object131219/131220 extra-description keywords say emeralds for rubies/diamonds;131221 sapphire keywords/ground text say diamonds and its extra says emeralds. Thulum dialogue includes Thull Thullstone; Sunnis's departure uses actor/recipient pronouns inconsistently. These are bounded wording/targeting candidates, not chosen mechanic fixes. Builders should select consistent names and displayed speakers in separate fix/news commits. Qualify played accounting-on sources, ground controls/trap, hand-ins, rewards, wandering, rare resets and persistence before promoting those outcomes. |

## The Temple to Skrentherlog: explain the full lead without inventing a receipt

The [journal](../../areas/story/yuan_ti.story.json) and [dossier](../design/zone-stories/THE_TEMPLE_TO_SKRENTHERLOG.md) separate current blood readiness from acceptance and the custom hammer lead. Preserve pick-only chests, shared routes, hazards and foreign ownership; define actual outcomes and admitted stock before adding milestones.

| Follow-up | Evidence, capability and qualification |
| --- | --- |
| ZSQ-YUANTI-CAMPAIGNS | Keep the ranger's accepted blood return, captive stone clue, hammer action, epic touch and foreign memory return as distinct episodes. The native Q binds only blood80570→scimitar80572/D1. Select an all-stage story with explicit optional routes, participants and outcomes before joining these into a larger completion. |
| ZSQ-YUANTI-BLOOD-AND-CURE | Skrentherlog80538 G80570/cap1 is the declared blood source. Matching supplied loose blood is also accepted; corpse, vials80524/80525 and personal combat are not substitutes. Current custody does not prove first recovery. Add committed item UID/source/actor and gift/party policies for selected recovery milestones. The D prose narrates a cure and departure; extraction does not persist a separately cured or escaped subject. |
| ZSQ-YUANTI-ADMITTED-SUPPLY | Active accounting refuses reset O/P/E/G issuance before read_object or placement. Blood, keys, containers, hammer, stone, post-action loot and epic stone all need admitted existing stock or a qualified generation-bound issuance adapter. Neither discovery nor a daily clock creates supplies. NPC M/F loading and item issuance have separate admission. |
| ZSQ-YUANTI-CHESTS-AND-ACCESS | Shaman80516@80520 Gkey80551 opens pickproof iron chest80550@80536 containing key-amulet80561 for80542E↔80543W. Warrior80530@80538 declares80544 for landing side doors and80560 for prison. Boss80538 declares80581 for hidden80544E→80545. Drowcrusher80556 is in chest80549 flags13/key0: closed/locked but pickable, requiring actual skill and a held pick. Chest80548 has the same flags despite broken-lid prose. Preserve existing keys, pick access and prior/shared unlocks; do not fabricate a key, remove a lock or require every route for the receipt. |
| ZSQ-YUANTI-ATOMIC-HAMMER-USE | drowcrusher handles HIT with the hammer worn in WIELD, a live actor in80545 and a visible ground80563. It unequips then extracts both roots and relocates occupants. invoke_object_special adds only a pet-helper guard; extract_obj explicitly does not retire durable custody. A selected adapter needs two admitted UID graphs, actor/wearer and ground-generation checks, frozen retirement intent, rejection with no live effects, committed outcome, replay/recovery fences and post-commit effect publication. No current custom completion is manufactured from possession or command text. |
| ZSQ-YUANTI-PARTICIPANTS-AND-PVP | The active loop moves every current treasure-room occupant, including NPCs, bystanders and opponents. It ignores char_from_room veto and char_to_room failure results. No hang or PvP exploit is reproduced here. Select and qualify intended participant/consent/faction/combat rules before changing that scope. Use a finite frozen list, stable runtime identities and checked movement outcomes; test refused/vetoed removal, offline/dead actors, pet/rider links, intervening movement and restart. |
| ZSQ-YUANTI-EFFECT-AND-TRAVEL | HIT consumes the hammer/stone and publishes a narrated collapse, but does not explicitly kill Skrentherlog, free captives, replace the zone or record the selected outcome. A scoped committed event must identify the successful action, source/controller generation, consumed roots, subjects and destinations. Demolition, rescued subjects, actual escape and reward collection need their own evidence. Keep normal travel separate from post-action relocation. |
| ZSQ-YUANTI-STATIC-RUINS-GRAPH | The fixed rubble route80546UP→80547S→80548UP→80549S→80550S→80551S→80552S→80553S→80554 already exists;80554N→80500 returns to the intact approach, and80500S→80554 is already an ordinary edge. setup_dir masks raw state to3;80546UP raw4 has no D reset, so it is open under the current loader. Closing-behind and permanent-collapse prose has no matching transition here. Qualify builder intent before selecting any graph/state repair; do not infer a phase switch or a softlock from prose. |
| ZSQ-YUANTI-ZERO-MODE-RENEWAL | D1/reset0 makes the static blood contract story-only: one achievement, zero dailies. Epic touch can request age/empty/probability renewal, so mode0 does not prove never-renewing stock. Ranger80517 Mcap1/chance30 is an initial/forced roll and is not admitted by ordinary caller0 after departure. no_reset_zone_reset receives runtime zone index but selects/updates SQL zones.id; the schema separates auto id from virtual number, while committed touch writes by number. Qualify an isolated case with divergent SQL id/runtime index/virtual805, then select a separate stable-identity fix if warranted. No live wrong-zone reset is claimed. |
| ZSQ-YUANTI-COMBAT-GEAR | Post-action Squelcher80579 differs from required Drowcrusher80556. Its proc refuses combat REMOVE/FLEE and has WIELD-only5% silence. Dragonslayer80569 autoattacks visible dragon/dragonkin, can absorb one-in-three dragon breath attacks, and refuses combat REMOVE. Its proc's FLEE block is commented out, but do_flee separately blocks non-CMD_FLEE escapes and has50% ordinary refusal/vitality cost when worn onBODY. Receipt/custody does not prove successful protection, silence, bravery or safe escape. Qualify actual effects and PvP policy before gear-use achievements. |
| ZSQ-YUANTI-TOUCH-AND-FOREIGN-OWNERSHIP | Imported359@80546 binds epic_stone; selected peaceful eligible touch settles its own frozen zone transaction. Imported memory55177 onSkrentherlog feeds Winterhaven55206 Q2881→55362/C1000000/55033, whose two declared starts55005/55400 disperse. That foreign receipt remains Winterhaven-owned. WH55632UP→80545 is an ordinary incoming edge outside the local key chain; no guessed gnomish gate is required. Astral-Tiamat also declares two80571 coin piles@19953. Preserve recipient/zone ownership and shared caps; no extra local Q or inferred demolition receipt. |
| ZSQ-YUANTI-CAPTIVE-OUTCOMES | Both addressed captives provide dialogue, but only the ranger has a native receipt. Prison key possession, hearing distress or lethal requests, meeting slaves and defeating captors do not establish rescue. A selected liberation episode needs living subject IDs, consent/escort/state/destination, credited participants and failure/expiry/restart policy. Explain the setting plainly without rewarding victim distress or inventing a lethal quest. |
| ZSQ-YUANTI-EDITORIAL-AND-PLAYED-QUALIFICATION | Source contains bounded wording candidates (snakkekin ground text, bird/feather belt wording, named chest spelling, and collapse/closing-behind descriptions) plus unplaced80527 mobile and80512/80557 objects. Unplaced prototypes are not automatically broken quest prerequisites. Verify intended naming, pick-only chests, phase scope and recipient renewal before selecting any repair. Every actual native change needs its own named fix/news commit with trigger, before/after and validation. Qualify accounting-on admitted sources, hand-in/reward, custom use/retirement/movement, combat gear, touch and persistence; source review alone does not prove played success. |

## Caves of Mt. Skelenak: explain narrative stages without imposing gates

The [journal](../../areas/story/caves_skelenak.story.json) and [dossier](../design/zone-stories/CAVES_OF_MT_SKELENAK.md) retain nine exact native identities, paired materials and independent eye returns. Keep native topic spelling, hazards, shared access, foreign ownership and missing-supply caveats explicit. Selected acquired/learned/cure/liberation events need authoritative evidence before credit.

| Reference | Finding and intended follow-up | Qualification / reporting |
| --- | --- | --- |
| ZSQ-SKELENAK-CAMPAIGNS | Explain Goortok’s three tests and the monk’s six returns together while retaining nine independent native identities. A future campaign must choose narrative ordering, optional stages and explicit gates rather than assuming the dialogue enforces them. | Preserve nine units/raw receipts and out-of-order acceptance; source context/material rows add no units. |
| ZSQ-SKELENAK-LORE-AND-TOPICS | Raw9M/18 aliases differs from inventory8M/17 because gid'nama fails the shared ASCII topic filter. Add dedicated native-topic text/normalization and alias support without loosening stable IDs; select successful ASK/READ learned events with giver/topic/response identity, spoiler and phase policy. Qualify Moralon/tree/cave/oasis/name leads separately. | Native isname accepts punctuation. Player orientation gives the monolith clue without exposing the answer keyword; exact native keyword is builder evidence. No verified current oasis continuation or monolith placement is fabricated, and native unguarded answers remain unchanged. |
| ZSQ-SKELENAK-REWARDLESS-AND-ORDER | Three Goortok returns have zero rewards but admitted native offering/continuation paths. Narrated test/tyrant order does not restrict hand-ins. Qualify rewardless accepted settlement, replay and recovery as well as sphere-first departure. | Synthetic history and complete source review pass separately from a future played accounting-on journey; no invented reward or prerequisite. |
| ZSQ-SKELENAK-CUSTODY-AND-PAIRS | Two monk requests require distinct exact pairs together; supplied matching loose stock is valid. First recovery, source versus gift, kill, current custody and accepted history need distinct selected evidence. | Project exact current rows read-only; reject held/nested/reward-only or duplicate-one-kind readiness. Provenance policy must not retroactively reject native matching supplied goods. |
| ZSQ-SKELENAK-ADMITTED-SUPPLY | Mode2/20–30 resets declare capped goods, but active accounting refuses fresh O/P/E/G issuance. Integrate generation-bound source episodes for local skeletal stock, gear pairs, key, travel controller and foreign desk/skulls/tyrants. | Daily eligibility is not fresh stock. Freeze UID graphs/ownership, retirement, recovery and replay before live effects. |
| ZSQ-SKELENAK-MISSING-PRODUCERS | Exact platinum4022/vial4023 prototypes have no active reset/native reward producer or literal custom producer in the reviewed source; monolith4792 is also unplaced. Choose intended placement/producer and lore availability with the builder. | Existing stock and prototype presence differ. ASHRUMITE_VILLAGE’s separate4372 request is not a replacement4022 source. Any chosen placement is a separate named fix/news commit. |
| ZSQ-SKELENAK-MONK-ACCESS | Rock4027@4110 targets4153 with verb555 (CMD_SACK); current climb is556 and SACK registration is commented. No active ordinary inbound exit or other incoming type25 portal reaches4153. Qualify intended access and select an appropriate repair. | Exact command matching/dispatch verified; alternate magical/staff travel is not disproved. Do not silently replace the verb or claim climbing skill grants arrival. Separate repair/news commit after qualification. |
| ZSQ-SKELENAK-ROCK-TRAVEL-AND-FALLS | Non-takeable type25 rock has unlimited charges−1. Preserve ordinary item travel rules, return-only DOWN, mountain/gorge room flags and declared hazardous sectors. A selected access event must distinguish possession, activation, arrival and safe passage. | Keep exact command/destination/controller generation and participant rules; no invented escape, safe traversal or repeat charge reward. |
| ZSQ-SKELENAK-NATIVE-SPEC-BINDINGS | Literal mob4070→piercer and4120→guild_guard refer to no active mobile prototype; local piercer is4026 and both literal IDs exist as rooms. Direct real_mobile0 indexing returns0 when absent. Qualify builder intent and guarded binding hygiene. | Full handlers/fallback reviewed. No reproduced live overwrite or actual local class gate is asserted. Do not automatically attach either handler to a different mob; selected repair separate with tests/news. |
| ZSQ-SKELENAK-KEYS-AND-BARRIERS | Goortok declares4004 for4076DOWN↔4090UP. Boulder approach is ordinary closed; trapdoor and slabs are closed/unlocked; gorge door resets closed/secret. No PUSH controller or universal key sequence is inferred. | Custody, unlock, open, shared access and actual passage need distinct facts. Preserve hazards and graph until builder intent is qualified. |
| ZSQ-SKELENAK-FOREIGN-EYES-AND-TRAP | Pine Hollow16072 desk/16071 eye uses closed/locked/pickable flags13/key0 with one-charge room-wide OPEN acid trap30d10. Elven tomb20603 skull pile/20604 eye is open/key0. | Qualify pick/open/trap and first recovery without deleting hazards or converting container prose to new prerequisites. Local monk owns the accepted eye returns. |
| ZSQ-SKELENAK-FOREIGN-TYRANTS-AND-OWNERSHIP | Warlord26402@26555 declares26438 and has qc_unblock26555 north death behavior. Troll king26006@26133 declares26013 behind door key26011. Those foreign world effects differ from local shield/sphere acceptance. | Keep stable source/giver/controller identities and native supplied-item alternatives. No local personal-kill, barrier or foreign campaign completion is fabricated. |
| ZSQ-SKELENAK-WORLD-BOUNDARIES |4111E targets153645 absent from active world but present in inactiveDuris3.wld. Surface556876 returns only to4131 despite two local incoming approaches. | Qualify intended boundary destinations and asymmetry without treating all routes as broken; any chosen exit repair gets a separate named fix/news commit. |
| ZSQ-SKELENAK-LIBERATION-AND-SUBJECTS | Artificial eyes fill sockets for appearance, without a native promise of restored vision. Tribal tasks anticipate dispersal and the final sphere narrates farewell, without persisted cosmetic subjects, rescued settlements or defeated-tyrant history. | Add selected authoritative subject/outcome adapters before credit. Generic NPC extraction and earlier-looking narrative stages are insufficient evidence. |
| ZSQ-SKELENAK-RENEWAL-EDITORIAL-PLAYED | Monk D1/mode2 can renew on future admitted resets, not immediately; active accounting does not make fresh goods. Preserve native tyrrany spelling/aliases and distinguish lore inconsistencies from actual failures. | Played accounting-on source/return/recovery, availability, access and hazard journeys remain pending. Every chosen actual zone/quest repair needs separate named fix/news reporting with trigger, before/after, scope and validation. |

## Southern Coastal Highway: explain access without inventing gates

The [journal](../../areas/story/highway.story.json) and [dossier](../design/zone-stories/SOUTHERN_COASTAL_HIGHWAY.md) retain independent requests and optional cross-zone history. Distinguish raw command IDs, separate prototypes, exact source actors, shared barriers, current custody and accepted outcomes.

| Follow-up | Evidence and implementation proposal | Qualification / repair boundary |
| --- | --- | --- |
| ZSQ-HIGHWAY-CAMPAIGN-OWNERSHIP | Victor’s Bastine Q187 supplies the local proof ring and steel key; six other Bastine returns consume Highway trophies. Use optional foreign accepted history for Morlanthra and retain three local receipt identities. | No duplicated foreign achievement, inferred royal rank, forced dialogue sequence or new native gate. Qualify cross-zone cold recovery and supplied-ring acceptance. |
| ZSQ-HIGHWAY-PERSONAL-SOURCING | Exact loose ring, chalice and distinct tooth/hair fit native matching. First recovery, gift, equipped/nested custody, personal/group kill and accepted history need separate selected evidence. | Project present inventory read-only. Never infer first recovery or reject matching supplied stock from native receipts. |
| ZSQ-HIGHWAY-KEYS-AND-SHARED-ACCESS | Entry key41414, Victor key41394, Esterra key41396, Nynevena key41395 and loose tree key41343 govern separate doors. Include controller generation, exact direction and authoritative unlock/open/passage events. | Rusty entry key is separate from Victor’s narrated four keys. Shared access and one-way unlocked reverses prevent a universal all-keys possession gate. |
| ZSQ-HIGHWAY-ADMITTED-SUPPLY | Mode2/15–25 declares capped ring-source equipment, loose chalice/tree key, tooth chance10 and hair chance20. Active accounting refuses fresh O/P/E/G issuance. | Daily eligibility is not availability. Qualify admitted generations, UID ownership/retirement, correlated resets, refusal and recovery before live effects. |
| ZSQ-HIGHWAY-DEPARTURE-AND-RENEWAL | Morlanthra’s Q19 D removes him after the accepted ring reward; collector receipts do not remove their recipient. | Receipt is native history, not a persistent rescued mage, changed tree or promised immediate return. Require selected subject state and admitted renewal episodes. |
| ZSQ-HIGHWAY-DIALOGUE-AND-SPOILERS | All8M/25 aliases describe trust, collections and choices without persisted knowledge or accepted membership. Keep exact native topics; world E inscriptions are clue content. | ASK encounters do not prove learned topics. Selected response/read events and spoiler policy must precede hidden-stage credit; out/in dialogue is not a saved branch. |
| ZSQ-HIGHWAY-SWITCH-VERBS | Placed organ41412/type29 uses259=RUB on41584WEST; statue41351 uses176=PRAY on41559DOWN. Generic item_switch changes EX_BLOCKED. | Do not confuse PLAY254 with RUB259 or rebind the organ automatically. Dust/rub-down prose can support current intent. Distinguish exact activation, shared barrier change and passage. |
| ZSQ-HIGHWAY-SCRYING-SUBJECT | Custom organ41349/type13 uses PLAY while sitting, shared static working and LOOKAFAR; no active declared reset/native reward producer was found. | Choose intended prototype/availability and per-actor/controller state after builder review. Far sight is not arrival, and no live state-leak reproduction is claimed. Any chosen repair gets its own fix/news commit. |
| ZSQ-HIGHWAY-TRAP-COMPATIBILITY | Placed chest41353 T516/9/1/50 has OPEN+ROOM flags but damage9 is absent from current enum0–8. Placed skeleton41361 T3140/10/-1/100 has absent damage10 and no implemented MOVE/GET-PUT/OPEN bit; unplaced41362 T3136/0/-1/30 also lacks those trigger bits. | Compare builder intent and historical format before selecting damage/trigger repairs. Qualify charges, actor/group effects, death and accounting persistence. Do not claim dangerous or safe behavior from stale numeric data. Any shipped repair needs separate fix/news reporting. |
| ZSQ-HIGHWAY-MORPH-AND-EQUIPMENT | Original hide41411 is beast equipment; Bastine Q124 rewards distinct cursed41304 bound to kearonor_hide. Imported41915 lightning sword can destroy/release corpses;41917 flame sword has light/dark/fly and combat effects. | Receipt, replacement identity, wearing, transform, equipment activation, death and corpse custody are separate authoritative subjects. No extra local quest or automatic curse correction. |
| ZSQ-HIGHWAY-WONDER-ACTIONS | Wand41350 custom USE takes held source/charges and selects20 outcomes; optional modern adapter freezes RNG, UID, actor/target/origin, charge commit and deferred resolution. Both gem branches clear ITEM_SECRET on the wand. | Property-dependent legacy/modern paths, interruptions, synthetic loot, partial effects, legal targets and intended secret-flag subject need qualification. Charge use and generated loot do not prove a quest; proposed correction separate after intent review. |
| ZSQ-HIGHWAY-MOUNTS-AND-RESET-PARENTS | R changes current mob to the mount: Black Knight’s subsequent visor/saddles belong to stallion41347. Xalinor41361 has no reviewed active shop/literal sale binding; local41362 horses also appear in Alatorin83361 atchance5. | Preserve exact parent/actor/owner and active producer context. Merchant prose or ordinary mount use is not a paid quest; do not fabricate a sale or silently alter world resets. |
| ZSQ-HIGHWAY-BOUNDARIES-AND-HAZARDS | Five active foreign rooms supply ten reciprocal boundary edges;41488DOWN isNOWHERE/-1 despite climb-down prose. Secret doors, panels, well water, currents/falls and pallet-key/pickproof chest routes are distinct. | No automatic missing destination or safe-travel fix. Choose destination/prose intent with builder evidence; keep valid asymmetric routes and hazard data. Separate any actual route repair into fix/news commit. |
| ZSQ-HIGHWAY-PLAYED-AND-EDITORIAL | Native names/king/mentor spellings and collector walking prose versus sentinel flags vary. Preserve receipt identity while documenting source differences. | No replay-free activation, played reward, trap or rescue qualification is claimed. Any chosen native repair needs concrete before/after, separate commit, tests and prominent player news; journal authoring is distinct. |

## Labyrinth of No Return: distinguish proofs, clues and route outcomes

The [journal](../../areas/story/labyrinth.story.json) and [dossier](../design/zone-stories/LABYRINTH_OF_NO_RETURN.md) keep exact independent bundles. Treat F parents, raw door state, registered switch verbs, map reliability, foreign receipt ownership and source-versus-transfer evidence explicitly.

| Follow-up | Evidence and implementation proposal | Qualification / repair boundary |
| --- | --- | --- |
| ZSQ-LABYRINTH-INDEPENDENT-CAMPAIGN | Adventurer QA24, knight Q85 and Vadatorn Q117 remain three independent native returns. The adventurer's farewell suggests the other two; no native history gate is enforced. | Builders may select an optional larger campaign with explicit stage/ALL/ANY policy. Do not silently require map rescue, learned topics, personal hunts or local navigation before acceptance. |
| ZSQ-LABYRINTH-SOURCE-OR-TRANSFER | Thirteen exact offerings come from E/G/P sources, including the held explorer map and web-container piece. Exact matching supplied loose stock fits. | First recovery needs committed root UID/source actor/room/container/generation and transfer/party policy. Holding, nesting, kills and source narrative cannot stand in for accepted history. |
| ZSQ-LABYRINTH-EXACT-BUNDLES | Knight requires nine different kinds, including all four moss colors; Vadatorn requires distinct tail/heart/eye. Large web5019, salamander tail5045 and rewards differ from offering IDs. | Current-material rows project only loose readiness. Qualify missing kinds, duplicates, nested/equipped roots, admitted transfers, partial offerings and authoritative consumption without adding nine achievements. |
| ZSQ-LABYRINTH-FOLLOWER-PARENTS | Grove5248 starts green5010 M/G5008; subsequent F5011/G5009, F5008/G5006 and F5009/G5007 change equipment parent to the follower. Caps are shared across other declared moss placements. | Preserve exact M/F parent, spawned UID, source episode and refusal. F grouping is source provenance, not player party credit or proof that every reset produced each patch. |
| ZSQ-LABYRINTH-ADMITTED-SUPPLY | Mode1/lifespan60–69 declares capped map/proofs/keys/containers/imports. Active accounting currently refuses fresh O/P/E/G issuance before placement. | Journals require active, ready accounting, but a daily clock cannot create stock. Qualify admitted issuance/generation/owner/retirement, renewal, refusal and recovery before live availability claims. |
| ZSQ-LABYRINTH-CONTROL-OUTCOMES | Three placed levers use PULL340 on5276N/5235E/5002E; stone5046 uses PUSH270 on5134W; strange moss5047 uses GET10 on5134N. Generic specials precede ordinary commands. | Non-takeable moss can activate the switch without an inventory item. Selected milestones need actual visible controller UID/generation/command/actor, barrier pre/post state and passage; already-open shared routes grant no personal activation proof. |
| ZSQ-LABYRINTH-DOORS-AND-KEYS | Diamondine key5057 is salamander5006 G@5134 and fits5134E/5135W plus5295E. Entrance5000W has key0/WLD15/D6; reverse5295E has key5057/WLD3/D2. | setup_dir masks WLD state to low2 bits; reset D6 means closed/locked/secret, without BLOCKED. Preserve asymmetric/shared access and verify reverse unlock behavior. No guessed key, cleared door or required personal key recovery. |
| ZSQ-LABYRINTH-MAP-RELIABILITY | Map5014 says start at entrance and3 north. Current graph gives5000N→5008N→5019, then noN at step3. It explicitly questions its own remembered directions. | Builder must choose intentional unreliable clue versus outdated route. Proposed separate fix/news work: validate the intended complete route and edit approved directions, or clarify unreliable lore. Never replace navigation silently or claim player escape from QA24. |
| ZSQ-LABYRINTH-DEPARTURE-AND-CURE | QA24 and Q117 D remove their recipients after reward; knight remains. Vadatorn's ward/release and the adventurer's homecoming are narrated. | Persistent cured/free/escaped subjects need committed actor/subject/generation/outcome and admitted renewal. Accepted departure alone does not persist a cure, prove arrival home or guarantee immediate fresh NPC stock. |
| ZSQ-LABYRINTH-RESPONSE-AUDIENCE | Eight M/MA blocks expose31 addressed aliases; MA65 proof list and QA24 use echoAll. A is audience, not an alignment gate or credit rule. | Encounter concerns the actor. Learned response needs delivered reply/actor/selected topic evidence; same-room eligible group completion recipients come from frozen settlement context, not everybody who hears prose. No keyword achievement spam. |
| ZSQ-LABYRINTH-FOREIGN-OWNERSHIP | Tower9321 QA15 accepts calcite5011+flask5016+tattoo5035 for E18100/I9375/C53000/D1@9340. Local sources: worm@5129, explorer's nested sack@5189, ground treasure@5003. | That foreign recipient owns the exact receipt; no fourth local achievement. Preserve conflicting custody, supplied routes, foreign encounter visibility and cold history. Four foreign web-container and one guano declaration do not issue local proofs. |
| ZSQ-LABYRINTH-EQUIPMENT-AND-NODES | Sigil5031 carries protection effects without a local navigation proc. Imported epic node359, memory55184 and treasure67233 are separate from three native returns. | Actual wearing/protection/reading/arrival and committed node effects need selected subject and successful effects. Narrated guidance, _noquest_ memory, zero-charge trap metadata and ordinary loot give no extra Q completion. |
| ZSQ-LABYRINTH-EPIC-CLAIM-AND-ABSORPTION | Node359 uses eligible peaceful TOUCH, loaded-zone checks, frozen same-room group participants and committed MySQL zone_touch settlement. Absorption retires a root, then inspects it and follows next_content that removal clears. | Integrate selected committed claim identity/beneficiary/zone/generation and frozen recovery, never touch/hum observation. Source-only maintenance candidate: save successor before extraction and choose one retirement branch. Qualify ground/carried multiple roots and pool reuse before a separate fix/news commit; repeated periodic sweeps can clear later roots, no reproduced crash claimed. |
| ZSQ-LABYRINTH-BOUNDARY-AND-PLAYED | The only ordinary foreign boundary pair is5000UP↔mountaintracks20971DOWN; no active incoming type25 portal. Dark/water/hidden-door metadata and repeated maze descriptions are world context. | No guaranteed safe route or denial of other magical travel. Accounting-on acquisition, control, shared doors, handover, reward, departure, group credit and recovery remain unplayed. Any chosen native repair needs separate named commit, before/after validation and prominent news. |

## Khildarak Stronghold: map the expedition without inventing gates

The [journal](../../areas/story/khildarak.story.json) and [dossier](../design/zone-stories/KHILDARAK_STRONGHOLD.md) preserve both exact independent offerings. Keep source-versus-transfer evidence, typed identities, guard dispatch, shared barriers, learned clues, bone-keyed case and actual rescue separate. Missing references and unfinished lore need builder intent before separate native repairs.

| Follow-up | Evidence and implementation proposal | Qualification / repair boundary |
| --- | --- | --- |
| ZSQ-KHILDARAK-INDEPENDENT-CAMPAIGN | Cook Q36 and priest Q106 are independent. Elder lore, cook's tattoo clue, the bone-keyed case and enchanted hammer suggest a larger mine expedition. | Builders may map an optional campaign with explicit stage/ALL/ANY policy. Retain both exact returns without a mandatory cook-first, learned entrance or personal hunt gate. |
| ZSQ-KHILDARAK-LEARNED-CLUE | Eight M blocks/22 aliases span elder, cook and priest. Cook's accepted response identifies a drunken circle-eye clansdwarf; elder names Clan Spore Seeder. No precise selected next reply was found in full local/dynamic/foreign review. | Select intended responder and successful delivered reply, then bind durable actor/topic/response evidence. If the native chain is incomplete, implement an approved response in a separate named fix/news commit; do not invent a location or keyword achievement. |
| ZSQ-KHILDARAK-SOURCE-OR-TRANSFER | Egg17074 is ground Ocap2@17344/17348; tentacle17022 is ground Ocap1@17648. Both native returns accept matching supplied loose stock. | First recovery needs committed root UID/source room/container/actor/generation and transfer/party policy. Kill, possession, source provenance and accepted proof stay distinct. No monster-drop promise. |
| ZSQ-KHILDARAK-ADMITTED-SUPPLY | Mode2/lifespan25–30, capped goods, case and contents describe reset intent. Fresh O/P/E/G issuance is refused under current active accounting authority. | Qualify admitted generation/ownership/renewal/retirement/refusal and recovery. Active accounting is required for progress, but discovery or a daily clock cannot create supplies or prove availability. |
| ZSQ-KHILDARAK-PROOF-AND-RESCUE | Priest accepts I17022 for I17020/D0; thanks narrate killed krakens and homecoming. Three17247 resets@17643 coexist with his uncertain estimate of two. | Add personal-kill and committed rescued/returned subject outcomes only with selected actor/subject UID/generation/episode and renewal. No kill-count prerequisite, false definite narration bug or automatic rescue from proof. |
| ZSQ-KHILDARAK-GUARD-AND-PASSAGE | Kraken17247 intentionally uses guild_guard:17643DOWN is denied at its birth room, with trusted bypass. Outer dispatch requires awake/nonimmobile eligible NPC. | Qualification must cover awake/sleeping/immobile/moved/removed/shared guard state, actual passage and participant identity. Passage or supplied proof cannot establish personal combat credit. |
| ZSQ-KHILDARAK-MISSING-ROCK-CONTROLS | Objects17014/17015 declare SHOVE274 for17537DOWN/17637UP, but no local/global object reset producer was found. Both WLD exits are state0 and no D reset covers the pair. | Builder must choose intended barrier/control policy after played inspection. Proposed separate fix/news work may restore a chosen controller and matched barrier or clarify obsolete narrative; missing objects alone do not prove blocked access. |
| ZSQ-KHILDARAK-CONTROL-OUTCOMES | Fourteen other type29 controllers declare exact PUSH/PULL/REMOVE verbs and source/target directions. Ash uses REMOVE66; crank uses PUSH270, despite turnable description. | Bind visible controller UID/generation, actor, exact command, target pre/post state and passage. Shared/already-open routes are not personal activation. Review prose/verb mismatch before any separate editorial repair. |
| ZSQ-KHILDARAK-BONE-CASE-HAMMER | Bone17020/type18 keys case17019/type15@17160; hammer17021 is P inside. Priest return, key custody, unlock/open, loot and wearing are different. | Integrate successful shared container mutations and admitted item recovery with actor/subject/generation. Do not claim a hammer receipt or personal unlock from carrying the bone, or assume untouched shared contents. |
| ZSQ-KHILDARAK-WORN-EFFECT | khildarak_warhammer periodically checks worn state and1/30 chance, then one of seven wearer spells. Legend describes seeming invincibility. | Record only selected successful equipment/effect outcome with subject and episode; ownership, timer/chance and legend do not guarantee protection or a new local return. |
| ZSQ-KHILDARAK-SERVICES-AND-CUSTOM-EXCHANGES | Bartender17022 offers generated quests; Kannard400000 uses epic_store; Harvester400001 commits3 loose400230→400231 under harvester craft discipline. Teachers, guild founder, inn and pet shops are other services. | Future selected adapters must bind committed operation/recipe/actor/beneficiary identity, refusal/replay/recovery. Service previews do not award progress. Accounting-on pet purchase/rent/unrent is intentionally refused; no service prerequisite or third nativeQ. |
| ZSQ-KHILDARAK-MISSING-PROTOTYPES | M17271@17271 has no active/any discovered mobile prototype. G6070/6109/6110 on merchant17003@17281 and exact SHP17003 stocks have no active object prototypes. | Source reference gaps lie outside the two reward recipes. Builder must identify intended replacement/renumber/removal. Any selected repair needs separate named fix/news commit and original-fails/repaired-passes reference and availability checks. |
| ZSQ-KHILDARAK-WORLD-EDITORIAL-INTENT | Outcast17722 explicitly says COMING SOON;17737/17738 have empty guild descriptions and17749 opens with truncated prose. Armor-room lore has no uniquely identified hero armor source. | Review intended content, archived design and builder ownership before adding rooms, equipment or exchanges. Proposed approved prose/content completion belongs in separate fix/news work; no invented outcast quest. |
| ZSQ-KHILDARAK-TYPED-AND-FOREIGN-OWNERSHIP | Mobile17022 is bartender, item17022 is tentacle; mobile/shop17074 is weapon vendor, item17074 is egg. Five Alatorin groups reuse selected actors; seven boundary edges and travel handlers cross zones. | Preserve typed identities and actual recipient/zone authority. Source reuse or foreign arrival does not duplicate local units. Dynamic travel must use committed arrival evidence with actual destination. |
| ZSQ-KHILDARAK-DEVOURED-STOCK | Female17199/devour is at17343/17345/17347; eggs are at17344/17348. Periodic devour consumes food/corpses only where the actor currently is. | Movement/co-residence and admitted retirement need played evidence before any supply fix. Do not claim immediate nest egg destruction or reassign a special based only on adjacent reset rooms. |
| ZSQ-KHILDARAK-ACCOUNTING-AND-PLAYED | Complete source mapping preserves two achievements/two potential dailies; admitted supply, controls, handover, coin/item reward, rescued subjects and durable history are separate. | Run active-accounting acquisition/supplied/held/nested/shared guard/case/return/replay/cold journeys before live claims. Native repairs require separate named commits, scoped before/after evidence and prominent news reporting. |

## Strathor Valley: explain the war without inventing gates

The [journal](../../areas/story/stormht.story.json) and [dossier](../design/zone-stories/STRATHOR_VALLEY.md) keep one exact native proof offering. Source recovery, supplied proof, public thanks, learned war lore, remote passage changes and container/equipment outcomes remain distinct. Builders should select book/torch/rune/vault/chest intent before any separate named native fix/news commit.

## Builder and universal capability follow-ups

| Follow-up | Evidence and implementation proposal | Qualification / repair boundary |
| --- | --- | --- |
| ZSQ-STORMHT-PROOF-AND-SAFETY | King30871 QA63 accepts I40073 for I30846/E60000/D0. Thanks narrate a slain Sultan and safe kingdom; native acceptance checks the offering. | Add selected personal combat and committed threat/subject outcomes only with actor/subject UID/generation/episode and renewal. Keep the native proof return available to matching supplied goods without invented kill or safety gates. |
| ZSQ-STORMHT-LEARNED-WAR-LORE | Eight addressed M/MA blocks/sixteen aliases cover seven actors. Greetings describe recruitment, war and local duties; no further local return exists. | Bind a selected successfully delivered response to actor/topic/response identity when learned clues matter. Encounter, alias invocation and a public echo are different; no per-keyword achievement or mandatory greeting chain. |
| ZSQ-STORMHT-PERIODIC-SONG | Bard30868 has raw M qc_action200 and a50% M declaration@30822 with instruments. Generic periodic execution differs from advertised ASK topics. | A future hearing event needs actual recipient/delivery, subject and episode identity. Periodic intent, shared listeners, instrument custody and successful playing must remain distinct; do not expose the control token as a player topic. |
| ZSQ-STORMHT-FOREIGN-SOURCE-OR-TRANSFER | Exact sword40073 is Ecap1/slot16 on Sultan40073@40198 in Nizari. Mob/item40073 and local mob30845/item30845 are typed identities. | First recovery needs committed item UID, owner/source room/container, actor and generation plus transfer/party policy. Foreign source ownership does not create another local unit or imply a comprehensive review of Nizari. |
| ZSQ-STORMHT-ADMITTED-SUPPLY | Mode2/lifespan30–45, capped sword/equipment/containers and potion caps describe reset intent. Active accounting refuses fresh O/P/E/G issuance. | Qualify admitted stock, ownership, renewal, refusal, retirement and cold recovery. Discovery and a daily clock cannot create supplies or guarantee each declared potion row is admitted. |
| ZSQ-STORMHT-REMOTE-BOOK | Red book30825 Ocap1@30923 is PULL340 for30916S→30934. D8 setsBLOCKED; reverse30934N D0 is open. RoomE describes PUSH. | Record exact controller/actor/command/pre/post state and distinguish remote shared changes from personal passage. Confirm intended verb/prose and access symmetry before a separate named editorial or control fix/news commit. |
| ZSQ-STORMHT-TORCH-PASSAGE | Torch30812 Ocap1@30884 is PULL340 for30884S. WLD5/D5 is closed+secret withoutBLOCKED; generic item_switch requiresBLOCKED. | Source reproduces a switch/state mismatch, not impossible access. Qualify standard OPEN and shared state, then choose a separate barrier/control or prose repair with before/after coverage. |
| ZSQ-STORMHT-MANSION-RUNES | Room30848 describes tracing magical runes. Its west door is WLD1/D1/key0; no selected TRACE/rune handler or controller was found. | Builder should confirm intended action or obsolete prose. Add a selected interaction only after design/authority is explicit; otherwise clarify prose in separate fix/news work. Do not invent a magical admission milestone. |
| ZSQ-STORMHT-GRANITE-DOOR | Key30841 is Gcap1 on the first of two guard30812 declarations@30845.30845N is WLD2/key30841/D2, reverse30849S WLD1/key0/D1. | Map exact key custody, successful unlock/open and arrival independently. Shared state and direction asymmetry need played checks; possessing the key does not prove personal unlock. |
| ZSQ-STORMHT-VAULT-STATE | Vault30850@30934 values400/9/key30851/20 is closeable+locked withoutCLOSED4. Blue key30851 is G on king30871. get checksCLOSED and unlock refuses open containers. | Confirm intended secured vault and played state before changing flags. A selected separate fix/news commit needs original-fails/repaired-passes lock, open/get and shared access tests; do not describe the blue key as presently mandatory. |
| ZSQ-STORMHT-SEA-CHEST-KEY | Chest30830@30931 values600/29/key0/600 is closed/locked/pickproof. Rusty key30854 is G on the third of four30861 sea hands in that room, not matchingkey0. | Builder should choose intended key or alternate supported access after checking template keys/trust/shared state. Any selected key repair belongs in separate fix/news work; current source alone does not prove impossible recovery. |
| ZSQ-STORMHT-EQUIPMENT-OUTCOMES | King weapon30845 packs263030/power50/chance18:lightning bolt30 and blur263. Selected item actions and legacy proc have different authority; hat/instruments/ballista are separate context. | Bind actual successful use/effect to source UID, actor/target and episode, including refusal/partial/replay/recovery. Custody, chance, prose or a callback does not prove a landed effect or extra quest. No selected ballista firing handler was found. |
| ZSQ-STORMHT-SERVICES-AND-CROSS-ZONE | Bartender30831 offers generated world_quest requests; three shops and ordinary treasure add local services. Goat leg30820 is reused by Alatorin warrior83305; four surface edges cross the boundary. | Preserve selected generated giver/target/recipient authority, shop/craft commits and actual arrival. Do not duplicate the king's unit from ordinary stock, source reuse, service previews or war narration. |
| ZSQ-STORMHT-ACCOUNTING-AND-PLAYED | Complete source mapping retains one achievement/potential daily. Proof delivery, public echo, equipment, shared controls/containers and lasting safety are distinct. | Run active-accounting source/supplied/held/nested/return/reward/shared access/replay/cold journeys before live claims. Fresh progress requires active, ready accounting; frozen recovery is separate. Native repairs require separate named commits and prominent PR/news reporting. |

## Gagga-Jobo: show costs without claiming safe acceptance

The [journal](../../areas/story/goblincave.story.json) and [dossier](../design/zone-stories/GAGGA_JOBO_CAVE_SYSTEM.md) retain six exact accepted histories, four paid services and two item-only requests. Eight material checks cannot select a recipe or pay a fee. Explain current mixed-fee refusal and backpack precedence blocker, exact repeated quantities, valid LEATHER recipient selection and independent source/transfer/commission/use evidence. Selected native dispatch, alias or editorial repairs require separate named fix/news work.

## Builder and universal capability follow-ups

| Follow-up | Evidence and implementation proposal | Qualification / repair boundary |
| --- | --- | --- |
| ZSQ-GOBLINCAVE-CRAFT-CLASSIFICATION | Four Q recipes accept materials plus C fees; two accept only exact items. All six remain journal cards; four paid services have no achievement/daily credit. | Preserve every native binding and history. Two item-only requests retain two candidates; the four-unit achievement reduction is explicit classification. Product custody is not commissioning evidence. |
| ZSQ-GOBLINCAVE-EXACT-QUANTITIES | Shoes1 hide/C1000; shirt2/C2000; backpack3/no fee; gloves4/C10000; swordbone+flesh+hide/C100000; earring2 bones. | Bind exact kinds, repeated root counts and complete terms; alternatives cannot replace repeated quantity or distinct materials. Inputs are loose current inventory; supported completion must use accepted transaction history. |
| ZSQ-GOBLINCAVE-MIXED-FEE-AUTHORITY | Four paid recipes have C inputs; current durable item-only path and active numeric coin handover refuse them. C is copper,1000/platinum. | Extend selected atomic item+coin fee manifests, recipient/recipe identity, balance reservation, refusal/rollback/replay/recovery before enabling. Do not relax current guards or mark a material count as paid service readiness. Selected engine repair needs separate named fix/news work. |
| ZSQ-GOBLINCAVE-RECIPE-PRECEDENCE | Loader prepends Q46 gloves beforeQ39 backpack and G C10000 before its hide goals. Durable match finds the hide, flags the non-item goal unsupported and breaks the outer recipe scan. | Current standard durable GIVE cannot reach the backpack despite item-only daily shape. Add explicit recipe selection or a carefully specified supported-candidate policy, preserving full terms and unsupported refusal. Separate fix/news tests must reproduce original blocker, revised backpack acceptance, safe paid refusal and ambiguity/partial/supplied/replay/cold behavior. |
| ZSQ-GOBLINCAVE-COIN-READINESS | Sidecar schema supports carried/equipped item and completion rows, without a coin balance/reservation row. Eight optional material rows cannot check four fees. | Add read-only selected fee readiness with authoritative balance/reservation context and clear current-versus-accepted display. Funds, banked money, material stock and settlement are separate; never deduct or award from journal rendering. |
| ZSQ-GOBLINCAVE-SOURCE-OR-TRANSFER | Six large chothe19000 declare G19006hide/G19011bone; five pootata19001 declare G19013flesh. Matching supplied roots fit recipes. | First recovery needs committed item UID/source owner/room/container/actor/generation and transfer/party policy. Livestock kill, corpse recovery, loose custody and commissioned acceptance are different. No automatic skinning/slaughter milestone. |
| ZSQ-GOBLINCAVE-ADMITTED-SUPPLY | Mode2/lifespan40–50, capped materials, recipient product stocks and P/O meat are reset intent. Fresh O/P/E/G issuance is refused under active accounting. | Qualify admitted sources, cap exhaustion, ownership, renewal/retirement/refusal and cold recovery. Six declarations do not guarantee six usable hides; discovery/daily clocks cannot create supplies or craft rewards. |
| ZSQ-GOBLINCAVE-PRODUCT-VERSUS-RECEIPT | Leatherworker G19007/19008/19009/19010 and bone-worker G19014 declare ready products in ordinary inventories. Shoes APPLY_MOVE14/+20 is finite; prose says walk forever. | Source loot or transfer of a finished item cannot complete commissioning. Wearing and actual effects need their own selected authority. Confirm intended boast or descriptive correction before separate editorial fix/news work; no infinite movement guarantee. |
| ZSQ-GOBLINCAVE-LIVESTOCK-AND-SHARED-ACCESS | Closed trapdoor19010DOWN/19012UP and gate19017E/19018W have D1/key0. No selected guard/slaughter/feed/transport/conservation special was found. | Add only builder-selected committed door/arrival and subject/episode outcomes. Described prods, troughs, chains, hide scraps and historical conservation do not implement interactions or require a guard kill. |
| ZSQ-GOBLINCAVE-TYPED-GOODS | Item19006 is hide while mob19006 is bone-worker; item19005 is badge while mob19005 is leatherworker. Meat19003/19004, chained mob19003 and pack/table contents are not exact craft proofs. | Preserve type and recipient/source authority. No extra achievement from the apprentice, butcher, ground meat or matching names. Boundary842047 arrival differs from material collection and accepted recipe. |
| ZSQ-GOBLINCAVE-LEARNED-CRAFT-LORE | Seven M blocks/eleven aliases explain quantities and materials. The sword description calls for two flesh kinds, while exact recipe uses pootata flesh and chothe hide. | Record selected successfully delivered responses if builders make learned recipes objectives. Keep exact recipe terms in the journal; builder may clarify prose in separate editorial fix/news work after confirming material intent. |
| ZSQ-GOBLINCAVE-RECIPIENT-ALIAS | Native mob19005 keyword is leatherworker&n; strict isname requires word termination, so plain leatherworker differs. Valid LEATHER is also on apprentice19007 in the same room. | Use the valid alias and actual numbered recipient in the journal. Proposed separate fix/news work should remove the color suffix from the unique intended alias, then cover original mismatch/repaired exact targeting, awake/visible/race/shared-room selection and no accidental apprentice offering. No prototype change ships here. |
| ZSQ-GOBLINCAVE-ACCOUNTING-AND-PLAYED | Full source mapping closes six Q, four service cards and two request candidates; backpack dispatch, mixed fees, stock, reward and equipment remain different qualifications. | Run active-accounting supplied/exact-count/held/nested/partial/dispatch/refusal/return/reward/replay/cold journeys before live claims. Frozen recovery remains separate. Any selected native repair requires a separate named commit and prominent PR/news reporting. |

## Tharnadia: distinguish a suggested story sequence from accepted requirements

The [journal](../../areas/story/tharnadian_ruin.story.json) and [dossier](../design/zone-stories/THARNADIAN_RUIN.md) retain five exact receipts within three cards: five-proof clearance, necromancer proof and spirit-key return. Seven optional material rows distinguish exact supplied loose identities. Alternative recipients are not successive requirements; the final native return has no prior-clearance receipt gate. Keys/shared access and city retaking remain separate. Source findings, compiled-out archaeology and possible editorial corrections need qualified builder follow-up; no native repair ships here.

| Follow-up | Current source evidence and limit | Planned capability or builder action | Required qualification |
| --- | --- | --- | --- |
| ZSQ-THARN-01: crossing zone boundaries | Braddistock and Old Quarter proof feeds Tharnadian recipients, with keys leading onward to the Twin Keeps. A local receipt does not complete the entire expedition. | Add builder-authored cross-zone narrative links that preserve independent local histories and explain source leads, suggested sequence and actual gates separately. | Follow each linked zone in both directions; supplied proof and an already-open shared route must not falsely record a personal predecessor. |
| ZSQ-THARN-02: five distinct proofs | Five different item identities share broad remains names. Four guild/master remains repeat the necromancer extra description. Loose counts are supported; ordinal targeting and provenance are separate. | Keep five exact material rows; add clearer identity/source presentation. Ask the builder whether repeated descriptions should be corrected in a separate editorial fix. | Four kinds plus a duplicate must remain incomplete; five distinct supplied loose kinds may fit. Worn/nested/wrong remains and ambiguous targeting need explicit cases. |
| ZSQ-THARN-03: alternative recipients | Leader5506 and cloaked5528 accept the same five-proof clearance and separately accept necromancer66201; their narrative purposes and disappearance differ. | Retain all four exact native histories within two story units. Future faction/choice branches need explicit committed outcome events rather than inferring allegiance from nearby dialogue. | Either recipient first, both recipients later, replay, spent materials and raw-to-authored recovery must retain history without double achievement/daily units. |
| ZSQ-THARN-04: sequence versus requirements | Dialogue asks for both tasks and proof of readiness, but the final native recipe checks only66201. The necromancer is outside the armoury; his remains have a separate floor reset. | Show suggested progression without adding a synthetic prior-clearance or personal-kill gate. A stricter builder-designed quest needs an explicit reviewed native prerequisite and migration/history policy. | Accept valid supplied66201 with no earlier clearance history; verify normal source route independently. No existing receipt may disappear when narrative links are added. |
| ZSQ-THARN-05: first recovery and source custody | Proof includes foreign G inventory, a floor O reset, Braddistock quest rewards and keys found in a local pouch or a staff area. Acquisition origin is not an accepted-return fact. | Add committed acquisition/transfer/loot/source instance events for first personal recovery, keeping legitimate supplied proof eligible under current recipes. | Source recovery, player handover, nested extraction, inherited stock, duplicate acquisition and cold replay must remain distinguishable without inventing personal combat. |
| ZSQ-THARN-06: keys and shared access | Armoury66200, keep5504, cathedral82110, safe5521, abbatoir5534 and tunnel5527/5528 have different targets. Some opposite exits use different keys or reset states. | Add attributed successful unlock/open/travel events and builder-declared access relationships. Inventory readiness remains read-only and does not prove an untouched door or reward. | Correct/wrong key, both directions, open/shared/locked states, refused action, key supplied by a player and actual arrival; never credit every room listener. |
| ZSQ-THARN-07: spirit departure | Local5514 disappears on5513; cloaked5528 disappears on66201; Braddistock1314 and jaguar1316 have their own foreign disappearing returns. | Record the accepted variant and committed actor departure separately from lasting spirit release, rat extermination, city peace or faction restoration. | Different recipients, absent actor, shared reset/respawn, interrupted settlement, refusal, replay and cold recovery without a second personal release award. |
| ZSQ-THARN-08: howl dispatch and balance | Enabled5503/undead_howl returnsfalse for SET_PERIODIC and rejects nonzero cmd; spawn scheduling therefore declines it and MobCombat sends-102. Its conditional body uses0..99<=10 despite a5% comment and a fear save, not a direct CON threshold. | Reproduce the dispatch mismatch in an isolated executable/gameplay case. Builder must decide intended activation, probability, scope and death/save balance before a separate named native fix/news commit. Do not automatically activate lethal behavior during journal mapping. | Original dispatch failure, intentional repaired dispatch, opponents/group/self/trusted saves, probability boundaries, death iteration and refusal; document exact gameplay consequences and before/after validation. |
| ZSQ-THARN-09: compiled-out source candidates | Mechanical inventory lists30 literal assignments,29 under#if0. Old guild, portal, dagger, bouncer and paid-following code is not enabled Tharnadian behavior. | Extend future source discovery reports with compiler-condition status and confidence; retain raw candidates for builder archaeology without presenting inactive code as live prerequisites or defects. | Disabled/nested/conditional assignments, active5503, dynamic bindings and build defines; make uncertainty explicit instead of guessing whether a legacy block should be enabled. |
| ZSQ-THARN-10: bank and old currency | Banker5520 G5521 opens a closed/locked/pickproof safe5520 containing5522. Ordinary scattered5519 and its prose are a different object. No bank commission is selected here. | If desired, author a treasure exploration branch with committed container access, recovery and currency realization authority; clarify actual money fields before promising spendable value. | Correct safe identity, shared/empty stock, wrong old currency, active accounting container/money refusal, admitted value and recovery without inventing a native quest. |
| ZSQ-THARN-11: hazards and escape route | Room5651 has F25 metadata; fissures have falling/travel context; shadowrift5539@5659 targets surface568439 with ENTER/negative charge. These do not complete any selected return. | Add explicit builder-selected hazard/travel milestones only after reviewing successful outcome hooks and actual restrictions. Keep atmosphere and environmental risk separate from journal completion. | Actual damage/fall/travel arrival, refused/absent portal, shared instance, accounting supply, arena restrictions and no credit from merely seeing prose. |
| ZSQ-THARN-12: supply and accounting | Local mode1/lifespan20–25, cap-limited floor/nested/equipped/given proof and foreign rewards depend on admitted stock. Fresh O/P/E/G mutation is guarded while accounting is active. | Qualify safe proof/reward issuance and recovery through existing accounting authority before promising repeatable source journeys. Keep new journal/discovery/daily credit active-and-ready gated; frozen recovery remains separate. | Disabled/not-ready accounting, admission failure, empty/capped stock, exact settlement, interruption, replay, recovery and an actual active-accounting journey. |
| ZSQ-THARN-13: narrative and restoration | Leader urges retaking; cloaked farewell mentions no treasure despite a gate-key reward. The ethereal key is declared on Perrin without a selected worthiness exchange. Ruined weapons, tracks and older guild prose are context. | Builder decides intended chain, key acquisition, actor motives and restoration outcome. Possible prose corrections need separate editorial commits; a declared inventory key is not proof of a peaceful award. | Verify intended source and gate before rewriting; retain historical returns and legitimate alternative/supplied paths. Do not equate accepted proof with killing every foe or retaking the city. |
| ZSQ-THARN-14: repair/news reporting | This checkpoint changes journal data, documentation and focused regressions; no native repair ships. Dispatch and editorial candidates have different confidence and gameplay consequences. | Any chosen native fix must ship in a separately named commit/PR section with a prominent news-ready trigger and before/after description. Record unresolved intent honestly. | Scope/native-diff audit plus original-fails/repaired-passes and applicable played qualification. Source findings and planned fixes must never be reported as already repaired. |

## Aravne: explain exact multi-stage proof without inventing a gate

The [journal](../../areas/story/clfhaven.story.json) and [dossier](../design/zone-stories/FOREST_CITY_OF_ARAVNE.md) retain nine-heart clearance, three royal hearts and the legacy luck-scroll exchange. Fifteen optional exact-material checks distinguish shared HEART names and supplied proof; final royal acceptance has no earlier receipt gate. Canonical ANESENTHE/MAROONED targets work with actual aliases. Sand-to-Dawndale, epic luck practice, information purchase and successful access are separate leads with authority gaps. Literal discovery misses dynamic teacher and same-VNUM creation paths; no native repair ships.

## Builder and capability follow-ups

| Reference | Evidence and limit | Planned integration or repair | Qualification |
| --- | --- | --- | --- |
| ZSQ-ARAVNE-01: exact proof identity | Nine hearts21649–21657 and three royal hearts21658–21660 share HEART names. Sapling heart21566, corpse21509 and dead hopper21511 are different identities. | Keep separate optional exact-material checks and one accepted return per recipe. Builder names the individual sources; never substitute duplicate hearts or infer rescue from a dead animal. | Missing ninth kind, duplicate first kind, similar name, held/nested proof, supplied loose identity and exact native acceptance. |
| ZSQ-ARAVNE-02: sequence versus prerequisite | Q32 awards21663; the throne door uses21663. Q51 checks only its three royal proofs, without a previous Q32 receipt gate. | Show the suggested key route and independently accepted final return. Stronger story prerequisites require explicit reviewed native rules and a history policy. | Final return first, already-open door, supplied proof, key custody, replay and raw-to-authored history; never erase existing accepted receipts. |
| ZSQ-ARAVNE-03: source and first recovery | G hearts, externally equipped orb, external heart, quest-reward key and custom-created sand have different acquisition histories. | Record admitted creation/source instance, committed pickup/loot/transfer and first personal recovery with actor/recipient identity. Carrying exact supplied proof remains valid for native recipes. | Source pickup, player transfer, shared loot, nested extraction, repeat acquisition, inherited stock and cold recovery; no personal-kill claim from custody. |
| ZSQ-ARAVNE-04: door attribution | Stargazer21559, mansion21613, sun21620, courtyard21636 and throne21663 have different targets. Stargazer reverse declareskey0; generic picking and shared reciprocal state also matter. | Add builder-declared key relationships and successful attributed unlock/open/arrival events. Inventory is read-only readiness, not proof of key use or exclusive access. | Both directions, correct/wrong/supplied key, shared open/closed/locked/secret/pickproof states, refusal, reciprocal unlock and actual arrival. |
| ZSQ-ARAVNE-05: staging and command identity | Vine21500 uses command65/GRAB; five portals use7/ENTER. External55632 D9 reaches21937; its incoming ordinary path starts in Winterhaven staging rooms. | Preserve routes; qualify actual player entry/controller before calling the staging graph an access bypass. Tooling must decode command numbers and directional exits accurately. | GRAB versus ENTER, absent portal, negative charge, arena restrictions, staff/normal entry and successful destination; graph existence alone gives no credit. |
| ZSQ-ARAVNE-06: sand death and issuance | Enabled21673/wh_corpse_to_object allocates21673, places it locally and sets a timer. The death branch invokes the callback instead of make_corpse; FALSE does not restore ordinary corpse creation. UID/creation-candidate is not admitted ownership. | Add a safe committed custom-death object issuance adapter with source/death identity, expiration and recovery. Review intended corpse replacement and refusal behavior before a separate native fix. | Actual death dispatch, missing prototype, admission refusal, valid room ownership, pickup failure, timed expiry, replay/recovery and intentional corpse behavior; original-fails/repaired-passes before news. |
| ZSQ-ARAVNE-07: sand-to-lens handoff | Dawndale smith77543@77603 Q159 accepts21673 plus25000copper for77566. It is an outside-zone native return, without a local fourth receipt. | Add cross-zone story links and a safe mixed item/fee reservation, reward issuance and exact acceptance path. Keep origin, handed-in material, fee and lens distinct. | Both payment orders, insufficient/wrong denomination, wrong sand, supplied sand, authority refusal, interrupted settlement, exact replay and cold recovery. |
| ZSQ-ARAVNE-08: legacy tablet source | Tablet402 has no declared global reset or selected compiled object producer; prose requests another arbitrary artifact, while Q83 only lists heart/orb/tablet. Other foreign recipes compete for those proofs. | Builder chooses to restore a supported starter, retire the legacy exchange or document its heritage. Implement authority-backed arbitrary-artifact acceptance only if intended; do not invent a current producer. | Actual starter/source, artifact eligibility/ownership, destructive offering, competing endpoints, old history, active accounting admission and replay. An absent source is not proof that all trainers are broken. |
| ZSQ-ARAVNE-09: scroll versus epic training | Scroll409 is type13 with zero values and no selected stat-use callback. Babedo21535 is dynamically bound to epic_teacher/SKILL_EPIC_LUCK at full boot; PRACTICE purchases are guarded under active accounting. | Explain the separate paths. Builder decides whether the scroll should have a supported effect; design accounted epic/coin/skill mutation separately before enabling training. | Full versus mini boot, actual binding, full skill name, level/max/class/funds, atomic effect and refusal/replay/recovery. Do not credit a luck increase from receiving or reading the scroll. |
| ZSQ-ARAVNE-10: paid information authority | Marooned xixchil21549 LIST sells unique/main-or-major/ioun at50/500/175platinum; accounting guard precedes fee. Legacy code destroys NPC cash and publishes artifact locations. | Preserve refusal; add atomic fee, information-release receipt and source category policy with authoritative snapshots. Questions remain leads and payment is a service outcome. | No charge on invalid category/refusal, one charge/release, interrupted result, replay, nested and player-owned artifacts, empty result, actor/recipient changes and readiness. |
| ZSQ-ARAVNE-11: nested artifact owner | llyren climbs the outer container but reads worn/carried owner from inner t_obj->loc, whose active union member is inside. Source suggests invalid owner interpretation; no executed failure claimed. | Isolate an owner-resolution regression, then repair to the verified outer owner in a separately named fix/news commit if reproduced. Preserve player-owned and corpse exclusions. | One/multiple nested levels, NPC/player carrier, worn outer container, room container, player corpse, null/malformed chain and disclosure rules; original-fails/repaired-passes plus applicable gameplay. |
| ZSQ-ARAVNE-12: lore and actual restoration | Tor awakening, Prime Tree vision, justice leaf, city curse, freeing Kotuss, noble columns, merchant/craft roles and ordinary corpses have no selected accepted state change solely from prose. | Builder names intended branches and writes explicit adapters for successful rescue, observation, crafting, judgment or restoration. Keep atmospheric description out of automatic achievements. | Successful/refused/partial outcomes, original actor, shared state/reset, supplied material and permanent versus temporary effects; exact accepted return cannot stand for every promised outcome. |
| ZSQ-ARAVNE-13: dynamic discovery | Literal assignment index finds three enabled candidates but misses Babedo’s table-driven binding and21673 same-VNUM death-object creation. Class _spec3_ labels are specialization, not quest callbacks. | Extend source discovery with compiler condition, typed prototype role, computed/table-driven bindings and creation provenance. Maintain manual full-handler qualification and confidence. | Disabled table row, full/mini boot, literal/dynamic overlap, source actor/object same number, numeric spell/command false positives and resolved callback precedence. |
| ZSQ-ARAVNE-14: editorial accuracy | Dialogue says Anasenthe/Barbrathos rather than actual Anesenthe/Berbrathus; ASK ioun advertises LIST unique. Early reward prose differs from key-first actual acceptance. | Journal uses actual targeting aliases and native results now. Review intended lore, then place any source prose corrections in separate named editorial/news commits. | Correct NPC targeting, actual LIST category, exact reward and builder approval of story intent; never present a journal wording improvement as an already-shipped native repair. |
| ZSQ-ARAVNE-15: supply and accounting | Mode1/lifespan30–35, capped G/O/E/P and shops, rare/shared sources and active-accounting fresh reset guards limit stock. Custom creation and mixed fees add other admission gaps. | Qualify safe proof/reward/shop/custom issuance and recovery before promising complete daily source journeys. All new discovery/journal/achievement/daily credit requires active, ready accounting; frozen recovery stays separate. | Disabled/not-ready accounting, capped/empty stock, safe supply refusal, admitted stock, actual settlement, timer, interruption, replay and played source-to-return journey. |
| ZSQ-ARAVNE-16: repair/news reporting | This checkpoint adds a journal, source dossier, plans and focused regression coverage. No native quest, room, object, shop, callback or fee is repaired here. | Chosen native repairs require separate named commits and prominent PR/news trigger, before/after behavior, scope and validation. Track qualification and unresolved intent fairly. | Exact native-diff audit, preserved old journals/history, original-fails/repaired-passes and applicable accounting/gameplay verification; findings are plans until implemented. |

## Great Shaboath: separate supplied proof, shared access and wards

The [journal](../../areas/story/shabo.story.json) and [dossier](../design/zone-stories/THE_GREAT_SHABOATH.md) retain notebook→entry phrase and four distinct spheres→spirit key. Ten contacts/eighteen topic aliases/five optional materials explain the route without minting keyword achievements. The sphere return has no notebook-history gate and remains daily-excluded. SAY handles key-2 door phrases; PUSH270 controls the obelisk, leaving secret/closed state. Computed room loops, temporary race changes, corpse movement, transformations and finite mode0 supply need explicit authority/outcome adapters. No native repair ships.

| Reference | Required support or qualified finding | Builder and implementation follow-up |
| --- | --- | --- |
| ZSQ-SHABO-01 | Information-only notebook acceptance | Preserve exact Q41 receipt despite empty reward list. Distinguish accepted notebook, privately learned phrase, speech and successful entry. Qualify active authority refusal/retry/replay without requiring a physical reward. |
| ZSQ-SHABO-02 | Speech-controlled shared door | Attribute successful SAY and actual key-2 door change with actor/room/direction/before/after/attempt; check speech refusals and reciprocal door state. No notebook-history gate in current code. Retain exterior Darlakanand and interior Darkaland until builder confirms intent. |
| ZSQ-SHABO-03 | Four exact spheres and common targeting names | Retain four item identities and tower/source locations; supplied proof can satisfy the recipe but cannot stand in for personal kill or first source recovery. Test duplicate, held, nested, wrong and transferred proof. |
| ZSQ-SHABO-04 | Spirit departure, key and larger story | Keep Q87 independent and story-only. Key32847 declares breakchance100, not a timed expiry. Priest-release and Grand Savant lore have no selected accepted completion; plan actor-bound defeat/rescue/world-state outcomes before recording them. |
| ZSQ-SHABO-05 | Finite resetmode0 supply | Qualify boot/manual stock, consumed notebook, actor departure, authority admission and refusal/recovery. A daily clock does not imply refreshed stock. Preserve notebook potential daily and spirit daily exclusion; no automatic supply or reset edit. |
| ZSQ-SHABO-06 | Computed room callback discovery | Extend inventory tooling to recognize assign_rooms loops and emit seven actual room bindings with preprocessor/runtime qualification. Three loops are absent from the literal assignment list; standard periodic dispatch uses room VNUM then real_room. |
| ZSQ-SHABO-07 | Temporary race and gear lifecycle | Add authoritative actor-bound effect/recovery observations with original race, affect/tag, source room, gear custody and reconnect/cold recovery. Qualify leaving strict32800–32929 bounds, delayed restoration, disguise removal and accounting refusal; do not promise immediate restoration or a permanent quest achievement. |
| ZSQ-SHABO-08 | Relocated NPC and player corpses | Qualify ITEM_CORPSE room→32912 movement with death identity, owner, contents, authority admission/refusal and restart recovery. Record ward transport separately from a player's rescue, source kill or first loot. Source moves all corpse types; builder decides intended scope before any repair. |
| ZSQ-SHABO-09 | Enchantment examines first occupant only | Handler returns inside its first occupant iteration, including an NPC. Reproduce occupant-order differences in isolation; confirm whether single-target or room-wide hazard is intended before a separate fix/news commit. Do not silently increase hazard coverage. |
| ZSQ-SHABO-10 | Summoning cap and status condition | Precheck permits count9 followed by two spawns, yielding11; status expression uses GET_STAT(ch)==!STAT_NORMAL. Qualify boundary/status cases and intended cap with builder. Include temporary actor admission/death/recovery; commented MobStartFight is inactive. Any repaired cap/status behavior needs separate news evidence. |
| ZSQ-SHABO-11 | Butler and servant transformations | Track source/created actor IDs, global-helper scope, inherited target and gear ownership through read/transfer/equip/extract with refusal/recovery. Shared helper transformation is not the player's personal quest victory. Review whether global servants outside this encounter should participate before changing scope. |
| ZSQ-SHABO-12 | Pallistren shared state and duplicate allocation | askedquestion/timerr are function statics; transformation calls read_mobile32847 twice and overwrites the first pointer. Qualify two instances, unrelated commands, reset/restart and first/second allocation failure in isolation. isname safely rejects NULL, so do not claim a null-argument crash. Repair instance state/allocation only in a separate named fix/news commit after demonstrated before/after. |
| ZSQ-SHABO-13 | Petrechella standard dispatch and legacy replacement | SET_PERIODIC returns FALSE and nonzero combat command is rejected; standard periodic body is dormant. Body relocates to32885 and creates armed replacements without extracting original servants. Confirm intended retired behavior, then isolate dispatch/custody before a separate repair; do not activate legacy combat as a journal change. |
| ZSQ-SHABO-14 | Directional hazards, helpers and chest imps | Attribute actual movement/effect/survival with actor, source object/room, result and attempt, while retaining trusted/status/open-door checks. Seven shout handlers use combat/path-qualified helpers; chest opening may spawn three imps. Dialogue, encounter and shared actor presence do not prove a kill or survived trap. |
| ZSQ-SHABO-15 | Obelisk and treasure/artifact custody | PUSH270 on switch32828 clears the near-side BLOCKED bit of secret UP exit32895; SECRET/CLOSED remain. Keep reveal/open/unlock/travel/chest creation/first-source recovery as separate facts. Gold/iron keys, random chest stock, holy weapon faction transfers, artifact cooldowns and temporary actor gear require authority-qualified custody. No additional accepted return inferred. |
| ZSQ-SHABO-16 | Journal presentation and fair repair reporting | Show two accepted cards, optional exact supplies, useful ASK contacts and contextual routes/wards. Reveal source details after relevant encounters with explicit shared/personal distinctions; never mint per-keyword credit. Keep source-comprehensive and played qualification separate. Every selected native repair must have a separate named commit and prominent trigger/before/after/scope/validation/news entry; this checkpoint ships none. |

## Firesworn: make material proof clear without inventing a continuation

The [journal](../../areas/story/firesworn_altar.story.json) and [dossier](../design/zone-stories/THE_ALTAR_OF_THE_FIRESWORN.md) retain four different essences→key/hilt/blade fragment. Both sword pieces are rewards; one accepted card/four optional checks does not record guardian kills, Maelshem defeat, dream travel or reforging. Eight contacts/twelve aliases explain keys and larger leads. Static external entry, finite mode0 stock, blank QA text, placeholders and missing sword continuation need separate qualification; no native repair ships.

| Reference | Required support or qualified finding | Builder and implementation follow-up |
| --- | --- | --- |
| ZSQ-FIRESWORN-01 | Four exact essences and three outputs | Preserve QA93 identity:135106–109→135110/135119/135120. Both sword pieces are rewards. Distinguish supplied loose readiness from source recovery/kill; test duplicate, held, nested, wrong and transferred proof, replay and recovery. |
| ZSQ-FIRESWORN-02 | Blank native completion response | QA sets echoAll; it is not automatic completion. Its text is empty. Journal explains the return now; builder should select a clear acceptance/reward message. Any native editorial fix needs a separate named commit/news entry with before/after. |
| ZSQ-FIRESWORN-03 | No reviewed static external entry | Active graph has no external room boundary or fixed/random-zone portal from outside. Qualify intended runtime-created entry before declaring unreachable. Builder selects connection/release intent; no automatic exit or access change. |
| ZSQ-FIRESWORN-04 | Key and directional access outcomes | Attribute actual unlock/open/pick/travel with player, key UID, room/direction/shared state and attempt. Vrolithin/guardian-sentry keys, skeleton key and hellhound collar differ; reverse WLD/reset settings differ. No prior receipt gate; key breakchance100 is not a quest timer. |
| ZSQ-FIRESWORN-05 | Dream phase and portal presentation | Maelshem and projection have independent M resets and EA/no-corpse floor-spill path. Qualify actual death, root object publication, authority custody and successful ENTER135101→135157/135131→135171. No death-triggered projection spawn or personal second-phase credit inferred. Preserve flags and level60 takeability override. |
| ZSQ-FIRESWORN-06 | Unmapped sword continuation | Active selected recipes do not consume hilt135119 or fragment135120; no compiled exact producer/consumer was found. Verify Library of Drakenstone design and implement its intended material/assembly/reward contract separately. Do not promise a reforged weapon from the local receipt. |
| ZSQ-FIRESWORN-07 | Generic combat support beyond literal bindings | No literal custom callback exists, yet packed weapon values select spells and configuration-dependent item actions. Inventory should expose these separately from quest callbacks; `_spec1_` is class specialization and `_no_move_` no-lure/cover. Actual effect/kill outcomes need authority-qualified adapters. |
| ZSQ-FIRESWORN-08 | Hidden proc placeholders | Blood items135114/115 and enchantment135117 are type13/zero-value/unbound, unlike actual packed weapons. Confirm intended retired/missing behavior before any repair; activating new attacks requires builder balance decision and isolated before/after/news. Names alone do not prove a proc. |
| ZSQ-FIRESWORN-09 | Unused bloodstone key | Construct supplies135130; no active exit uses it. Confirm intended target, retirement or omitted passage before changing doors or key supply. Keep a qualified source finding; no automatic key/door edit. |
| ZSQ-FIRESWORN-10 | Finite mode0 supply and daily | Qualify admitted guardian essences, reset/boot/manual stock, rewards, source transfer, refusal and restart recovery. Preserve one achievement/potential daily; neither zone discovery nor daily reset implies regenerated stock or an available payout. |
| ZSQ-FIRESWORN-11 | Unfinished scene and lore boundaries | Several tower/vault rooms have blank prose, mobile135119 is an unreset placeholder, and altars have no selected quest callback. Auril/cold vulnerability/Voluntown/Tiamat descriptions require zone-specific design or gameplay qualification. Do not mint altar, ritual, rescue or permanent ascent-stopping receipts from prose alone. |
| ZSQ-FIRESWORN-12 | Player clarity and repair reporting | Show one card with four distinct supplies, accepted history, eight useful contacts and contextual key/dream/library leads. Source-comprehensive mapping is separate from played qualification. Selected zone/quest repairs require a separate named commit and prominent PR/news trigger/before/after/scope/validation; none ships here. |

## IceCrag mausoleum: keep family names and accepted proof distinct

The [journal](../../areas/story/Voluntown.story.json) and [dossier](../design/zone-stories/THE_ROYAL_MAUSOLEUM_OF_CASTLE_ICECRAG.md) retain six distinct family fragments→Drakenstone key and golden strand→cloak as two independent cards. Seven optional checks/ten contacts/nine aliases explain the larger route. Mary’s lead and the caretaker’s acceptance text do not establish a recorded rescue. Platyr urn starts open, trap type9 is unsupported and Crav charge0 is inactive; preserve builder intent. Imported stone touch and reset/party authority are separate from caretaker returns. Preserve exact registered case, enabled by the separately shipped loader fix; no native repair ships.

| ID | Finding and fair next action | Qualification needed |
| --- | --- | --- |
| ZSQ-VOLUNTOWN-PROOF | Six fragments share KEY FRAGMENT but are six distinct VNUMs. Preserve family-labelled exact checks and accepted receipts. Extend source ownership/first recovery only with a durable actor/source/custody adapter. | Duplicate/wrong/held/nested/supplied proof, spends/replay/reconnect; personal kills must remain separate. |
| ZSQ-VOLUNTOWN-RESCUE | Mary142401 has only a dialogue lead; no selected return accepts seal142400 or records her departure. QA77 accepts strand142450 and describes freedom. The source supports the exchange, not an independently verified rescue. | Builder-approved intended rescue state, actor/group eligibility, committed state changes and reconnect before any new rescue credit. |
| ZSQ-VOLUNTOWN-ACCESS | Outside key97018, six containers, Drakenstone key142443 and seal142400 govern different routes. Door state is shared and the Platyr urn starts open. | Successful unlock/open/reach adapters must distinguish assistance, already-open access and personal actions; no earlier journal gate added to native returns. |
| ZSQ-VOLUNTOWN-TRAP-TYPE | Platyr urn142438 declares trap type9, outside the implemented0–8 cases. Its container flags0 also make it already open, so ordinary OPEN does not fire that trap. This may be placeholder or disabled design. | Confirm builder intent before a separately named trap/urn fix; preserve current access and avoid activating a hazard as a journal repair. |
| ZSQ-VOLUNTOWN-TRAP-STATE | Crav coffin142401 declares sleep/open with charge0. Boru slash/open, Llywelyn room sleep/open and Tavinshir piercing/open have charge1. These states are not interchangeable. | Treat charge0 as inactive, qualify trap detection/disarm/survival outcomes, and obtain design intent before changing charges or effects. |
| ZSQ-VOLUNTOWN-STONE | Imported object359 is bound to epic_stone despite zero literal local assignments. Full touch has power/peace/level/location/busy checks and committed participant/zone rewards. | Extend inventory closure to imported callbacks; keep stone claim, group reward, recovery and zone reset distinct from caretaker return proof. |
| ZSQ-VOLUNTOWN-SUPPLY | Resetmode0 and global caps constrain key/fragment/strand supply. A committed eligible epic touch can request a reset; an attempted touch or daily clock does not guarantee new stock. | Played stock lifecycle, held/nested/spent copies, independent returns, settlement and reconnect; no reset or accounting activation here. |
| ZSQ-VOLUNTOWN-COMBAT | Packed weapons select shatter487, Sepsis197428 and Frozen Star648472; full generic selection and spell effects are separate combat. | Attribution for Haratius/Hynera and six monarch victories, outcome/recovery and item-action configuration before combat story credit. |
| ZSQ-VOLUNTOWN-IDENTIFY | Sanguine Song has _id_ but no declared _id_name_/short/desc extras; generic reveal cannot infer a hidden naming quest. Chaos equipment tables can also supply several local items. | Builder intent for identification prose and alternate custody; do not treat a matching item as local source recovery or silently add a proc. |
| ZSQ-VOLUNTOWN-SCENES | Room142472 is Unnamed with blank prose; unreset mobile142428 has empty keywords and142429 is a placeholder Name. Other dynasty descriptions give rich progression lore. | Builder-approved purpose before separately named scene/placeholder cleanup; no new quest or deleted prototype inferred from incompleteness. |
| ZSQ-VOLUNTOWN-CONTRACT | Both QA markers use room echo, not automatic quests. The caretaker has no departure, coin/XP cost or prerequisite receipt check. | Preserve exact bindings/categories/two achievement and potential-daily units; acceptance wording must describe supported proof and any repair needs a separate fix/news commit. |
| ZSQ-VOLUNTOWN-PRESENTATION | Two accepted cards, six family labels and ten contacts explain one larger journey. Context encounters and optional readiness are not completed steps. | Pagination, accessibility, supplied/spent proof, Mary-only encounter, independent strand receipt, cold/raw-to-authored recovery and active-accounting gameplay qualification. |

## Negative Material Plane: explain native travel and separate accepted forms

The [journal](../../areas/story/negplane.story.json) and [dossier](../design/zone-stories/NEGATIVE_MATERIAL_PLANE.md) retain stars/scrolls→Mournblade/key and orb of unmaking→coins/experience/orb of destruction/glowing key as two independent cards. Seven optional checks/ten contacts/nine aliases explain the route. ENTER, LICK120 and EXAMINE166 use actual portal objects. The searching projection and spirit reset independently; keywords, acquired gear and the spirit note do not prove earlier completion, transformation, failure or personal kills. Preserve ordered effective assignments: orb26662’s later neg_orb overwrites orb_of_destruction. Reward-dialogue/vault-access gaps need builder intent and separate fix/news commits; no native repair ships.

| ID | Finding and fair next action | Qualification needed |
| --- | --- | --- |
| ZSQ-NEGPLANE-PROOF | Q24 takes six distinct stars/scrolls; Q98 takes the orb of unmaking26614, not the rewarded orb of destruction26662. Preserve exact loose checks and accepted receipts. | Duplicate/wrong/held/nested/supplied proof, first-source attribution, spends, replay and reconnect; possession must remain distinct from a guardian kill. |
| ZSQ-NEGPLANE-STAGES | Projection26608 and spirit26644 reset independently at26600/26857. Dialogue describes a previous ritual and death; no selected handler transforms one into the other or requires the earlier receipt. | Builder-approved sequence, per-player versus shared transformation, failure/reset ownership and committed outcomes before adding an enforced phase gate. |
| ZSQ-NEGPLANE-REWARDS | The spirit’s response promises potions, but its actual outputs are C500000/E750000/I26662/I26667. The glowing key leads toward separately reset vault potions. | Confirm whether the prose intends direct potions or access to them; any reward or dialogue change needs a separately named fix/news commit and accounting/replay qualification. |
| ZSQ-NEGPLANE-TRAVEL | Dust/Vacuum/mirrors use ENTER; Salt uses LICK120 and Ash EXAMINE166. Dynamic gate/nether gate/plane shift samples26601–26681 including absent26660, with native eligibility and NOWHERE refusal. | Actor/source/destination and successful movement adapters; refused attempts, mistaken targets, assistance and reconnect; builder intent before any separately named destination-selection repair, and preserve PvP movement restrictions. |
| ZSQ-NEGPLANE-ACCESS | Ominous, shadowy, unmaking, obsidian, sigil and glowing keys serve different gates; file state is decoded by setup_dir and D resets supply shared lock state. The sigil on26835S has file4, which decodes to no door. | Capture successful unlock/open/reach independently from carrying a key or using an already-open route; builder intent before changing the non-gating sigil field or lock design. |
| ZSQ-NEGPLANE-FAILURE | The spirit’s carried note26663 says the powers are lost. It is ordinary carried stock, with no selected failure handler; successful D departure destroys remaining stock. | Distinguish a killed giver/corpse note from accepted departure and a personal failure; qualify reset/recovery before adding durable failure or lockout credit. |
| ZSQ-NEGPLANE-BINDING | Orb26662 is assigned orb_of_destruction then overwritten by neg_orb. Seven literal local assignments describe six effective IDs. | Represent ordered effective assignments in source inventory; confirm intended orb behavior before any separately named proc repair, never silently restore an overwritten effect. |
| ZSQ-NEGPLANE-COMBAT | Negative pockets explode on death; their lethal act messages use the pocket as actor rather than the dying victim, while the death log names the victim. Equipment procs and the Dark’s orb custody are separate effects. | Qualify victim-facing narration before a separate message repair; add durable credited actor/group victory, custody and survivor outcomes without treating passive/proc success as source recovery or return completion. |
| ZSQ-NEGPLANE-IMPORTED | Epic stone360, intelligence pool65, stalker cloak67281 and Living Necroplasm67243 have global callbacks beyond local assignment inventory; Elvenkind cloak and misty gloves have outside reset sources. | Expand imported callback/source closure and configuration-aware outcomes; pool/stone/transform rewards and outside artifacts need separate accounting, custody and recovery qualification. |
| ZSQ-NEGPLANE-SUPPLY | Both Q returns depart and remain story-only: two achievements, zero potential dailies. Mode0, world caps, independently reset givers and stock also constrain retries. | Builder-approved repeatability before any daily opt-in; played stock lifecycle, settlement, departure and reconnect; no reset or accounting activation here. |
| ZSQ-NEGPLANE-VAULT | Nine potion-vault rooms have terse/blank prose and no selected incoming fixed portal, world edge or exact compiled entry. The glowing key reaches26860’s pool/monolith instead. Coins26625 also has a get/room sleep trap. | Builder intent for vault access and scene/reward expectations; retain alternate runtime entry as unqualified, and require successful stat/level/trap outcomes plus separate fix/news commits for repairs. |
| ZSQ-NEGPLANE-PRESENTATION | Two cards, ten contacts, nine dialogue aliases and seven exact materials explain a larger non-linear story. The two Sodolum forms share a name; keywords are not achievements. | Context-only encounters, independent spirit receipt, current/spent proof, missing stock, cold/raw-to-authored recovery, pagination and active-accounting gameplay qualification. |

## Carthapia: three distinct scales and optional shared preparation

The [journal](../../areas/story/prison.story.json) and [dossier](../design/zone-stories/PRISONS_OF_CARTHAPIA.md) retain tarnished coin→coins and three exact Smaug scale sets→shield as two independent cards. Four optional checks/nine contacts/eight aliases explain hidden proof and SEARCH, keys, TUG341 remote torches, ENTER gateways and actual planar routes. Portal7371’s effective nexus callback selects a random eligible surface room despite fixed data7497; no fixed route is inferred. Chaplain7357’s dynamically bound Ki Strike lesson has active-accounting refusal and no story credit. Inmate rescue, tunnel freedom, rune touch and personal source/kill/access outcomes need explicit settled contracts. Builder intent and separate named fix/news commits precede native repairs.

## Builder and capability follow-ups

| ID | Finding and fair next action | Qualification needed |
| --- | --- | --- |
| ZSQ-PRISON-PROOF | Q20 accepts item7372, not a monetary payment; Q47 requires7342/7343/7344 despite their identical names, and grants shield7345. | Exact loose/held/nested/wrong/duplicate checks, accepted spends and reward receipts, replay and reconnect. Do not collapse distinct scale identities or invent a coin fee. |
| ZSQ-PRISON-SOURCE | Coin7372 is P inside hole7349@7353; all three scales are G stock of Smaug7353@7443. No selected local carving or custom issuance handler creates them. | Hidden coin/scales and advanced key SEARCH/extraction, first-source recovery, transfer versus source custody, credited actor/group dragon defeat and container extraction need settled provenance. Possession and accepted return history remain separate. |
| ZSQ-PRISON-SUPPLY | Both givers remain after their Q returns; mode1 and world-cap-one proof/givers constrain availability. Both native units remain daily candidates. | Played stock lifecycle and settlement; daily rollover must not repop a source, reset shared walls or promise stock. Preserve discovery/active-ready-accounting admission and native reset policy. |
| ZSQ-PRISON-KEYS | Guard keys gate ordinary blocks; captain7351 carries obsidian7340 for hidden hatches/oubliette; Warden7333 carries cylinder7334 for locked desk7333 with glass7335. Robed7354 carries elemental7338 and Nexus7377 keys. | Successful SEARCH/unlock/open/entry, shared assistance, key break probability and exact gate roles; carrying a key does not prove personal access. |
| ZSQ-PRISON-SWITCHES | Two type29 torches auto-bind item_switch:7354@7364 TUG341 targets7358W;7355@7387 targets7482W. Ordinary7353 torches have no switch effect. | Actor/object/remote-target and before/after shared state, refusal/no-op/replay/reset; preserve near-side and conditional reverse updates. Journal route facts must follow D-state decoding rather than room names alone. |
| ZSQ-PRISON-TRAVEL | Gateways7337@7386 and7336@7465 connect prison/Limbo; seven type25 portals serve planar branches. Portal7371’s active nexus handler randomly chooses eligible surface rooms rather than fixed value7497. | Successful attributed movement, exact command/ordinal portal resolution, no fixed-destination credit from data alone; preserve combat and native movement restrictions. Outside copies share its cap and callback. |
| ZSQ-PRISON-TRAINING | Chaplain7357@7483 is dynamically bound epic_teacher for KI_STRIKE in normal full boot. PRACTICE requires full skill name and class/level/current-skill/payment rules; active accounting refuses purchases. | Accounted quote/epic+coin debit/skill+save outcome and replay/refund/recovery before adding a lesson completion. No training gate on either Q, and no automatic purchase activation. |
| ZSQ-PRISON-ESCAPE | Secret tunnel7422 says freedom lies upward but has only a north exit; the visitor suspects his partner perished, while other inmate prose suggests rescue. No selected local release or accepted rescue endpoint exists. | Builder intent for tunnel exit and rescue/campaign endings, plus actor/subject/world-state settlement. Any actual exit or prose repair needs a separately named fix/news commit; do not invent an escape route. |
| ZSQ-PRISON-VAULT | Nexus7481N uses key7377/file3; vault7498S has key0/file1, with both reset closed/locked. Normal north unlock/open mirrors reciprocal state despite different keys. Vault flags suppress speech/magic/psi/healing and travel. | Qualify access, return, reset/reconnect and staff intervention before calling the asymmetric reverse key broken. Preserve vault restrictions; rune touch is a distinct settled outcome. |
| ZSQ-PRISON-COMBAT | Warden’s1/8 periodic call invokes guarded shout_and_hunt with eight helper types and100 range. Axe7365 has1/30 temporary Azer/fire spell assistance; dagger7368 packs poison33 at50 with1/30 chance. | Credited actor/pet/group victory, delayed spell/custody and liveness outcomes; no quest credit from ambient shouts, summons, equipment or procs. Any confirmed native repair belongs in a separate fix/news commit. |
| ZSQ-PRISON-IMPORTED | Rune358 auto-loads zone epic parameters and settles through epic_stone/zone_touch_transaction; imported memory55183 is ordinary stock with T2/0/0/0, so its get/put trap has zero charges. Outside Tarrasque stock grants claws83628 instead of local carapace. | Configuration-aware stone payout/level/group/peace/authority, actual stock/proc and provenance. Memory custody, passive lore and outside equipment cannot forge a local return, training, rescue or stat change. |
| ZSQ-PRISON-PRESENTATION | Two cards/nine contacts/eight dialogue aliases/four optional materials/six steps explain independent requests and optional preparation. The three scale items look identical; many torches are ordinary. | Player-readable identity/progress labels, context-only encounters, pagination, current/spent proof, raw-to-authored and cold recovery, and played active-accounting qualification. Builder-owned hints must reflect supported outcomes. |

## Cerberus: alternative trades and precise ingredients

The [journal](../../areas/story/cerebusp.story.json) and [dossier](../design/zone-stories/PITS_OF_CERBERUS.md) explain three coconut recipes, three alternative single-treasure prybar exchanges, two imp crafts and a four-badge vault key. Preserve all nine achievement/daily units; do not require all three prybar alternatives or prior personal victories. Hidden/loose proof and similar coconut/strand/badge identities need precise labels. Accounted shopping is refused, and declared handler loot is distinct from effective dispatch. Internal load-room prose and potential old balance decisions need builder confirmation before native edits.


## Builder and capability follow-ups

| ID | Finding and next action | Qualification needed |
| --- | --- | --- |
| ZSQ-CEREBUSP-PROOF | Preserve nine exact recipes, repeated ingredient quantities and distinct coconut, leather and badge identities. Mojo’s overlapping recipes use first complete matching native offering order, rather than journal selection. | Loose versus equipped/nested proof, wrong/duplicate quantities, simultaneous readiness, offered-item filtering, spent proof, independent receipts, replay and reconnect. Similar names do not establish interchangeable prototypes. Qualify an authoritative explicit recipe selector separately if builders want one. |
| ZSQ-CEREBUSP-SUPPLY | Hidden coconuts reset on the ground and inside palms/nest/anthill; bananas are merchant stock. Sources and all four givers remain shared. | Successful SEARCH/extraction, stock lifecycle and transfer provenance. Daily rollover cannot restock ingredients, repop NPCs or reset shared routes. |
| ZSQ-CEREBUSP-SHOPS | Nomadic shop22026 produces bushel22016/banana22017; outside fruit shop120008 produces22017. Active accounting refuses shop purchases. | Port owned quotes, exact currency payment, produced-item delivery, container destinations, partial batches, refund/replay/recovery before promising a purchase route or recording it. Keep accounting required for new story credit. |
| ZSQ-CEREBUSP-KEYS | Imp key22024 fits hidden prince slab22027E; prybar22009 fits hidden boulders22042S; vault key22042 fits22053D. | Successful SEARCH/unlock/open/entry, exact key roles and break probabilities, shared assistance and reset state. A supplied tool or receipt does not prove personal access. |
| ZSQ-CEREBUSP-CHOICE | Soldier dialogue mentions treasure from two pits, but Q50/55/60 each accepts one treasure for the prybar. | Preserve all three alternative receipts and nine native units. Confirm builder intent before a separately named prose or recipe repair; never silently require two treasures or all three trades. |
| ZSQ-CEREBUSP-BADGES | Four captains supply hidden Hate/Pain/Fear/Rage badges; golem Q100 exchanges the complete set for the vault key. | Attributed actor/group defeats and first source recovery need adapters. The accepted exchange requires exact proof rather than a native earlier-recipe or personal-kill history gate. |
| ZSQ-CEREBUSP-ROUTES | Actual pit portals and hidden doors differ from room lore about golem obstruction, angel rescue and airship plots. | Record attributed movement/search/world-state changes and establish builder-owned endpoints. Check live golem obstruction before calling its prose broken or adding a mandatory kill. |
| ZSQ-CEREBUSP-ROCKWORM | Bound cerberus_load contains random CMD_DEATH room drops, but prototype22024 lacks the death-dispatch flag required by normal die. | Confirm intended availability before any separate fix/news commit. Test flag/caller, credited killer/group, created item ownership and first custody, failures/replay and NPC extraction. Do not automatically activate retired or balance-sensitive loot. |
| ZSQ-CEREBUSP-LOAD | Huge worm22024 resets at internal22072 with sentinel; vault22071 is NO_MOB. Internal room drafts describe a scavenging/delivery network. | Qualify conversion, wandering, scavenging, actual ingress/delivery and reset behavior. Internal builder/load text can be intentional; distinguish unfinished player content from implementation rooms before changing flags or prose. |
| ZSQ-CEREBUSP-SETS | Cloak22063 uses legacy master_set with seven prototypes; set_proc adapter table lists an additional82559, without establishing a local rebind. | Confirm intended equipment membership/counting, duplicates and temporary effects before unification. Worn gear is separate from an accepted crafting or quest receipt. |
| ZSQ-CEREBUSP-CROWN | Crown22070 has periodic revenant transformation and delayed restoration/removal; outside IceCrag stock also contains it. Artifact locator llyren explicitly omits it. | Qualify race/affect/equipment/save/reconnect and delayed liveness. No transformation achievement, quest prerequisite or permanent change is inferred from custody. |
| ZSQ-CEREBUSP-OUTSIDE | Imported Sealot heart links to Divhome; PureDark is an outside departing trade; Alatorin consumes banana22017. Relic76713 says south in its identity but east in its long text; stone358/memory55452 are separate stock. | Outside receipts and issued/transported/consumed ownership remain distinct. Confirm relic wording before a named prose repair. Stone uses authoritative zone epic configuration/settlement; zero-charge memory trap is not a completion. |
| ZSQ-CEREBUSP-PRESENTATION | Nine cards, seventeen contacts, five aliases and twenty-one optional ingredient rows explain linked preparation and alternative trades. | Clear counts/names, pagination, context-only contacts, partial preparation, current/spent proof and raw-to-authored/cold recovery; played active-accounting qualification remains required for native world outcomes. |

## Kimordril: enrich an existing journal without changing its receipts

The [journal](../../areas/story/kimordril.story.json) and [dossier](../design/zone-stories/KIMORDRIL.md) upgrade schema2/revision1 to schema3/revision2 while preserving four request cards and their story/step/contract identities. Label the exact errand potato, explain partial skin stock and the departing accepting mother, and keep shop availability and town service context explicit. Fourteen contacts and aliases explain the zone without creating new native achievements or dailies.

## Builder and capability follow-ups

| ID | Finding and next action | Qualification needed |
| --- | --- | --- |
| ZSQ-KIMORDRIL-PROOF | The supper recipe accepts potato95506, knife95507 and carrot95508 together. Potato95510 has the same displayed name and keywords but is a different item. Three distinct skins retain independent returns. | Exact prototype, loose versus worn/nested proof, lookalike targeting, partial ingredients, supplied proof, spent proof and independent receipts. Preserve all four existing story/step/contract identities. |
| ZSQ-KIMORDRIL-SUPPLY | Dorfgon produces and carries the three errand materials; current accounting refuses shop purchases. Crate95504 at95573 contains three95506 potatoes. | Port owned quote/payment/delivery/refund/recovery before promising purchases. Successful extraction and first recovery remain separate; merchant death removes non-artifact shop stock rather than guaranteeing a loot supply. New credit still requires active ready accounting. |
| ZSQ-KIMORDRIL-DEPART | Q20 pays60 copper/100 experience and removes the accepting mother instance. Two instances can reset at95625/95639 under a shared prototype cap. | Committed reward, exact runtime giver, disappearance, another live instance, native reset/cap and receipt replay/reconnect. Daily rollover cannot respawn mothers or restock their world. |
| ZSQ-KIMORDRIL-SKINS | Specific goat/angry-boar/tame-boar instances carry prepared goatskin95512, black95513 and brown95514. Other instances lack the G stock. No selected carving handler issues these materials. | Attributed death/corpse recovery, source instance, admitted item ownership, group/shared loot and transfer provenance. An accepted trade does not establish a personal hunt or promise a skin from every animal. |
| ZSQ-KIMORDRIL-PANELS | Hidden panel95500 and grey panel95501 use PUSH270 to clear the two secret blocked walls95574S/95598N. Cell key95548 is worn by jailor95549 and fits95643N/95644S. | Successful SEARCH/PUSH, exact selected object/direction, personal movement and shared open state; key access is not a native supper/skin prerequisite. No key or switch receipt is inferred from custody. |
| ZSQ-KIMORDRIL-TRAVEL | Three reciprocal surface routes, secret cliff rocks and ordinary city gates differ from two imported compiled ENTER portals. | Attributed movement, accepted ordinal target, level refusal, random underworld destination versus fixed130200 and native access rules. Neither portal has a static type25 destination, and travel is not a local exchange receipt. |
| ZSQ-KIMORDRIL-DEFENCE | General95535’s periodic shout requests elite guards95505/95532; gate archer95506 has an actual target-room table. Justice has a separate hometown registration. | Actual caller/periodic setup, combat attribution and defender selection; actor/group outcomes and shared safety state need adapters. General’s teacher flag does not override his explicit shout callback. Do not infer a defeat or rescue endpoint from civic lore. |
| ZSQ-KIMORDRIL-WORLDQUEST | Bartender95517 retains world_quest as the shop’s secondary callback. Assignment/map/abandon use committed currency callbacks and native quest state. | Admit exact actor/giver, fee, assigned objective and final reward; failed/stale payment and refund recovery. Keep random world quests separate from four fixed local receipts and their dailies. |
| ZSQ-KIMORDRIL-SERVICES | Counter3097 supplies banking/locker hooks; inn95569 persists terminal rental; money_changer95503 redirects to bank conversion. Paid lockers and shop mutations are refused under accounting. | Service versus quest classification, exact committed currency/ownership outcome and terminal-save/recovery. Observing a counter, forge, guild, mount or equipment cannot stand in for an accepted errand. No epic-teacher purchase is established by the general’s flag alone. |
| ZSQ-KIMORDRIL-POSTAL | Clerk95552 and Postal Center95645 exist, but95552 appears only in a commented old postmaster list. Active lookup registers6097/16695/97583/73. | Builder chooses intended supported service or retired context before a separately named repair/news commit. A room/NPC name does not establish a currently working mail operation or delivery quest. |
| ZSQ-KIMORDRIL-OUTSIDE | General’s actual silverstone hammer95526 is accepted in two outside Alatorin QA recipes; local gear, mounts, table and container also appear in reviewed outside stock groups. | Preserve exact outside giver/recipe/disappearance and reward identities. Local general encounter/equipment cannot complete the outside exchange, and outside zones are not comprehensively claimed here. |
| ZSQ-KIMORDRIL-PROSE | Small Home descriptions at95625/95638/95640/95641 refer to a westward outside doorway while actual exits are east/north/east/north. Postal/justice/guild lore has no accepted local campaign endpoint. | Confirm builder wording and intended routes before a separate named prose fix/news commit. These are directional guidance mismatches, rather than evidence the native exchanges or entire town are broken. |
| ZSQ-KIMORDRIL-PRESENTATION | Upgrade existing schema2/revision1 to schema3/revision2 with four preserved request cards, fourteen contacts/aliases, six optional material checks and ten steps. | Lookalike item labels, context-only contacts, pagination, partial readiness, spent proof, legacy metadata/raw receipt and cold recovery. Played active-accounting purchase/recovery/travel/reward qualification remains separate. |

## Shady Grove: distinguish a return from its source and surroundings

The [journal](../../areas/story/shady.story.json) and [dossier](../design/zone-stories/SHADY_GROVE.md) retain three independent native returns. Explain exact collar/key/amulet proof, partial source stock and accepting NPC departure. Current custody may come from another player; do not infer a personal hunt, first recovery, door opening or learned clue. Keep town services and ambient callbacks contextual, and require builder intent before separately named native repair/news commits.

## Builder and capability follow-ups

| ID | Finding and next action | Qualification needed |
| --- | --- | --- |
| ZSQ-SHADY-PROOF | Three independent native returns accept collar97514, golden skeleton key97582 and old bloodied amulet97572. Other collars, cell/red/brittle keys and Lyren's pendant have different roles. | Preserve all three story/category/step/receipt identities. Exact loose, worn/nested, supplied, spent and replayed proof; another blood sword cannot prove T'Zoul's accepted return. |
| ZSQ-SHADY-SOURCE | Mezef97511@97502 wears collar97514; selected toddler97557@97729 carries key97582; selected assassin97572@97700 wears amulet97572. Other same-prototype instances can lack this stock, and world caps restrict availability. | Committed custody and extraction, actual source instance, death/corpse/group loot and transfers. Do not require a personal hunt or attack on a child. Historical first recovery and source-versus-player provenance need explicit adapters rather than possession inference. |
| ZSQ-SHADY-ACCESS | Jaws97522S/97523N use red key97538 from a selected maggot97500@97523, not the dragon. White gate97509N/97505S is closed with key0. Mezef's brittle97535 opens separate treasury97502E/97503W. | Accepted SEARCH/open/unlock/movement, selected room/direction, actor and shared door state. Key and travel history are guidance, not mandatory native return prerequisites; supplied collar remains valid. Preserve the alternate church/stair approach. |
| ZSQ-SHADY-DEPART | Morgar's Q36 pays2100 experience and removes the accepting instance. His one declared M reset at97580 uses a shared prototype cap4, not four declared spawn sites. | Exact runtime giver, committed reward/disappearance, remaining instance, reset/cap and reconnect/replay. Daily rollover cannot repop the troll or key. |
| ZSQ-SHADY-REWARD-TEXT | T'Zoul Q11 promises1000 gold in prose but encodes C10000, equivalent to100 gold/10 platinum; its blood sword97570 also exists as bank-guard gear. | Builder chooses intended wording or payout before a separate named fix/news commit. Preserve encoded terms meanwhile. Qualify denomination, exact committed reward and independent equipped copies; do not automatically increase the payout. |
| ZSQ-SHADY-AMULET-LORE | Lyren's native dialogue describes a guard while actual amulet stock is on a selected assassin at97700. | Builder confirms intended source/lore before a separate named wording or stock fix/news commit. The exact accepted amulet exists; this mismatch does not establish a broken exchange or missing item. |
| ZSQ-SHADY-AMBIENT | Four ambient assignments mismatch current NPC roles: dog→skunk97509, fisherman→baker97534, jailkeeper→training guard97554 and woman→old mage97539. Named counterparts are97529/97532/97552/97537; unregistered shady2 retains the same selected names/IDs. | Confirm builder intent, callback/periodic setup and shop secondary dispatch before a separately named fix/news commit. These handlers have no item reward or quest state branch; mismatches do not prove the three native recipes are broken. |
| ZSQ-SHADY-MISSING-STOCK | Bakery shop97534 and G resets name6070/6109/6110, absent from all registered object sources. Invalid G object lookups disable those reset commands; production lookup also cannot create missing prototypes. | Builder chooses intended existing prototypes, restoration or deliberate retirement before a separate named stock fix/news commit. Scope is three bakery stock entries; none is a local quest ingredient. Qualify shop production and reset loading without claiming the entire town is broken. |
| ZSQ-SHADY-CELL | Actual jailkeeper97552 wears key97564 for cell97673E/97726W; prisoner97577 occupies it. Morgar stands separately at97580 and requires97582. Justice registration has office/cell0 despite physical prison rooms. | Keep cell access, physical rescue lore and Morgar receipt distinct. Builder defines any intended rescue endpoint and reconciles justice metadata before a separately named change. Do not infer a reward from unlocking or meeting a prisoner. |
| ZSQ-SHADY-SERVICES | Bank counters3097, inn97663, registered postmaster97583, auction97756 and pet-shop97757/loadroom97758 are actual separate services. No local mobile has ACT_TEACHER; explicit null97585 is not a functioning teacher assignment. | Port paid shop/mail/pet payment, ownership, spawn/persistence/refund/recovery before promising them under accounting. Keep terminal inn saving and bank/locker transactions separate. Guild prose and registration tables cannot create quest completions. |
| ZSQ-SHADY-WORLDQUEST | Ixie97540 has two duplicate enabled world_quest assignments to one prototype. World assignment, fees, map, abandon and final reward are separate from the three local returns. | Exact actor/giver/objective, committed fee/reward, stale callback/refund and recovery. Count one effective binding; do not merge random world quests or dialogue keywords into local receipts. |
| ZSQ-SHADY-TRAVEL-OUTSIDE | Three reciprocal surface routes differ from staff/heavens arrivals and imported compiled ENTER portals465/468. Selected foreign stock and an Alatorin QA use local prototypes; wonder devices and Winterhaven helper selection can summon locals. | Preserve random good-continent versus fixed130200 travel, level/target guards, actor/source context and separate outside giver/receipt. Ward54 is a device, not a local prerequisite. Outside zones are not comprehensively claimed here. |
| ZSQ-SHADY-PRESENTATION | Upgrade schema2/revision1 to schema3/revision2:three preserved cards, fourteen contacts, eleven aliases, three optional materials and six steps. | Distinct keys/pendants, context-only contacts, partial readiness, pagination, spent proof, old metadata/raw receipts and cold recovery. Source review and compiled journal journeys remain separate from played active-accounting native rewards and service qualification. |

## Apocalypse Castle: four proofs and independent return

The [complete Apocalypse Castle dossier](../design/zone-stories/APOCALYPSE_CASTLE.md) adds two cards, twelve contacts and thirteen follow-ups. Four different skulls lead to one keeper receipt; the lost bracelet is an independent departing-giver, story-only return. Extend attributed source recovery, container extraction, key/door/search/switch/climb/travel outcomes, shared stock/reset availability and committed epic participants. Distinguish dormant skill tables from effective handlers and clue prose from configured commands. Preserve two achievements and one potential daily. New credit requires active, ready accounting; no native repair ships.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-4HORSE-01 | Four different skulls satisfy one keeper QA; native acceptance does not require an ordered personal boss campaign. | Keep four optional preparation rows and one accepted receipt. Future grouped victory objectives need participant, encounter and committed outcome identities. |
| ZSQ-4HORSE-02 | Skull possession can result from shared loot or a player handoff. | Add attributed first source recovery only through committed custody with source NPC/object/container and transfer lineage. Supplied proof must remain acceptable to the native recipe. |
| ZSQ-4HORSE-03 | The departing zombie and reset mode0 produce a story-only unit; the staying keeper remains one potential daily. | Preserve both unit classifications. Model actual giver/stock availability and shared reset epochs separately from daily rollover. |
| ZSQ-4HORSE-04 | The old diamond bracelet is P-stock in non-takeable rocks below a locked well; the vampire carries the well key. | Add accounted container extraction and successful key/door/movement outcomes. Current loose readiness does not establish opening the well or recovering the bracelet yourself. |
| ZSQ-4HORSE-05 | Keeper lore describes respect; the configured portal uses ENTER. | Confirm intended clue wording, then prepare a separately named prose repair if appropriate. Do not infer or activate a new bow trigger from dialogue. |
| ZSQ-4HORSE-06 | The scroll describes disabling travel magic; PUSH slab actually relocates the actor and does not mutate room flags. A slab extra at the well has no second teleporter stock. | Qualify intended clue and scenery wording before any named text repair. Keep existing travel and balance policy; record successful dispatch and arrival separately from clue exposure. |
| ZSQ-4HORSE-07 | DREAM fountain, SHOVE rubble, climbing/falling and secret doors have different generic dispatch and visibility rules. | Extend attributed switches, searches and movement with prior state, target, destination and success. NOSHOW rubble is addressable by keyword in generic lookup; visible description alone is not a complete affordance model. |
| ZSQ-4HORSE-08 | Boot reset and epic-driven conditional reset coexist with mode0 and shared stock caps. | Add availability annotations backed by actual reset/stock outcomes; never promise a daily repop or treat the zone as permanently disabled. |
| ZSQ-4HORSE-09 | Imported monolith360 is bound to epic_stone. Touch submits an accounted group award and can request a mode0 reset. | Reuse committed epic outcomes; distinguish qualifying participants, actor, pending settlement, replay and reset request. A vault visit or held monolith is not the accepted skull receipt. |
| ZSQ-4HORSE-10 | An unbound skill_beacon table contains vault34804; effective object360 assignment is epic_stone. | Expand effective callback inventory to distinguish dormant tables from reachable bindings. Do not advertise the five skills as a working monolith service without a separate design and qualification. |
| ZSQ-4HORSE-11 | Mankiller and Brainripper have enabled item combat callbacks; no local literal mobile callback or teacher flag creates an extra native quest. | Keep wear/attacker/chance/combat effects separate from accepted returns. Any future weapon trial needs an owned committed objective adapter. |
| ZSQ-4HORSE-12 | Volo's cloak exchange and Sootfoot's thirteen-item set exchange use local loot outside this zone; foreign reset groups also stock local items. | Preserve outside giver and full recipe identities. Do not merge the outside exchange into the keeper card or claim comprehensive review of those entire zones. |
| ZSQ-4HORSE-13 | Groundskeeper key, beholder stalk, gloves and dragon-heart extras contain mismatched descriptions; native exchange ingredients and active targets resolve. | Confirm each intended description and prepare named text fixes with before/after and news scope. Present preparation, access, conversation and accepted history distinctly; no native repair ships at this checkpoint. |

## Ixxillikor: exact returns and unavailable services

The [complete Ixxillikor dossier](../design/zone-stories/IXXILLIKOR.md) adds two cards, twelve contacts and thirteen follow-ups. Three exact ingredients lead to one legacy scroll receipt; the unavailable auction retains one historical identity. Extend availability with source freshness, selected purchase/reward kind, atomic owned settlement and source custody. Effective training is assigned through a startup table loop, while active accounting refuses purchases. Qualify missing tablet production, the inert scroll, missing auction reward and elder-brain loading graph before separately named native fixes. Preserve two achievements and one potential daily. New credit requires active, ready accounting; no native repair ships.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-IXXILLIKOR-01 | Ezallixxel accepts heart32490, orb26614 and tablet402 together for scroll408, without departure. | Keep three optional exact loose checks and one accepted receipt. Supplied ingredients may fit native acceptance; attributed source recovery needs committed custody/source/transfer lineage. |
| ZSQ-IXXILLIKOR-02 | No current tablet402 producer was found in the reviewed global reset and compiled reference inventory. Tablet prose asks for an arbitrary artifact, but the recipe has only three exact I entries. | Confirm intended supported starter and payment design. Restore a producer or retire the legacy route in a separately named fix/news commit; do not invent an artifact gate from prose. |
| ZSQ-IXXILLIKOR-03 | Scroll408 is ITEM_TRASH with zero values and no selected bound effect. READ is look; RECITE requires ITEM_SCROLL. | Confirm intended legacy reward/effect, then separately repair or retire it with before/after, cost and balance qualification. A committed item grant, reading and an ability purchase are distinct outcomes. |
| ZSQ-IXXILLIKOR-04 | Two auction Q entries have identical C10000→I35708/stays; dwarf C20000 exists only in dialogue. Reward35708 is absent from the active object catalog. | Preserve one existing receipt identity. Design explicit selected choice, reward kind, companion ownership and availability before separately restoring the auction or correcting its advertisements. |
| ZSQ-IXXILLIKOR-05 | Active accounting refuses legacy currency quest offerings. The inactive legacy path can consume the payment and skip the missing item reward. | Retain refusal until an owned quote/settlement validates price and reward, commits payment and grant together and supports replay/refund. Restore or retire only with a named fix/news commit and adverse-path qualification. |
| ZSQ-IXXILLIKOR-06 | Ezallixxel4203 and elder brain4208 receive epic_teacher through epic_initialization's table loop during normal non-mini startup. Their purchases refuse active accounting. | Expand effective callback inventory beyond literal assignments and ACT_TEACHER flags. Model paid training with owned epics/coin/skill settlement; it must not forge a Q receipt or free power reward. |
| ZSQ-IXXILLIKOR-07 | Five addressed M/MA records expose seven aliases. Four qc_action records are probabilistic ambient echoes without a selected spawn or harvest mutation. | Record targeted dialogue exposure separately from ambient broadcasts. Future growth, rescue and harvest stories require actual owned adapters, rather than deriving objectives from text or interval numbers. |
| ZSQ-IXXILLIKOR-08 | Elder brain4208/F4254 load at4332. Adjacent4333–4335 are NO_MOB and lead one-way to the lore chamber4322. Normal wandering cannot reach their loot. | Confirm intended loading/arrival/scavenging graph and accessibility; prepare a separate balance-aware fix if appropriate. Never silently open builder rooms or require an unqualified personal boss victory. |
| ZSQ-IXXILLIKOR-09 | One entrance golem stocks key4226. Forward/reverse gate key/reset state differ. Cliff and secret rubble routes are ordinary edges; F25 records a fall at4353. | Reuse the actual legacy door loader and reset state, rather than treating world state as raw runtime flags. Add successful key/search/door/movement outcomes with prior state, target and destination; preserve asymmetry until intended behavior is qualified. |
| ZSQ-IXXILLIKOR-10 | Clothing4204/metalsmith4212 are registered shops; banker4205 has imported counter3097 with storage callback. Ezallixxel's shop prose is not a registered shop. | Distinguish effective service bindings from prose. Reuse accounted bank authority and existing paid-service refusals; expose availability separately from accepted quest progress. Reception lore is not a native room-key return. |
| ZSQ-IXXILLIKOR-11 | Heart32490 is G-stock on Bel32420; orb26614 is E-stock on Dark Prince26642. Other stat-scroll and Sodolum/Zariel recipes compete for the same proofs/tablet. | Preserve exact source and outside giver/recipe identity. Wrong orb, femur, supplied proof and unique-artifact custody are separate; selected dependency closure does not make those entire outside zones comprehensive. |
| ZSQ-IXXILLIKOR-12 | Mode2, cap-limited common stock and unavailable legacy services coexist. Auction is repeatable in the catalog but excluded from dailies for lack of an item offering. | Preserve two achievements/one potential daily and historical recovery. Add actual availability/reset epochs without promising daily restock or changing classifications to hide a blocker. |
| ZSQ-IXXILLIKOR-13 | Exact preparation, unavailable auction/training, ambient lore, shared access and historical receipts need different presentation. | Keep optional Ready now/Missing now rows and Recorded acceptance. Add universal availability with reason/source freshness, selected purchase state and owned participant outcomes as zones gain real adapters. No native repair ships here. |

## Moonshae Island: suggested progression and alternatives

The [complete Moonshae Island dossier](../design/zone-stories/MOONSHAE_ISLAND.md) adds two independent returns, twelve contacts and thirteen follow-ups. Spirit robe→Brigit orb suggests access toward the foreman sword→Tristan reward, but duplicate orb stock and direct acceptance preserve alternatives. Extend committed source custody, actual purification state, successful door/speech/travel, shared combat participants, catch/grant outcomes and availability. Qualify dormant ticket/ship references, retired port destinations and the outside chance20 sword reset before separate repairs. Preserve two achievements/two potential dailies. New credit requires active, ready accounting; no native repair ships.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-MOONSHAE-01 | Brigit's robe→orb suggests access toward the sword, but foreman26214 also carries that orb and Tristan accepts sword26233 directly. | Present suggested progression with two independent receipts. Future dependency edges must name actual enforced state and alternatives; never turn lore into a personal-history prerequisite. |
| ZSQ-MOONSHAE-02 | Spirit26204 G-stocks robe26212; foreman26214 G-stocks sword26233. Supplied proof can satisfy either return. | Add attributed first recovery through committed source/corpse/container custody and transfer lineage. Possession must not manufacture owned combat victory. |
| ZSQ-MOONSHAE-03 | Brigit's acceptance proves robe delivery; no selected local handler changes the cursed moonwell's room/object state. | A real purification objective needs successful world mutation, actor/participants, prior state, shared reset epoch and replay-safe outcome. Confirm intended story before a separately named native implementation. |
| ZSQ-MOONSHAE-04 | Brigit departs, while mode2 retains repeatability and daily eligibility for both receipts. Singleton loot/givers have shared caps. | Preserve two achievements/two potential dailies. Expose actual stock and giver availability; daily rollover does not force repop or erase historical acceptance. |
| ZSQ-MOONSHAE-05 | Orb26209 unlocks the stone route; key26201 fits the king's secret room. Negative-key speech operates only when locked, and reset states do not require all advertised passwords. | Add attributed successful search/unlock/open/movement and conditional speech outcomes. Track loose/held key semantics and prior state; current access or another orb copy cannot replace a Q receipt. |
| ZSQ-MOONSHAE-06 | Five enabled sister_knight callbacks use probabilistic combat shout_and_hunt, selecting helpers by effective callback and viable path. | Keep assistance, faction combat and Brigit's request separate. Future ally/group objectives need encounter identity, qualifying participants and committed outcomes; no ordered sister-kill campaign is invented. |
| ZSQ-MOONSHAE-07 | cc_fisherffolk/cc_female_ffolk have no active binding; the sword's cymric_hugh assignment is commented and has no selected implementation. Structured weapon values remain separate. | Effective callback inventory must distinguish dormant names from reachable handlers and generic item effects. Do not activate obsolete code or promise an extra sword ability as a journal repair. |
| ZSQ-MOONSHAE-08 | Pole26200 is recognized by get_pole. Fishing has skill/water/timing guards and grant_tradeskill_item submits owned creation; XP and catch grant occur on separate branches. | Add successful catch/source receipts backed by committed ownership and clarify paired XP/item outcome before building a fishing quest. Failed grant or ambient water sound must not complete either native return. |
| ZSQ-MOONSHAE-09 | Pawldo26203 and receptionist26233 are registered shops. Ticket table names11100/11300, but11100 is absent,11300 is an active brittle key, and ticket_taker has no active binding. | Confirm whether to retire legacy ticket stock/text or design a supported passenger route. Any restoration needs owned purchase, target voyage, accepted boarding and arrival outcomes, with a separately named fix/news commit. |
| ZSQ-MOONSHAE-10 | Port26200 has two destinations present only in inactive Duris3 data; renum_world removes unresolved exits. Sea26285N points to itself. Current surface/Llyrath/underworld/Vale boundaries resolve. | Confirm intended current port and sea connections before separate zone/generator fixes. Preserve existing access/balance; qualify the effective loaded graph instead of assuming every static edge is usable. |
| ZSQ-MOONSHAE-11 | Cityruin66251 has O26233/cap1/chance20. Current ordinary O handling requires chance100 unless forced and otherwise disables the row. | Record a legacy source reference, not a reliable20% source. Qualify intended rare-reset policy and forced-load behavior before a separate fix/news change; no global rarity behavior is changed here. |
| ZSQ-MOONSHAE-12 | Abal142229's outside QA consumes letter142214 and grants keys142234/26201 despite scepter prose. Vale waterfall26407@26440 ENTER leads to camp26309. | Preserve actual outside recipe and effective travel identities. Selected dependencies do not make those entire zones comprehensive; clue repairs and travel adapters need their own qualified scope. |
| ZSQ-MOONSHAE-13 | Orb26209's extra calls it white/black; green sleeves26221 and silver belt26225 have conflicting color extras; Earthmother robe26217 uses a staff keyword. | Confirm intended names/extras and prepare bounded text fixes separately with news treatment. Show preparation, suggested progression, legacy availability and accepted history distinctly; no native repair ships here. |

## Prison of Fort Boyard: exact recipes, source and reset authority

The [complete Prison of Fort Boyard dossier](../design/zone-stories/PRISON_OF_FORT_BOYARD.md) maps five dragon scales to one cloak exchange, sixteen contacts and thirteen follow-ups. Preserve exact ALL materials, supplied proof and spent-proof acceptance. Extend attributed source/corpse/container custody, successful shared PULL gates, ENTER/JUMP arrivals, intended rescue state and committed epic TOUCH participants/zone reset. Qualify mode0 stock/reset availability, blank paper/note prose, copied dragon descriptions, crate parent identity and the northern scenery/unfinished-route question before separate repairs. Preserve one achievement/one potential daily. New credit requires active, ready accounting; no native repair ships.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-PRISONB-01 | Gixmo43010 consumes scales43003/43006/43008/43011/43012 together for cloak43015, without departure. | One request/achievement/potential daily with five optional readiness rows. Preserve ALL ingredients and commit the exchange once; extra copies of one color, equipment, nested proof, reward possession and conversation do not substitute. |
| ZSQ-PRISONB-02 | Five dragons G-stock their exact scales with cap1/chance100. Supplied scales can satisfy the return. | Add attributed committed first recovery, corpse/container/source identity, transfer lineage and qualifying combat participants. Current readiness and accepted exchange must not manufacture personal victories or five separate achievements. |
| ZSQ-PRISONB-03 | Entrance guard43000@43004 G-stocks prison key43000. State3/key43000/D2 gates are locked and pickproof. | Qualify successful unlock/open/passage, held/loose keys, shared prior state and controller/reset epoch. Access may already be open; personal key or guard-victory history is not an enforced Q input. |
| ZSQ-PRISONB-04 | Lever43010@43014 PULL340 clears43014N→43015; lever43009@43017 clears43016N→43018. D8 blocks only the north approaches; reverse D0 remains open. | Observe successful blocked-state mutation, exact verb/object/target and actual reciprocal behavior. An already clear switch is a no-op. Preserve shared and asymmetric access without requiring every actor to replay it. |
| ZSQ-PRISONB-05 | Fortb pier38106 O-stocks ship43014 ENTER7→43020; plank43023@43022 JUMP264→38106. Both have unlimited charges. | Add authoritative boarding/arrival/return events with exact object UID, command, origin and destination. Preserve current routes and source capacity; no ticket or moving-voyage history is invented. |
| ZSQ-PRISONB-06 | Imported stone359@43018 binds epic_stone and submits a separate accounted group TOUCH reward. Its committed publication can request a mode0 reset. | Reuse committed participant/award/stone UID/zone-epoch authority for future optional objectives. Pending/refused TOUCH, ambient sound, a global zone completion or another member's reward cannot become the actor's cloak receipt. |
| ZSQ-PRISONB-07 | The header's second numeric term is reset mode0; its last1 is difficulty. Gixmo stays and the exchange remains repeatable/daily-eligible. | Expose actual cap-limited scale/giver availability and touch/reset policy. Daily rollover never forces restock. Source reset scheduling and DB-dependent no-reset policy require played qualification before promising renewal. |
| ZSQ-PRISONB-08 | Mebme/Ximoto/Ximota are fort captives; Gatt/Jebira are ship captives. Ship key43020 differs from prison key43000. | A real rescue needs intended prisoner/controller, successful state or relocation, recipient, shared reset epoch and committed participants. Captivity prose, cell opening, prayer and combat alone are not native rescue receipts. |
| ZSQ-PRISONB-09 | Gixmo mentions paper and a magical spell but Q43 requires only five scales. Note43031 has no readable body and is ground stock in43048. | Explain actual inputs. Confirm whether paper is flavor or a planned readable clue; supply text or a designed prerequisite only in a separately named fix/news commit with recipe and balance review. |
| ZSQ-PRISONB-10 | Red dragon43006's full description says blue; black43007's says green. Several captive names/pronouns and prose have bounded copy errors. | Prepare separate text fix/news scope after builder confirmation of intended identities; keep native prototypes, stock, combat, gates and recipes intact. No native repair ships here. |
| ZSQ-PRISONB-11 | Four shared shipping crates43024 use closed/locked flags13 and key0;61 P rows supply food. P lookup uses get_obj_num of the container prototype. | Track actual parent container UID/location and successful unlock/pick/open/recovery. Confirm intended key/picking and food placement before a separate fix; food and note are not cloak inputs. |
| ZSQ-PRISONB-12 | No local literal special or ACT_TEACHER flag was found. Switches bind by type, imported359 binds epic_stone, and cloak/crown appear in chaos kit tables. | Maintain effective callback and source-category inventory. Lore about teaching is not an active quest. Alternate reward grants or generic equipment effects cannot manufacture the exact Q receipt. |
| ZSQ-PRISONB-13 | All52 rooms/108 exits and reset targets resolve. Room43051 describes a northern door/water but has only a south exit. Church prayer and study lore have no extra local Q. | Confirm whether the northern door is scenery or an unfinished route before separate topology/generator work. Add world-state/prayer/travel outcomes only for intended reachable mechanics; preserve valid current access. |

## Purple Worm: exact counts and effective spawn/combat authority

The [complete Purple Worm dossier](../design/zone-stories/LAIR_OF_THE_PURPLE_WORM.md) maps one family amulet plus seven identical hides to one return, eleven contacts and thirteen follow-ups. Preserve exact multiset quantities, supplied proof and spent-proof acceptance. Extend committed nested-source/container lineage, combat participants, real shared door/body arrival and current giver/reset availability. The65-percent giver row is skipped on ordinary resets but rolled on fresh boot/forced repop; qualify a separate global loader-probability fix. Clarify omitted hides, northeast mouth/body directions, loading/trap intent and bounded identity text in separately named repair/news scope. Preserve one achievement/one potential daily. New credit requires active, ready accounting; no native repair ships.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-PWORM-01 | Draknahov42506 accepts amulet42502 plus seven identical hides42500 for leggings42504; giver stays. | One accepted identity/achievement/potential daily. Preserve the exact multiset, aggregate loose quantity and spent-proof history; six hides, armor, equipment, nested proof or owned leggings cannot substitute. |
| ZSQ-PWORM-02 | Seven segments42505 G-stock hide42500/cap7. Final segment@42593 G-stocks corpse42503, with P-loaded amulet42502 and ring42515. | Add attributed committed source recovery, parent container UID, location, transfer lineage and qualifying combat participants. Supplied proof may fit the return without proving seven victories or first recovery. |
| ZSQ-PWORM-03 | Corpse42503 is ITEM_CONTAINER15, non-takeable, not ITEM_CORPSE14. Its amulet gives evidence of the missing brother. | Model actual inspection/retrieval separately from generated player-death corpse custody or living-prisoner rescue. No owned rescue or brother relocation is implemented by this Q. |
| ZSQ-PWORM-04 | Five M responses expose thirteen aliases. The amulet response announces an ending without consuming proof; dialogue omits seven required hides. | Keep targeted clue exposure distinct from acceptance. Propose a separate clue-text fix/news commit that explains the existing full recipe; do not silently remove hides or award each keyword. |
| ZSQ-PWORM-05 | Mouth42551 says northward, but D8 is northeast to42589. Body exits traverse east, southeast and south;42593SW returns to42554. | Prepare a bounded direction-text fix/news commit after builder confirmation. Observe actual successful arrival and one-way graph; do not add a north exit, swallowing gate or teleporter from prose. |
| ZSQ-PWORM-06 | Generic PW race combat calls PwormCombat for shriek/salivation/corrosive blast. Separate purple_worm swallow/death callback binds4480/700004/131232, not local425xx. | Inventory effective race and literal bindings. Do not confuse digestive combat, outside death/swallow or static body rooms with a local quest milestone. Future custom swallowing must identify actual source, actor and committed destination/death outcome. |
| ZSQ-PWORM-07 | Draknahov42506, mutated42508 and goblin42509 have cap1/chance65 M rows at42594. Ordinary M admission requires chance100; fresh boot/forced reset permits the probability roll. | Document unavailable ordinary renewal. Plan a separately scoped loader-probability fix with global cap/shop/forced/boot regression and balance review; do not make these mobs guaranteed or claim they can never appear. |
| ZSQ-PWORM-08 | RandomLoadRoom42594 has normal exits, several leading to exitless42595. UltraRareLoadRoom42596 routes toward42594 or42595. Names have no current automatic-dispersal flag or selected special. | Qualify intended placement and wandering/trap reachability before a separate topology/spawn repair. Present current availability and actual encounter rather than a promised normal giver location or safe loading-room route. |
| ZSQ-PWORM-09 | Entrance boulder42500N/42501S and breeding routes42510SW/42511NE,42563SE/42564NW use D1 closed/unlocked state. World state5 is masked to low door bits; no D4 secret reset exists. | Track successful shared open/search/pass with loaded state. Confirm whether secret wording describes scenery or intended active hidden-door behavior before a separate fix; no personal door history is enforced by Q. |
| ZSQ-PWORM-10 | Header42598/1/0/40/50/1 gives reset mode1 and difficulty1. Seven shared hides equal the required quantity; mode1 reset needs age and empty state. | Add authoritative stock/availability/reset epochs. Daily rollover is not restock; other players' retained stock and missed giver boot roll can prevent a fresh run. Preserve historical and potential-daily classifications. |
| ZSQ-PWORM-11 | All98 rooms/216 exits and88 reset commands resolve. Unused42597 is an unreferenced ID gap. Eight reciprocal outside edges and nine infant Alatorin groups are selected dependencies. | Keep graph validity separate from intended hidden/loading routes. Outside infants supply no quest hides; selected dependency closure does not make the outside zones comprehensive. No absent room should be created merely to fill numbering. |
| ZSQ-PWORM-12 | Bracer42507 describes a bone bracelet, boots42511 golden boots, earring42514 a golden stud, ring42515 a titanium keyword; mutated mob keyword is puprle. | Prepare a separate identity/clue-text fix/news scope after confirming intended prototypes. Preserve recipe IDs, stock, equipment and balance; these bounded caption mismatches are not extra quest inputs. |
| ZSQ-PWORM-13 | P handler chooses a container by prototype via get_obj_num; source parent grouping cannot establish immutable runtime corpse UID or nested item location. No local shop, ACT_TEACHER or numeric compiled binding exists. | Expand universal quantity, current availability, nested-source lineage and effective callback inventory. Reuse committed Q/custody/arrival authority; new custom outcomes need actual adapters. No native repair ships in this journal commit. |

## Vargan: SEARCH, quoted payment and accepted history

The [complete Vargan dossier](../design/zone-stories/VARGAN.md) maps two identical shoulders, one breast plate and C40000 to one historical armor exchange, eleven contacts and thirteen follow-ups. Hidden ground pieces require real shared SEARCH/recovery; current material readiness does not establish discovery or payment. The mixed item-and-currency offering is refused with accounting active and retains zero potential dailies. Extend owned quoted funds-plus-item settlement, source/reveal/container lineage and actual shared door/crawl/water/no-ground arrivals. Qualify old NPC pooling, secret-wall intent, spectator race and bounded direction text in separate fix/news scope. Preserve one achievement/zero potential dailies. New credit requires active, ready accounting; no native repair ships.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-VARGAN-01 | Norkon2104 accepts two shoulders2126, one breast plate2127 and C40000 for armor2128; giver stays. | Preserve one historical accepted identity/achievement, two optional quantity rows and zero potential dailies. The mixed offering remains unsupported; repeated shoulders, current money or owned armor do not manufacture acceptance. |
| ZSQ-VARGAN-02 | Shoulders2126 O-load at2127/2160 with shared cap2; breast2127 O-loads at2152/cap1. Both have ITEM_SECRET4096 and TAKE1. | Explain successful room SEARCH then pickup, current shared reveal state and exact prototype/quantity. Add owned first recovery/source/transfer lineage; ground stock is distinct from defeating nearby orogs. |
| ZSQ-VARGAN-03 | Full find_chance/do_search remove ITEM_SECRET only after a successful chance and visibility check; failure restores it. Ordinary actors stop after one finding. | Observe actual revealed UID, actor, source room/container, before/after flag and shared epoch. Failed SEARCH, repeated no-op or another player's reveal cannot become personal discovery; personal search is not a Q history gate. |
| ZSQ-VARGAN-04 | Durable offering selects distinct physical roots but refuses non-item goals; active accounting also refuses legacy coin/non-durable offerings. | Restore only through owned mixed settlement: exact quoted C40000 debit plus three actor-owned roots, reward obligation, continuation/replay/refund and crash recovery. Never remove the fee or suggest disabling accounting. Use a separately named fix/news commit. |
| ZSQ-VARGAN-05 | Inactive legacy quest_completion counts the NPC's pooled inventory and GET_MONEY(mob), then consumes items/payment. It does not identify each depositor. | Avoid attributing earlier players' pieces or the NPC's existing purse to the final actor. Restoration must capture the actual payer, source ownership, fixed recipe and reward recipient, with shared/partial-offering migration policy. |
| ZSQ-VARGAN-06 | Five M responses expose armor/betrayed/pieces/weld/hello/hi; three pieces and 40 platinum match the encoded recipe. The two shoulders share one prototype. | Explain exact two-plus-one composition and current refusal in player-facing availability. Words are leads, not separate achievements or an implemented welding mutation. Add actual action adapters only after owned settlement exists. |
| ZSQ-VARGAN-07 | World state5/key0 and D0 leave2127W↔2128E and2151E↔2152W open without selected D4 hidden flags. | Qualify intended secret-wall design before a separate flag/text repair. Preserve actual shared loaded state; do not require SEARCH of currently nonsecret doors or invent a key payment. |
| ZSQ-VARGAN-08 | Crawl rooms2115/2116 have SINGLE_FILE8192/NO_MOB4 and actual vertical exits. Forest/cave approach is ordinary graph. | Track successful entry, mounted/ordering policy and actual destination; crawling prose is not a low-ceiling sector gate. Generic arrival can expose contacts, without manufacturing personal traversal history. |
| ZSQ-VARGAN-09 | Scrag River uses underworld-water sector16. All room metadata is S; default current/fall fields have no local C/F configuration. Upper2190–2193 use no-ground18. | Preserve actual water/no-ground/flight/fall policy rather than inferring a configured current or mandatory boat from prose. Add attributable environmental travel only with successful authoritative outcomes. |
| ZSQ-VARGAN-10 | Forge2100 is a non-takeable container with flags5 closeable/closed and key0; P supplies flame2101/type0. Druid corpse2103 is static trash13. | Separate OPEN/container retrieval, flavor fire and scenery from forging or rescue. No flame, dagger, helmet or corpse is a Q input. Confirm any intended catalyst or rescue before separate design/repair work. |
| ZSQ-VARGAN-11 | Spectator2107's spherical/eye-stalk prose has native DK dragonkin race. Current BeholderCombat dispatch requires IS_BEHOLDER, not this race label. | Confirm intended race/combat identity before a separate balance-aware fix/news commit. Literal binding absence is not absence of generic racial/class effects; shield possession is not an armor receipt. |
| ZSQ-VARGAN-12 | Cave2107 north-exit caption says south; vertical crawl captions say south;2135 bend prose says east but exit is west;2182 corner prose says north but exits south. | Prepare a bounded direction-text correction after builder confirmation. Keep the valid graph, stock and balance; do not change topology to match copied prose. Native repairs remain separate from journal work. |
| ZSQ-VARGAN-13 | All94 rooms/239 exits/70 resets resolve; mode2 uses age-based resets and cap-limited pieces. Three outside rooms close six boundary edges; matching recipe inventory is local only. | Expose stock/service availability separately from history and daily eligibility. Selected Vargan II/surface closure is not comprehensive outside mapping. Thirty-five compiled numeric matches are namespaces/comments/tables, not selected quest bindings. |

## Maze of Undead Army: exact recipes and attributed access

The [complete Maze of Undead Army dossier](../design/zone-stories/MAZE_OF_UNDEAD_ARMY.md) maps all seven exact exchanges, ten progression contacts and seventeen follow-ups. Separate five lich choices, duplicate colors, two departing givers, hidden five-dust recovery, five-finger return and the golden/black-key route. Fresh reset stock is currently refused with accounting active; add durable world-generation ownership, explicit selected recipe intent, successful reveal/access/destruction authority and effective keeper/training availability. Qualify source caps, inaccessible carrier staging, follower replenishment and misattributed potion wording before separate repair/balance work. Preserve seven achievements/seven potential dailies; new credit requires active, ready accounting. No native repair ships.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-MAZEARE-01 | Seven native item-only Qs: five distinct three-piece lich combinations, five fingers for key94020 and five dust piles for potion94014. Pentagram and dust givers depart. | Preserve seven canonical receipts/achievements and seven potential dailies through the resettable zone, including departing givers. Retain duplicate prototypes as exact quantities, separate choices and spent-proof history. Potential eligibility does not guarantee live availability. |
| ZSQ-MAZEARE-02 | Q loading prepends completions. The durable offering scans that reverse order and selects the first matching full loose-inventory multiset containing the offered prototype. | Explain carrying only the intended recipe. Add an explicit selected recipe/giver quote and immutable owned-root intent for multi-choice services; journal selection and a matching keyword do not currently choose the reward. |
| ZSQ-MAZEARE-03 | Active accounting refuses A/O/P/G/E reset issuance before allocation and placement. All local ingredients, keys, sarcophagus contents and ordinary loot use those paths. | Restore through durable reset-generation ownership and replay-safe stock issuance, with no-loss refusal, caps, old world stock and restart policy. Keep accounting required; mark fresh-stock unavailability separately from accepted history. |
| ZSQ-MAZEARE-04 | Green94002/red94003/yellow94004 each have one G source and cap1; four lich recipes require repeated colors. G checks resident prototype count, with force-reset exceptions outside the active refusal. | Report stock constraints without declaring the recipes universally impossible. Qualify stored/nonresident/supplied copies, global ownership and repeated acquisition. Any cap change needs separate balance-aware fix/news scope. |
| ZSQ-MAZEARE-05 | Color carriers load at94117/94119/94121 with exits into the maze and into exitless94118. Those four rooms are unreachable by the ordinary directed graph from94177. | Guide players to current carrier whereabouts. Preserve intended staging/trap behavior; do not add player exits or assume personal access to spawn rooms. Qualify roaming, carrier death/corpse and source UID lineage. |
| ZSQ-MAZEARE-06 | Five hidden dust94001 O sources at94009/94013/94046/94062/94093, cap5. Actual Q113 belongs to Krugor94043, although its response names Kalroh. | Track actual SEARCH/reveal then recovery, failure/shared reveal and supplied proof. Correct recipient wording in a separately named text-fix/news commit after builder review; keep five quantities and Krugor departure. |
| ZSQ-MAZEARE-07 | Fingers94024 use one visible O source@94141 and four G sources: bard94010@94095, Kalroh94017@94115, bone dragon94039@94163 and conservator94041@94176; shared cap5. | Preserve five distinct owned roots. Separate ground recovery, corpse/theft/transfer provenance and encounter outcomes; no automatic SEARCH or personal-victory gate follows from the dialogue's secret-places wording. |
| ZSQ-MAZEARE-08 | Coordinator94035@94139 gives key94020 and stays. Its hand-healing text does not introduce another injury/cure field or a native custom quest mutation. | Record the accepted return once. A future personal cure/rescue adapter needs actual before/after state authority; do not infer durable healing from dialogue or key possession. |
| ZSQ-MAZEARE-09 | Graveyard94126DOWN→94137 has world state7/key94020: loader preserves pickproof and D6 adds secret/closed/locked. Blocking this edge isolates the five catacomb rooms and black-key source. | Explain successful SEARCH then UNLOCK/OPEN when still required, shared prior opens and supplied keys. Use attributed successful door/arrival state, not a mandatory personal finger-return gate for every player. |
| ZSQ-MAZEARE-10 | Black key94023 O-loads@94120. value1=100 makes a successful native unlock request key break; golden94020 has value1=0. UNLOCK changes shared state before its owned destruction publication completes. | Track actual key UID use and committed destruction separately from shared unlock; refusal/replay/disconnect must not fabricate consumption or erase prior return history. Qualify pending key-break behavior and any future atomic route action in separate fix scope. |
| ZSQ-MAZEARE-11 | Sarcophagus94019@94132 is non-takeable container15, flags29 closeable/closed/locked/pickproof and key94023. Four P sources provide rose, armplates, money and wrist band; band94025 is OTHER12. | Separate key readiness, actual unlock/open and contained retrieval with parent UID. Confirm opened-room prose and wrist-band type intent before any repair. P prototype lookup does not prove immutable runtime parent identity. |
| ZSQ-MAZEARE-12 | Twenty D records set real hidden wells/tombs/path/shaft entrances. SEARCH clears shared secrecy after successful chance; ordinary UNLOCK refuses still-secret doors. | Add actor/direction/shared-epoch reveal and successful access observers. Failed SEARCH, opening by someone else and repeated no-ops are different from personal first discovery or passage credit. |
| ZSQ-MAZEARE-13 | Lich94018 F1 follows freshly loaded Kalroh94017@94115. M cap refusal clears the follower parent; a departed lich is not guaranteed to restock while its master remains. | Expose keeper/follower availability and leader generation. Preserve the deliberate pentagram departure. Any independent follower-restock repair requires separate intent/balance review and fix/news treatment. |
| ZSQ-MAZEARE-14 | Full boot binds Kalroh through epic_teachers to Spellbind, denied by Enchant. The Spellbind epic_reward row is commented; epic_teacher returns false before pricing. No native ACT_TEACHER enables ordinary fallback. Active paid practice/purchases also refuse. | Describe current unavailable teaching separately from Qs. Confirm whether disabled teaching is intentional before a separate restoration. Add effective service availability, actor prerequisites/exclusions and committed skill/payment receipt adapters; registration alone proves no lesson. |
| ZSQ-MAZEARE-15 | Kalroh's class is ALCHEMIST; Krugor's encoded class is warrior despite alchemy prose. Full generic alchemist AI uses cached mixtures; its optional poison-vial spawn refuses with active accounting. | Preserve real combat/template behavior separately from dust94001, potion94014 or source stock. Confirm intended assistant class before balance work. Generated vial issuance needs owned world-generation authority and is not another native Q. |
| ZSQ-MAZEARE-16 | Repeated tunnel prose names directions absent from many actual exit sets; Dusty Tomb says sarcophagus open although its object is locked. Staging-room descriptions are plainly builder-facing. | Prepare bounded wording corrections using the valid graph and container state. Keep intentional staging and combat unchanged until reviewed. Every actual repair needs named fix/news scope with trigger, before/after and validation. |
| ZSQ-MAZEARE-17 | All177 rooms/395 exits/43 mobiles/25 objects/150 resets resolve. Mode1 needs an empty zone; one complete outside surface room closes two boundary edges. Seven matching recipes are local; no foreign stock/shops/imports/portals. | Keep source-comprehensive mapping distinct from played recovery and stock/service availability. Preserve all seven histories and original priority order. New credit requires active, ready accounting; daily rollover does not issue stock, restore a follower or reset personal history. |

## Bandit Camp: prisoner instances and cross-zone source proof

The [complete Bandit Camp dossier](../design/zone-stories/BANDIT_CAMP.md) maps four accepted returns, fourteen contacts and seventeen follow-ups: two rescue keys, the boy's chance-stocked ring and one insignia per paladin return. Distinguish GIVE recognition from physical release/escort, locked parent containers from their child keys, shared access/destruction from personal history, outside/supplied insignias from camp kills and diary lore from a verified campaign link. Fresh stock is refused with accounting active. Expand owned world-generation, prisoner-instance/action, parent custody, reading and availability adapters; qualify secret-reset/prose/weapon/lore intent before separate fixes. Preserve four achievements/four potential dailies and active, ready accounting for new credit. No native repair ships.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-BANDITCA-01 | Four item-only Qs: shackle key14831→E10000/depart, ring14826→eye protectors14827/stays, insignia14821→C5000/stays and key14828→E125000/depart. Mode2 permits all four potential dailies. | Preserve four canonical histories, exact one-item quantities and classifications. Base XP, actor/party frozen entitlements, coin payout and acceptance are separate; keeper/stock availability remains qualified. |
| ZSQ-BANDITCA-02 | Only paladin14820 has addressed M topics: hi/hello/quest/help/hail, boy, girl and kill. Four M/eight aliases differ from fourteen qc_action MA broadcasts. | Preserve dialogue exposure and independent endpoints. Ambient intervals, each keyword and the three-mission prose are not separate achievements, completed rescues or a mandatory chain. |
| ZSQ-BANDITCA-03 | Slave giver14807 M-loads in two pens14835/14845 under cap2. Guards14823 there E-hold key14831/cap2/100%; only the first also gets an insignia. One Q38 identity accepts the key and removes that group. | Add actor/item/source/prisoner-instance rescue authority if actual release/escort tracking is desired. Prototype history does not distinguish two prisoner UIDs, kill a guard, prove source recovery or authorize a second achievement branch. |
| ZSQ-BANDITCA-04 | Young woman14824@14843 directly accepts key14828 in Q174 and departs. No local shackle object, UNLOCK-NPC command, rescue callback or return-to-paladin endpoint is defined. | Explain the real GIVE endpoint. Add an explicit owned release/captive/escort adapter before recording physical unshackling or safe arrival; key readiness, NPC presence and response prose are insufficient. |
| ZSQ-BANDITCA-05 | Leader14833E↔14834W is locked D2/key0 and world type2/pickable; blocking the pair isolates14832/14833/14842/14843. Main gate14806N↔14914S is open type0. | Guide native access or shared prior opens without inventing a rescue-key, Isoril-kill or disguise-history gate. Confirm intended lock/stealth design before any route repair; attributed successful access needs shared-state epochs. |
| ZSQ-BANDITCA-06 | Desk14820 O-loads@14842/cap1; CONTAINER15/flags13 closeable/closed/locked/key0, without pickproof. P children are diary14822 and daughter key14828/cap1/100%. | Explain locked-container preparation and ordinary lockpicking/access possibilities. Qualify successful open/retrieval, exact parent UID and custody. The child shackle key does not unlock its own parent desk; do not silently remove a deliberate stealth gate. |
| ZSQ-BANDITCA-07 | Auctioneer chest14832@14864 is locked flags13/key14833; ring14826 P-loads/cap1/70%, alongside Calimshan currency14834. Auctioneer14814 carries key14833/cap1/100%. | Preserve chance-based availability, genuine contained retrieval and separate ring acceptance. An empty chest, coin pile, reward eye protectors or another player's recovery cannot fabricate the boy's return. |
| ZSQ-BANDITCA-08 | Steel key14833 value1=100 requests a break after successful native UNLOCK. Shared lock mutation precedes durable key-destruction publication; refusal can leave the key intact. Rescue keys have value1=0. | Keep key use, committed destruction, source retrieval and accepted quest history separate. Qualify refusal/replay/disconnect and atomic route actions in independent fix scope; losing a key does not erase an accepted return. |
| ZSQ-BANDITCA-09 | Twelve local E-insignia sources/ten carrier prototypes share cap12/100%; foreign leader5519@5508 in tharnadian_ruin G-loads the same14821 under cap1. Q158 wants one insignia, not a kill quota. | Preserve native acceptance of supplied/outside proof. Track actual carrier/source UID, equipment/corpse/theft/transfer lineage and personal combat independently. A same-named bandit or foreign copy is not evidence of the actor's camp victory. |
| ZSQ-BANDITCA-10 | Active accounting refuses A/O/P/G/E reset issuance before allocation/placement, including all local quest proof, containers, keys and ordinary loot. | Restore through durable world/reset-generation ownership, caps/chances, parent identity and no-loss replay/restart policy. Keep accounting required; existing owned proof/history is distinct from fresh guaranteed daily stock. |
| ZSQ-BANDITCA-11 | Three keys carry ITEM_NORENT; legacy save paths omit it. Current snapshot capture explicitly retains an ITEM_NORENT object with active durable custody. Rescue keys are quest-tagged; steel key is separate. | Do not promise automatic expiry or disappearance from the flag alone. Qualify intended transient-key lifetime, owned retirement and canonical recovery; any expiry must commit destruction without corrupting proof or historical receipts. |
| ZSQ-BANDITCA-12 | Auctioneer/ogre broadcasts describe bidding; four non-takeable gongs and guard prose describe alarms. No selected literal assignment, numeric callback or gong command implements a local purchase/release/alarm quest. | Keep context distinct from actual actions. Builder-designed auctions, alarms or ogre rescue need explicit owned settlement/NPC/world mutation adapters, effective callback inventory and availability before adding achievements. |
| ZSQ-BANDITCA-13 | Diary14822 names an escaped halfling, serpentine dagger and Mt. Sknak. Selected dungeon thief93001@93006 is a goblin carrying dagger93000/armor93001; Mountain's Gulik21025 wants armor93001 for key21027 and calls dagger destruction a rumor. | Treat the lore as an unbound cross-zone lead. Confirm identities/backstory/door effects before defining campaign edges; similarly named Shady locations, diary possession and rumors do not establish a verified downstream quest or read receipt. |
| ZSQ-BANDITCA-14 | Native guards include race-dependent aggression, ACT_HUNTER and leader/follower formations. Isoril carries imported wondrous mace67262 despite sword prose. No local ACT_TEACHER or numeric compiled binding is present. | Preserve generic combat and formations; slave-trader disguise advice is not guaranteed protection. Confirm weapon/prose intent before balance or text work. Absence of literal custom code does not remove generic class/AI behavior. |
| ZSQ-BANDITCA-15 | Tunnel world state5 is masked to door type1; its D1 resets close/unlock without setting secrecy. Copied room prose describes missing directions and a blocked main gate although its edges are open. | Prepare bounded state-aware wording corrections, or separately review intended secret-route design. Do not add topology or hidden prerequisites to satisfy prose. Actual changes need named fix/news scope with trigger, before/after and validation. |
| ZSQ-BANDITCA-16 | All127 rooms/311 exits/30 mobiles/36 objects/234 resets resolve. One imported object, two selected foreign reset groups, one ale-only foreign shop and three boundary edges are fully reviewed; no incoming portals or numeric compiled rows. | Retain complete selected closure and original priority order. The foreign Incarnate dispersal edge is context, not another local quest or ordinary player return route. No whole outside zone or downstream campaign is claimed comprehensive here. |
| ZSQ-BANDITCA-17 | Mode2 resets at its age even with players present; header flags0/min10/max15/difficulty1, with the server repop dial affecting lifespan. Givers/caps/chances/formation generations remain distinct from daily rollover. | Preserve four potential dailies while qualifying stock and giver availability. All new credit requires active, ready accounting; frozen recovery, repeated ambient messages and shared prior opens do not create new personal history. |

## Myrloch Vale: control blocks and native route choreography

The [complete Myrloch Vale dossier](../design/zone-stories/MYRLOCH_VALE.md) maps four returns, eleven contacts, six addressed aliases and seventeen follow-ups. Trace hidden proof, TOUCH shrine travel, alternate flaming-key stock, PULL switches, actual death opening and inherited floor loot separately from accepted history. Qualify the unstocked advertised stairway, asymmetric middle route and reverse-target safety before separate repairs. Expand owned action/participant/custody/reset availability and reuse committed epic-touch authority. Four achievements/four potential dailies remain; active, ready accounting is required. No native repair ships.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-MYRLOCH-01 | Master26411 accepts old key26417→flaming key26409/stays, bracelet26420→bracelet26421/stays and dagger26433→sphere26434/departs. Elder26417 accepts head26418→key26406/departs. | Preserve four exact independent canonical histories, achievements and potential dailies. Recommend leaving the dagger return last without fabricating a mandatory chain or another acceptance from its reward. |
| ZSQ-MYRLOCH-02 | Five raw M entries include one ambient qc_action and one qc_unblock control. The inventory lists four M entries/nine tokens; actual addressed topics are three M/six aliases. | Expose quest/quests/head/warlord/fire and chest only. Improve generic evidence typing for control, ambient and addressed blocks; do not expose qc_unblock, its room number or north as NPC topics or achievements. |
| ZSQ-MYRLOCH-03 | Horse26400@26413 and statues26401–26404@26425 are ITEM_TELEPORT25/CMD_TOUCH320, charges−1. Waterfall26407@26440 uses ENTER7 to outside26309. | Reuse check_item_teleport and successful char_to_room authority, not portal_door by type-name inference. Model object UID, selected command, source/destination and actual actor arrival. Appearance or an attempted touch is insufficient. |
| ZSQ-MYRLOCH-04 | Old bracelet26420@26471, dagger26433@26472 and key26417@26492 are hidden O stock/cap1/100%. SEARCH can reveal an item subject to chance/visibility and shared prior state. | Add attributed successful reveal/retrieval with item/source UID and shared epochs. Current loose proof may be supplied, transferred or revealed by another actor; it cannot fabricate personal SEARCH, first acquisition or own victory. |
| ZSQ-MYRLOCH-05 | Red flame26413@26480 G-loads flaming key26409. Purple dragons26400@26514/26515 carry jade26412/tiny26443; quartz26401@26530 carries brimstone26411. All six key prototypes use100% break values/NORENT. | Preserve alternate key sources and exact door/container roles. Successful unlock precedes durable destruction; refusals/replay and current durable NORENT custody need qualification. A reward key is not an old-key receipt or guaranteed expiry. |
| ZSQ-MYRLOCH-06 | Four hidden floor switches26413–26416 use PULL340 and open26540N,26534E,26534N,26540W respectively. Directed reset-stage closure reaches them in order26414,26415,26416,26413. | Add successful switch effects with actor/object/target/old and new state, existing shared opens and reset epochs. Do not make all four toggles prerequisites for the head exchange or credit each attempt as an achievement. |
| ZSQ-MYRLOCH-07 | item_switch validates the near exit but clears a nonsecret reverse exit without a null/identity guard. All four selected switch targets have correct reciprocal exits. | Prepare a separately named generic safety fix for malformed reverse targets and adverse-path regression. Preserve this zone's working effects and choreography; no selected reverse-target repair is justified here. |
| ZSQ-MYRLOCH-08 | Middle edges are asymmetric:26539N→26545,26545S→26540. After the first two switches, the source graph can reach the warlord through that approach. | Confirm builder intent before changing topology or requiring both farther switches. zcheck's live-switch-only warning also needs effective qc_unblock/compiled callback qualification; its caveat is not proof that the warlord wall is broken. |
| ZSQ-MYRLOCH-09 | Warlord26402 has ACT_SPEC_DIE and a registered unblock_on_death; read_mobile adds ACT_SPEC. die dispatches CMD_DEATH, and qc_unblock26555 north clears only that near wall. The callback ignores the killer argument. | Capture NPC/item identities and qualified participants before teardown, and publish actual death/opening separately from encounter or accepted head proof. Shared access must not imply the observing player killed the warlord. |
| ZSQ-MYRLOCH-10 | This special death branch bypasses ordinary make_corpse, then ordinary NPC extract_char can drop nontransient equipment/inventory onto the room floor. Head26418, shield26438, stone358 and memory55455 are intended stock. | Support inherited floor-loot provenance as well as corpse loot, before NPC UID retirement. Do not misdiagnose the absence of a corpse as missing loot or introduce a double-corpse fix; durable custody/refusal/recovery remains to qualify. |
| ZSQ-MYRLOCH-11 | Elder chest26405@26434 has flags31/locked/pickproof/key26406; P children are artifact26419 and harp pin26550. Head acceptance removes the elder but defines no actual altar mutation. | Explain the real GIVE endpoint and subsequent key/open/contained retrieval. Add committed parent custody and action outcomes before recording sacrifice, chest opening or reward-child recovery from farewell prose. |
| ZSQ-MYRLOCH-12 | Return stairway26437→26425 exists as a hidden ENTER teleport prototype, but no local or selected foreign reset group stocks it. Chamber26559 has an ordinary south return;26467UP returns to the shrine. | Confirm whether to restore or retire the advertised shortcut in a separately named fix/news commit. Preserve existing backtracking, route balance and active accounting; do not silently add a producer or claim the entire return route is absent. |
| ZSQ-MYRLOCH-13 | Warlord shield26438 has a separate caves_skelenak Q155, blind monk4038@4153→reward4029. Selected giver/room/reset/recipe are reviewed. | Keep this outside lead independent from Myrloch head acceptance and personal victory. A verified campaign link needs explicit endpoint design; the whole outside zone is not remapped by this dependency review. |
| ZSQ-MYRLOCH-14 | Imported rune stone358 is explicitly bound to epic_stone. Its current touch uses zone_touch_transaction_submit, qualified participant awards, stone UID and recorded zone authority. | Reuse committed epic-touch evidence for a future optional journal adapter; item acquisition, touching a powerless stone and a native Q receipt remain separate. Preserve existing payout/eligibility and replay authority. |
| ZSQ-MYRLOCH-15 | Memory55455 is quest-tagged/_noquest_; generic world reward policy withholds handcrafted proof/reward and excluded item kinds. Four actual shops and inn26566 have effective bindings, without local quest-proof sales. | Distinguish souvenir, paid service and persistence from native story completion. Do not infer a memory exchange or paid ability from item names; active durable NORENT payloads are not guaranteed to disappear on rent. |
| ZSQ-MYRLOCH-16 | Active reset_zone refuses A/O/P/G/E before allocation, including proof, statues, switches, containers and loot. Header26578/1/0/19/29/1 means empty-zone mode1, flags0, lifespan19–29/difficulty1. | Restore durable world/reset-generation issuance with caps, parent custody and no-loss replay/restart. Keep active, ready accounting required. Fresh route availability and departed givers cannot be promised merely by daily rollover. |
| ZSQ-MYRLOCH-17 | Full178 rooms/373 exits/18 mobiles/45 objects/184 resets/249 raw reset lines/82 QST lines resolve. Four shops, two imports, two boundary edges, six stocked teleports and no selected foreign reset groups are qualified. | Preserve complete source scope and original priority order. Record neutral wording/shortcut/route and generic safety proposals with explicit intent and named fix/news scope if changed. No native repair ships; source closure does not prove played stock or action journeys. |

## Cloud Giant Kingdom: exact sets and effective service bindings

The [complete Cloud Giant Kingdom dossier](../design/zone-stories/CLOUD_GIANT_KINGDOM.md) maps three returns, seventeen contacts, thirteen addressed aliases and eighteen follow-ups. Preserve count5 pelt preparation, six distinct scalp prototypes, alternative F/E/G carriers, actual key access and shared hidden routes. Nonstros legacy scroll and separately bound Empower Song training need different authority; qualify tablet availability, reward effect and owned service settlement. Three achievements/three potential dailies remain; active, ready accounting is required. No native repair ships.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-CLDGT-01 | Trader99501 Q7 consumes five distinct I99509 instances plus strap99510→girdle99503/stays. Anne99520 Q36 consumes one each99518–99523→tiara99516/departs. Nonstros99548 Q68 consumes32490/26614/402→405/stays. | Preserve three canonical histories/achievements/potential dailies. Group repeated pelts as count5 preparation while retaining the native multiset; six same-name scalps are different prototypes, not six rewards. |
| ZSQ-CLDGT-02 | Six raw M include two Nonstros qc_action echoes. Four addressed M expose13 aliases: trader yeti, leader giants/city/giant/cloud, Anne hi/hello/greetings/salutations/scalp/evils/evil/revenge. | Expose only actual addressed dialogue. Hearing an ambient echo, asking each alias or encountering a source carrier must not create an accepted receipt or extra achievement. |
| ZSQ-CLDGT-03 | Pelts are G stock on99503@99555/99560/99578 and99505@99574/99576; each uses global cap5/chance100. Strap is E99510 on99507@99561/slot13/cap1; three other warriors lack it. | Model exact per-instance source, distinct item UIDs, caps and committed recovery. Stock chance is conditional on availability; five carriers do not promise five new items under active accounting. |
| ZSQ-CLDGT-04 | Brass key99505 G-loads on quiet farmer99516@99518 and leader99508@99562/shared cap2. Leader99562 is reachable before boulder99538N↔99561S. | Explain both valid key leads and shared existing opens. Personal key receipt is optional route preparation, not an invented native exchange gate. |
| ZSQ-CLDGT-05 | Boulder world types3/6 share99505/D2. setup_dir masks low2 bits, making the near side pickproof and reverse pickable. Key value1=100; successful unlock can consume it. | Qualify successful unlock, picking, key destruction and shared epochs separately from attempts. Confirm intended side asymmetry before any named repair; do not interpret raw6 as live reset state. |
| ZSQ-CLDGT-06 | Secret closed D5 down routes99584→99585,99586→99587,99589→99590 lead to scalp carriers; reverse UP uses D1. Notes99525/99526/99531 provide graveyard/dis/quiet clues. | Add actor-attributed successful reveal/open/arrival and current shared state before recording SEARCH or note-reading progression. Clues do not force an ordered conversation or personal search chain. |
| ZSQ-CLDGT-07 | Duergar99524@99585 G99518; troll99525 F and diseased farmer99517@99516 G99519; ogre99527 F G99520; orc99529@99587 G99523; drow99528 F and Gilmorock99522@99580 E99521/slot18; illithid99526@99590 G99522. | Preserve parent-aware F/E/G source identities and alternative carriers. Handcrafted scalp prototypes are not arbitrary PvP scalps, and current supplies do not establish personal kills or first acquisition. |
| ZSQ-CLDGT-08 | All six scalp prototypes have ITEM_SECRET; duergar also NOLOCATE. Strap is wearable; orb/other proof can be held or worn; Anne is a departing giver. | Loose preparation needs exact current custody/counts. Visibility, equipped recovery and source/supplied lineage need owned outcomes; secret flags alone do not prove a mandatory personal SEARCH. Preserve accepted history after spending proof or giver departure. |
| ZSQ-CLDGT-09 | Heart32490 is G stock on Bel32420@32469/cap1. Orb26614 is E stock on Dark Prince26642@26859/slot16/cap1. Complete selected source groups and bodies are reviewed. | Keep bounded outside recovery leads separate from mandatory campaign or own boss-victory credit. Capture source/actor/item identity before death or transfer; fresh outside stock remains refused by active reset issuance. |
| ZSQ-CLDGT-10 | No current tablet402 producer appears in the reviewed reset/compiled inventory. Tablet prose asks for an arbitrary artifact, while Nonstros's actual Q takes only three exact I inputs. | Confirm a supported modern starter and intended cost, then restore or retire the legacy route in separately named fix/news scope. Existing owned/staff/supplied copies are not disproved; do not silently issue a tablet or add a fourth artifact requirement. |
| ZSQ-CLDGT-11 | Charisma scroll405 is ITEM_TRASH13 with zero values and no selected effect binding. READ delegates to look. | Confirm intended reward/effect, then separately repair or retire with before/after and balance qualification. Current item receipt or description cannot promise a stat increase or Empower Song training. |
| ZSQ-CLDGT-12 | Normal non-mini epic_initialization binds epic_teacher to99548 using the teacher table. Its Empower Song service has dynamic class/level/progression prices and refuses active accounting before submission. Primary callbacks dispatch independently of qst_func and without an ACT_SPEC prerequisite. | Inventory effective startup bindings as well as literal assignments. Add owned epics/coin/learned-skill settlement, persistence/replay/refund and an actual learned-skill journal kind before integrating training. Do not diagnose a missing binding or enable free purchases here. |
| ZSQ-CLDGT-13 | Anne carries ACT_TEACHER32768; read_mobile installs the separate teacher fallback. ASK level uses class/runestone guidance, and ordinary FindTeacher can select visible flagged NPCs. | Keep service eligibility/guidance separate from her addressed revenge aliases and Q receipt. A future teaching adapter must qualify the actual successful skill outcome; no extra learned step or unconditional teaching promise ships. |
| ZSQ-CLDGT-14 | Ten outside selected recipes consume these legacy ingredients independently, including eight other tablet exchanges, Negative Plane's orb return and Belial's heart return. | Explain competing ingredients without a synthetic prerequisite chain. Current supplies, previous unrelated rewards and existing comprehensive outside dossiers retain separate identities. |
| ZSQ-CLDGT-15 | One real shop99501 sells99513/99514/99515; two merchant names lack a local shop binding. Statue99524 is non-takeable trash without a selected travel effect; horn/forging/raid lore has no selected local quest action. Sleeves99538 also appear in two Chaos druid kits. | Keep services, décor, stock and mode-specific grants distinct from quest proof. Confirm lore expectations before designing additional owned outcomes or changing native zones. |
| ZSQ-CLDGT-16 | Actual NO_GROUND sector8 exists only99591/99595/99600. Several air-themed rooms instead retain indoor sector/copied village prose. Royal temple99653–99659 has ROOM_HEAL. | Confirm builder intent before separate prose/sector fixes, since sector changes affect flight/access balance. Successful movement and healing are distinct from names or ambient descriptions. |
| ZSQ-CLDGT-17 | Header99680/1/0/25/30/1 uses empty-zone mode1 and the repop dial. Active reset_zone refuses A/O/P/G/E before allocation, including forced resets. | Add owned reset generations, caps/chances, equipped/contained parents and no-loss restart/replay before restoring stock. Daily rollover does not restock proof, unlock doors or return Anne; active accounting remains required for new credit. |
| ZSQ-CLDGT-18 | Complete181-room/391-exit closure reaches every local room when ordinary gates are assumed available; removing both boulder sides leaves90 rooms inaccessible. All local active identities resolve. | Preserve complete source review separately from played qualification. Record missing tablet/effect, service settlement and possible builder intent fixes in named follow-ups; no native repair or balance change ships here. |

## The Mountain of the Banished: spoken words, typed sources and owned cleanup

The [complete Mountain of the Banished dossier](../design/zone-stories/THE_MOUNTAIN_OF_THE_BANISHED.md) maps three returns, fourteen contacts, twenty-nine addressed aliases and eighteen follow-ups. Keep exact floor warangel/mirror/four-essence proof, two reward items and two priest instances under independent canonical histories. Trace spoken Pandora/Lokpan unlocking and six ENTER objects separately from shared access, lore and accepted credit. Plan separately named extraction-loop/R safety, clerical and NPC-circulation fixes; integrate owned source/transfer/action/service/retirement authority before extra objectives. Three achievements/three potential dailies remain; active, ready accounting is required. No native repair ships.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-MOUNT-01 | Torturer9107 Q11:9106→9109; wildmage9123 Q31:9107→9108; priest9136 Q56:9117/9118/9119/9120→9132/9135. All stay. | Preserve three exact canonical histories/achievements/potential dailies. One four-input/two-reward exchange is one receipt; partial preparation and reward possession must not split or duplicate it. |
| ZSQ-MOUNT-02 | Four addressed M expose29 aliases. Wildmage broom/three beetles are wager stakes; prophecy, spawning crack and reincarnation are lore without a selected action effect. | Explain actual objectives and native snowglobe reward. Future spellcasting, sabotage, resurrection or escape stories need actual owned adapters, not keyword counting or inference from dialogue. |
| ZSQ-MOUNT-03 | Battered warangel9106 is TAKE/HOLD ITEM_TRASH O stock@9145/cap1/100%, beside interrogator9112. | Support committed floor recovery and exact item UID/custody. Keep the live warangel NPC, ordinary corpse, capture/escort and supplied proof distinct; do not invent a mandatory live-prisoner action. |
| ZSQ-MOUNT-04 | Mirror case9107 is G stock on concubine9124@9123/cap1/100%, with TAKE/HOLD. Accepting wildmage9123@9152 differs from other wildmage prototypes. | Keep actual source and giver identity distinct. Source recovery/transfer and holding/loose states need owned evidence before first-acquisition credit. No extra broom or beetle grant is established. |
| ZSQ-MOUNT-05 | Hidden essences9117/9118/9119/9120 are G stock on Tolog9110@9141, Pakar9114@9147, Zooox9125@9155 and Lokpan9119@9150/cap1/100%. | Preserve four exact material types and source identities. Supplied proof may fit native acceptance without personal kills, first recovery or an observed spawning-room malfunction. |
| ZSQ-MOUNT-06 | Peak9112D uses world7/key−2/Pandora/D2; reverse9113U uses1/key0/D1. Slab9113W↔9158E uses3/key−2/Lokpan/D2. Notes9127/9128 are ITEM_NOTE/TRASH, not ITEM_KEY. | Use actual say→check_magic_doors authority; its last keyword unlocks/clears secret flags, with reciprocal identity checks. Collecting or reading notes is optional clue discovery, not a required key transaction or accepted quest. |
| ZSQ-MOUNT-07 | Successful speech can unlock a shared door but does not clear EX_CLOSED. Silenced, underwater, suppressed, wraith or otherwise speech-refused attempts do not reach the hook. | Add actor/room/near and reciprocal exit/old-new state/reset epoch evidence, then distinct successful open and arrival. Already-open access or another actor's speech cannot fabricate one's own unlock. |
| ZSQ-MOUNT-08 | Six non-takeable ITEM_TELEPORT25 ENTER7/charges−1 objects pair9103↔9114,9138↔9139 and9158↔9159. | Use check_item_teleport→teleport_to→char_to_room, with selected UID/command/destination and actual arrival. Do not infer portal_door binding or a direct escape from their names; no outside incoming teleport is selected. |
| ZSQ-MOUNT-09 | Priest9136 M-loads@9100/9163/shared cap2, with F bodyguards/high-mages. Both copies use one canonical Q. | Keep NPC instance/circulation and offering participants separate from giver-prototype history. Meeting or paying both copies does not create a second achievement; NPC follower relations do not award group completion automatically. |
| ZSQ-MOUNT-10 | Imported fountain72 O@9147/cap100 is bound to spell_pool. DRINK invokes one of nine spells, with shared selection rotating after40 minutes. | Model actual effect, recipient, source UID and shared service state before recording a buff visit. The current callback ignores its argument, so a future objective also needs explicit target qualification; no new daily or mandatory buff is added. |
| ZSQ-MOUNT-11 | Imported359 G@Tolog is bound to epic_stone and submits qualified zone_touch participants/awards. Memory55444 is _noquest_/quest-tagged. | Reuse committed touch authority for a future optional action. Current possession, local essences and memory are separate. Preserve source/participant freeze before custody changes and existing reward exclusions. |
| ZSQ-MOUNT-12 | Imported potion371 O@9155 has APPLY_LEVEL8/46. epic_stone_absorb only extracts smaller stones or level potions; it does not increase level/payout, and dereferences extracted tobj again or at loop advance. extract_obj releases object memory. | Prepare a separately named generic safety fix: retain next identity before extraction, avoid subsequent access, and test multiple eligible items. Qualify owned durable retirement/replay before enabling any absorption objective or promising an upgrade; builder intent for obsolete potion stock remains open. |
| ZSQ-MOUNT-13 | Pakar's selected R9128@9147/cap1/100% creates a ridden griffon. Generic R chance-refusal sets mob=null but can continue to dereference it when last_mob exists. The selected100% source does not take that random-refusal path. | Prepare a separate null/refusal safety fix with failed-roll/read/limit and valid-rider regression. Preserve this working relationship and item-issuance guard; R is a mount relationship, not random-room placement or another quest grant. |
| ZSQ-MOUNT-14 | Two outside incoming load rooms71326D→9122 and87673D→9106 stock Tallin bodyguard71248 and Dinok87602; their proofs71228/87588 have independent outside returns to71236/87598. | Preserve bounded cross-zone NPC source/circulation and typed proof lineage. These outside essences do not replace any of the four local ones or create a mandatory campaign. Whole outside areas are not newly claimed comprehensive. |
| ZSQ-MOUNT-15 | Object71248 is a necklace; mobile71248 is Tallin bodyguard. The selected four-ring outside reward R I71248 is an item, despite the shared numeric ID. | Inventory references must retain item/mobile/room namespace and outcome kind. Do not treat that reward as a companion grant or reuse numeric overlap as prerequisite evidence. |
| ZSQ-MOUNT-16 | Demonic mask9109 extra description refers to sleeves; other lore contains clerical wording issues, without changing actual prototype/recipe identities. | Consider a separately named text fix identifying actual mask behavior and clarifying lore. Do not alter reward stats, source flags or requirements as part of a journal wording correction. |
| ZSQ-MOUNT-17 | Header9166/1/0/40/50/1 uses empty-zone reset mode1/repop dial. Active reset_zone refuses A/O/P/G/E before allocation; M/F/R and D follow their own conditions. | Restore stock only with owned generations, caps/chances, floor/carried/equipped/parent identity and no-loss restart/replay. Daily rollover does not stock proof/clues/travel/services or unlock words; new credit continues to require active, ready accounting. |
| ZSQ-MOUNT-18 | Source entrance graph with stocked ENTER objects and ordinary gates reaches64/67 rooms;9157/9165/9166 are administrative load/holding rooms. Native9157N→9166 can admit eligible wandering NPCs into an exitless room. Numeric scan yields two real kit references and three decimal false hits. | Confirm holding/circulation intent before separately changing routes or sentinel behavior. Do not add administrative exploration requirements or a speculative player exit. Improve effective typed reference inventory and keep complete source review distinct from played qualification. |

## Tiamat: same-name types, addressed echoes and owned death outcomes

The [complete Tiamat dossier](../design/zone-stories/TIAMAT.md) maps eight returns, seventeen contacts, ten addressed aliases and twenty follow-ups. Explain the seven-key route while preserving exact same-name fragment types, parent F/G stock and borrowed/shared access. Keep the cadaver two-item tribute separate from timed heart, spirit, slave rare rolls and services. Owned death issuance/retirement, MA actor/audience, key-break/arrival and timed replacement require distinct authority. Inactive potion/dormant throne and producer/death safety have separately named repair plans. Eight achievements/eight potential dailies remain despite mode0; active, ready accounting is required.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-TIAMAT-01 | Eight Q14/19/25/30/36/41/47/53 share giver19616@19611; seven keys plus one two-item tribute. | Preserve eight canonical histories/achievements/potential dailies. Distinguish partial multiset readiness from acceptance and one two-reward receipt. |
| ZSQ-TIAMAT-02 | Two MA with seven aliases and one M with three aliases are addressed ASK/TELL replies; MA echoes to the room. | Carry actor, actual target and audience in dialogue evidence. A listener must not inherit another actor conversation credit; aliases are not separate achievements. |
| ZSQ-TIAMAT-03 | Two M19600 golems@19600 block east through block_dir; dark key19601 G on selected worshiper19601@19612. | Use actual blocker presence/retirement and arrival. Borrowed key or already-clear gate can provide access without personal kill or crafting history. |
| ZSQ-TIAMAT-04 | Dark→bright→ruby→golden→jeweled→bronze→red→green is the ordinary source key route. Crystal19606 guards vault entry. | Explain the chain without imposing false historical gates. Shared opens and supplied keys remain valid; model successful SEARCH/unlock/open/arrival separately. |
| ZSQ-TIAMAT-05 | Eighteen fragment prototypes share generic or ruby-fragment names. Seven exact sets require2/3/2/3/2/3/3 types. | Use distinct readable fragment A/B/C labels and exact item identities, with source hints. Do not substitute duplicate fragments or matching dragon colors. |
| ZSQ-TIAMAT-06 | F green/white copies hold different G fragments; black/white/green prototypes recur across chambers. | Retain parent instance, reset group, room, cap and chance in source lineage. A prototype alone cannot identify which stock or first acquisition occurred. |
| ZSQ-TIAMAT-07 | Assembled keys19602–19605/19611–19613 and crystal19606 have break value100; starter dark19601 has0. | Preserve native break behavior. Add durable retirement and reciprocal door outcomes before route credit; spent keys must not erase recorded assembly. |
| ZSQ-TIAMAT-08 | Imported boss19700 from astral_main has local M@19617/cap2; cadaver19641 G33% and spirit19642 G10% are separate. | Bind source identity to the actual living instance and selected roll. Do not turn a boss sighting, spirit or random equipment into an accepted cadaver return. |
| ZSQ-TIAMAT-09 | Full tiamat CMD_DEATH moves carried/equipped stock to floor and creates timed heart55080; IS_IMMOBILE precedes the death branch and heart allocation is unchecked. | Plan a separate null-result/death-order safety review with valid, missing-template and immobilized-death cases. Preserve floor stock and balance; freeze owned participants/source/outcome before new credit. |
| ZSQ-TIAMAT-10 | read_object creates a UID candidate; legacy boss/slave callbacks allocate and place without an owned death-outcome transaction. Reset issuance guards do not by themselves cover callbacks. | Add owned creation/admission, source episode, floor custody, restart/replay and effect settlement. Do not claim all creation is refused or all generated loot is durably admitted. |
| ZSQ-TIAMAT-11 | Heart55080/_noquest_ has three-mud-day initialization and dragon_heart_decay replaces it across floor/carried/worn/nested custody. | Keep it separate from cadaver proof. Model owned countdown, replacement/retirement and observed deadline; qualify failed replacement retry before a timed story objective. |
| ZSQ-TIAMAT-12 | Slave19617@19624 selects19911/19916/19637/19638 on CMD_DEATH, writes value0 and returns FALSE; death dispatch ignores that result, while later extraction drops eligible stock. | Treat this as a native death lead without an accepted rescue recipe. Qualify actual source/circulation, roll and floor recovery; do not infer a generic one-day expiry from value0 alone. |
| ZSQ-TIAMAT-13 | Wisdom pool66@19620 binds stat_pool_wis/common: PC level51, TAG_POOL two-day restriction, healing and bounded base-stat adjustment. | A future service step needs actual admission, recipient, old/new stat and stored cooldown. Refused or harmful outcomes and other actors drinks must not earn successful-benefit credit. |
| ZSQ-TIAMAT-14 | Monolith360@19620 binds epic_stone with existing committed zone_touch authority. | Reuse committed participant/award evidence for optional touch, separately from crafting. Possession or pre-submit echoes are not a touch receipt. |
| ZSQ-TIAMAT-15 | Cloak19916 binds artifact_hide; ioun911 binds artifact_stone; shield19638 binds absorption; stinger51006 binds its combat proc. Chaos kits can also supply shield19638. | Keep actual powers and worn/effect qualification separate from source recovery. A kit, transfer or rare roll copy cannot establish personal slave or boss history. |
| ZSQ-TIAMAT-16 | O25106@19632 is missing from active registry, though brass-old-3.obj contains an archived level52 potion. renum_zone_table disables the unresolved command. | Plan an explicit inactive-reference cleanup or replacement only after builder intent. Do not reactivate the archived area or add a new potion benefit as a journal repair. |
| ZSQ-TIAMAT-17 | Bounded random-zone branch also reads25106 then uses the returned pointer without a null check; another selected relic branch has a guard. | Plan a separately named producer null/admission safety fix and failed-load regression. Full outside random-zone campaigns are not newly claimed comprehensive. |
| ZSQ-TIAMAT-18 | Disabled TiamatThrone points west to missing19623; ordinary19617W currently leads19622. Peace alcove19639 and fifteen load/wander rooms lack ordinary entrance reachability. | Confirm intended obsolete route and administrative roles before a separately named route/data fix. Do not enable the disabled procedure, fabricate an exit or add impossible exploration requirements. |
| ZSQ-TIAMAT-19 | Header19639/0/0/50/60/6 uses no automatic reset; source graph reaches23/39 with all ordinary gates available. Existing catalog still has eight potential dailies. | Qualify owned stock generations, reset/touch/manual episodes, chances/caps and availability before daily assignment. Daily rollover does not create fresh fragments, boss stock or services. |
| ZSQ-TIAMAT-20 | Nine boundary edges include Avernus entrances and vault/load-room exits; incoming pool19907 O@19902/O@19949 enters19600. Local barb19607 has an outside vault O25% source. | Keep cross-zone access, actual ENTER arrival and typed outside item lineage distinct. The barb name does not prove it was recovered from a green wyrm; only bounded external sources are reviewed. |

## Obsidian Citadel: exact fees and custom prerequisites

The [complete Obsidian Citadel dossier](../design/zone-stories/THE_OBSIDIAN_CITADEL.md) maps five returns, seventeen contacts and twenty follow-ups. Mixed-fee commissions are currently refused while accounting is active until owned mixed payment is supported. Keep the three commissions independent; distinguish follower shard stock, Rolart corporeal/shade identities and supplied proof. Authoritative current-coin readiness is missing and planned; fees stay in textual hints. CACKLE bookshelf access, exact non-key gate identities, source summon episodes and effective startup services need separate outcome authority. Active accounting refuses legacy training/ore forging; normal-PC ascension is currently gated out. Record separate forging correctness, ascension eligibility and trap/narrative intent repair plans. Preserve five achievements/two potential dailies and require active, ready accounting.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-OBCITA-01 | Five Q13/27/37/47/68 accept staff, three shard-and-fee commissions and Rolart shield. | Preserve five histories/achievements and two potential dailies; three departing commissions remain independent, with no sequential requirement to finish all at one instance. |
| ZSQ-OBCITA-02 | Three addressed M records have17 aliases; vampire and necromancer offers differ from Rolart shade. | Carry actor, actual target and successful reply evidence. Topics reveal guidance without creating17 achievements or a listener history. |
| ZSQ-OBCITA-03 | M75603@75622/G75618, M75613@75636/G75607, F75618@75668/G75620; all proof caps1/chance100. | Keep minuscule/small/large types exact and retain leader/follower/reset source episodes. A transferred shard is material readiness without first-source history. |
| ZSQ-OBCITA-04 | Cleric M75602@75610/E75602; Rolart M75615@75699/E75641. Same numeric values also name other typed entities. | Distinguish mobile/item/room namespaces, actual equipped proof and corpse/floor custody. An unrelated shield, knight or current item does not prove personal victory. |
| ZSQ-OBCITA-05 | Vampire commissions require C10000/C20000/C100000 with exact shard, then D departure. | Mixed item/coin offerings are currently refused under active accounting: durable offering admission accepts only item goals. This is the present daily exclusion, not departure alone; reset mode1 retains repeatability. Add owned atomic payment/reward/retirement settlement and a universal authoritative current-coin readiness field. Stable summaries show exact fees and current refusal even when a ready item hides its hint. Structured service-availability display remains a capability gap. Partial item/coin readiness must not debit money or retire the giver. |
| ZSQ-OBCITA-06 | Necromancer speaks of beating reconsecration; shade says goodbye without D. | These are narrative leads, not an implemented deadline or disappearance. Confirm builder intent before a separate timer/retirement repair; freeze deadline and actual transition for future credit. |
| ZSQ-OBCITA-07 | Shelf75660 O@75768 and orb75661 O@75762 are type25, command26/CACKLE, fixed destinations75762/75768, charges−1. | Use actual addressed object and successful arrival, separately from lore or social text. Ordinary shelf75622 shares the keyword; keep instance selection and availability visible without inventing READ/ENTER requirements. |
| ZSQ-OBCITA-08 | Source graph reaches147/179 without keys,178 with valid gate items,179 with CACKLE; exact key0 boss trapdoor lacks a source key route. | Preserve the supported bookshelf bypass and shared access. Graph qualification assumes SEARCH/open/supplied items and excludes picking/magic; it is not played proof or an all-room achievement. |
| ZSQ-OBCITA-09 | Ancient key75630 O@75669 opens library store; keystone75631 is type27/TAKE and matches chest75632 key field; note75623 hints at stone. | has_key uses held/loose exact identity without ITEM_KEY filtering. Do not label the keystone impossible because of its type; qualify custody, non-key uses and chest outcome before new access credit. |
| ZSQ-OBCITA-10 | Opposite holy-room doors reference75627 small key and75628 torc; Kyrir stocks both. Chest75643 uses distinct key75656. | Record asymmetric exact identities and reciprocal unlocking; do not normalize them speculatively. Keys75627/75630/75656 have break100; preserve historical proof after retirement. |
| ZSQ-OBCITA-11 | Armory key75634 belongs to one selected barracks guardian; cell key75640 to two livestock guardians; first-floor75654 to Rileas; hoard75644 to Kaketkralix. | Explain current access without requiring invented personal kills or prior receipts. Retain selected instance, cap, paired-door and container relationships. |
| ZSQ-OBCITA-12 | Satar75640@75762 binds full summon handler; helper creates75648, max2 followers. Three M75648@75756 reset mobs are separate. | Use boss encounter/source episode, actual summon generation, participant and retirement authority; avoid farmable prototype-count credit. NPC master makes setup_pet charm duration−1 despite helper duration input. |
| ZSQ-OBCITA-13 | Nine active death-knight bindings75631–75639; Rolart75615 combat assignment commented, but epic_initialization assigns epic_teacher. | Inventory effective startup table bindings and distinguish PRACTICE from combat callbacks. Expert Riposte purchase refuses active accounting; no successful training objective is currently promised. |
| ZSQ-OBCITA-14 | Undead smith75628@75727 receives smith through initialize_tradeskills; shard giver is75614. FORGE refuses active accounting. | Keep optional ore service separate. Future owned forging needs payer material selection, price, grant, consumption, refund and replay settlement before service credit. |
| ZSQ-OBCITA-15 | Legacy smith choice bound uses smith-array index i, while Citadel menu has10 entries and duplicate67; ore loop searches ch carrying rather than pl. | Plan a separate named legacy forging correctness fix: validate menu length/index before access and select payer ore; test all choices and failure refunds. Keep active refusal until owned integration is qualified; duplicated menu entry needs intent review. |
| ZSQ-OBCITA-16 | do_ascend rejects !IS_NPC before its paladin/avenger shrine75610 branch; committed transformation code and epic debit exist. | Current normal-PC path is refused, so the shrine is not a promised working ascension quest. Plan separate actor-guard/eligibility repair and durable debit/effect tests after intended service scope is confirmed. |
| ZSQ-OBCITA-17 | Bracelet75608/ring75619/gadget75616 have directionless move-trap T1; memory55188 has T2 with0 charges. Generic checks need direction or object/open flags. | Do not promise worn negative-energy damage, a live gadget trap or memory hazard. Confirm builder trap intent before a separate data fix; qualify trigger, charges, recipients and actual harmful outcome. |
| ZSQ-OBCITA-18 | Book75624/note75623/translation scroll75625 describe Orcus, Satar rise, fallen paladins and underground lore. Captives and egg have no native rescue/hatching return. | Use contextual leads with explicit availability. Liberation/escort, boss conquest, keystone destruction or hatch stories need builder-defined outcomes, actor eligibility and restart/replay authority. |
| ZSQ-OBCITA-19 | Imported359 rune stone binds epic_stone;55188/_noquest_ memory is Satar G stock. Ravenloft independently stocks local acid/cold scrolls. | Reuse committed zone_touch participants for optional rune service; possession of memory or outside scroll copy is not boss victory. Bounded outside stock does not newly claim whole Ravenloft comprehensive. |
| ZSQ-OBCITA-20 | Header75778/1/0/20/30/1 resets when empty; active reset_zone refuses item A/O/P/G/E while retaining M/F/D. All active targets resolve. | Qualify owned refill and giver availability before daily assignment. Rollover does not repop consumed proof or restore the vampire; account-ready authority remains required for all new credit. |

## Chasm of the Misty Vale: exact scales and alternate routes

The [complete Chasm of the Misty Vale dossier](../design/zone-stories/THE_CHASM_OF_THE_MISTY_VALE.md) maps four exchanges, ten contacts and twenty follow-ups. Preserve exact small/large duplicate quantities, four independent departing histories and four potential dailies. The shaman potion is optional bridge aid; WAKE head and TOUCH flowers expose alternate shared routes. Qualify owned potion effects, successful switch transitions, stock and selected giver retirement before new step credit. Casket key is available but not required for its initially unlocked state. Family lore has no reunion receipt. Link the existing Winterhaven Lancer missing-reward repair and keep native fixes in separate named news commits. All new credit requires active, ready accounting.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-MIST-CHASM-01 | Q11/35/47/69 define four independent, departing, all-item offerings; reset mode1 preserves repeatability. | Keep four accepted histories/achievements/potential dailies. Do not infer a required potion-to-shield chain or all four exchanges at one giver instance. |
| ZSQ-MIST-CHASM-02 | Three addressed M records have11 aliases. | Record actual actor, addressed target, successful reply and source epoch; topics reveal guidance without11 achievements or listener completion. |
| ZSQ-MIST-CHASM-03 | Weaver requires two I15017 and two I15018; shaman requires one of each. | Count distinct owned loose instances by exact prototype. Four copies of one size, held/nested copies or rewards cannot replace the stated multiset. |
| ZSQ-MIST-CHASM-04 | Large-scale G15018 is on six M15005 forest salamanders; small G15017 is on six M15014, with O15017@15099 alternative. | Keep reset parent/location/source episodes and distinguish first-source recovery from another player transfer, ground stock and personal victory. Names alone are insufficient. |
| ZSQ-MIST-CHASM-05 | Warrior pays C2000 or C4000; weaver returns I15019 and E81000 together. | Rewards are not offering fees. Apply item/coin/experience grants and item consumption atomically under their existing accepted transaction; retain one shield exchange history across replay and recovery. |
| ZSQ-MIST-CHASM-06 | All four exchanges depart the selected giver; warrior has four reset locations, shaman/weaver one each. | Own giver retirement and actual availability before daily assignment. Completing one sale cannot consume or create every warrior instance; rollover does not restock. |
| ZSQ-MIST-CHASM-07 | Potion15008 level50 contains LEVITATE83, REMOVE_CURSE35 and REMOVE_POISON43. | Qualify owned consumption, selected instance, successful effect publication and retirement/replay before potion-use credit. Inventory readiness, QUAFF invocation and potion reward history are not effect proof. |
| ZSQ-MIST-CHASM-08 | Full do_quaff can spill in combat, refuse timers or consume in no-magic without casting; legacy extraction follows casts. | Add an authoritative consumed/effect outcome integration where required; do not infer a durable consumption receipt from raw extract_obj or promise every attempt grants levitation. |
| ZSQ-MIST-CHASM-09 | Bridge15021–15024 F5 and shaft15030–15034 F3 feed chance_fall through command_interpreter. | Treat F as a per-command fall probability, not weight capacity. Levitation/flight suppress falling_start; mount protection depends on the actual mount. Keep actual fall/movement outcomes separate. |
| ZSQ-MIST-CHASM-10 | Chasm air rooms15079–15086 use NO_GROUND; levitation does not replace flight for lateral movement. | Explain the bridge aid without promising unrestricted air travel. Qualify arrival/safe crossing and alternate routes before adding a mandatory travel objective. |
| ZSQ-MIST-CHASM-11 | Fixed switch15000@15029 responds to WAKE46 and opens the downward block;15016/15023 flowers respond to TOUCH320 across15004E/15087W. | Use successful actor/instance/room/epoch state transition authority. Already open Nothing happens and shared access do not prove personal activation. Never turn fixed scenery into offerings. |
| ZSQ-MIST-CHASM-12 | Granite head15003@15052 is type12 with stale-looking switch values and T5/2/5/35. | Generic switch does not bind type12; its trap lacks required direction/object/open flags in reviewed checks. Confirm builder intent before a separately named data/behavior fix; do not automatically activate it. |
| ZSQ-MIST-CHASM-13 | Casket15010 values250/5/15024 starts closed/unlocked; guardian15013@15078 carries skeletal key15024 with break100. | Opening the reset casket does not require key recovery. Qualify actual lock/unlock/key-break/container outcome separately if a player changes the lock state. |
| ZSQ-MIST-CHASM-14 | Secret trapdoors15073U/15077D are locked with key−2; skeletal key fits the casket. | Normal unlocking and picking reject negative key IDs; container KNOCK does not open these doors. SEARCH and pass-door conditions need separate qualification; confirm intended access before any repair. |
| ZSQ-MIST-CHASM-15 | Source-only reachability depends on successful switches/SEARCH and door-state policy, while room arrival remains distinct. | Keep the physical graph an explanatory tool, not all-room completion or played travel proof. Fall hazards, stock availability, magic and shared world state must remain explicit assumptions. |
| ZSQ-MIST-CHASM-16 | Mother15015 and young15016 provide reunion lore; banshee, cleric and guardian have no native victory/rescue receipt. | Add builder-defined actor eligibility, escort/reunion/cleansing outcomes and restart-safe authority before credit. Cosmetic spider/guardian description inconsistencies need intent review, not automatic gameplay changes. |
| ZSQ-MIST-CHASM-17 | Banshee15008@15063 carries imported unique67273 and local necklace15013; reward potion item15008 shares only its numeric ID. | Keep typed mobile/item namespaces and imported stock provenance. Possession or identical IDs do not establish banshee victory or another local exchange. |
| ZSQ-MIST-CHASM-18 | Outside hermit97901@97907 QA26 consumes I15008 for I97930; its bounded dialogue explicitly refers to the Khomani shaman. | Keep foreign accepted history in its own zone. The same potion instance cannot be spent and drunk; supplied potion does not require personal shaman history. Outside zone is not newly comprehensive. |
| ZSQ-MIST-CHASM-19 | Winterhaven Lancer55151@55603 Q2627 consumes casket necklace15012 for missing33719, already excluded in its audit. | Link the existing builder repair requirement. Restore a verified intended reward in a separate named fix/news commit with owned grant and persistence tests before enabling the exchange. |
| ZSQ-MIST-CHASM-20 | Header15099/1/8/5/15/1 means empty-zone reset, ZONE_TOWN, lifespan5–15/difficulty1; all local targets resolve. | Preserve town/reset policy and native classifications. Active accounting requires owned A/O/P/G/E refill qualification; stock issuance, giver availability and frozen recovery remain separate from new credit. |

## Varathorn Keep: hidden proof and independent returns

The [complete Varathorn Keep dossier](../design/zone-stories/VARATHORN_KEEP.md) maps three independent returns, twelve contacts and twenty follow-ups. Two exact follower talons, the stolen ring and hidden ossuary remain distinct loose preparation and accepted history. Successful SEARCH, alternate shared passages, initially unlocked study, conditional fall hazards and wandering giver availability need separate owned outcomes. Current claymore data bypasses its named custom slayer and selects a spell slot with no normal skill registration; confirm builder intent before a separate native fix/news commit. Captive and curse lore do not invent release receipts. Preserve three achievements/three potential dailies and require active, ready accounting.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-KASTLE-01 | QA24/QA84/Q153 define three independent all-item returns, all departing; header reset2 retains repeatability. | Preserve three accepted histories/achievements/potential dailies. No mandatory talon-to-ring-to-ossuary chain or all-room objective. |
| ZSQ-KASTLE-02 | Three M records have ten aliases; QA output includes room-facing ritual or release text. | Own actor, addressed target, actual reply and source epoch before topic-step credit. Listener echo and individual aliases do not create achievements or prove accepted returns. |
| ZSQ-KASTLE-03 | Volordles requires I98914 twice, then grants I99447 plus C384000. | Count two distinct exact owned loose instances. Wrong claws, one talon, held/nested copies and reward weapons cannot fit the multiset; supplied copies do not prove personal source. |
| ZSQ-KASTLE-04 | Two E98914 rows belong to F98904 at98949/slots16 and17/cap2, rather than M98900 or same-name other crawlers. | Retain reset parent/follower/location/source episode; distinguish direct source recovery, transferred items and personal victory. Bound outside qualification to this selected group. |
| ZSQ-KASTLE-05 | Nrohtereb99401@99471 wears I99414 slot2; also carries a different insignia99448. | Bind the exact stolen ring and selected source. Insignia99448 is not reward99419; possession, meet, victory and accepted return remain different outcomes. |
| ZSQ-KASTLE-06 | Slordlak grants E124000 and C50000; Volordles grants item and copper together. | Coin amounts are rewards, not fees. Qualify atomic owned offering consumption/grants/giver retirement and recovery as one accepted exchange, not one receipt per reward. |
| ZSQ-KASTLE-07 | O99430@99463/cap1 is ITEM_SECRET4096; full SEARCH reveals room stock on a successful chance. | Add actor/objectUID/room/epoch successful reveal authority where needed. An attempt, shared visibility, raw flag change or later supplied copy does not prove personal reveal/recovery. |
| ZSQ-KASTLE-08 | Ossuary is type15 container with open/no-key values9/0/0/9; no defined P bones. | Treat the exact container as the offering. Do not invent a separate required bones item, locked opening or smashing step from prose; qualify any builder-defined destruction/release separately. |
| ZSQ-KASTLE-09 | Type29 fixed controls99400 PUSH270@99406D,99401 TOUCH320@99418U,99405 TOUCH320@99420S clear blocked paths. | Own successful selected-instance transition/epoch and reciprocal effects. Already open/shared paths are not personal activation; preserve fixed scenery and optional alternate routes. |
| ZSQ-KASTLE-10 | Source graph reaches92 public rooms with or without these activations;99492–99498 distribution and99499 void have no public incoming route. | Do not make each switch mandatory or require all100 rooms. Explain source graph assumptions separately from played movement and actual encounter availability. |
| ZSQ-KASTLE-11 | Study99482E/99483W has key99404, Dstate1 initially closed/unlocked; Dicalk carries key in slot28. | No mandatory key-before-first-entry requirement. Qualify actual lock/open/unlock/key custody/consumption outcomes only when required; exact key identity remains meaningful after a lock. |
| ZSQ-KASTLE-12 | Open bookshelf99402@99483 P stock99403 tome and99445 feather are both ITEM_SECRET; foreign feather stock also exists. | Own successful container reveal/recovery separately from shared opening and outside purchase. Neither item is an offering for the three native returns. |
| ZSQ-KASTLE-13 | Room99486 F81 feeds chance_fall; downward trapdoor starts closed/unlocked and VIRTUAL_CAN_GO excludes closed/secret/blocked. | Document conditional hazard, adequate movement and flight/levitation/climb/mount eligibility. Do not promise inevitable falling, safe transit or a receipt from raw command execution. |
| ZSQ-KASTLE-14 | Volordles/Slordlak load in distribution99498/99493 and may wander; Varathorn sentinel@99491. All retire on acceptance. | Qualify owned reset issuance, current NPC availability and selected giver retirement before daily assignment. Wander restrictions and daily rollover do not guarantee replenishment. |
| ZSQ-KASTLE-15 | Dagger99447 is effectively bound in assign_objects; value5=0 dispatches custom worn-owner/live-target1/30 devitalize via CMD_MELEE_HIT1000. | Treat fixed command as event code, not actual damage. Qualify committed vitality effect/target/instance and death continuation independently; reward possession, beam text and invocation are not quest history. |
| ZSQ-KASTLE-16 | Claymore99432 is bound in assign_rooms but values5/6/7=48/41/30 select packed slot48 at level41 with1/30 selection, bypassing custom slayer. Normal skill initialization clears pointers and has no FIRE_BREATH48 registration; generic dispatch requires a nonnull pointer. | Builder intent review: choose intended owned packed magic or undead-specific custom behavior, and repair the conflicting data/missing registration only in a separate named fix/news commit. Verify dispatch, owned action routing, restrictions, target survival and outcomes before changing values or handlers. Audit other consumers of slot48 before any global spell-registration repair. |
| ZSQ-KASTLE-17 | Full custom slayer has1/25 undead/wraith check, destroy-undead39 and cure-serious39; these three referenced spell handlers are registered. Packed slot48 lacks normal registration, so current guidance must not promise a fire-breath effect. | Do not advertise custom damage/heal as current melee behavior or infer live property settings. An initiated/scheduled effect is not a successful damage, healing, kill or quest receipt. |
| ZSQ-KASTLE-18 | Captive99429/30/31, shackled99437/39/40 and escaped99438/41/42 are distinct reset prototypes; cursed residents and tower narrators provide lore. | Builder mapping needs actor eligibility, actual rescue/escort/reunion/release transitions and restart-safe authority. Confirm ward/vampire-hunter/narrator intent before separately selected prose or behavior repair; no whole-death-pipeline claim. |
| ZSQ-KASTLE-19 | Three bounded foreign merchants list selected local gear; Tharnadia Q69/132613 takes132664 for132620 plus local quill99412. | Keep exact shop lists distinct from G inventory and actual available purchase. Own outside accepted histories/provenance separately; no fourth local return or newly comprehensive outside zone. |
| ZSQ-KASTLE-20 | All100 rooms/222 exits/63mobiles/49objects/331resets/191expanded groups and bounded outside active targets resolve. | No missing local prototype or selected native repair found. Keep accounting active/ready for new credit; extend builder integrations for owned source/reveal/access/combat/stock outcomes before enabling deeper steps. |

## Goblin City: multiset returns and explicit builder prerequisites

The [complete Goblin City dossier](../design/zone-stories/GOBLIN_CITY.md) maps three independent returns, twelve contacts and twenty follow-ups. Three circle and charm multisets remain distinct from the hidden/no-rent pirate hat; coins/XP are rewards. Addressed MA echoes credit the actor, while ambient gambling chatter does not. City-side rock/tapestry access, conditional Shipyards approach, hidden corpse proof, trapped pickable desk, currency recovery and services need separate outcome authority. Missing teacher flags and speaker spellings are explicit builder repair plans, with any selected native repair kept in a separate named fix/news commit. Preserve three achievements/three potential dailies and require active, ready accounting.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-MALCH-01 | Three QA returns require multisets3 circles/1 hat/3 charms and pay coin/XP rewards; all depart under reset2. | Keep exact accepted recipe/giver identity and full reward commit. Qualify insufficient/wrong/duplicate pointer/held/nested items, wallet capacity, grant failure, retirement, replay and cold recovery before claiming played completion. |
| ZSQ-MALCH-02 | Current carried-item rows show present exact loose quantity; they cannot prove acquisition from a selected NPC or that the player killed it. | Add source episode/instance/reset generation and committed actor custody receipts. Accept supplied copies for native recipes; make builder-selected personal recovery or victory optional distinct achievements. |
| ZSQ-MALCH-03 | Snarg dialogue names Shadowclave druids; eight equipped93901 sources include five cloaked druids and Grelim/Verra/Iliyad, plus Alatorin visitor G93901. | Builder-map intended source family and alternative circulation. Distinguish original source, NPC loot, player transfer and reacquisition; do not narrow the native recipe to one source without an explicit design decision. |
| ZSQ-MALCH-04 | Shablem needs three43131 charms carried by three43188 orc warlocks at43317; same-ID mobile43131 is a different fisherman. | Retain typed item/mobile identities and distinct source instances. Add per-episode altar/victory receipts only after confirmed live eligible actors, kills and item custody; possession alone remains preparation. |
| ZSQ-MALCH-05 | Hat23622 is E stock of Threzik23622 at23756; the shared numeric ID does not mean an item is a mobile or prove its defeat. | Keep exact prototype/source identity and reset cap/chance separate. Qualify current Threzik/hat instances, actual combat outcome, recovery and supplied copies. |
| ZSQ-MALCH-06 | Hat23622 and charm43131 have ITEM_SECRET4096; ordinary visibility rejects the flag and successful corpse SEARCH can clear it. | Add confirmed actor reveal/recovery receipts with object UID/container/source generation. Cover invisible or failed searches, shared reveal, another actor, already visible copies, empty corpse, reset and restart. |
| ZSQ-MALCH-07 | Hat has ITEM_NORENT8388608; authoritative snapshots include active durable custody even when omit_norent is requested. | Builder-confirm intended lifecycle before promising expiry or persistence. Qualify rent/logout/death/recovery/transfer under active accounting and keep a separately named lifecycle fix/news commit if behavior needs correction. |
| ZSQ-MALCH-08 | From city entries23600/23721/23741 the source graph reaches159 rooms without switches and161 with them;23759/23760 are behind the blocked pair. | Map successful PUSH rock23618@23758N and tapestry23619@23759S as optional actor actions. Admit actual control identity and before/after state; shared open passages or attempts must not fabricate personal activation. |
| ZSQ-MALCH-09 | Switch value3=1 chooses a message; actual EX_SECRET controls the reciprocal branch. Both selected reset edges are blocked8 with verified reciprocal targets. | Retain current fixed scenery/TAKE policy. Test wrong same-name object, absent/disabled target, already open, actor/recipient changes and reciprocal effects before a durable access integration. |
| ZSQ-MALCH-10 | Shipyards43320DOWN enters23760 and23759; outside43234W is secret/closed D5 and reverse43320E is D1. | Describe a separate conditional approach, not an unconditional bypass. Qualify SEARCH/open/arrival, directions and outside stock without declaring the entire foreign campaign comprehensive. |
| ZSQ-MALCH-11 | City secret door23716W starts D5 and reciprocal23717E D1; room search can reveal unblocked secrecy. | Add successful reveal/open/arrival authority when builders want access achievements. World loader masks low door bits; use effective resets, not raw world secrecy labels alone. |
| ZSQ-MALCH-12 | Desk23607 values100/15/0/100 is closed/locked but not CONT_PICKPROOF16; key0 is nonnegative, and eligible PICK/KNOCK can unlock it. | Explain skill/tool/level/chance restrictions. Record successful unlock separately from opening and disarming; no key acquisition prerequisite or guaranteed access is inferred. |
| ZSQ-MALCH-13 | Desk T518/2/10/71 is object+room+opening fire trap with ten charges and71d10 raw damage before downstream policy. | Builder-confirm intended strength, charge count and room impact against the zone's intended gameplay balance without assuming a defect. Qualify disarm/trigger/actual damage/affected actors/survival/recovery before separate trap achievements or a named balance repair. |
| ZSQ-MALCH-14 | Hidden P ivory23608 in the desk and book/quill P stock in open bookcase23601 are not offered in any local QA. | Keep optional exploration context. Builder-defined material use, successful container recovery and an explicit accepted endpoint are needed before inventing additional story completion. |
| ZSQ-MALCH-15 | Hidden coin sack23616 also circulates at Newhaven, Minotaur Pass and Surface; selected coin GET submits source pile/wallet transactions. | Credit only committed wallet effects and an owned source episode, not visible treasure or GET echo. Qualify pile remainder, capacity, busy/refusal, concurrent actor, publication and replay; schema3 has no coin/source step. |
| ZSQ-MALCH-16 | Addressed MA replies echo to the room but encountered credits only the addressing actor; seven qc_action blocks emit ambient TO_ROOM without actor encounter. | Keep exactly seven addressed aliases and three topic families. Add actor-qualified conversation outcomes only where builders explicitly design them; gambling/chatter/listener presence are not bets, wallet grants or achievements. |
| ZSQ-MALCH-17 | Snarg/Gisdan are sentinel card-table stock; Shablem is nonsentinel@23615. All three retire on accepted return. | Qualify encounter eligibility, current instance/generation, wander restrictions, reset/cap timing and actual daily availability. Day rollover must not manufacture sources or an accepting giver. |
| ZSQ-MALCH-18 | Guildmaster23613 and Maglo23617 describe teaching but lack ACT_TEACHER32768; Gobo23608 and Sulog23614 have it. FindTeacher checks the flag, not prose. | Builder-decide whether the first two are intended trainers or guild lore. Any enabling flag change belongs in a separate named native fix/news commit with class/level/visibility/service tests; positive paid practice remains refused under active accounting. |
| ZSQ-MALCH-19 | One dialogue names Gisban, another ambient line repeats it; Snarg prose also says Skarg. Prototype/accepted reply names are Gisdan and Snarg. | Journal uses actual addressable prototype keywords. Queue a separately named text correction with before/after examples after builder intent review; do not confuse a spelling repair with new quest mechanics. |
| ZSQ-MALCH-20 | F60 ledge23742/F5 pirate rooms23743–23760, githyanki SHIFT PRIME destination23691, outcast revenge and ritual corpse/altar scenery lack local actor-specific story receipts. | Qualify conditional travel and actual source outcomes before new achievements. Builder-map living rescue/ritual/revenge endpoints where intended; scenery, chance events, arrival and corpse descriptions alone are contextual leads. |

## Miaeril Village: exact recipes and explicit cross-zone prerequisites

The [complete Miaeril Village dossier](../design/zone-stories/MIAERIL_VILLAGE.md) maps three independent returns, twelve contacts and twenty follow-ups. Shared exact frozen-heart material pays two distinct cloak recipes; five salmon pay a spellbook without a personal catch or pet-feeding predicate. Secret stump access, river/fall hazards, stock chance, reward possession and outside five-skull lead keep separate authority. Expand imported-material recipe traversal with typed visited identities and advisory links; native narrative order cannot manufacture a stage prerequisite. Stable placeholder, salmon wording and book contents have balanced builder intent/repair plans, with any selected native repair kept in a separate named fix/news commit. Preserve three achievements/three potential dailies and require active, ready accounting.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-MRIL-01 | All3 item-only QA returns stay and are repeatable/daily eligible under reset2. Their distinct native bindings are unchanged. | Require active, ready accounting for new credit and retain frozen recovery separately. Qualify actual accepting instance, owned materials, reward entitlement, busy/refusal, replay and recovery; day rollover cannot replenish stock. |
| ZSQ-MRIL-02 | Hooded33700 and Uduku33701 both consume I33710 but pay different33702/33713 cloaks. One current heart prepares either card and cannot pay both. | Keep independent completion receipts and current resource availability. If builders want mutually exclusive choices, reputation or shared campaign stages, design explicit branch policy instead of inferring it from one shared material. |
| ZSQ-MRIL-03 | Falga33711 QA48 consumes five distinct33709 instances; six33718 salmon placements at33712..33717 G the food item with cap6/chance100. | Document exact quantity and native stock. Add personal catch or first recovery only with confirmed source/actor/item UID episodes; generic fishing and item names cannot establish the required source or quantity. |
| ZSQ-MRIL-04 | Owned offering accepts exact supplied copies and has no local personal-kill/source predicate. Mobile/item33710 and33711 are different typed identities. | Keep provenance, acquisition mode and present inventory separate. Builder-required personal recovery needs a durable actor/source receipt; another player's valid material can prepare an exchange without granting the recipient a source achievement. |
| ZSQ-MRIL-05 | Heart33710 has ITEM_SECRET4096; ordinary visible-item selection and successful SEARCH matter, not simply owning a hidden object. | Qualify successful actor reveal and recovery, corpse/source instance, already revealed/shared discoveries, failed searches, reset, transfer and restart. Schema3 possession rows alone do not record personal SEARCH or original source. |
| ZSQ-MRIL-06 | Frost giant33720 M@33752 has cap1/chance70 and G33710 cap1/100. Reward dialogue says stopped for good but neither QA D nor global retirement is defined. | Preserve native spawn/reset and balanced narrative guidance. If permanent village safety is intended, design an explicit scoped world/campaign outcome; do not infer a lasting global kill, guarantee a live spawn or change chance as a mapping repair. |
| ZSQ-MRIL-07 | F33710 at Falga33711@33711 is following pet stock. QA accepts fish without checking that the pet lives or actually eats. | Add builder-defined actor/recipient feeding authority only if desired: pet instance, consumed food, actual effect, affected actor and recovery. Keep delivery completion separate from a living-pet or feast outcome. |
| ZSQ-MRIL-08 | Stump33741DOWN has secret/closed D5; reverse33742UP D1, key0. Source graph misses33742/33743 without SEARCH and reaches65 with conditional SEARCH/open. | Record successful actor reveal/open/arrival only after effective action admission. Shared opened access does not prove who opened it; no special key, belt or magical prerequisite is inferred. |
| ZSQ-MRIL-09 | F40 at33742 down33743 and33744 down33740, with single-file passages; tree canopy reaches Tarik33745. | Qualify fall/climb/flight/levitation/mount and actual exit state before an access or survival achievement. Source reachability is not safe movement or a successful actor action. |
| ZSQ-MRIL-10 | River33712..33717 has C50/2 south; selected loader, arrival, command and upstream movement branches are conditional on water/exit/chance and actor state. | Explain current risks without promising automatic sweep or a boat bypass. Actor-qualified travel/survival requires confirmed destination/effect and replay-safe authority; exclude merely attempted movement. |
| ZSQ-MRIL-11 | Bridge33718 connects village33702 and forest33719. Source graph can reach59 nonriver rooms with conditional SEARCH/open; forest uses ROOM_BLOCKS_SIGHT and snowy sector38. | Preserve the optional river approach and actual sight/movement restrictions. Do not require river entry for hearts, all-room visits, a magic-garden-style blocker or knowledge inferred from unseen distant actors. |
| ZSQ-MRIL-12 | Imported Gorlon skull31317 participates in Qin31310 Q46: one each31316/31317/31318/31319/31320 ->31315. Q27 takes four31311 shards separately, with no first-return predicate on Q46. | Use exact multiset and distinct-lookalike identities. A cross-zone campaign needs explicit stage/branch and source receipts; narrative order alone cannot impose the earlier shard exchange. Outside selected leads do not make the whole Dream campaign newly comprehensive. |
| ZSQ-MRIL-13 | All five Dream skulls have SECRET/NODROP/NORENT flags; selected ordinary GIVE rejects no-drop while quest dispatch has its own owned-offering path. Ownership/death prose has no qualified timer. | Builder-confirm actual source recovery, reveal, transfer/quest-turnin, rent/death and active accounting lifecycle before promising restrictions or outcomes. Any intended lifecycle repair requires a separately named fix/news commit with concrete before/after and validation. |
| ZSQ-MRIL-14 | The local-material foreign recipe scan omitted Qin because31317 is imported; explicit source search recovered the real recipe. | Expand inventory to traverse imported reset prototypes and exact native give/receive edges, with bounded visited identities, depth, source citations and unresolved leads. Keep candidate connections advisory; typed IDs, cycles, trade alternatives and script branches must not become inferred runtime prerequisites. |
| ZSQ-MRIL-15 | Selected Chaos starter profiles can grant local cloak33702/book33711 and equipment prototypes using starter_grant authority. | Reward possession is not an accepted quest receipt. Keep grant/transfer/source histories distinct and qualify mode/recipient/config/eligibility/recovery before adding a source-acquisition story outcome. |
| ZSQ-MRIL-16 | Lost book33711 is ITEM_SPELLBOOK with values0/0/300/0 and no native encoded spell list in its reviewed extras. Generic value2 is page count. | Describe the actual reward without promising spell300 or ancient spells learned. Builder-confirm whether an intentionally blank useful book or incomplete contents were intended; any content correction needs a separate named fix/news commit and scribing/class/language checks. |
| ZSQ-MRIL-17 | Gordo33714@33710 shop lists33708 milk/33706 meat/33707 nut, matching G stock; his prose invites asking about supplies but defines no local MA. | Keep stock and purchase guidance separate from quest dialogue. A successful purchase/service needs actual committed debit/item grant, keeper/open/recipient eligibility and recovery; prose alone cannot add a keyword or achievement. |
| ZSQ-MRIL-18 | Tarik scouting, Mordat spiritual journey/lyre, nomadic family and youth trap-setting have no local accepted quest endpoint or player escort linkage. | Builder-design explicit help, music, escort, trap or rescue episodes if desired. Qualify actual recipient/effect/living endpoint and distinguish repeated stock, adjacent family members and scene descriptions from player actions. |
| ZSQ-MRIL-19 | Five stable theezak placements include one33714 mane; olyx source prose, bones on Gorlon/giant, protector medallions and crystals are separate stock. | Keep optional mount/material/equipment exploration. Actor-specific taming, mount travel, harvesting or guard honor needs confirmed source/actor outcomes, not owning mane/bone/medallion or inferring a kill from its name. |
| ZSQ-MRIL-20 | Stable33709 title still reads AXSmallXStableX; freshly killed salmon extra says rotting, and several prose lines have clear spelling/grammar candidates. | Queue a separately named builder text repair with exact before/after examples: intended stable title, frigid climate, survival/beard wording and consistent salmon description. Confirm whether freshness wording is merely text or meant to express an actual mechanic; preserve IDs, flags, stock and balance until that intent is settled. |

## Temple of the Sun: explicit key stages and accounted service outcomes

The [complete Temple of the Sun dossier](../design/zone-stories/TEMPLE_OF_THE_SUN.md) maps three independent returns, twelve contacts and twenty follow-ups. Distinct pine-heart/bear-mind evidence earns the vine key; inner keys reach the demon materials for two separate ring exchanges. Physical access/current keys and accepted history remain distinct, including supplied materials and shared opened doors. Epic Charisma and generic foraging refuse under active accounting, so future service/source objectives need accounted adapters. Imported stone/fountain handlers and outside ring/ingredient recipes expose bounded typed discovery gaps. Ring/mace/barding wording, disposable-key ordering and fountain target selection receive fair intent/qualification plans; selected repairs require separate named fix/news commits. Preserve three achievements/three potential dailies and active, ready accounting.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-SUNTMPL-01 | Three staying item-only Q returns remain repeatable/daily eligible under reset1. No native contracts or unit classifications change. | Require active, ready accounting for new credit and keep frozen recovery separate. Qualify actor/giver eligibility, exact owned loose roots, reward entitlement, refusal, replay and recovery. Daily rollover does not restock an empty-world reset. |
| ZSQ-SUNTMPL-02 | Trentloss Q11 takes one82402 pine heart AND one82403 bear mind for82420 vine key; oak Q29 takes82408 for82409; Grentkas Q44 takes82406 for82405. | Retain distinct multiset and giver/reward identities. Supplied materials can prepare the requests without source credit; repeated one-kind evidence cannot replace the missing other kind. |
| ZSQ-SUNTMPL-03 | Native pine82402@82409/bear82403@82412 stock supplies different evidence; demon82406@82433 carries both sunbeam82408 and rock82406. Typed mobile82408 is Frolikk. | Add durable actor/source/item UID acquisition and personal victory episodes only after confirmed outcomes. Separate source killing, actual reveal/recovery, player gifts and current inventory; numeric identity does not join mob and item namespaces. |
| ZSQ-SUNTMPL-04 | Pine/oak/bear passages use secret closed D5; ordinary SEARCH and opening are needed for unrevealed access. | Add builder-declared successful reveal/open/arrival prerequisites with effective target and state. Shared revealed/opened access is not the arriving player's personal SEARCH or opening achievement. |
| ZSQ-SUNTMPL-05 | Vine82420 matches pickproof locked82406N/82413S; Rantah82423@82452 gives82423 for secret82415UP/82432DOWN; statue82424@82432 gives82424 for locked82432UP/82433DOWN. | Explain the three physical access stages dynamically, with current keys/door state, actual source and admitted arrival. Do not require earlier exchange history when a valid supplied key or already open door can admit the actor. |
| ZSQ-SUNTMPL-06 | All three keys have NORENT and value1=100. has_key checks HOLD/loose carried roots; native unlock clears door flags before an asynchronous owned-key destruction submission. | Qualify spent-key persistence and door/destruction ordering under failure, concurrent use, restart and recovery. Plan a separately named generic fix only after defining intended atomicity; source ordering is a review candidate, not a played exploit claim. |
| ZSQ-SUNTMPL-07 | Source graph from82493 reaches51 without SEARCH/keys,54 with SEARCH,88 with vine key,89 with cellar key too,90 with all3 keys and94 with qualified PICK. | Use these as source-only access guidance with explicit movement/open/search/unlock/stock assumptions. Live hazards, destruction, reset and actor state need separate journeys; do not turn graph reachability into an all-room or safe-travel objective. |
| ZSQ-SUNTMPL-08 | Locked82418E/82437W uses key0 with world traits1/2, without pickproof. All3 named keys alone leave82437..82440 outside the source graph; skilled held-pick access qualifies. | Builder-confirm intended locked-wing route before any data correction. Preserve actual PICK eligibility and shared reciprocal state. Full Knock handler only supports containers; do not advertise it as a room-door bypass. |
| ZSQ-SUNTMPL-09 | Oak dialogue/82409 extras promise a mace, but actual reward82409 is type9 armor, ring name/keyword and TAKE/FINGER wear. | Queue a separate builder text/intent fix with precise before/after and item compatibility tests. Confirm whether dialogue should describe the ring or a different reward was intended; do not convert the item to a weapon as a mapping repair. |
| ZSQ-SUNTMPL-10 | Grentkas greeting has an incomplete noun and acceptance describes centaur barding/armor;82405 actual wear33554435 is TAKE/FINGER/TAIL. | Queue a separately named text/intent repair after confirmation, preserving actual item identity and balance. Tail wear does not equal BARDING. Enfil's outside82405 recipe is a compatibility consideration for any chosen repair. |
| ZSQ-SUNTMPL-11 | Oak acceptance says restore the woods; other prose describes valves, ritual, evil spirits and sun power. Three Q blocks define no additional world restoration or valve-action receipt. | Builder-design scoped actor/campaign/world outcomes if intended, with authoritative mutation, target, duration and reset/replay policy. Acceptance history does not prove a permanent forest-state transition. |
| ZSQ-SUNTMPL-12 | Frolikk82408 Epic Charisma is table-bound through epic_initialization, independently of ordinary teacher flags. epic_teacher refuses purchases under active economic accounting. | Add an accounted composite epic/coin/skill service and committed outcome adapter before offering training as a runnable story objective. Qualify level/class/prerequisite/deny/mastery costs and recovery; do not bypass the accounting refusal or alter teacher flags. |
| ZSQ-SUNTMPL-13 | Imported359 epic_stone at82433 performs periodic DB setup and durable group award submission, with magic/readiness/locality/peace/trust/level checks. | Integrate committed stone outcome with explicit actor/group role and policy before any story credit. Pending touch messages, co-presence and seeing an award are not receipts. Qualify recovery without re-crediting recovered claims or assuming fixed source payout values. |
| ZSQ-SUNTMPL-14 | Imported72 spell_pool at82432 chooses9 buffs and rotates after40 minutes; its CMD_DRINK handler ignores the argument and object specials precede generic command handling. | Plan a separate target-selection qualification/fix with argument resolution, unrelated drink, visibility, eligible actor and actual effect tests. A typed DRINK command alone cannot prove the player used this fountain or received a particular spell. |
| ZSQ-SUNTMPL-15 | The local-vnum custom scan omits imported359/72 handlers; Frolikk is bound through an epic teacher table. | Expand bounded typed inventory closure to imported reset specials, table-bound mobiles and effective enabled assignments, with citations/depth/cycle limits. Surface unresolved handlers as advisory integration gaps, never inferred runtime prerequisites. |
| ZSQ-SUNTMPL-16 | Imported821/822/823/826 ingredients are placed stock. Generic forage also names these prototypes but do_forage refuses under active accounting. Farmer belts82426 have no selected magical garden predicate. | Distinguish floor pickup from actual foraging/harvesting and player transfer. Add an accounted source generation/action adapter before any personal forage goal; preserve stock and do not invent a belt gate from gardening prose. |
| ZSQ-SUNTMPL-17 | Seven selected outside native recipes use local ring82405 or imported plant materials: Enfil, Branay, Red and four Alatorin cooks. | Keep separate exact foreign giver/contracts and full multiset requirements. The local-material-only scan misses imported ingredient edges. Expand advisory typed recipe traversal; a local reward/ingredient does not complete an outside receipt or establish stage order. |
| ZSQ-SUNTMPL-18 | Frolikk holds lyre82407 and unique imported music box67253; Pelakra carries type33 tome82404 with150 pages. Research assistants and Llentor have no further local Q return. | Design performance/research/learning outcomes only through qualified action recipients/effects and unique-item/book lifecycle. Source/reward possession and readable lore do not prove training, learned spells or a research completion. |
| ZSQ-SUNTMPL-19 | Three addressed M families have11 aliases; oak and pine spirits share visible name a spirit but distinct typed mobs/rooms and contact keywords. | Keep actor-addressed conversations, selected target and visibility distinct from room listeners/repeated aliases. Builder UI should explain the oak-versus-pine location without renaming native prototypes or multiplying achievements. |
| ZSQ-SUNTMPL-20 | Native text has ring/mace/barding mismatch, incomplete crafting wording and duplicate the the; source graphs and imported services have qualification gaps. No native repair ships here. | Record fair intent/safety follow-ups with source citations. Any selected gameplay/text repair must use a separate named fix/news commit with trigger, before/after, exact scope and validation. Played active-accounting exchanges, keys, services and outside outcomes remain separate qualification. |

## Gibberling King: exact offerings and separate action outcomes

The [complete Gibberling King dossier](../design/zone-stories/LAIR_OF_THE_GIBBERLING_KING.md) maps two independent departing-giver returns, eight contacts and twenty follow-ups. The cook accepts one roper tentacle; the noble accepts distinct wand/essence together. Exact visible supplied proof does not require source victories, a cast or earlier cook history. Keys, resolved boulder PUSH, opening and arrival are separate physical stages; shared/alternate access remains valid. SECRET materials, chance-based Roshen stock, internal seeder/sink rooms and P/F instance semantics need explicit source qualification. The blindness-configured dismissal wand, cook rock wording and nonblocked dung switch receive fair builder intent/repair plans; selected repairs require separate named fix/news commits. Imported stone and outside memory/wind recipes are bounded leads with independent committed outcomes. Preserve two achievements/two potential dailies and active, ready accounting.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-GIBBER-01 | Both native Q returns have D departure but remain repeatable/daily eligible under reset1, with separate2 achievements/2 potential dailies. | Qualify committed actor/giver/reward entitlement, departure, refusal, replay and restart under active, ready accounting. Keep frozen recovery separate. Daily rollover does not replenish absent NPCs or material stock. |
| ZSQ-GIBBER-02 | Cook8715 Q2 requires8706 for8709; noble8731 Q32 requires distinct8717 AND8734 for8718. No coin, cast, earlier-return or personal victory requirement is in these native contracts. | Retain exact multiset/giver/reward identities and optional current-proof rows. Supplied materials prepare the requests without source history; duplicate wands cannot replace the missing essence. |
| ZSQ-GIBBER-03 | Only the selected young-roper8713 reset at8713 carries8706, although the prototype has5 placements. Roshen8735@8765 has50percent reset chance and G8717; Crymson8746@8788 has G8734 and55190. | Add durable actor/source/item-UID acquisition episodes after actual recovery. Separate defeating a prototype, stock availability, successful reveal, recovery and player transfer; do not award source credit from current possession. |
| ZSQ-GIBBER-04 | Wand8717 and essence8734 begin SECRET/NORENT; has visible loose proof differs from finding the source. do_search distinguishes floor/corpse/container reveal from room-exit reveal. | Integrate successful reveal with target UID/state and admitted acquisition. Failed search, hidden leftovers, another actor's reveal and merely seeing a corpse must not create a personal first-recovery achievement. |
| ZSQ-GIBBER-05 | Entrance8701E/8702W D2 uses8739 brittle key: O@8701 cap2 and G on Zsrsr8702@8711 cap2; type18 value1=11, SECRET/NORENT. | Explain actual visible key/live door access, with supplied keys and shared opened doors. Qualify11percent break attempts, key persistence and source authority; source victory and unlocking are distinct. |
| ZSQ-GIBBER-06 | Ggraz8719@8727 supplies8715 bent key for secret locked pickproof8727DOWN/8728UP D6; key value1=0 and SECRET/NORENT. | Model reveal/current keyed access, successful unlock/open and arrival separately. Preserve the pickproof data and no configured key-break chance. Do not impose the cook receipt as a route requirement. |
| ZSQ-GIBBER-07 | Dung8708 type29 ENTER7 targets8713DOWN; resetD5 is secret/closed without BLOCKED. Auto-bound item_switch returns Nothing happens when BLOCKED is absent. | Builder-confirm the intended dung-pile action before selecting a separate text/switch/state fix. SEARCH/open is a separate native path. Do not add BLOCKED as a mapping repair or promise ENTER success from prose. |
| ZSQ-GIBBER-08 | Boulder8716 type29 PUSH270 targets8784NORTH D9, with reverse8785SOUTH present/open. Generic switch resolves the actual object and clears BLOCKED but leaves CLOSED. | Add authoritative switch/action-state receipts with actor/object/direction and before/after flags; opening and actual travel remain separate. Qualify wrong target, repeat/shared/concurrent/reset/restart behavior before making this a runnable story step. |
| ZSQ-GIBBER-09 | Conditional directed graph from8700 reaches2 without keys,51 with brittle key/SEARCH,110 with both keys,114 with both keys/PUSH/open. Realm14209NORTHEAST directly leads to8788. | Present bounded access alternatives without requiring a single entrance history. Graphs ignore hazards/live stock/key destruction/actor eligibility; they are source evidence, not safe-travel or played-completion proof. |
| ZSQ-GIBBER-10 | Eight rooms8712/8747..8750/8790..8792 remain outside the key/PUSH source graph and include seeders/sink8791. Archer8748 is non-sentinel STAY_ZONE/HUNTER; only its first of6 resets has E8735. | Classify internal world rooms as advisory context, not mandatory player discovery. Track actual roaming material location and acquisition; builder-review sink/availability intent without opening internal rooms or adding stock as a repair. |
| ZSQ-GIBBER-11 | Cook acceptance promises a rock but actual8709 is equipment. Native noble assist text ends imprison- and mobile prose has typos. | Queue fair builder text/intent review with exact native reward semantics. Any selected text/gameplay repair needs a separate named fix/news commit with trigger, before/after, scope and validation; no reward/balance alteration is implied. |
| ZSQ-GIBBER-12 | Dismissal-named wand8717 is type3 values55/1/1/4; SPELL_BLINDNESS=4. Modern device/legacy use qualify actual targets/charge/effect; native item return only selects VNUM. | Confirm whether name/lore or spell data should change before any separate repair. Do not require casting or charges for the existing item-only return. A USE attempt, channel start or consumed charge is not a dismissal/world-liberation receipt. |
| ZSQ-GIBBER-13 | Noble acceptance/departure describes freedom, while local prisoner, warfare, blood-trail and cave-in prose define no extra native return or lasting world mutation. | Builder-design explicit actor/campaign rescue or restoration outcomes if intended, with target, authority, duration, reset and replay policy. Avoid treating an NPC disappearance or narrative line as a permanent world transition. |
| ZSQ-GIBBER-14 | F5 at8700 is a5percent chance-fall field; no local C current metadata exists. Waterfall sectors16 and lake6 are water, not automatic current or no-ground proof. | Qualify actual fall trigger, flight/levitation/mount/climb/event admission, resulting arrival and water/visibility eligibility. Capture admitted movement outcomes rather than command text, and keep source graphs conditional. |
| ZSQ-GIBBER-15 | Trapped-boulder8736 T1037/6/-1/10 is effect/damage/charge/level data, but no local reset places it. Imported memory55190 has T2/0/0/0 and is a separate token. | Keep prototype-only traps out of runnable objectives until effective placement/trigger is confirmed. Builder-confirm intent before any separate placement/trap repair; do not infer active hazards from a T record with no live source. |
| ZSQ-GIBBER-16 | Five P commands put badge8719/chain8720 into corpse8725; P reset uses global get_obj_num, not guaranteed adjacency. Thirty F follower commands alter effective parent/leader and sentinel state. | Qualify actual corpse UID/container selection and follower attribution. Extend source audits to effective P/F semantics and flag unresolved instance placement; corpse possession or group co-presence is not an actor quest receipt. |
| ZSQ-GIBBER-17 | Imported359 epic_stone O@8788 has enabled effective handler outside a local-VNUM-only custom scan; its selected service requires readiness/magic/locality/peace/trust/level and committed group awards. | Use explicit committed actor/group service outcomes and recovery policy before story credit. Pending touch messages/co-presence are insufficient. Expand bounded typed imported-special closure with citations and unresolved advisory leads. |
| ZSQ-GIBBER-18 | WH ambassador55226 QA3199 consumes memory55190 for55362 scroll,1000000coins and55033 token, stays, and has two distributor resets55005/55400. | Expose a separate outside receipt and actual roaming giver availability. Keep player-give versus player-receive directions explicit, including source-parser terminology and coin entitlement. Memory possession or local noble acceptance does not finish this outside quest. |
| ZSQ-GIBBER-19 | Air Plane north wind131616 Q98 requires8735 plus8 distinct materials for131650 Cloudseeker and departs; full selected recipe/giver/reward/material prototypes reviewed, other campaigns remain bounded leaves. | Expand advisory cross-zone material/reward traversal with depth/cycle limits and exact direction/multisets. Keep outside accepted receipts independent. Living-breeze ownership alone does not establish Cloudseeker or personal archer recovery. |
| ZSQ-GIBBER-20 | Three addressed M families/5 aliases belong only to noble8731. Typed Roshen mob8735 is not item8735 living breeze; mob8717 is not wand8717; no additional local King quest contract exists. | Preserve addressed target/conversation semantics and namespaces. Builder-designed journals should explain progression and availability dynamically without multiplying keyword achievements or inventing an endpoint from the zone title. Record all selected repairs separately and distinguish source qualification from played outcomes. |

## Ice Tower: shared doors, consumed keys and exact returns

The [complete Ice Tower dossier](../design/zone-stories/ICE_TOWER.md) maps two independent returns, eight contacts and twenty follow-ups. Exact loose necklace or wedding ring prepares its own native request; supplied material fits without personal victory/recovery or earlier other-card history. Speech unlocks the magic entrance but leaves it closed; key consumption, shared state, opening and arrival remain separate. Source graph qualifies speech/SEARCH/keys/PICK without room-door KNOCK. Hidden materials, similar slave targets, reset/F instance semantics, imported epic service and directed memory exchange need authoritative outcomes. Permanent rescue and the disabled saber require builder intent/capability work; selected repairs need separate named fix/news commits. Preserve two achievements/two potential dailies and active, ready accounting.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-ICETOWER-01 | Two item-only Q returns stay/repeat under reset1; both have independent achievements/potential dailies. | Qualify committed actor/giver/item/reward entitlement, refusal, replay and recovery under active, ready accounting. Daily rollover must not manufacture NPC/material restock. |
| ZSQ-ICETOWER-02 | Three addressed M families have eight aliases across nomad and wounded wife; husband has no native M. | Group actual conversation families and exact addressed targets. Alias text is not eight achievements. Add actor-specific successful dialogue events without treating an attempted command as proof. |
| ZSQ-ICETOWER-03 | Entrance31811N/31813S has key=-2, final keyword thrym and pickproof world trait3. | Recognize generic speech-door prerequisites outside local custom-vnum scans. Preserve complete case-insensitive word matching and actual admitted speech; map cue provenance separately from inferred vocabulary. |
| ZSQ-ICETOWER-04 | check_magic_doors unlocks/reveals local and valid reciprocal side, but leaves CLOSED. | Provide committed actor/room/direction/outcome evidence for successful speech unlock, then ordinary opening and travel. Distinguish shared open state, failed/repeated attempts and later arrivals; do not demand personal action for native item acceptance. |
| ZSQ-ICETOWER-05 | Speech can fail under silence/suppression, underwater conditions, form restrictions and other admission rules. | Expose available prerequisites without marking attempted speech complete. Keep eligibility/refusal and actual mutation authoritative, with durable receipt/replay policy for any future gate milestone. |
| ZSQ-ICETOWER-06 | Ice key31804 and rusted key31810 have value1=100; ordinary unlock requests breaking before reciprocal unlock, but durable destruction may be refused or uncommitted. | Separate successful local/reciprocal unlock from committed key destruction. The caller ignores break_key failure; owner mismatch, submission refusal or uncommitted publication can leave the key intact. Qualify UID/actor, pending/refused/recovered consumption and current readiness. Review whether unlock should depend on admitted destruction before any separate accounting/gameplay fix; preserve native balance intent. |
| ZSQ-ICETOWER-07 | Sergeant31801@31850 and slavemaster31818@31942 have cap1 key stock; local availability depends on actual resets. | Map instance-specific stock, holder, room and source episode, with live availability separate from static prototype lists. Receiving a key from a player proves no personal source recovery. |
| ZSQ-ICETOWER-08 | Holding cells and slavemaster office use world trait2/key0 locks permitting normal PICK; staircase/gate trait3 remains pickproof. | Qualify actual skill/tool/admission and successful PICK; distinguish zero-key lock configuration from a guaranteed key source. Ask builders about intended class/skill access before any separate change. |
| ZSQ-ICETOWER-09 | The complete spell_knock89..163 has container/storage/quiver handling and no room-door branch. | Do not suggest room-door KNOCK as an access substitute. If builders intend that capability, scope a separate gameplay fix with target validation, pickproof compatibility, failure/receipt and regression coverage. |
| ZSQ-ICETOWER-10 | Ring31815 and necklace31821 begin SECRET; ring is NORENT and necklace NOLOCATE. | Record successful reveal and actual recovery with source instance, actor and transfer provenance. Separate corpse/container SEARCH from room exits; no source victory or current possession alone proves first recovery. |
| ZSQ-ICETOWER-11 | Wife31822/husband31823 share barbarian/slave targeting with other mobs; husband prose identifies natural-colored eyes. | Resolve actual mob identity before dialogue/turn-in, explain target disambiguation and avoid granting another slave encounter as husband progress. Item31822 platemail is a separate namespace. |
| ZSQ-ICETOWER-12 | Wife asks for a personal action while her native block is dialogue-only; husband return checks only ring. | Preserve faithful narrative and exact accepted receipt. Optional personal action or witness progress needs authoritative outcome/attribution; never make undocumented kill/recovery/conversation history a prerequisite for supplied ring. |
| ZSQ-ICETOWER-13 | Nomad says killing Ashivahr should free people, but its native Q only checks necklace. | Plan explicit persistent rescue/enchantment/service outcomes if builders want them, including group roles, actor proof, resets, refusal and recovery. Do not infer lasting liberation from acceptance text or a boss victory. |
| ZSQ-ICETOWER-14 | Source graph13/59/135/144/153/162/160/18 depends on speech, keys, successful SEARCH/open/PICK and starting point. | Use conditional directed access advisory with live shared state and qualified actions. No safe-travel/all-room objective; supplied materials or alternate legal arrival remain valid for native exchanges. |
| ZSQ-ICETOWER-15 | Room flags restrict teleport/summon/gate and selected sight; windy/slippery prose has no F/C or no-ground metadata. | Keep atmosphere separate from generic movement/weather hazards and capability admission. Add a hazard milestone only from actual applied effect or relocation, with source citations and played qualification. |
| ZSQ-ICETOWER-16 | Two F31814 follower resets depend on actual preceding mobile runtime state, and handlers make them sentinel. | Track reset instances and selected leader/follower rather than naive line-adjacent prototypes. Source attribution must distinguish shared NPC groups, multi-player victories and transferred loot. |
| ZSQ-ICETOWER-17 | Ashivahr carries imported359 with enabled epic_stone; touch has locality/peace/trust/level/group and transaction admission. | Integrate the committed actor/group award service, not TOUCH text or pending messages. Keep source UID, eligibility, payout/level outcomes and replay/frozen recovery distinct from local item-return history. |
| ZSQ-ICETOWER-18 | Imported memory55192 is consumed by WH55228 Q3221 for55362 scroll,1000000 coins and55033 token; ambassador stays and has two distributor resets. | Extend typed directed imported/material/reward closure with depth/cycle bounds, actual target/circulation and independent outside receipt. Distributor room is not a promised current location; possession is not source victory or outside completion. |
| ZSQ-ICETOWER-19 | Saber67258 custom assignment is commented out; wife description says cave walls while reset is a balcony. | Confirm intended custom effect and location narrative before a separate named fix/news commit. Do not enable disabled gear behavior or relocate stock as mapping cleanup. Treat text/intent mismatches separately from proven broken acceptance. |
| ZSQ-ICETOWER-20 | Local QST/room/mobile/object/reset and effective/bounded imported scope is complete; no played outcome or native repair ships. | Keep documented source completeness and played accounting qualification separate. Future selected repairs require clear trigger, before/after behavior, scope and validation in a separate fix commit and prominent PR/news section. |

## Durian Underdark: paired materials, roaming contacts and source limits

The [complete Durian Underdark dossier](../design/zone-stories/DURIAN_UNDERDARK.md) maps two independent returns, eight contacts and twenty follow-ups. Both exact halves prepare one exchange; the exact skin prepares its separate bounty. Source recovery, grouped14 aliases, ambient action, wandering contacts, sparse-map admission and the later seal exchange remain distinct. Supply review covers50-percent ordinary M admission, active-accounting reset issuance, unresolved skin source and50/100platinum prose/payout mismatch. Dynamic portal/relic selectors and generic ecology/services need outcome authority. Preserve two achievements/two potential dailies; no native repair ships. Selected future fixes require separate named commits and prominent PR/news treatment.

| Reference | Finding and qualification | Capability or builder follow-up |
| --- | --- | --- |
| ZSQ-UNDERDARK-01 | Two item-only Q returns stay/repeat under reset2; two independent achievements/potential dailies remain. | Qualify committed actor/giver/material/reward entitlement, refusal, replay and recovery under active, ready accounting. Daily rollover must not manufacture source stock or NPC availability. |
| ZSQ-UNDERDARK-02 | Q35 needs distinct700000/700001 halves together, not two copies of one half; Q29 needs exact700008. | Keep separate current-material rows with one final accepted receipt. Source episodes and earlier other-card history are not native acceptance prerequisites; distinguish assembly preparation from reward creation. |
| ZSQ-UNDERDARK-03 | Three addressed M families have14 aliases; separate qc_action100 is automatic NPC ambient behavior. | Group actual topics and require actor-specific successful conversation evidence for future dialogue milestones. Exclude ambient events and attempted keywords from quest completion and daily credit. |
| ZSQ-UNDERDARK-04 | Haz700034/Oz700035 stock exact halves at neck slot3; all three drows start at847218 and wander. | Attribute first acquisition to an actual item/source instance and episode, including corpse/container transfers. A handed-over item satisfies native materials but proves no personal source recovery or victory. |
| ZSQ-UNDERDARK-05 | Half carriers use cap1/chance50 M rows978/980; ordinary M admission requires100 unless forced. | Confirm intended ordinary restock and boot/forced-reset behavior with builders. A separate reset fix must preserve caps/probability and cover refusal, duplicate population and stock issuance; journal mapping changes no native reset. |
| ZSQ-UNDERDARK-06 | Active accounting refuses reset item commands before read_object/placement; E100 also has cap/load admission. | Finish durable reset-generation/issuance authority before promising fresh halves, boots or repeatable supply. Expose source unavailability separately from journal eligibility; test committed issuance, replay and frozen recovery. |
| ZSQ-UNDERDARK-07 | No exact skin source appears in selected stock/rewards/bindings or maintained weighted table; generic CARVE makes8. | Confirm intended roper loot or historical/custom source. Add typed source-availability diagnostics and builder overrides with deployment evidence. Any selected new drop is a separate balance/quest fix, not an inferred mapping repair. |
| ZSQ-UNDERDARK-08 | Bounty dialogue promises50platinum; native C100000 converts to100platinum. | Confirm intended prose or payout and document a separately named correction with before/after and news impact. Preserve current amount until intent is settled; generic display must use native coin units. |
| ZSQ-UNDERDARK-09 | Neither half nor skin is SECRET; both halves can be worn and materials must be visible loose offerings. | Keep current visible-loose readiness separate from equipped/held/nested possession and past recovery. Do not invent a SEARCH prerequisite for these exact materials or infer acceptance from inventory. |
| ZSQ-UNDERDARK-10 | Drows have scavenger/memory/flight/swim/hunter flags, no sentinel; generic move retains native zone/state guards. | Resolve current NPC instance/location and source circulation, rather than exposing distributor847218 as a fixed public destination. Distinguish shared stock, scavenging, player transfer and reset instance changes. |
| ZSQ-UNDERDARK-11 | Physical map has13587 rooms;6813 unresolved raw targets are pruned by renum_world rather than synthesized. | Use active-loader topology and explicit sparse cells. Preserve all numeric gaps/native exits; do not create a grid-completion achievement or fill absent rooms as cleanup. |
| ZSQ-UNDERDARK-12 | 202 valid boundary directions form101 reciprocal pairs over80 full foreign records; six weak local components exist. | Map bounded neighboring entrances and directed routes with source/cycle limits. Foreign room and recipe leaves do not claim whole neighboring campaigns comprehensive; alternate arrivals remain valid. |
| ZSQ-UNDERDARK-13 | Map-to-map mountain27 admission rejects non-trusted PCs and NPCs;20328 internal source edges are excluded. | Qualify terrain/admission before giving route guidance. Static reach4014 from distributor and5807 from all incoming leaves assumes other admission/survival; it is not a safe route or played reachability proof. |
| ZSQ-UNDERDARK-14 | Distributor847218 is dark/no-magic/no-recall/no-teleport/no-gate mountain terrain; ten outgoing destinations are sector29. | Map outward NPC distribution separately from ordinary player arrival.4000 source rooms overlap incoming/distributor reach; actual contact is live world state. No broad gate/teleport or room-flag change is warranted by journal authoring. |
| ZSQ-UNDERDARK-15 | Water, fungal growth, lava, low-ceiling lore and selected flags have generic movement/sight admission; local metadata has no F/C. | Keep capability posture/visibility/hazards advisory until actual applied effects or movement receipts are available. Do not turn descriptive ecology into automatic falls, current milestones or guaranteed access. |
| ZSQ-UNDERDARK-16 | Enabled giant purple_worm700004 swallows victims and retains/releases corpses through combat/death/periodic paths. | Future rescue/recovery milestones need victim/corpse UID, actual release location, actor/group roles and durable outcomes. Separate combat ecology and dropped possessions from safe rescue promises and native returns. |
| ZSQ-UNDERDARK-17 | One outside recipe WH55151 Q2605 consumes seal700005 for55054 ring/C250000/E250000; Lancer is sentinel at55603. | Track a separate accepted outside receipt and directed material dependency. Supplied seal needs no local amulet history. House access lore has no mapped persistent gate predicate; add explicit world-state outcomes only with builder integration. |
| ZSQ-UNDERDARK-18 | Enabled467 ud_portal selects a random existing829678..850075 non-mountain/non-ocean destination for eligible living actors. | Extend dynamic destination discovery beyond fixed prototype values. Keep level10..35/trust admission, live portal availability and committed relocation separate from static graph, contact or source victory. |
| ZSQ-UNDERDARK-19 | Legacy create/reset_lab overlaps current map numbers; creation starts705050/805050 are absent and guarded; relic first command can mutate score/reset. | Confirm historical campaign/map intent, source availability and100-wide legacy offsets versus400-wide current map before any separate fix. Use mutation-generation/relocation/score receipts; transferred relic possession is not first personal recovery. Do not execute generators or broaden local Q prerequisites. |
| ZSQ-UNDERDARK-20 | Mining/node/guild services use regional/generic admission; unaccounted refill is refused during active accounting, and old Khildarak distance code is commented. | Keep service/ecology separate from quest contracts. Plan committed issuance/harvest/claim/placement outcomes and builder-specific milestones only when source authority exists. Selected repairs require separate named fix/news commits and clear validation. |

## Bahamut Palace: departing custodian and source-aware access

The [complete Bahamut Palace dossier](../design/zone-stories/BAHAMUT_PALACE.md) maps one departing seal-to-key exchange, eight contacts and twenty follow-ups. Three topics group six aliases. The13-key circuit, flight/SEARCH, hidden loose seal, source recovery, actual vault access, timed heart and competing outside returns remain distinct. One achievement/non-daily classification is preserved. Active accounting stock issuance, unchecked heart allocation/expiry retry, conditional key retirement, inactive transformation, imported mask cleanup and archive-only potion stock need qualification or separate intent/safety fixes. No native repair ships; future selected fixes require separate named commits and prominent PR/news treatment.

## Builder follow-ups and capability expansion

| Reference | Observed boundary | Planned qualification or fair repair |
|---|---|---|
| ZSQ-BAHAMUT-01 | One Q24 consumes seal25760 and creates key25724, followed by D; one achievement remains. | Qualify committed actor/giver/source/reward entitlement, refusal, replay and recovery. Source victory, blessing and vault entry must use separate successful outcomes. |
| ZSQ-BAHAMUT-02 | Reset0 plus departing custodian makes this exchange non-repeatable and non-daily. | Keep the exclusion, NPC-generation availability and attempted transaction explicit. A new day or reacquired seal must not invent another accepting custodian. |
| ZSQ-BAHAMUT-03 | Three addressed M families have six aliases, all on Tamarand25723. | Group the actual topics. Future conversation milestones need successful actor-specific responses; attempted words and encounter history do not complete the return. |
| ZSQ-BAHAMUT-04 | Seal25760 is SECRET/MAGIC/NORENT and TAKE-only; exact current visible loose proof is required. | Keep reveal, pickup, current loose material and accepted history separate. Cover hidden, held, worn, nested, supplied, spent and reacquired proof without asserting personal provenance. |
| ZSQ-BAHAMUT-05 | Bahamut25700@25918 carries G25760 cap1/100; Tamarand25723@25921 has no seal stock. | Attribute first recovery to the actual source generation/item UID/custody episode. A gift fits native material admission without source victory or blessing. |
| ZSQ-BAHAMUT-06 | Active accounting refuses O/P/G/E reset item issuance before placement. | Provide durable reset-generation issuance and idempotent recovery before promising fresh seal/key/treasure supply. Preserve world caps, chances and equipment placement. |
| ZSQ-BAHAMUT-07 | Thirteen local keyed gates have declared source stock and a conditional static entrance closure. | Keep route hints conditional on actual keys/SEARCH/shared state and survival. Qualify each successful unlock/open/arrival, key consumption and source absence before historical objectives. |
| ZSQ-BAHAMUT-08 | 223 rooms use SECT_AIR_PLANE; horizontal departure needs flight, levitation alone only vertical departure. | Show current actor or mount movement needs and refusal reasons. Qualify each participant and direction; air prose does not imply ordinary walking or a universal falling rule. |
| ZSQ-BAHAMUT-09 | Raw WLD7 high bits are masked; D reset5/6 supplies secrecy and closed/locked state. | Use effective door state from loader/reset and current shared mutation. SEARCH reveals only one side, while matching reverse unlock updates shared state; attempts alone are not receipts. |
| ZSQ-BAHAMUT-10 | Key25724 has value1=100; generic unlock changes state before break_key may refuse destruction. | Model unlock and key retirement as distinct outcomes. Review the ownership/publication partial-success boundary separately with refusal/replay tests; preserve existing break chance and access behavior. |
| ZSQ-BAHAMUT-11 | Bahamut death spills existing stock and dereferences a newly loaded55081 without a null guard. | Plan a separate allocation-safety fix with missing-template, refusal and valid-output cases. Freeze source/participants/owned stock movement before new credit; preserve encounter and drop balance. |
| ZSQ-BAHAMUT-12 | Combat summons25758 under a world-wide cap3 and number1..100<50; helper load is guarded. | Preserve shared cap and49-percent trigger. Future summon milestones need actual NPC generation/arrival and participant evidence, without rewarding attempted combat ticks. |
| ZSQ-BAHAMUT-13 | Heart55081 is initialized for three mud days; decay replaces it with55024 across custody forms. | Own the per-item deadline and replacement/retirement result. The failed replacement branch decrements below zero, so plan a separately named retry/recovery fix before timed objectives. |
| ZSQ-BAHAMUT-14 | WH55211 and Alatorin83176 consume the same three exact hearts for different rewards. | Keep competing physical allocation and distinct campaign receipts. Native exchange does not require personal kills, door use or a family-wide first-death/noon deadline; clarify timing lore with builders. |
| ZSQ-BAHAMUT-15 | Epic360 at25922 uses a durable group touch and reset_requested for reset0 zones. | Qualify committed eligible participants, refusal/replay, award and later reset independently. Access, attempted TOUCH and stone possession do not finish the seal card. |
| ZSQ-BAHAMUT-16 | THARKUN_ARTIS is defined; six local enabled bindings use current handlers. | Distinguish active BloodFeast effects from its inactive branch. DragonLord race-change callback is called only by unbound old routines; do not advertise that transformation as active. |
| ZSQ-BAHAMUT-17 | Imported67200 vigor_mask uses CMD_PERIODIC0/null actor dispatch, with REMOVE rejected by an earlier guard. | Preserve actual periodic behavior and review intended removal cleanup separately with worn/periodic/remove fixtures. Combat/equipment effects need successful effect evidence rather than handler booleans. |
| ZSQ-BAHAMUT-18 | Ravenloft58564 resets Sunblade25745 inside58430; its long/extra prose describes a different whip. | Ask builders to settle sword/whip identity and inherited Ravenloft history before a separate prose fix. Do not invent a recovery campaign or change weapon power from that mismatch. |
| ZSQ-BAHAMUT-19 | Thirteen25923..25935 staging rooms are outside the public entrance graph; imported25106 is archive-only. | Confirm operational intent before a separate disabled-stock/reference fix. The missing potion does not block the selected seal route; do not reconnect staging rooms or resurrect old power stock automatically. |
| ZSQ-BAHAMUT-20 | Cabinet25703 has trap metadata; beacon/random-potion/global equipment source leaves lack local Q contracts. | Keep trap/container outcomes and generic service source identities separate. Bounded compiled leaves and foreign staging exits are follow-ups, without claiming played availability or whole foreign campaign coverage. |

## Clan Stoutdorf: repeated materials and independent castle progression

The [complete Clan Stoutdorf dossier](../design/zone-stories/CLAN_STOUTDORF.md) maps one sixteen-item collection, nine contacts and eighteen follow-ups. Eight optional rows preserve seven individual materials plus nine distinct meteorites; three topics do not create keyword prerequisites. Active accounting remains required, so current14-root admission/decoder limits make the exchange unavailable and non-daily. Expand live/frozen owned offering capacity and dynamic journal admission status together. Hidden material/source/gift, potion use, pre-castle collection, actual castle key, unused black-key passage, gauntlet class/race lore, separate epic/memory and alternative foreign axe returns remain distinct. No native repair ships; selected future fixes require separate named commits and prominent PR/news treatment.

## Builder follow-ups and capability expansion

| Reference | Observed boundary | Planned qualification or fair repair |
|---|---|---|
| ZSQ-DRST-01 | Sixteen native item goals exceed the14-root admission and cold continuation limit; Q34 remains active/zone-completion eligible/repeatable but non-daily. | Expand durable offerings as a coordinated runtime/catalog/decoder/recovery change. Audit768-byte context and8192-byte continuation budgets, preserve versions1..6, freeze exact items/party/XP and test refusal, replay and cold recovery. Preserve the full recipe; use a separately named capability/fix commit and clear news before enabling hand-ins. |
| ZSQ-DRST-02 | Nine repeated34216 goals require nine different physical item instances across eight material types. | Keep repeated quantities explicit. Select independent owned UIDs without reusing one item, atomically retire the full set, and preserve an incomplete set. Test eight/nine/ten pieces, duplicate UIDs, contention and spent/reacquired proof. |
| ZSQ-DRST-03 | All eight material prototypes are hidden; current offering selection needs visible loose inventory. | Separate successful SEARCH, recovery, visible preparation and accepted history. Test hidden/worn/held/nested omissions, independent source custody and current stock; attempted SEARCH or keywords are not receipts. |
| ZSQ-DRST-04 | Exact items supplied by another player satisfy native material identity without personal recovery requirements. | Provide source generation/UID/custody episode attribution for optional first-source milestones. A gift can prepare an offering without awarding a personal source kill or recovery. |
| ZSQ-DRST-05 | Mobile/object identities collide within34200..34230; ruby34203 is not spider34203, meteorite34216 is not mercenary master34216. | Keep typed namespaces, exact prototype identity and stable native recipe IDs in authored dependencies. Never infer material meaning from the number or a similar name alone. |
| ZSQ-DRST-06 | Tinkerer34221 has three M topics hi/task/list; completion does not test prior dialogue. | Group actual successful responses and aliases. Future actor-specific conversation objectives need committed response evidence; encounter or attempted words do not add completion prerequisites. |
| ZSQ-DRST-07 | Gauntlet34217 has allowed-class flags and no ordinary race exclusions, while dialogue says only mountain dwarves/duergar. | Settle builder intent before a separate prose or equipment-policy fix. Preserve existing class/specialization behavior and avoid imposing an invented race condition on the native exchange. |
| ZSQ-DRST-08 | Blood34212 is both a requested material and a level50 armor/barkskin potion; QUAFF consumes it. | Show preservation advice and current spent proof. Future use/retirement objectives need owned successful effect/consumption outcomes and replay handling; a drank vial does not count as a collection return. |
| ZSQ-DRST-09 | Reset1 stock uses world caps/chance100; material34216 has cap9 and nine O entries, while source NPCs carry selected single materials. | Qualify durable epoch/reset-generation issuance and recovery before promising fresh supply. Preserve shared caps, exact parent stock and slot18 castle-key equipment; daily rollover does not restock the zone. |
| ZSQ-DRST-10 | Raw WLD5 storage/tunnel exits lose high bits; local D resets are closed1, with no secret D resets. | Use loader/reset/current shared state. Do not advertise a required SEARCH/fireplace trigger to enter these rooms merely from their titles; review desired secrecy separately with builders. |
| ZSQ-DRST-11 | Castle34382N/34383S is pickproof and locked2, key34218 worn by guardian34204; all collection sources are reachable before it. | Keep castle recovery/unlock/open/arrival optional to the collection. Qualify exact key, stock, sight/occupancy and shared state; ownership refusal can retain a key after unlock, so access and retirement need separate outcomes. |
| ZSQ-DRST-12 | Black key34230 is referenced by reciprocal34351S/67138N raw4 exits without D resets, so they currently load as ordinary passages. | Record incomplete or historical gate intent without inventing a present blocker. Any future door/secrecy/lock change requires a separate builder-approved access change with both directions and current travel covered. |
| ZSQ-DRST-13 | Only34226 has an enabled local object binding;34200/34300 board assignments are wholly commented out. | Preserve effective dispatch. DwarfSlayer requires a live actual wielder, melee hit and dwarf/duergar target with1/25 spell proc; effects or legacy handler booleans do not become quest achievements. |
| ZSQ-DRST-14 | King34236 carries epic358 and memory55451 alongside equipped axe/crown/sash; epic touch has distinct durable group rewards. | Track recovery, current ownership, eligible co-located touch recipients, committed reward and optional reset separately. Resetmode1 does not request the resetmode0 immediate reset; memory possession does not complete the tinkerer story. |
| ZSQ-DRST-15 | Ragmor83336/Wikzor83439 have departing alternative five-weapon returns; memory goes to WH ambassador55283 in a separate staying request. | Keep physical allocation and native receipts distinct. Outside prose says two tokens while numbers give warhammer83485/token83463; confirm intended wording before a separate prose fix. Staging ambassador stock is not a qualified public circulation route. |
| ZSQ-DRST-16 | Alatorin resets gauntlets34217 in locked display83367@83569 with key83371; possession can come from outside the collection. | Keep current gauntlets and native collection history separate. Preserve shared container/key/recovery rules; display-case torque prose needs contextual builder review because the prototype serves several cases. |
| ZSQ-DRST-17 | Crown lore calls the axe DorfSlayer; cloak/sword extra descriptions differ from their actual names, while court invitation/alliance lore lacks a separate native quest. | Review intended names and prose with builders, then use separately named text fixes with exact before/after and prominent PR/news treatment. Do not change equipment power, invitation gates or court aggression based only on narrative mismatches. |
| ZSQ-DRST-18 | Preparation can be ready while native admission is unavailable; current prose explains the capacity limit. | Add universal runtime capability/admission status to journals so unavailable reasons and prerequisites update from effective authority rather than permanent zone prose. Test capacity upgrades, inactive/unready authority, current stock, receipt recovery and dependent gates without changing native contract identity. |

## Charcoal Palace: typed materials and conditional route guidance

The [complete Charcoal Palace dossier](../design/zone-stories/CHARCOAL_PALACE.md) maps one two-material forging return, nine contacts, three effective responses/nine aliases and twenty follow-ups. Two optional loose-material rows remain distinct from one accepted receipt. Active READY accounting remains required; native one-achievement/one-potential-daily classification is preserved. Expand owned ordered switch→search→arrival outcomes, shared key/access/retirement, hidden source/gift attribution and effective journal capability status before promoting guidance to mandatory steps. Guard/helper dispatch, thermal/current behavior, two unreachable local rooms, epic/pool/equipment services, alternative Ra recipes with conditional Navift availability and independent nine-item cloak return remain distinct. No native repair ships; selected future fixes require separate named commits and prominent PR/news treatment.

## Builder follow-ups and capability expansion

| Reference | Observed boundary | Planned qualification or fair repair |
|---|---|---|
| ZSQ-FIREP-01 | Wardstone USE removes blocking but leaves secrecy; SEARCH and arrival are later actions. Chain PULL has a different non-secret branch. | Add ordered owned success contracts with exact target exit and before/after state, then search success and arrival. Distinguish already-open/refused/invalid handled commands and access shared by another actor. |
| ZSQ-FIREP-02 | Four keys88308/88324/88325/88302 address separate gates; native key lookup supports loose or HOLD, and unlock precedes retirement. | Model current shared lock/open state separately from personal recovery and committed key retirement. Test reciprocal endpoints, exact key, equipped HOLD,100-percent retirement attempts with refusal, durability0 and replay. |
| ZSQ-FIREP-03 | Scales88322 and blank88323 are secret stock, with source custody differing from visible loose offerings and gifts. | Use owned UID/source-generation/custody outcomes for optional first-source achievements. Test reveal/get/gift/spent/reacquired proof, equipped/nested/hidden exclusions, visibility and quantity. Preserve acceptance of supplied exact items. |
| ZSQ-FIREP-04 | Q33 consumes two exact items for two separate rewards; giver stays and no source/access/topic prerequisite exists. | Keep one stable native receipt and atomically freeze both roots and reward obligations. Exercise incomplete/duplicate/contended ownership, reward capacity, publication refusal, replay and cold recovery without fabricating route completion. |
| ZSQ-FIREP-05 | Resetmode0, Borren50-percent appearance, vault80-percent stock and shared caps do not guarantee another set each day. | Report effective stock and active READY capability where supported. Keep daily eligibility independent of restock; separate normal reset-generation issuance from the reset requested by a committed epic touch. |
| ZSQ-FIREP-06 | Typed identities collide: mobile88318 is Hephaestus/object88318 chain; mobile88322 magi/object88322 scales; mobile88323 minion/object88323 blank. | Require typed namespace and exact prototype in builder dependencies, parser diagnostics and source contracts. Never turn the minion or magi into a required material-source kill by matching only a number. |
| ZSQ-FIREP-07 | Three MA/MA/M responses declare11 alias occurrences; prepend matching makes private create/forge/items shadow public duplicates, leaving9 effective words. | Expose effective response families and audience. Future conversation steps require actor-specific committed successful response evidence; encounter, attempted keywords and bystander room echoes do not award keyword completion. |
| ZSQ-FIREP-08 | Eleven charcoal_guard types are non-periodic; SCAN_COMBAT5, memory, path and InitNewMobHunt constraints determine actual pursuit. | Qualify dispatch, combat target, successful hunt scheduling and refusal/home outcomes before custom combat objectives. Preserve helper limitations; a handled/false boolean is not an owned outcome contract. |
| ZSQ-FIREP-09 | Mobile88329 block_dir has no directional keywords; Fruaack has _block_up_ but only fruaack_shout is assigned. | Ask builders whether these are historical or incomplete barriers. Preserve present access. Any selected keyword/dispatch/barrier repair needs a separately named fix/news commit with ordinary/trusted traffic and both directions verified. |
| ZSQ-FIREP-10 | Kossuth world helper limit6 can already be exceeded by initial88323 cap11 stock; no initial local room is SINGLE_FILE. | Distinguish successful custom spawn, world count, source generation and conditional widening from combat attempts. Review intended helper/widening behavior separately; never promise guaranteed summons or a local widening achievement. |
| ZSQ-FIREP-11 | 142 rooms use fire sector; thermal entry/event includes race/trust/pet, ENJOYS_FIRE_DAM and protection removal branches. | Model actual actor environmental effects and survival with current capability, event, source room and committed result. Avoid treating a generic protection toggle, lore or arrival alone as successful hazard completion. |
| ZSQ-FIREP-12 | C10 4 metadata in two fire-sector rooms sets current speed/direction, but sweeping/refusal requires IS_WATER_ROOM, which excludes FIREPLANE. | Record effective current behavior and ask builders about intended magma movement. Do not infer climbing or silently activate water-current hazards; a selected mechanics change needs separate balance and travel validation. |
| ZSQ-FIREP-13 | Boundary25415W enters88321 while88301W exits25415; kitchen88399 and fissure88448 are unreachable in the unrestricted local static graph. | Review asymmetric boundary and room intent, including custom travel, before adding passages. Preserve bounded static claims, shared access and actor survival; any selected topology fix belongs in a separate named fix/news commit. |
| ZSQ-FIREP-14 | Epic stone359 touch publishes a separate eligible same-zone group award, then may request resetmode0 reset. | Keep actual target, peace, level, recipient eligibility, owned award, publication and reset distinct from forging. Test rejection, replay, source-zone identity and reset after accepted publication only. |
| ZSQ-FIREP-15 | Dexterity pool61 requires PC level51/48-hour cooldown; common handler ignores command argument and can refuse or produce zero stat gain. | Add authenticated target selection and explicit refusal/heal/stat delta/cooldown outcomes to future service contracts. Settle intended argument behavior before a separate service repair; preserve existing effect and stat limits. |
| ZSQ-FIREP-16 | Zion, Flame of the North, Ra and rogue stiletto have different owner/equipment/speech/event/melee/death effects; CheckMultiProcTiming immediately returns true. | Describe actual dispatch and timers. Review the disabled guard with balance owners before any change, and never infer effective debounce from its name. Effects, starter-kit possession and spell success are not forging receipts. |
| ZSQ-FIREP-17 | Sootfoot55103 and Navift74254 have alternative five-input Ra returns; Navift has no ordinary placement in reviewed sources. | Keep exact physical inputs, independent receipts and conditional giver availability. Integrate wider producer campaigns through existing dossiers; review Navift placement as a separate builder/service fix, without requiring personal Hephaestus history. |
| ZSQ-FIREP-18 | Aeolyn cloak88304 is equipped stock and one of nine items in departing north wind131616 return for Cloudseeker131650. | Show separate reward demand, actual recovery, exact nine-item admission, departing giver and its own durable receipt. Test one cloak cannot pay two exchanges and no dependency on the local forging return. |
| ZSQ-FIREP-19 | Current schema supports carried/equipped/completion rows, not owned switch/search/hazard/service outcomes or dynamic route admission. | Expand versioned builder contracts and journal capability/status presentation before promoting narrated route guidance to required steps. Preserve old maps and receipts, active READY accounting gating, current proof/history and raw fallback recovery. |
| ZSQ-FIREP-20 | Lore calls55270 essence of morning while the actual item is broken rays of morning Sunrise; inactive highdrop table does not confer a current bonus. | Use actual names plus lore explanation. Review wording and inactive historical code fairly; do not infer broken gameplay from a comment or silently enable rewards. Ship selected native repairs separately with exact before/after and prominent PR/news treatment. |

## Menden: spoken access and owned service outcomes

The [complete Menden-on-the-Deep dossier](../design/zone-stories/MENDEN_ON_THE_DEEP.md) maps one departing relic/coin return, eight contacts, three responses/four aliases and twenty follow-ups. One optional current-custody row remains separate from an accepted receipt; active READY accounting and native one-achievement/one-potential-daily classification are preserved. Spoken house unlock→OPEN→arrival, exact chest key/alternate PICK, paired pools, guarded figurine assertion/pet consumption ordering and reciprocal lore/reward mismatch require explicit owned outcomes or fair builder review. Altar and native shop mutations currently refuse under active accounting; expand dynamic availability, general treasure predicates, frozen RNG/effect/currency/pet/world-item publication and separate ship-service outcomes before counted episodes. Foreign Zorta and ravine receipts compete for relics and retain recipient-zone identity. No native repair ships; selected repairs require named commits and prominent PR/news treatment.

## Builder follow-ups and capability expansion

| Reference | Observed boundary | Planned qualification or fair repair |
|---|---|---|
| ZSQ-MENDEN-01 | Kitan Q19 holy4402→C100000/D1 promises restoring life; linked Zorta Q18 unholy88807→S113/D1 currently names cause light, while resurrect is88. | Review intended reciprocal rewards and present-day class/level eligibility with builders. Preserve both recipes until a selected repair has its own named fix/news commit, exact before/after and reward/persistence validation. |
| ZSQ-MENDEN-02 | Three Kitan M responses have four actual words, including historical resurection; dialogue contains name/gender and wording differences. | Keep accepted command spelling and explain actual responses. Select aliases/prose cleanup separately after intent review; never award each attempted keyword or infer a taught ability from text. |
| ZSQ-MENDEN-03 | Holy4402 is Zorta Gcap1@4610; unholy88807 is Kitan Gcap1@88860; both are armor and can be supplied by other players. | Add optional first-source proof from UID/source parent/reset generation/actor/party and committed custody. Keep gifts, current preparation, recovery history, spent/reacquired and the accepted receipt distinct. |
| ZSQ-MENDEN-04 | Local departing one-item/coin return has no source, house, prior conversation or kill prerequisite. | Preserve one native receipt and exact repeatable/daily classification. Qualify atomic root retirement, frozen wallet reward, giver departure, publication refusal, replay and cold recovery with active READY accounting. |
| ZSQ-MENDEN-05 | Mode1 resets/shared caps/departing recipients do not recreate relics or givers at daily rollover. | Expose actual supported stock/capability status and reset generation. Daily selection must not promise guaranteed supplies or issue a new relic from a journal read. |
| ZSQ-MENDEN-06 | House88855E/88856W key-2 uses successful SAY serpent, clears reciprocal LOCKED/SECRET and leaves CLOSED. | Add owned speech target/before-after state contracts followed by OPEN/arrival. Distinguish refused speech, already-open access, NPC speech and another actor's shared unlock; possession of ivory key is not this action. |
| ZSQ-MENDEN-07 | Ivory88810 is magus HOLD stock; chest88824 is closed/locked15 with key88810, not PICKPROOF16; figurine/staff are P stock. | Model exact key or eligible successful PICK, OPEN, container/source recovery and actual custody separately. Preserve valid alternate access and shared state; durability0 does not attempt key break. |
| ZSQ-MENDEN-08 | Figurine FLEX on actual HOLD/WIELD owner creates bugbear88813 before redundant unequip_char(pos=-1); guarded callee logs and returns, extraction later finds the actual slot. | Plan a separately named repair removing the invalid-slot assertion after focused equipment/pet lifecycle validation. No memory-safety fault or duplication is inferred; first qualify owned retirement and durable pet issuance ordering. |
| ZSQ-MENDEN-09 | Figurine and altar pets use setup_pet/follower links and custom death prose; active pet creation lacks a journal-owned success receipt. | Add exact actor/subject/source UID, successful creation/ownership/lifetime, frozen recovery and pet outcome contracts. A handled FLEX, figurine custody or death narrative cannot count pet issuance or a relic return. |
| ZSQ-MENDEN-10 | Active accounting refuses Llym altar before sacrifice, while new tracking requires active READY accounting. | Expose service unavailable in journals. Port held-root retirement and all effect/currency/pet/world-item publication atomically before enabling tracked altar steps; preserve current refusal and prevent legacy fallback. |
| ZSQ-MENDEN-11 | Altar accepts any nonartifact ITEM_TREASURE of cost>=10000; either armor relic is ineligible despite its price. | Expand versioned builder predicates for type/value/artifact/equipped visibility rather than enumerating a misleading single mandatory prototype. Qualify actual admission with actor and target altar. |
| ZSQ-MENDEN-12 | Legacy altar blesses or grants random coins, then rolls1..700 for golem/hippogriff/items/vitality; later load failure can follow earlier benefit. | Freeze RNG and all conditional outcomes under one operation with owned sacrifice, rejected/no-op/partial publication/replay/cold recovery policy. TRUE/FALSE alone cannot classify sacrifice or every promised reward. |
| ZSQ-MENDEN-13 | Local88821/88827 pools are stationary paired floor routes88859↔88860 with damage7; value2=17 has no selected level-gate use. | Track authenticated ENTER selection and actual before/after arrival/damage; retain native clamping, visibility, survival and stock policy. Do not impose invented level17, house or spell prerequisites. |
| ZSQ-MENDEN-14 | Foreign4403/4404 pools are explicit magic_pool bindings4609↔4610 with damage200, despite their treasure type. | Qualify bounded actual source access and return separately from the wider Underworld campaign. Use selected handler/placement rather than inferring behavior solely from ITEM type or narrative. |
| ZSQ-MENDEN-15 | Room88860 has no exits; all71 local rooms are reachable only when selected house/pool routes are assumed successful. | Preserve conditional directed graph68/70/69/71 and distinguish current occupancy, stock, visibility and played arrival. Plan source/access journeys; no unexplained local topology repair is selected. |
| ZSQ-MENDEN-16 | Object88828 onyx pylon has target41023 but no ordinary stock or selected binding; default ENTER resolves exits, not arbitrary teleport prototypes. | Review whether unused prototype is historical or incomplete. Preserve current behavior; any selected placement/activation needs a separate balance/travel fix with return policy and prominent news treatment. |
| ZSQ-MENDEN-17 | M0 10 at temple88827 and M0 25 at house88856 have no selected loader effect; HEAL, NO_HEAL, PRIVATE and SINGLE_FILE flags have separate actual meanings. | Explain effective environment rather than infer magical regeneration or cliff fall/current from prose. Review metadata intent before a separately selected mechanics repair; preserve sight/speech/occupancy restrictions. |
| ZSQ-MENDEN-18 | All three native shop bodies resolve, but active accounting refuses buy/sell/peruse/repair/forge; LIST/context remains separate. | Expose actual availability and integrate durable trade/service outcomes before any shopping episode. Goods, alternate food stock in Alatorin and shop encounters cannot prove Kitan acceptance. |
| ZSQ-MENDEN-19 | Dock room88846 is ship authority/port550725; hull callback follows epic transaction but coin settlement/world ship save are separate, and six-command dispatcher can handle refusals. | Expand owned service completion across wallet/epics/hull/cargo/maintenance/save before ship episodes. Keep bounded dispatcher/admission review separate from full ship economy qualification and never use seller encounter or handled command as completion. |
| ZSQ-MENDEN-20 | Minopass spirit Q130 consumes both relics for symbol94726/XP250000/D1; Zorta and Kitan have separate recipient zones. | Keep independent native receipts, competing physical roots and outside availability; no mandatory three-return sequence or duplicated local credit. Larger journals use the existing ravine dossier and later Underworld mapping. |

## Mitashi: effective bindings and owned outcomes

The [complete Mitashi dossier](../design/zone-stories/MITASHI.md) maps one departing six-sword return,14 contacts,three responses/four aliases and22 follow-ups. Six optional loose-custody rows remain separate from the accepted receipt; active READY accounting and one-achievement/one-potential-daily classification are preserved. Hidden source/search,actual equipment flags,current presence versus actual offerability,competing Savannah/Air uses and generic Retribution effects need owned outcome contracts. Computed inn/generic shops require effective binding and actual successful service status; poison/stable/garden/follower lore does not create an implemented endpoint. No native repair ships; selected intent/prose/keyword/type/travel fixes require named commits and prominent PR/news treatment.

## Builder follow-ups and capability expansion

| Reference | Observed boundary | Planned qualification or fair repair |
|---|---|---|
| ZSQ-MITASHI-01 | One six-item Q48 consumes138267..138272 for138279/D1; current READY accounting keeps incomplete sets with the actor. | Keep distinct root identity, exact AND quantities, atomic retirement/reward/departure and frozen recovery. Qualify six-root rejection, publication refusal, replay and cold recovery; do not count six copies of one sword or six independent completions. |
| ZSQ-MITASHI-02 | Three responses use four words: sword/swords, torment, empress. | Add owned addressed-response contracts only after actual target/response dispatch. Retain aliases and no-op/refusal state; each keyword attempt cannot be an achievement. |
| ZSQ-MITASHI-03 | Six source parents mix four WIELD declarations with two SECRET carried swords, all cap1/chance100. | Expand optional first-source UID/parent/reset-generation/actor/party contracts. Distinguish source recovery, gift, current custody, spent/reacquired and accepted receipt; personal kills are not native requirements. |
| ZSQ-MITASHI-04 | Lotus138270 and Torrential138272 are SECRET; full SEARCH may reveal one eligible container/corpse item by chance. | Capture selected container/source and committed before/after reveal separately from GET ownership. Expose retry/blocked/visibility status without credit for a failed SEARCH or another player's shared reveal. |
| ZSQ-MITASHI-05 | Journal snapshot counts directly carried prototypes without sight or UID ownership checks; native selection checks visibility of the initiating offering, then matches other loose roots without another sight check. | Expose current presence separately from actual offerability. Add versioned sight/UID ownership/busy/capability status and commit-aware snapshots; test hidden initiating versus other hidden loose roots. Preserve native admission and accepted receipts until a separately selected implementation. |
| ZSQ-MITASHI-06 | Input extra83008/87104 is NOREPAIR/ALLOWED_CLASSES/NORESET/NOIDENTIFY with optional SECRET; extra2 marks QUESTITEM. | Use loader field names and actual BIT values. NORESET is unused selected runtime metadata; do not prescribe curse removal, declare unlocatable items or deny fresh stock from a misdecoded flag. Review current mutable flags independently. |
| ZSQ-MITASHI-07 | North Wind supports WIELD/HOLD; remaining inputs WIELD. Active REMOVE requires one eligible item at a time and may reject current enchant/curse/capacity. | Add successful owned equipment-to-inventory outcomes and actual capability status. Gear eligibility is separate from loose offering admission; preserve valid supplied swords and alternate equipment use. |
| ZSQ-MITASHI-08 | Mode2 shared stock and departing giver do not guarantee daily availability. | Expose supported source/recipient availability and reset generation. A journal read or daily rollover cannot mint blades, restore Kunji or promise a full set today. |
| ZSQ-MITASHI-09 | Revenge, seven original lords and Empress control are narrative; no local Empress endpoint or later combat objective is declared. | Let builders choose an explicit versioned campaign extension or retain the lore. Require exact actors, durable combat/outcome gates and balance review before presenting release or revenge as implemented stages. |
| ZSQ-MITASHI-10 | Six Savannah base exchanges and six matching Retribution upgrades compete for the same physical swords; Air Q98 also consumes North Wind. | Present branch choices and recipient-zone receipts. Link existing dossiers; optional earlier history cannot replace currently required roots. A chosen fresh six-epic collection costs42 clan swords, six Kunji and six Lynstar appearances, without imposing that route on supplied equipment. |
| ZSQ-MITASHI-11 | Savannah base prose repeatedly requests a lyre, while native upgrade terms require the matching instrument; its sister endpoint is already absent from active bindings. | Retain existing Savannah follow-ups. Select prose/endpoint intent separately, with a named fix/news commit and exact native before/after if a repair is chosen; no extra Mitashi completion is inferred. |
| ZSQ-MITASHI-12 | Retribution216015015/30/30 packs cure critical15 twice plus spirit anguish216, level30,1-in-30 trigger. | Qualify actual legacy/scheduled action settings, target continuity, refusal, effect resolution and recovery. Chance denominator is not a cooldown; healing message or handled melee proc does not prove successful healing/damage or story completion. |
| ZSQ-MITASHI-13 | Full selected graph229;217 without OPEN,220 without water. Barracks are three closed/unlocked pairs; all sword leaders and Kunji have land routes. | Add selected owned OPEN/arrival contracts and current combat/sight/occupancy eligibility. Preserve alternate paths and distinguish shared open state from personal action. No unexplained local topology repair selected. |
| ZSQ-MITASHI-14 | Northern gate138200→647688 and incoming647288→138200 are nonreciprocal; southern bay edge138409↔648088 is reciprocal. | Review surface approach intent before any separately named travel repair. Current movement is directed; static connectivity is not a played round trip or a promise of reciprocal exits. |
| ZSQ-MITASHI-15 | Bay138401..409 is WATER_NOSWIM; docks have DOCKABLE flags without selected local ship-shop authority. | Use actual boat/flight/levitation/mount/wraith/altitude eligibility. Dockability alone cannot create ticket, hull purchase or ship service receipts; broader ship economy remains a separate qualification. |
| ZSQ-MITASHI-16 | ROOM_INN138426 computes inn even though the explicit assignment index is empty; RENT saves terminal state and rolls home back on failure. | Expand effective binding discovery beyond literal assignments. Track successful terminal save/exit separately from arrival, handled refusal or text; preserve combat/PvP/visibility/raid/character admission. |
| ZSQ-MITASHI-17 | Four SHP bodies bind generic shopkeepers; active accounting refuses buy/sell/peruse/repair/forge. | Expose unavailable service state and port durable trade/service publication before counted shopping episodes. LIST, stocked goods and an NPC encounter do not prove purchase or payment. |
| ZSQ-MITASHI-18 | Le Ming says the poison service is closed; no selected shop/special binding or vial issuance exists. | Review intended poison application design with builders before a separate mechanics/balance fix. Preserve current closure; do not enable the service or recommend harmful potions as an incidental journal repair. |
| ZSQ-MITASHI-19 | Askume138250 has vial ajida keywords despite Askume name; Xial picks are weapons rather than ITEM_PICK. | Select keyword/prototype intent cleanup separately, with named repair/news treatment and command/type regression. Preserve current types and effects while presenting reliable guidance. |
| ZSQ-MITASHI-20 | Can Hy stable has no selected mount service; Lilari/noble F followers are NPC scene stock; Harmony Garden has no declared flower quest/ward. | Keep scene orientation separate from owned purchase/escort/collection outcomes. A sidecar needs exact successful event contracts and builder endpoints; do not import another zone's magical garden prerequisite by name. |
| ZSQ-MITASHI-21 | CHAOS cleric kit includes138254; support bundle includes two138280 scrolls. These are alternate configured issuance contexts. | Retain UID/source/owner and configured profile identity. Starter stock or a scroll name cannot prove local first-source recovery, purchase, curse removal or Kunji acceptance; do not require these ordinary goods. |
| ZSQ-MITASHI-22 | Runtime journal snapshot174..178 counts all loose prototypes and equipment without visibility or durable UID checks; Ready now is preparation,not authoritative offering admission. | Expand presence/offerable/blocked/unsupported status with actual sight,ownership,operation and target eligibility. Keep a builder-selected predicate aligned with native admission; preserve valid alternate source/gift routes and partial-root refusal. |

## Dark Stone Tower: source containers and admitted outcomes

The [complete Dark Stone Tower dossier](../design/zone-stories/DARK_STONE_TOWER.md) maps one departing rhinestone return with two rewards,12 contacts,three addressed families/six aliases and22 follow-ups. One optional current-count row remains separate from acceptance; READY active accounting and one-achievement/one-potential-daily classification are preserved. Dragon-egg OPEN/SEARCH/GET, generic TOUCH/ENTER/STARE/PULL travel, exact keys/secret doors/water/fall/current, normal-boot teachers and node dispatch require owned outcomes and dynamic admission. Epic training refuses under active accounting, and Aerlyn's departure affects availability. Mining reports/resurrection/rescue lore has no selected endpoint. No native repair ships; cave topology, lore/outcome, old mace and weight/trap intent reviews precede any named fix/news commit.

## Builder follow-ups and capability expansion

| Reference | Observed boundary | Planned qualification or fair repair |
|---|---|---|
| ZSQ-TEKA2-01 | Q32 takes75559 for75560 plus75561 and D1 departure. | Preserve one native receipt, one achievement and one potential daily; qualify atomic single-root retirement, two-reward issuance, publication refusal, departure, replay and frozen/cold recovery. Reward counts are not independent quests. |
| ZSQ-TEKA2-02 | Three addressed responses use six aliases; three qc_action80 rows are random ambient CMD_NONE messages. | Add owned addressed-response outcomes after successful actual target/response dispatch. Ambient text, aliases, failed attempts and replay cannot create independent keyword achievements. |
| ZSQ-TEKA2-03 | M278 dragon75524/G281 egg75542/P282 stone75559/P283 blood75529 are cap1/chance100 declarations. | Add optional first-source UID, parent/container path, reset generation, actor and party ownership. Distinguish source recovery, supplied gift, current custody, spent/reacquired and accepted receipt; personal kills are not native gates. |
| ZSQ-TEKA2-04 | Egg flags5 is closed/unlocked/closeable, key0; SECRET rhinestone extra45057 includes NOLOCATE. | Capture successful OPEN, selected-container SEARCH reveal and committed GET separately. Preserve RNG, sight and shared state; a successful reveal by another player cannot prove your own reveal/recovery. |
| ZSQ-TEKA2-05 | Runtime snapshot counts loose prototypes without CAN_SEE or UID ownership; initiating native offering must be visible and owned. | Expand versioned preparation/capability status for hidden/ownership/held/nested/blocked roots. Ready now is raw presence until runtime admission is represented; a sidecar cannot silently alter native turn-in rules. |
| ZSQ-TEKA2-06 | Mode1 shared cap-one source and departed Aerlyn need not be available on daily rollover. | Expose source/recipient/reset-generation availability. A new daily slot does not regenerate items, restore the trainer or satisfy a source or departure receipt. |
| ZSQ-TEKA2-07 | Aerlyn promises to raise a long-dead Ruzdo mage; native result is two items and her departure. | Let builders choose clarified prose or an explicit versioned resurrection campaign. Require exact actor, durable spawn/state/effect outcome and balanced prerequisites before displaying a resurrection finale. |
| ZSQ-TEKA2-08 | Ilbe asks to report the relic so work can stop; no selected report/stop-work endpoint exists. | Review intended report and worker outcome with builders. Add an owned callback/state transition only as a separately selected campaign extension; the ASK response is not a completed report. |
| ZSQ-TEKA2-09 | Four ITEM_TELEPORT objects are dispatched by interpreter/check_item_teleport without literal bindings. | Extend effective capability discovery to generic type/command dispatch. Use TOUCH altar, ENTER mirror, STARE orb, PULL board; value1 is command rather than damage, value2=-1 unlimited charges. Capture successful actual arrival and admitted actor, not a handled refusal. |
| ZSQ-TEKA2-10 | Door loader masks raw fields to low two bits; reset states control CLOSED/LOCKED/SECRET. | Keep actual lock/pick/search flags, reciprocal/shared state and key or eligible alternate access. Raw secret4 without a reset does not create a search gate. Add owned access outcomes without treating another player's shared unlock as personal progress. |
| ZSQ-TEKA2-11 | Graph from75546: OPEN10,keys11,portals15,keys+portals94,secret access96;without water83. | Qualify actual travel visibility, arena/state/destination admission, combat, source stock and round trips. The graph assumes successful actions and is separate from played access. Preserve supplied relics and eligible alternate travel. |
| ZSQ-TEKA2-12 | Rooms75577/78 have outgoing links but no selected incoming exit or portal; unused75579 has no dangling reference. | Review intended cave connection with builders before a named topology fix. Retain existing paths until intent is selected; an unused numbering gap alone needs no repair. |
| ZSQ-TEKA2-13 | Golden75510/ivory75527/priest75564/prison75517/iron75531/small75562 keys have distinct holders and doors. | Show exact access context and current key custody, visibility, loose/HOLD eligibility and durability. Recovering a key or opening a branch is optional guidance, not an additional Aerlyn offering. |
| ZSQ-TEKA2-14 | F10 above mirrors75581 has DOWN75556; river75596/97 flows east and75598 west with C10 and water sector16. | Add owned fall/current/arrival outcomes with survival/flight/levitation/water admission. Retain command and entry current checks; cold lore does not create a temperature requirement or guarantee a hazard-free route. |
| ZSQ-TEKA2-15 | Normal boot epic_initialization assigns Aerlyn chant mastery and Osmirim sneaky strike; purchases refuse under active accounting. | Add durable combined epic/coin/skill publication and dynamic availability before counted training. Retain configured minimum(default56), full skill name, class/cap/prerequisites and growing prices. Teacher max100 is not a price or player level. |
| ZSQ-TEKA2-16 | Aerlyn is both quest giver and epic teacher, and accepted relic return removes her. | Present the availability choice explicitly. Practice display, refusal or future training is separate from quest acceptance; daily rollover cannot recreate the teacher. Builder intent review precedes any departure or trainer relocation repair. |
| ZSQ-TEKA2-17 | Mace75561 packs blindness4/firelance637,level40,1-in-25 generic trigger; old sunray procedure binding is commented. | Qualify configured legacy/scheduled item-action outcomes and actor/target continuity. Preserve packed settings and inactive binding; review retirement/intended effect separately, with a named mechanics/news commit if selected. Flame prose/handled proc is not successful damage or another quest. |
| ZSQ-TEKA2-18 | Imported rune358 in crypt binds epic_stone; imported memory55454 has _noquest_ keyword and MAGIC/QUESTITEM metadata. | Retain distinct node, participant, zone-touch and native receipt authority. TOUCH/visit/memory possession cannot prove an epic payout or Aerlyn acceptance; qualification of broader touch settlement remains separate. |
| ZSQ-TEKA2-19 | Prison demons, Lashan's soul, Itzkar and12F scene-following rows have no selected rescue/escort/resurrection endpoint. | Use scene orientation while builders explicitly map any intended campaign. Track owned escort/control/rescue/state outcomes before counted stages; NPC reset following does not establish player ownership. |
| ZSQ-TEKA2-20 | Alatorin exports blood75529 to two flesh golems; typed staff83179 differs from mobile83179. | Keep exact typed identity, parent and configured source context. Exported potion stock is not rhinestone75559 or source history; broader Alatorin campaign remains in its existing dossier. |
| ZSQ-TEKA2-21 | Egg weight-30 and robe weights-40/-60 are legacy container data; trapped-soul staff75555 is ITEM_OTHER with trap metadata. | Review weight/trap intent fairly before any balance or type repair. Names do not prove weapon/wand capability, and negative container weight is not automatically corruption. Selected repairs require before/after validation and prominent news. |
| ZSQ-TEKA2-22 | No local shop or inn is selected;81 SECT_INSIDE rooms gain indoors/no-precipitation, mobile race-row final value is size. | Keep current field semantics and ordinary trainer/room lore separate from effective service outcomes. Do not invent shopping, renting, direction blockers or cold gates from names/numeric misdecoding. |

## Troll Hills: source equipment and optional outcomes

The [complete Troll Hills dossier](../design/zone-stories/TROLL_HILLS.md) maps one departing idol return with item/experience rewards, three contacts, three addressed families/eight aliases and 20 follow-ups. The optional loose-count row remains separate from durable acceptance;READY active accounting and one-achievement/one-potential-daily classification are preserved. Source HOLD recovery, secret SEARCH→OPEN, mound falls, optional desk money and wand/potion effects need owned outcomes. Exact supplied idols fit without invented personal source or use gates. Concrete fair fixes: malformed1820 metadata andstale bridge binding/mismatched destinations/active-accounting payment;candidate cloud portal andold-man inscription need builder intent. No native repair ships;selected fixes require named commits, prominent news and before/after qualification.

## Builder follow-ups and capability expansion

| Reference | Observed boundary | Planned qualification or fair repair |
|---|---|---|
| ZSQ-TROLL-HILLS-01 | Q23 consumes idol1804 for goo1807/E5000 and D1 departure. | Preserve one receipt, one achievement and one potential daily. Qualify atomic root retirement, item/experience publication, departure, rejection/replay and frozen recovery. Item count and experience are not two quests. |
| ZSQ-TROLL-HILLS-02 | Three addressed families use eight aliases: hello/hi/greetings, they, statuette/idol/description/details. | Add owned successful addressed-response dispatch outcomes. Aliases, failed attempts and repeated responses cannot multiply keyword achievements; conversation is not a native prerequisite. |
| ZSQ-TROLL-HILLS-03 | M72 magi1804@1873/E73 HOLD idol1804/G74 key1803 declare source stock. | Capture first-source UID, reset generation, source equipment path, actor and party ownership after committed recovery. Distinguish supplied gift, current custody, spent/reacquired and accepted return; personal combat is not a native recipe gate. |
| ZSQ-TROLL-HILLS-04 | Idol is ITEM_WAND3, NOIDENTIFY, TAKE/HOLD, MAGIC/QUESTITEM;its prototype also matches the native offering. | Use typed identity and separate lifecycle capabilities. Holding, charges or successful snailspeed cannot substitute a loose visible owned root or create an additional acceptance; hand-me-down exact idols remain eligible. |
| ZSQ-TROLL-HILLS-05 | Current journal snapshot counts loose prototypes without visibility or UID ownership admission. | Add versioned admission-aware preparation state before Ready now promises offerability. Retain loose/held/worn/nested and hidden/ownership refusal distinctions without changing native acceptance. |
| ZSQ-TROLL-HILLS-06 | Secret1868N begins state5 while1869S begins state1;full graph140 with OPEN, 148 with SEARCH+OPEN. | Capture eligible personal SEARCH reveal, OPEN and actual arrival separately from shared door state. Preserve alternate admitted routes; another player's access action is not your personal discovery. |
| ZSQ-TROLL-HILLS-07 | F40 at ladder1870/1871 has valid DOWN1871/1872. | Qualify fall scheduling, flight/levitation/climb/state, survival and resulting arrival before counted hazards. A destination, attempt or fall message alone is not a durable stage. |
| ZSQ-TROLL-HILLS-08 | Desk1802@1875 is flags13/key1803;P18 gold1805 declares250 gold. | Keep optional key/PICK→OPEN→committed money recovery separate from idol source and quest currency. Preserve actual sight, key durability, skill/tool/cooldown/shared state; require owned wallet publication before treasure credit. |
| ZSQ-TROLL-HILLS-09 | Wand 1804 has power36/four charges/snailspeed198;generic legacy and configured item-action paths differ. | Qualify actual charge consumption, actor/target continuity, resolved save/effect and scheduled refusal/recovery. Successful use is optional context, not an invented turn-in prerequisite or effect receipt inferred from a handled command. |
| ZSQ-TROLL-HILLS-10 | Goo1807 is coldshield131 potion;quaff can be blocked, spilled, suppressed or consumed without a new shield. | Capture root consumption and actual effect separately with timer/combat/NO_MAGIC/conflicting shield conditions. Reward issuance and potion custody do not prove coldshield; no counted drinking stage exists now. |
| ZSQ-TROLL-HILLS-11 | Literal bridge assignment targets missing1919;actual half-breed1819 resets on1812. | Add effective binding validation against existing typed prototypes and final assignment order. real_mobile0 miss writes index0;do not claim a valid local handler or a harmless dummy. Review coherent actor and bridge intent before a separately named repair. |
| ZSQ-TROLL-HILLS-12 | bridge_troll selects1862/1864 only at1863, otherwise foreign14236/14238;actual actor is at1812. | Review intended bridge directions and destinations before activation. Require valid admitted destination, actor/location continuity and owned successful crossing. Repointing one assignment alone is not a complete fix; preserve current routes pending selection. |
| ZSQ-TROLL-HILLS-13 | bridge_troll checks immediate wallet delta after do_give;active accounting refuses NPC coin gifts. | Add a durable NPC-service payment/arrival contract before paid bridge stages. Freeze price/recipient/destination;ensure refusal, replay, recovery and refund/publication semantics. Do not use immediate balance changes as asynchronous payment receipts. |
| ZSQ-TROLL-HILLS-14 | Room1820 has an extra tilde before18/33554436/3;current loader skips its intended flags/sector. | Plan a separate named world-data header fix after confirming forest/TWILIGHT/NO_MOB intent. Current MEMCHK zeroed allocation becomes inside with indoors/no-precipitation. Add loader-level before/after checks and prominent news;do not silently normalize journal evidence. |
| ZSQ-TROLL-HILLS-15 | Monolith1929 inscription asks for an old man without a selected local endpoint. | Let builders identify the intended campaign, recipient and explicit owned activation/report/state reward. Keep an unresolved exploration lead until mapped;no universal old-man or corpse-recovery completion can be inferred from text. |
| ZSQ-TROLL-HILLS-16 | Airp cloud portal131641@131652 declares target1823 with command0/charges0 and no selected literal binding. | Treat it as candidate incoming context. Review intended command and travel balance with builders;generic ENTER7 is not command0. Any restoration needs a separate mechanics/news commit and actual admitted round-trip qualification. |
| ZSQ-TROLL-HILLS-17 | Three reciprocal boundaries connect surface and Ghore;Lortower134000E enters1883 only. | Retain directed cross-zone source and recipient identity. Capture successful actual entry and return availability without assuming reciprocal exits or whole foreign campaign qualification. |
| ZSQ-TROLL-HILLS-18 | Water eligibility changes graph148 to124 on road/marsh entries;Ghore1909 cannot leave in the land variant. | Present route-specific swimming/flight/state admission and current availability. Source and recipient remain reachable by the selected land route after secret access;water or personal source history is not a native return gate. |
| ZSQ-TROLL-HILLS-19 | TWILIGHT fog prose differs from BLOCKS_SIGHT;ladder SILENT, mound NO_TELEPORT, rivers NO_HEAL, monolith NO_MAGIC/HEAL are actual metadata. | Decode current flags and loader adjustments rather than prose. Preserve magic/visibility/speech/healing admission;no F/C current, inn orshop service is selected. Environmental state is separate from an owned quest outcome. |
| ZSQ-TROLL-HILLS-20 | Mode2 resets, source caps and a departed adventurer can change recipient/source availability independently of daily rollover. | Add reset-generation and recipient availability status. Daily eligibility cannot regenerate source stock or restore an NPC. Retain supplied exact offerings and durable historical receipt while builders select explicit longer campaigns. |

## Twisted Wood: optional camp and explicit campaign outcomes

The [complete Twisted Wood dossier](../design/zone-stories/DARK_AND_TWISTED_WOOD.md) maps one two-item faerie return, three contacts, three addressed responses/five aliases and 20 follow-ups. Two optional loose-custody rows remain separate from one accepted receipt; active READY accounting and one-achievement/one-potential-daily classification are preserved. Imported armor/lance compete with Pine Hollow returns anddeparture cleanup; the faerie's promised report/further reward needs explicit builder intent. Secret camp and nested belt/emerald treasure are optional. Expand dynamic admission/source availability, owned cross-zone choice/report/recovery/access and durable reset issuance before counted episodes. Roomcases alone do not establish class barriers. No native repair ships; selected fixes require named commits, prominent news and before/after qualification.

## Builder follow-ups

### 1. Q29 consumes16014+16016 for16313 without departure.

Preserve one transaction, one achievement and one potential daily. Commit both distinct roots, collar publication and acceptance once; qualify refusal, replay, interruption and frozen recovery.

### 2. Three addressed responses use five aliases.

Capture owned successful NPC response dispatch separately from attempts and aliases; do not require saved conversation for the existing native offering or multiply keyword achievements.

### 3. Pine Hollow warrior M265/E266 armor16014 and Auriam M258/G259 lance16016 declare source equipment.

Capture exact UID, NPC/reset episode and equipment recovery lineage after committed transfer. Supplied items remain valid; personal kill and first-source acquisition are separate optional history.

### 4. Loose journal preparation omits sight and UID admission.

Add dynamic admission-aware preparation state before promising offerability. Distinguish worn/held/nested/hidden/foreign custody without weakening native offering checks.

### 5. Two different imported materials are consumed together.

Keep exact typed prototypes and distinct roots; one material alone is insufficient. Multi-input does not imply saved sequential subquests, partial deposits or two completions.

### 6. Pine Hollow Q69 consumes armor; Q35 consumes lance.

Add optional cross-zone links and explicit branch/attempt policy if builders want allegiance. Do not infer mutual exclusion, source kills or mandatory foreign completion from narrative.

### 7. Foreign D1 returns retire remaining NPC equipment and inventory.

Project live source availability and warn about prior recovery before departure. Qualify both orders, supplied stock, absent/replacement episodes and cancellation; daily rollover cannot restore sources.

### 8. Pine Hollow Q35 consumes gold scale16015 and creates a new same-type scale.

Keep recipient-zone receipts, root consumption and new reward UID separate. Prototype equality does not establish uninterrupted custody or a shared faerie completion.

### 9. Faerie promises Auriam report and possible further reward, without a matching local terminal.

Builders choose prose clarification or explicit linked reporting/reward contract. Do not grant Auriam items or relationship state from the faerie receipt alone.

### 10. Native faerie accepts supplied exact items without kill/faction gate.

Preserve delivery eligibility while adding independent first-source or campaign history. Narrative dark-man defeat is not confirmed personal combat or alignment choice.

### 11. Secret16388W/16389E each reset state5; graph91 OPEN/97 SEARCH+OPEN.

Capture successful personal reveal, shared open state and actual owned arrival independently. Keep camp optional and preserve eligible alternate movement paths.

### 12. Corpse16333 contains belt16330, which contains three emerald16334.

Add nested current-custody/access projections and committed OPEN/GET evidence. Belt flags5 is closed/unlocked, key0; container presence or open state is not personal recovery.

### 13. Other corpse/barrel, sword and throne/crown stock has no native local quest consumer.

Keep ordinary optional treasure separate from quest receipts. Builders can map explicit treasure episodes with exact roots, stock generations and count policy without inventing corpse-recovery quests.

### 14. Obelisk16300, fallen paladin16330 and barbarian F49/F51 scene have no selected terminal.

Author explicit ritual/redemption/rescue/follower participants, success/failure and reward policy before counting. Reset followers are not a player escort or rescue contract.

### 15. Bloodyclaw16336 has SECRET/NOIDENTIFY/FLOAT, weight-2 and regeneration-themed prose.

Review intended negative weight and lore effect fairly; no selected binding establishes regeneration suppression. Select any balance/content repair separately with named fix/news and before/after tests.

### 16. Roomcases16392/16383 exist in guild_guard, but no local literal guard binding is selected.

Validate effective typed actor binding and birthplace at dispatch before advertising rogue/shaman barriers. Roomcase matching alone cannot manufacture a class prerequisite; review builder intent before a separate fix.

### 17. All100 rooms are forest; holding16397..99 have outgoing-only local routes.

Project normal source movement and actual current location separately from spawn owner. Holding names do not prove random scatter; preserve intentional staging and do not add public entrances without builder review.

### 18. Eligible Githzerai SHIFT from Astral19701 can randomly arrive16302.

Capture race/current-plane/combat/cooldown and actual successful arrival separately. Keep this conditional route optional; it is not a universal access requirement or native faerie stage.

### 19. Active reset_zone refuses item issuance before live placement.

Add durable reset-generation identity and owned O/P/E/G issuance before promising fresh-world stock. Qualify both imported materials and nested treasures under active accounting; preserve existing recovered stock and refusal safety.

### 20. Four reciprocal boundaries, two incoming-only Pine Hollow load links and foreign recipes are bounded.

Keep owner-zone/current-location/recipient identities distinct and retain wider dossiers. Qualify foreign arrival, source wandering, daily renewal and cold persistence without double credit or unsupported played claims.

## Underworld: owned pools, keys and exploration outcomes

The [complete Underworld dossier](../design/zone-stories/UNDERWORLD.md) maps one departing relic/conditional-skill return, six contacts, three responses/four aliases and 20 follow-ups. One optional loose-custody row remains separate from accepted receipt and actual new learning; active READY accounting and one-achievement/one-potential-daily classification are preserved. Zorta's resurrection/party-cleric promise differs from encoded actor S113 Cause Light. Pools, consumable green keys, palace PICK alternatives, treasure, forge/slave scenes and corpse hazards need owned resolved outcomes; foreign relic returns retain independent recipient identity. Expand frozen skill learning, source/gift, cross-zone choice, access/arrival/key break/corpse custody, dynamic stock and durable reset issuance. Fair sector/current, ignored M, unreachable placeholder and reward-lore decisions precede any separately named fix/news commit. No native repair ships.

## Builder follow-ups and capability plan

1. **Actual reward and lore.** Resolve the resurrection/party-cleric promise versus R S113 Cause Light and actor-only eligibility with builders. Choose clearer dialogue or an explicitly designed reward/recipient change. Any selected native fix needs a separate named fix/news commit, before/after eligibility tests and a played hand-in.
2. **Training outcome.** Add a durable skill outcome that freezes admitted actor eligibility, prior learned state and exact reward identity, then waits for skills persistence. Test ineligible, already learned, absent actor, changed specialization, replay and recovery. An accepted receipt does not prove new training.
3. **Conversation knowledge.** Publish owned addressed-response facts for resurection, payment and relic/kitan, distinguishing recognized aliases from successful dialogue and saved knowledge. Keep conversation optional unless a builder deliberately adds a native gate.
4. **Source versus supplied relic.** Record first acquisition by item UID, source episode, source NPC and transfer cause. Distinguish Kitan recovery, gifts, theft, trade, nested transfers and reacquisition. Preserve supplied exact-item acceptance; personal recovery needs an explicit optional objective.
5. **Competing relic branches.** Represent Zorta, Kitan and ravine spirit requests with independent recipient-zone identity and consumed-root choices. Test both orderings, departure cleanup, spent sources, stale preparations and alternative custody; do not impose an unencoded allegiance or required sequence.
6. **Pool access and return.** Publish an owned successful ENTER only after exact live floor-pool admission, frozen damage/destination and actual arrival. Qualify paired-pool availability, HP-one clamp, missing destination, failed transfer, restart and usable return route before adding a travel stage.
7. **Keys and key break.** Integrate exact loose/HOLD key use, successful unlock, reciprocal lock state and child key-break settlement. Green keys4440/4450 have 100-percent post-unlock break chance; sapphire/mithril/black keys do not. Test nested/wrong keys, repeated use and failed accounting settlement.
8. **Palace alternatives.** Model two palace locks and eligible PICK as alternatives, followed by OPEN and actual arrival. Keep the sapphire/mithril preparation, spare-key note, king battle and one-way escape tunnel separate from Zorta's receipt. Do not require both keys when PICK works.
9. **Search and shared access.** Distinguish personal successful SEARCH of the boulder passage from shared reveal/open state and movement. Rug treasury and other prose-concealed doors reset closed rather than secret. Test already revealed, blind, wrong direction, failed search and another player opening the way.
10. **Nested treasure.** Add current accessible treasure predicates for chest money, crate light orb, priestess desk gem, locked desk note and floor treasury money. Preserve container flags, exact keys, eligible PICK, OPEN, capacity and UID custody. A coin pile is not a quest delivery or repeated reward.
11. **Forge and slave stories.** Work with builders on explicit ore, blacksmith, human-slave and priestess outcomes. Mithril ore is scenery treasure without TAKE; no local forging or liberation contract exists. Design source recovery, recipient, attempt, custody and completion policy before counting those scenes.
12. **Hazards and corpses.** Integrate worm swallow/death/corpse custody and later drop or death release with corpse UID, original owner and durable death/item settlement. Roper/elemental hunts and one-shot piercer ambush require resolved actor-specific outcomes. Do not award a rescue from an attack or corpse sighting.
13. **Hammer effect.** Expose the actual owner-worn melee-hit lightning outcome with frozen trigger, target and damage settlement if a builder wants an artifact goal. The handler's dam1 gate is satisfied by CMD_MELEE_HIT1000; no broken proc is inferred. Test trigger/nontrigger, invalid owner, dead target and recovery.
14. **River sector and currents.** Review river/lake lore against actual sectors13/14/15/29/5 and thirteen C30 rows. Current handlers require water sectors, so these declarations alone do not establish swimming or current movement. Preserve travel and balance until a separately tested builder-approved sector/content decision.
15. **Ignored metadata.** Review seven legacy room M records with builders. The selected room loader has no M branch; the tokens do not establish regeneration or damage. Choose removal, clearer lore or a designed supported field with explicit behavior and separate fix/news qualification.
16. **Unreachable rooms and terminal exit.** Review rooms4626..4631 and4645, which lack a selected incoming local/boundary route. Exit4645N targets missing4646 and is discarded by renum_world. Preserve placeholders until intended access/retirement is established; room ID gaps alone are not defects.
17. **Effective teacher and encounter dispatch.** Retain separate qst_func despite Zorta's explicit func.mob=0. The epic-teacher table contains no selected local actor. Random encounters require a map zone; local Underworld has flags0. Test effective binding and admission before treating teacher labels or exported hazards as local campaign stages.
18. **Conditional arrivals.** Qualify actual Githyanki SHIFT PRIME, MIND TRAVEL and the bound Ixarkon veil96402 as optional random arrivals at4437. Preserve race/plane/cooldown, combat/no-teleport and exact target rules. Arrival credit must come from successful movement, not a candidate destination or issued command.
19. **Stock and reset issuance.** Implement durable reset generation/issuance before fresh active-accounting stock guarantees. Current reset guard refuses item loading before live placement. Project unavailable/departed/cap-suppressed stock without inventing replenishment; qualify O/P/E/G children, issued UIDs, restart and duplicate suppression.
20. **Daily and played qualification.** Exercise discovery, giver encounter, exact visible loose offering, independent foreign receipts, frozen skill/departure, replay, restart and both pool directions with active READY accounting. Daily rollover changes eligibility; it restores neither source items nor actors. Keep synthetic/source qualification separate from played evidence.

## Zalkapfaan: spoken access, roaming sources and owned outcomes

The [complete Zalkapfaan dossier](../design/zone-stories/ZALKAPFAAN.md) maps eight native returns as eight cards across four progression families, nine contacts, two addressed aliases and twenty follow-ups. Engineering, bounty and three two-material armor requests retain five potential dailies; two departing token-for-key returns and Zekrallin's two-token soul return remain story-only under resetmode0. Twelve optional custody rows never claim personal recovery or reusable consumed roots. Spoken-password unlock→OPEN→arrival, breakable keys, actual staged roaming, queen reset admission and conditional post-node reset are qualified by current dispatch. Commodore purchases and mining refuse during active accounting; pool/node effects need owned persisted outcomes. No native repair ships; selected data/quest fixes require separate named commits and prominent PR/news treatment.

## Follow-ups and builder decisions

1. **Consumed branches.** Represent consumed-root alternatives for three armor requests and the two golems versus Zekrallin. Preserve all eight independent receipts and five-daily/three-story-only classification. Do not impose an exclusive choice or a quest order that native contracts do not require.

2. **Source versus supplied materials.** Record item UID, source actor/container, original spawn episode and transfer cause for assembly, ears, requisition, scales and tokens. Distinguish gifts, theft, trade, corpse recovery, worn token removal, nested transfer and reacquisition. Preserve supplied exact-item acceptance.

3. **Durable native returns.** Freeze exact giver/binding, admitted visible owned loose roots, recipient eligibility and item rewards before publication. Test all eight receipts, duplicate delivery, departure cleanup, replay, absent actor and cold recovery. A receipt does not prove personal source work or newly obtained rewards.

4. **Armor assembly.** Show each exact two-input requirement with missing/current counts and its independent receipt. Native dialogue mentions hours, but no wait, partial deposit or additional fee is encoded. Any future asynchronous crafting job needs its own accepted state, cancellation/recovery and delivered outcome.

5. **Conversation knowledge.** Record addressed project and piety responses separately from SAY, alias recognition and a saved knowledge fact. Golem reward text reveals names; it does not automatically produce an owned password objective. Keep conversations optional until builders deliberately encode a gate.

6. **Spoken access.** Publish admitted successful word-key unlocks with actor, exact exit pair, keyword identity and before/after lock state. Follow with owned OPEN and actual arrival. Test wrong words, unadmitted speech, repeated unlock, reciprocal state, reset/relock and recovery; do not count raw speech.

7. **Physical key access.** Track current held/loose keys2732/2749/2750/2767/2773 and actual unlock/open/arrival. Virtue and copper keys break100percent after successful unlock; others have zero native break chance. Nested custody and historic ownership do not establish usable access.

8. **Conditional routes.** Keep two actual entry targets and the directed40/52/54 access scenarios explicit. Key branches and spoken-password branches differ; shared alternate approaches can avoid a gate. Do not require all religious returns to reach a scene or award progress from static reachability.

9. **Staged roaming actors.** Qualify queen/advisor/religious-advisor initial stock, movement through2784..2788, actual encounter, holding-room diversion and death. Existing non-sentinel STAY_ZONE actors can roam into ten scenes; six rooms without player incoming paths alone do not prove broken quests.

10. **Declared chance and reset admission.** Review queen M331 arg4=33 against current M admission requiring arg4==100 on ordinary resets unless forced. Initial boot force2 can admit the33percent roll. Prose1/16..1/2 is not the reset probability. Establish intended behavior with builders before selecting a named native fix.

11. **Dynamic availability.** Explain absent givers, departed story-only actors, consumed materials, holding-room stock, copied world recovery and pending authority. Mode0 omits ordinary timer scheduling but has a conditional post-node-touch DB reset path. Daily rollover neither issues stock nor guarantees a meeting.

12. **Accounting reset issuance.** Add a durable reset-generation identity and atomically publish admitted O/P/G/E equipment, pools, earth, nested ears and node/memory stock with source lineage. Existing active accounting guard declines item issuance before read_object; never count a declared reset as a successful acquisition.

13. **Charisma pool outcomes.** Record exact selected successful DRINK, level51 eligibility, shared48hour TAG_POOL, prior stat/health, frozen RNG, bounded result and persisted effect. Separate cooldown injury, healing, neutral or negative stat outcomes and positive gain; possession is no pool completion.

14. **Owned node touch.** Qualify commander-carried stone359 periodic zone identity, recovery to visible loose/floor custody, peaceful eligible TOUCH, exact UID and committed group award/reset request. Memory55189 is a separate souvenir. G issuance, possession and command text do not prove a committed node outcome.

15. **Epic teacher accounting.** The Commodore is bound through the epic teacher table, but current active accounting declines purchases. Add an atomic epic/coin payment plus frozen skill state and persisted learned outcome, dynamic availability and rollback/recovery. Do not bypass the current guard or invent class restrictions.

16. **Mining accounting.** Broken earth193 is generically bound to mine, which currently declines during active accounting. Add frozen resource depletion, pick/skill eligibility, timed work, output item UID/quality and cancellation/recovery publication before any counted quarry stage. Keep mining separate from scale delivery.

17. **Temple acts.** Guillotine, prisoner, cardinal, severed head and newly animated headless are lore/combat scenes without a selected execution, conversion, rescue or soul-release terminal. Builders must design participant eligibility, victim ownership, reversible failure and saved outcomes before a story hook.

18. **Water and combat outcomes.** Moat2769 has current15west in actual water-swim6; flight/levitation, command eligibility and successful movement matter. Record owned arrival/survival only after resolution. Mobile1d1+1 is augmented by level-squared HP in the loader; do not infer a combat repair from raw dice.

19. **Lore and builder decisions.** Review engineer quartermaster blame versus advisor source, promise of engine immunity, armor work duration and obsolete holding fractions. Current mappings state the encoded behavior fairly. Selected data/quest repairs need separate named fix/news commits, precise before/after and prominent PR/news notes.

20. **Played qualification.** Run authorized active READY accounting journeys for all five daily and three story-only exchanges, gifts/source recovery, repeated/competing inputs, departure, passwords/keys, staged encounters and recovery. Qualify pool/node outcomes separately; teacher/mining guards remain visible. Source and compiled regressions do not replace played proof.

## Spires: switches, multi-material returns and owned access

The [complete Spires dossier](../design/zone-stories/SPIRES_OF_ELDER_EVIL.md) maps six independent returns, twelve contacts, two addressed aliases, eleven optional current-material rows, seventeen steps and twenty follow-ups. Four scales and three books retain exact atomic sets; two visage makers retain independent receipts and fresh consumed inputs. Six achievements/six potential dailies remain classified. Generic PULL→near unblocking→SEARCH→arrival and breakable keys are qualified. Hide issuance, floor-book pickup, corpse locality and absent external/disconnected elder routes are fair builder decisions; static source-comprehensive mapping does not prove playability. Add causal availability and owned source/transfer/access/reward outcomes under active READY accounting. No native repair ships; selected repairs need separate named fix/news commits and prominent PR/news treatment.

## Owned follow-ups and fair builder decisions

| Area | Required capability or decision |
| --- | --- |
| Independent native receipts | Preserve six exact giver/binding receipts and six achievement/six potential daily units. Identical mask recipes remain separate; narrative families must not collapse receipt units or consume one visage twice. |
| Multiple current materials | Show four distinct scale and three distinct book counts with missing/current state. Freeze all exact loose roots and consume each set atomically. Do not invent partial deposits, timers, a coin charge or a personal kill requirement. |
| Source versus supplied custody | Publish item UID, source actor/container, spawn generation and transfer cause. Distinguish corpse recovery, gifts, theft, trade, carried-container recovery and reacquisition while retaining supplied exact-item acceptance. |
| Reward delivery and recovery | Separate accepted offering, six retained receipts and actual mask/key/horns/weapon grants. Freeze all reward identities and recover committed entitlements without duplicate issuance; reward possession does not credit another giver. |
| Addressed conversations | Record Issis loss and Horanth outrage only after admitted addressed responses. Keep aliases, raw SAY, encounter, knowledge and action outcomes separate; neither topic is a native return prerequisite. |
| Switch dispatch | Automatic ITEM_SWITCH bindings use CMD_PULL340, visible exact selected object and configured room/direction. Record actual near BLOCKED transition after success; periodic setup or a social echo is not success. |
| Secret passage discovery | After a switch clears blockage, eligible room SEARCH can clear the near SECRET bit. Record that discovery and owned arrival separately; these switches reset open walls, so do not require an invented OPEN step. |
| Reverse passage and reset | Naga switch clears135523E only;135639W remains blocked. Shadow reverse135645E starts secret and unblocked. Qualify alternate return paths, near versus reciprocal state, absent stock, repeats and reset/recovery before promising travel. |
| Usable keys and breakage | Current loose or HOLD key135458/135459/135460, actual admitted unlock and arrival differ. Emerald/elder value1=100 breaks after unlock, skull value1=0 does not. A granted non-takeable skull key remains usable in current loose custody; nesting changes access. |
| World arrival | No selected registered boundary or incoming object portal connects the zone. Builders must nominate and qualify the intended normal entry, eligibility and owned arrival before journals promise discovery or reachable stock. Do not fabricate a zone connection. |
| Disconnected elder section | Directed foyer union256 ordinary/264 keys/272 reachable stocked switches leaves61 rooms outside; ignoring every exit state still reaches272. Weak components275+43+15 singletons expose a separate43-room elder section with Dendar and Kezef. Review intended access or staging before choosing a separately named layout repair. |
| Hide source builder review | Hide135440 is required by Issis but no active registered reset, native reward or selected compiled producer issues it. Hound135412 M370@135732 has no hide G/E. Generic corpse preserves existing possessions and CARVE creates prototype8, not this hide. Decide an intended source, lineage and balance, then test it as a separate fix/news commit. |
| Book pickup builder review | Books135437/438/439 are reset on floors with wear0, not TAKE/HOLD. Normal nonlocal GET declines unless PC level>=60. Decide whether books should be portable or acquired by a deliberate interaction; do not mass-enable TAKE based on quest names. |
| Corpse material admission | Scales and visage also have wear0, but carried/worn local container GET can bypass nonlocal pickup flags. Qualify floor versus carried corpse, weight, sight, owner, transfer/grant and nested recovery before labeling those sources impossible or selecting flags repairs. |
| Reset stock accounting | Native mode2 and100percent rows declare stock, while current active accounting refuses O/P/G/E before read_object. Add durable generation and atomic source issuance with UID/custody lineage before counted recovery; daily rollover does not replenish stock. |
| Dynamic availability | Explain source unconfigured, disconnected source route, pickup refused, absent giver, prepared set, spent input, pending reward and unavailable accounting separately. Current schema cannot dynamically derive these causal states; add typed availability facts without turning prose into gates. |
| Revenge and faction outcomes | Issis's Slaazh revenge and Horanth's kill-all request are not native terminal predicates. Builders must design exact targets, participant scope, repeats and persisted combat outcomes before additional counted stages; keep existing hand-in rewards unchanged. |
| Rescue, summoning and nightmare scenes | Flant/emissary captivity, sacrifices, summoning circle, Elf-Eater, Dendar swallowing sun and planar text have no selected resolved rescue/conversion/summoning/sun terminal. Require deliberate actor/world ownership and failure/recovery design before adding hooks. |
| Placeholder and unused intent | Kezef, Elf-Eater, Lady, Tix and ordinary doppleganger retain PH/level1 placeholder records; level-squared loader handling does not establish intended balance. Unused souvenirs, weapon variants, serpentine key135431 and copied switch extra descriptions require fair builder review, not automatic combat/item changes. |
| Played qualification and repair reporting | After source mapping, play active READY discovery, source/gift/local-corpse recovery, all six returns, four-/three-root consumption, breakable keys, PULL→SEARCH→arrival, reverse travel, reset and reward recovery. Selected native repairs require separate named fix/news commits, before/after evidence and prominent PR/news wording. |

## Shaughin: hazardous routes and owned potion outcomes

The [complete Shaughin dossier](../design/zone-stories/SHAUGHIN_SETTLEMENT.md) maps four independent paired-trophy returns, six contacts, two response bodies/four aliases, four optional quantity-two rows, eight steps and twenty follow-ups. Four achievements/four potential dailies remain classified. Full source parents, imported lower trophies,149-room conditional access, ENTER portals, actual waterfall/current behavior, camp secret-state mismatch and active inn are qualified. Source mapping does not prove stock or played completion. Extend causal availability, source/gift and exact root ownership, admitted access and actual potion publication under active READY accounting. No native repair ships; selected native repairs need separate named fix/news commits and prominent PR/news treatment.

## Owned follow-ups and fair builder decisions

| Area | Required capability or decision |
| --- | --- |
| Four independent receipts | Preserve all four exact QA contracts, four achievements and four potential dailies. Narrative upper/lower progression does not impose an order, personal kill, conversation gate or combined campaign terminal. |
| Quantity-two custody | One optional current row per card requires two distinct exact loose roots. Keep one-item, held/worn/nested, wrong prototype, spent and reacquired states truthful. Freeze and atomically retire both roots; never count one UID twice. |
| Source versus supplied trophies | Publish committed source UID, parent corpse/NPC, reset generation, recipient and transfer cause. Distinguish own recovery, gifts, theft, trade and reacquisition while retaining native acceptance of supplied exact pairs. |
| Partial deposits and actor ownership | Legacy completion reads accumulated giver inventory; active durable offering gathers the current actor's loose roots together. Explain that one-at-a-time or cross-player deposits are not supported as active paired preparation. Add escrow only with an explicit ownership/refund/recovery design. |
| Potion delivery | Keep accepted input retirement, retained completion and actual potion publication/recovery distinct. Freeze exact reward prototype/UID and recipient. Pending grants, repeated callbacks and cold recovery must not duplicate potions or credit another return. |
| Potion use and teaching lore | Native rewards are four potion items, not skill or XP training. If consumption/effect stages are designed later, hook committed QUAFF/consumption and actual effect outcome separately; flavor about spirit ascension is not a teaching receipt. |
| Addressed conversations | Two MA bodies expose hi/quest and reward/trophy. Record knowledge only after admitted addressed responses, retaining aliases and speaker/recipient scope. QA room broadcast does not create group eligibility or shared reward credit. |
| Upper-source variations | Only selected adult6507/king6509 G rows issue heart6509; some matching mobiles lack that row, and young6510/6511 or death-viper6508 supply none. Track actual stock lineage and generation rather than infer every snake drops a heart. |
| Lower foreign sources | Close bounded zone66 providers: bear6600→6613 at6603/6611/6684/6692; panther6602→6614 and ancient6603→6615 at6609/6621/6674/6677. Credit the zone65 return separately from future lower-ground encounters and stories. |
| Reset availability | Mode2/chance100 rows declare stock, while active accounting currently refuses O/P/G/E before issuance. Add durable reset generation and atomic source publication with UIDs before counting recovery. Daily reset cannot promise replenishment. |
| Visible object travel | Trapdoors6500/6600 and maelstrom6501/opening6601 use ITEM_TELEPORT25/CMD_ENTER7 with fixed destination and unlimited negative charges. Record visible selected object, admitted relocation and actual arrival; copied words or current portal stock are not travel receipts. |
| Camp passage intent | 6546N↔6686S has raw4 but no corresponding D secret reset; setup_dir masks to3, leaving an open passage. Review whether the hidden prose or current openness is intended. If changed, use a separately named layout fix/news commit and qualify both sides and valid alternate routes. |
| Outer hidden doorway | 6696W↔6697E uses raw5/D5: secret, closed, unlocked. Eligible successful SEARCH, OPEN and arrival differ from seeing fire prose. Keep shared door state, reset and reverse travel distinct; do not invent a key requirement. |
| Mushroom route protection | Intact path uses NO_MOB, with ordinary glowing6602 scenery; shattered-path rooms permit normal mobile movement. No selected custom ward-restoration handler or player invulnerability exists. Qualify actual movement and threats before calling a path safe. |
| Waterfall and currents | 6547/6548 are underwater sector9/F100/C75DOWN;6535 has C75DOWN;6695 sector7/C100NORTH. Fall/arrival/command current checks can redirect or intercept actions. Model actual breath, swim/flight/levitation, relocation and return route outcomes without making one approach mandatory. |
| Inn placement builder review | Effective room6535 binding is inn despite lake-current/maelstrom context. RENT has normal combat, status, welcome and PvP-delay guards and terminal persistence, but command current interception happens first. Review historical intent before moving/removing this service; no repair is selected from prose alone. |
| Carcasses, workers and nets | Imported workers6610/6611 and ordinary campfire6604, net6503 and hanging-carcass prose have no selected processing/fishing/recovery quest terminal. Builders may define exact carcass ownership, worker service, inputs, result and failure/recovery before adding counted episodes. |
| Incidental services and artifacts | Citizenship ring6502 is a chaos-kit reference, not admission to Thregamar. Brew6508 is local loot and foreign vendor120010 stock; it is not a requested trophy. Disabled6683 ship-shop assignment supplies no active service. Keep wider trade and unselected lower scenery bounded. |
| Causal dynamic availability | Explain absent giver/source stock, one-of-two current preparation, nested/spent roots, accounting unavailable, hazardous/redirected travel and pending potion delivery separately. Extend typed runtime availability/provenance facts; current schema provides static route hints and custody/receipt projection. |
| Played qualification and repair reporting | Play active READY discovery, upper/lower source and supplied pairs, one-item refusal, exact atomic retirement, independent receipts, trapdoor/secret/water travel, reward recovery and persistence. Selected native repairs require separate named fix/news commits with exact before/after evidence and prominent PR/news wording. |

## Trakkia: tomb progression and owned treasure access

The [complete Trakkia dossier](../design/zone-stories/TRAKKIA_MOUNTAINS.md) maps four independent signet/soul/hide/four-root returns, seven contacts, two addressed responses/four aliases, eight steps and twenty follow-ups. Four achievements/four potential dailies remain classified. Full source parents, actual tomb/gem keys, candle/PULL/PUSH mechanisms, ENTER caves,161-room conditional access, wandering-giver staging, FIREPLANE hazards, epic node and bounded Saints campaign are qualified. Root reward does not prove treasure unlock; ring narrative does not impose a prior soul return. Expand source/gift, exact quantity-four ownership, causal availability and owned access/reward outcomes under active READY accounting. No native repair ships; selected repairs need separate named fix/news commits and prominent PR/news treatment.

## Owned follow-ups and fair builder decisions

| Area | Required capability or decision |
| --- | --- |
| Independent native identity | Preserve four exact Q receipts, four achievements and four potential dailies. Do not invent conversation, personal kill, earlier soul-return or overall liberation prerequisites. |
| Four-root preparation | Require four distinct exact loose57040 roots; refuse one-to-three, duplicate UID, worn/held/nested, wrong and spent roots. Atomically retire the actor-owned roots; partial deposits need explicit escrow/refund/recovery design. |
| Source and transfer lineage | Publish committed source UID, parent NPC/corpse/nest, reset generation, recipient and cause. Own recovery differs from gift, trade, theft and reacquisition; native supplied-item acceptance remains valid. |
| Ancestral liberation intent | Kaloian Q2 checks ring57024 only; narrative release is broader than the predicate. Builders must decide whether a future liberation episode requires Aspuru death, soul return, world effect or party credit before adding any gate. |
| Saint reward publication | Q24 gives55323 aura and32019 vial. Retain exact receipt and freeze both reward identities/recipients; pending or recovered grants, aura custody and vial consumption are separate from the return. |
| Wider Saints campaign | Aura55323 is one of six ingredients in wh55103/bs74254 buckler55322 recipes; Jade76688 yields aura55324/two32019 vials for67116+76730. Preserve competing material ownership and foreign giver/zone receipts. Broader acquisition and buckler effects need their own mapping. |
| Lost-flock evidence | Hides57044 are E248 on captured wildman57039@57072. Ordinary goats and prose do not encode carcass collection, live rescue or restored flock. Define those exact outcomes only if builders design new episodes. |
| Wandering giver availability | Shepard57036 M276@57084 lacks ACT_SENTINEL; staging has four outward exits and no incoming player route. Qualify actual wandering and encounter location before calling this broken or adding a fixed destination. Do not connect staging rooms as an automatic repair. |
| Departure and daily renewal | Tribesman57058 Q52 is D1 and zone reset mode1. Keep receipt, reward publication, departure and later reset generation distinct. Daily eligibility cannot promise an available giver or reissued roots under active accounting. |
| Nest contents and root use | Four P roots are in closeable/closed/unlocked57037 nests@57152/57157/57163/57165. OPEN, actual content transfer and first recovery need owned facts. Curative lore is not a heal/cure terminal or a requirement to personally visit four nests. |
| Gem unlock and arrival | Gem57041 is type18/value1=100 for57157N↔57158S raw7/D6. SEARCH, exact key custody, admitted UNLOCK, key retirement, OPEN and actual arrival must differ. A return receipt does not prove those later outcomes. |
| Tomb key chain | Skeleton57019 P123 in casket57018@57100 opens57094N↔57096S raw3/D2. Aspuru carries57055 hellstone with100% break chance;57096N→57097 raw7/D6 needs it. Preserve valid supplied keys, shared reverse state, alternate admission and return route. |
| Candle and remote switches | Type29 candle57028 PULL340 targets57001D; lever57026 PULL targets57119S,57027 PULL targets57134W, flatstone57056 PUSH270 targets57136E. These nonsecret branches clear BLOCKED both sides; CLOSED remains until OPEN. Record admitted selected switch, affected edges and actual movement. |
| Cave and hut travel | Six paired type25 caves and hut57022 use ENTER7/unlimited negative charges. Hut goes57003→57133, with physical south return. Count successful visible-object dispatch and actual arrival rather than word matching or current stock. |
| Hazards and room flags | F40/F60 summit and F90/F80 shaft metadata are real falling chances. Magma rooms57114/57132 use FIREPLANE11, not LAVA39; firesector schedules actual heat damage. NO_TELEPORT/NO_GATE/NO_MAGIC and narrow rooms matter; map actor admission/survival without a promised safe route. |
| Hireling and witch services | Pet shop57068 uses next real room57069; active accounting refuses purchase/rental mutations. Witch57007 has native shop stock and qc_action30 scenery. Explain unavailable service separately from missing quest items; no mandatory payment or potion crafting receipt exists. |
| Epic node and memory | Imported359@57097 has active epic_stone TOUCH/periodic behavior and independent durable reward publication. Memory55436 has _noquest_/QUESTITEM and no selected special. Node completion, token custody, quest returns and zone discovery are separate. |
| Chest and figurine intent | Chest57057 remains an open type15 container despite values resembling switch parameters; it is not auto-bound to PUSH. Figurine57010 is a wand with FIRE_AURA492, not a selected pet summon. Review historic intent before any retyping, proc or loot change. |
| Causal availability and bounded foreign gaps | Extend typed missing-stock, nested/spent preparation, unreachable/hidden passage, absent wandering giver, unavailable accounting/service and pending reward reasons. Active accounting refuses O/P/G/E issuance. Incidental Deramuth57744 is missing from registered prototypes and foreign Navift lacks a declared placement; review those separately without treating them as Trakkia prerequisites. |
| Played qualification and repair reporting | Play active READY discovery, source versus supplied singleton/four-root inputs, exact atomic retirement, four independent receipts, wandering giver, switch/door/key consumption, node and reward recovery/persistence. Selected native repairs require separate named fix/news commits, before/after evidence and prominent PR/news wording. |

## Kelek: physical locations, spoken gates and distinct outcomes

The [complete Kelek dossier](../design/zone-stories/STONE_TOMB_OF_KELEK.md) maps three independent bishop/captive/smith returns, thirteen contacts, two addressed responses/two aliases, six optional singleton custody rows, nine steps and twenty follow-ups. Three achievements/three potential dailies remain classified. Complete local tomb/source closure and bounded Church sources, blasphemy speech gate, cell key, note follow-on, crypt key breakage, Deliverer combat, carried epic node and214/215-room conditional access are qualified. Document the concrete numeric-owner mismatch: two Church givers are catalog-owned by Kelek. Plan explicit receipt-preserving ownership migration at Church priority178; no native repair ships. Expand source/gift, mixed-material ownership, owned passage/reward outcomes and causal availability under active READY accounting. Selected repairs require separately named fix/news commits and prominent PR/news treatment.

## Owned follow-ups and fair builder decisions

| Area | Required capability or decision |
| --- | --- |
| Exact independent identity | Preserve three native receipts, three achievements and three potential dailies. No topic, personal kill, earlier Church badge return, forge restoration, boss defeat, rescue or escort is an encoded prerequisite. |
| Mixed-material preparation | Expose six singleton custody rows: bishop1, captive3, smith2. Distinct exact loose actor-owned roots must be retired atomically; one input type cannot replace another. Worn, held, nested, wrong, duplicate UID and spent inputs do not count. |
| Source versus supplied inputs | Commit source UID, parent NPC/corpse/room, reset generation, recipient and transfer cause. First personal recovery differs from gift, trade, theft and reacquisition; supplied exact ingredients remain valid native offerings. |
| Explicit quest ownership | Bishop87860 and paladin87869 lie above Church top87852, so numeric giver bands assign their Church Q47/Q62 to Kelek879. Add explicit authored owner metadata with validated source/giver/contract bindings; independently retain physical encounter and completion rooms. |
| Receipt-preserving owner correction | Coordinate Python catalog and compiled zone_for_giver_vnum/runtime capture. Preserve definition IDs and frozen committed receipts, explicitly version projections and migrate achievement/daily/discovery ownership. Test old receipts, pending recovery and both journals before moving these two units at the Church priority178 review. |
| Cross-zone discovery and hints | Currently Church physical discovery admits its NPC encounters; Kelek discovery exposes the catalog-owned cards and gates their dailies. Completion can record the actual Church room. Explain real locations now; future hints and daily availability must use explicit quest owner while validating physical access. |
| Bishop betrayal narrative | Q47 gives250000 copper+100000XP, not membership, office, durable expulsion or faction change. Q14 badge87846→XP at cardinal87821 is an optional earlier Church receipt, not a native requirement. Builders must define any future political episode and world effects. |
| Captive proof and rescue | Q62 consumes87870/87871/87872 and grants87873 note; no release, movement, following or escort effect exists. The note claims rescue in its prose. Preserve exact proof receipt and define actual captive release/escort/party credit only through deliberate builder integration. |
| Note delivery continuity | General87847 M229@87847 Church encampment accepts note87873 for mask87874 through independent Q24 owned by Church878. Track committed reward UID and later note return separately. Do not merge it into Kelek credit or count mask custody as a rescued prisoner. |
| Spoken Church gate | 87842UP↔87848DOWN uses key-2 and last door keyword blasphemy. Admitted SAY invokes check_magic_doors, clears reciprocal LOCKED/SECRET and leaves CLOSED. Freeze actor/source room/affected edges and record OPEN plus actual arrival separately; learned keyword and shared unlocked state are not owned passage history. |
| Cell key and alternate admission | Jailor87840 M235/G236@87849 holds nonbreaking key87868 for north/south/east cells. Qualify actual key use, eligible alternate access, door opening and actor arrival rather than requiring a personal jailor kill or a bishop completion. |
| Smithy progression | Smith87963 M147@88163 returns87963 cloak for87961 chunk O49@87961 plus87962 hammer E107 on87953@88099. His hot-forge story does not change forge state. Define restoration/world effects explicitly before adding any counted crafting episode. |
| Hidden tomb passages | 88009S↔88163N,88109N↔88150S,88148W↔88158E,88156S↔88157N are reset D5 secret/closed/unlocked. Eligible SEARCH and OPEN lead to actual movement. Current reachable graph does not prove stock, visibility or personal passage. |
| Crystalline crypt key | Lich87958 M140/G142@88161 holds87959 key/value1=100.88161DOWN raw7/D6 uses it;88162UP raw3/D2 has key0. Successful admitted unlock retires the key and unlocks reverse state. Qualify reclosure/reset/return recovery before promising an escape path. |
| Deliverer combat outcome | 87950 Deliverer E136 on cleric87956@88157 has an active wielded/alive CMD_MELEE_HIT callback, undead/WRAITHFORM target and1-in20 holy damage. Combat, damage, defeat, personal source and any future purification episode need separate authoritative outcomes. |
| Carried epic node | Kelek87955 M144/G146@88162 carries imported359. Periodic obj_zone_id resolves carrier room; TOUCH accepts actual carried/floor node and checks origin, peaceful room, level and UID before durable reward. Boss kill, node recovery and committed payout are different; reset mode0 requests reset only through committed node publication. |
| Conditional access and innate arrival | Underdark857219↔87950 and Krethik20115↔88165 are full boundary leaves. Conditional graph214 before crypt key/215 after;87951 is isolated. Prime-shift88122 is a conditional random astral arrival, not guaranteed access or an escape grant. No fall metadata or hazardous sector is encoded locally. |
| Fair historical data review | Raw4 vertical exits87953UP/88126UP/88164DOWN have no D reset and start open; do not invent SEARCH. Isolated empty87951 and advisor87964 guard-copy prose require builder intent review before topology/lore changes. Document concrete defects separately from deliberate staging or historic descriptions. |
| Guarded special assignments and issuance | Stale87891 world_quest assignment has no registered prototype and real_mobile0 falls back to index0; actual Morg bartender is88611. Audit missing special bindings globally and decide guarded assignment/historical renumbering in a separate named fix, never retarget on guesswork. Active accounting refuses O/P/G/E issuance; implement durable reset generation before promising replenishment. |
| Played qualification and repair reporting | Play active READY real-room discovery/encounters, supplied versus personal material recovery, all three exact atomic returns, spoken/cell/crypt gates, note delivery and node/reward recovery/persistence. Selected native repairs require separately named fix/news commits and prominent PR/news before/after evidence. |

## Krethik: exact requests, links and mechanism stages

The [complete Krethik dossier](../design/zone-stories/KRETHIK_KEEP.md) maps three independent sandwich/totem/note cards, fifteen contacts, two local responses/five aliases plus one foreign topic, three optional singleton rows, six steps and twenty follow-ups. Three achievements/three potential dailies remain classified. Complete148-room source closure covers key/PICK/chest/statue/remote-brick/paired-portal/fall routes, mindbreaker, spell fountain, floor node and two independent owner550 ambassador deliveries. Preserve receipt ownership and actual encounter/discovery locations. Add versioned linked_requests presentation for independent cross-zone stories: canonical IDs, owner and visibility, without duplicate units or receipt migration. Expand owned source/gift/access/availability/departure/political/reward outcomes under active READY accounting. Ignored fountain/scroll target arguments need separate deliberate guard reviews; historic staging/lore/captive intent remains a builder decision. No native repair ships; selected repairs require separately named fix/news commits and prominent PR/news treatment.

## Owned follow-ups and fair builder decisions

| Area | Required capability or decision |
| --- | --- |
| First source and supplied copies | Record first acquisition with item UID, source actor/container/reset-generation, transfer chain and authoritative publication. All three current preparation rows are optional current loose custody; supplied copies still satisfy native recipes. Do not infer personal recovery or a kill from a matching prototype. |
| Food and Freth's shop | Separate native sandwich offering from buying/selling armor and eating food. Freth20003 has both shop and qst_func channels; interpreter dispatch still needs an awake actual giver. Track service availability and exact accepted input/reward, not a purchase or generic stomach dialogue. |
| Departure versus rescue | Troll Q22 consumes20016 and awards20066 with D1. Persist accepted receipt and native removal together; qualify cold recovery and replay. Chains breaking, escort, surviving a route, actual freedom and rescue of other captives need owned outcomes if builders add them. |
| Note and political consequences | Advisor Q34 consumes20034, awards250000 copper+100000XP and departs. Durable input, XP, currency, native departure, informing Tyrik and faction/council changes are distinct. Define an actual political episode before counting narrated intent as a completed plot. |
| Cross-zone linked requests | Root story contracts are restricted to owner200. Add explicit linked_requests presentation with stable canonical IDs, owner550, independent projection and discovery visibility, while retaining native definition IDs/frozen receipts and avoiding duplicate achievements/dailies. Foreign completion steps exist but must not couple two independent ambassador requests to a local return merely to fit the schema. |
| Foreign encounters and credit | Memory55176 and head20076 independently go to ambassador55205 in Winterhaven. Source-zone discovery must not fabricate550 discovery, encounter or daily eligibility. Preserve real completion room, actual visible awake ambassador and owner550; test source-first, recipient-first and cold/replay histories. |
| Dungeon key and alternate PICK | Guard20042 M381/G383@20051 carries rusty20043; four raw2/D2 cell doors use it. Eligible skilled held-pick attempts are an alternate route. Count successful actor-owned unlock/pick, OPEN and passage separately from key custody, source kill or troll receipt. |
| Upper floor gate | Priest20019 M558/G560@20103 carries black20065 for20030S↔20032N raw3/D2 pickproof door. Capture successful use and reciprocal state rather than treating a priest encounter or supplied key as admission. No local request imposes a personal priest kill. |
| Emerald chest and stone key | Tyrik20004 M662/G664@20139 carries nonbreaking20073. Chest20072 O258@20144 is closed/locked/pickproof flags29 with nested stone20074 P259. Separate emerald custody, admitted chest unlock, OPEN and exact key extraction, including supplied keys and empty/currently unavailable stock. |
| Statue and trapdoor | Statue20005 O233@20037 PUSH270 targets20037DOWN raw11/D10. Successful item_switch clears BLOCKED on both matching nonsecret sides but leaves CLOSED/LOCKED; stone20074 unlock and OPEN are still needed. Capture actor, switch UID, origin, target, reciprocal state and actual passage. |
| Remote brick and tomb wall | Brick20075 O260@20145 PUSH270 targets20146E raw9/D9, with20147W raw1/D1 reverse. It clears BLOCKED in a different room, leaving CLOSED. Track source versus affected room, successful state transition and subsequent OPEN/arrival; do not grant an automatic secret-door/search achievement. |
| Paired portal admission | Triangles20048/20049 connect20132↔20141 and cracks20058/20059 connect20047↔20142; all type25 ENTER7/unlimited. Actual interpreter item-teleport dispatch, selected object, arena parity and char_to_room decide arrival. Distinguish entering, being moved by another actor, return availability and current object stock. |
| Falling and route survival | Actual F5 metadata20005/20019 and F10@20046 are sampled by the interpreter and falling policy. Record scheduling, survival/removal/relocation and completed arrival if needed; a static reachability count cannot prove safe traversal or an earned exploration episode. |
| Mindbreaker combat | Object20000 E388 on torturer20001 M387@20055 has30% reset chance. Active callback requires encoded hit damage, alive actual wielder and1-in30 feeblemind60 with temporary+15 save. Separate successful proc, damage, defeat, personal source and native receipt; do not award a quest from equipment possession. |
| Spell fountain targeting | Imported72 O261@20147 binds spell_pool, not object71 magical_fountain. It rotates nine level60 effects every40 minutes and responds to DRINK while ignoring the argument. Plan exact selected-object/actor/eligibility attribution and frozen effect persistence; select any guard repair as a separate named fix with before/after tests. |
| Epic node outcome | Imported359 O262@20147 is floor stock. Actual UID, origin, peaceful-room/level admission and committed TOUCH reward remain separate from Ferrik death, memory custody or ambassador return. Persist node issuance and recovery, and do not promise a mode1 refill from daily rollover. |
| Random reward transformation | Ambassador memory returns55362+55033+1000000 copper. Bound attribute_scroll transforms55362 into random55352..55360 on a generic scroll-word RECITE outside combat; it lacks exact target selection. Plan validated object identity, frozen RNG, atomic retirement/reward publication and replay/cold recovery before counting reward-use stages or selecting a separate guard fix. |
| Causal stock and reset issuance | Mode1/591 reset declarations are not live availability. Active accounting refuses O/P/G/E before placement without durable reset-generation identity. Implement governed source issuance, shop stock, container contents, depletion and availability projections before promising source recovery or repeatable daily supply. |
| Fair historical builder review | 20143 explicitly distributes spirits and has outgoing exits; do not reconnect it automatically. Winterhaven planning maps versus other-war mobile lore, captured dragon/slaves, altar/corpse/sacrificial descriptions and old gnomish-key staging labels need intent review. No encoded rescue/sacrifice/political request was found; future mechanics require explicit designs. |
| Played qualification and repair reporting | Play active READY discovery/encounters, source/gift singleton offerings, both D1 departures, two independent owner550 returns, key/chest/switch/portal/fall paths, node and transformed-reward persistence. Any selected native repair needs its own named fix/news commit and prominent PR/news before/after evidence; this journal checkpoint changes no native behavior. |

## Thetis: exact exchange, foreign treasure and admitted access

The [complete Thetis dossier](../design/zone-stories/THETIS_REALM.md) maps three independent gem/earring/returned-map cards, sixteen contacts, two responses/four aliases, three optional singleton rows, six steps and twenty follow-ups. Three achievements/two potential dailies remain classified; Burgadan returns the map prototype and remains daily-excluded. Full100-room closure includes actual courtyard/cell/throne/PICK/chest routes, blue gem consumption and breathing, D1 departures and the bounded owner226 Hordine/owner772 Jade silt/treasure route. Supplied materials can fit without personal source or referral history. Expand versioned linked_requests and owned source/gift/consumption/door/reading/hazard/loot outcomes while preserving canonical IDs, frozen receipts and active READY accounting. Record missing38095N→104, mask waterbreathing prose/effect mismatch, chest/lore/boundary intent and unscripted captives/royal restoration for fair builder decisions. Establish intended behavior before selecting a separately named fix/news commit; no native repair ships in this checkpoint.

## Owned follow-ups and fair builder decisions

| Area | Required capability or decision |
| --- | --- |
| Active source issuance and renewal | Mode1 and O/P/G/E declarations do not guarantee current sources. Active accounting refuses item resets without durable generation identity. Governed issued stock, container contents and depletion must precede promised daily supply. |
| First source versus supplied material | Admit actor/UID/prototype/source NPC/container/room/prior ownership once. Gifts and later recustody may satisfy exact offerings without proving first personal recovery or kill. Preserve current singleton custody rows until these durable facts exist. |
| Independent exact singleton returns | Three loose offerings have distinct receipts/rewards; wrong, held, equipped, nested, spent and reacquired items must not merge requests. Supplied exact copies fit without invented source history. |
| Gem consumption and breathing |38018 itself is a potion. Track admitted selected consumption/effect, combat spill/NO_MAGIC/cooldown and present breathing; never retain a spent gem as ready or award the crab return for using its spell. |
| Earring chest route | Red key/octopus, selected chest UNLOCK/OPEN, exact nested earring extraction, courtyard passage and mermaid hand-in are separate. Do not require a personal octopus kill from room prose. |
| Functional iron key and alternate PICK | Preserve exact type13 prototype compatibility. Actor-owned door change, successful eligible pick and passage differ from guard encounter/loose key possession. No type or gate repair from a name. |
| Golden key and multi-lock stock | Six cells plus gem chest use a100% break key. Qualify current replacements/shared pre-open state, failed destruction, resets, contained multigem stock and quantities without granting a rescue from access. |
| Throne and directed return | Locate raw7/D6 secret/pickproof throne entrance, use skeleton key, OPEN and survive passage. Preserve one-way38083 return and low-bit raw interpretation; current lore cannot add a reverse or secret objective. |
| Returned-map proof and daily policy | Burgadan's consumed UID/reissued map prototype/spade and committed receipt need replay/cold compatibility. Returned current map neither proves source nor whole treasure hunt. Keep native Item exchange daily exclusion. |
| Independent foreign linked requests | Stable canonical owner226 Hordine receipt and owner772 source/access visibility need `linked_requests` presentation. Do not move IDs or grant recipient discovery/encounter/daily credit from source380 discovery. |
| Silt and door/key transaction | Spade is a key; actual above PICK is possible. Secret/lock/OPEN/reciprocal state, key-break submission/refusal and passage need explicit admitted outcomes. Any transactional repair must be isolated and preserve legitimate alternate routes. |
| Treasure chest and money recovery | Key receipt, chest OPEN, selected item extraction and coin settlement differ. Frozen source/loot/currency output must handle retries, shared emptied chest and supplied gear; no new380 treasure achievement inferred. |
| Underwater hazard and safe preparation | Current temporary WATERBREATH, expiry, drowning schedules and completed movement determine survival. Persist durable episode evidence only when designed; discovery or breathing-item name alone is insufficient. |
| Learned questions and reading | Four aliases have two responses. Add admitted selected NPC/response and map/spade reading facts with correct command path; failures, generic chat refusal and text inspection are not completion. |
| D1 departure and availability | Crab/mermaid leave after their own accepted Q; Burgadan stays. Durable removal/reset/current encounter and frozen receipts need cold/replay qualification independently of global daily rollover. |
| Missing active starfish destination |38095N→104 is absent from active manifest and removed by renum_world. Establish historical intended home or retire stale exit/prose; do not guess a target. Select a named isolated area fix and prominent news entry if resolved. |
| Mask role and effect intent | Waterbreathing description contradicts selected prototype effects. Choose historically appropriate truthful prose or balanced ability after qualification; no ability activated by the journal. |
| Fair chest, boundary and item prose | Chest lid/open/broken-lock prose conflicts with current flags29; foreign key/break semantics differ from DIG lore. Review38075 staff-feedback boundary intent and spellbook200 stored capacity versus300-page prose. Preserve topology/rarity while selecting specific repairs. |
| Designed wider realm episodes | Captive mage/slave, sirens, clams, royal vengeance, queen, purple potion giver, translator crown and turtle help need authored prerequisites/controllers/outcomes or honest prose retirement. No inferred rescue/restore/language/ride achievement. |
| Played qualification and repair reporting | Play active READY discovery/encounters, first source/gift/loose returns,D1 recovery,returned-map flow,foreign owner226 exchange,doors/chests/keys and underwater/treasure persistence. Any native repair needs its own named fix/news commit and prominent before/after PR treatment. This checkpoint plans changes without native gameplay repair. |

## Azhural: exact quantities, source routes and unfinished finale

The [complete Azhural dossier](../design/zone-stories/AZHURAL.md) maps two independent story-only exchanges: eight exact bone shards for the entrance key, then one exact essence from each flight for the chromatic talisman. Nine contacts, two MA responses/nine aliases, six optional material rows and two receipts provide eight steps. Two achievements and zero potential dailies remain classified because both givers depart and reset mode is zero. Full local closure covers 89 rooms/216 exits, 26 mobiles, 38 objects and 129 reset commands. All eight shard parents lie outside the bone gate; five ward doors, the talisman gateway, two native ENTER portals and secret ruby-key vault have separate access outcomes. Supplied exact materials fit without personal kill or earlier receipt history. Expand durable source/transfer, batch feedback, learned responses, door/key publication, admitted arrival, group encounter and reward facts under active READY accounting. Review the isolated Malsperanze room, public arrival route, placeholder Tiamat finale, blank scenes and unstocked crown/rewards fairly; any selected native repair requires its own named fix/news commit. No native repair ships here.

## Owned follow-ups and required capability

| # | Owner | Follow-up and acceptance evidence |
| --- | --- | --- |
| 1 | Accounting/source runtime | Give O/G/E issuance a durable reset-generation identity; qualify initial boot, forced repop, refusal and cold recovery before advertising live supply. |
| 2 | Story/source runtime | Record first actual shard or essence acquisition with actor, UID, prototype, source NPC/slot, generation and transfer reason; gifts and source recovery must remain distinguishable. |
| 3 | Quest/UI runtime | Expose all-eight and all-five batch acceptance clearly; test seven shards, repeated same UID, wrong color, held/nested items, refusal and preserved ownership. Avoid a deposit-progress display without persisted escrow semantics. |
| 4 | Quest/receipt runtime | Preserve independent actor-owned receipts through accepted reward/departure, replay and cold load; room echo and nearby group members do not share automatic credit. |
| 5 | Builder/access runtime | Qualify the bone door from supplied key, existing opening and ordinary source route; no personal source-kill prerequisite. |
| 6 | Builder/access runtime | Map each flight ward to its exact parent and door; record admitted unlock/OPEN/arrival, not possession or key text. |
| 7 | Builder | Review the white reverse key-zero asymmetry against intended return/reset behavior. If correction is warranted, isolate it as a named route fix with news treatment. |
| 8 | Accounting/door runtime | Coordinate keyed unlock and key destruction publication/refusal/recovery; the current 100 percent roll does not prove committed consumption. |
| 9 | Story/combat runtime | Design personal and group consort-defeat credit explicitly, including encounter identity, eligible contributors, death/source ordering and gifts. Do not substitute essence possession for battle success. |
| 10 | Builder/combat runtime | Qualify actual proc/class/equipment behavior and survival for the five flights; several hidden prototypes are unused or incomplete. Balance decisions belong in separate reviewed changes. |
| 11 | Quest/access runtime | Represent talisman reward, guardian departure and gateway door as separate outcomes; prose about lowered wings cannot mark an unlocked passage. |
| 12 | Story/transport runtime | Record admitted ENTER and exact arrival through each portal, including denial, recovery and other actors; current receipt history is not a native portal requirement. |
| 13 | Builder/world runtime | Establish intended public arrival through historical transport design and live configuration; absence of an ordinary boundary is a source lead, not proof of universal inaccessibility. |
| 14 | Builder | Decide the purpose of isolated Malsperanze room135283; add an intentional connection/controller or retire unused content only in a separate named fix. |
| 15 | Builder/encounter runtime | Decide Azhural’s Tiamat finale and replace/retire the explicit placeholder deliberately. A full multi-head encounter requires lifecycle, group credit, rewards and accounting recovery design. |
| 16 | Builder/reward runtime | Establish the ruby key and vault reward route, exact source loot versus supplied items and native treasure stocking; neither existing receipt completes the finale. |
| 17 | Builder/relic runtime | Decide intended local source of the crown and other reserved rewards. Record successful crown activation/timer/effect only after source and balance approval; do not auto-stock them. |
| 18 | Story/dialogue runtime | Add learned-response facts for the two MA blocks with content revision, actor and successful response; nine aliases remain two conversations, and bystander echo is separate. |
| 19 | Builder/editorial | Review blank rooms, clipped Ynndakaneil response and unused invasion/guardian prototypes fairly. Choose authored completion or honest retirement; publish actual repairs in clearly named fix/news commits. |
| 20 | Integration/testing | Run played active READY discovery, batch custody/denial, source/gift, departure, gates, portal arrival, consort/queen and reward persistence journeys before promotion; keep blockers and source qualification visible. |

## Castle: exact hand-ins, native access and reserved content

The [complete Castle dossier](../design/zone-stories/CASTLE.md) maps two independent Remy returns: exact family necklace for the blue sword and Povtail’s exact diamond-studded bone for the flaming orb. Thirteen contacts, two MA responses/five aliases, two optional loose material rows and two receipts provide four steps. Native retained-giver policy preserves two achievements and two potential dailies. Full selected closure covers 130 rooms/392 exits,39 mobiles,51 objects and283 commands/full414 ZON/38 QST. The keyed public route, optional containers, actual follower slots, no-ground sky, dynamically assigned Gilman epic teacher, secured imported rune stone and externally owned WH memory exchange remain separate outcomes. Supplied exact materials fit without personal kill/key/dialogue prerequisites. Expand effective table/loader discovery, source/gift, admitted doors/containers/flight, actor-owned response/receipt and group/teacher/stone/external economic facts under active READY accounting. Gilman purchases are currently blocked while accounting is active. Review suppressed F25, seven private/editorial rooms, three unstocked altars/two absent destinations, key-description mismatches and unused kitchen/claw context fairly; any selected repair requires a named fix/news commit. No native repair ships here.

## Owned follow-ups and required capability

| # | Owner | Follow-up and acceptance evidence |
| --- | --- | --- |
| 1 | Accounting/source runtime | Qualify native mode-one empty-zone resets, global caps and durable item-generation issuance; discovery/midnight must not create stock. Test refusal and cold recovery. |
| 2 | Story/source runtime | Record first exact necklace/bone recovery with actor, UID, source mobile/slot, generation and transfer reason; supplied gifts remain distinct from personal source acquisition. |
| 3 | Quest/receipt runtime | Preserve the two actor-owned Remy receipts in either order through replay, cold/raw recovery and daily rollover; source possession, the other receipt and room echo must not add credit. |
| 4 | Quest/UI runtime | Show loose-item readiness and independent return instructions; test held/worn/nested/wrong/reward/spent/reacquired items, denials and original ownership. |
| 5 | Story/dialogue runtime | Record two successful learned responses with content revision; five aliases remain two conversations and bystanders receive no automatic personal credit. |
| 6 | Story/investigation runtime | Record successful reading of the exact map/note/sign/extra rather than any LOOK attempt; separate narrative clues from new rewards. |
| 7 | Builder/access runtime | Map all seven main key sources and real gate sides; preserve forward entrance PICKABLE/reverse PICKPROOF and ordinary attempt denial without a guaranteed bypass. |
| 8 | Accounting/door runtime | Coordinate actual unlock, reciprocal state and key destruction/refusal/recovery for zero/10/100-percent break rolls; possession is not a consumed-key success. |
| 9 | Builder/container runtime | Qualify secret closet, tiny-key source, exact bookcase/contents and admitted container recovery. Preserve ordinary PICK attempts for masks13/15; bit16 PICKPROOF is absent and legacy HARDPICK does not guarantee denial or success. Review failure/pick-break behavior separately before any repair. |
| 10 | Builder/editorial | Review key2425 floor text and key2436 dog-house text against actual doors. If corrections are warranted, use named editorial fix/news commits. |
| 11 | Story/combat runtime | Design Povtail/Llamanby personal and group defeat credit explicitly; source slots, follower counts and supplied items do not prove personal kills. Preserve native encounter balance. |
| 12 | Story/movement runtime | Record admitted sky arrival, active support and actual fall/survival outcomes, including dispel/mount/climb/denial/recovery; no-ground route is not qualified by a flight item alone. |
| 13 | Builder/world runtime | Decide whether suppressed F25 hazard2463 is intentional or unfinished; verify intended DOWN target and safety/balance before changing topology. |
| 14 | Accounting/teaching runtime | Make Gilman’s existing epic purchase compatible with active accounting before a teaching objective; qualify eligibility, costs, mutation, refund/denial and committed skill acquisition. |
| 15 | Story/loader tooling | Discover teacher-table and initialization bindings alongside ordinary assignments; expose effective handler/property state rather than concluding there is no special from one file. |
| 16 | Story/zone-touch runtime | Reuse secured rune-stone participant/zone outcomes; distinguish loot/drop/touch/pending/denied/recovered/group award and avoid duplicate zone completion. |
| 17 | Story/external ownership | Present the WH memory exchange as a separate externally owned service; retain its exact memory, scroll, native coin value/token, caps and economic authority. |
| 18 | Builder/reserved content | Decide seven private/editorial rooms and three unstocked altars with missing targets/legacy commands; complete or retire deliberately, never auto-connect or activate. |
| 19 | Builder/editorial | Review clipped/spacing/lore discrepancies, unused claw and non-takeable kitchen props fairly; distinguish intentional atmosphere from verified incomplete quest mechanics and publish actual repairs separately. |
| 20 | Integration/testing | Run played active READY discovery, source/gift custody, both returns, gates/containers, sky encounter, stone group award and external economy/teaching journeys before promotion. Keep capability gaps and native repair plans explicit. |


## Reviewed quest ownership metadata

Use `areas/quest_owners.json` only for a reviewed exact giver/canonical contract whose historical numeric owner is wrong. The schema-one `owners` array requires `giver_vnum`, readable `completion_key`, `previous_zone_number`, `previous_source_area`, `zone_number`, `source_area` and current `content_revision`. Both zones/source pairs must be registered; the predecessor must match the numeric band, differ from the target, and the exact native Q must exist. Python and booted runtime consume the same file. Missing file preserves numeric fallback; invalid present data fails closed. Limits:64KiB,256 entries,1024 UTF-8 bytes per text. Coordinate old/new complete sidecars and preserve stable native IDs. Multiple contracts on one giver may have distinct owners; ambiguous corrected-giver dialogue requires a builder decision. Successive ownership changes need explicit compatibility review.

Church demonstrates two exact corrected contracts. Its bishop/cardinal and captive/general links are optional narrative context, not invented prior-return conditions. Three different captive trophies need three singleton rows. Saying a shared door word can unlock an exit without recording personal discovery, opening or passage. A report saying rescue does not implement an escort. The archive case mask29 includes PICKPROOF; raw exit7 is decoded to lowbits3 and does not itself require SEARCH. Keep current custody, owned source/transfer facts, admitted access and durable returns distinct. Native changes belong in separate named fix/news commits with before/after evidence.


## Domain builder integration

The [Domain dossier](../design/zone-stories/DOMAIN_OF_LOST_SOULS.md) owns twenty follow-ups for exact material counts, source/gift provenance, absent producers, corpse-derived proof policy, library/dialogue learning, shared access, outbound continuity and availability. Its two cards require a joint pair of distinct pact items and three copies of one skull, respectively. Preserve the exact native hand-ins and optional preparation rows.

When generating hints, resolve G/E stock to the preceding M/F parent and exact room/slot; proximity and prototype number alone are insufficient. Index default object handlers and teacher tables as well as literal assignments. A service that refuses under required active accounting needs an availability explanation and an accounting-compatible native settlement before adding an achievable step. Descriptive carving or rescue text needs an authored outcome adapter; it cannot manufacture historical facts from possession or command observation. Any selected native restoration, prose correction or service repair must use a separately named fix/news commit with before/after evidence.


## Drifting Realm builder case

See the [complete dossier](../design/zone-stories/DRIFTING_REALM.md) and [journal](../../areas/story/dream.story.json). Exact typed proofs, native item restrictions, effective handler order and mode-zero stock must remain distinct from personal source history and accepted receipts.

| Follow-up | Source finding | Required capability or fair builder decision |
| --- | --- | --- |
| ZSQ-DREAM-01 | Qin Q27 accepts four identical31311 shards; Q46 accepts one each31316–31320, with no prior-receipt check. | Preserve two independent receipts and optional earlier context. Require an explicit builder contract before enforcing progression, personal kills or distinct shard sources. |
| ZSQ-DREAM-02 | The skulls share a plain name and aliases but have five exact identities and different colors. | Project an accessible source/color label alongside exact identity; five duplicate skulls must not satisfy five distinct rows. Supplied exact proofs remain valid native inputs. |
| ZSQ-DREAM-03 | Whonh, Devin, Milrd and Nomd each declare one31311 under global cap4. | Record reset parent, source generation, corpse/container path, actor and committed recovery. Custody, transferred material, reacquisition and first personal recovery need separate evidence. |
| ZSQ-DREAM-04 | Four foreign bosses declare four skulls under cap1; only selected source groups and room leaves are reviewed. | Link exact source instances without moving receipt ownership or discovering foreign zones. Qualify their full admission, combat, recovery and renewal journeys separately. |
| ZSQ-DREAM-05 | Skull NODROP/NORENT flags coexist with the exact quest interception path. | Preserve ordinary transfer restrictions and qualify durable loose-root UID/owner admission, atomic consumption/reward, refusal, replay and cold continuation. Do not infer a legal player gift from a recipe accepting supplied material. |
| ZSQ-DREAM-06 | Raw PROCLIB flags are cleared by prototype loading; no local _proclib_ extras restore them. | Inventory effective bindings after defaults/tables/shops, not raw flags alone. The selected paths implement no original-owner retaliation endpoint; builders must define scope and fairness before adding one. |
| ZSQ-DREAM-07 | Two M responses expose eight aliases; no learned keyword prerequisite. | Persist an addressed semantic response once, with actor/recipient and successful dispatch. Aliases are vocabulary, not eight achievements or enforced knowledge. |
| ZSQ-DREAM-08 | O81/O82 lights use LOOK15; Arcium bed uses ENTER7; cliff uses JUMP264. | Capture admitted exact object/command, target resolution, arena/charge checks and actual arrival separately. Use insanity/reality keywords to disambiguate the lights; observers do not inherit actor history. |
| ZSQ-DREAM-09 | Whirlpool31302 prose describes travel, but type17 drink-container data provides no such control. | Builders choose accurate decorative wording or an explicitly designed travel endpoint. Any native repair needs a separate fix/news commit and qualification; do not automatically change type or enable travel. |
| ZSQ-DREAM-10 | Four PICKPROOF vision pairs use31304; four keys declare100-percent break. | Distinguish key custody, admitted unlock, reciprocal state, committed destruction, OPEN and passage. Shared unlock currently precedes break settlement; coordinate pending/refused key destruction before claiming consumed-key history or proposing a balance repair. |
| ZSQ-DREAM-11 | Channelled/sanctum water-noswim rooms use native movement support checks; type25 fixed travel has a different admission path. | Qualify boat/swim/flight/mount/current exceptions and actual arrivals, with survival distinct from travel. Do not invent a psychic-reform or universal equipment gate. |
| ZSQ-DREAM-12 | Reality light and cliff return to85805, whose southeast exit is locked with85703. | Explain configured return separately from onward admission. Qualify Arcium key/picking/shared state and a fair return route before promising a safe or unlocked exit. |
| ZSQ-DREAM-13 | Holy/evil groups, princess, bodyguards and lounge provide prose/equipment without an accepted moral, romance or invitation return. | Builders author explicit personal/shared episodes, branch consequences and settled outcomes before adding those objectives. Preserve ordinary native NPC/combat behavior. |
| ZSQ-DREAM-14 | Qin's shop binds primary func.mob while quester remains qst_func. | Audit effective dispatch order and shop fall-through, including death and disabled-special conditions. A shop binding does not erase the independent exact quest handler. |
| ZSQ-DREAM-15 | Tarot BUY/SELL/PERUSE/REPAIR/FORGE refuse with active accounting. | Implement guarded wallet/item/service settlement and clear availability before an achievable journal purchase. LIST/VALUE output alone is not admission or purchase evidence. |
| ZSQ-DREAM-16 | Tarot31314 is a scroll for serendipity/luck and thornskin, with no future-reading receipt. | Separate acquisition, charge/root consumption, actual effect and expiry. Add an authored fortune-reading endpoint if intended; spell names/prose are not history. |
| ZSQ-DREAM-17 | Voice G139 exports dreamfoil83283; Alatorin has a rare alternative and a four-flower recipient. | Keep local flower recovery distinct from Alatorin83244 Q3646 completion. Track typed directed ingredient/reward edges, all four exact flowers and retiring recipient; no whole-quest credit from one pickup. |
| ZSQ-DREAM-18 | Resetmode0 has normal boot stock but no ordinary boot reset timer; generic conditional/manual renewal paths exist. | Qualify actual durable stock, caps, reset ownership and deliberate renewal across local and foreign zones. Daily rollover/discovery must not manufacture replenishment; policy stays disabled by default. |
| ZSQ-DREAM-19 | Curated chaos gear includes suit/force/cards, without shard/skull proof constructors. | Reward possession or alternate issuance must never mint Qin receipts. Qualify actual issuance lineage separately from declared table membership. |
| ZSQ-DREAM-20 | Source coverage is complete locally, with bounded shared/foreign closure and unplayed outcomes. | Exercise active READY-accounting source/supplied custody, both exact hand-ins, every access control, flags/shop denials, stock and persistence. Publish actual repairs prominently and separately from hints or planned work. |


## Clavikord builder case

See the [complete dossier](../design/zone-stories/LIZARDMAN_SWAMPS_OF_CLAVIKORD.md) and [journal](../../areas/story/lizard.story.json). Preserve exact independent head returns, competing source/recipient generations, rare wandering availability and actual effective handlers.

| Follow-up | Source finding | Required capability or fair builder decision |
| --- | --- | --- |
| ZSQ-LIZARD-01 | Q14 accepts Sslith head6008 for lights6009/D; Q40 accepts Bemon head6005 for shadows6007/D. | Preserve two independent receipts, optional loose preparation and supplied exact materials. Neither requires an earlier receipt, conversation, personal kill or generic carved part. |
| ZSQ-LIZARD-02 | Bemon M191/G193 carries the proof Vornin requests; Bemon's own Q14 D path attempts to remove his inventory. | Model competing source/recipient episodes and shared NPC generation. Qualify actual retirement, preserved/supplied proof and renewal before an ordered chain; builders select any explicit branch policy. |
| ZSQ-LIZARD-03 | Sslith M26/G28@6022 and foreign merchant100015 M15/G16@105302 declare the same6008. | Separate typed origin, reset parent, actor/party, corpse/container route and owned first recovery from trade, supplied custody, spending and reacquisition. Alternate source does not prove personal king defeat. |
| ZSQ-LIZARD-04 | Ordinary CARVE creates limbo bodypart8, not6005 or6008. | Keep exact stocked heads as the native proof, or author a scoped corpse/actor/provenance constructor after a deliberate builder decision. Do not alter universal carving or count a generic part. |
| ZSQ-LIZARD-05 | Durable reward continuation precedes attempted D equipment/carrying extraction and giver removal. | Qualify atomic exact-root offering, UID/owner admission, reward/departure, rejection/replay/cold recovery and unsupported/pending NPC-stock destruction. A returned command or narrated departure is not every settled consequence. |
| ZSQ-LIZARD-06 | Vornin M194 uses chance25/cap1; Bemon/source heads have separate caps. | Expose instance-specific recipient/source availability and retirement episodes. Do not promise daily presence or renew an absent recipient merely because a day changed. |
| ZSQ-LIZARD-07 | Givers/wight start in6099 with normal wandering; Bemon/Vornin have ACT_STAY_ZONE and no raw sentinel bit. | Resolve actual visible NPC generation/location and admitted movement. The dispersal reset room is not a promised encounter location; should_teacher_move does not freeze every epic teacher. |
| ZSQ-LIZARD-08 | Bemon is table-bound Expert Parry teacher and separately quester. | Inventory effective primary/table/qst dispatch and actual service eligibility. Active accounting refuses purchases; port guarded epic/copper/skill settlement, frozen fees/caps and recovery before training credit. |
| ZSQ-LIZARD-09 | Vornin has a local shop with no declared products; foreign merchant stocks6008. | Preserve primary shop plus independent qst handling, active trade denial, STEAL refusal and carrying-only shop death behavior. Builders review empty-shop intent before any separate unbinding or stock change. |
| ZSQ-LIZARD-10 | Surface634225 and Underdark834369 are reciprocal public leaves; Heavens15/WH55402 provide other incoming declarations. | Qualify actual arrival and administrative/selector provenance. Incoming graph edges do not promise accessible shortcuts or foreign discovery; retain source ownership and disclosure policy. |
| ZSQ-LIZARD-11 | Willow6002 DOWN is D5 SECRET/CLOSED, reciprocal6003 UP is D1 CLOSED; key0. | Record successful SEARCH/reveal, shared exit generation, OPEN and passage separately. No native keyed, personal-kill or learned-word prerequisite; unsuccessful attempts or already-open shared state are different evidence. |
| ZSQ-LIZARD-12 | Two type25/ENTER7 pools link6102↔6103; physical paths reach the shrine and Underdark opening. | Capture exact object/command, target/arena/charge admission and actual arrival separately. No head receipt unlocks these controls; an observer does not inherit travel history. |
| ZSQ-LIZARD-13 | Mukrok's high-skill exclusion prose has no matching selected gate; affected_by4=8192 is regeneration. | Preserve ordinary native combat/affects. Builders decide whether the prose is atmosphere, obsolete or an intended rule before separately designing a fair actor/level/direction admission contract. |
| ZSQ-LIZARD-14 | Wight bracelet T2/12/1/23 uses GET/PUT; damage12 has no current switch case. | First eligible object attempt can consume one charge, run no defined damage effect and be rejected before pickup. Qualify trap state and actual recovery separately; builders choose retained behavior, corrected data or an explicit effect before a named native fix. |
| ZSQ-LIZARD-15 | Trap mask1=movement,2=GET/PUT,4=room; supported damage codes0..8. | Extend builder validation using effective selector constants and flag combinations. Domain key T6/2 is GET/PUT plus room-wide fire, not movement. Correct only its current dossier phrase; native bytes and exact archived PR history remain intact. |
| ZSQ-LIZARD-16 | The warning sign is type5 weapon with readable extra text; pools/totem have separate generic types. | Preserve typed identity and actual object behavior. Review sign intent before a decorative/equipment change; lore and equipment effects require owned semantic outcomes before objectives. |
| ZSQ-LIZARD-17 | Redemption/revenge prose has no temple reinstatement, brother/world-state or later Bemon-help endpoint. | Builders define personal/shared moral branches, recipients, consequences and persistence explicitly. The D departure conflicts with a literal guaranteed future-help promise; editorial changes need separate before/after reporting. |
| ZSQ-LIZARD-18 | Witchdoctor shrine, tako/bodyguard, shadows/undead and tribal raid lore have no extra native Q. | Author optional episodes only after explicit accepted tasks, combat/source/reveal/effect or settled corruption/rescue outcomes exist. Mere keyword, visit, loot or ordinary NPC defeat must not fabricate a quest. |
| ZSQ-LIZARD-19 | Mode1/lifespan20–30 waits until due and descriptor-based emptiness; stock caps/chances are independent. | Qualify active READY durable renewal, NPC/source generation and availability across local/merchant stock. Retain two potential candidates with default policy disabled; discovery/UTC rollover does not reset the world. |
| ZSQ-LIZARD-20 | Comprehensive local reading and bounded shared/foreign closure are distinct from played proof. | Exercise both exact returns, supplied/source custody, competing D episodes, rare/wandering recipients, secret/pool/public routes, trap/service denials, reset and cold persistence. Report any actual native repair in a separate named fix/news commit. |


## Tower of High Sorcery builder case

See the [complete dossier](../design/zone-stories/TOWER_OF_HIGH_SORCERY.md) and [journal](../../areas/story/tower.story.json). Keep exact independent bundles, source/container lineage, party reward settlement, wandering availability and actual controller outcomes separate.

| Follow-up | Source finding | Required capability or fair builder decision |
| --- | --- | --- |
| ZSQ-TOWER-01 | Smedgewack9321 Q15 accepts5011/5016/5035; Zbarnos9367 Q47 accepts9365/9366. | Preserve independent exact atomic bundles, optional loose preparation and supplied materials. No earlier dialogue, personal source/kill, other receipt or generic carved part prerequisite. |
| ZSQ-TOWER-02 | Labyrinth declares calcite G on rock worm5016@5129, tattoo O@5003 and flask P inside explorer5019's equipped sack5067@5189. | Record typed parent/source generation, exact root UID, container lineage and owned first acquisition separately from transfers, wearing, spending and reacquisition. Owning the sack does not prove retrieving its flask. |
| ZSQ-TOWER-03 | Gargantuan kraken9360@9389 wears scale9366 at slot21; giant9361@9391 carries eye9365. Mysterious9354 holds neither requested part. | Qualify equipment-to-carrying/death/corpse or no-corpse release and admitted GET independently of personal kill and accepted return. Preserve supplied exact parts and source/party identity. |
| ZSQ-TOWER-04 | Ordinary CARVE constructs prototype8, while exact9365/9366 have type8 and QUESTITEM item2 flags. | Distinguish item type from prototype ID. Do not substitute a generic carved part or change universal carving. A deliberate source constructor needs actor/corpse lineage and builder approval of the intended materials. |
| ZSQ-TOWER-05 | Q47 text says all three krakens died, the curse faded and Zbarnos returned home; native terms check two items then D. | Builders choose whether text describes the exchange, needs wording repair or requires a separate personal/shared three-kill and curse endpoint. No automatic world/curse state or personal third-kill achievement. |
| ZSQ-TOWER-06 | Smedgewack rewards E18100/I9375/C53000; Zbarnos rewards I9369/I9373/E33333. | Display nominal terms, accepted receipt and per-child XP/coin/item settlement separately. Qualify partial failure, child identity, replay/cold recovery and actual delivery before saying the complete reward bundle settled. |
| ZSQ-TOWER-07 | Reward capture freezes eligible in-room party PIDs; offering actor XP cap is next-level XP/10, eligible others use next-level XP. | Record frozen party credit and effective XP terms without granting every recipient original material recovery or three-kill history. Qualify leaving, disconnect, leveling, duplicate membership, caps and retained component persistence. |
| ZSQ-TOWER-08 | Both Q blocks use D; durable reward continuation is dispatched before attempted carried/equipped stock extraction and giver removal. | Model receipt, reward children and actual NPC retirement as separate episodes. Qualify rejected/pending destruction, callbacks and repeated/cold recovery. A returned command or narrated departure is not all settled consequences. |
| ZSQ-TOWER-09 | Smedgewack is sentinel at9340; Zbarnos is nonsentinel/STAY_ZONE at dispersal9395. | Resolve actual visible NPC generation/location and native wandering admission. Reset location is not a guaranteed public counter or encounter; no new journal credit when accounting is inactive/not READY. |
| ZSQ-TOWER-10 | The dispersal graph can reach9399, which has no exits and no ROOM_NO_MOB bit. | Qualify actual actor/NPC movement and capped-recipient availability. Builders review whether the sink is intentional dispersal, obsolete routing or an availability defect before a separate narrowly scoped fix; do not promise daily Zbarnos presence. |
| ZSQ-TOWER-11 | Four key routes use9300/9301/9336/9337; archmage9301 UP is PICKPROOF while9300 DOWN is not. | Separate key custody, exact door/direction, unlock, open and arrival with shared exit generation. Preserve legal supplied keys, already-open routes and native PICK/KNOCK alternatives rather than forcing a personal source kill. |
| ZSQ-TOWER-12 | Main gate key9300 break chance15 differs from the other three keys'0; native UNLOCK clears the lock before possible durable break handling. | Capture actual unlock and key-destruction settlement separately, including ownership/admission failure and reverse-door publication. Do not claim every unlock consumes a key or failure to break undoes access. |
| ZSQ-TOWER-13 | Skull9361@9353 and spine9360@9372 are default-bound type29 PULL340 controls for reciprocal BLOCKED8 exits. | Record actual matching object/room/command admission and changed shared exit state, then passage independently. PUSH270, wrong object, Nothing happens and another player's already-open state are different evidence. |
| ZSQ-TOWER-14 | Native switch clears the reverse exit for these nonsecret controls; the reciprocal link exists. | Qualify exact reciprocal graph and shared state generations; observers must not inherit pull or traversal history. Builder validator should check reverse identity and secret behavior before extending controllers elsewhere. |
| ZSQ-TOWER-15 | Karkran9363@9376 carries type22 skiff9367; no local shop/fare endpoint exists. | Keep ordinary boat custody separate from transport service, fare, passenger capacity and actual lake arrival. Generic water admission has other legal alternatives; boat lore is not a proved one-passenger gate or underwater breathing rule. |
| ZSQ-TOWER-16 | Surface586837 NORTH↔9334 SOUTH is current public access; two old surf declarations are unregistered;872 type25 declarations across442 raw object files have no declared Tower targets. | Advertise only qualified current routes. Preserve foreign ownership/disclosure policy, actual admitted arrival and administrative provenance; graph edges or material possession do not discover Labyrinth/WH. Qualify dynamically assigned destinations separately. |
| ZSQ-TOWER-17 | Eight enabled bulette bindings supply periodic random speech; no Tower raw teacher flags, local shops or literal room/object special assignments. | Inventory default/table/qst roles as well as explicit assignments. Atmospheric lines do not define keyword achievements. Sinister rogue24576 lacks CLASS_ROGUE4096, so its keyword alone does not bind default thief. |
| ZSQ-TOWER-18 | Native loader/conversion derives effective stats; raw kraken1d1+1 and race/home/class fields are not effective encounter balance. | Qualify level/race/class/zone/difficulty before a balance finding. Do not repair raw dice or mistake home12/raceX for custom quest selectors. Preserve ordinary native combat; no automatic stat repair. |
| ZSQ-TOWER-19 | Drestah carries imported memory55441 for WH55273 Q3663; dragon, priests, Armonder/followers, guardians and treasure have no other local accepted Q. | Keep foreign contract ownership and optional exploration distinct from Tower returns. Builders need explicit accepted task, source/reveal/effect or settled outcome adapters before added curse/rescue/mage stories. Review prison inaccessible-trapdoor prose against the actual keyed UP exit before a named wording repair. |
| ZSQ-TOWER-20 | Mode1/lifespan15–25, caps, foreign proof stock, wandering, D and source consumption jointly govern renewal. | Qualify active READY owned renewal, availability and played/cold persistence across zone boundaries. Two potential dailies are a policy classification, not guaranteed replenishment. Keep policy disabled by default and report actual native repairs in named separate fix/news commits. |


## Vecna's Tomb builder case

See the [complete dossier](../design/zone-stories/VECNAS_TOMB.md) and [journal](../../areas/story/vecna.story.json). Declare exact independent returns, explanatory versus required routes, actual participant/owner/episode credit and committed item/effect/travel outcomes. Review effective bindings and dormant controls before separately named fair repairs.

| Follow-up | Source finding | Required capability or fair builder decision |
| --- | --- | --- |
| ZSQ-VECNA-01 | Kairvo130023 Q11 accepts brain130034; Kaervek130024 Q30 accepts trinket130033. Both give one different reward item and D. | Keep independent exact returns and optional loose preparation. Supplied materials fit without earlier dialogue, personal recovery, survival, combat or the other receipt. |
| ZSQ-VECNA-02 | Brain P121 is inside Gerd130023 O119@130069, alongside separately stocked rotten meat. Gerd is type15, not ITEM_CORPSE24. | Preserve exact container/root/parent lineage and admitted retrieval. A corpse name, an empty container, generic carving or the half-dissolved creature130029 does not produce this exact brain. |
| ZSQ-VECNA-03 | Trinket G276 is carried by apprentice130027 M275@130076. | Distinguish equipped/carried death release, actual corpse/no-corpse disposition, admitted first recovery, supplied custody and spending. Personal kill and accepted return need separate actor/episode evidence. |
| ZSQ-VECNA-04 | Current loose material differs from held, worn, nested, transferred, spent or reacquired custody. | Use exact UID/source generation/container lineage and owned committed outcomes. Show current preparation separately from durable history; do not infer original recovery from possession. |
| ZSQ-VECNA-05 | Kairvo's warning describes preservation and survival, while Q11 checks only the exact brain. | Keep descriptive route and dangerous-world warning. Builders decide whether an optional personal survival story needs new authoritative endpoints; do not silently add a native prerequisite or encourage a fabricated completion. |
| ZSQ-VECNA-06 | Kaervek's prose says the polished trinket is handed back, but input130033 and reward130036 are distinct prototypes. | Present the accepted return and distinct reward truthfully. Same-root transformation/continuity would require an explicit owned mutation transaction and deliberate recipe decision. |
| ZSQ-VECNA-07 | Each return has an I reward/D, with frozen offering context and separate durable reward child recovery. | Separate accepted receipt, item grant/save acknowledgement and actual recipient retirement. Qualify rejected/pending grants, disconnect, replay and cold recovery; the receipt does not settle every consequence. |
| ZSQ-VECNA-08 | Shared native offering captures eligible in-room party identities; personal source history is different. | Freeze eligible credit without attributing each recipient original recovery, preservation survival or black-mass defeat. Qualify actual reward recipient and party membership changes rather than promising duplicate material rewards. |
| ZSQ-VECNA-09 | Kairvo M254@130067 and Kaervek M256@130068 are sentinel, cap1/chance100. Both depart. | Resolve actual visible NPC generation/location, accepted retirement and owned replenishment. Encounter visibility and historical receipts do not guarantee a fresh giver today. |
| ZSQ-VECNA-10 | Vecna GOTHIT has fighting/random0..50 gates; missing/trusted opponent is excluded. At half HP or less the opponent is moved to130069, otherwise takes cold damage. | Capture initiator, actual affected opponent, HP/episode and admitted destination separately. This function has no player-only gate, brain constructor or survival receipt; do not invent one. |
| ZSQ-VECNA-11 | Black mass CMD_DEATH moves all room occupants to130076 without a party filter. | Record successful movement for each affected participant independently of killer/credit/party. A bystander, pet or newly arrived occupant does not inherit a personal defeat or source-recovery achievement. |
| ZSQ-VECNA-12 | Bubble130079 has a room periodic callback with null actor; random branches may move all occupants to130074. | Use exact room generation and affected identities with actual arrival outcomes. Preserve random/native behavior and distinguish a float message, pop event, survival and later passage. |
| ZSQ-VECNA-13 | Portal130006@130066 uses ENTER7, unlimited charge-1 and randomly selects130072/73/75 before generic teleport dispatch. | Preserve custom-before-generic admission and command/object identity. `enter death` can mutate a target without resolving the actual portal keyword. Target selection or command return is not admitted arrival; test rejection and replay without credit. |
| ZSQ-VECNA-14 | Five named block_dir statues have directional `_block_*_` and `_nosneak_` keywords. | Model currently present barrier, shared route availability and actual crossing. Retain legal native movement/trust conditions; another player's defeat or an absent statue is not personal combat proof. |
| ZSQ-VECNA-15 | Bone key130005 G225 on Ali130018@130050 gates130005 NORTH↔130049 SOUTH; charcoal key130012 O116@130056 gates130070 NORTH↔130071 SOUTH. Both declare100percent break chance. | Separate admitted custody, near unlock, possible key destruction, reciprocal state, opening and passage. Preserve supplied/already-open and native alternatives; READY destruction can be rejected or pending independently of unlock. |
| ZSQ-VECNA-16 | Gorge/burial routes have secret/closed resets, pit130037 declares F100 and DOWN130038, and embalming130069 has no ordinary incoming local edge. | Preserve actual SEARCH/open/fall/arrival outcomes and branch disclosure. F100 command admission still permits native flight/levitation/mount/climb exemptions and event rejection; scheduling, landing, injury and survival differ. Scripted access is a source route, not an enforced personal hand-in prerequisite. Do not change barriers or fall balance to simplify a journal. |
| ZSQ-VECNA-17 | Enabled130041 ghosthands assignment is immediately overwritten by torturerroom; placed130037 torture controls have no selected literal binding. | Record effective current behavior. Builders review whether this is obsolete suppression or a mistaken binding before a separate named repair; restoring corpse transport can change player-loss and access behavior. |
| ZSQ-VECNA-18 | Effective corpse handler moves the first ITEM_CORPSE north; neither mover has a periodic-only gate after SET. Named Gerd/Aiden/Gueronomous containers are type15. | Qualify command/periodic context, valid location/direction, corpse owner/root/children and authoritative movement. A fair repair needs explicit ownership fences and preserves intended direction/cadence; it must not move the quest-stock containers by name. |
| ZSQ-VECNA-19 | Object periodic dispatch passes null actor; stonemist130040 requires IS_ALIVE(ch), so its periodic path returns without stripping effects. | Document dormant behavior. Builders decide intended effect scope/cadence before activation; any selected fix needs actor-independent room iteration, recipient/trust rules, no-effect/rejection tests and balance review. |
| ZSQ-VECNA-20 | Altar condition is TOUCH OR nonnull argument, followed by an unchecked first-room-occupant NPC cast for avatar130015@130044. | Qualify actual dispatch and intended ritual vocabulary. A scoped safety plan checks argument, rooms, NPC identity and iteration; choosing TOUCH-only changes accepted commands and needs builder intent. Credit only a settled actual summoning. |
| ZSQ-VECNA-21 | Unplaced glyph130002 declares T9/9/-1/60: effect9 is MOVE+NORTH, damage9 has no case among0..8. | Builder chooses intentional unused content, valid declared damage or a deliberate dispel implementation and placement. Never auto-enable an unlimited hazard or claim unsupported selectors already dispel. |
| ZSQ-VECNA-22 | O103 references absent230038@130027; renum_zone_table disables the missing-arg1 reset. | Resolve intended obsolete object, typo or replacement prototype before a separate fix. Do not infer a runtime negative-index crash or replace it with a combat proc just because IDs look similar. |
| ZSQ-VECNA-23 | Four undead rebirth handlers construct replacements at control room130028 without consulting normal reset caps in that function. | Define owned spawn/death episodes, generation/cap policy and failure behavior. Preserve intended renewable population; qualify duplicate/reentrant death and NPC/pet identity before changing availability or rewards. |
| ZSQ-VECNA-24 | Gorge control periodically redistributes NPC occupants from130028 to130003..130027. | Distinguish control-room provenance and successful NPC relocation from ordinary player access/discovery. Qualify pets, unrelated occupants, admission and episode renewal; do not advertise the control room as a public travel service. |
| ZSQ-VECNA-25 | Oaken staff130027 has outdoor healing, indoor life drain/fatal drop-and-death logic, combat effects and SAY abilities, including freedom cooldown. | Add settled effect/life/root-disposition adapters with caster/holder/target/party/cooldown identity. Keyword utterance, item possession or a death message is not a completed buff, rescue or personal survival quest. |
| ZSQ-VECNA-26 | Device130028 changes type/text/wear/anti/affects/timer according to holder class, with several class-specific helpers. | Preserve same-root identity across committed form revisions. Form change is not another first acquisition or native quest reward. Qualify holder changes, type legality, reconnect, rejection and class balance before new objectives. |
| ZSQ-VECNA-27 | Device container reset unlinks a child then advances through that child's next_content. | Qualify multiple-child iteration and authoritative custody before a separate repair. Preserve every child UID/lineage, capacity and owner; save next before unlinking only within an admitted complete transfer design. |
| ZSQ-VECNA-28 | Selected device helpers have actor/payload assumptions; defensive hit dispatch passes the defender/owner and data.victim attacker despite a stale shared comment. | Use the actual typed caller contract. Qualify null/periodic/command/data cases, reentrant death and owner changes; do not claim mask/device procs never fire or change balance from an obsolete comment. |
| ZSQ-VECNA-29 | Hidden helper130038 supplies many native NPC combat effects; shared restriction follows actual root holder for player-owned pets. | Keep NPC-helper versus player gear identity and current custody explicit. Preserve pet restrictions; personal contact or collection is not an accepted quest or permission to activate hidden NPC powers. |
| ZSQ-VECNA-30 | Pestilence, minifist, dispel dagger, bone axe, death mask and helper gear use native combat callbacks/effects. | Record actual admitted combat/effect/kill/survival endpoints if builders author optional challenges. Mere possession, hit attempt or an atmospheric message cannot substitute for those outcomes. |
| ZSQ-VECNA-31 | Vecna carries imported monolith360 and ordinary imported gloves26666. Instance-bound epic stone ownership/payout can differ from prototype file origin. | Preserve local instance/zone, stone UID, frozen eligible party, effective award and consumed claim separately from original item-file zone and both Q receipts. No foreign discovery or hand-in is inferred. |
| ZSQ-VECNA-32 | Imported gloves use transp_tow_misty_gloves: worn periodic stone skin and RUB invisibility/hide/timers. | Follow the actual effect handler rather than inferring teleportation from its function name. Any journal objective needs actual target/effect/cooldown settlement and supplied-versus-source identity. |
| ZSQ-VECNA-33 | Fresh committed epic touch can schedule a mode0 reset; event_reset_zone/no_reset_zone_reset also depend on age/emptiness, zone-done, SQL reset percentage and hourly retry. | Define a proved owned renewal generation instead of UTC replenishment. Accepted Q, submitted touch, secured award, reset request, executed reset and fresh stock are distinct. Qualify cold/ambiguous recovery without duplicate world/player effects. |
| ZSQ-VECNA-34 | Full custom handlers coexist with default conversion/dispatch, absent world.trg, no local teacher/ROOM_INN/switch/proclib and raw1d1+1 stats. | Qualify effective loader order, no_specials, actual actor and intended installation. Raw dice or names do not prove balance defects. New tracking requires active READY accounting; frozen committed recovery stays separate. |
| ZSQ-VECNA-35 | Two potential dailies share mode0 sources and D recipients; no day rollover owns their replenishment. | Keep policy disabled by default. Qualify source/root and giver renewal, partial reward/retirement and truthful availability before enabling daily selection. Real native repairs need a separate clearly named commit and prominent PR/news before/after text. |


## Killing Fields builder case

See the [dossier](../design/zone-stories/KILLING_FIELDS.md) and [journal](../../areas/story/killing_fields.story.json). Declare one exact accepted note return, optional present preparation, semantic versus atmospheric leads, actual actor/party/source/episode outcomes and verified commerce/currency/renewal.

| Follow-up | Source finding | Required capability or fair builder decision |
| --- | --- | --- |
| ZSQ-KILLING-FIELDS-01 | Q14 accepts one exact note33302 for C45000/D, with no earlier prerequisite. | Keep one accepted return and optional current loose preparation. Supplied exact material fits; dialogue, original recovery, decoding, combat, rescue and friend-death history require separate endpoints. |
| ZSQ-KILLING-FIELDS-02 | M2 friend and M9 hi/hello supply three aliases in two addressed families. | Use acknowledged dialogue-family outcomes if optional conversations become objectives. Do not award one achievement per alias or require all words before native acceptance. |
| ZSQ-KILLING-FIELDS-03 | The note has coded extra-description prose but no selected local decoding handler. | Builders specify the intended code, clue disclosure, semantic decoding outcome and actor/episode credit before adding an optional puzzle. Reading or possessing the note does not prove decoding. |
| ZSQ-KILLING-FIELDS-04 | Maelron infers his friend is dead when shown the note; no source links a named friend, rescue or death outcome. | Keep the inference as NPC dialogue. Builders identify the intended colleague and verifiable arrival/rescue/death endpoints before a larger campaign; nearby skeletons and Scarlock are not automatically that friend. |
| ZSQ-KILLING-FIELDS-05 | Note P14 is inside corpse-named type15 container33306 O10@33475, alongside bracelet and two rings. | Capture parent/root UID, reset generation and admitted retrieval. The container is not actual ITEM_CORPSE24 and its name does not prove a player killed its former owner. |
| ZSQ-KILLING-FIELDS-06 | Exact custody may be supplied, transferred, nested, spent or reacquired. | Persist original admitted recovery separately from later ownership and accepted return. A supplied exact note can prepare the offering without receiving personal source credit. |
| ZSQ-KILLING-FIELDS-07 | Skeleton33302 and two shadows33307 are reset at33475; CARVE constructs generic8. | Qualify actual kill, death disposition and source retrieval independently. Defeating local undead, creating a corpse or carving a generic part does not create this exact note. |
| ZSQ-KILLING-FIELDS-08 | Held/worn/nested note, source container, other text and magical stock differ from loose preparation. | Show Missing now/Ready now separately from Recorded. Rendering preparation must not mutate durable history; spending a note must not erase an accepted return. |
| ZSQ-KILLING-FIELDS-09 | C45000 is a native wallet-value reward, with D recipient retirement and no input fee, XP or item reward. | Preserve frozen terms and identified durable currency child recovery, actual wallet/save acknowledgement and retirement separately from accepted receipt. Qualify rejection, ambiguity, disconnect and replay without duplicate money. |
| ZSQ-KILLING-FIELDS-10 | Native offering freezes eligible in-room player identities while currency recovery follows the actor continuation. | Separate party completion credit, actual wallet recipient and each player’s personal recovery. Party presence does not duplicate the coin reward or prove every participant found the note. |
| ZSQ-KILLING-FIELDS-11 | Maelron M30@33469 is nonsentinel/STAY_ZONE; Scarlock M19@33375 is sentinel. | Resolve actual visible NPC generation/location rather than treating reset coordinates as permanent. A previously met giver or stored receipt does not promise present availability. |
| ZSQ-KILLING-FIELDS-12 | All198 local rooms are reachable from33302;730 local exits are reciprocal, plus7 outward boundaries. | Credit admitted arrival in the actual zone. Reviewed foreign leaves establish routes without awarding foreign discovery from a local receipt or promising every declaration is public/active. |
| ZSQ-KILLING-FIELDS-13 | 33422 EAST→Bloody Plains33597 has no direct WEST return; five other plains links and Surface approach reciprocate. | Builders decide whether the outward-only link is deliberate before an exit repair. Preserve current navigation in hints; test actual movement and legal alternative return rather than silently making the boundary reciprocal. |
| ZSQ-KILLING-FIELDS-14 | 33302 prose points north to troll hills, while its current edge joins Surface585374’s jungle. | Confirm current geography before a separate prose repair. Correct explanatory text deliberately; do not rewire the game to match old prose. |
| ZSQ-KILLING-FIELDS-15 | All198 rooms declare TWILIGHT;16 of24 road-named rooms also have NO_MOB. Specter and merchant have road reset placements. | Do not promise invulnerability or an undead-free road. NO_MOB constrains selected mundane movement, not ROOM_SAFE or reset occupancy. Review each mismatch and intended danger before adding flags or changing combat. |
| ZSQ-KILLING-FIELDS-16 | Scarlock, six undead types and camp/struggle prose have no additional accepted local Q or literal custom sequence. | Add optional settled encounter/combat/survival/inspection endpoints only with builder intent and personal/shared episode credit. Looking for someone or a struggle description does not establish another completed quest. |
| ZSQ-KILLING-FIELDS-17 | SHP record labelled33308 binds keeper33308 as a roaming, killable shop; raw windows0..28 and prices0.80/1.10. | Qualify effective source/fallback stream, actual keeper location, stock, pricing and admitted commerce. Static prototype labels or source coordinates are not runtime shop identity or a completed purchase. |
| ZSQ-KILLING-FIELDS-18 | Shop production lists33310/33311/33312/33315; reset G stock also includes vial33309 and sack33314. | Keep produced catalog, carried stock, container contents and player custody distinct. All six G roots are carried separately; an arcane sack name does not put the other stock inside it. |
| ZSQ-KILLING-FIELDS-19 | shop_keeper CMD_DEATH releases artifacts but extracts other carried stock in its selected branch. | Preserve actual stock disposition and authoritative ownership. Do not advertise ordinary merchant gear as guaranteed corpse loot; validate destruction/rejection/recovery before new acquisition objectives. |
| ZSQ-KILLING-FIELDS-20 | Shared shop magic dispatch reads shop_index[number_of_shops].shop_is_roaming after resolving shop_nr; boot count is outside the populated array. | Plan a separately named safety repair using the resolved valid shop record, with focused multi-shop/bounds tests and intended roaming/magic policy preserved. This source finding does not ship a global behavior change in the journal checkpoint. |
| ZSQ-KILLING-FIELDS-21 | Wands/staff/potions, cloak/sandals, dagger/leggings and corpse jewelry have ordinary native item properties. | Use admitted purchase, exact acquisition and settled effect/charge/cooldown outcomes for optional equipment stories. Possession, command attempt, source numbers and raw dice do not prove effect completion or a balance defect. |
| ZSQ-KILLING-FIELDS-22 | Mode1/lifespan15..25 depends on scheduled age and emptiness, caps/if-flags/chances and successful construction; Maelron departs. | Keep the one daily candidate policy-disabled by default. Require owned note/container/giver renewal and truthful availability; a UTC day or reset request is not fresh stock. Qualify cold recovery without repeated rewards. |
| ZSQ-KILLING-FIELDS-23 | No333xx/334xx literal custom binding, local teacher/ROOM_INN/type29/proclib or world.trg declaration; quester/shop defaults and conversion still apply. | Qualify effective boot/no_specials/defaults, actual class/flags/scavenging and installation. Current source hashes or raw level/dice are not full shared-engine or played proof. All new tracking requires active READY accounting; frozen committed recovery stays separate. |
| ZSQ-KILLING-FIELDS-24 | Container33306, exact note33302, bracelet33307 and ring33308 have ITEM_SECRET4096. Room SEARCH reveals a root; targeted SEARCH traverses an open container and normally stops after one successful find. Visibility can restore SECRET on failure. | Explain room search, then container search, chance/visibility and admitted retrieval. Capture personal successful reveal versus shared already-revealed availability, UID/parent/generation and actual custody separately. Do not require personal SEARCH for supplied notes or equate a command attempt/container reveal with recovering the note. |


## Magma builder case

See the [dossier](../design/zone-stories/MAGMA.md) and [journal](../../areas/story/magma.story.json). Map the exact heart source, actual recipient generation, three output children, intended planar entry/return, movement controls, source footprint and original-versus-transferred credit. Qualify optional lore before adding prerequisites.

| Follow-up | Source finding | Required capability or fair builder decision |
| --- | --- | --- |
| ZSQ-MAGMA-01 | QA34 accepts exact I142000 for I142002/I142003/I142004 and D, without native prior prerequisites. | Keep one accepted exchange with optional current loose-heart preparation. Supplied exact material fits; original recovery, personal kill, dialogue and rescue need separately owned endpoints. |
| ZSQ-MAGMA-02 | MA2 hi is broadcast; M15 task is addressed; two aliases describe the same research request. | Add acknowledged semantic dialogue-family outcomes and participant policy before optional conversation objectives. Alias count or ambient echo is not a separate quest achievement. |
| ZSQ-MAGMA-03 | Heart142000 is G21 on Cypheral142018 M20@142127; generic drake142002 has no exact-heart G. | Bind exact source root UID, recipient generation, reset/episode identity and admitted recovery. A generic drake’s heart prose does not establish this proof source. |
| ZSQ-MAGMA-04 | The heart is ordinary carried G stock; death and GET use shared corpse/custody handling. CARVE creates prototype8. | Qualify source death disposition, actual corpse/root release and admitted retrieval independently. Heart item type8 and generic prototype8 are different namespaces; combat alone is not recovery. |
| ZSQ-MAGMA-05 | Exact heart custody may be supplied, held, nested, spent or reacquired. | Show Missing now/Ready now separately from Recorded. Persist original acquisition apart from ownership and acceptance; transferred exact material remains legal without granting personal source credit. |
| ZSQ-MAGMA-06 | The three exact I reward terms are one QA, with no coin or XP term. | Model one accepted return, one input and three outputs. Do not invent three quests, repeat hand-ins, reward choices or staged native prerequisites from the output count. |
| ZSQ-MAGMA-07 | Native identified item reward continuations and D can settle separately from accepted history. | Freeze each output prototype/child UID and original offering operation. Qualify partial reward, rejection, disconnect, ambiguous recovery/save and retirement without duplicate output or a falsely complete bundle. |
| ZSQ-MAGMA-08 | Native offering freezes eligible in-room player identities while rewards follow the actor continuation. | Separate party completion credit, actual item recipient and personal source/combat history. Presence does not duplicate three rewards for every participant. |
| ZSQ-MAGMA-09 | Palenian and Cypheral are nonsentinel, begin at142127; Palenian has STAY_ZONE/NO_SUMMON and movement-related flags. | Resolve actual visible generation/location and native admission. Remembered encounter or reset coordinates do not promise a present giver/source; preserve summon and mobility policy. |
| ZSQ-MAGMA-10 | 142127 has eight exits: E/UP to142021 and six to holding142126; its name says12.5percent. | Treat names as builder labels. Mundane movement draws0..NUM_EXITS inclusive, then checks actor/state/master/sentinel/last direction/traversability. Qualify real availability and distribution rather than publishing the label as a spawn rate. |
| ZSQ-MAGMA-11 | Holding142126 has no exits and shares sector0 with the grid; several control edges point there. | Review whether nonreturning holding is intentional rarity or unfinished dispersal. Specify fair source/giver renewal and any separately named routing repair with bounded population tests; do not silently increase rare access. |
| ZSQ-MAGMA-12 | Grish has two M lines sharing cap2;142128 SOUTH reaches142009, three exits reach holding; F followers become sentinel and follow last M. | Keep actual stock caps, leader/follower lineage and conditional construction. The25percent room name, repeated M or F count is not a guaranteed public escort or independent wandering count. |
| ZSQ-MAGMA-13 | 125 grid rooms have raw sector0/NO_RECALL/NO_PRECIP/NO_HEAL/TWILIGHT; loader adds INDOORS and selected prime predicates classify sector0 as prime. | Builders decide intended planar sector, hazards, healing and travel before repair. Molten prose does not implement a settled heat-survival outcome. Preserve existing flags/balance until played qualification supports a deliberate change. |
| ZSQ-MAGMA-14 | Grid has600 reciprocal edges and no ordinary external edge;517 raw incoming declarations are all unregistered. | Track admitted actual arrival under READY accounting. Exclude inactive map declarations from public hints; obtain an intended public entrance and return design before promising access or adding exits. |
| ZSQ-MAGMA-15 | Named plane table has no Magma selector; cast_gate/cast_plane_shift use listed anchors, while source-room sector affects prime return. | Represent resolved destination, admission and actual arrival separately from a spell attempt. Plan a validated builder access declaration; changing the plane table or sector can alter PvP escape/travel and needs explicit fair design. |
| ZSQ-MAGMA-16 | 442 raw object files contain21682 records/872 type25 declarations, with neither fixed local target nor random-zone value3 local anchor. | Mechanical scans qualify these declarations only. Do not assert every dynamic/script/actor-target destination is impossible; require owned target resolution and played entry/return proof before a public route. |
| ZSQ-MAGMA-17 | Charred platform142013 and torrent142070 are blank descriptions; obsidian gates appear in grid prose without a local gate exit/control. | Separate inspect/scenic leads from admitted traversal and settled hazards. Builders identify actual gate hardware, prerequisites and safe/fair route policy before deeper objectives. |
| ZSQ-MAGMA-18 | Info142000 describes a future Citadel of Fire, giant invasion and dao slave story; info and control leaves are unreachable from the grid by ordinary exits. | Keep planned lore out of mandatory progression. Map a real citadel/raid/rescue source, captive identity, lifecycle and affected participant outcomes before a linked campaign. |
| ZSQ-MAGMA-19 | Many PH/class0/level1 mobiles and blank descriptions include reset-stocked emperor, guardians and emissary; other efreeti/dao/captive prototypes are unplaced. | Record intended builder roles, stock and conversion first. Separate prose/source completion from intentional combat rebalance; raw1d1+1 or names alone do not justify buffing encounters or inventing services. |
| ZSQ-MAGMA-20 | Displayed Palenian has keyword plaenian and raw class0/level60; conversion assigns WARRIOR at level15 or above unless ignored. | Use an existing native alias now. Plan a separate alias/prose repair if builders confirm intent, and deliberate class review for the described planeswalker; do not auto-convert it to a caster or alter combat. |
| ZSQ-MAGMA-21 | Reset mode0/lifespan40..50 has no ordinary boot reset timer; fresh boot, committed epic/SQL percentage/hourly retry and recovery remain separate. | Keep the one potential daily policy-disabled by default. Require owned heart and giver renewal with caps/parent/chance/construction and completed zone context; UTC rollover or stored receipt is not fresh stock. |
| ZSQ-MAGMA-22 | Rewards are wand142002, pipe142003 and vambraces142004; shard142001 is unplaced and no local shop exists. | Use admitted exact reward/acquisition and settled effect/charge/equipment outcomes for optional equipment stories. Possession or item name does not prove use, infinite effects or a purchase service. |
| ZSQ-MAGMA-23 | No literal actual142000..142128 special, local teacher/inn/type29/proclib/world.trg or shop declaration identified; quester/default conversion still apply. | Qualify effective boot/default/no_specials and custom adapters separately from mechanical findings. New discovery, encounters, journal, achievement and daily credit require active READY accounting; frozen committed recovery remains separate. |
| ZSQ-MAGMA-24 | Registry interval139953..142128 mechanically includes140854 ship_shop_proc, but its only reviewed leaf is in unregistered Duris3 and outside actual local rooms. | Use actual registered source ownership for effective script/service reporting. Keep the broad audit as mechanical evidence, exclude this from player contacts, and plan a source-footprint validator; missing real_room0 returns0, not a proven negative-index crash. |


## Mazzolin builder case

See the [dossier](../design/zone-stories/MAZZOLIN.md) and [journal](../../areas/story/mazzolin.story.json). Use one exact prototype per source-labelled material row, one joint accepted return, independently settled rewards and explicit source-versus-transfer credit. Qualify protected access, hidden return roots, current encounters and renewal before deeper prerequisites.

| Follow-up | Source finding | Required capability or fair builder decision |
| --- | --- | --- |
| ZSQ-MAZZOLIN-01 | Q15 accepts exactly I21309/I21310/I21311/I21312/I21313 together for I21316/I21320 and D. | Keep one accepted exchange with five optional present-custody rows. No native earlier dialogue, personal kill, original recovery, rescue, coin, XP or fee prerequisite is defined. |
| ZSQ-MAZZOLIN-02 | M2 hello/hi/help and M10 star are two addressed families/four aliases. | Add acknowledged semantic-family outcomes with actor/recipient/participant policy before conversation objectives. Words or echoes are not four quest achievements. |
| ZSQ-MAZZOLIN-03 | All five shard bodies have identical names/descriptions but different exact prototypes. | Retain one exact prototype per preparation row, source labels and readable Missing now/Ready now/Recorded status. Five copies of one prototype must not prepare the other four. |
| ZSQ-MAZZOLIN-04 | Queen21301 M113@21308 owns G117 shard21313; followers come afterward. | Bind exact source root UID, queen generation and reset lineage. Do not attribute her shard to Arl’Karoth21302 or Zanzilanen21303. |
| ZSQ-MAZZOLIN-05 | White blugrul21308 M137@21331 owns G138 shard21312; other same-prototype placements lack this G. | Qualify the specific stocked instance and current location after movement. Encountering or killing any white blugrul is not proof of acquiring this piece. |
| ZSQ-MAZZOLIN-06 | Zantilanis21315 M222@21410 owns G223 shard21311; the pit has no ordinary exit. | Separate source defeat, admitted recovery and return. Qualify hidden orb visibility/activation and actual destination before making a required dungeon itinerary. |
| ZSQ-MAZZOLIN-07 | First tentamort21325 M239@21421 owns G240 shard21309; another M of that prototype has no matching G. | Preserve cap2 and actual conditional parent construction. Bind source generation rather than granting credit for every tentamort of the same kind. |
| ZSQ-MAZZOLIN-08 | Sludge dragon21328 M246@21422 owns G247 shard21310 and G248 foreign stone358. | Keep shard recovery, rune-stone claim, combat and return as distinct events. A node or a reward item cannot substitute for a star piece. |
| ZSQ-MAZZOLIN-09 | A full raw ZON producer scan finds exactly the five local G sources for the requested pieces. | Own reset/episode/root identity and admitted first recovery. A source scan is declaration evidence rather than proof of current stock, visibility or personal retrieval. |
| ZSQ-MAZZOLIN-10 | Native death/root/GET handling is shared; CARVE creates generic prototype8. | Record actual corpse or equipment release and admitted recipient custody independently of combat. A carved part is not any requested shard. |
| ZSQ-MAZZOLIN-11 | Exact loose pieces may be supplied, held, nested, spent or reacquired. | Separate current preparation from durable history and original acquisition from transfer. Supplied exact bundles remain legal; spending inputs must not erase acceptance. |
| ZSQ-MAZZOLIN-12 | Two I reward terms belong to one accepted exchange; D announces reconstruction and departure. | Freeze both output prototype/child UIDs and original operation. Qualify partial reward, rejection, disconnect, ambiguous recovery/save and retirement without duplicate outputs or invented rescue/escort completion. |
| ZSQ-MAZZOLIN-13 | Native offering freezes eligible in-room party identities; item delivery follows the actor continuation. | Keep party completion credit, actual reward recipient and each actor’s acquisition/combat history separate. Presence does not duplicate two rewards for every participant. |
| ZSQ-MAZZOLIN-14 | Aeirayne21323 M227@21411 is sentinel/class CLERIC; chained/rescue prose does not define an escort endpoint. | Resolve actual visible giver generation and admitted offering. Add explicit captive identity, release/destination/safety outcome and party policy before a larger rescue story. |
| ZSQ-MAZZOLIN-15 | Sixteen D resets include secret throne DOWN flags7/key−2 and reverse UP flags3/key0, both _nobash_. | SEARCH reveals locally; OPEN/PICK/bash and movement have separate native guards. Preserve protected policy; builders qualify intended actors/effects/route before any separately named door repair. |
| ZSQ-MAZZOLIN-16 | Sewer21309DOWN/21311UP reset locked with raw flags2, lacking EX_ISDOOR; keys0/−1. | Ordinary OPEN/PICK/bash reject missing door hardware, but PASSDOOR can admit the closed edge. Decide intended gate/return policy with played evidence before adding ISDOOR, keys or bypasses. |
| ZSQ-MAZZOLIN-17 | Lavish21359UP↔21362DOWN has real locked doors; curtains/council/whirlpool gates start closed. | Record actual reveal/unlock/open/crossing rather than treating door labels or attempts as admission. Preserve skill, held pick, cooldown, combat, reciprocal state and actor conditions. |
| ZSQ-MAZZOLIN-18 | Closed movement allows AFF2_PASSDOOR unless locked AND pickproof after other native gates; KNOCK handles objects, not room doors. | Qualify effect acquisition, duration, secret reveal and actual traversal per actor. Shadow projection, phantasmal form and molecular control are bounded source paths, not a universal current route. |
| ZSQ-MAZZOLIN-19 | Prison branches descend without reciprocal UP;21310N reaches shrine21411, whose only ordinary edge leads into21412 fire-plane grid. | Separate arrival and return. Define reachable route contracts with current barriers, one-way edges and played class/effect/terrain conditions before adding mandatory travel steps or repairing exits. |
| ZSQ-MAZZOLIN-20 | Tar21317@21419 ENTER→21300 and acid21318@21412 ENTER→21421 are unlimited fixed, nonTAKE roots. | Bind selected object UID/generation, argument, action and admitted destination. Teleport attempts, created travel intent and actual arrival are different outcomes; preserve fixed-route policy. |
| ZSQ-MAZZOLIN-21 | Hidden nonTAKE orb21319 has four O placements21381/21385/21388/21410; RUB→21302 unlimited. | SEARCH may clear SECRET only after chance and visibility admission. Own discoverer and shared visibility/reset policy separately from successful RUB/arrival; the orb is not a star piece. |
| ZSQ-MAZZOLIN-22 | Unplaced sphere21308 is TAKE/HOLD, HOLD→21300 with one final charge; active durable ownership has a final-charge gate. | Do not promise usable supply or bypass durable final-charge policy. Any future ownership/charge/arrival change needs an explicit reviewed plan and separately named repair if native behavior changes. |
| ZSQ-MAZZOLIN-23 | Complete442-file/21682-record/872-type25 scan finds exactly four local fixed-target bodies and no foreign local-target declaration. | This covers raw numeric declarations, not every dynamic/script/actor destination. Validate intended ingress/return and actual source-footprint ownership; exclude the inactive incoming map from public routes. |
| ZSQ-MAZZOLIN-24 | All136 local rooms forbid recall/teleport/summon/gate; dungeons/shrine add NO_MAGIC; selected areas add NO_HEAL/BLOCKS_SIGHT and fire-plane sectors. | Qualify effective actor/casting/terrain/hazard admission and settled survival. Prose heat, a decoded flag or a stored arrival is not played safe access or a verified periodic damage outcome. |
| ZSQ-MAZZOLIN-25 | Wand21320 is type3/power45/four charges/spell167/FLOAT, ARMS256 without TAKE/HOLD; sleeves21316 also use arms. | HOLD rejects it, but legacy USE looks in any visible equipment and ARMS wear has actor restrictions. Review intended slot/custody/device effect and reward competition; do not call it unusable or automatically change TAKE/HOLD. |
| ZSQ-MAZZOLIN-26 | Court followers, inquisition, illithid/spider/tentamort variants, barracks and gladiator are native encounters; shadowy blugrul/chef/acolyte lack local M/F stock. | Define encounter, arena, captive and temple outcomes before deeper objectives. Chef is not a shop; source names/duplicate names/raw stats are not proof of service or a reason to rebalance combat. |
| ZSQ-MAZZOLIN-27 | Mode1/lifespan30..45 uses age/empty-zone renewal plus caps/if/chance/construction; stone358 has shared epic_stone binding. | Keep the one potential daily disabled by default. Own giver and all five source renewal, completed reset context and separate committed node publication; UTC rollover, accepted return and epic claim are not interchangeable. |
| ZSQ-MAZZOLIN-28 | Actual local footprint21300..21436 lacks21375; broad registry starts21150. No local literal special/shop/teacher/inn/type29/proclib/world.trg found. | Validate registered complete source ownership and effective boot/default conversion separately. Review stale cave-direction prose and any intended missing content fairly. New discovery/encounter/journal/achievement/daily tracking requires active READY accounting; frozen committed recovery remains separate. |


## Myconid builder case

See the [dossier](../design/zone-stories/MYCONID.md) and [journal](../../areas/story/myconid.story.json). Declare external exact-spore source and alternative return independently of the local key/cache and mushroom passage. Qualify actor-specific access, admitted recovery, currency settlement, supplied-input legality and external renewal before deeper objectives.

| Follow-up | Source finding | Required capability or fair builder decision |
| --- | --- | --- |
| ZSQ-MYCONID-01 | QA18 on giver2320 accepts exact I88502 for C50000, with no D. | Keep one accepted exchange for 50 platinum. No native earlier dialogue, original recovery, combat, crafting, green-key/cache, coin, XP or fee prerequisite is defined. |
| ZSQ-MYCONID-02 | M2 specific and M7 giant/mushrooms/mushroom are two addressed families/four aliases. | Record acknowledged semantic-family outcomes with actor/recipient/party policy before conversation objectives. Words or QA broadcast echoes are not four achievements. |
| ZSQ-MYCONID-03 | Foreign88502/type13 differs from local2301/type18 key,2304..06 foods, gas-spore2304 mobile and mushroom roots. | Use the exact prototype for current loose preparation. Preserve typed identities; generic CARVE8, gills, switches, potions or jar29291 do not substitute. |
| ZSQ-MYCONID-04 | Registered udmini O9 gills88501@88501 owns P10 spores88502, both cap 1. | Bind actual constructed container/root UID, child, episode and reset lineage. Literal stock declarations do not establish current personal recovery. |
| ZSQ-MYCONID-05 | Gills88501 are NOSHOW/NOLOCATE, CLOSEABLE/CLOSED but not locked; native lookup remains keyword-addressable. | Qualify actual visibility/argument/OPEN/GET admission. Do not infer unusability from NOSHOW or invent a required unlock/disarm action. |
| ZSQ-MYCONID-06 | Gills trap512/0/10/60 has OPEN trigger, sleep damage selector0,10 charges,level60 and no ROOM flag. | Opening clears CLOSED before checkopen. Separate admitted open, actor-targeted sleep, awakening/survival and actual recovery; an OPEN attempt or trap echo is not acquisition. |
| ZSQ-MYCONID-07 | Giant mushroom88500/type25 ENTER7→88500 has unlimited charge−1, no TAKE and no literal O/P/G/E producer in all raw ZON files. | Builders qualify intended root availability/access before a new supply plan. Same-number mobile stock is not item supply; dynamic or restored stock remains unqualified, not universally absent. |
| ZSQ-MYCONID-08 | Foreign stem88500 and cap88501 have only reciprocal UP/DOWN edges between them. | Qualify actual actor/magic/current travel and intended return. Do not assert universal entrapment or automatically add an exit or root. |
| ZSQ-MYCONID-09 | Registered newbie M361 mother29225@29275 owns G362 exact88502, cap 1. | Treat this as independent foreign stock with its own source generation/recovery/transfer/availability. It does not prove the intended giant-mushroom route was used. |
| ZSQ-MYCONID-10 | Foreign Maerg29218 Q240 also accepts I88502 for jar29291. | Keep this alternative foreign return independent. One consumed input cannot prove both exchanges, a mandatory chain, foreign discovery or simultaneous extra reward. |
| ZSQ-MYCONID-11 | Both exact88502 source declarations use cap 1 in external zones. | Own global population/conditional construction and completed source renewal. Local reset23 or UTC rollover does not prove fresh external spores. |
| ZSQ-MYCONID-12 | Exact spores can be recovered from a source or transferred by another actor. | Add admitted first-recovery events with source/root/actor/episode lineage and explicit recipient credit. Legal supplied input must not fabricate original gathering or combat. |
| ZSQ-MYCONID-13 | Loose exact88502 prepares now; held, worn, nested or spent stock does not. | Keep transient Missing now/Ready now separate from Recorded acceptance. Spending, transfer and later reacquisition must not erase accepted history or create another receipt. |
| ZSQ-MYCONID-14 | One C50000 output uses native1000 currency units per platinum. | Own acceptance/consumption, identified 50-platinum settlement, save/recovery and publication separately. Synthetic receipt, replay and cold projection do not qualify actual wallet delivery. |
| ZSQ-MYCONID-15 | Native offering freezes eligible in-room party completion identities; currency follows the actor continuation. | Separate party history, actual wallet recipient and each actor’s source/custody/combat evidence. Do not duplicate 50 platinum for every participant. |
| ZSQ-MYCONID-16 | QA narrates thanks; alchemist collecting/brewing prose supplies motivation. | Require actual crafting recipe/input/effect/outcome before a brewing story. Dialogue and a spore return do not establish a produced potion or collection service. |
| ZSQ-MYCONID-17 | Giver2320 M114@2384 is nonsentinel, raw ACT2060; has_quest removes SCAVENGER and native class is ALCHEMIST524288. | Resolve current giver generation/location and effective default binding. Do not rename its class, infer an actual collector, or rebalance raw level40/1d1+1 without played evidence. |
| ZSQ-MYCONID-18 | Type29 roots2314 O32@2374 and2313 O38@2392 use CMD_PULL340 with respective targets2374/EAST and2392/WEST; db auto-binds item_switch. | Own selected root UID, command, argument, actual location/edge and admitted operation. Identical descriptions do not merge the two root targets. |
| ZSQ-MYCONID-19 | Selected item_switch removes near BLOCKED and the actual nonsecret reciprocal BLOCKED edge; it does not toggle/reblock or award. | Separate successful actor operation, shared-open state, admitted crossing and D reset restoration. A handled attempt or another actor’s open passage cannot prove personal operation. |
| ZSQ-MYCONID-20 | 224 exits comprise222 local and two reciprocal registered UnderDark boundaries; all442 raw object files/21682 records/872 type25 have no numeric local target. | Use actual2300UP↔836064DOWN and2392DOWN↔811016UP. Exclude two inactive incoming declarations; this numeric scan does not cover every dynamic/script/actor route. |
| ZSQ-MYCONID-21 | All 93 local rooms have sector13 and five DARK/INDOORS/TUNNEL/SINGLE_FILE flag groups; both dead-end descriptions say back WEST although2392 has ordinary DOWN. | Qualify effective actor/terrain admission and review copied directional prose fairly. Source labels/flags do not prove safe access; no automatic route/prose repair ships. |
| ZSQ-MYCONID-22 | King2303 M91@2368 owns G93 green key2301; locked red cache2302 O29@2368 owns P30 potion2311/P31 potion2310. | Map a separate optional court-key→cache access→two potion recoveries, with exact stock/child/reset ownership. No king kill or key custody is required by the alchemist’s native request. |
| ZSQ-MYCONID-23 | Cache flags13 mean CLOSEABLE/CLOSED/LOCKED with key2301, not PICKPROOF; has_key accepts exact held or loose key, whose break value1 is0. | Qualify admitted UNLOCK/OPEN/GET separately, supplied-key legality and PICK/KNOCK alternatives under actual actor/skill/held-pick/cooldown/casting rules. Do not force personal key combat or equate unlock with retrieval. |
| ZSQ-MYCONID-24 | King flask2300 and cache potions2310/2311 are type10 with distinct raw spell vectors. | Qualify actual drink/quaff effect, recipients, extraction and persistence before potion-use objectives. These are native stock, not products of the alchemist return. |
| ZSQ-MYCONID-25 | Scarlet2304 food has poison value3=1; violet2305/crimson2306 do not. Legacy do_eat applies TAG_EATEN regeneration penalty/sickness echo while poison_lifeleak is commented with TODO. | Separate source recovery, admitted consumption and settled effects. Builders decide intended food penalty versus poison mechanics before a named repair; do not claim a verified poison affliction or silently replace current effects. |
| ZSQ-MYCONID-26 | Pool2312 O36@2388 is unlimited ordinary water with D+S/K+G prose; ring2303 O34@2383 and crystal ooze2314 M113 are separate stock. | Define any future inscription puzzle or water/ring/encounter outcome explicitly. A pool name, letters, ring pickup or combat does not complete another accepted native request. |
| ZSQ-MYCONID-27 | Mode 2/lifespan 5..15 renewal and nonsentinel/no-D giver coexist with external cap 1 spore sources. | Keep one potential daily disabled by default until owned giver/source construction, eligible actor/party acceptance, wallet settlement and completed renewal are qualified. A new day or stored receipt is not fresh stock. |
| ZSQ-MYCONID-28 | Actual local footprint2300..2392 differs from registry first2296; no local literal special/shop/teacher/inn/proclib/current world.trg declaration found. | Review effective generic/default bindings and registered ownership. New discovery/encounter/journal/achievement/daily tracking requires active READY accounting; frozen committed recovery remains separate. Native repairs need separate named fix/news commits. |


## Vargan II builder case

See the [dossier](../design/zone-stories/V2.md) and [journal](../../areas/story/v2.story.json). Declare hidden source recovery, legal supplied exact pieces, joint return and separate later campaign explicitly. Qualify personal reveal versus shared visibility, physical-root storage, optional key/map/access and hazard outcomes before deeper objectives.

| Follow-up | Source finding | Required capability or fair builder decision |
| --- | --- | --- |
| ZSQ-V2-01 | Q13 on Vurlok7822 accepts I7844/I7845/I7846 together for I7847, no D. | Keep one joint return with three optional exact loose-item rows. No earlier dialogue, personal combat/recovery, key/map, rescue, campaign, coin, XP or fee prerequisite is defined. |
| ZSQ-V2-02 | M2 hi and M6 champion are two addressed families/two aliases. | Record acknowledged conversation-family outcomes before dialogue objectives. A keyword or reply is not another achievement. |
| ZSQ-V2-03 | The three different type13 parts have TAKE1, raw SECRET4096/NORENT8388608 and extra2 QUESTITEM32768. | Retain one exact prototype per row. Do not accept three duplicates, a shattered weapon7822, reward7847, scroll7852, key7800 or generic CARVE8 as the joint bundle. |
| ZSQ-V2-04 | Sellis7814 M199@7887 owns G201 hilt7844, cap1. | Bind exact parent generation, child UID and reset lineage. Throne combat, source encounter and admitted recovery remain independent. |
| ZSQ-V2-05 | Black disciple7813 M210@7890 owns G213 cross-piece7845, cap1. | Keep east-chamber source identity separate from black warriors/overlord and the white disciple. Similar names or colours do not establish stock. |
| ZSQ-V2-06 | White disciple7812 M206@7889 owns G209 blade7846, cap1. | Use the west-chamber source. Conjurer7803 M202@7888 occupies the separate southern chamber and does not carry this blade. |
| ZSQ-V2-07 | Red undead7820 M145@7826 owns E146 key7800 in HOLD18; M144 of the same prototype has no key. | Own the specific stocked instance/equipment release/recovery. Any red undead encounter or defeat cannot prove key acquisition, and key custody is not a sword-return prerequisite. |
| ZSQ-V2-08 | Prototype instantiation copies extra flags; selected G and NPC corpse transfer preserve hidden piece state. | Own constructed root and death/corpse containment before recovery. Defeating a source does not automatically reveal or put its hidden piece in player custody. |
| ZSQ-V2-09 | SEARCH can clear a selected hidden child on chance and admitted visibility; ordinary room-container GET checks visibility. | Separate actor reveal, shared visibility, selected corpse/container root, actual GET/custody and later transfer. Do not invent SEARCH history as a prerequisite for an already supplied exact bundle. |
| ZSQ-V2-10 | Carried/worn local-container GET paths have distinct visibility/weight/takeability exceptions. | Qualify actual container topology and actor admission rather than declaring every hidden piece universally unrecoverable. Accounted transfer/settlement still needs independent evidence. |
| ZSQ-V2-11 | Parts declare NORENT; key conversion also applies NORENT. | Qualify current owned-item save/logout/recovery and intended storage policy before persistent acquisition objectives or a repair. Synthetic cold journal restoration does not prove these physical roots survive rent/logout. |
| ZSQ-V2-12 | Exact parts may be recovered personally, supplied, transferred, spent or reacquired. | Add first-source recovery with actor/source/root/generation lineage and explicit transfer credit. Native legal supplied input must not fabricate original gathering or combat. |
| ZSQ-V2-13 | Current loose preparation is transient; recorded acceptance survives spending. | Keep Missing now/Ready now independent of Recorded history. Held/nested stock or repeated rendering must not mutate durable completion or turn one return into three quests. |
| ZSQ-V2-14 | One I7847 output follows joint consumption of three exact inputs. | Own identified reward child, acceptance/consumption, settlement/save/publication and ambiguous recovery. No D retirement exists; replay cannot mint another sword. |
| ZSQ-V2-15 | Eligible in-room party identities freeze completion history; the reward follows the actor continuation. | Separate party credit, actual sword recipient and each actor’s gathering/combat history. Presence does not duplicate one sword for every participant. |
| ZSQ-V2-16 | Q response says knowledge to kill Vargan and advises a distant graveyard/back entrance; the native output is one item. | Declare campaign start, foreign boss/route identity, actual skill teaching if intended and victory/end state separately. Advice is not a learned skill, foreign discovery, prerequisite chain or completed campaign. |
| ZSQ-V2-17 | Vurlok7822 M237@7918 is sentinel/ISNPC/MEMORY/HUNTER and native NECROMANCER1024. | Resolve actual visible giver generation/availability. Prison/rival/champion prose does not define release, escort, destination or safety; add explicit rescue outcomes before a larger story. |
| ZSQ-V2-18 | Many legacy exit labels use raw5; setup_dir masks state to its low two bits, and all30 D resets use state0. | Raw5 constructs an ordinary door; D0 clears CLOSED/LOCKED and adds no SECRET/BLOCKED. Qualify current restored or dynamically changed state, actor OPEN/crossing and personal operation separately; raw labels or shared state do not prove a personal reveal. |
| ZSQ-V2-19 | Throne7887SOUTH↔7888NORTH has legacy raw9, masked to ordinary door state1; D0 adds no blocked flag. | Review intended conjurer-room access against actual fresh-load/open state and any restored or dynamic flags. Ordinary movement rejects a currently blocked edge, but the raw9 label alone does not create one. The conjurer is separate from all requested sword sources. |
| ZSQ-V2-20 | Burial bronze raw2/key7800 constructs ISDOOR/PICKABLE; D0 leaves it open. Temple panel raw5/key7801 constructs an ordinary door; the key prototype is claws armour. | Qualify current restored/dynamic state and intended hardware/key policy before repair. Native has_key uses exact identity rather than item type; neither editor labels nor unusual keys prove a currently required closed or hidden gate. |
| ZSQ-V2-21 | Hidden drawer7851 O129@7917 is nonTAKE/SECRET, open values4/0/0/100, and owns P130 scroll7852. | Map admitted drawer reveal→scroll retrieval→actual reading separately. The ASCII map, table/keyhole prose and unplaced table7850 do not define a coded puzzle or require a nonexistent lock. |
| ZSQ-V2-22 | Reward7847 is type5, TAKE/WIELD8193, TWOHANDS with additional flags, raw2d4 weapon dice and affects18/5,19/6. | Qualify actual actor/class/wield/combat/effect policy before a required weapon-use or foreign-boss objective. Narrative enchantment does not install a specifically verified Vargan kill outcome. |
| ZSQ-V2-23 | All23 mobile bodies have M stock; court/overlords, guardian/abishai, triton, death tyrant, grimlocks, undead and crazed adventurer are distinct leads. | Define encounter, feeding, captive/rescue or religious outcomes before deeper objectives. Names and duplicate prototypes are not services or justification for combat/stat rebalance. |
| ZSQ-V2-24 | Flooded7846..7852 have UNDERWATER flags41/45 and sector9; native entry schedules breathing/drowning under actor/effect conditions. | Own admitted entry, effect availability/duration, breath/drowning ticks and settled survival/return. A room label, event intent or journal arrival is not a played hazard objective. |
| ZSQ-V2-25 | Upper7892..7896 use NO_GROUND8;7893..7896 lack DOWN. Falling fallback converts such a room to INSIDE; a leave_by_exit light/effect branch dereferences target DOWN without a null guard. | Review intended upper geometry and current fallback/actor/light/mount state. Plan a separately named null-guard/regression fix preserving travel policy; qualify settled fall/survival before objectives. Do not automatically add exits or declare the whole sword request inaccessible. |
| ZSQ-V2-26 | Actual source keywords include albinor and daeth; some equipment extras/descriptions retain copied names. | Use valid current contact aliases and exact typed item/source identity. Record fair keyword/prose consistency review before separately named copy repairs; no class/slot/stat rewrite is inferred. |
| ZSQ-V2-27 | Mode2/lifespan35..45 and capped/conditional/chance construction govern three cap1 pieces plus a no-D giver. | Keep one potential daily disabled by default until completed giver/source renewal, READY joint acceptance and reward settlement are qualified. UTC rollover, a receipt or an attempted reset does not prove fresh pieces. |
| ZSQ-V2-28 | Actual119 rooms7800..7918 have249 exits and one reciprocal registered vargan2182 boundary; all442 files/21682 objects/872 type25 have no numeric local target. Selected local literal special/shop/teacher/inn/type29/proclib/current world.trg scans found no declaration; fresh exit construction is distinct from editor labels. | Keep raw scans bounded to declarations and qualify effective default/registered ownership separately. New discovery/encounter/journal/achievement/daily tracking requires active READY accounting; frozen committed recovery remains separate. Repairs need named fix/news commits and prominent before/after evidence. |


## Phantasmagoric Caverns builder case

See the [dossier](../design/zone-stories/VALDRAK.md) and [journal](../../areas/story/valdrak.story.json). Declare exact three-source return, valid dialogue family, hidden heart recovery versus supply, capped reward recipients, giver/follower retirement and intended ritual/community state. Keep garden, spring, shop and single-file journeys independent.

| Follow-up | Source finding | Required capability or fair builder decision |
| --- | --- | --- |
| ZSQ-VALDRAK-01 | Q40 on Xolot53110 takes I53108/I53117/I53118 together for C81000/E13300 with D. | Keep one joint accepted return and three optional exact loose-item rows. Dialogue, personal kills/recovery, shop, garden, spring and ritual are not native prerequisites. |
| ZSQ-VALDRAK-02 | Raw M2 gravf, M14 valdrak and M28 punctuation/acopiltaczet form three addressed families. | Own acknowledged conversation-family outcomes. Keywords and replies are not three achievements or prior return requirements. |
| ZSQ-VALDRAK-03 | The inventory omits the whole M28 family because one whitespace alias has a comma; acopiltaczet is valid. | Plan a separately scoped inventory regression/correction retaining safe exact aliases without silently normalizing punctuation. Requalify affected inventory outputs before global regeneration. |
| ZSQ-VALDRAK-04 | Native isname treats whitespace tokens exactly; azopiltaczet without the comma does not match the raw first alias. | Use valid acopiltaczet now. A builder may choose a separately named keyword/prose consistency fix, with before/after news evidence; no native data repair is assumed. |
| ZSQ-VALDRAK-05 | Gravf53132 M309@53190 owns G310 longsword53108, cap1. | Bind exact source parent generation, child UID and reset lineage. Nearby sword spiders or combat cannot prove original sword recovery. |
| ZSQ-VALDRAK-06 | Acopiltaczet53134 M305@53187 owns G306 claymore53117, cap1. | Keep this exact source separate from Gravf, ValDrak and other web-cave spiders; actor combat and accepted custody are independent. |
| ZSQ-VALDRAK-07 | ValDrak53131 M318@53199 owns G320 heart53118, cap1; the heart declares SECRET4096. | Own source encounter, death/corpse construction, admitted reveal and actual heart recovery separately. The two weapons have no SECRET declaration. |
| ZSQ-VALDRAK-08 | G319 on ValDrak is Raptor object53110; mobile53110 is the separate giver. | Preserve typed identity and the three exact requested prototypes. Raptor, moss stones, shop goods or duplicate weapons cannot substitute. |
| ZSQ-VALDRAK-09 | Instantiation/G stock and selected NPC corpse transfer preserve root flags and containment lineage. | A killed source does not automatically reveal or deliver its heart. Qualify the actual corpse/container root, generation and actor visibility. |
| ZSQ-VALDRAK-10 | SEARCH can reveal a hidden child on chance; room-container GET and carried/worn-container exceptions differ. | Separate actor reveal, shared visibility, actual GET/custody and later transfers. Do not require invented SEARCH history for supplied legal input. |
| ZSQ-VALDRAK-11 | Exact items can be supplied, transferred, spent or reacquired; none declares NORENT in its raw extra field. | Add first-source acquisition and transfer policy with owner/root/generation lineage; qualify physical save/logout custody independently of synthetic journal recovery. |
| ZSQ-VALDRAK-12 | Loose preparation is current inventory; accepted completion persists after spending. | Keep Missing now/Ready now separate from Recorded history. Held/nested stock, duplicate items, rendering or partial deliveries cannot create another completion. |
| ZSQ-VALDRAK-13 | C81000 is a nominal currency output, separate from XP and party history. | Own actual actor currency amount/recipient, wallet settlement/save/publication and ambiguous recovery. Presence does not grant the same cash to every party member. |
| ZSQ-VALDRAK-14 | E13300 is nominal XP; frozen actor and eligible companion levels use different caps. | Qualify frozen identities, XP entitlement, actual capped amount, recipient save and recovery. Do not promise 13300 XP to every participant or duplicate currency through party credit. |
| ZSQ-VALDRAK-15 | D departure follows the accepted reward continuation; carried and equipped giver roots are extracted. | Own reward completion and giver-generation retirement separately. Hidden potion/scroll on Xolot are not unconditional post-return loot. |
| ZSQ-VALDRAK-16 | F207 creates zombie53129 after Xolot, with E children; extraction invokes follower/group cleanup. | Qualify actual follower/master generation, group changes and residual equipment/root policy after retirement. The follower is not a personal rescue, escort or independently completed story. |
| ZSQ-VALDRAK-17 | Mode2/lifespan3..9 uses capped/conditional/chance construction; three sources and giver are cap1. | Qualify completed source and giver renewal plus accounted acceptance. UTC rollover, attempted reset or a departed giver receipt does not prove a fresh daily bundle. |
| ZSQ-VALDRAK-18 | The response says the community is free and a later ritual ends evil; the native terms are C/E/D. | Declare intended ritual start, participant, actual operation, endpoint and community-state effects before ritual/restoration objectives. Departure prose is not a played ceremony. |
| ZSQ-VALDRAK-19 | Elders, paranoid myconids, Muscaria and the ancient myconid have independent stocked bodies. | Define rescue, trust or restoration outcomes if intended. A returned bundle or similar myconid name does not prove behavior changes, teaching or an external community quest. |
| ZSQ-VALDRAK-20 | Wejtew53156 is the shopkeeper at53119; current native class16 is ANTIPALADIN. | Qualify shop admission, hours, generation and primary/secondary callback ownership independently. Narrative wisdom and caster-looking stock do not install a teacher. |
| ZSQ-VALDRAK-21 | Shop stock contains three local goods and nine external registered smokev/kastle prototypes. | Own actual purchase, pricing, stock and recipient settlement. These are services, not requested source substitutes, foreign discovery or a quest fee; keeper-death stock cleanup differs from ordinary loot. |
| ZSQ-VALDRAK-22 | Spring53100 O10@53119 is type17 with values9000/9000/0/0; room uses underworld-water16 and no UNDERWATER flag. | Separate admitted visible-container drinking, finite amount/condition changes and optional use from sector movement, breathing and current root/water behavior. Spring/vapor prose does not prove drowning or a cure. |
| ZSQ-VALDRAK-23 | Purple53101 and speckled53105 food use value0 durations33/43, poison0 and epic value5 zero. | Own recovery, admitted eating, actual TAG_EATEN effects, extraction and settlement. Existing regeneration/satiety paths and commented poison code need fair effect/prose review before a named fix; no hallucination/level reward is inferred. |
| ZSQ-VALDRAK-24 | Base rooms declare DARK/NO_RECALL/NO_SUMMON/NO_GATE; selected NO_MAGIC/NO_PSI/NO_TELEPORT/BLOCKS_SIGHT/HEAL differ. | Qualify actual actor spell/effect admission, sight and return policy. No local ROOM_TUNNEL, PRIVATE, NO_HEAL or INN is declared; prose cannot create those restrictions or services. |
| ZSQ-VALDRAK-25 | 53128/53130 each have two exits and SINGLE_FILE; mounted entry and occupancy/list swapping affect movement. | Own actual crossing and return separately from a bypass that only swaps people and returns FALSE. Current actors, posture/combat and trusted/wraith exceptions govern admission. |
| ZSQ-VALDRAK-26 | Raw4 on53157DOWN/53177UP and53194EAST is masked to zero by setup_dir; there are no D resets. | Review intended access versus actual fresh-loaded/current/restored/dynamic state. Do not create mandatory SEARCH or an access repair from a legacy raw label alone. |
| ZSQ-VALDRAK-27 | 100 rooms53100..53199 have234 edges and two reciprocal registered underdark boundaries; an ud_map2 incoming leaf is inactive. | Preserve actual registry/route identity and return policy. All442 files/21682 objects/872 type25 have no numeric local target; dynamic/script/actor destinations remain outside the raw scan. |
| ZSQ-VALDRAK-28 | All57 mobile prototypes have M or F stock, including garden duplicates, dismal creatures and web-cave variants. | Keep exact valid contact aliases and typed source identities. Define wider encounter, feeding or exploration outcomes before awards; duplicate names are not interchangeable request sources. |
| ZSQ-VALDRAK-29 | Xolot owns ring/belt and hidden potion53106/scroll53107 before the follower reset. | Separate optional equipment/reveal/recovery, use and disposal from accepted quest outputs. Their descriptions, spell values and retirement do not establish a required ritual item or guaranteed collection. |
| ZSQ-VALDRAK-30 | Raw ACT_TEACHER/proclib/type29 and selected local literal special/current world.trg scans find no declaration; generic quester/shop/default paths still apply. | Keep absence claims bounded; qualify effective ownership separately. New discovery/encounter/journal/achievement/daily tracking requires active READY accounting. Native repairs need separate named fix/news commits with prominent before/after evidence. |


## Bronze Citadel builder case

See the [dossier](../design/zone-stories/BCTDL.md) and [journal](../../areas/story/bctdl.story.json). Declare independent seal/heart returns, practical keys/cube/prison routes, rejected hazard versus acquired custody, personal operation versus shared access, actual gate arrival and accounted mode0 renewal. Custom worn-owner effects, rescue/restoration endpoints and response/reward consistency need explicit integration.

| Follow-up | Source finding | Required capability or fair builder decision |
| --- | --- | --- |
| ZSQ-BCTDL-01 | QA11 seal32483 → key32422 and QA23 heart32490 → armor32029/D on Zariel32448 are separate native bindings. | Keep two request cards with optional exact loose preparation and independent accepted receipts. No native remembered seal, dialogue, source kill, key use or cube prerequisite is declared for the heart return. |
| ZSQ-BCTDL-02 | MA2 hello and QA responses use echoAll; room witnesses can hear the response. | Own addressed acknowledged conversation versus witness exposure and credited acceptance separately. Room broadcast does not grant everyone a keyword achievement or completion. |
| ZSQ-BCTDL-03 | Dungeon Master32439 M348@32482 owns G351 seal32483 and G350 key32482, cap1. | Bind parent generation, child UID and source lineage. Recovering its seal, acquiring its key and opening Zariel’s cell are distinct; equipment or a defeated source is not item custody. |
| ZSQ-BCTDL-04 | Bel32420 M281@32469 owns G284 heart32490, cap1; followers and other equipment have distinct roots. | Own exact source battle, corpse/root creation and personal acquisition separately from supplied input or the actor’s accepted return. |
| ZSQ-BCTDL-05 | Seal and heart are TAKE/QUESTITEM, with no raw SECRET or NORENT; key32422 is NORENT. | Do not invent hidden-heart SEARCH. Qualify actual visibility/custody and physical-root save/logout, key disposal and supplied-item policy independently of synthetic journal recovery. |
| ZSQ-BCTDL-06 | Heart T6/2/1/50 is OBJECT+ROOM/fire/one charge/level50; dagger32449 T2/2/1/50 is object/fire. | Qualify admitted trigger, charge consumption, room-wide actor/recipient damage/death/effect settlement and later recovery. Triggering a hazard is not surviving it or acquiring the item. |
| ZSQ-BCTDL-07 | Single/bulk GET call checkgetput before ownership submission and reject the triggering selection. | A first trapped pickup cannot award first acquisition. Record actual successful later custody with actor/source/root/generation identity; distinguish unrelated items in a bulk operation. |
| ZSQ-BCTDL-08 | Current PUT5735..6007 has a Trap check comment but no checkgetput call; current call sites are only GET. | Keep the observed GET-only boundary explicit. A builder can separately review intended PUT or rearm behavior and balance; do not install a new hazard or promise current PUT damage from the helper’s name. |
| ZSQ-BCTDL-09 | Exact supplied, transferred, spent and reacquired items remain legal; current preparation is ephemeral. | Add distinct first-source acquisition and transfer policies with durable lineage. Ready now/Missing now must not erase Recorded history or grant source credit to a supplied-item return. |
| ZSQ-BCTDL-10 | Seal acceptance issues one exact key32422; it is not current unlocked-door state. | Own actual reward root/UID, recipient settlement/save/recovery, key use and later disposal. An accepted receipt or displayed key hint is not proof of possession or unlocking. |
| ZSQ-BCTDL-11 | Heart acceptance issues one I32029 armor from registered avernus, then D retirement. | Own reward issuance, recipient root/save/publication and ambiguous recovery separately from frozen eligible party history. A foreign reward does not discover or complete Avernus. |
| ZSQ-BCTDL-12 | The QA/D response promises two items/potions while I32029 is one type9 Nessus royal guard platemail. | Fair repair decision: correct prose to the intended existing armor, or separately evaluate any intended reward change with exact IDs, balance and recovery compatibility. No guessed potions or silent reward rewrite. Use a named fix/news commit and prominent before/after PR/news evidence. |
| ZSQ-BCTDL-13 | The heart reply narrates devouring, transformation and shadow departure; only I/D terms define outputs. | A transformation or restored sovereignty story needs actual operation, participant and state endpoints. Acceptance and extraction do not prove a playable ritual or changed regional power. |
| ZSQ-BCTDL-14 | D extracts giver roots and the giver; seal return leaves the giver present. | Qualify giver-generation retirement, visibility, equipment/follower lifecycle and recovery. Pending/asynchronous output and missing giver cannot silently convert to a second completion. |
| ZSQ-BCTDL-15 | Key32422 fits foyer32471EAST ↔32472WEST; the supplied heart has no remembered key prerequisite. | Describe practical route without imposing personal access history on legal native acceptance. Separate possession, admitted unlock/open, shared state and actual crossing/return. |
| ZSQ-BCTDL-16 | Glowing32425, white32423, bronze32484, obsidian32485 and platinum32426 keys come from different parents. | Bind exact key source/generation and each door side. Similar names, unused keys and room numbers are not substitutes; recovery, unlock and passage have separate outcomes. |
| ZSQ-BCTDL-17 | White32488, demonbone32489, unremarkable32482 and red prison32479 keys govern distinct upper/dungeon edges. | Qualify intended route, chance/caps and current secret/locked states, including asymmetric reciprocal key declarations. Do not promise every prisoner or chamber is currently reachable. |
| ZSQ-BCTDL-18 | Raw D6 becomes SECRET+LOCKED and D8 becomes BLOCKED when reset executes; raw loader masks high bits. | Own actual fresh/reset/restored/dynamic state. Mandatory SEARCH, unlock or switch credit needs admitted current state and actual route outcome, not raw labels alone. |
| ZSQ-BCTDL-19 | Cube32421 O138@32480 is type29, NOSHOW, wear0, values42/32471/4/0; current CMD_STAND42. | Declare stand cube using exact command and target foyer UP. The room prose gives the standing clue; direct non-track NOSHOW keyword lookup differs from normal PC object visibility. Resolve actor admission and current blocked state; do not require PUSH, pickup or an invented invisible cube. |
| ZSQ-BCTDL-20 | item_switch clears near BLOCKED and handles reciprocal state according to SECRET; target is remote from the cube. | Separate personal successful operation, shared door state and later admitted crossing/return. Seeing a pre-opened route does not prove the player operated the cube; define restoration/save/reset ownership. |
| ZSQ-BCTDL-21 | Cube T4/5/1/60 declares ROOM/energy but no MOVE/OBJECT/OPEN trigger bits in reviewed admission paths. | Record unresolved trap intent. Review a fair separately named fix only if the builder chooses an actual trigger; do not silently activate a new trap or award cube-use hazard from this declaration. |
| ZSQ-BCTDL-22 | Foreign gate32013 O9@avernus32057 →32420; local gate32436 O137@32420 →32057, command ENTER7, charge-1. | Qualify exact live gate, actor command/visibility/arena policy and actual arrival/return. Unlimited charges and declarations do not prove accounted source issuance or successful travel. |
| ZSQ-BCTDL-23 | Full type25 scan covers442 files/21682 objects/872 type25 records, one numeric local target and no unparsed records. | Keep raw numeric-route scope bounded. Administrative realm14208/wh55633 incoming leaves and dynamic/script routes are separate from ordinary player paths or reciprocal exits. |
| ZSQ-BCTDL-24 | check_item_teleport returns TRUE after teleport_to; teleport_to ignores char_to_room’s return. | Award discovery/access only from actual admitted room state. Handler consumption, command intent or emitted messages cannot prove arrival. Review any failed-transfer handling separately with actor/state evidence. |
| ZSQ-BCTDL-25 | Mode0/lifespan40..50 has no ordinary boot periodic event; fresh boot resets, copyover/Redis recovery preserves. | Daily/replay availability needs completed accountable source/giver generation renewal. Do not enable periodic mode or assume elapsed time/UTC rollover creates a new key, heart or giver. |
| ZSQ-BCTDL-26 | Committed epic touch can schedule mode0 reset; event/no_reset_zone_reset uses DB-backed completion/chance conditions. | Qualify exact zone identifiers, frozen participants, current completion, reset request versus executed generation and recovery. A pending stone touch is not a completed renewal; no live DB/reset action is authorized by mapping. |
| ZSQ-BCTDL-27 | Active economic authority rejects O/G/E and other reset item issuance before read_object for missing generation identity. | Implement accountable reset-generation issuance before claiming READY fresh stock. This affects gates, cube, requested items, keys and equipment; preserve restored/existing roots and fail closed, without bypassing accounting. |
| ZSQ-BCTDL-28 | Room flags differ: NO_HEAL/sight restrictions, Zariel NO_MAGIC/NO_PSI, HEAL32485/32497 and NO_MOB groups; cold cell sector25. | Own actual actor spell/effect/sight/movement and any settled heat/cold hazard. No local DARK/TUNNEL/PRIVATE/INN/UNDERWATER is declared; descriptive heat/prison magic does not create an adapter or service. |
| ZSQ-BCTDL-29 | bel_sword32486 has worn-owner eld/SAY240-second flame operation, REMOVE cleanup and demon/devil-only random damage/vamp proc. | Add admitted equipment operation/effect/combat adapters with owner/UID/cooldown/recipient/settlement/save policy. Saying eld or wielding the sword does not grant the seal or heart return. |
| ZSQ-BCTDL-30 | artifact_invisible32428 has worn-owner invisible/SAY60-second operation; cube uses automatic type binding; no local literal mobile assignment. | Keep actual callback/default/quester ownership and actor spell admission explicit. No raw ACT_TEACHER or configured world.trg declaration is present; bounded absence is not a whole-engine audit. |
| ZSQ-BCTDL-31 | Thirty-five mobile prototypes include two without local M/F stock, duplicate goblin variants, humans, dragons, guards and prison occupants. | Map exact encounters and optional exploration/recovery leads. Define rescue, release, alliance, wing victory and follower outcomes with builder endpoints before awards; narrative captivity is not a completed rescue. |
| ZSQ-BCTDL-32 | Catalog retains one potential seal daily; heart is story-only and runtime resettable policy sees mode0. | Keep daily policy disabled pending active READY acceptance, actual issuance/settlement/save/recovery/retirement and completed renewal. Preserve all native definitions/previous maps/order; repairs require named fix/news commits and prominent before/after evidence. |


## BrimStone Forge builder case

See the [dossier](../design/zone-stories/BRIMEFORGE.md) and [journal](../../areas/story/brimeforge.story.json). Declare three distinct locket sources, simultaneous accounted turn-in, independent rune acceptance, exact fragile keys/current pickproof access and actual rescue/device outcomes. Missing static sources and incorrect key references require intended builder decisions, distinct from player supplied-input legality.

| Follow-up | Source finding | Required capability or fair builder decision |
| --- | --- | --- |
| ZSQ-BRIMEFORGE-01 | Q8 takes exact lockets131002/3/4 for wand131005, XP300000 and ethereal key131006; Q18 rune131019→mithril key131012, both without D. | Keep two independent request cards with four optional exact loose checks and two recorded acceptances. Neither native Q declares remembered greeting, earlier return, personal battle or access history. |
| ZSQ-BRIMEFORGE-02 | M2 hi/hello/hey/howdy is one native addressed greeting family, not four quests. | Record acknowledged addressed encounter separately from keyword progression or room exposure. One family can show four suggested topics without granting four achievements. |
| ZSQ-BRIMEFORGE-03 | The same overseer131008 has M121@131004/E122second131003, M129@131009/E130third131004 and M149@131016/E150first131002. | Bind exact parent instance/reset generation and child UID. Killing three copies or obtaining duplicate lockets is not the exact three-item set. |
| ZSQ-BRIMEFORGE-04 | Each locket E has cap1/neck slot3; overseer cap3; all16 mobile prototypes have M/F placement. | Qualify worn source→actual corpse/recovery/custody and source caps before claiming personal acquisition or fresh availability. A contact/source encounter does not reveal an unmet giver’s card. |
| ZSQ-BRIMEFORGE-05 | Current preparation checks exact actor loose inputs; held/worn/nested/wrong prototypes and repeated copies differ. | Render Ready now/Missing now without persisting possession or accepting substituted inputs. Recorded accepted history survives spending and cold/raw receipt recovery. |
| ZSQ-BRIMEFORGE-06 | Legacy completion counts giver loose holdings; durable offering selects all required distinct actor roots in one batch. | With active accounting keep incomplete sets on the actor and require all three together. Do not describe staged legacy deposits as the accounting turn-in flow or credit earlier contributors without explicit policy. |
| ZSQ-BRIMEFORGE-07 | Supplied exact lockets/rune fit native acceptance without source combat or first-return history. | Separate source acquisition/lineage and player transfer from accepted return. First-source achievements need explicit actor/root/generation and transfer rules; do not invent prerequisites for existing native Q. |
| ZSQ-BRIMEFORGE-08 | Lockets, rune and fragile keys declare NORENT; physical roots can differ from durable journal history. | Qualify physical save/logout/recovery and loss policy separately from synthetic receipt serialization. A recorded return cannot recreate spent inputs or grant current key possession. |
| ZSQ-BRIMEFORGE-09 | First locket tail stores bitvector4=32, AFF4_STORNOGS_GREATER_SPHERES, not first-vector sense life. | Own actual admitted wear/effect/protection, UID/state/save and balance intent. Numeric fourth-vector flag or equipment text cannot award an effect or cleanse the mines. |
| ZSQ-BRIMEFORGE-10 | Wand131005 has power41, charges4/4 and spell277 IMMOLATE; full USE/device channeling paths differ. | Qualify actor/source/target runtime identity, command admission, charge commit, interruption and actual effect/recipient settlement/save. USE intent or charge consumption is not successful combat effect. |
| ZSQ-BRIMEFORGE-11 | Selected spell_immolate ignores passed level/power and uses caster-level fire damage with recurring pulses/random expiry. | Describe the actual combat device without promising power41 damage or purification. A builder balance decision on device power/effect is separate from journal acceptance and requires named fix/news evidence if changed. |
| ZSQ-BRIMEFORGE-12 | Ethereal key131006 and mithril131012 have value1=100 break chance; has_key uses exact HOLD/loose vnum. | Separate current possession, unlocked shared state, durable key destruction success/rejection, later passage and cold recovery. No nested key lookup or automatic ownership receipt from unlock text. |
| ZSQ-BRIMEFORGE-13 | 131018EAST→forge131027 uses key131006/raw3/resetD2; reverse131027WEST is raw1/resetD1. | Qualify asymmetric sides/current state. Raw3 adds PICKPROOF, so ordinary pick fails on the loaded locked side; legal supplied key and reverse/alternate routes remain separate from personal efreeti history. |
| ZSQ-BRIMEFORGE-14 | Forge131027NORTH→131042 is locked raw3/resetD2 with key0; reverse side is closed raw1. | Record unresolved intended portcullis key/control. Check admitted alternate/shared/restored routes before declaring all deeper access impossible; do not remove lock/pickproof or weaken encounter balance as a journal fix. |
| ZSQ-BRIMEFORGE-15 | Guardian131011 M192@131038 has independent Q18 rune return, empty reply and no D. | Bind actual acknowledged guardian encounter and exact supplied rune acceptance independently of the earlier efreeti return. Define response/context and generation policy without imposing invented access history. |
| ZSQ-BRIMEFORGE-16 | Rune131019 has no raw O/G/E/P producer in888 scanned files, no other exact numeric raw QST/src match. | Keep absence bounded to reviewed static sources. Builder chooses an intended source/quest/recovery with exact ID/cap/balance and accountable generation; computed DB/script/crafting/admin/supply sources are not disproved. |
| ZSQ-BRIMEFORGE-17 | Mithril key131012 is guardian reward only in reviewed declarations and fits northern cell131038NORTH→131039. | Qualify actual reward root/recipient/save, key break and cell crossing; current key possession and Kaynor rescue differ from accepted rune history. Preserve supplied rune legality. |
| ZSQ-BRIMEFORGE-18 | General131016 M163@131027→G164large key131013, cap1; F165/166/167 guards131015. | Bind parent source/UID/generation, battle, root custody, follower lifecycle and eastern cell131038EAST→131040 crossing. A general defeat or meeting a guard is not key acquisition or prisoner release. |
| ZSQ-BRIMEFORGE-19 | Southern cell131038SOUTH→131041 requires131014, which is the non-takeable massive chest prototype. | Fair repair: confirm intended key/control, then correct only that reference or intended source with balance/access/recovery tests in a separate named fix/news commit. Preserve the chest’s carry restrictions; journal mapping does not unlock the cell. |
| ZSQ-BRIMEFORGE-20 | Chest131014 O118@131048 is nonTAKE/NORENT container values0/29/131020/0; key131020 has no raw stock producer. | Normal PICK and current container-only KNOCK refuse PICKPROOF29; search requires an open container. Builder chooses exact key source and intended contents/endpoints, avoiding guessed rune-in-chest, takeability or free access changes. |
| ZSQ-BRIMEFORGE-21 | Six crack routes use raw5/resetD5 and reciprocal raw1/resetD1; loader masks raw high bits, D adds SECRET/CLOSED. | Own current successful SEARCH/reveal, OPEN/shared reciprocal state and actual actor crossing/return separately. Witness messages, a pre-revealed passage or raw labels cannot award personal exploration objectives. |
| ZSQ-BRIMEFORGE-22 | 49 local rooms/98 edges include two registered reciprocal foreign entrances;196 Duris3 incoming declarations are inactive; type25 numeric scan has no local-range target. | Use exact admitted arrival/discovery and registry membership. Read registered foreign leaves, keep inactive legacy/computed-route scope bounded, and do not treat handler intent or map candidate as player arrival. |
| ZSQ-BRIMEFORGE-23 | Six room flag groups have universal BLOCKS_SIGHT, selected NO_MOB/NO_RECALL/NO_TELEPORT/HEAL; Kaynor also NO_SUMMON/NO_GATE. | Qualify current actor/effect/movement. Peace room131026 has HEAL but no SAFE; no raw NO_MAGIC/NO_PSI. Heat, night sign, darkness and captivity prose need actual hazard/endpoint adapters before awards. |
| ZSQ-BRIMEFORGE-24 | All16 raw mobile act fields lack ACT_TEACHER/ACT_SPEC_TEACHER; no literal local assignment/SHP/configured world.trg declaration appears. | Retain generic class/default/quester/caster/ordinary combat ownership and bounded static absence. Contact names or no local assignment do not prove no computed proc or service elsewhere. |
| ZSQ-BRIMEFORGE-25 | Five slave races, slavemasters and demons have extensive repeated placement; native return narrates cleansing only. | Define liberation/restoration/campaign state, actor/contributors and durable endpoints before awarding rescue or zone transformation. Locket acceptance cannot automatically free every worker or remove all slavemasters. |
| ZSQ-BRIMEFORGE-26 | Kaynor, child and Azurael have distinct cells/equipment; selected amulet/bifocals E chance25 differs from G stock. | Define prison rescue/release/alliance and exact optional equipment recovery with recipients/generation. Chance/caps, a defeated prisoner or empty description is not a completed rescue. |
| ZSQ-BRIMEFORGE-27 | Efreeti M2 says lockers instead of lockets; both Q response bodies are empty. | Fair prose fix can correct the item word and add accurate intended acceptance text, preserving terms/IDs/XP/caps. Repair remains separately named fix/news with before/after evidence; journal describes verified outputs now. |
| ZSQ-BRIMEFORGE-28 | Quest1/2/3 placeholder room names, malformed7N collapsed entrance, copied troll descriptions and Azureal/Azurael spelling differ. | Builder reviews intended names/prose/context and completes descriptions without changing balance or award semantics. Isolate native text repairs in named fix/news commits; placeholders alone do not define a quest. |
| ZSQ-BRIMEFORGE-29 | Active economic authority refuses reset O/G/E issuance before read_object for missing durable generation identity. | Implement accountable reset-generation issuance before claiming READY fresh lockets/keys/chest/equipment. Preserve fail-closed authority and restored/existing root ownership; mob/door resets alone do not prove issued quest stock. |
| ZSQ-BRIMEFORGE-30 | Mode2/lifespan40..50 schedules periodic renewal; neither native giver departs. | Qualify completed accountable item/giver generation, caps/current availability and replay/recovery before daily enablement. UTC rollover, elapsed timer or reset request is not proven renewable source or repeatable reward. |
| ZSQ-BRIMEFORGE-31 | Accounting admission freezes eligible local party and exact item/XP obligations; actor XP capped at next-level/10, other party at next-level. | Qualify actual two reward item roots/recipients/settlement/save/recovery and capped300000 XP entitlements separately from shared accepted history. Full inventory, disconnected recipient and stale giver-generation ambiguity need explicit evidence. |
| ZSQ-BRIMEFORGE-32 | Two native achievements/two potential daily candidates remain in unchanged catalog; journal uses schema3/revision1. | Require active READY accounting for new discovery/encounter/journal/achievement/daily tracking; frozen committed recovery separate. Keep daily policy disabled, preserve native definitions/order/prior maps, and own all unplayed qualification/fair repairs explicitly. |


## Bugger Caves builder case

See the [dossier](../design/zone-stories/BUGGER.md) and [journal](../../areas/story/bugger.story.json). Use the exact lost/misplaced egg and carapace set, optional current custody checks and a wrong-food exclusion. SEARCH, container GET, personal source lineage, global reset P parent selection and accepted batch differ. Wider brood/transport/colony stories require explicit lasting endpoints.

| Follow-up | Source finding | Required capability or fair builder decision |
| --- | --- | --- |
| ZSQ-BUGGER-01 | QA7 takes exact eggs6403/4 and carapace6405 for armor6406; QA14 rejects food6402 and returns6402. | One exact joint-set request plus an explicit refusal exclusion. Preserve both native definitions; the refusal must not increase story, achievement or daily progress. |
| ZSQ-BUGGER-02 | MA2 hi/hello/agitated is one addressed broadcast reply. | Keep three suggested topics in one family. Credit the acknowledged addressed actor’s encounter; room witnesses and repeated keywords are not three achievements. |
| ZSQ-BUGGER-03 | Egg6402/3/4 keywords and shortnames are identical; descriptions and exact identities differ. | Explain lost versus misplaced versus tasty food egg. A fair naming/prose repair requires a separate fix/news commit and must preserve IDs, visibility and native terms. |
| ZSQ-BUGGER-04 | Requested eggs6403/4 have SECRET, NOSELL and NORENT;6403 also NOLOCATE/NOIDENTIFY; neither is BURIED. | Use admitted room SEARCH followed by actual GET/current custody. Do not require DIG or promise locate/identify bypasses. A failed reveal, room message or observation is not first-source acquisition. |
| ZSQ-BUGGER-05 | SEARCH clears SECRET under chance/visibility admission and can invoke CMD_FOUND before an eventual GET. | Add actor/object UID/generation/reveal outcome receipts only for defined objectives. Separate reveal, ownership publication and first-source versus player-transfer lineage. |
| ZSQ-BUGGER-06 | O17 dead bugger6400@6413 is an open nonTAKE type15 container; P18 carapace6405 is TAKE. | Bind actual container-child recovery. Do not require carrying the body, CARVE, a fresh NPC death or a guessed generated corpse; none is declared for this source. |
| ZSQ-BUGGER-07 | Inactive reset P uses get_obj_num, the first global matching parent prototype, under conditional last-command admission. | Accountable reset generation must freeze the actual parent UID/child UID/location. The last O/E command and a name are not sufficient parent identity; qualify existing/restored parents and caps. |
| ZSQ-BUGGER-08 | Attendant6409 M30@6402 wears pouch6401 E31/WAIST13; P32/P33 add food6402 with cap2. | Distinguish four similarly named attendant prototypes and exact equipment parent. Actual worn recovery, corpse/container movement and duplicate food caps require qualification. |
| ZSQ-BUGGER-09 | Misplaced egg O19@6416/O21@6423 both cap2; lost egg O20@6419 cap1; carapace cap1. | Qualify global caps and actual renewable stock rather than promising a fresh copy at every source. Reset declarations are not proven current READY availability. |
| ZSQ-BUGGER-10 | Three optional current loose item rows feed a single accepted return. | Keep Ready now/Missing now ephemeral and exact. Held, worn, nested, duplicate and wrong eggs cannot substitute; Recorded survives spending without restoring item custody. |
| ZSQ-BUGGER-11 | Active accounting selects all distinct actor-owned loose roots together; legacy completion can stage giver holdings. | Keep incomplete sets with the actor and describe simultaneous batch acceptance. Earlier deposits/contributors need explicit policy; legacy staging is not the accounting flow. |
| ZSQ-BUGGER-12 | Native acceptance has no remembered greeting, personal finding, combat, entrance or colony-care prerequisite. | Allow supplied exact items. First-source achievements and player transfers need separate policy, not invented prerequisites for existing native acceptance. |
| ZSQ-BUGGER-13 | Party completion context is frozen separately from actual armor reward UID/recipient/save. | Qualify successful obligation publication, capacity fallback, recipient disconnect/save and cold recovery. Broadcast thank-you text and shared completion do not prove every party member received armor. |
| ZSQ-BUGGER-14 | Wrong-food native self-exchange consumes an offering and issues a same-prototype reward. | Preserve the excluded settlement receipt and replay recovery. Do not call it the same UID returned, source-earned acquisition or a meaningful reward; classify before daily/achievement selection. |
| ZSQ-BUGGER-15 | Mapping exclusion removes one story unit/achievement while native QA14 remains unchanged. | Test mapped progress versus raw compatibility and exclude refusal after replay/cold recovery. Counts intentionally become205 journals/1521 achievements/1408 candidates/2183 units. |
| ZSQ-BUGGER-16 | Queen persists; neither QA declares D, XP, coins or another return order. | Use exact native bindings and actual giver generation. Empty fee/XP assumptions or reward text must not invent obligations or unlock an unrelated story. |
| ZSQ-BUGGER-17 | Requested eggs are NORENT; carapace and armor do not declare NORENT. | Qualify physical logout/rent/save/root recovery separately from durable journal history. A lost physical egg does not erase acceptance or recreate consumed material. |
| ZSQ-BUGGER-18 | Armor6406 has WHOLE_BODY/RETURNING/NOREPAIR and APPLY_HIT+5/APPLY_DAMROLL+1. | Own admitted wearing/coverage/effect/state/save and balance intent. Receiving armor or reading flags is not a proven wear benefit, crafting result or combat objective. |
| ZSQ-BUGGER-19 | All rooms are DARK; twenty have INDOORS/NO_PRECIP,6417 additionally TUNNEL. | Qualify actual sight/light and airborne ordinary entry. Trusted bypass/landing differs; naming a dark or narrow room is not a player achievement or presumed magic block. |
| ZSQ-BUGGER-20 | 6420/21/22 use UNDERWORLD_SLIME sector28 without raw UNDERWATER; pheromone/heat/sticky prose adds no quest adapter. | Define any hazard, brood-care or transformation endpoint with admitted actor outcome/state. Do not infer underwater breathing, lava damage or maturation from descriptive prose. |
| ZSQ-BUGGER-21 | 6425N/6426S raw5/D5 yields ordinary CLOSED+SECRET doors, not a locked key puzzle. | Track personal near-side reveal, actual reciprocal opening and successful crossing separately from shared passage state. Preserve native doors and alternate routes. |
| ZSQ-BUGGER-22 | Two registered reciprocal entrances:6400N↔underdark807099S and6426UP↔ixxillikor4380DOWN. | Use actual successful placement and registry ownership for discovery. Mechanical map edges, intended travel and foreign witnesses do not prove a local arrival. |
| ZSQ-BUGGER-23 | do_shift_prime includes room6405 in the githyanki/pillithid/illithid target pool. | Qualify astral membership, race, runtime innate timer/use admission, random valid target and successful placement. Numeric6405 is a room target, not a carapace producer or mandatory route. |
| ZSQ-BUGGER-24 | All22 mobiles are stocked and lack raw teacher bits; local literal proc assignments/SHP/world.trg are absent. | Keep race/class/default combat and computed configuration separately bounded. Similar names and contact roles do not prove a native quest, teacher or custom outcome. |
| ZSQ-BUGGER-25 | Brood, larvae/pupa/young adults, attendants, servants, drones and egg depository form colony-care prose. | Builder defines nursing, transport, maturation or preservation journeys and lasting endpoints before awards. Bind exact actor/contribution/state changes rather than NPC exposure. |
| ZSQ-BUGGER-26 | Fungi, storage caretaker, slime matron, six-legged insects and workers provide side stories. | Define threat resolution, excavation, food transport and colony restoration with source/recipient/outcome receipts. Native item acceptance is not automatic colony rescue. |
| ZSQ-BUGGER-27 | Queen chamber size,6415/6423 direction prose, six=legged keyword and spelling need review. | Make fair text/clarity repairs without changing IDs, caps, aggression, hidden stock or rewards. Every actual native repair needs a separately named fix/news commit and prominent before/after PR/news details. |
| ZSQ-BUGGER-28 | Mode2/lifespan40..50 schedules periodic resets; active authority refuses O/P/E before item instantiation. | Implement accountable item/giver generation, actual source replenishment and replay/recovery before daily enablement. Require active READY accounting for new tracking; frozen recovery separate and daily policy disabled. |


## Dirk’nspire builder case

See the [dossier](../design/zone-stories/DIRKN.md) and [journal](../../areas/story/dirkn.story.json). Preserve independent document/paper requests, meaningful coin rewards and current preparation versus recorded acceptance. Qualify actual container/parent UID, legal key0 lock attempts and source versus transfer lineage. Separate default services, weapon effects, cross-zone bounty custody and wider builder-defined rescue/infiltration/clan endpoints.

| Follow-up | Source finding | Required capability or fair builder decision |
| --- | --- | --- |
| ZSQ-DIRKN-01 | Q10 document96845→scarab96843;Q19 paper96802→C2570 are independent meaningful contracts without D. | Keep two cards and exact receipt bindings. Disappointed prose still pays coins; no return order, greeting, source kill or personal search is required. |
| ZSQ-DIRKN-02 | M2 maps/secrets/map/secret is one addressed response family. | One acknowledged actor encounter; do not turn alternate keywords into four achievements or credit room witnesses. |
| ZSQ-DIRKN-03 | Paper96802 SECRET/TAKE+HOLD/QUESTITEM, no NORENT, is P187 in guano96800 O185@96802. | Personal named-container SEARCH → actual GET/root custody → accepted return are distinct. Supplied exact items remain legal. |
| ZSQ-DIRKN-04 | Named SEARCH checks open visible permitted parent/chance, clears SECRET and may invoke CMD_FOUND. | Bind actor, actual child/parent UID and reveal outcome. Shared revealed state, failed visibility or a message is not first-source acquisition. |
| ZSQ-DIRKN-05 | Document96845 P216cap1 parentmoneybox96815 O215@96868; boxes alsoO206@96851/O219@96884. | Qualify actual global first-matching parent UID, conditional reset admission, shared cap3, restored parents and current stock; preceding O is not a frozen source identity. |
| ZSQ-DIRKN-06 | Moneyboxes/desks flags13,key0 are closeable/closed/locked without PICKPROOF. | Admitted PICK/KNOCK attempts can clear the lock under current skill/pick/state/chance. Do not diagnose an impossible missing-key quest or promise success; opening and GET remain separate. |
| ZSQ-DIRKN-07 | Golem96804 M283@96848 carries key96810 G284; main96846E↔96848W raw3/D2 is pickproof locked. | Actual key custody, visible door unlock, reciprocal current state, opening and successful crossing need separate outcomes. Alternate entry is allowed by native acceptance. |
| ZSQ-DIRKN-08 | 96808W raw4→96814 has no D reset; setup masks raw state to lowbits. | Selected loader makes this an open non-door passage. Define intended boulder interaction before any fair access repair; do not invent a mandatory PUSH puzzle. |
| ZSQ-DIRKN-09 | 96821E raw5/D5 secret closed door; reverse96820W raw1/D1 ordinary closed. | Record personal near-side reveal, admitted reciprocal opening and actual crossing separately from shared door state. |
| ZSQ-DIRKN-10 | 96824 F5 staircase has DOWN→96825 and native 5percent eligible falling admission. | Qualify grounded checks, scheduled movement, placement rejection/restoration, landing and impact. FALL prose or an attempted descent does not complete a story. |
| ZSQ-DIRKN-11 | All86 rooms DARK/NO_RECALL/NO_TELEPORT/NO_SUMMON; threeNO_MOB/sixTUNNEL; mixed indoors. | Qualify current visibility/light and ordinary versus bypass arrival/movement. Raw flags do not prove every alternate destination placement is denied. |
| ZSQ-DIRKN-12 | 96850 reception/96885 store NO_MAGIC;96834/96868 HEAL;96861/96878 NO_HEAL. | Preserve native spell/healing admission. Service prose is not SAFE protection or successful temple/quest resolution. |
| ZSQ-DIRKN-13 | Valinhav90810↔96800 is the sole numeric foreign world edge; room96803 is in three native random arrival pools. | Successful registered placement owns discovery. Qualify shift_prime race/astral/timer, mind_travel origin/combat and veil callback/random placement/restore; none is guaranteed or a required item source. |
| ZSQ-DIRKN-14 | Veil96402 O@ixarkon96524 is explicitly assigned illithid_teleport_veil. | Qualify exact ENTER dispatch, current object/actor, placement and do_restore effects. A selected target or arrival message is not proof of admitted arrival/save or quest success. |
| ZSQ-DIRKN-15 | Wemic96820 ACT_TEACHER/class512 gets default teacher; M@96827 wears blindfold96840. | Current teacher visibility/class/level/skill/circle/fee and successful learned/save outcome matter. Blind/homeless prose, ASK and payment are not automatic assistance or learning awards. |
| ZSQ-DIRKN-16 | Current mobile class512 is sorcerer,256 shaman,2048 conjurer; role prose can differ. | Use exact runtime ability/admission and flag defaults; own any intended class/prose repair separately without silently changing balance. |
| ZSQ-DIRKN-17 | Flyrr96838 store96885 sells flask96808/cereal96844; native stock/hours/visibility/race/pricing admission. | Bind actual purchase settlement and issued source root. Meeting, displayed products, carried food and shop reset declarations are not quest completion or fresh availability. |
| ZSQ-DIRKN-18 | Gartuk/brothel/temple/clan priests have role prose but no literal local custom assignments. | Define successful service, initiation, prayer or worship endpoints and check generic defaults/computed configuration before an absence claim. |
| ZSQ-DIRKN-19 | Water96804 and urine96807 have nonzero poison; actual liquids16/14 differ from readable names. | Explain or separately repair misleading safety prose fairly after builder intent review. Bind admitted drinking/poison/effect/save; do not auto-award a poison-food mission. |
| ZSQ-DIRKN-20 | Holybasin96821 liquid17,well96824 slime9,blood96839 liquid13,ivorypool96842 liquid16; flask liquid5. | Qualify native condition/heal/blast/poison/consumption and alignment admission. Pool names and spell messages do not resolve a temple or clan story. |
| ZSQ-DIRKN-21 | Type27 altar96822 values0/197/-1; signs/boulder alsotype27, no established local action proc. | Define intended altar/boulder outcome and actual adapter; spell-looking values alone do not prove casting. Avoid speculative balance/access repairs. |
| ZSQ-DIRKN-22 | Firb96816 M311@96868 wields FoeHammer96835 E314/WIELD16. | Recover exact current equipped/corpse/root identity. Personal kill, supplied weapon and custody lineage differ; acquiring or wielding the hammer is not a Balith return. |
| ZSQ-DIRKN-23 | Hammer packed27188=188SOULSHIELD/27HARM/0, effectlevel40,inversefrequency35; three message extras. | Bind runtime enabled/configured action, windup/actor/target/UID/mana and actual admitted spell effect. These are not charges; messages/invocation are not proven damage, shield, kill or completion. |
| ZSQ-DIRKN-24 | Ragmor83336/Wikzor83439 each request96835 with78419/95526/34226/4505 and retire on success. | Explain keep/use/spend choice. Preserve two distinct Alatorin giver receipts and rewards; no forced Balith chain or _noquest_ keyword override of explicit native G I. |
| ZSQ-DIRKN-25 | ForeignP1655 loosechange96820cap1 uses rubble83330 O1654@Alatorin83544. | Shared local/global stock caps need actual generation and parent custody. Foreign coin stock is not a second Balith input or local completion. |
| ZSQ-DIRKN-26 | Balith M236@96813 wears scarab96843 E237slot24; Q10 also newly issues96843. | Separate worn/theft/death recovery from issued reward birth/recipient/save. Possessing the reward does not reconstruct accepted document history. |
| ZSQ-DIRKN-27 | Q19 native C2570 uses copper-value units:2platinum/5gold/7silver. | Freeze exact wallet_value obligation, recipient/save acknowledgement and replay recovery. Readable denomination is not proof of native settlement. |
| ZSQ-DIRKN-28 | Two optional exact loose current item checks feed two independent Recorded rows. | Held/worn/nested/reward items cannot substitute. Render checks must not mutate history; spent materials and receipt replay/cold recovery preserve independent acceptance. |
| ZSQ-DIRKN-29 | Damaged paper describes infiltration, food poisoning and a threatened daughter; no corresponding outcome adapter is established. | Builder defines source actor/target/food identity, prerequisites, admitted poison/mission endpoint and rescue state before awards. Do not infer a local NPC identity or mandatory native history from torn prose. |
| ZSQ-DIRKN-30 | Slave pens96873/74, bathing slave96875, freedmen, family hovel and burnt dwelling support captivity/restoration stories. | Define release, safe destination, recipient survival/state and actor contribution. Encounter, door opening, killing a guard or seeing a family is not a lasting rescue. |
| ZSQ-DIRKN-31 | Swifthammer and Broken Will leadership/priests/kitchens/barracks plus town/gallows form wider rival-clan stories. | Builder defines allegiance, investigation, justice, worship and conflict endpoints. Existing Balith returns do not automatically resolve these narratives. |
| ZSQ-DIRKN-32 | Fair leads include torn-paper clarity, liquid labels, class/role prose, boulder route and spelling. | Keep existing IDs, flags, gates, caps, rewards and balance. Every actual native repair needs a separately named fix/news commit and prominent before/after PR/news details. |
| ZSQ-DIRKN-33 | Mode2/lifespan10..20; active authority refuses O/P/G/E before instantiation without fresh generation identity. | Qualify accountable giver/parent/child generation, cap-safe actual replenishment and replay/recovery before daily enablement. Timer expiry and UTC rollover do not produce stock. |
| ZSQ-DIRKN-34 | Full local/registered foreign/bounded shared closure and numeric888reset/world/442object scans were reviewed. | Computed DB/scripts/crafting/admin/supplied routes remain outside static absence claims. Require active READY accounting for new tracking; frozen committed recovery separate and daily policy disabled. |


## Storm Port builder case

See the [dossier](../design/zone-stories/STORMPORT.md) and [journal](../../areas/story/stormport.story.json). Use one completion for each exact native bundle. Keep foreign physical contact discovery separate from the quest home; treat source acquisition, supplied custody, hidden reveal and accepted history separately. Keep unavailable accounting services and unresolved ship access truthful.

| Follow-up | Source finding | Required capability or fair builder decision |
| --- | --- | --- |
| ZSQ-STORMPORT-01 | Q12 Tchan four exact proofs → mantle22435; Q21 citysmith granite/straps → shield22407; neither D. | Keep two independent receipt-bound cards; no invented personal kill, carve, greeting, source route or turn-in order. |
| ZSQ-STORMPORT-02 | Tchan Q has empty reply but a real R I reward. | Preserve meaningful completion. A separately named clarity fix may add an acceptance reply after builder intent review without changing rewards. |
| ZSQ-STORMPORT-03 | M2 hi/hello is one addressed family; M30 qc_action30 is timed room prose. | Alias greetings are one lead. Action metadata and witnesses receive no ASK achievement or completed passage. |
| ZSQ-STORMPORT-04 | Tchan22428 M19 in foreign tchan.zon@4809; no local M; old22520 prose claims his residence. | Explain physical monastery48 versus quest home224. Qualify visible actual giver and local discovery independently; own a fair location/prose correction rather than duplicating stock. |
| ZSQ-STORMPORT-05 | Tchan explicit clear_epic_task_spec overrides teacher default; ACT_TEACHER still contributes native teacher lookup. | Current speech/visibility/task/class/skill admission matters. Encounters and advice are separate from admitted learning or task relief. |
| ZSQ-STORMPORT-06 | ASK prayer refuses active accounting before fee, task-affect removal or money mutation. | Keep unavailable status. Add durable fee/task-clear/recipient/save/replay coupling before integrating this custom service; do not require it for the mantle. |
| ZSQ-STORMPORT-07 | Shackles22431 G371 from minotaur22466@22460; all four proof roots cap1. | Bind actual source generation/root UID and recipient custody. Corpses, theft, transfers and supplied materials have distinct lineage; the Q does not require a personal kill. |
| ZSQ-STORMPORT-08 | Tooth22432 G426 from verbeeg22461@22512 is SECRET+NORENT. | Separate hidden basement passage, near-side reveal/open, object reveal and actual GET. A shared reveal or readable tooth name is not personal first-source acquisition. |
| ZSQ-STORMPORT-09 | Stone22434 G367 from golem22463@22458 is SECRET+NORENT. | Separate rocks passage22527W, current object reveal, recovery and receipt. Define any cliff challenge outcome before awarding one. |
| ZSQ-STORMPORT-10 | Badge22433 G420 from captain22430@22509 in old frigate. | Current ordinary/source access is unresolved. Builder must choose intended balanced ship access or source relocation and validate it; do not silently add an entrance, stock or free transit. |
| ZSQ-STORMPORT-11 | Old frigate22508..22511/22538..22540 has no raw ordinary inbound edge or raw type25 first-value route. | Bounded source scan is not an arbitrary computed-route absence proof. Qualify actual loaded ship, actor admission and access before promising badge availability or daily renewal. |
| ZSQ-STORMPORT-12 | Old22423 boat/22424 ladder/22440 schedule/22441 navigator/22442 ticket have no raw reset producer. | Review obsolete presentation separately from actual modern callbacks/configuration. Ladder exits UP to22439 rather than entering the old ship. |
| ZSQ-STORMPORT-13 | Granite22406 G394 from deathknight22433@22476. | Exact loose custody prepares smith only. Define personal combat/recovery outcomes separately; no inferred tombstone carve or Tchan order. |
| ZSQ-STORMPORT-14 | Desk22404 O268@22503; straps22405 P269cap1, SECRET; flags5 CLOSED+CLOSEABLE,key0. | Actual parent UID/cap, current open state, named SEARCH and GET matter. Drawer prose about a lock needs a fair clarity review; do not add a PICK prerequisite. |
| ZSQ-STORMPORT-15 | Four mantle inputs NORENT; smith inputs and rewards do not carry that flag. | Explain physical logout loss versus durable acceptance. Qualify cold/copyover/spent input/reward save and receipt replay without using possession as history. |
| ZSQ-STORMPORT-16 | Local secret closed ordinary doors:22422DOWN,22455DOWN,22527WEST,22511DOWN and their reverses. | Actual D reset5 supplies secret state; personal reveal, reciprocal opening and successful crossing differ. No local item-key gate is declared. |
| ZSQ-STORMPORT-17 | Local WLD147 rooms/110 exact prose families/24 controls/320 edges lacks E/F hazards. | Preserve DARK/TWILIGHT/INDOORS/UNDERWATER/DOCKABLE/INN/NO_TELEPORT/NO_GATE and sector admission. Cliff, flooded ground and crumbling prose are not a scripted success or safe route. |
| ZSQ-STORMPORT-18 | Registered surface coast/road leaves; Sarmiz9597 has no reverse; Savenge22541 load routes and22542 empty. | Qualify current physical arrival. Administrative/unregistered Duris3/surf routes do not establish ordinary player progression; own empty/load-room intent separately. |
| ZSQ-STORMPORT-19 | Type25 waterwheel22401 O@22447 DOWN7→22530 with infinite-1 charges; target has ordinary exit. | Bind dispatched current object, actor/destination admission and actual placement. Entering the interior does not restore or operate the wheel. |
| ZSQ-STORMPORT-20 | Modern Seaspray id2/object47014/47026–25:22444↔550724; Stromvok id7/object47018/47198..47215:22445↔66688↔30929. | Freeze boat UID, route/leg/stop, actor and current availability. Old local ship names/interiors/ticket do not identify these services. |
| ZSQ-STORMPORT-21 | Ferry init creates automat47006; valid ticket47003 loose with value0 matching ferry ID; native price10000=10platinum. | Actual fee settlement, item birth/UID/recipient/root/save acknowledgement and correct route differ from LIST, possession or held/nested/wrong-ID tickets. |
| ZSQ-STORMPORT-22 | ferry_automat_proc BUY ticket refuses active accounting before debit/read_object. | Keep blocked service; implement atomic durable fee/issuance/refund/replay before offering new passage under required READY accounting. |
| ZSQ-STORMPORT-23 | Boarding ENTER and standing/noncombat DISEMBARK use actual room callbacks; movement relocates boat, not passengers. | Define boarded PID/leg, ticket admission, actual destination placement and save. Arrival announcement, lookout and stowaway ejection are not successful travel. |
| ZSQ-STORMPORT-24 | Passenger list bypasses later checks until stop; NPC/trusted bypass; panic relocates players to first stop. | Qualify cold/copyover/passenger lifecycle and privileged/ejected/emergency paths. Never manufacture paid-journey or exploration credit from a message. |
| ZSQ-STORMPORT-25 | Ferry eta returns seconds/60, while automat labels result as hours. | Own a separate unit-label correctness fix with before/after news; preserve timetable/speed/price and verify displayed units. |
| ZSQ-STORMPORT-26 | Ship shop22441 owns LIST/BUY/SUMMON/SELL/REPAIR/RELOAD; ticket BUY falls through; crew hall22481 restricts racewar/ownership/selection. | Offers, owner/location/docked/maintenance/skill/frag/fee admission and settled hull/equipment/repair/crew state need separate durable outcome adapters. |
| ZSQ-STORMPORT-27 | Ship hull callback rechecks ship/wallet, creates or changes hull and settles coin/epic obligations; crew hire changes crew and queues ship save. | Couple actual fee/resource/ship recipient acknowledgement and recoverable save before awards; queue submission or success prose alone is insufficient. |
| ZSQ-STORMPORT-28 | Citysmith SKILL_FIX teaching table and default ACT_TEACHER guild leaders; native learning admission. | Track admitted fee/learned change/save separately from teacher encounter, ASK, displayed instruction and citysmith’s Q shield reward. |
| ZSQ-STORMPORT-29 | do_fix1182..1294 checks loose target, skill, condition/material then RNG; final loop extracts every matching material. | Own a focused balanced repair to consume exactly one promised material and verify success/failure/durable target save. Review failure item=NULL versus destruction separately; no repair made here. |
| ZSQ-STORMPORT-30 | Room22439 dock explicitly inn;22486 has ROOM_INN flag and inn prose; inn uses terminal save/home rollback then extraction. | Qualify effective flag-based/default room dispatch and actor/combat/PVP/raid admission. Own fair location clarity review without changing rent safety or material persistence. |
| ZSQ-STORMPORT-31 | Five SHP blocks, poison/liquid containers, potions/totems/lights/equipment and native rewards. | Current visibility/stock/hours/race/prices, birth/root and admitted use/effect/recipient/save differ from shopping or magical prose. Wearing a reward is not its accepted receipt. |
| ZSQ-STORMPORT-32 | Slaves/prisoners, gallows, Bloodstone priest, guilds/dojo, families, graveyard and city defenses have broader narrative leads. | Builder defines release, safe arrival, survival, allegiance, investigation, justice, learning or restoration endpoints and actor contribution before achievements. |
| ZSQ-STORMPORT-33 | Mode2/lifespan40..50; source/parent/cap and accounting O/P/G/E admission govern replenishment. | Qualify fresh accountable giver/item generations and actual badge access before any daily enablement. UTC reset and a zone timer do not create available stock. |
| ZSQ-STORMPORT-34 | Full local/selected foreign/bounded shared closure;888 raw reset/world and442 object files hash-pinned. | Computed DB/admin/crafting/supplied sources remain outside static absence claims. Require active READY accounting for new tracking; frozen committed recovery separate; daily policy disabled. |
