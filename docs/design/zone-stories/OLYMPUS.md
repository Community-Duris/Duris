# Olympus: the Golden Halls

Active source `olympus`, zone 998, contains ten rooms (#99800–#99809), one mobile prototype, and eight object prototypes. It is listed in `areas/AREA` and `areas/qst/AREA`, but has no local QST file, giver, contract, SHP file, or story sidecar. Its authored content is a small gold-and-gem portal hall with one sentry and an unresolved western boundary.

Sources: [world rooms](../../../areas/wld/olympus.wld#L1), [mobile](../../../areas/mob/olympus.mob#L1), [portal objects](../../../areas/obj/olympus.obj#L1), [resets](../../../areas/zon/olympus.zon#L1), [full-world membership](../../../areas/AREA#L363), [QST-area membership](../../../areas/qst/AREA#L321), [route, identity, and dependency review](../quest-story-followups/OLYMPUS_AUDIT_REVIEW.md).

## Story in the source

Rooms #99800–#99804 are titled “A Golden Hall.” Their repeated descriptions show gold architecture etched with runes and figures and suggest an abode made by powerful magic. Four connected rooms (#99805–#99808) contain emerald, sapphire, ruby, and diamond archways. The large golden arch at #99809 is filled with strange light and further runes. The WLD text establishes a magical gateway setting, but names no maker, destination realm, task, or outcome.

The only local mobile is an ancient sentry. Four repeated reset commands place it at the huge archway room #99809, each with a maximum count of four and a 100 percent field. The source defines no dialogue, tracked challenge, or reward around this sentry.

The Behemoth Herders QST has one name-only reference: a response says that a “flaming horde” lives deep within Mount Olympus while the Herders continue digging ([QST text](../../../areas/qst/herders.qst#L34)). It gives no room or mobile VNUM and declares no route. The local Olympus mobile list contains only the ancient sentry, so this remains a narrative lead rather than a verified encounter dependency. See the [Herders dossier](THE_BEHEMOTH_HERDERS.md#lore-foreign-ownership-and-orphan-closure) and [audit review](../quest-story-followups/HERDERS_AUDIT_REVIEW.md#bounded-typed-references).

## Portal layout

Eight type-25 objects form four color-coded routes. Each uses the `enter` command and has unlimited charges. The local ZON resets four portal objects into #99804 and the matching partner into each colored archway room:

| Color route | Portal in #99804 | Destination | Partner in colored room | Destination |
| --- | --- | --- | --- | --- |
| Emerald | #99801 | #99805 | #99800 in #99805 | #99809 |
| Sapphire | #99803 | #99806 | #99802 in #99806 | #99809 |
| Ruby | #99805 | #99807 | #99804 in #99807 | #99809 |
| Diamond | #99807 | #99808 | #99806 in #99808 | #99809 |

These item teleports complement the authored WLD connections between the hall and archway rooms. The standard teleport path uses each item's declared target. A separate `olympus_portal` procedure contains code to move four selected portals periodically to random non-ocean rooms in four zone ranges and update their partner portals' return targets. However, all four object assignments to that procedure are inside a `#if 0` block in `specs.assign.c`. Static procedure code does not establish that these portals currently roam or lead to those distant zones.

## Current interactions and services

The raw WLD network is fully connected across all ten local rooms. Its only external exit is the west exit from #99809 to unresolved room #218064. That target appears only in two inactive Surface sources, under different room titles; the normal active-area load prunes the exit. The route and destination ambiguity are detailed in the [audit review](../quest-story-followups/OLYMPUS_AUDIT_REVIEW.md#active-wld-graph-and-unresolved-west-exit).

There is no local `areas/qst/olympus.qst` despite the QST-area manifest entry, and no local story sidecar. No source-defined QST outcome, shop, or reward is present. The [audit review](../quest-story-followups/OLYMPUS_AUDIT_REVIEW.md#checkpoint-316-twenty-direct-builder-questions-and-recommendations) records twenty builder questions and conservative recommendations. Keep the active area **DO NOT MOVE** while its unresolved exit identity, portal behavior, and name-only Herders reference are clarified.
