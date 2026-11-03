# Desolate Under Fire audit supplement

**Checkpoint 52 — active and source-only connectivity, exact typed and code
references, title, and tracked-path history.** The existing
[comprehensive source map](../zone-stories/DESOLATE_UNDER_FIRE.md) remains the
authority for local contracts, phase behavior, sources, controls, portals,
resets, and the previously repaired trial directions. This supplement records
bounded cross-area evidence without changing native sources.

## Active WLD boundary and legacy target identity

The active WLD census has 346 authored exits, of which 345 resolve in the
current world lookup. The local graph has four weak components, the largest
with 149 rooms; six rooms have fall fields and none has a current field. The
one positive-unresolved exit is Desolate Under Fire room #77301 south to VNUM
#217609 ([source](../../../areas/wld/desolateinv.wld#L16),
[target tuple](../../../areas/wld/desolateinv.wld#L19),
[unresolved ledger row](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L260)). That target
does not resolve to a contributor in the current `areas/AREA` world.

The same target VNUM appears as three different source rooms in unregistered
WLD files:

| Source-only file | Room identity and north exit | Source lines |
| --- | --- | --- |
| `surf.wld` | “The Road to Desolate”; north points to #22200. | [room](../../../areas/wld/surf.wld#L369445); [north target](../../../areas/wld/surf.wld#L369452) |
| `surface2011.wld` | “Lush Plains Covered with Shrugs and Small Trees”; north points to #217309. | [room](../../../areas/wld/surface2011.wld#L369790); [north target](../../../areas/wld/surface2011.wld#L369797) |
| `underdark2011.wld` | A large mossy cave; north points to #217309. | [room](../../../areas/wld/underdark2011.wld#L67554); [north target](../../../areas/wld/underdark2011.wld#L67561) |

All three files are outside the un-commented `areas/AREA` list. The unresolved
register records that the normal build omits these sources and `renum_world`
prunes the exit. The matching VNUMs do not identify the intended map owner or
prove an active route; keep this boundary unresolved pending source and map
selection.

A stateful parse of all 95 current unregistered WLD files read 2,080,773
authored exits with zero parse issues. None of their 527,791 source rooms
reuses a Desolate Under Fire room VNUM, and no unregistered exit target matches
a Desolate Under Fire room. No source-only ingress candidate was found. The
curated source-only follow-up total remains 586; the individually reviewed
`surf.wld` subset remains 26. This reverse scan does not resolve the separate
active exit target #217609.

## Bounded typed and code references

The exact external typed scan found one active reset group reference:

| Source | Exact reference | Evidence boundary |
| --- | --- | --- |
| SurfaceKeeps reset | Two `G` commands give object #77366 to hunter mobile #120046 in room #122629 ([first](../../../areas/zon/surfacekeeps.zon#L453); [second](../../../areas/zon/surfacekeeps.zon#L454); [object](../../../areas/obj/desolateinv.obj#L992)). | #77366 is the leaf-and-bark potion, distinct from local shop potion #77356. This foreign reset is not proof of live supply or transaction availability. |

No external QST, SHP, or other story-sidecar numeric VNUM reference was found.
A structured scan of the other 231 story sidecars parsed without errors and
found no exact room, mobile, or object VNUM value. The exact C/C++ literal scan
found only the local inn assignment for room #77442
([assignment](../../../src/specs/specs.assign.c#L2306)); no external code literal
matched a local room, mobile, or object VNUM. The comprehensive dossier's scan
of 713 active type-25 objects found seven incoming Desolate Under Fire
destinations, each declared by a local object, and no foreign portal
destination into this area.

Tracked code history records removal of room #77422 from the Githzerai
`shift_prime` destination list because the zone might not be loaded and could
strand a shifted character ([2010 source change](https://github.com/Community-Duris/Duris/commit/9d8406a896457b6a31c5bed34159d79cfd5f1b4d)).
That historical route is absent from the current direct-literal scan and is not
an active arrival path.

## Active title and tracked history

Color-code stripping, whitespace collapse, and case folding across all 442
current `.zon` titles yield one exact “Desolate Under Fire” owner:
`desolateinv` ([manifest entry](../../../areas/AREA#L278),
[QST entry](../../../areas/qst/AREA#L248),
[zone title](../../../areas/zon/desolateinv.zon#L2)). The exact title “Desolate”
belongs separately to active `desolate` at zone 222–223
([AREA](../../../areas/AREA#L112),
[QST AREA](../../../areas/qst/AREA#L105)). Followed WLD, MOB, OBJ, ZON, QST,
and SHP paths for `desolateinv` remain on that stem from the August 2009
area-tree move. The story sidecar was added on 2026-10-05.

Later history records the 2010 initial closed-zone flag, the 2015 quest-item
flag overhaul, and the 2026 two-direction text correction already documented
in the comprehensive dossier. The 2010 removal of room #77422 from the
`shift_prime` destination list addressed possible stranding when the zone is
not loaded; it does not indicate area retirement or rename. No alternate-stem
predecessor, retirement, or whole-area replacement is identified. The normal
Desolate dossier treats the invasion-phase relationship as a separate world
event and owner question; the separately registered `desolate` source is not
treated as a replacement.

## Scope and disposition

Desolate Under Fire remains an active `areas/AREA` contributor. Its static WLD
boundary to #217609 remains unresolved because that number has conflicting
source identities in three unregistered WLD files; no direct active cross-area
pair is recorded. The all-95 WLD scan found no source-room overlap or target
match to local rooms. The only exact external typed prototype reference is
SurfaceKeeps' two resets for local potion #77366; the bounded QST, SHP,
sidecar, and external code scans found none. Actual entry, map identity, safe
travel, reset supply, phase behavior, and persisted-player outcomes remain
unverified. No migration or gameplay decision follows from this review.

This documentation-only review used source and static registers. It did not
run tests, a build, server/runtime/gameplay, database work, a migration, or an
operational script. The source-comprehensive audit queue is now **92/220
complete, 128 pending**; priority 93, Storm Port Stronghold (`spshold`), is
next.

## Checkpoint 787: Divine Home reset import

Active `desolateinv.zon` G row 477 imports Divine Home object #40768
([reset](../../../areas/zon/desolateinv.zon#L477)). Preserve Divine Home as the current
prototype owner; this declaration does not establish runtime admission or availability.
See the [Divine Home typed audit](DIVHOME_AUDIT_REVIEW.md#active-external-consumers).
