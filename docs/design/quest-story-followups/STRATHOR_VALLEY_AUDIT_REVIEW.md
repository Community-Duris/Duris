# Strathor Valley of the Storm Giants: repository-wide connectivity and dependency audit

**Checkpoint 87; priority 127 of the original 220-zone queue.** This supplement
reconciles the [source-comprehensive dossier](../zone-stories/STRATHOR_VALLEY.md)
with the active WLD graph, inactive WLD sources, typed references, compiled
numeric matches, normalized title, and bounded source history. Authored edges
and static code destinations do not establish ordinary availability, safe
travel, or played qualification.

## Active WLD graph

The active manifest has 351 WLD contributors. Strathor Valley of the Storm
Giants has 136 rooms and 297 exits; all 297 resolve. Its internal-exit graph
has one weak component of 136 rooms. The two cross-area exits are reciprocal:

| Strathor edge | Active return | Cross-area register rows |
| --- | --- | --- |
| #30800 south to Surface #600071 | Surface #600071 north to #30800 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L432), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1576) |
| #30929 north to Surface #599271 | Surface #599271 south to #30929 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L433), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1575) |

There are no negative or positive unresolved exits, fall-chance fields, or
current fields. The one global active-WLD duplicate room VNUM, #51800 in Hell
Two and Hell Three, is unrelated. These authored pairs do not qualify live
doors, access, reset state, route safety, or other dynamic travel.

## Inactive WLD sources

The all-95 inactive WLD scan parsed 2,080,773 exits without issues. No inactive
source-room VNUM overlaps Strathor. It found one source-only approach:
`surf.wld` room #216305, “A Stone Road Through the Stragathor Valley,” has a
north exit to Strathor #30800. This is not active ingress. The curated
source-only reference total is now 1,092, and the reviewed Surf subset is now
121. See the [source-only exit](../../../areas/wld/surf.wld#L342101),
[cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L432), and
[coverage row](ZONE_COVERAGE_REGISTER.csv#L283).

## Typed references and compiled numeric matches

The external typed scan found one active foreign reset reference: Alatorin
spawns mobile #83305 at room #83517 and its G record loads Strathor object
#30820, the bloody goat leg. This is a foreign use of an existing local object,
not a second local unit. See the [Alatorin reset](../../../areas/zon/alatorin.zon#L3284)
and [object definition](../../../areas/obj/stormht.obj#L310).

No external QST item command or SHP product matched Strathor objects. The scan
found no external mobile or object prototype definitions, no destination into
Strathor among 872 external type-25 declarations (713 active, 159 source-only),
and no exact `item_vnums` hit across 231 parsed external story sidecars.

A context-reviewed C/C++ scan found two direct references:

| Local target | Static reference | What the source establishes |
| --- | --- | --- |
| Mobile #30831 | [Special assignment](../../../src/specs/specs.assign.c#L769) | The local bartender is assigned the generated `world_quest` special, already mapped by the source dossier. |
| Room #30929 | [Stromvok ferry stop](../../../src/world/ferryact.c#L137) | The ferry definition includes this room as a Strathor stop. This is a dynamic travel service, not an additional WLD boundary. |

Remaining raw numeric matches are constants, decimal values, comments, or
namespace collisions. Room, mobile, object, reset, and travel values were
classified by source context rather than treated as interchangeable.

## Title, history, and disposition

The color-stripped title in [stormht.zon](../../../areas/zon/stormht.zon#L2)
normalizes to “Strathor Valley of the Storm Giants” and is unique among 351
active contributors. The followed WLD and ZON histories include older source
edits and the 2009 area-tree path move; the curated
[replacement register](DUPLICATE_AND_REPLACED_ZONES.md) identifies no distinct
predecessor or replacement. The active, un-commented
[areas/AREA row](../../../areas/AREA#L150) remains **DO NOT MOVE**.

The existing dossier covers the local king's proof return, actors, the ferry,
resets, shops, controls, equipment, and selected foreign ownership. This
repository-wide scan adds the active boundary census and classifies the
source-only Surf approach, foreign object reset, and compiled room/mobile
references. Editor/tooling closure, persisted identity, dynamic route
availability, door/reset state, safe travel, and played qualification remain
separate.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.


## Checkpoint 370 builder-question coverage

Physical coverage line 283, `area:stormht`, already had a source-comprehensive map, active graph census, typed-reference review, and title/history review. This checkpoint completes the coverage register's builder-question review with twenty source-backed questions and conservative recommendations. The [Strathor Valley story map](../zone-stories/STRATHOR_VALLEY.md) and this [repository-wide audit](STRATHOR_VALLEY_AUDIT_REVIEW.md) hold the full evidence.

### Builder questions and conservative recommendations

1. Should `stormht` remain the active owner of its rooms, prototypes, resets, quests, shops, and story identity? Keep it **DO NOT MOVE**; qualify any ownership change against all of these maintained sources.
2. Should the two reciprocal Surface pairs remain Strathor Valley's active WLD borders? Preserve #30800 south/#600071 north and #30929 north/#599271 south; qualify live access and return separately.
3. Should the inactive [Surf #216305 north exit to #30800](../../../areas/wld/surf.wld#L342101) remain source-only? Keep its exact `surf.wld` identity on existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); it is not active ingress.
4. Should the Stromvok ferry continue to list room #30929 as a dynamic stop beside its static north exit? Preserve the ferry reference and WLD edge as distinct source records; qualify live schedule, boarding, destination, and return before promising a ride.
5. Should King Stragathor's native contract continue to accept bronze sword #40073 for crown #30846 and 60,000 experience? Preserve the exact QST offering and result; keep this one proof requirement intact.
6. Should the player's own Sultan kill or first recovery of sword #40073 be required? Keep matching supplied sword stock eligible under the current contract; track its Nizari source and transferred custody separately.
7. Should the king's public QA response grant every listener story credit? Keep accepted settlement and eligible frozen participants as the credit boundary; a shared thank-you remains public text.
8. Should the eight addressed dialogue blocks and sixteen aliases remain contextual rather than a mandatory conversation chain? Preserve their current actor/topic links; add a prerequisite only with an explicit builder decision.
9. Should minstrel #30868's periodic `qc_action 200` song remain an environmental event? Preserve its periodic dispatch and current reset context; record a hearing or instrument outcome only after qualifying an actual delivered event.
10. Should torch #30812's PULL control remain paired with the declared closed/secret south exit even though the target lacks `EX_BLOCKED`? Preserve the target and controller records until a builder selects a barrier, control, or prose resolution in separate fix work.
11. Should the red book's room prose say PUSH while its registered command is PULL? Confirm the intended verb and update the source description or command in a separate, reviewed editorial/control change.
12. Should the book continue targeting the remote #30916 south exit while D8 marks it BLOCKED and #30934's north return is open? Preserve both directions and qualify the shared state change and actual passage before describing an access outcome.
13. Should room #30848's magical-rune prose imply an interaction? Keep it descriptive until a builder defines a real action; the source review found no selected TRACE command or rune handler.
14. Should granite door #30845 north to #30849 south keep its current key and reset asymmetry? Preserve key #30841 and the reset states on both faces until a builder confirms intended lock state and both travel directions.
15. Is the vault at #30934 intended to require blue key #30851 when container #30850 is closeable/locked without `CLOSED`? Keep the declared key and flags while builders qualify ordinary open/get behavior; avoid promising the key is mandatory from the source tuple alone.
16. Is sea chest #30830 intended to use a key when its configured key VNUM is zero and rusty key #30854 is reset on a sea hand in the same room? Preserve both prototypes and their reset ownership until the builder chooses a supported access rule and it is qualified.
17. Should the single potential daily remain conditional on current sources and accepted accounting? Preserve mode 2, the capped reset sources, and ready-accounting gates; qualify actual supply renewal and recovery before promising repeat availability.
18. Should the three local shop records count as story progress? Keep their transactions and stock separate from the king's one native contract; preserve the current shop owners and source rows.
19. Should bartender #30831's generated `world_quest` requests remain separate from King Stragathor's static QST contract? Preserve their different giver, target, runtime-zone, and reward identities; require their own qualified outcome.
20. Should Alatorin's external reset of goat-leg object #30820 and Strathor's weapon, hat, instrument, and ballista effects create additional king-story credit? Keep the outside reset under Alatorin and equipment use under their own source paths; award no extra war outcome without a separately authored and qualified result.

These recommendations complete the builder-question review; owner decisions remain open. The existing Surf approach is already represented on its candidate row, so this checkpoint adds no route record and changes no source behavior. This review is documentation-only; no build, test, runtime, database, or migration action was performed.

## Checkpoint 825: current WLD and typed-source reconciliation

A current stateful scan reproduces the 446-file `areas/wld` inventory (351 active, 95 unregistered). The 95 unregistered files parse 527,791 rooms and 2,080,773 exits with no issues or source-room overlap; their only target into Strathor remains Surf #216305 north to #30800. A repository-wide exact room-range scan across all 448 `.wld` files finds only two active Surface returns—#600071 north to #30800 and #599271 south to #30929—plus that one source-only Surf target. No other WLD file contains a Strathor-range reference. Preserve both reciprocal active pairs and the Surf edge's separate source identity.

Raw exact-number scans across current QST and every ZON reset command type reproduce the sole external reset reference: active Alatorin `G` loads Strathor object #30820 on mobile #83305 at room #83517 ([reset](../../../areas/zon/alatorin.zon#L3286), [object](../../../areas/obj/stormht.obj#L310)). No external QST item operand or SHP keeper/product matches a local room, mobile, or object. All 232 current story sidecars parse without errors; none of the 231 other sidecars has an exact local prototype number in a structured field or completion string. Same-type room, mobile, and object overlap remains zero across the current area source files.

The current C/C++ numeric scan again finds only two local bindings: bartender #30831 receives `world_quest` ([assignment](../../../src/specs/specs.assign.c#L769)), and room #30929 appears as the Stromvok ferry stop ([ferry table](../../../src/world/ferryact.c#L137)). The ferry record is dynamic travel metadata, not another WLD edge. A WLD exit-tuple key scan finds no external use of a Strathor object; local key #30841 occurs at [the local exit](../../../areas/wld/stormht.wld#L1073).

Within the 21,682 prototypes in canonical `areas/obj`, the scan reproduces 872 type-25 declarations (713 active, 159 unregistered) and 380 type-29 declarations (362 active, 18 unregistered). The additional `areas_mini/mini.obj` contributes 801 prototypes, including one type-25 and one type-29; repository-wide totals across all 443 `.obj` files are 873 type-25 and 381 type-29. No fixed or random type-25 destination from any source reaches Strathor. The only type-29 room targets are local object #30812 to #30884 and object #30825 to #30916; neither the local nor extra Mini-zone declarations add an outside target into Strathor. These are the torch and book controls already reviewed in the [source dossier](../zone-stories/STRATHOR_VALLEY.md#secret-controls-mansion-runes-and-doors). The nonstandard files `areas_mini/mini.qst`, `areas_mini/world.qst`, `areas_mini/mini.zon`, and `areas_mini/world.shp` contain no exact Strathor prototype VNUM. `areas_mini/mini.mob` and `areas/wld/fooquest.mob` define no same-type local mobile; `areas_mini/mini.wld` and `areas/mob/astral_main.wld` contain no Strathor-range room reference.

This refresh confirms existing dependencies without changing the title/history review or the twenty builder questions. Keep active `stormht` **DO NOT MOVE** and keep the source-only Surf approach on candidate row 97 **RETAIN IN PLACE**. No native source changed; tests, build, runtime travel, service/reset availability, database, migration, and operational work were not run.
