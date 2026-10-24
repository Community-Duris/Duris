# The Caverns of Armageddon: source audit supplement

This supplement records the repository-wide connectivity, typed-dependency,
title and tracked-history review for active contributor `hunt` at source
snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. The existing
[source-comprehensive dossier](../zone-stories/CAVERNS_OF_ARMAGEDDON.md)
documents all eighteen local contracts, environmental narrative, reset
custody, keys, traps and escape descriptions.

## Static connectivity and dynamic routes

The active `hunt.wld` contains 149 rooms and 320 exits. All 320 targets resolve:
318 target local Hunt rooms and two leave for Surface. There are no negative
targets, unresolved positive targets, room prototypes outside the zone's
numeric interval, or `C`/`F` current and fall records. The local directed graph
has one weak component of 149 rooms and five strong components sized 119, 15,
13, 1 and 1.

Using the Surface boundary room #13307 as a graph-analysis seed, 134/149 rooms
are reachable by authored local WLD edges and 134/149 have authored WLD paths
back. The fifteen rooms outside forward reach are #13350, #13429–#13433,
#13435–#13436 and #13439–#13445. The fifteen without a local WLD return to
#13307 are #13348, #13417–#13428, #13434 and #13437. This is an induced WLD
graph only. It omits the tent portal, rift, cross-zone portal and rescue
ladder; it does not establish an actual player path, live availability, safe
travel or a verified return.

The four active cross-area records are:

| Source | Destination | Classification |
| --- | --- | --- |
| Hunt #13307 south | Surface #650096 | Reciprocal with Surface #650096 north to Hunt #13307. |
| Hunt #13307 west | Surface #649693 | No matching reverse edge. |
| Surface #649695 east | Hunt #13307 | Inbound edge; Hunt has no reverse west edge to #649695. |
| Surface #650096 north | Hunt #13307 | The reciprocal edge paired with Hunt's south exit. |

See the full [cross-area exit rows 228–229 and 1681–1682](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L228)
and directional [pair summaries 196 and 517](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L196).
The asymmetry is also explicit in the [Hunt WLD](../../../areas/wld/hunt.wld#L125)
and [Surface WLD](../../../areas/wld/surface.wld#L3142983).

The scan of the 95 WLD sources outside `areas/AREA` found no room-number
overlap and no source-only exit into Hunt. Therefore Hunt adds no source-only
approach to Surf, Underworld2 or another candidate's retained records.

Dynamic source records explain some of the raw graph gaps:

- Tent object #13302 at room #13313 enters recovery room #13350, whose south
  WLD exit returns to #13313 ([object](../../../areas/obj/hunt.obj#L26),
  [reset](../../../areas/zon/hunt.zon#L154),
  [destination room](../../../areas/wld/hunt.wld#L981)).
- Rift object #13339 at #13426 enters #13429. Local WLD routes continue through
  the spiral into throne #13431; there is no ordinary reverse rift declaration
  at #13429 ([object](../../../areas/obj/hunt.obj#L617),
  [reset](../../../areas/zon/hunt.zon#L177),
  [destination](../../../areas/wld/hunt.wld#L2602)).
- Portal object #13363 at #13341 enters Myrabolus #82627. Active Myrabolus
  portal #82539, reset at #82627, targets Hunt #13341, forming a source-defined
  type-25 pair ([Hunt object and reset](../../../areas/obj/hunt.obj#L936),
  [Myrabolus object](../../../areas/obj/mira.obj#L565),
  [Myrabolus reset](../../../areas/zon/mira.zon#L197)). Runtime reset state,
  access and return safety were not tested.
- The hidden rescue route from throne #13431 goes up to #13439 and through the
  Sunweaver ladder/ship sequence. Its exit into the throne is blocked, and the
  final disembark path at #13445 returns to forest #13355. Source review found
  no active kill, victory or timed callback that creates or summons this route
  ([rope entrance](../../../areas/wld/hunt.wld#L2786),
  [ship bridge](../../../areas/wld/hunt.wld#L2820),
  [forest disembark](../../../areas/wld/hunt.wld#L2896)).

## Typed dependencies and dynamic travel

The all-source typed-reference scan found these outbound Hunt dependencies:

| Hunt source | External prototype / reference | Interpretation |
| --- | --- | --- |
| [`hunt.zon` lines 164–166](../../../areas/zon/hunt.zon#L164), object placements at #13371 | Objects #67278 from `unique.obj`, #359 from `heavens.obj`, and #55449 from `wh.obj` | Active imports; Soulsword #67278 has encoded effects, #359 is assigned `epic_stone`, and #55449 is imported memory. |
| [`hunt.zon` line 178](../../../areas/zon/hunt.zon#L178), object placement at #13431 | Fountain object #72 from `heavens.obj` | Active import bound to `spell_pool`; procedure semantics and dispatcher target behavior remain separate from Hunt's quests. |
| `hunt.qst` line 87 | Jade Empire object #76613 | Foreign Scralack head accepted by local Clond #13304 for medallion #13368. |

Prototype and binding evidence: [Soulsword](../../../areas/obj/unique.obj#L1225),
[epic rune](../../../areas/obj/heavens.obj#L3725), [imported memory](../../../areas/obj/wh.obj#L5299),
[spell-pool fountain](../../../areas/obj/heavens.obj#L1000),
[epic-stone binding](../../../src/specs/specs.assign.c#L1604),
[spell-pool binding](../../../src/specs/specs.assign.c#L1620), and
[local foreign-head acceptance](../../../areas/qst/hunt.qst#L87).

Active outside sources also reference Hunt prototypes:

| Source | Hunt prototype use | Scope |
| --- | --- | --- |
| `desolateinv.zon` | Loads mobile #13385 into three Desolate rooms (#77311, #77328, #77333). | Three active reset placements of a Hunt mobile. |
| `mira.zon` | G-loads object #13321 at four reset locations. | Active use of Hunt's master arrows. |
| `vehicles.zon` | G-loads Hunt objects #13320 and #13321. | Active use of the local longbow and master arrows. |
| `mira.qst` and `mira.story.json` | Twelve typed references across eight Hunt item VNUMs, including Maximas and Krazzi heads, four treasure receivers and the lost monkey. | Myrabolus-owned consumers/rewards; these do not create extra Hunt receipts. Maximas head #13357 is also accepted by a Myrabolus Roland with no declared reward. |
| `cosmic.qst` and `cosmic.story.json` | Accepts Hunt demon heart #13366 for item #76243. | Ultarium-owned acceptance, documented as foreign in the Hunt dossier. |

The full source scan found no external SHP product using a Hunt object, no
direct compiled C/C++ literal reference in the Hunt VNUM ranges, and no other
external type-25 destination into Hunt. The code bindings for imported #359
and #72 are typed by their actual object prototypes and are not Hunt-range
numeric bindings. These results cover checked-in typed references, not editor
state, persisted inventory, indirect construction or live availability.

The incoming reset locations are [Desolate Invaders](../../../areas/zon/desolateinv.zon#L343),
[Myrabolus](../../../areas/zon/mira.zon#L444) and [Vehicles](../../../areas/zon/vehicles.zon#L829).
The foreign quest ownership is visible in [Myrabolus QST](../../../areas/qst/mira.qst#L4)
and [Cosmic QST](../../../areas/qst/cosmic.qst#L155). Detailed accepted roots,
outputs and the separation from Hunt's eighteen outcomes remain in the
[source dossier](../zone-stories/CAVERNS_OF_ARMAGEDDON.md#foreign-follow-ups-and-shared-effects).

## Title, history and disposition

After color-code removal, “The Caverns of Armageddon” is unique among the 442
current `.zon` titles. Similar cave and cavern names resolve to distinct
contributors. Tracked `hunt` WLD/MOB/OBJ/QST/ZON history remains on the same
stem through the 2009 area-tree move and later edits. No alternate-stem
predecessor, title rename or explicit whole-area replacement was found. The
story sidecar was first added in 2026; it is not evidence of an older
replacement.

Commit `ddbb2549e10de17552514ff94aa9bd4bc4d3a95d` (2026-04-06) includes the
message “Added The Hunt forest exit to Jade Empire cont” and changes Surface
#649695 east from #649696 to Hunt #13307. The current Hunt west exit from #13307
still targets Surface #649693. The commit documents route history, but does
not decide whether the current asymmetric placement is intentional.

Keep active `hunt` **DO NOT MOVE**. No attic candidate row is warranted. Preserve
the local and external prototypes, all active reset/quest references, and the
source-defined Myrabolus portal pair while builders review intent. This review
is static; no tests, build, server, database, migration, gameplay or operational
script was run.
