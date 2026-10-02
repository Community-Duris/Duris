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

Every shown field is required. Unknown/duplicate fields are rejected. Files
are bounded to 256 KiB. Plain-text strings are at most 1,024 UTF-8 bytes,
without controls or `$` substitution tokens. IDs use lowercase letters,
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

## The gardener dependency

Alvinar's Q contract does not describe the barrier. `gardener_block` in
`src/specs/specs.twintowers.c` accepts item 13521 only in `equipment[WEAR_WAIST]`
for guarded movement. The reset loads gardener 13504 and equips the belt.
There is no native Q contract that awards it.

The reset exposes a source link; the special supplies the access predicate;
the sidecar relates them to Alvinar's story. A mapped condition can update from
live equipment dynamically. Narrative relationships implemented in arbitrary
C++ specials cannot reliably be discovered by parsing commands or dialogue.

## Later schema capabilities

Version 1 rejects dialogue, personal recovery, kills, arbitrary scripted events,
coin objectives, and new reward fields. Do not deploy placeholders claiming those
events are tracked. The integration plan specifies the durable adapters and
credit policies needed before those kinds can enter a versioned schema.
