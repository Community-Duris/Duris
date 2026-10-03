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
the counting semantics. See the [Alatorin draft](../design/zone-stories/ALATORIN.md)
for the two/six/eight-matching-material examples and planned qualification.

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

The [Alatorin draft](../design/zone-stories/ALATORIN.md) gives concrete examples:
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
