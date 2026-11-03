# Buildings repository-wide audit supplement

This checkpoint audits `buildings` as an active room-template and outpost
support area. The reviewed native-source snapshot is
`72ba3deff3bb30bde880018cdebe7c6db675b697`; the current worktree has no
`areas/` or `src/` changes from that snapshot. No native area, gameplay, or
persisted data is changed.

## Active source identity and story scope

The active manifest entry is [`buildings` at zone 978](../../../areas/AREA#L352);
`areas/qst/AREA` also lists it at
[line 311](../../../areas/qst/AREA#L311), but there is no local `.qst`, Q/QA
contract, `.shp`, or story sidecar. The source has 100 WLD rooms (#97800–#97899),
two mobiles (#97800–#97801), four objects (#97800, #97801, #97820, #97821),
and zero ZON reset commands.

The rooms are identical empty “Inside a building” shells. The local prototypes
describe an outpost tower and guard, rubble, a destination-text vortex, and two
tree/resource props. There is no independent local quest or environmental
story arc. See the [source-story dossier](../zone-stories/BUILDINGS.md) for the
inventory and builder resolutions.

## Static WLD connectivity and runtime routes

The active WLD graph has 100 rooms and no authored exits. Each room is a
singleton weak component; from any template room, raw WLD reach and return
cover only that room. No active area has a WLD exit into #97800–#97899, and no
active Building WLD exit leaves the range. A whole-WLD source sweep found no
unregistered source-only exit into these rooms. These results describe the
static files, not the code-generated outposts.

The normal non-mini startup path calls `init_outposts()`. Its loader expects
three persisted outpost records and attempts to instantiate one `Building`
for each. The building allocator searches the reserved room range; its
generator changes a selected room name and creates a down exit to the
outpost's exterior location. The outpost mobile's `CMD_ENTER` handler moves
an authorized player into that room. These dynamic edges do not appear in the
authored exit ledgers. Current record contents, live permissions, and player
traversal were not inspected or tested.

The three source-defined exterior anchors resolve in the active `surface.wld`:

| Outpost slot | Code direction | Active exterior source room | Runtime relation |
| --- | --- | --- | --- |
| UC | east | [Surface #588184, Burnt Plains of a Fallen Realm](../../../areas/wld/surface.wld#L1851237) | Outpost mobile at this location can transfer an authorized entrant into its generated interior; generated room has a down exit to the exterior. |
| KK | west | [Surface #628856, Grasslands of Khomani-Khan](../../../areas/wld/surface.wld#L2705433) | Same code-generated relation. |
| IC | south | [Surface #531100, partially frozen forest](../../../areas/wld/surface.wld#L652915) | Same code-generated relation. |

When a persisted outpost has portals enabled, C++ creates the shared portal
prototype at the generated interior and the guildhall hearthstone room, then
sets their target room values. The travel handler recognizes this object by
VNUM. These are optional, runtime-assigned links, not authored WLD edges.

`initialize_buildings()` is commented out in startup, but `init_outposts()` is
called in the normal non-mini path. The header's older “not attached” comment
therefore does not establish that the VNUMs are unused. The runtime outpost
loader and generator are direct current source consumers.

## Inbound and outbound dependency closure

Repository-wide typed scans found no external active or unregistered WLD exit
into the Buildings room range, no external ZON reset-role reference to its
room/mobile/object IDs, no exact numeric line hit in other MOB/OBJ files, and
no external duplicate mobile or object prototype definition. Exact line scans
found no external QST, SHP, or story-sidecar ID hits. External type-25 portal
and type-29 object declarations have no target matching the local room,
mobile, or object VNUMs. The current external ZON data uses D/E/F/G/M/O/P/R;
A/B/C/Y command rows are absent.

The principal dependencies are compiled code:

- `buildings.h` reserves rooms #97800–#97899 and names the rubble, portal,
  outpost-tower, and guard prototypes
  ([range and prototype constants](../../../src/world/buildings.h#L17)).
- `buildings.c` allocates from that room range, adds the generated down exit,
  loads the tower mobile, and uses portal object #97801
  ([allocator](../../../src/world/buildings.c#L41),
  [room generator](../../../src/world/buildings.c#L567),
  [portal generation](../../../src/world/buildings.c#L720)).
- `outposts.c` defines three Surface anchors, loads persisted records, reads
  rubble object #97800, and creates guard mobile #97801
  ([anchors](../../../src/world/outposts.c#L77),
  [record loader](../../../src/world/outposts.c#L275),
  [rubble creation](../../../src/world/outposts.c#L1038)).
- The mob-special table binds the two mobile prototypes, the `outpost` command
  is registered, and travel code recognizes portal #97801
  ([mobile bindings](../../../src/specs/specs.assign.c#L1130),
  [command registration](../../../src/cmd/interp.c#L3448),
  [portal handling](../../../src/magic/spell_travel.c#L1017)).
- `outposts.h` still declares resource prototype IDs #97820 and #97821, but
  current source references are declarations only; `init_outpost_resources()`
  is not called by `init_outposts()`.

A raw source scan also found comment-only mentions of #97810 in
`kingdom_guards.c` and a commented-out object-proc assignment for #97800.
They are not executable consumers. Exact typed closure of editor data,
external tooling, persisted outpost records, and live dynamic state remains
unverified. No database or player data was read.

## Title, history, and disposition

The normalized active `.zon` title “Buildings” has one owner among the 351
active contributors. Followed WLD history begins at the 2009 repository
source import; MOB, OBJ, and ZON history retains the `buildings` stem through
later outpost additions and same-path maintenance. The reviewed tracked paths
show no alternate-stem rename, distinct predecessor, or whole-zone
replacement. The Human Outpost, Undead Outpost, and Newbie Outpost are
separately named areas, not duplicate copies of this shared template source.

The outpost code's KK anchor is active Surface #628856. Unregistered
[`newbiemaps.wld` #628856](../../../areas/wld/newbiemaps.wld#L605044) and
[`surface2011-temp.wld` #628856](../../../areas/wld/surface2011-temp.wld#L605977)
also contain that number with different room text; neither source is in
`areas/AREA`. This numeric map parallel does not make either file an alternate
Buildings source or establish replacement lineage.

Keep active `buildings` **DO NOT MOVE**. Removing its room templates from the
full world list would leave the runtime allocator unable to resolve the
reserved range, so the intended interiors could not be generated. The current
`load_building()` wrapper returns a `Building*` even when `is_loaded()` is
false, and `load_outposts()` checks only that pointer; source review alone does
not prove the exact startup consequence of missing rooms. A future retirement
proposal must first reconcile the outpost command, startup loader,
room/prototype consumers, and persisted records with the feature owner. Do not
add an attic candidate row while it remains an active code-owned contributor.
