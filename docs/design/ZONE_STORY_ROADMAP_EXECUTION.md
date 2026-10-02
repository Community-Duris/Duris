# Zone story roadmap execution register

The full [priority roadmap](ZONE_STORY_ZONE_PRIORITIES.md) remains the work queue.
The [expanded plan](ZONE_STORY_INTEGRATION_PLAN.md) remains the shared capability
contract. This register records concrete progress and findings without treating
static binding coverage as comprehensive script or gameplay qualification.

## Completion criteria

For each touched zone, review the complete active Q/M source, resets, rooms,
prototypes, special assignments and implementations, and relevant shared execution.
Map every named story, intermediate exchange, alternative, service, rejection,
lore closure, scripted gate, and quest-like orphan. Record exact prerequisites,
counts/fees/rewards, source alternatives, personal versus supplied materials,
recipient/branch/attempt policy, and cross-zone ownership. Add all guidance that
the shipped schema can safely express. Unsupported objectives need a precise
event/transaction proposal and qualification matrix, not an invented completion.

Source-comprehensive means no inspected quest interaction is left unexplained.
Gameplay-qualified requires actual committed journeys and recovery evidence.
These labels are independent. A single successful terminal exchange proves
neither every branch nor every historical prerequisite.

## Progress

| Priority | Zone | Source story map | Deployable journal | Remaining qualification |
| ---: | --- | --- | --- | --- |
| 1 | Twin Towers Forest | [Comprehensive source dossier](zone-stories/TWIN_TOWERS_FOREST.md): 84 Q contracts, 58 M blocks, three special implementations, 30 assignments, 345 reset commands | Revision 3: ten stories/requests, twelve support services, forty rejections; optional belt preparation and complete addressable topic guidance | Active reset sources; flower/access journey; learned topics; animal birth/decay/lineage; atomic mixed fees; full alternative journeys; shared client projection |
| 2 | Plains of Life | [Comprehensive source dossier](zone-stories/PLAINS_OF_LIFE.md): Q-free tutorial, all four specials, 21 reset commands, creation tag and travel semantics | Revision 2: full route, optional sign aids, both racewar aliases, encounter guidance; no invented terminal receipts | Active reset scenery; transactional tag/sword; validated travel; durable scripted objectives; played failure/restart cases |
| 3–220 | Remaining roadmap | Pending comprehensive review; earlier rough proposals and complete Q classification remain useful evidence | Existing authored maps/native fallback retained | Work through original queue; record each reviewed family and custom dependency |

The next area is Ailvio, followed by Braddistock. Their earlier native bindings
do not establish comprehensive source review. Do not advance a zone's status
merely because the map parses, the Q denominator matches, or a candidate item
graph was extracted.

## Findings that expand or reorder the implementation plan

| Finding | Status / scope | Concrete next action |
| --- | --- | --- |
| ZSQ-RESET-AUTHORITY | Confirmed source blocker for fresh active-accounting item reset generation, including scenery and nested supplies. Existing recovered items may still exist; this is not proof every deployed area is empty. | Qualify committed reset generation before the active-world pilots. Stable reset occurrence/command keys, exact custody, parent dependencies, retry/replay, NPC equipment, non-takeable portals/signs, and cross-area reset ownership are required. |
| ZSQ-TUTORIAL-GRANT | Confirmed ordering weakness: the tag is removed before sword creation/publication is confirmed. | Implement committed script grants with recoverable tag outcome; avoid success prose before confirmed grant. Keep failure distinct from repeated completed lesson. |
| ZSQ-TUTORIAL-TRAVEL | Confirmed failure-path weakness: refresh/success text precede destination validation; room index zero is excluded. | Validate destination and mutation admission first, then publish confirmed arrival. Test valid index zero and missing room, alongside normal Ailvio travel. |
| ZSQ-ANIMAL-LIFECYCLE | Direct custom death creation and decay replacement lack committed source/transform evidence. | Add actor-aware birth, freshness/deadline, placement-aware transformation, and exact input/output retirement. Reject personal credit for unknown origins; preserve gifts for delivery. |
| ZSQ-OPTIONAL-PREPARATION | Implemented first portion: schema 3 optional steps cannot take `Next:` priority. Native goals and receipt IDs are preserved. | Qualify conditional subrecipes, shared reveal rules, and all-stage campaigns separately. An optional check is not a new historical event or branch model. |
| ZSQ-TWIN-TERMS | Confirmed prose/contract differences; some giver price differences are legitimate alternatives. | Review the complete term table before deciding whether to fix prose or execution; do not normalize prices without a world-content decision. |
| ZSQ-TUTORIAL-NOTE | Isolated Grandma invitation prototype; no literal active source route found. | Confirm retirement or restore a complete supported route. Keep it out of player promises meanwhile. |
| ZSQ-SCRIPT-OBJECTIVES / ZSQ-LEARNED-LORE | Current completion steps require Q receipts; printed commands or cleared tags are not durable story evidence. | Define committed accepted-topic, scripted grant, and successful travel events with explicit terminal/one-time policies. |
| Evidence extractor gaps | Fixed chained literal assignment detection and ignored commented assignments. Object/mobile membership follows source prototypes rather than room bounds. | Continue review for aliases, computed VNUMs, preprocessor branches, dynamic assignments, and foreign reset sources; extraction remains a lead list, not execution proof. |

## Verification record

Record exact test/build results for each implementation batch here. Source audit
exports are read-only and can run without accounting or database activation:

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence twin_towers_forest
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence newbie2
```

The inventory regression checks real chained mob/object/room assignments,
commented/cleared functions, exact Q/M counts, reset counts, and prototype
membership beyond room bounds. The story regression checks native parsing of
all maps, optional versus required `Next:` behavior with a supplied plant,
equipment slot accuracy, service exclusion from achievements/dailies, and old
schema compatibility. These checks do not activate accounting or prove the
unimplemented reset/grant/lineage paths.

### First comprehensive mapping batch — October 2, 2026

Passed both maintained C++20 builds:

```bash
make -C src -j6 CC=g++-12 BIN_ROOT=../bin
make -C src -j6 CC=g++-12 BIN_ROOT=../bin PERSISTENCE_BACKEND=flatfile \
  DMS_BINARY=../bin/server/dms_zone_story_builder_flatfile
```

The host's existing hiredis TLS library directory was supplied with
`LIBRARY_PATH`; no dependency was installed. Focused regressions passed:

- `python3 tests/async/test_zone_story_quest_story.py`: all 36 maps parsed in
  native C++; optional preparation and supplied-plant next action; exact waist
  slot; schemas 1/2 compatibility; invalid optional fields; distinct feathers;
  achievement/daily projections; receipt preservation and read-only/restart checks.
- `python3 tests/async/test_zone_story_quest_production_catalog.py`: full catalog
  snapshot, generated world inventory and both source indices, exact assignment
  chains, omitted comments/quoted examples/cleared functions, reset counts,
  and prototype membership checks.
- `python3 tests/async/test_zone_story_quest_feature.py`.
- `python3 tests/async/test_zone_story_quest_arrival.py`.
- `python3 tests/async/test_zone_story_quest_production.py`.
- `python3 scripts/zone_story_quest_home_coverage.py --check`: all 27 required areas.
- `./scripts/format.sh --check`: changed lines and complete touched files.

Document links resolve. All 2,668 native definitions, zone registry, source
fingerprint, content revision, and the other 34 maps are unchanged as parsed
objects. Twin Towers still contributes ten achievements and three daily groups;
the twelve newly displayed services increase total projected rows without
changing achievement/daily eligibility. No production operation or accounting
activation was performed. Fresh active-world gameplay remains open because the
reset, script grant, and transformation adapters above are not implemented.
