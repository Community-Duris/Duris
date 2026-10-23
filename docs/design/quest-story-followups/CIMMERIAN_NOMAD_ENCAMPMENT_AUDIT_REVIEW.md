# The Cimmerian Nomad Encampment audit supplement

**Checkpoint 50 — active and source-only connectivity, exact typed and code
references, title, and tracked-path history.** The existing
[comprehensive source map](../zone-stories/THE_CIMMERIAN_NOMAD_ENCAMPMENT.md)
remains the authority for local story, quest, room, mobile, object, reset,
hidden-source, and access details. This supplement records bounded cross-area
evidence without changing native sources.

## Active WLD boundaries

All 124 authored exits resolve across the 49 Nomads rooms, which form one weak
component in the static authored-exit graph. Four reciprocal Surface pairs
connect these local endpoints:

| Surface room → Nomads room | Nomads room → Surface room | Exit ledger rows |
| --- | --- | --- |
| #582095 south → #6203 | #6203 north → #582095 | [L1523](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1523); [L109](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L109) |
| #582494 east → #6221 | #6221 west → #582494 | [L1524](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1524); [L110](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L110) |
| #582496 west → #6227 | #6227 east → #582496 | [L1525](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1525); [L111](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L111) |
| #582895 north → #6246 | #6246 south → #582895 | [L1526](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1526); [L112](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L112) |

These are the four incoming and four outgoing boundary records already present
in the active ledger. Pairing does not prove door state, travel safety, or
actual player reachability.

## Unregistered WLD review

A stateful parse of all 95 current unregistered WLD files read 2,080,773
authored exits with zero parse issues. None of the 527,791 unregistered source
rooms reuses a Nomads room VNUM, and no unregistered exit target matches a
Nomads room. No source-only ingress candidate was found. The curated source-only
follow-up total remains 586; the individually reviewed `surf.wld` subset
remains 26.

## Bounded typed and code references

Exact external QST and active/unregistered ZON reset scans found no references
to Nomads rooms, mobiles, or objects. A structured scan of the other 231 story
sidecars found no exact numeric value or decimal-only string equal to a local
room, mobile, or object VNUM. Three broad text matches in `wh.story.json` are
only digit substrings inside generated IDs, not VNUM fields
([fox exchange](../../../areas/story/wh.story.json#L946),
[hummingbird exchange](../../../areas/story/wh.story.json#L950),
[centaur exchange](../../../areas/story/wh.story.json#L1051)).

Two unregistered SHP files each contain two shop records whose typed room
locations point into Nomads: `tharnadia1.shp` record #6025 sets keeper mobile
#6025 and room #6242, while record #6066 sets keeper #6066 and room #6243
([first record](../../../areas/shp/tharnadia1.shp#L123),
[first keeper field](../../../areas/shp/tharnadia1.shp#L147),
[first room field](../../../areas/shp/tharnadia1.shp#L149),
[second record](../../../areas/shp/tharnadia1.shp#L315),
[second keeper field](../../../areas/shp/tharnadia1.shp#L338),
[second room field](../../../areas/shp/tharnadia1.shp#L340)); `tharold.shp`
repeats those two locations and keeper values
([first keeper field](../../../areas/shp/tharold.shp#L144),
[first room field](../../../areas/shp/tharold.shp#L146),
[second keeper field](../../../areas/shp/tharold.shp#L330),
[second room field](../../../areas/shp/tharold.shp#L332)). The `#6025` and
`#6066` shop headers are record labels; the same values in the keeper fields
are mobile lookups. Neither mobile VNUM has a current prototype. The only
Nomads references are the four typed `in_room` fields.

Both files are unregistered and omitted by normal `areas/AREA` aggregation.
The normal shop loader opens `areas/world.shp` first, then falls back to
concatenating every `areas/shp/*.shp` file if the catalog is absent
([fallback loader](../../../src/economy/shop.c#L3276)). It resolves the keeper
through the mobile table and reads `in_room` as a separate field
([keeper](../../../src/economy/shop.c#L3489),
[room](../../../src/economy/shop.c#L3496)). This establishes potential
fallback parser exposure, not working shops; neither keeper resolves, and no
runtime was run. Retain both candidates pending their broader source-owner,
typed, fallback, and tooling closure.

The bounded exact C/C++ literal scan found two code-defined connections. The
`do_shift_prime` handler's Githzerai destination array includes Nomads bonfire
room #6224 ([array](../../../src/classes/innates.c#L2010),
[room source](../../../areas/wld/nomads.wld#L498)); selection is
conditional and random, so this is an existing code route rather than a WLD
edge or guaranteed arrival. The CHAOS starter item table contains 24 fixed
slot entries for local crown object #6222
([profile entry](../../../src/account/chaos_eq_data.h#L106),
[object source](../../../areas/obj/nomads.obj#L396)); the selected kit
roots go through the durable before-entry grant
([grant call](../../../src/account/nanny.c#L751)). These declarations do not
prove that every character receives the crown or that crown possession is a
Nomads quest receipt. No other direct C/C++ VNUM literal matched the local
room, mobile, and object sets. The comprehensive dossier also records no
foreign fixed type-25 destination into Nomads rooms, no local shop source, and
no local special assignment.

## Active title and tracked history

Color-stripping and whitespace normalization across all 442 current `.zon`
titles yields one “The Cimmerian Nomad Encampment” title, owned by `nomads`
([manifest entry](../../../areas/AREA#L31),
[zone title](../../../areas/zon/nomads.zon#L2)). The tracked WLD and ZON paths
reach the August 2009 area-tree relocation. The 2015 ZON change updates only
the editor-version comment. No distinct predecessor or replacement appears in
the reviewed path history.

The broader source comparison keeps related names and maps distinct. The only
other current `.zon` title containing “Encampment” is “Orog Encampment” under
active `orogs` (*458–459), a separate title, source stem, and contributor. The
active Surface WLD has one room titled “Cimmerian Nomads” (#582495), with an
empty description and four exits to neighboring Surface map cells; it does
not represent a second Nomads zone. The adjacent Surface room #582494 is the
actual boundary room: its east exit reaches active Nomads #6221, whose west
exit returns to #582494 ([map label](../../../areas/wld/surface.wld#L1731739),
[Surface boundary](../../../areas/wld/surface.wld#L1731718),
[camp room](../../../areas/wld/nomads.wld#L430)).

The only other exact “Cimmerian” source-text reference found is a Savannah
sword description that calls Dodecimus a Cimmerian chieftain and names his
assassin ([object](../../../areas/obj/savannah.obj#L249)). This is a separate
artifact-history mention, not another camp map or a whole-zone replacement.
Global same-type scans also found all 49 Nomads room IDs, 21 mobile IDs, and
23 object IDs only in their `nomads` files. The tracked title, map, and prose
evidence identifies no alternate-stem predecessor or replacement; the
command-target owner question remains separate.

## Scope and disposition

Nomads remains an active `areas/AREA` contributor. Its four reciprocal Surface
pairs stay in the active ledger; the all-95 WLD scan found no source-room
overlap or target matches. The active source has no foreign exact QST or reset
references, while the two unregistered SHP candidates expose four room
locations through the conditional loader fallback. The Githzerai prime-shift
array and CHAOS starter table remain code-defined routes and supply. Dynamic
access, actual travel, shop operation, reward settlement, and persisted-player
outcomes remain unverified. No migration or gameplay decision follows from
this review.

This documentation-only review used source and static registers. It did not
run tests, a build, server/runtime/gameplay, database work, a migration, or an
operational script. The source-comprehensive audit queue is now **90/220
complete, 130 pending**; priority 91, The Ruins of Undermountain
(`undermountain`), is next.
