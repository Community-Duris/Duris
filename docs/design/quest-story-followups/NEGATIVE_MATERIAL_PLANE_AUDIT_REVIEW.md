# Negative Material Plane: repository-wide connectivity and dependency audit

**Checkpoint 94; priority 134 of the original 220-zone queue.** This
supplement reconciles the [source-comprehensive dossier](../zone-stories/NEGATIVE_MATERIAL_PLANE.md)
with the active WLD graph, all inactive WLD sources, typed external
references, numeric C/C++ matches, normalized active title, and followed
WLD/ZON history. Authored exits and static code destinations do not establish
ordinary availability, safe traversal, or played qualification.

## Active WLD graph

The active manifest has 351 WLD contributors. Negative Material Plane has
270 rooms and 999 authored exits; all 999 resolve, with no WLD parse issues.
All exits are local to this contributor. Its internal graph has five weak
components of 168, 27, 25, 25, and 25 rooms. The largest spans #26600–#26869.
The smaller components are Dust (#26682–#26706, 25 rooms), Salt (#26707–#26730
plus #26870, 25), Ash (#26731–#26755, 25), and Vacuum (27 rooms with VNUMs in
#26780–#26816). The four quasi-elemental plane components are disconnected
from the larger component in authored WLD topology. The source dossier maps
LICK and other dynamic movement separately; those mechanisms are not WLD
edges.

Two active areas have one-way exits into Negative Material Plane:

| Active source | Authored exit | Return |
| --- | --- | --- |
| Realm room #14210, “No Repair Smithy Dispersement Room” | Up to room #26852 ([source record](../../../areas/wld/realm.wld#L4752), [target](../../../areas/wld/realm.wld#L4756); [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L260)) | None |
| Winterhaven room #55633, “First teleport room for gnomish key” | South to room #26860 ([source record](../../../areas/wld/wh.wld#L13049), [target](../../../areas/wld/wh.wld#L13052); [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L791)) | None |

There are no active WLD exits from Negative Material Plane to another area,
and neither inbound has an authored reverse exit. No target is negative or
unresolved; no source room lies outside the numeric zone range, no exit is
authored from a range-mismatched room, and no inactive-WLD target reference
is unregistered. Rooms #26760 and #26836 carry the two F100 fall-chance fields
([first](../../../areas/wld/negplane.wld#L4364),
[second](../../../areas/wld/negplane.wld#L6149)); no room has a current field.
The only global active-WLD duplicate found remains the unrelated #51800
collision in Hell Two and Hell Three.

These counts describe the authored graph. Portal, LICK, spell, reset, access,
fall, and return behavior remain distinct, and no live route or hazard outcome
is qualified here.

## Inactive WLD sources

The all-95 inactive-WLD scan parsed 2,080,773 exits without issues. It found
no exit targeting a Negative Material Plane room and no source-room VNUM
overlap. These files add no source-only approaches; the curated source-only
reference total remains 1,505 and the reviewed Surf subset remains 122.

## Typed references and external producers

Ten external active QST typed item-role records refer to local objects. Nine
independent `G I 26614` records consume the orb of unmaking: Bandit
([QST line 19](../../../areas/qst/bandit.qst#L19)), Ceothia
([line 348](../../../areas/qst/ceothia.qst#L348)), Cliff Giant Dungeon
([line 74](../../../areas/qst/cldgt.qst#L74)), Cliff Haven
([line 89](../../../areas/qst/clfhaven.qst#L89)), Connector Zones
([line 58](../../../areas/qst/connectorzones.qst#L58)), Darkfall
([line 43](../../../areas/qst/darkfall.qst#L43)), Ixxillikor
([line 16](../../../areas/qst/ixxillikor.qst#L16)), New Cave City
([line 23](../../../areas/qst/new_cavecity.qst#L23)), and Tribal Oasis
([line 17](../../../areas/qst/oasis.qst#L17)). Alatorin has one separate
`R I 26631` reward record for boots #26631
([QST line 3824](../../../areas/qst/alatorin.qst#L3824)); it does not consume
that object. No external SHP product or mobile/object prototype definition
matches a local Negative Material Plane prototype.

Five active external ZON records supply local object types. Bahamut, Celestia,
and Tiamat reset potion #26661 ([Bahamut](../../../areas/zon/bahamut.zon#L168),
[Celestia](../../../areas/zon/celestia.zon#L165),
[Tiamat](../../../areas/zon/tiamat.zon#L94)); Nyneth3 equips cloak #26665
([line 231](../../../areas/zon/nyneth3.zon#L231)); Vecna equips gloves #26666
([line 266](../../../areas/zon/vecna.zon#L266)). These external sources are
distinct from local reset groups and callbacks mapped in the dossier.

Across 856 external type-25 declarations (697 active and 159 source-only), one
fixed destination targets this area: portal object #59087 in
[`ravenloft2.obj`](../../../areas/obj/ravenloft2.obj#L1299) targets room
#26600, and its active reset places it in room #59169
([`ravenloft2.zon`](../../../areas/zon/ravenloft2.zon#L925)). This is an
external declared portal route, separate from the two WLD inbounds.

All 231 external story sidecars parse without errors. A structured field walk
finds nine `item_vnums` references to #26614 and Ceothia's contact reference
to mobile #26642 ([sidecar line 163](../../../areas/story/ceothia.story.json#L163)).
A deeper token scan finds 40 numeric occurrences across ten sidecars, with
unique IDs #26614, #26631, and #26642. Most are mirrored completion-key or
step identifiers for those same exchanges; they are not 40 separate
dependencies. Alatorin's #26631 appears inside completion-key strings, while
the typed QST record is its reward. The ten files are Alatorin, Bandit,
Ceothia, Cliff Giant Dungeon, Cliff Haven, Connector Zones, Darkfall,
Ixxillikor, New Cave City, and Tribal Oasis.

## C/C++ numeric source matches

The static source scan found 30 exact numeric match lines in nine C/C++ files,
covering 16 unique IDs. The matches classify as follows:

| Source context | Classification |
| --- | --- |
| [`utils.h`](../../../src/core/utils.h#L1089), [`map.h`](../../../src/world/map.h#L67) | Shared constants cover #26600–#26869 and #26600–#26681 respectively; a numeric bound does not itself add a route. |
| [`chaos_eq_data.h`](../../../src/account/chaos_eq_data.h#L174) | Equipment profiles include local necklace #26639 ([line 174](../../../src/account/chaos_eq_data.h#L174)), boots #26631 ([line 193](../../../src/account/chaos_eq_data.h#L193)), and Sanguine #26621 ([line 203](../../../src/account/chaos_eq_data.h#L203)). |
| [`specs.assign.c`](../../../src/specs/specs.assign.c#L1154) | Ordered literal bindings include the negative-pocket mobile #26603 ([line 1154](../../../src/specs/specs.assign.c#L1154)) and local object callbacks #26653 ([line 1374](../../../src/specs/specs.assign.c#L1374)), #26665 ([line 1360](../../../src/specs/specs.assign.c#L1360)), and #26666 ([line 1622](../../../src/specs/specs.assign.c#L1622)). Object #26662 is first assigned `orb_of_destruction` at [line 1413](../../../src/specs/specs.assign.c#L1413), then overwritten by `neg_orb` at [line 1415](../../../src/specs/specs.assign.c#L1415); six of seven literal statements are effective. |
| [`spells.c`](../../../src/magic/spells.c#L295) | The plane table includes #26600; dynamic room selection also uses #26601–#26681 at [line 367](../../../src/magic/spells.c#L367), [line 505](../../../src/magic/spells.c#L505), and [line 677](../../../src/magic/spells.c#L677), not a fixed WLD edge. |
| [`artifact.c`](../../../src/guild/artifact.c#L273) | The artifact index has entries for local object #26636 on a player and corpse. |
| [`randomeq.c`](../../../src/item/randomeq.c#L122), [`specs.character_progression.c`](../../../src/specs/specs.character_progression.c#L33) | The Dark mobile #26642 is in a random-equipment list; room #26860 is an optional beacon location. |
| [`random.zone.c`](../../../src/world/random.zone.c#L464), [`specs.assign.c`](../../../src/specs/specs.assign.c#L1360) | Other exact matches load local potion #26661 and bind Elvenkind cloak #26665; the source dossier classifies the full callback set and imported behavior. |

This is a static numeric source scan. It does not claim compilation,
configuration-specific binding, runtime availability, or successful movement.

## Title, history, and disposition

The color-stripped title in
[`negplane.zon`](../../../areas/zon/negplane.zon#L2) normalizes uniquely among
the 351 active contributors. Followed WLD/ZON history records the original
area addition, later fixes and content maintenance, including negative-pocket
and quest-item changes; it identifies no distinct predecessor or replacement.
An early combined commit message also names a Royal Crypt addition to IceCrag,
which is not evidence that the Royal Crypt was a Negplane predecessor. The
[duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md) has no named
Negplane entry; broader semantic comparison remains open.

The current un-commented [`areas/AREA` row](../../../areas/AREA#L134) remains
**DO NOT MOVE**. The existing dossier retains the detailed local story,
callback, travel, supply, and builder-question analysis. This supplement adds
the static graph and repository-wide dependency reconciliation; door/reset
state, dynamic route availability, safe return, editor/tooling closure, and
played qualification remain separate.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.


## Checkpoint 302 all-442 title and followed history

A case- and whitespace-normalized scan after removing color codes from all 442 current .zon titles finds one exact owner for “Negative Material Plane”: active negplane ([zone title](../../../areas/zon/negplane.zon#L2)). This extends the CP94 title check, which covered the 351 active contributors.

Followed current WLD, MOB, OBJ, ZON, and QST paths remain on the negplane stem from the 2009 areas-tree move through their reviewed same-stem maintenance; the latest reviewed changes are WLD/QST in 2013, MOB/ZON in 2015, and OBJ in 2016. The story sidecar was added October 6, 2026. There is no local SHP file. The 2012 combined commit message also mentions a Royal Crypt addition to IceCrag; the message does not identify Royal Crypt as a Negative Plane predecessor. No alternate-stem predecessor, tracked title rename, or explicit whole-zone replacement claim was found in these bounded paths. Preserve the active contributor as DO NOT MOVE.

The CP94 source closure and later cross-area evidence remain current: the two one-way active WLD inbounds, the fixed Ravenloft2 portal to #26600, nine external QST consumers of orb #26614, outside reset sources for local #26661/#26665/#26666, and the New Cave City, Bandit Canyons, and Nyneth3 references recorded in the coverage row. No route or candidate is added.

## Checkpoint 302: twenty direct builder questions and recommendations

These questions preserve the full source findings in the [comprehensive dossier](../zone-stories/NEGATIVE_MATERIAL_PLANE.md) and the existing [CP94 audit](NEGATIVE_MATERIAL_PLANE_AUDIT_REVIEW.md). Recommendations keep the two native contracts, typed items, and route owners distinct until a builder chooses a documented change.

1. **Should Projection #26608’s six exact offerings remain one completed return?** Recommendation: keep Q24 as one six-item recipe for the Mournblade and key; do not split its inputs into six independent achievements.
2. **Should Spirit #26644 continue to accept orb of unmaking #26614 while rewarding the different orb of destruction #26662?** Recommendation: preserve the two prototype identities and exact recipe unless the owner defines a new explicit conversion.
3. **Does the spirit’s promise of potions describe the intended reward?** Recommendation: resolve the dialogue against the declared currency, experience, orb, and key outputs before changing either prose or reward data.
4. **Are Projection and Spirit intended as independent accepted endpoints rather than a required player-by-player sequence?** Recommendation: preserve independent receipts and add a phase prerequisite only after the owner specifies transformation, failure, and reset ownership.
5. **Should carried note #26663 remain ordinary stock rather than a durable failure record?** Recommendation: keep a failure state separate from the note until its actor attribution, recovery, and persistence behavior are specified.
6. **Should exact supplied proof remain valid without the player’s own guardian kill or first recovery?** Recommendation: preserve current item-based acceptance and record source history separately from receipt history.
7. **How should ordinary players enter the Dust, Salt, Ash, and Vacuum components from the larger forest?** Recommendation: document the intended ENTER, LICK, or EXAMINE routes and their return before treating the static graph as a connected map.
8. **Is absent room #26660 within the dynamic plane-selection range intended to refuse movement?** Recommendation: retain NOWHERE refusal unless the owner chooses a valid destination and qualifies spell restrictions.
9. **Should the sigil field on #26835 south become an actual gate?** Recommendation: confirm the desired door and key behavior first; file value 4 currently decodes to no door and no matching D reset exists.
10. **Should Realm #14210 and Winterhaven #55633 remain one-way WLD entrances?** Recommendation: preserve the current source records and document any safe return as a separate builder-owned route.
11. **Is Ravenloft2 portal #59087 at #59169 intended as a fixed entry to #26600?** Recommendation: retain its exact source and target ownership and qualify admitted ENTER, arrival, and return before promising availability.
12. **Should the plane keep zero ordinary WLD exits back out while spells provide separate travel?** Recommendation: document static and dynamic routes separately; add no reciprocal WLD edge without a chosen design.
13. **Should an already open key door satisfy only access, while personal key use remains a separate action?** Recommendation: preserve successful unlock/open evidence, current shared door state, and story receipt as distinct outcomes.
14. **Should lethal Negative Pocket messages identify the dying victim rather than the pocket as actor?** Recommendation: review the exact victim-facing output before a separate message repair and preserve current damage behavior.
15. **Is #26662’s effective neg_orb callback intended after the earlier orb_of_destruction assignment is overwritten?** Recommendation: confirm the final ordered binding before any callback repair; retain exactly one effective behavior.
16. **Should nine external QST consumers continue to use orb #26614 as an independent exact input?** Recommendation: preserve each external recipe and ownership; do not make any of them an unstated stage of the local Sodolum story.
17. **Are external reset sources for local potion #26661, cloak #26665, and gloves #26666 intended supply dependencies?** Recommendation: keep source areas and local objects separately owned and qualify stock and custody before using them as availability guarantees.
18. **How should the nine-room potion vault be accessed from the pool and monolith scene at #26860?** Recommendation: confirm the intended entry and return because no reviewed WLD edge or fixed portal targets the vault rooms.
19. **Should the F100 fields at #26760 and #26836 remain fall hazards without an automatic story outcome?** Recommendation: preserve their current hazard role and qualify surviving-player recovery before changing fall values or adding credited outcomes.
20. **Should the two accepted returns remain story-only, with no daily candidate?** Recommendation: keep repeatability and daily classification disabled until actor availability, full stock renewal, accepted settlement, and durable accounting are qualified.
