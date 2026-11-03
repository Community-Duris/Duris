# Ny’Neth’s Stronghold Continued: repository-wide route, dependency, and history supplement

This bounded static supplement accompanies the [comprehensive Ny’Neth’s
Stronghold Continued source dossier](../zone-stories/NYNETH3.md). It
reconciles active and source-only WLD routes, typed QST/story/reset references,
compiled prototype references, title ownership, and bounded same-stem history.
Static source review does not establish runtime access or gameplay outcomes.

## Active WLD boundaries and directed reach

Active `nyneth3` has 118 rooms (#38700–#38817) and 232/232 resolving exits,
all local; it has no authored outbound WLD edge. The local graph has three
weak components of 98, 11, and 9 rooms. Raw directed traversal from #38700
reaches 11 rooms, and only two rooms have a path back. These figures assume
all authored exits are available and do not include item portals or establish
actual access, return, or safe travel. The [WLD source](../../../areas/wld/nyneth3.wld#L1)
has no parse issues or internal flag-8 exits.

Two active WLD exits enter Treasure Room #38811: Realm #14208 southwest and
White Harbor #55633 up ([Realm source room](../../../areas/wld/realm.wld#L4633),
[White Harbor source room](../../../areas/wld/wh.wld#L13037),
[target room](../../../areas/wld/nyneth3.wld#L2286),
[exit-register row 249](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L249),
[exit-register row 792](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L792),
[pair-summary row 397](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L397),
[pair-summary row 808](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L808)).
Neither has a reciprocal Nyneth3 WLD exit. The Realm source is a no-repair
smithy dispersal room and the White Harbor source is a multi-exit gnomish-key
teleport room. Treat both as technical controller leads, not verified
ordinary approaches; actual activation and safe return remain unqualified.

All 95 WLD files outside the active `areas/AREA` list parsed without issues
(527,791 rooms and 2,080,773 exits). None overlaps Nyneth3 room VNUMs or exits
into them. Curated source-only totals therefore remain 3,404 overall, 172 for
Surf, and 15 for ud_map2. No candidate row is added.

## Exact typed and compiled dependencies

The active prior-stage `nyneth` reset places object #22904 in room #22910
([reset](../../../areas/zon/nyneth.zon#L99)). This type-25 portal targets
Nyneth3 room #38700 ([prototype and target](../../../areas/obj/nyneth.obj#L33)).
It is the one external type-25 target into Nyneth3 among 872 scanned
declarations; five additional matching targets are local templates.

Active incoming references to Nyneth3 prototypes are:

| Source | Typed reference | Evidence |
| --- | --- | --- |
| Alatorin | QST reward and ZON reset for object #38779, the potion of swashbuckling; its story sidecar mirrors the completion key. | [QST](../../../areas/qst/alatorin.qst#L6522), [ZON](../../../areas/zon/alatorin.zon#L2519), [story contract](../../../areas/story/alatorin.story.json#L3615), [object](../../../areas/obj/nyneth3.obj#L828) |
| Barovia | Two G reset rows reference object #38715, the potion of bubbling acid. | [ZON row 378](../../../areas/zon/barovia.zon#L378), [object](../../../areas/obj/nyneth3.obj#L156) |

Nyneth3’s active resets also reference external stock and mobile prototypes:
Heavens objects #17, #360, and #903; White Harbor object #55208, the distinct
illithid-lord soul; Negative Plane object #26665; and prior-stage Nyneth mobile
#22904 ([reset rows](../../../areas/zon/nyneth3.zon#L43),
[Heavens objects](../../../areas/obj/heavens.obj#L115),
[White Harbor object](../../../areas/obj/wh.obj#L2702),
[Negative Plane object](../../../areas/obj/negplane.obj#L1015),
[Nyneth mobile](../../../areas/mob/nyneth.mob#L66)). The illithid-lord soul
is a separate object from the fourteen exact accepted soul VNUMs.

The bounded scans found no SHP product dependency or local SHP file, and no
external duplicate mobile or object prototype. Outside the one Alatorin story
sidecar contract above, the other 230 story sidecars parsed without errors and
had no numeric-token matches. C/C++ source binds mobile #38737 to the Nyneth
special procedure and lists it in the high-drop mob table
([assignment](../../../src/specs/specs.assign.c#L1250),
[high-drop table](../../../src/item/randomeq.c#L133)); that procedure can read
local mobile #38735, a Fury ([spawn call](../../../src/specs/specs.nyneth.c#L82),
[Fury prototype](../../../areas/mob/nyneth3.mob#L637)). Chaos equipment arrays
reference local objects #38720, #38725, #38761, #38763, #38764, #38765,
#38772, #38774, and #38776
([kit entries](../../../src/account/chaos_eq_data.h#L71)). Additional object
special assignments bind #38725, #38761, #38763, and #38772 to Stormbringer,
shield-block, regeneration-ring, and defense-plate procedures
([assignments](../../../src/specs/specs.assign.c#L1336)).

## Title and bounded history

The normalized title “Ny'Neth's Stronghold Continued” belongs only to active
stem `nyneth3` among 442 scanned `.zon` files
([title source](../../../areas/zon/nyneth3.zon#L2)). The separate active
`nyneth2` title omits “Continued.” Followed `nyneth3.wld` history reaches the
2009 areas-tree move and an opaque 2013 same-stem edit. `nyneth3.zon` history
also contains same-stem area tweaks, a 2015 quest-item flag pass, and a 2017
gear-value adjustment. These bounded paths establish no alternate-stem
rename or whole-zone replacement; the earlier `nyneth2` is a separate
campaign stage. The two technical WLD inbounds and portal/reset references are
route and dependency evidence, not replacement proof. See the [duplicate and
replacement review](DUPLICATE_AND_REPLACED_ZONES.md).

## Migration disposition

Keep active `nyneth3` DO NOT MOVE. Preserve both one-way WLD inbounds, the
active prior-stage portal, Alatorin and Barovia item references, cross-area
reset dependencies, and compiled procedure/equipment bindings. Keep `realm`,
`wh`, `nyneth`, `alatorin`, `barovia`, `heavens`, and `negplane` as separate
active contributors. There are no inactive-WLD approaches and no new
candidate row. Editor, administrative, installed-data, persisted-player,
indirect-runtime, and actual traversal closure remain outside this bounded
scan; no gameplay, migration, or source change is proposed.
