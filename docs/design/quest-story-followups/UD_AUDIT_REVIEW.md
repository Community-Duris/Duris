# ud repository-wide audit supplement

Canonical key: `component:ud`. Coverage data row 460 / CSV line 461;
candidate dependency data row 109 / CSV line 110. Reviewed October 12, 2026
(Checkpoint 547) against checkout `c500ac51b8cd26540943257e7ca4e9807263ef74`.
This is a static source, graph, dependency, identity, and followed-history
review. It does not establish that the map is loaded or that any source
direction or reset executes at runtime.

## Source inventory and narrative review

The only same-stem files are [ud.wld](../../../areas/wld/ud.wld#L1) and
[ud.zon](../../../areas/zon/ud.zon#L1). The ZON header is #300245, titled
“Underworld of Duris,” with top VNUM #385443 and reset mode 0; line 4 contains
`300 300` before the terminator. There is no ud MOB, OBJ, QST, SHP, map JSON,
or story sidecar. The stem is absent from both [areas/AREA](../../../areas/AREA)
and [areas/qst/AREA](../../../areas/qst/AREA).

The WLD has 17,462 unique rooms from #304052 through #385443, all within the
ZON header’s stated numeric range. Every room description is empty. The eight
room-title groups are “The Ocean” (11,241), “Zone Entrance” (2,488), “Slime”
(1,602), “Mountains” (1,385), “Arctic” (597), “Scorching-Hot Lava” (106),
“An Ancient Stone Road” (27), and “Rock-Filled Cavern” (16). This is terrain
and location labeling, with no room-description narrative in the checked
source.

## Authored WLD graph

The 288,474-line WLD contains 50,291 exit records: D1/east 16,424, D2/south
16,899, and D3/west 16,968. There are no D0/north or D4–D9 records. Of these
directions, 50,018 target another room in ud.wld. The other 273 positive
targets are absent from every current WLD source; they contain 248 distinct
target VNUMs, and all 273 start in rooms titled “The Ocean.” Examples are
[#304123 east to #304124](../../../areas/wld/ud.wld#L150),
[#307117 south to #307417](../../../areas/wld/ud.wld#L9663), and
[#307420 west to #307419](../../../areas/wld/ud.wld#L11136). The source does
not establish whether these are intended map-edge gaps, omitted cells, or
stale targets. Some missing destinations repeat: #385040 is named by three
directions from source rooms #384740, #385039, and #385041
([south](../../../areas/wld/ud.wld#L284773),
[east](../../../areas/wld/ud.wld#L286374),
[west](../../../areas/wld/ud.wld#L286399)).

When local exits are treated as undirected adjacency for component counting,
the source graph has 11 weak components: 7,143; 5,978; 3,863; 175; 126; 70;
53; 51; and three single-room components. The largest three are separate
blocks, not a demonstrated travel itinerary. No start room or directed
reachability claim is assigned. There are no F fall-chance or C current
directives in this WLD. No source room falls outside its ZON number range.

No ud room VNUM overlaps any other WLD source in the checkout. No ud-authored
exit targets an active room or any other current WLD room; the 273 missing
targets do not resolve elsewhere in the source tree. Thus this component has
no source-backed active outbound WLD edge and no recorded return edge.

## Active WLD targets into ud

The existing [candidate dependency row 109](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L110)
and [unresolved-exit register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L59)
already record 18 active exits from mobs_underdark. This audit confirms that
all 18 target 18 unique rooms found only in ud.wld. They fall in three of the
map’s weak components; their descriptive titles do not establish functioning
entrances.

| Active source exit | ud target record | Target title | ud component |
| --- | --- | --- | --- |
| [#24002 north](../../../areas/wld/mobs_underdark.wld#L29) to [#312785](../../../areas/wld/ud.wld#L40616) | #312785 | Ocean | #304052, 5,978 rooms |
| [#24002 east](../../../areas/wld/mobs_underdark.wld#L33) to [#313988](../../../areas/wld/ud.wld#L47813) | #313988 | Ocean | #304052, 5,978 rooms |
| [#24002 west](../../../areas/wld/mobs_underdark.wld#L41) to [#313077](../../../areas/wld/ud.wld#L42155) | #313077 | Ocean | #304052, 5,978 rooms |
| [#24003 north](../../../areas/wld/mobs_underdark.wld#L54) to [#313092](../../../areas/wld/ud.wld#L42410) | #313092 | Ocean | #304052, 5,978 rooms |
| [#24003 east](../../../areas/wld/mobs_underdark.wld#L58) to [#314295](../../../areas/wld/ud.wld#L49749) | #314295 | Ocean | #304052, 5,978 rooms |
| [#24003 south](../../../areas/wld/mobs_underdark.wld#L62) to [#316094](../../../areas/wld/ud.wld#L60770) | #316094 | Ocean | #304052, 5,978 rooms |
| [#24003 west](../../../areas/wld/mobs_underdark.wld#L66) to [#313991](../../../areas/wld/ud.wld#L47864) | #313991 | Ocean | #304052, 5,978 rooms |
| [#24007 west](../../../areas/wld/mobs_underdark.wld#L182) to [#339932](../../../areas/wld/ud.wld#L104941) | #339932 | Ocean | #337526, 7,143 rooms |
| [#24010 north](../../../areas/wld/mobs_underdark.wld#L253) to [#350071](../../../areas/wld/ud.wld#L161403) | #350071 | Zone Entrance | #346458, 3,863 rooms |
| [#24010 east](../../../areas/wld/mobs_underdark.wld#L257) to [#351873](../../../areas/wld/ud.wld#L173909) | #351873 | Zone Entrance | #346458, 3,863 rooms |
| [#24010 south](../../../areas/wld/mobs_underdark.wld#L261) to [#352770](../../../areas/wld/ud.wld#L180506) | #352770 | Zone Entrance | #346458, 3,863 rooms |
| [#24010 west](../../../areas/wld/mobs_underdark.wld#L265) to [#352166](../../../areas/wld/ud.wld#L176012) | #352166 | Zone Entrance | #346458, 3,863 rooms |
| [#24010 northwest](../../../areas/wld/mobs_underdark.wld#L277) to [#347960](../../../areas/wld/ud.wld#L146107) | #347960 | Mountains | #346458, 3,863 rooms |
| [#24010 northeast](../../../areas/wld/mobs_underdark.wld#L285) to [#347675](../../../areas/wld/ud.wld#L144098) | #347675 | Mountains | #346458, 3,863 rooms |
| [#24012 south](../../../areas/wld/mobs_underdark.wld#L343) to [#350651](../../../areas/wld/ud.wld#L164950) | #350651 | Zone Entrance | #346458, 3,863 rooms |
| [#24013 west](../../../areas/wld/mobs_underdark.wld#L372) to [#349448](../../../areas/wld/ud.wld#L156979) | #349448 | Mountains | #346458, 3,863 rooms |
| [#24013 southwest](../../../areas/wld/mobs_underdark.wld#L388) to [#348251](../../../areas/wld/ud.wld#L148236) | #348251 | Mountains | #346458, 3,863 rooms |
| [#24014 northwest](../../../areas/wld/mobs_underdark.wld#L429) to [#314885](../../../areas/wld/ud.wld#L53225) | #314885 | Ocean | #304052, 5,978 rooms |

The 18 active records remain unresolved in the current full-area manifest.
This review adds no live route, reciprocal direction, or candidate row.

## Other typed references

One separate source-only zone reset targets the map. [surf.zon line 162](../../../areas/zon/surf.zon#L162)
requests mobile #100116 in room #349161. That room exists only in ud.wld at
[line 155058](../../../areas/wld/ud.wld#L155058), is titled “Zone Entrance,”
and lies in the #346458 component. Mobile #100116 is defined in
[random.mob](../../../areas/mob/random.mob#L1475). Surf is not in areas/AREA,
so this is not an active reset or WLD route; preserve it as a source-only
typed placement.

The bounded checked-in reference scan found no other WLD source targeting ud,
no active ZON room reset into ud, no QST, SHP, map/story JSON integer, or
type-25 OBJ room destination targeting these room VNUMs, and no matching
numeric literal in maintained src code. The one Surf reset above is separate
from the 18 active WLD targets. Runtime-generated/editor data and persisted
room identities were not inspected.

## Title, history, and disposition

After color stripping and title normalization, “Underworld of Duris” is unique
among all 442 current ZON files. Followed same-stem history reaches the
August 12, 2009 area-tree move and shows no later tracked-file edits or
replacement statement. Current WLD room VNUMs have no numeric overlap with
another WLD file. These results do not establish a predecessor or successor;
similar names such as underworld, underworld3, underdark, udmini, ud-inside,
and ud_map0–2 are not replacement evidence.

Keep ud **RETAIN IN PLACE** as an unregistered map candidate. Preserve the 18
active unresolved approaches, the inactive Surf reset reference, all 273
unresolved source directions, and the authored component boundaries. Do not
add it to the manifest, join it to another map, move it, or archive it until
the map/editor/generator owner confirms the sparse graph, the `300 300`
trailer, ingress intent, the Surf placement, and runtime/persisted identity.
The [20 builder questions](UD_BUILDER_QUESTIONS.md) record those decisions.
No build, boot, deployment, editor project, or database state was inspected.
