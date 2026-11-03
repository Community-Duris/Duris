# Jindon the Deathwood Forest: repository-wide route, dependency, and history supplement

This bounded static supplement accompanies the [comprehensive Jindon source
dossier](../zone-stories/JINDON_DEATHWOOD.md). It records active and
source-only WLD edges, raw directed reach, selected typed references, title
ownership, and bounded same-stem history. Static source review does not
establish that a player can traverse a route or complete an interaction.

## Active WLD boundaries and directed reach

The active area is `jin`, zone 820, with 90 rooms from #82000 through #82089.
The source dossier's former printed lower bound, #81678, was a typo; the raw
room records and coverage inventory both begin at #82000. Its 193 authored
exits all resolve: 192 are internal and one leaves the area. The local graph
has weak components of 65 and 25 rooms. Authored directed traversal from
#82000 reaches 65 rooms, and 65 rooms have a directed path back to #82000.
The remaining component is #82065–#82089; those rooms are absent from both
root-forward and root-return WLD reach. This is a raw graph result, not a
claim about runtime access.

The only active boundary is reciprocal with Surface: Surface #619653 north to
Jindon #82063, and Jindon #82063 south to Surface #619653
([pair rows 212 and 523](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L212), [Surface
exit](../../../areas/wld/surface.wld#L2512122), [Jindon exit](../../../areas/wld/jin.wld#L1279)).

The ticket-master procedure has a separate code-driven movement. The compiled
assignment binds mobile #82000 to `jindo_ticket_master`
([assignment](../../../src/specs/specs.assign.c#L1225)); its handler contains
a direct transfer to room #82062 after its legacy transaction path
([handler](../../../src/specs/specs.jin.c#L72)). That code path is not a WLD
edge and does not enter the isolated #82065–#82089 component. This static
inspection does not qualify an in-game purchase or transfer.

## Inactive WLD sweep

All 95 current WLD sources outside the active areas/AREA list parsed cleanly.
There is no source-room VNUM overlap with Jindon. One source-only exit targets
the active area: `surf.wld` room #244020 north to #82063
([source room](../../../areas/wld/surf.wld#L923644)). It is not an active
Surface ingress. Curated source-only totals rise to 3,334 overall, 165 for
the reviewed Surf subset, and 15 for `ud_map2`.

## Exact typed and structured references

The selected scans found no exact QST item command referring to Jindon's
objects. The apparent `#82067` match in White Harbor's sidecar is part of the
string story ID `request-55200-e82067b8d7c6`, at `$.stories[155].id`; it is
not a room, mobile, or object VNUM reference
([sidecar row](../../../areas/story/wh.story.json#L1024)). No external
duplicate mobile or object prototype and no external type-25 target into
Jindon was found.

Two reset records reference local objects across source boundaries:

| Source | Typed reference | Status |
| --- | --- | --- |
| Active Otower | G reset of object #82018 at room #11331 | Active reset source; #82018 is Jindon's psionicist power locket ([reset](../../../areas/zon/otower.zon#L162), [object](../../../areas/obj/jin.obj#L249)). |
| Unregistered Nexus56 | G reset of object #82011; SHP #83032 lists #82011 as a product | Source-only ZON/SHP references; #82011 is Jindon's delicious onion. Nexus56 is not active AREA input, and the checkout has no `areas/world.shp`, so fallback shop parsing is conditional and does not establish an operating shop ([reset](../../../areas/zon/nexus56.zon#L1325), [SHP product](../../../areas/shp/nexus56.shp#L250), [object](../../../areas/obj/jin.obj#L148), [candidate row 79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79)). |

The CP241 full-catalog recheck parsed 445 MOB, 442 OBJ, 265 QST, 180 SHP,
442 ZON, and 232 structured story files. It found no duplicate Jindon mobile
or object prototype in another current catalog, and no external QST `I`
command or active SHP product referencing a Jindon object. Nexus56 remains the
single source-only SHP product record; the checkout has no `areas/world.shp`,
so it does not establish an operating fallback shop. The full reset-role scan
found only the active Otower object #82018 import and inactive Nexus56 object
#82011 import described above; no Jindon reset imports a currently defined
foreign prototype. All 231 outside story sidecars parsed without errors and
none contains an exact numeric room/mobile/object reference into Jindon.

The 872 current type-25 declarations were rechecked for Jindon destination and
random-zone seed values; none points into its rooms. An exact-token C/C++ scan
found only the area's own compiled references: mobile #82000 is assigned to
`jindo_ticket_master`, which transfers the player to #82062 after its legacy
transaction path. These do not establish external item ownership or a WLD
boundary. The false #82067 White Harbor string-ID match remains non-typed.

## Title and bounded history

The normalized title “Jindon the Deathwood Forest” belongs only to active
`jin` among the 442 scanned `.zon` files
([title source](../../../areas/zon/jin.zon#L2)). The bounded histories of all
six current WLD/MOB/OBJ/QST/SHP/ZON paths remain under the `jin` stem after
their tracked moves and edits; no alternate-stem rename, distinct
predecessor, or whole-zone replacement was established. Semantic comparison
reviewed the separately titled Dark Forest, Dark and Twisted Wood, Forest of
Mir, Haunted Woods, and Mosswood contributors ([Dark Forest](../../../areas/zon/dark_forest.zon#L2),
[Twisted Wood](../../../areas/zon/twstwd.zon#L2), [Forest of Mir](../../../areas/zon/mir.zon#L2),
[Haunted Woods](../../../areas/zon/hwood.zon#L2), [Mosswood](../../../areas/zon/moss.zon#L2)).
Their forest theme does not establish duplicate room ownership or replacement;
the Jindon dossier's carnival, exhibits, Sirax return, ticket admission, and
Deathwood routes remain separately sourced. The reciprocal Surface edge and
source-only Surf approach are connections, not supersession evidence. See
the [duplicate and replacement review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-241-jindon-title-history-and-semantic-comparison).

## Migration disposition

Keep active `jin` DO NOT MOVE. Preserve the reciprocal Surface boundary, the
separate ticket-master code reference, active Otower's object #82018 reset,
the source-only Surf approach, and the Nexus56 source-only #82011 reset/shop
reference. Keep `surf` and `nexus56` RETAIN IN PLACE, retaining their prior
candidate dependencies. The string-only White Harbor story ID is not a typed
VNUM dependency. No new candidate row is added. Editor, administrative,
installed-data, persisted-player, indirect-runtime, and actual traversal
closure remain outside this bounded scan; no gameplay, migration, or source
change is proposed.
