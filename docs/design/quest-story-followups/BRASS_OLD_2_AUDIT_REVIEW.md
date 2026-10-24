# Brass Old 2 Audit Review

Reviewed October 11, 2026, Checkpoint 455. This is a static source, history,
connectivity, and dependency review of the unregistered `brass-old-2` files.
It does not establish live access, persisted character/object identity,
editor ownership, or runtime behavior. No area or code source was changed.

## Source scope and identity

The current component contains only
[`brass-old-2.wld`](../../../areas/wld/brass-old-2.wld),
[`brass-old-2.mob`](../../../areas/mob/brass-old-2.mob),
[`brass-old-2.obj`](../../../areas/obj/brass-old-2.obj), and
[`brass-old-2.zon`](../../../areas/zon/brass-old-2.zon). Its ZON header is
zone 250, titled “Plane of Fire, Brass,” with last VNUM 25105 and reset mode
1. The component has 105 rooms (#25001–#25105), 13 mobile prototypes, and 19
object prototypes. It has no matching entry in `areas/AREA`, no current
`brass-old-2` QST, SHP, map, or story sidecar, and no source dossier of its
own. The [active Brass dossier](../zone-stories/PLANE_OF_FIRE_BRASS.md) belongs to the distinct active `brass` source.

The 105 WLD room descriptions, 13 mobile descriptions, and 19 object names and
descriptions were reviewed. Together they depict an Efreeti palace of brass
gates and flame, courts and halls, guard and training areas, an armory, slave
quarters and prison, flying-mount facilities, Padashaw's chambers, a harem and
scrying pool, an Amir's suite, a prayer tower and library, the Sultan's
throne room, treasure room, kitchen and dining room, and a magic tower and
Arch-Magi's laboratory. The mobile roster names Shadam Quilen as Sultan, along
with the Arch-Magi, High Priest, elite guards, and a captured slave described
as hostile after captivity. The objects include elemental keys, fiery arms
and armor, Doombringer, a crown, a ring of elemental control, and ornate
brass gloves. Room prose also names Kossuth. The roles Padashaw, Amir, and
Caliph are not consistently explained across room text; resolve that hierarchy
with the owner before a story rewrite. This source contains narrative setting
but no local QST or SHP file or story sidecar that establishes a quest, shop,
or active narrative contract.

## Followed history and the QST index

At the parent of commit `0e925fa9`, `areas/AREA` listed
`plane_fire_one *250-251`. That commit changes the listed active contributor to
`plane_fire_one *254-255` and adds `brass-old-2` as an unlisted source. The
new Old 2 WLD, MOB, OBJ, and ZON blobs are byte-identical to the former
`plane_fire_one` files at the parent revision:

| Source | Parent `plane_fire_one` blob | Added `brass-old-2` blob |
| --- | --- | --- |
| WLD | `07af804c644bb8234e425478bcb13a668ca8a121` | `07af804c644bb8234e425478bcb13a668ca8a121` |
| MOB | `dabf7721cb6de7560f61897880daf78187625e67` | `dabf7721cb6de7560f61897880daf78187625e67` |
| OBJ | `88af793b10a82ed095e0cdb05643fe1bd9bb94d4` | `88af793b10a82ed095e0cdb05643fe1bd9bb94d4` |
| ZON | `d11825147135f8cf34762091964243fa08c7eb42` | `d11825147135f8cf34762091964243fa08c7eb42` |

This identifies Old 2 as the preserved former 250-range `plane_fire_one`
source. It is distinct from current active `plane_fire_one` at *254–255,
from Old 1's +100 New Brass predecessor, and from Old 3's former
`plane_fire_two` source. The separate 254-range transplant is documented in
[Checkpoint 323 of the replacement review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-323-plane-of-fire-title-and-followed-history).
The same normalized ZON title across these files does not merge their
identities.

There is a related QST index discrepancy. `areas/qst/AREA` still lists
`plane_fire_one *250-251`, but the parent revision had no
`areas/qst/plane_fire_one.qst`. The current file at that path came from a
100%-identical rename of `plane_fire_three.qst` in the 2025 commit and begins
at giver #25401. Therefore the old QST index row is not evidence of a local
Old 2 QST file. The same index still names `plane_fire_two` and
`plane_fire_three` after their QST files were renamed. Reconcile the index
with its owner before using those labels to make a content or migration
decision.

## WLD graph and current inbound references

The old map has 105 rooms and 243 authored exits. All 243 targets resolve to
rooms in this WLD; none leaves the component. The local graph is one weak
component, and authored directed traversal from entry #25001 reaches all 105
rooms. This source-only graph does not evaluate closed/locked doors, resets,
live access, or runtime reachability. No negative target, fall-chance field,
current field, room outside the declared *250–251 range, or exit from a
range-mismatch room was found.

Seven active WLD records target Old 2 room #25087, “Treasure Room of the
Efreeti.” All are currently unresolved by the active world loader because
`brass-old-2.wld` is absent from `areas/AREA`; the unresolved-exit register
records that the normal build omits these targets and prunes the exits.

| Active source | Exact authored records to #25087 |
| --- | --- |
| `potions` | #30705 D1 east, D2 south, D3 west, D4 up, D5 down; #30706 D5 down |
| `realm` | #14210 D9 southeast |

These are source records from two active areas, not loaded Old 2 entrances.
No reciprocal active route or Old 2 outbound boundary is present. Preserve
all seven references pending the Potions, Realm, and Brass owners' decision
about a loaded destination or explicit retirement. See the [unresolved exit
register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L38), the [Potions audit](POTIONS_AUDIT_REVIEW.md#active-graph-and-authored-access),
and the [Realm audit](FAERIE_REALM_AUDIT_REVIEW.md#active-wld-graph-and-reset-door-reach).

Checkpoint 456 adds one more, separate source-only inbound edge: Old 3 room
#25341 D0 north targets Old 2 room #25001 (“Entry Way”). Old 2 still has no
outbound WLD boundary or reciprocal to #25341; this inactive-to-inactive
record does not change the seven active inbound references or candidate
active-inbound count. See the [Old 3 graph review](BRASS_OLD_3_AUDIT_REVIEW.md#wld-graph-and-source-only-routes)
and [World Connectivity, Checkpoint 456](WORLD_CONNECTIVITY.md#checkpoint-456-brass-old-3-connectivity-and-the-old-2-link).

## ZON prototype references and code consumers

The ZON contains 333 M/O/G/E reset commands: 279 M, 1 O, 40 G, and 13 E.
Of these, 263 refer to prototypes defined in Old 2's own MOB/OBJ files. The
remaining commands break down as follows:

| Source-only reset references | Count | Current source owner |
| --- | ---: | --- |
| M resets for #25100, #25110, #25120, #25130, #25140, #25150, #25160, #25170, #25180, #25190, #25200, and #25210 | 57 | `brass-old-3.mob` |
| E/G resets for #25101, #25110, #25117, #25118, and #25120 | 9 | `brass-old-3.obj` |
| O #60 | 1 | The active object set owns #60 in `heavens.obj`; other unlisted OBJ sources also define #60, so the old reset's intended item identity remains ambiguous |
| G #67245 | 1 | Active `unique.obj` |
| M #25019 | 2 | No current mobile prototype; #25019 exists in Old 2 only as an OBJ key |

The 66 Old 3 references are dependencies between two separate unlisted
components, not active resets. The two M #25019 commands at ZON lines 210 and
225 are typed as mobile resets; the same VNUM in Old 2 is an object key, so
the numeric match does not resolve the reset. Keep these source commands
unmodified until their builder identifies the intended mobile behavior.

The reciprocal reset dependency is now also documented: Old 3 has 30 M
commands for Old 2 mobile #25000. Together the two ZON files contain 96
cross-candidate reset commands in both directions. These source-only links
are separate from the seven active WLD references to Old 2 room #25087.
See the [Old 3 dependency review](BRASS_OLD_3_AUDIT_REVIEW.md#typed-references-reset-closure-and-code-consumers).

A bounded scan found no active ZON resets, active shop products, or typed QST
item references that resolve to Old 2's 19 object prototypes.
`areas/qst/halfcut.qst` has `R I 25000`, but Old 2 #25000 is a mobile and Old
2 defines no object #25000; this numeric overlap is not a typed Old 2
dependency. Old 2 has no SHP file or local QST file.

The code still contains direct references to this old VNUM range:

| Code reference | Old 2 identity | Static finding |
| --- | --- | --- |
| [`specs.assign.c:1052`](../../../src/specs/specs.assign.c#L1052) | Mobile #25000 | The “Plane of Fire One” block assigns `guild_guard` to this VNUM. |
| [`specs.assign.c:1958`](../../../src/specs/specs.assign.c#L1958) and [`specs.firep.c:163`](../../../src/specs/specs.firep.c#L163) | Object #25080, ring of elemental control | The assignment binds `ring_elemental_control`; Firep code also compares against this object VNUM. |
| [`specs.guards.c:230`](../../../src/specs/specs.guards.c#L230), [`specs.guards.c:231`](../../../src/specs/specs.guards.c#L231), [`specs.guards.c:435`](../../../src/specs/specs.guards.c#L435), [`specs.guards.c:438`](../../../src/specs/specs.guards.c#L438) | WLD rooms #25001 and #25086 | Room-number switches contain north-block and direction cases for these Old 2 rooms. |

These code references establish source-level dependencies, not that the
prototypes or rooms are loaded or that a runtime fault occurs. The
`randomeq.c` high-drop list also contains mobile #25040, but its only lookup
loop is commented out; it is not counted as an active consumer. The commented
object binding for #25030 is likewise not counted.

## Disposition

Keep `brass-old-2` **RETAIN IN PLACE**. The seven active exits depend on room
#25087; Old 2 and Old 3 have a one-way source-only WLD link plus 96 typed ZON
references in both directions; Old 2 also has two active object VNUMs, two
mobile resets with no current mobile owner, and source-code references. The
former source's identity is now clear, but source-list omission does not
resolve its story, code, editor, persisted-record, or owner status. No source
path move or deletion is cleared by this review. Do not edit Old 2, active
Fire/Brass, code, or QST index data as part of this record.

The source history, links, WLD exits, and typed references are static
findings. Player/account records, persisted VNUM consumers, live gameplay,
editor/map-generator projects, and runtime behavior were not inspected.
See the [21 builder questions](BRASS_OLD_2_BUILDER_QUESTIONS.md).
