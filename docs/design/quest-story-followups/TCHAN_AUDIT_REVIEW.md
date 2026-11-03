# Tchan repository-wide source audit

This supplement reconciles the [Tchan monastery dossier](../zone-stories/TCHAN_MONASTERY.md) with the active world graph, inactive-WLD approaches, typed reset and procedure references, normalized title, and followed source history. All findings are from the current source checkout. No game source, zone data, or runtime state was changed.

## Active graph and Surface boundary

The active [Tchan WLD](../../../areas/wld/tchan.wld#L1) contains rooms #4800-#4809 and 19 authored exits; all 19 resolve in the current active WLD set. Eighteen exits stay within Tchan. The remaining exit is west from Tchan #4800 to Surface #513140. Surface room #513140 has a matching east exit back to #4800 ([Surface source](../../../areas/wld/surface.wld#L275937)). The local weak graph has one component of 10 rooms. Directed traversal from #4800 reaches all ten rooms, and each room can reach the outward edge when door, guard, and dynamic movement conditions are ignored.

The two monastery directions are raw door exits with key VNUM 4800. The paired 100-percent D resets in tchan.zon set state 1 at #4808 east and #4809 west; the comments say closed. The current source does not reset the key prototype. The active graph therefore records an authored path behind a closed door and a guard, not ordinary player reachability. The inventory reports no negative or unresolved target, out-of-range room, fall-chance, or current marker.

## Inactive-WLD approaches and same-number rooms

The bounded scan parsed all 95 inactive WLD files without parse issues. It found 20 directions in inactive alterian_wilderness.wld whose target VNUM is a Tchan room. Nine source-room VNUMs, #4801-#4809, are also active Tchan room IDs. Their Alterian titles differ: the inactive source calls #4801-#4805 “Gravely Dirt Road,” #4806 “End of a Gravely Dirt Road,” #4807 “Front Yard,” #4808 “Next to a Small Well,” and #4809 “Corn Field”; active Tchan uses mountain-trail and monastery titles. Sixteen of the twenty target matches originate at those overlapping source IDs. Keep those records tied to their inactive source map identity; they are not additional active ingress.

Four exits originate at non-overlapping Alterian room IDs. These are source-only links and do not enter the active world loader:

| Inactive source exit | Exact WLD evidence |
| --- | --- |
| Alterian #4810, Corn Field, south to Tchan #4809 | [room](../../../areas/wld/alterian_wilderness.wld#L416), [exit](../../../areas/wld/alterian_wilderness.wld#L433), [target](../../../areas/wld/alterian_wilderness.wld#L437) |
| Alterian #4815, Large Front Porch, south to Tchan #4807 | [room](../../../areas/wld/alterian_wilderness.wld#L514), [exit](../../../areas/wld/alterian_wilderness.wld#L528), [target](../../../areas/wld/alterian_wilderness.wld#L532) |
| Alterian #4821, Grassy Meadow, west to Tchan #4803 | [room](../../../areas/wld/alterian_wilderness.wld#L633), [exit](../../../areas/wld/alterian_wilderness.wld#L652), [target](../../../areas/wld/alterian_wilderness.wld#L656) |
| Alterian #4822, Grassy Meadow, west to Tchan #4804 | [room](../../../areas/wld/alterian_wilderness.wld#L658), [exit](../../../areas/wld/alterian_wilderness.wld#L677), [target](../../../areas/wld/alterian_wilderness.wld#L681) |

Preserve all four on inactive candidate row 8. The curated source-only total rises from 7,517 to 7,521; Surf remains 286. The other sixteen same-number matches do not increase that distinct-source-room total. See the [candidate-specific plan](ATTIC_MIGRATION_PLAN.md#checkpoint-378-tchan-source-only-approaches-and-alterian-retention) and [candidate row 8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8).

## Typed dependencies and compiled references

| Source owner | Exact reference | Source interpretation |
| --- | --- | --- |
| Active tchan | [ZON #48](../../../areas/zon/tchan.zon#L8), [ZON #48](../../../areas/zon/tchan.zon#L12) | Reciprocal D resets to closed state 1 at the monastery threshold. |
| Active tchan | [ZON #48](../../../areas/zon/tchan.zon#L18) | Resets local mobile #4800 guard into room #4808. |
| Active tchan | [ZON #48](../../../areas/zon/tchan.zon#L19) | Imports Storm Port mobile #22428 into room #4809; this prototype is not locally defined. |
| Inactive Alterian source | [ZON G reset](../../../areas/zon/alterian_wilderness.zon#L76) | Source-only object #4800 reset in the inactive component. |
| Inactive Alterian source | [ZON M resets](../../../areas/zon/alterian_wilderness.zon#L128), [second M reset](../../../areas/zon/alterian_wilderness.zon#L140) | Source-only mobile #4800 resets in the inactive component. |
| Maintained code | [mobile special assignment](../../../src/specs/specs.assign.c#L600) | Mobile #4800 is assigned newbie_guard_east; the typed owner is Tchan's active guard. |
| Maintained code | [Tchan special assignment](../../../src/specs/specs.assign.c#L194) | Mobile #22428 uses clear_epic_task_spec at its Storm Port-owned definition. |

Local object #4800 is the relic-shaped key, and local mobile #4800 is the guard; the same number does not merge their object and mobile types. The external mobile and object definitions with #4800 are both in inactive alterian_wilderness files ([MOB](../../../areas/mob/alterian_wilderness.mob#L1), [OBJ](../../../areas/obj/alterian_wilderness.obj#L1)). The external ZON rows above remain inside that inactive source component. They do not establish active Alterian resets or extra Tchan stock.

Exact scans found no external native QST item/giver use, SHP product, or story-sidecar prototype field for Tchan's local #4800 prototypes. No external type-25 object portal targets rooms #4800-#4809. The QST area index lists tchan at [line 23](../../../areas/qst/AREA#L23), but the local QST file is absent and the coverage row has zero native contracts. The existing Storm Port QST and story-sidecar reference mobile #22428, not a local Tchan contract ([QST](../../../areas/qst/stormport.qst#L1), [sidecar contact](../../../areas/story/stormport.story.json#L23), [sidecar contract](../../../areas/story/stormport.story.json#L531)).

The bounded numeric scan also surfaced code values #4801 and #4802 in object-typed magic-pool assignments, #4800 as a queue-length constant, and #4800 in a guildhall golem comment. Those are not Tchan room bindings. No direct compiled room destination for #4800-#4809 was found. Computed paths and persisted player references are outside this source scan.

## Title, same-type identity, and followed history

After removing color tokens, the title normalizes to “The Monestary of Tranquility,” unique among 442 current ZON titles and 351 active area contributors. No same-type mobile or object definition duplicates the active Tchan prototypes. The two same-type #4800 definitions are both in inactive Alterian sources, which remain separate from the active source list.

The nine same-numbered Alterian rooms #4801-#4809 have different titles and map text from the Tchan rooms. They are WLD identity collisions, not evidence that the current monastery replaced an Alterian map or vice versa.

Followed WLD history reaches the 2010-05-21 door/guard commit 98b714b7d. The 2010-05-19 commit 79c3c3a40 describes Tchan moving to an icy mountain top; the 2010-05-20 commit fb93dbbb5 adds the zone to the active list and joins its trailhead to Surface. The same-day reset-disable change was reverted before the 2010-05-21 key/door update. Followed MOB history reaches 2015-10-13, OBJ history reaches 2013-11-13, and ZON history reaches 2015-10-13. These changes do not establish a whole-zone replacement. There is no local QST, SHP, or story-sidecar history because those files are absent. See [duplicate and replacement review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-378-tchan-and-alterian-identity-and-history).

## Checkpoint 759: Cross-area monastery names and character references

The color-stripped title “The Monestary of Tranquility” remains unique among
all 442 current `.zon` titles and belongs to active `tchan`, zone 48. The
“Monestary” spelling also occurs in separate rostered contexts: Storm Port's
two city rooms “Before a Small Monestary” (#22519) and “Inside a Small
Monestary” (#22520), Jade's Guardian Monestary, and Trakkia's Rilla Monestary.
Their room, reset, and contributor identities remain separate. The Storm Port
rooms still form their own closed door pair, but Storm Port's ZON has no M
reset for mobile #22428 there. Its #22428 occurrences are a room destination
for mobile #22409 and an E-reset object VNUM. `tchan.zon` instead imports Storm
Port mobile #22428 into summit room #4809. Storm Port's sidecar states the
current location and keeps the four-proof contract with Storm Port; its old
local monastery description does not establish a second Tchan reset. See
[Storm Port room pair](../../../areas/wld/stormport.wld#L2426), [Storm Port
resets](../../../areas/zon/stormport.zon#L248), [Tchan reset](../../../areas/zon/tchan.zon#L19),
and [Storm Port story](../../../areas/story/stormport.story.json#L10).

Ship Yards mobile #43163, Ayden Stoneheart, calls himself Tchan's last
surviving disciple and says Tchan “has long since been lost.” The current reset
and Storm Port sidecar instead place Tchan at the Monestary. No source dates
Ayden's account or explains this apparent character-history conflict. This is
not evidence of a second Tchan map or a whole-zone replacement; preserve both
authored records until an owner clarifies their timeline. See the [Ayden source
text](../../../areas/mob/shipy.mob#L958) and the [Tchan dossier](../zone-stories/TCHAN_MONASTERY.md#cross-area-monastery-names-and-character-references).

The `tchan` area key names the summit map. The `tchan` mobile keyword and
Storm Port's native request refer to person/mobile #22428, which Storm Port
defines and Tchan's ZON imports. Keep the map, prototype, and request ownership
separate. Existing Alterian same-number rooms and four source-only approaches
remain on candidate row 8 under their reviewed retention plan. This comparison
adds no new candidate approach or move clearance.

## Twenty-one builder questions and conservative recommendations

1. Should tchan remain the active owner of rooms #4800-#4809 and its local guard and key prototypes? Keep it DO NOT MOVE and preserve the source ownership.
2. Should the display title retain the legacy spelling “Monestary” and color-built “Tranquility”? Keep the current title until a separately reviewed copy change is approved.
3. Should the reciprocal Surface boundary remain Surface #513140 east to Tchan #4800 and Tchan #4800 west to Surface? Preserve both directions and qualify live movement separately.
4. Should the mountain approach keep its current nine-pair path from #4800 through #4809? Preserve all authored directions until a builder chooses any navigation change.
5. Should the repeated boulder, arctic-weed, and distant-trail prose across #4800-#4808 remain repeated? Review clarity and copy separately; do not infer nine unique hazards from the repeated text.
6. Should “Rubbled,” “birm,” “withinin,” “harnassed,” and “burdons” be corrected? Record an editorial pass without changing room identity or exits.
7. Do the mountain sector values and prose imply any actual traversal hazard? Keep the current no-fall/no-current inventory finding distinct from live safety qualification.
8. Should the #4808/#4809 door reset closed in both directions? Preserve both state-1 resets until builders choose and qualify the intended reset state.
9. Should the WLD key VNUM 4800 remain on both door records while the current D state is closed rather than locked? Treat this as a stale-field or intended-future-key question; do not claim the door requires an available key.
10. Should the relic-shaped key object #4800 remain in the active object source with no local E/G reset? Preserve the object definition and request its intended placement/source before adding stock.
11. Should the guard continue to spawn in #4808 directly in front of the monastery entrance? Preserve the current reset and qualify actual access.
12. Should the guard's eastward special continue to allow low-level and trusted passage while blocking other high-level attempts? Confirm intended level, trust, and mounted-rider handling against the prose before changing either.
13. Should the guard remain marked _no_move_ and keep its current privacy dialogue? Keep its present role unless builders separately revise gate behavior or text.
14. Is it intentional that active Storm Port retains the “Small Monestary” pair #22519/#22520 while its Tchan mobile #22428 is reset by tchan.zon at #4809 and its request remains Storm Port-owned? Preserve the two map identities and one mobile/request owner; clarify whether the old local high-cleric prose is dated rather than adding another Tchan reset.
15. Should Tchan's task-clearing special and native four-proof request continue to be explained through the Storm Port source record? Keep the Tchan area free of a duplicate contract or second mobile definition.
16. Why does areas/qst/AREA retain a tchan entry when no local .qst contract file exists? Confirm whether the index line is historical or intended tooling metadata; do not infer an active contract.
17. Should the Storm Port story sidecar remain the owner of Tchan's contact and proof request? Keep its contact and contract references attached to Storm Port.
18. Should inactive Alterian room IDs #4801-#4809 remain distinct from the different-title active Tchan rooms? Preserve both source identities and do not count overlapping origins as active entrances.
19. Should inactive Alterian MOB and OBJ #4800 definitions and their G/M resets remain on candidate row 8? Preserve the distinct source types and reset ownership while the component is RETAIN IN PLACE.
20. Should the four non-overlapping Alterian WLD exits into Tchan remain preserved as source-only map references? Keep all four on candidate row 8 and make no area-list or path move from this evidence alone.
21. Should Ship Yards Ayden Stoneheart's statement that Tchan has “long since been lost” remain alongside the current tchan.zon reset and Storm Port sidecar location? Preserve both source texts and have the owner clarify the timeline; do not infer Tchan's death, absence, or a second map from either statement alone.

## Validation and limits

The focused source census confirms 10 local rooms, 19/19 resolving authored exits, one local weak component of 10 rooms, one active inbound and one active outbound Surface record with a reciprocal direction, four distinct inactive Alterian approaches, and the exact typed references above. The normalized title scan covered all 442 current ZON files. This audit now records 21 source-backed builder questions, including the CP759 external character-timeline discrepancy. No build, test suite, server, gameplay session, database, migration, or operational script was run. Live access, door use, guard outcomes, safety, and item availability remain unqualified.

## Checkpoint 833: Current typed-source roles

An exact-VNUM refresh against the unchanged pinned sources confirms the existing ownership and type distinctions:

- **Local reset roles:** [`tchan.zon`](../../../areas/zon/tchan.zon#L18) resets local mobile #4800, the guard, at #4808; its next row imports Storm Port mobile #22428 to summit room #4809 ([reset](../../../areas/zon/tchan.zon#L19)). Local OBJ #4800 is the key, but no local reset places it. The paired closed-door WLD records still carry key VNUM #4800; the authored field does not prove key availability.
- **Storm Port quest owner:** Storm Port's [`stormport.qst`](../../../areas/qst/stormport.qst#L1) and story sidecar use mobile #22428 as contact/giver ([contact](../../../areas/story/stormport.story.json#L23), [contract](../../../areas/story/stormport.story.json#L531)); maintained code assigns that mobile `clear_epic_task_spec` ([assignment](../../../src/specs/specs.assign.c#L194)). The native four-proof request stays Storm Port-owned; Tchan has no local QST contract or sidecar.
- **Same-number typed roles in Storm Port:** Its active WLD also defines room #22428, its MOB defines the Tchan character, and its OBJ defines a separate item ([room](../../../areas/wld/stormport.wld#L609), [mobile](../../../areas/mob/stormport.mob#L410), [object](../../../areas/obj/stormport.obj#L399)). Storm Port's `M #22409` targets room #22428 and its `E #22428` row equips OBJ #22428 ([ZON](../../../areas/zon/stormport.zon#L324), [object reset](../../../areas/zon/stormport.zon#L472)); neither row is a Storm Port reset of mobile #22428. Tchan's `M #22428` at #4809 is the separate mobile placement.
- **Source-only same-number records:** Inactive Alterian keeps its own MOB/OBJ #4800 and its own `M/G #4800` resets ([object reset](../../../areas/zon/alterian_wilderness.zon#L76), [mobile resets](../../../areas/zon/alterian_wilderness.zon#L128)). These remain source-only on existing candidate row 8; they add no active Tchan reset or same-type collision.

No exact external QST item/giver, story-sidecar prototype field, SHP product, or type-25 destination to Tchan's local #4800 prototypes/rooms was found. The `qst/AREA` entry still does not provide a local contract. Keep the ten-room active map and four distinct Alterian approaches as already reviewed; this refresh adds no route, candidate, or move clearance.

