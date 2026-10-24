# Cityruin repository-wide source audit

Checkpoint 202 reconciles active `cityruin` against its source-story review,
the active WLD loader list, compiled room/mobile references, source history,
and typed reset/QST/shop/sidecar consumers. This is a static source audit; it
does not inspect a live server, player state, database, editor, or persisted
world state. The [canonical source dossier](../zone-stories/CITYRUIN.md)
summarizes the setting; the [detailed story review](CITYRUIN_STORY_REVIEW.md)
retains the per-room story and the original builder questions.

## Source identity and authored stories

Cityruin is header #662, reset mode 1, with 155 rooms (#66200–#66354), 42
mobiles (#66200–#66241), 37 objects (#66200–#66236), and 327 authored WLD
exits. It is in the un-commented `areas/AREA` full-world list. There is no
local SHP source or story sidecar. Its QST file defines messages for two
givers, #66228 and #66241, but no native `Q`/`QA` completion contract
([zone header](../../../areas/zon/cityruin.zon#L1), [active list](../../../areas/AREA#L249),
[local QST](../../../areas/qst/cityruin.qst#L1)).

The authored story centers on a ruined old quarter with a necromancer-created
undead population. Civic buildings, guild halls, an armoury, wealthy homes,
a failed shelter, sewers, and a jeweller's safe contribute the district's
history. The source does not provide one definitive destruction timeline.
The prior [source-story review](CITYRUIN_STORY_REVIEW.md) documents the
individual scenes and descriptions, including repeated remains descriptions
that conflict with the distinct item identities required by the neighbouring
quest.

## Active WLD graph and reset-state reach

The active WLD contains 327 resolving exits: 326 local and one outbound
boundary. Its 155 rooms form one local weak component. Active directed reach
with every local exit treated as traversable is 155/155 from the Tharnadian
Ruin arrival at #66200. No active unresolved, negative, or out-of-range target
was found in this WLD.

| Direction | Authored edge | Active status |
| --- | --- | --- |
| Inbound | Tharnadian Ruin #5588 west to Cityruin #66200 | Active and paired. |
| Outbound | Cityruin #66200 east to Tharnadian Ruin #5588 | Active reverse record. |
| Source-only inbound | `surf.wld` #262344 west to Cityruin #66200 | Present outside `areas/AREA`; it is not active ingress. |

The complete scan of all 95 inactive WLD files found exactly this one
non-overlapping source-only approach into Cityruin, at
[`surf.wld` line 1308240](../../../areas/wld/surf.wld#L1308240). The route was
already present in the global source-only ledger; this checkpoint does not
add a curated record or change the world totals. Preserve it on Surf candidate
row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

The zone has 48 unconditional ZON `D` resets. Applying the reset-state logic
in `reset_zone()` sets 27 exits closed, 14 closed and secret, and seven closed
and locked; the seven locked commands include the single reset-locked
Cityruin-side entry to the armoury, which uses key #66200. The reverse
armoury exit resets closed but unlocked. The #66200 entry gate also resets
closed but unlocked ([reset block](../../../areas/zon/cityruin.zon#L8),
[armoury door resets](../../../areas/zon/cityruin.zon#L100),
[reset implementation](../../../src/world/db.c#L7857)).

After those resets, 278 of 326 local directed exits are already open,
non-secret, and non-blocked. In that strict open-exit graph, directed reach is
1/155 from active arrival #66200 and 91/155 from street rooms #66201 and
#66202. The graph has 19 weak components; the largest are 91, 24, 14, 5, 4,
and 4 rooms, with thirteen single-room components. Closed unlocked doors may
be opened and secret or locked doors may have additional discovery or key
routes. These source counts do not simulate those commands, picking, live
reset timing, special procedures, or safe return, so they do not establish
inaccessibility or player safety.

## Typed inbound and outbound dependencies

The exact-role scan found these reset and shop references from other source
groups:

| Consumer | Cityruin target | Status |
| --- | --- | --- |
| Active `bs.zon` O reset at line 576 | Object #66219, black onyx statue | Active reset-role reference ([reset](../../../areas/zon/bs.zon#L576)). |
| Unregistered `nexus56.zon` G reset at line 1376 | Object #66211, “The Mt.Sknak Treasure Hoard” | Source-only reset reference ([reset](../../../areas/zon/nexus56.zon#L1376)). |
| Unregistered `nexus56.shp` product at line 499 | Object #66211 | Source-only shop product; conditional shop-reader fallback does not prove an operating shop ([stock](../../../areas/shp/nexus56.shp#L499)). |
| Active `tharnadian_ruin.qst` | Objects #66201, #66225, #66226, #66227, and #66230 as player-given items; #66200 as a received reward | Active quest consumer. The QST loader distinguishes `G` player-given goals from `R` rewards ([QST records](../../../areas/qst/tharnadian_ruin.qst#L70), [parser](../../../src/world/quest.c#L2202)). |
| Active `tharnadian_ruin.story.json` | `item_vnums` #66201, #66225, #66226, #66227, and #66230; completion keys also receive #66200 | Structured sidecar use of the same story items ([armoury contract](../../../areas/story/tharnadian_ruin.story.json#L81), [spirit proof](../../../areas/story/tharnadian_ruin.story.json#L179)). |

Cityruin itself imports Moonshae object #26233 and Heavens object #359 into
the armory. The imported prototypes remain tied to their defining areas
([Cityruin resets](../../../areas/zon/cityruin.zon#L216),
[Moonshae object](../../../areas/obj/moonshae.obj#L498),
[Heavens object](../../../areas/obj/heavens.obj#L3725)).

Compiled bindings and source scans add these dependencies:

| Binding or scan | Finding |
| --- | --- |
| `specs.assign.c`, mobile #66208 | `money_changer` procedure on the reset-placed zombie noble ([assignment](../../../src/specs/specs.assign.c#L810), [reset](../../../areas/zon/cityruin.zon#L261)). |
| `specs.assign.c`, room #66355 | `undead_inn` targets a room outside Cityruin's #66200–#66354 WLD range; no active WLD defines #66355 ([assignment](../../../src/specs/specs.assign.c#L2330), [last Cityruin room](../../../areas/wld/cityruin.wld#L3400)). |
| `constant.c`, `guild_locations` | Class-specific arrival rows reference Cityruin rooms #66345, #66348, and #66354 ([table](../../../src/core/constant.c#L1967)). |
| `constant.c` and `justice.c` | Hometown room #66202 and justice/home data #66201 are compiled room references ([hometown](../../../src/core/constant.c#L1737), [justice row](../../../src/combat/justice.c#L91)). |
| `chaos_eq_data.h` | Local object #66231 occurs in two chaos-paladin equipment rows ([first](../../../src/account/chaos_eq_data.h#L97), [second](../../../src/account/chaos_eq_data.h#L349)). |
| `specs.assign.c`, mobile #66238 | Its reaver special assignment is explicitly set to null ([assignment](../../../src/specs/specs.assign.c#L435)). |

The full bounded scan found no external ZON reset of a Cityruin mobile, no
external QST giver reference to a Cityruin mobile, no active external SHP
product for a Cityruin object, no duplicate external MOB/OBJ prototype
definition, and no external type-25 portal or type-29 switch targeting a
Cityruin room. It examined 872 external type-25 and 380 external type-29 OBJ
declarations. Outside story sidecars, no additional structured
`item_vnums` reference was found. Runtime use, editor/tool references, and
saved-player or persisted-world state remain uninspected.

## Title, tracked history, and disposition

The color-normalized `.zon` title “The Ruins of Tharnadia's Old Quarter” is
unique among all 442 current zone sources. The `areas/AREA` comment carries
the separate legacy note “kharne”; a bounded search found no same-title source
or compiled `kharne`/`cityruin` target alias. Tharnadian Ruin is the connected
neighbour, while active Tharnadia has a separate title and no source-backed
replacement relationship to Cityruin.

Followed WLD, MOB, OBJ, ZON, and QST history retains the `cityruin` stem from
the 2009 move into `areas/`. Later tracked edits change record formats, mobile
and object fields, and quest-item flags. The bounded history shows no alternate
stem, retirement, or whole-zone replacement. The legacy zone note and shared
Tharnadian quest items do not establish that Cityruin was replaced.

Keep active `cityruin` **DO NOT MOVE**. Preserve the Tharnadian Ruin WLD pair,
its active QST/story item references, the imported Moonshae and Heavens
objects, and the compiled money-changer, justice, hometown, and guild-room
bindings. Keep Surf candidate row 97 **RETAIN IN PLACE** and preserve its
already-counted source-only `surf.wld` approach. No Cityruin attic candidate
row or native source placement change is proposed.

## Builder follow-up

1. Reconcile closed gate prose, the armoury key/reward, and the one-sided
   reset-locked armoury entrance while preserving the active neighboring WLD
   pair.
2. Confirm whether the `noteforimm` topic and its epic-point adjustment text
   are intended player-facing dialogue.
3. Confirm the names and extra descriptions for the five Cityruin remains
   objects used by active Tharnadian Ruin QST/story records.
4. Resolve the #66355 `undead_inn` binding and confirm the compiled guild
   rooms #66345/#66348/#66354 remain intentional.
5. Confirm the money-changer/lich-alchemist roles and the story relationship
   between this ruined quarter, Tharnadian Ruin, and active Tharnadia.

The audit records static source evidence. It does not qualify gates, secrets,
quest interaction, NPC services, combat, safety, or live reachability.

## Bloodstone Checkpoint 778: object reset consumer

Active Bloodstone ZON has one object reset reference to Cityruin object #66219. Preserve current object ownership; see the [Bloodstone typed-dependency audit](BS_AUDIT_REVIEW.md#outbound-active-reset-references).
