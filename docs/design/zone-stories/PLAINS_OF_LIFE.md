# Plains of Life: comprehensive source story map

Reviewed October 2, 2026. Source area `newbie2`, zone 228,
journal revision 2. This is a source-comprehensive scripted tutorial map;
there are no native Q/M turn-ins or journal terminal receipts in this area.

## Reviewed evidence

Reviewed complete [rooms](../../../areas/wld/newbie2.wld),
[resets](../../../areas/zon/newbie2.zon),
[objects](../../../areas/obj/newbie2.obj),
[mobiles](../../../areas/mob/newbie2.mob), and
[four custom procedures](../../../src/specs/specs.newbie2.c).
The area has seven trail rooms, 21 reset commands, and four literal special
assignments: paladin 22801, signs 22800/22801, and stream 22803.
There is no local Q/M or shop source. Prototype files include objects above
the zone's last room VNUM 22806; membership must follow files, not assume that
room bounds enumerate every item.

The tutorial state is initially applied when
[starting location resolves to 22800](../../../src/account/nanny.c#L4284).
Reviewed [item creation](../../../src/world/db.c#L3059),
[player publication](../../../src/world/handler.c#L1941), and
[active reset guard](../../../src/world/db.c#L3309) for the accounting boundary.

```bash
python3 scripts/zone_story_quest_zone_inventory.py \
  --area-evidence newbie2 --output bin/plains-of-life-evidence.json
```

## Progression story: from the Plane of Life to Ailvio

| Stage | Player action and source | Actual prerequisite/outcome | Journal capability |
| --- | --- | --- | --- |
| Arrive | Start at 22800; inspect welcome sign 22800 | Creation/start-location path applies `TAG_LIFESTREAMNEWBIE`; sign explains help/newbie chat | Discovery and orientation exist; arrival does not prove lesson completion |
| Optional armor aid | `look sign` at 22800 | `newbie_sign1` calls `spell_armor(50)` after displaying the sign; no one-time story state | Explained as optional, currently untracked |
| Learn equipment | North to 22801; inspect sign 22801 | Explains attributes, score, wear all, equipment; `newbie_sign2` applies bless | Optional aid, not a required mastery test |
| Learn abilities | East to 22802, east to 22803, north to 22804 | Room prose recommends skills/innates; no special enforces them | Guidance only |
| Accepted racewar lesson | Meet living paladin 22801 in 22804; `ask paladin racewar` or `racewars` | Exact visible target, living participants, CMD_ASK, accepted topic, and existing tutorial tag; tag is removed and sword 22804 is instantiated/offered | Contact and both aliases shown; accepted dialogue, tag transition, and committed reward need an adapter |
| Repetition | Repeat either accepted topic after tag is absent | Peace response, no second sword | Must not count another lesson or reward |
| Prepare to travel | East to 22805, east to 22806; inspect signs | Group-toggle and stream instructions; no native prerequisite beyond tutorial state | Guidance only |
| Enter the world | `enter stream` targeting room object 22803 | Tag present: refusal. Tag absent: refresh hit points, resolve room 29201, then move if room index > 0 | Travel and successful destination arrival currently untracked |

The path is north → east → east → north → east → east. Return exits exist.
The live barrier is the tutorial tag, not sword possession, inventory, or a
recorded journal objective. A gifted sword cannot satisfy the lesson, and
losing the sword must not revoke a successfully completed lesson. Characters
without the tag can enter without earning a sword. Do not retroactively infer
accepted dialogue from a missing tag: initial state, migration, or staff actions
can also explain it.

Sign handlers recognize LOOK/EXAMINE and the `sign` alias. Room prose also says
"Read Sign", but there is no CMD_READ check in these two handlers; test the
actual read-command dispatch before claiming it provides the same bonus.
Repeated sign examinations may refresh spells through their normal spell rules;
this is not a daily quest and should not grant repeated journal milestones.

The paladin carries sack 22807 with seven reset-loaded equipment items. This
is NPC inventory, not a coded friendly sack reward. Ball-of-light and cloud-wisp
mobiles have no assigned local quest behavior. Decorative note 22805 and the
unloaded Grandma-of-Ako invitation 22808 do not establish another executable
local story. Repository references to literal 22808 were found only in its
object prototype; leave it an unintegrated content lead until its intended route
is restored or explicitly retired. Do not advertise an obtainable amulet here.

## Required semantic events and failure boundaries

The future tutorial should expose optional sign knowledge/aids, one accepted
racewar lesson, confirmed sword receipt, and confirmed stream arrival. Keep
these separate: a lesson can be acknowledged, a grant can fail, and travel can
fail independently. Use canonical topic `racewar` with alias `racewars` and
stable character/season/attempt/event identities. Mere command entry and
printed success prose are insufficient evidence.

The existing journal sidecar supplies orientation and encountered-contact
guidance only. It intentionally contains no invented native contracts. Scripted
stories need an event-backed definition and terminal policy before they can
contribute achievements. A one-time tutorial should not become a daily simply
because its area was discovered.

## Blockers and repair proposals

| ID | Evidence and impact | Proposed fix and required proof |
| --- | --- | --- |
| ZSQ-RESET-AUTHORITY | Active accounting suppresses O/G/P/E item resets. A fresh active epoch cannot assume signs, stream, sack, or sack contents exist. Mobiles can still load. | Qualify durable reset generation before this tutorial's active journey. Include non-takeable scenery/portal objects and nested NPC supplies, not only loot. Preserve conditional reset order and exact occurrence identity. |
| ZSQ-TUTORIAL-GRANT | `newbie_paladin` removes the tag before `read_object`, prints a gift before checking publication, and calls the void `obj_to_char` wrapper. Active authority rejects an unowned creation candidate for a real PID. A failed grant can therefore leave the tutorial cleared without the promised sword. | Make lesson/tag/reward admission recoverable with a committed grant result. Validate the prototype, publish only committed ownership, and freeze tag/grant outcome together. Test unavailable prototype, commit failure, repeated alias, interrupted acknowledgement, and restart. Preserve already-completed legacy tutorials without inventing receipts. |
| ZSQ-TUTORIAL-TRAVEL | Stream prose and hit-point refresh precede destination validation; invalid 29201 yields a success message and no movement. `real_room > 0` also excludes valid index zero. Existing tracked sources show 29201 exists, so this is a failure-path weakness, not evidence normal travel is currently broken. | Resolve destination first with the project's valid-room convention; confirm move and any resource mutation before publishing arrival. Test missing destination, index zero, tag refusal, wrong target, disconnected character, and reconnect. |
| ZSQ-TUTORIAL-NOTE | Grandma's note 22808 is an isolated prototype with no literal active quest/reset/code route found. | Ask world maintainers to confirm intended retirement or restore a complete route with source/reward ownership. Keep out of player guidance and achievement counts meanwhile. |
| ZSQ-SCRIPT-OBJECTIVES | Q-free scripts have no terminal contract for current schema 1–3 completion steps. | Add event-backed scripted stories, with explicit one-time/repeatable policy and verified tag/travel adapters. Cover aliases, gifts, absent initial tags, repeated commands, and state/commit failure. |

These are transaction and content gaps exposed by the accounting prerequisite,
not grounds for bypassing that prerequisite. Full active gameplay remains open.
Source-comprehensive mapping is complete for the inspected revision; sign aids,
lesson/tag/grant, travel, and the orphan note are all explicitly accounted for.

The [generated review index](../../reference/zone-story-audits/newbie2.md)
records all four special assignments and the reset-command coverage.
