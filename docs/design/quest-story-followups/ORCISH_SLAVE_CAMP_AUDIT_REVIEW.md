# The Orcish Slave Camp audit supplement

**Checkpoint 48 — active and source-only connectivity, exact typed and code
references, title, and tracked-path history.** The existing
[comprehensive source map](../zone-stories/THE_ORCISH_SLAVE_CAMP.md) remains
the authority for local story, quest, room, mobile, object, reset, key, and
access details. This supplement records bounded cross-area evidence without
changing native sources.

## Active WLD boundary

The only active cross-area boundary is one reciprocal pair: Split Shield room
#10327 east (`D1`) to camp room #53200, with room #53200 returning west
(`D3`) to #10327. Both declarations are recorded in the
[cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L198) and the
[camp return row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L659). All 27 authored
camp exits resolve; the local graph is one weak component containing all 14
rooms. There are no negative or positive unresolved local targets.

## Unregistered WLD review

A stateful parse of all 95 current unregistered WLD files read 2,080,773
authored exits with zero parse issues. None of the 527,791 unregistered source
rooms reuses one of the camp's 14 active room VNUMs, and no unregistered exit
target matches a camp room. The curated source-only follow-up total therefore
remains 586, and the reviewed `surf.wld` subset remains 26. No source-only
camp ingress candidate was found.

## Bounded typed and code references

Exact VNUM scans of external active and unregistered QST, ZON reset, SHP, and
story-sidecar sources found no references to the camp's rooms, mobiles, or
objects. A direct C/C++ literal scan likewise found no camp VNUM reference.
The [comprehensive source map](../zone-stories/THE_ORCISH_SLAVE_CAMP.md)
records the local two exact-item exchanges, separate type-based food branch,
and the scan of all 713 active type-25 teleport objects; none targets a camp
room. No foreign source consumer or fixed incoming type-25 destination was
identified in these bounded scans.

## Active title and tracked history

The color-stripped active title “The Orcish Slave Camp” is unique in the
current `areas/AREA` list ([manifest](../../../areas/AREA#L228),
[zone title](../../../areas/zon/shortc.zon#L2)). Tracked WLD history in
`bb0d59425` and `f18992b8f` changes the first value in each of the 14 room
metadata tuples from 1 to 532; `71d731c5b` returns those values to 1. These
changes do not alter directional exit records. Tracked ZON changes update the
editor-version comment while preserving the reset commands. No tracked
predecessor or replacement was identified in the reviewed path history.

## Semantic titles, source references, and map identity

A normalized scan of all 442 current `.zon` titles finds one exact “The Orcish
Slave Camp” owner, active `shortc` (*532). Related current titles have separate
keys and titles: `banditca` “Bandit Camp,” `goblincamp` “The Goblin Camp,”
`nomads` “The Cimmerian Nomad Encampment,” `orogs` “Orog Encampment,” and
`torture` “Troll Slave Compound.” `raxthan` is titled “Drustl's Yerdonia
Enslaved”; it is also a separate active contributor, not an alternate camp
map.

The source-name matches are likewise scoped to their own active sources:
`llzazan` has separately described Slave Camp rooms and a camp cook; Hunt has
its own slave-compound prisoner story; Quietus, Raxthan, and Thri each reset or
define an Orcish-slave mobile; and Herders and Khildarak use “slave compound”
for locations within their own WLD files. These descriptions and actors do not
identify another `shortc` map or claim the camp's 53200–53213 room identities.
The existing CP48 typed and all-95 inactive-WLD scans found no external
consumer, same-number inactive room, or source-only approach for the camp.
`ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv` has no `shortc` candidate key; no
candidate plan is changed by this comparison.

The normalized title and bounded semantic map-identity comparison are complete.
Whether `shortc` has a separate runtime command-target alias or owner remains an
independent question and is not inferred from these prose or prototype matches.

## Scope and disposition

The Orcish Slave Camp remains an active `areas/AREA` contributor and its title,
related-title, semantic-reference, and followed-path identity review is
complete. Its one
reciprocal Split Shield pair stays in the active ledger; the all-95 WLD review
found no overlapping source-room VNUMs or target matches. Actual access,
trapdoor and key state, recipient retirement, safe return, transaction
settlement, and persisted-player outcomes remain unverified. This
documentation-only review did not run tests, a build, server/runtime/gameplay,
database, migration, or operational script.

## Checkpoint 823: current WLD and typed-source reconciliation

The bounded WLD and typed-reference checks reproduce from the current checkout. `areas/wld` contains 446 files: 351 active-manifest contributors and 95 unregistered files. The stateful parse of all 95 unregistered files again reads 527,791 rooms and 2,080,773 exits with no parse issues, source-room overlap, or exit target into camp rooms #53200–#53213. A repository-wide inventory contains 448 `.wld` files; the two additional files outside `areas/wld` are `areas_mini/mini.wld` and `areas/mob/astral_main.wld`, with no camp-range reference. The only external exact WLD match remains the active reciprocal Split Shield boundary: #10327 east to #53200 and #53200 west to #10327 ([Split Shield direction](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L198), [camp return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L659)).

Fresh exact-role scans cover the camp's 14 room, 31 mobile, and 18 object prototypes. They found no external QST item, ZON reset-role, SHP, or story-sidecar reference, no direct C/C++ literal, and no same-type active prototype overlap. The active object census reproduces 713 type-25 and 362 type-29 prototypes; neither a fixed/random type-25 destination nor a type-29 room target points into the camp. There is no local SHP file. These findings refresh bounded checked-in sources only and do not qualify dynamic references, runtime access, installed services, or persisted-player state.

No `shortc` candidate key exists. The two inactive Surf exits #206049 south to #10300 and #206649 north to #10334 remain Split Shield approaches on Surf candidate row 97; those targets are outside the camp's room range and do not create camp ingress. Keep `shortc` **DO NOT MOVE**, preserve that existing Surf plan **RETAIN IN PLACE**, and make no activation or migration change.
