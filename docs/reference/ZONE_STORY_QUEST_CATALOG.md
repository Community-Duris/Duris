# Production zone-story quest catalog

The production catalog is generated from the active static quest sources listed
in `areas/AREA`, using the same source boundary as `areas/src/qst/make_qst.c`.
The checked-in snapshot is [ZONE_STORY_QUEST_PRODUCTION_CATALOG.json](ZONE_STORY_QUEST_PRODUCTION_CATALOG.json)
and currently contains 2,668 eligible repeatable quest definitions across the
active zone/story quest set.

Each legacy `Q` completion block receives a stable definition identity derived
from its giver VNUM and canonical give/receive/disappear contract. Prose changes
therefore do not silently reset a character's distinct completion credit, while
a goal or reward identity change is represented by a new content revision.
Multiple blocks with the same giver and canonical contract are alternative
prose/turn-in routes for one accomplishment and are deduplicated, so reordering
them cannot inflate the denominator. The initial catalog revision is `1`; bump
it deliberately when a compatible content edit is shipped and keep the previous
revision available to the migration/recovery tools until its retention window
is complete.

The runtime catalog is built from the quest index immediately after
`boot_the_quests()` and uses the same identity and zone-number rules. A catalog
definition is eligible only when it is active, repeatable, has a stable
completion key, and matches the booted catalog revision. The denominator for a
zone is the current eligible catalog, never the number of completion rows.

Bartender/random world quests are intentionally excluded. They are generated
assigned-run content with different reset and reward semantics; they are not
static zone/story definitions and must not inflate a zone's completion
percentage.

Regenerate and validate the snapshot with:

```text
python3 scripts/zone_story_quest_catalog.py \
  --source-root . \
  --content-revision 1 \
  --production-output docs/reference/ZONE_STORY_QUEST_PRODUCTION_CATALOG.json \
  --check
```

Boot logs report the number of definitions and the revision. A boot with an
empty or invalid catalog fails closed for completion credit and exposes `N/A` in
the personal achievement view; it does not invent a denominator.
