# The Haunted Woods: source audit supplement

This supplement records the static connectivity, source-only approach, typed
dependencies, title and tracked-history review for active contributor `hwood`
at source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. The new
[environmental story dossier](../zone-stories/THE_HAUNTED_WOODS.md) covers its
forest maze, spider web, stranded Orc scout, river, gnoll village and Oak Tree
Inn.

## Static connectivity and reachability

The active WLD contains 58 rooms and 157 authored exits: 156 local targets and
one outbound Surface edge. All targets resolve. There are no negative targets,
unresolved positive targets, source rooms outside the zone interval, or local
current/fall records. Its weak graph has one 58-room component; its directed
graph has a 57-room strongly connected component and a singleton, room #10835.

From the active Surface arrival, room #10800, 57/58 rooms are reachable by
authored local WLD exits: only #10835 is outside forward reach. All 58 rooms
have authored WLD paths back to #10800. Room #10835 is titled “Mob Load Room”; six
exits lead from it into the forest. The reset file places the roaming wildlife
there, while the room has no authored local entry from the Surface-seeded
component. This is a source graph and reset configuration, not proof of where
players or moving mobiles can actually travel.

The only active boundary is reciprocal: Hwood #10800 south exits to Surface
#517668, and Surface #517668 north returns to #10800. See [cross-area rows
205 and 1347](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L205) and [pair summary
197/518](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L197), plus the [Hwood exit](../../../areas/wld/hwood.wld#L19)
and [Surface return](../../../areas/wld/surface.wld#L370944).

### Source-only inactive WLD approach

The scan of the other 445 WLD files (350 other active contributors and 95
inactive sources) found one additional inbound approach and no room-VNUM
overlap. Inactive `surf.wld` room #209648 exits north to Hwood #10800. It is
source-only, not active ingress. Keep it in the existing Surf candidate record,
row 97, and do not create an Hwood candidate. The [Surf WLD target](../../../areas/wld/surf.wld#L202471)
and [updated Surf disposition](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97)
record this dependency.

## Typed dependencies and compiled service

The all-source typed scan found these cross-area uses:

| Source | Hwood reference | Classification |
| --- | --- | --- |
| Active Surface WLD | #517668 north to Hwood #10800 | The reciprocal active WLD return. |
| Inactive Surf WLD | #209648 north to Hwood #10800 | One source-only approach; it remains with Surf and is not active ingress. |
| Hwood ZON and SHP | Heavens objects #363, #368 and #369 are reset onto shopkeeper #10819 and listed as products in the general shop. | Active imported medical supplies: gauze bandage, gauze tourniquet and large medicated bandage. Preserve the active Heavens prototypes while the shop remains. |
| Compiled room table | Room #10857 is assigned the `inn` procedure. | Active source-level inn binding to the reception room, not a verified player check-in journey. |

Evidence for the imported products is in the [Hwood resets](../../../areas/zon/hwood.zon#L157),
[shop stock](../../../areas/shp/hwood.shp#L38), and active
[Heavens prototypes](../../../areas/obj/heavens.obj#L3783); the compiled room
assignment is [here](../../../src/specs/specs.assign.c#L2310). The second local
shop is the river merchant at #10809, whose two products are Hwood canoe #10802
and raft #10803 ([shop definition](../../../areas/shp/hwood.shp#L1)).

No external ZON reset, QST item/giver reference, SHP product or active type-25
portal destination points to Hwood's local prototype ranges. No Hwood QST file
or story sidecar exists. There is no direct compiled reference to a Hwood
mobile/object VNUM. One raw numeric object-value scan finds #10834 in the
values of Evkeep potion #44821; that prototype is item type 10, not a
type-25 portal, and does not create a room route
([potion](../../../areas/obj/evkeep.obj#L316)). These checks cover
checked-in typed source, not editor state, persisted items or live behavior.

## Title, history and disposition

After color-code removal, “The Haunted Woods” occurs once among the 442 current
`.zon` files. “Foggy Woods,” “The Forgotten Forest,” “Dreggan Woods,”
“Neverwinter Woods” and “The Deathwood Forest” have separate current
contributors; no source evidence identifies one as an explicit Hwood
replacement. Followed `hwood` WLD/MOB/OBJ/ZON history stays on the same stem
through the 2009 `areas/` tree move and later edits. There is no alternate-stem
predecessor, title rename or explicit whole-area replacement. No local QST
history exists because there is no local QST file.

Keep active `hwood` **DO NOT MOVE**. No Hwood attic candidate row is warranted.
Preserve the active reciprocal Surface route, the inactive Surf approach in
Surf's candidate row, Heavens' medical stock, and the compiled room #10857 inn
binding. The source-only Surf total rises by one to 227; the curated total
rises to 3,469. `ud_map2` remains 18 and Underworld2 remains eight. Actual
door behavior, mobile movement, boat availability, shop stock, inn operation
and travel safety were not tested.
