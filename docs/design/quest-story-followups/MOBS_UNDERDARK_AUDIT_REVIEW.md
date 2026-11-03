# Mobs_Underdark repository-wide audit supplement

This bounded source audit follows active `mobs_underdark` in
[coverage row 186](ZONE_COVERAGE_REGISTER.csv#L186). It adds the all-442 title
result, reciprocal local-room detail, outside typed consumers, bounded source
history, and twenty direct builder and owner questions. Existing native
contract dialogue and reset findings remain in
[MOBS_UNDERDARK_STORY_REVIEW.md](MOBS_UNDERDARK_STORY_REVIEW.md).

## Active graph and unresolved targets

The active WLD has 15 rooms and 98 exit records. Only four destinations resolve
in the active world: the reciprocal vertical pair #24013 up to #24014 and
#24014 down to #24013, plus two outbound area exits:

| Source room | Direction | Target | Source line |
| ---: | --- | ---: | --- |
| #24007 | north | Underworld3 #30149 | [exit](../../../areas/wld/mobs_underdark.wld#L174) |
| #24012 | north | Ixarkon #96471 | [exit](../../../areas/wld/mobs_underdark.wld#L335) |
| #24013 | up | local #24014 | [exit](../../../areas/wld/mobs_underdark.wld#L376) |
| #24014 | down | local #24013 | [exit](../../../areas/wld/mobs_underdark.wld#L425) |

The graph therefore has 14 weak components: thirteen single rooms and the
two-room local pair. The 94 remaining positive destinations are unresolved
in active input; 18 target VNUMs also occur in inactive `ud` WLD sources. No
external active WLD exit targets rooms #24001–#24015, and there is no reverse
active record for either outbound boundary. The 18 unregistered-map matches
remain source evidence, not routes to add; they are already listed on the
existing [`ud` candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L110),
where `mobs_underdark` is the active source key.
See the [unresolved-exit ledger](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L55) and
[active boundary summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L287).

## Typed source relationships

The active ZON has 549 `M` commands and 17 `G` commands. Its existing
source review maps the 549 mobile rows to 313 local-prototype resets, 187
Underworld resets, 22 Connectorzones resets, 17 RTun resets, four Ghore
resets, and six unresolved mobile #1301 resets. Of the 17 object rows, 14
give Connectorzones #53610 and three give local potion #24012. No external
active ZON object reset references a local object #24001–#24019.

The bounded reverse scan found one active outside mobile consumer: Alatorin
resets local pech miner #24011 six times at rooms #83916 and #83920
([first](../../../areas/zon/alatorin.zon#L4190),
[last](../../../areas/zon/alatorin.zon#L4197)). The active Mushroom Caverns
sidecar references givers #24021–#24023 and the legacy contract items
([contact mapping](../../../areas/story/mushroom_caverns.story.json#L21),
[contract mapping](../../../areas/story/mushroom_caverns.story.json#L124)).
No outside native `.qst` file or `.shp` record contains a typed reference to
these local prototypes.

Compiled CHAOS kit data references object #24016, Kryz'Kyssik's bronze
Zarbonesti seal, in slot 24 in eleven class tables. The tables include
`chaos_eq_standard_shaman` and ten `chaos_eq_enhanceable_*` profiles; the
consumer path is in [`nanny.c`](../../../src/account/nanny.c#L706), and the
generated item entries begin at
[`chaos_eq_data.h`](../../../src/account/chaos_eq_data.h#L144). This source
relationship is distinct from the local QST reward declaration.

There are no duplicate external MOB prototype headers #24001–#24023, OBJ
prototype headers #24001–#24019, or WLD room headers #24001–#24015. A broad
literal OBJ scan found #24010 in Mitashi object #138271's numeric weapon
record ([source](../../../areas/obj/mitashi.obj#L1189)); there it is the
weapon's cost field, not a room destination. No typed object teleport or
switch target points into the local room range.

## Title, source history, and disposition

After removing color codes and normalizing case and whitespace, “The
Underdark” has one owner among all 442 current `.zon` files:
active `mobs_underdark` ([title](../../../areas/zon/mobs_underdark.zon#L2)).
“The Twisting Tunnels of the Durian Underdark” (`underdark`) and “The
Minizones of the Underdark” (`udmini`) remain distinct normalized titles.

Followed WLD, ZON, MOB, OBJ, and QST paths remain on their existing stems
through the 2009 area-tree relocation. Later history records source edits,
not an alternate-stem rename, distinct predecessor, or whole-zone
replacement. Keep active `mobs_underdark` DO NOT MOVE. Preserve the 18
unresolved `ud` matches on the existing [candidate record](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L110)
without inferring an active route or map owner.

## Checkpoint 288: direct builder and owner questions

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 1 | Which current object prototype, if any, is meant by Haz'on'wyz's goblet VNUM #1515? | [Native QST](../../../areas/qst/mobs_underdark.qst#L1), [missing-prototype finding](MOBS_UNDERDARK_STORY_REVIEW.md#finding-1-three-linked-house-zarbonesti-contracts) | Identify the exact item before changing the contract; do not substitute by name alone. |
| 2 | Is Ozman's reward #24014 intentionally a money-type object in the legacy contract? | [QST terms](../../../areas/qst/mobs_underdark.qst#L43), [object prototype](../../../areas/obj/mobs_underdark.obj#L226), [Mushroom sidecar](../../../areas/story/mushroom_caverns.story.json#L150) | Confirm the durable item and accounting design; keep source declaration and live acceptance as separate questions. |
| 3 | Should the two same-named amulet halves #24013 and #24014 remain distinct prototype identities? | [Local item records](../../../areas/obj/mobs_underdark.obj#L212), [final QST](../../../areas/qst/mobs_underdark.qst#L105) | Preserve both VNUMs and exact contract inputs unless the story owner approves an explicit redesign. |
| 4 | What does the literal final `R E 35000` term intend to deliver? | [Final native contract](../../../areas/qst/mobs_underdark.qst#L105) | Preserve the token exactly until its native meaning and expected outcome are confirmed. |
| 5 | Should the story inventory map giver VNUMs #24021–#24023 when they exceed the zone's last room VNUM #24015? | [Zone header](../../../areas/zon/mobs_underdark.zon#L1), [range-based owner helper](../../../scripts/zone_story_quest_zone_inventory.py#L103) | Confirm owner mapping rules and keep all three direct QST records attached to this source. |
| 6 | Which area owns the accounting/story identity of the Zarbonesti arc: the native Mobs_Underdark QST source, or the Mushroom Caverns sidecar that maps it? | [Native QST](../../../areas/qst/mobs_underdark.qst#L1), [sidecar contact](../../../areas/story/mushroom_caverns.story.json#L21) | Name the source owner explicitly; do not merge the two contributor identities by shared contracts. |
| 7 | Is it intentional that mobile #24021 is statically reset while #24022 and #24023 have no current ZON reset? | [First giver reset](../../../areas/zon/mobs_underdark.zon#L567), [giver definitions](../../../areas/mob/mobs_underdark.mob#L303) | Identify any event or alternate placement before promising ordinary availability. |
| 8 | Should the local room descriptions remain empty while titles identify only dispersal points? | [Room records](../../../areas/wld/mobs_underdark.wld#L1) | Confirm the intended environmental-detail level; do not infer room functions beyond their source titles. |
| 9 | Is the reciprocal up/down link between #24013 and #24014 the intended only local traversable pair? | [Up exit](../../../areas/wld/mobs_underdark.wld#L376), [down exit](../../../areas/wld/mobs_underdark.wld#L425) | Retain the pair and thirteen other isolated records until the map owner supplies reviewed topology. |
| 10 | Are the 94 positive unresolved exits deliberate pruned targets, and which map generation owns the 18 `ud` matches? | [Local source](../../../areas/wld/mobs_underdark.wld#L1), [existing `ud` candidate](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L110) | Keep them unresolved and preserve the current candidate evidence until each intended destination is identified. |
| 11 | Should the one-way active exit from #24007 to Underworld3 #30149 remain without a reverse WLD record? | [Source exit](../../../areas/wld/mobs_underdark.wld#L174) | Confirm intended direction and return design before adding or removing a boundary. |
| 12 | Should the one-way active exit from #24012 to Ixarkon #96471 remain without a reverse WLD record? | [Source exit](../../../areas/wld/mobs_underdark.wld#L335) | Confirm the endpoint and return design separately; do not treat a source edge as safe travel. |
| 13 | Are six resets of mobile #1301 intentionally left without a current MOB prototype? | [Reset block](../../../areas/zon/mobs_underdark.zon#L383) | Confirm the intended mobile or remove the unresolved declaration only through a separately approved source change. |
| 14 | Should the 187 imported Underworld mobile resets remain assigned to their current external owner prototypes? | [Reset inventory](MOBS_UNDERDARK_STORY_REVIEW.md#finding-3-the-area-has-substantial-reset-and-topology-dependencies) | Preserve the source-owner distinction and verify each imported prototype before any migration proposal. |
| 15 | Are the Connectorzones, RTun, and Ghore mobile imports intentional shared populations in these dispersal rooms? | [Typed reset inventory](MOBS_UNDERDARK_STORY_REVIEW.md#finding-3-the-area-has-substantial-reset-and-topology-dependencies) | Keep source reset rows distinct by owner and prototype; do not infer live availability from them. |
| 16 | Should the 14 Connectorzones object #53610 placements and three local potion #24012 placements remain in the same G-reset stream? | [Object reset block](../../../areas/zon/mobs_underdark.zon#L103) | Confirm each typed product and reset destination; do not collapse object and mobile consumers. |
| 17 | Is Alatorin's six-reset import of local pech miner #24011 intentional? | [First reset](../../../areas/zon/alatorin.zon#L4190), [local prototype](../../../areas/mob/mobs_underdark.mob#L159) | Keep Alatorin recorded as the active external consumer until both source owners confirm placement intent. |
| 18 | Should Kryz'Kyssik's seal #24016 remain in eleven compiled CHAOS kit tables as well as the QST reward? | [CHAOS table](../../../src/account/chaos_eq_data.h#L144), [local seal](../../../areas/obj/mobs_underdark.obj#L259) | Confirm whether shared gear ownership is deliberate; keep the compiled and native reward references separate. |
| 19 | Should the active Mushroom Caverns sidecar continue mapping the legacy givers and item terms while this area has no local sidecar? | [Contact mapping](../../../areas/story/mushroom_caverns.story.json#L21), [active area](../../../areas/AREA#L166) | Record the sidecar owner and keep its cross-area relationship typed; do not copy it as a local sidecar without approval. |
| 20 | Is any retirement, merge, or replacement authorized by the title similarity, source history, or unresolved old-map links? | [Title/history review](MOBS_UNDERDARK_AUDIT_REVIEW.md#title-source-history-and-disposition), [coverage row 186](ZONE_COVERAGE_REGISTER.csv#L186) | No. Keep `mobs_underdark` DO NOT MOVE and require a separate owner decision and complete dependency review for any proposal. |

## Bounded closure and disposition

The normalized title scan, followed-path history review, and direct typed scan
are complete within repository source. The scan found one active external
mobile reset owner, one external story sidecar, and one compiled kit consumer
of a local object, alongside the reset, QST, and map references already
recorded in the existing source review. No duplicate external prototypes,
outside SHP records, active WLD inbound exits, or typed object portal/switch
destinations into the local room range were found. Tooling beyond the reviewed
owner helper, indirect runtime lookups, live reset/access behavior, and
persisted player references remain outside this result.

Keep active `mobs_underdark` DO NOT MOVE. The two active outbound area links,
18 inactive `ud` target matches, and cross-area reset/story/code references
remain separate typed evidence. This checkpoint adds no WLD route or candidate
row and proposes no native or gameplay source change.
