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
