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
