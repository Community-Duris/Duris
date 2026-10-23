# Nakral's Crypt audit supplement

**Checkpoint 44 — bounded repository-wide connectivity, typed-reference,
code-reference, and tracked-path-history review.** The comprehensive zone
source map remains
[NAKRALS_CRYPT.md](../zone-stories/NAKRALS_CRYPT.md). This supplement records
the additional cross-area findings without changing the native area sources.

## Active WLD boundary and unresolved exit

The active Crypt map has one paired cross-area boundary: room #14300 exits
west to Undead Outpost #98737, whose east exit returns to Crypt #14300
([Crypt exit](../../../areas/wld/crypt.wld#L15),
[Crypt target](../../../areas/wld/crypt.wld#L18),
[Outpost exit](../../../areas/wld/unoutpst.wld#L784),
[Outpost target](../../../areas/wld/unoutpst.wld#L787),
[Crypt outbound ledger row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L265),
[Outpost return row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1181)). This is the
one active inbound and one active outbound record in the Crypt coverage row.

Crypt room #14568 also authors an up exit to #265588
([source](../../../areas/wld/crypt.wld#L5954),
[unresolved-exit ledger row](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L39)). The
destination is absent from the active room registry, so loader resolution
prunes the edge. The register records matching old `surf`, `surface2011`, and
`underdark2011` sources. In the old `surf.wld` body, #265588 exits down to
#54500 rather than returning to Crypt
([old room](../../../areas/wld/surf.wld#L1376276),
[down target](../../../areas/wld/surf.wld#L1376299)). This remains an
unresolved authored reference, not an active route or a basis for choosing a
replacement endpoint; see the full
[Crypt source map](../zone-stories/NAKRALS_CRYPT.md).

## Source-only WLD references into Crypt

A stateful parse of all 95 current unregistered WLD inputs read 2,080,773
authored `D0`–`D9` exits without parse issues and found two exits into active
Crypt room #14300:

| Unregistered source | Authored exit | Evidence |
| --- | --- | --- |
| `surf.wld` room #276166, “Burnt Plains of a Fallen Realm” | North (`D0`) to #14300 | [room](../../../areas/wld/surf.wld#L1598266), [north exit](../../../areas/wld/surf.wld#L1598270), [target](../../../areas/wld/surf.wld#L1598273) |
| `underworld2.wld` room #422044, “Dried Lava of Lost Souls” | North (`D0`) to #14300 | [room](../../../areas/wld/underworld2.wld#L41110), [north exit](../../../areas/wld/underworld2.wld#L41114), [target](../../../areas/wld/underworld2.wld#L41117) |

Neither source has a room VNUM in Crypt's active range, so neither record is
an overlapping source-room map record. Both exits remain **source-only**, not
active-world ingress: their WLD files are outside `areas/AREA`, and their
targets do not add active edges. These two records bring the curated source-only
inbound list to 581; the complete table and prior records are in
[WORLD_CONNECTIVITY.md](WORLD_CONNECTIVITY.md).

## Code-defined and typed references

The Phantom racial innate registers `shift_prime`
([registration](../../../src/classes/innates.c#L730)). Its handler rejects
combat, requires the character to be in the Astral zone, and checks the innate
cooldown. The Phantom random destination list includes active Crypt room
#14373 ([destination list](../../../src/classes/innates.c#L2012),
[active room](../../../areas/wld/crypt.wld#L1769)); when invoked, the handler
selects a valid listed room and moves the character there
([selection](../../../src/classes/innates.c#L2051),
[room transfer](../../../src/classes/innates.c#L2068),
[destination transfer](../../../src/classes/innates.c#L2070)). This is a static
code-defined dynamic-route candidate outside the ordinary WLD census. The
declaration does not establish runtime availability, successful arrival, or a
safe return.

The C/C++ exact VNUM scan also found local object #14479, the segmented iron
necklace ([object](../../../areas/obj/crypt.obj#L1781)), in seven static
`chaos_kit_item` equipment arrays: standard Reaver, Avenger, and Dragoon, and
enhanceable Reaver, Dreadlord, Avenger, and Dragoon
([table type](../../../src/account/chaos_eq_data.h#L9),
[standard Reaver](../../../src/account/chaos_eq_data.h#L251),
[standard Avenger](../../../src/account/chaos_eq_data.h#L291),
[standard Dragoon](../../../src/account/chaos_eq_data.h#L316),
[enhanceable Reaver](../../../src/account/chaos_eq_data.h#L495),
[enhanceable Dreadlord](../../../src/account/chaos_eq_data.h#L519),
[enhanceable Avenger](../../../src/account/chaos_eq_data.h#L535),
[enhanceable Dragoon](../../../src/account/chaos_eq_data.h#L559)). Static table
membership does not prove kit selection, creation, or issuance.

Bounded exact scans of external native QST, ZON reset, SHP, and structured
`areas/story/*.json` numeric prototype references found no references to local
Crypt prototypes. The five recipes touching local items are Crypt-owned, and
no foreign reset source for local material was identified. The zone dossier's
scan of all 713 active type-25 teleport objects found no foreign portal
destination into Crypt; the identified local orb #14547 targets #14386.
Crypt also has a single literal
local `inn` assignment for cabin #14362
([assignment](../../../src/specs/specs.assign.c#L2331)). The exact scan does
not close indirect code, editor/tooling, saved-player, or runtime dependencies.

## Title and tracked WLD history

The normalized active `.zon` title scan finds no repeated title for Nakral's
Crypt, whose active owner is `crypt` in zone 143
([active manifest](../../../areas/AREA#L79),
[zone title](../../../areas/zon/crypt.zon#L2)). Following
`areas/wld/crypt.wld` history found that commit `5794d8067` temporarily changed
room #14300's west endpoint from #98737 to #8604. Commit `71d731c5b` later
restored #98737. An exact `#8604` room-record scan of the tracked WLD corpus at
the first commit's parent found no definition for that destination. This is a
temporary, unresolved endpoint change and restoration; it does not show a
replacement zone or qualify #8604 as a working route. Followed WLD, MOB, OBJ,
ZON, and QST paths remain on the `crypt` stem from the 2009 area-tree move;
these bounded histories show no alternate-stem predecessor, retirement, or
whole-area replacement.

## Scope and disposition

The Crypt remains an active `areas/AREA` contributor and is not a move or
archive candidate. Its authored WLD census remains 653 exits: 652 targets
resolve, one active boundary enters, one leaves, and those two records are
reverse-paired; the stale #14568→#265588 edge remains the single unresolved
positive target. Actual reachability, dynamic-route behavior, door/reset state,
safe access and return, persisted-player effects, and broader typed closure
remain unverified. This documentation-only review did not run a build, tests,
server, gameplay, database, migration, or operational script.
