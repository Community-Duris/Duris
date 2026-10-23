# A Dark and Twisted Wood repository-wide audit supplement

This supplement reconciles active and inactive connectivity, typed dependencies, title ownership, and bounded source history for the [source-comprehensive Twisted Wood dossier](../zone-stories/DARK_AND_TWISTED_WOOD.md). The source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. This is a documentation audit; no area or gameplay source is changed.

## Active graph and boundary records

The active `twstwd` WLD has 100 rooms and 335 authored exits, all 335 resolving: 331 are local and four leave the area. The weak graph is one 100-room component. With all authored local exits treated as available, directed traversal from #16300 reaches 97 rooms; rooms #16397–#16399 have no authored incoming local exit. Every room has an authored directed path back to #16300. The dossier separately records route-specific SEARCH/OPEN reach, six optional camp rooms, and staging-room intent; this raw graph does not model those gates or live actor placement.

| Active boundary | Record and relationship |
| --- | --- |
| Twisted Wood #16300 south → Surface #568510 | [Twisted Wood exit](../../../areas/wld/twstwd.wld#L27) and [Surface return](../../../areas/wld/surface.wld#L1438174), #568510 north → #16300; reciprocal pair. |
| Twisted Wood #16300 west → Surface #568109 | [Twisted Wood exit](../../../areas/wld/twstwd.wld#L31) and [Surface return](../../../areas/wld/surface.wld#L1429761), #568109 east → #16300; reciprocal pair. |
| Twisted Wood #16364 east → Pine Hollow #16097 | [Twisted Wood exit](../../../areas/wld/twstwd.wld#L2139) and [Pine Hollow return](../../../areas/wld/pineholl.wld#L2645); reciprocal pair. |
| Twisted Wood #16378 east → Krindor #16400 | [Twisted Wood exit](../../../areas/wld/twstwd.wld#L2576) and [Krindor return](../../../areas/wld/krimman.wld#L20); reciprocal pair. |
| Pine Hollow #16162 east → Twisted Wood #16308 | [Pine Hollow exit](../../../areas/wld/pineholl.wld#L4155); incoming-only NPC-load connection, with no authored reverse exit. |
| Pine Hollow #16163 east → Twisted Wood #16344 | [Pine Hollow exit](../../../areas/wld/pineholl.wld#L4177); incoming-only NPC-load connection, with no authored reverse exit. |

There are six active inbound records, four outbound records, and four reverse records. The dossier identifies the two extra Pine Hollow edges as NPC-load connections rather than ordinary entrances. The area has no fall-chance or current fields. The exact cross-area rows are also in [the exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L280) and [pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L233).

## Inactive-WLD approaches and attic evidence

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without parse issues. No inactive source room overlaps the 100 active Twisted Wood room VNUMs. It found five non-overlapping source-only approaches, all in unregistered `surf.wld`:

| Inactive source-only approach | Evidence |
| --- | --- |
| #226853 east → Twisted Wood #16303 | [Surf exit](../../../areas/wld/surf.wld#L563309). |
| #227153 east → Twisted Wood #16302 | [Surf exit](../../../areas/wld/surf.wld#L569601). |
| #227453 east → Twisted Wood #16301 | [Surf exit](../../../areas/wld/surf.wld#L575897). |
| #227753 east → Twisted Wood #16300 | [Surf exit](../../../areas/wld/surf.wld#L582189). |
| #228054 north → Twisted Wood #16300 | [Surf exit](../../../areas/wld/surf.wld#L588498). |

These records are not active ingress because `surf.wld` is not an active world contributor. They raise the curated source-only approach total from 1,999 to 2,004 and the reviewed Surf subset from 146 to 151. Preserve the original source-map identities and keep `surf` / `surface2011` **RETAIN** pending map ownership and full dependency/tool closure. No migration clearance follows; active `twstwd` remains **DO NOT MOVE**.

## Typed dependencies and bounded negative checks

| Dependency | Source-backed result and boundary |
| --- | --- |
| Faerie's two required materials | Local Q29 references outside prototypes #16014 and #16016 through [QST item commands](../../../areas/qst/twstwd.qst#L36) and [sidecar item VNUMs](../../../areas/story/twstwd.story.json#L67). Both definitions are in Pine Hollow: [black platemail #16014](../../../areas/obj/pineholl.obj#L241) and [broken mithral lance #16016](../../../areas/obj/pineholl.obj#L284). These are intentional foreign prototype dependencies shared with Pine Hollow's own returns, not local duplicate objects. Q29 still produces one local collar receipt #16313. |
| Outside QST, SHP, and story references | No outside exact QST item command or SHP product references a local Twisted Wood object. Twisted Wood has no local SHP source. The bounded external story-sidecar scan found no exact room, mobile, or object VNUM references into this area. |
| Duplicate prototypes and object portals | The bounded active/unregistered scan found no external duplicate mobile/object definitions and no type-25 portal target in rooms #16300–#16399. The source dossier's statement that no incoming object portal is selected remains consistent with this scan. |
| Conditional compiled arrival | [SHIFT PRIME](../../../src/classes/innates.c#L2011) includes room #16302 as a random destination for an eligible Githzerai currently in Astral #19701, with the handler's other eligibility and cooldown conditions. This conditional code route is not a universal entrance or native faerie objective. |
| Guild room cases | [guild_guard](../../../src/specs/specs.guards.c#L184) contains room cases #16392 and #16383, but the selected literal assignment scan finds no local actor binding to this procedure. A room-case branch alone does not make an active class gate; the dossier records the handler and binding boundary. |

Other direct numeric matches for #16384 are buffer, flag, and protocol constants, not room references. Static prototype, reset, and code matches do not establish item custody, actor availability, successful use of SHIFT PRIME, or played arrival.

## Title, history, and disposition

After stripping color tokens and normalizing whitespace and case, “A Dark and Twisted Wood” has one exact owner among 351 active `.zon` titles: `twstwd` ([title source](../../../areas/zon/twstwd.zon#L2), [coverage row](ZONE_COVERAGE_REGISTER.csv#L321)). Followed WLD, ZON, MOB, OBJ, and QST histories remain on the `twstwd` stem from the 2009 area relocation through same-stem updates; the tracked quest-item flag update appears in 2015. The story sidecar was added on 2026-10-06 in commit `14be6f392`. No alternate-stem rename, explicit predecessor, or whole-zone replacement claim appears in these bounded paths. Keep active `twstwd` **DO NOT MOVE** and `surf` / `surface2011` **RETAIN**. No source placement changed.

## Builder recommendations

1. Preserve the four reciprocal boundaries and distinguish them from Pine Hollow's two incoming-only NPC-load connections. Keep the conditional SHIFT PRIME route separate from universal access.
2. Preserve all five inactive Surf approaches as source-only map evidence. They do not make the zone reachable through the active loader or clear a source move.
3. Keep the faerie's two item identities tied to their Pine Hollow prototypes and competing source/consumption histories. Current custody, source recovery, and the single local accepted return remain separate evidence.
4. Validate the effective `guild_guard` binding before presenting the two room cases as player-facing class barriers.
5. Keep staging rooms #16397–#16399 as documented outgoing-only source locations until builders select and qualify any new arrival behavior.

## Checkpoint 408: explicit builder questions

Physical coverage row 321 now has exactly 20 direct-source builder questions
with conservative recommendations in the [Twisted Wood dossier](../zone-stories/DARK_AND_TWISTED_WOOD.md#explicit-builder-questions-and-recommended-resolutions).
They preserve the atomic faerie return and separate it from Pine Hollow source
history, report prose, optional camp treasure, conditional arrival, NPC room
gates, reset issuance, and connected-zone receipts. No area or gameplay source
changed.

## Checkpoint 863: current Surf approaches and dependencies carried forward

CP863 carries the CP127 active-graph, typed-dependency, and identity findings forward against the unchanged pinned area/code snapshot. The active graph remains 100 rooms and 335/335 resolving exits (331 local, four outbound) in one weak component. From #16300, raw authored traversal reaches 97 rooms and every room has an authored return path; the three staging rooms #16397–#16399 have no authored incoming local exit. The four active outbound records remain reciprocal with Surface, Pine Hollow, and Krindor; two additional Pine Hollow records terminate at #16308/#16344 from NPC-load rooms.

The five `surf.wld` exits into #16300–#16303 remain source-only and do not add active ingress. They are already included in current cumulative source-only totals of 11,993 overall / 415 Surf. CP127's 2,004 / 151 reviewed subtotal and CP408's 9,480 / 298 global total are historical. Preserve these five exact exits on existing Surf candidate row 97; the separate `surface2011` candidate remains a distinct source identity and does not own these records.

The static typed closure remains as documented: Twisted Wood's native Faerie Q29 consumes Pine Hollow objects #16014/#16016 and returns local collar #16313; no outside QST item command, SHP product, story item reference, duplicate active prototype, or incoming type-25 portal was found. Conditional SHIFT PRIME still names #16302 under its own Githzerai/Astral conditions; it does not prove universal access. The two `guild_guard` room cases still lack a selected local actor binding. Keep active `twstwd` **DO NOT MOVE**, retain `surf` and `surface2011` under their separate plans, and grant no route or migration clearance.
