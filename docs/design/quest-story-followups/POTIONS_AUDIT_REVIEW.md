# Potions repository-wide source audit

Reviewed October 10, 2026, Checkpoint 327 reconciles active `potions` against
its WLD, MOB, OBJ, and ZON sources, active-area membership, all inactive WLD
files, typed references, followed history, and the existing `brass-old-2`
candidate. This is a static source review; it does not inspect live access,
editor state, player state, or persisted world data. See the [source dossier](../zone-stories/POTIONS.md).

## Active source and authored story

The contributor is listed in `areas/AREA` line 149 and QST `AREA` line 138.
Its zone header is #307, titled “The Potion Treasure Vault,” with reset mode
0 and room range #30700–#30714. The WLD has 15 rooms, the MOB file has two
mobile prototypes, and the tracked OBJ file is zero bytes. There is no local
QST, SHP, or story sidecar, and no native Q/QA contract. The current
`areas/RANDOM_AREA` file is empty ([area lists](../../../areas/AREA#L149),
[QST list](../../../areas/qst/AREA#L138),
[zone header](../../../areas/zon/potions.zon#L1)).

The WLD names a Brass vault branch (#30700–#30706) and a Ceothia guard branch
(#30707–#30714). Only room #30705 has non-empty room description text, the
single word “First.” The Ceothian guard's prose places a guard at the Ceothia
start, while the “vault guardian” description names the Sultan; its reset
comment labels the Brass placement as a Brass guardian. The authored files do
not explain that relationship or give a player objective.

## Active graph and authored access

The WLD contains 86 authored exits. The current active world loader resolves
80: 73 local edges and seven outbound edges to active Ceothia room #81094.
Six positive targets do not resolve because room #25087 is only present in
inactive `brass-old-2.wld`. There are no active WLD inbounds, negative
targets, fall-chance rooms, or current fields.

| Branch | Rooms | Directed reach in local graph | Boundary evidence |
| --- | --- | --- | --- |
| Brass vault | #30700–#30706 (7) | From #30700, all seven local rooms are reachable; each room is its own strongly connected component. | #30705 D1–D5 and #30706 D5 target unresolved #25087. |
| Ceothia vault | #30707–#30714 (8) | From #30707, all eight local rooms are reachable; each room is its own strongly connected component. | #30709 D0, D1, D3, D4, and D5, plus #30713 D5 and #30714 D5, target Ceothia #81094. |

Across the 15 rooms, the weak components have sizes eight and seven, while
the directed graph has 15 single-room strongly connected components and no
reciprocal edge between distinct local rooms. The seven Ceothia exits do not
have a matching Ceothia-to-Potions WLD return. Treat these as authored graph
facts; they do not establish what non-WLD access or return behavior may
exist.

The all-95 inactive-WLD scan parsed without issues and found no room VNUM
overlap with #30700–#30714 and no inactive WLD exit into Potions. Thus no
source-only approach is added and the curated source-only WLD total remains
4,486 overall (Surf 252, ud_map2 19, Underworld2 eight).

| Boundary | Source record | Current resolution |
| --- | --- | --- |
| Unresolved Brass target | `potions.wld` #30705 D1–D5 and #30706 D5 → #25087 | Six active authored exits to a room present only in unregistered `brass-old-2.wld`; preserve on existing candidate row 18. The target title is “Treasure Room of the Efreeti” ([first source exit](../../../areas/wld/potions.wld#L155), [last source exit](../../../areas/wld/potions.wld#L196), [target room](../../../areas/wld/brass-old-2.wld#L2078)). |
| Resolved Ceothia target | `potions.wld` #30709 D0/D1/D3/D4/D5, #30713 D5, and #30714 D5 → #81094 | Seven one-way authored exits to active Ceothia's “Vault Room of Lord Lenbrea”; no reverse WLD edge is present ([#30709](../../../areas/wld/potions.wld#L275), [#30713](../../../areas/wld/potions.wld#L391), [#30714](../../../areas/wld/potions.wld#L416), [Ceothia room](../../../areas/wld/ceothia.wld#L5926)). |

## Reset and typed dependencies

Only two ZON records are active: mobile #30701 in #30700 and mobile #30700 in
#30707. The ten commented object placements, six commented monolith
placements, commented gear loads, and commented Negative Plane dire-stalker
load are not live reset imports. They name objects owned in other files,
including #25106 in `brass-old-3.obj`, #80831 in `ceothia.obj`, #51004 in
`hell_one_avernus.obj`, #26617/#26661 in `negplane.obj`, #25759 in
`bahamut.obj`, and #171 in `heavens.obj`. The zero-byte local OBJ file owns
none of them ([commented ZON block](../../../areas/zon/potions.zon#L9),
[active guard resets](../../../areas/zon/potions.zon#L26),
[local OBJ file](../../../areas/obj/potions.obj)).

Bounded typed scans found no external exact QST item command, SHP product,
ZON reset-role reference, story-sidecar numeric reference, duplicate MOB/OBJ
prototype, or direct C/C++ numeric binding to the local room/mobile IDs. The
external object scan examined 872 type-25 portal and 380 type-29 switch
declarations; none targets a Potions room. There are no local object VNUMs to
serve as quest or shop products. These scans do not establish live inventory
or editor state.

## Title, followed history, and retention

After game color-code removal and whitespace normalization, “The Potion
Treasure Vault” occurs once among all 442 current `.zon` titles and once
among the 351 active contributors. Followed WLD, MOB, and ZON paths remain on
the `potions` stem, with WLD edits through 2013 and MOB/ZON edits through
2015; no path rename appears in those histories. The OBJ file is empty in the
current tree and in the checked area-path snapshots from 2009 through 2015.
No alternate-stem predecessor or whole-zone replacement is established.
`brass-old-2` supplies one unresolved destination room, not replacement
evidence.

Keep active `potions` **DO NOT MOVE**. Preserve its two active guards, both
room clusters, seven one-way Ceothia exits, and six unresolved Brass exits.
Keep `brass-old-2` **RETAIN IN PLACE** on its existing candidate row with
seven active references: six from Potions and one from Realm. No active route,
candidate count, or curated source-only approach count changes here.

## Checkpoint 327: builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| PO-01 | Is `potions` intended to be two vault excursions, a potion-stock support area, or an incomplete area? | Preserve the current source boundary until the owner defines its intended player-facing role. |
| PO-02 | Who is the Sultan mentioned in #30701's guardian description? | Keep the name as authored and request an owner-defined relationship before adding any chronology or quest. |
| PO-03 | Is Sultan's vault guardian #30701 intentionally reset into the Brass Vault Start Room #30700? | Keep the active reset and its “Brass” comment; ask the owner to resolve the difference between the room and mobile descriptions. |
| PO-04 | Is #30700 meant to be a player arrival point for the Brass branch despite having no active WLD inbound? | Preserve the node and ask the owner to name any intended access mechanism before adding a route. |
| PO-05 | Are the six Brass exits from #30705/#30706 to #25087 still intended? | Keep all six source records and ask the relevant area owners to choose a loaded destination or confirm retirement in a separately reviewed change. |
| PO-06 | Should the five D1–D5 exits from #30705 and the D5 exit from #30706 all lead to the same Efreeti treasure room? | Preserve each direction and request a route-by-route owner decision; do not infer a direction mapping. |
| PO-07 | Are the local Brass links intentionally one-way, including paths from the potion rooms into #30705 and #30706? | Keep the current directed graph and ask the builder whether any reciprocal exits were omitted. |
| PO-08 | Is #30707, “Ceothia Guard Start Room,” intended as the entry point for its branch? | Preserve the room and current guard reset while the area owner confirms intended access. |
| PO-09 | Are five different exits from #30709 to the same Ceothia room #81094 intentional? | Preserve all five records and ask whether their direction labels are meaningful or accidental duplicates. |
| PO-10 | Should Ceothia #81094 have a WLD return to Potions? | Keep the current one-way boundary until both area owners define a safe, intended return path. |
| PO-11 | Are the D5 exits from #30713 and #30714 additional intended exits to #81094? | Preserve both records and request separate confirmation for each endpoint. |
| PO-12 | How should the Sultan, the Efreeti ruling family, and Lord Lenbrea relate to one another? | Leave their relationship open; the current source names do not establish shared ownership or chronology. |
| PO-13 | Should the numbered “Potion Room” names #51–#55 be explained by descriptions or retained as builder labels? | Keep the room VNUMs and labels until the area owner supplies player-facing text. |
| PO-14 | Should the 14 empty room descriptions be populated? | Preserve the existing empty fields while the owner identifies which rooms need distinct environmental text. |
| PO-15 | Is the zero-byte `potions.obj` file intentional, and should any objects be owned locally? | Preserve its current empty state until item ownership and intended availability are confirmed. |
| PO-16 | Which of the ten commented object placements, six monolith placements, and commented G/E records, if any, should be restored? | Keep every line commented; require prototype ownership and reset-state review before enabling any stock. |
| PO-17 | Was the commented Negative Plane dire-stalker group intended for an older alternate vault route? | Preserve it as disabled historical source and ask before reconnecting its mobile, weapons, or potion. |
| PO-18 | Is reset mode 0 with only two active guard loads intended for this vault? | Keep the header and two M records; qualify reset behavior and intended population before edits. |
| PO-19 | Does QST `AREA` membership indicate a planned Potions quest even though there is no local QST, SHP, or story sidecar? | Treat the membership line as inventory only; require a defined story and native contract before describing a quest or shop. |
| PO-20 | Should the inactive `brass-old-2` candidate remain attached to the six Potions exits as well as Realm's one exit? | Keep all seven active target references on existing candidate row 18 with **RETAIN IN PLACE**; do not add active ingress or clear the row without owner review. |

