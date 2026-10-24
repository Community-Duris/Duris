# Mosswood: repository-wide route, dependency, and history supplement

This bounded static supplement accompanies the [comprehensive Mosswood
source dossier](../zone-stories/MOSSWOOD.md). It reconciles active and
source-only WLD edges, typed QST/story/reset dependencies, compiled prototype
references, title ownership, and bounded same-stem history. Static source
review does not establish runtime access or gameplay outcomes.

## Active WLD boundaries and directed reach

Active moss has 161 rooms from #23400 through #23560 and 399/399 resolving
exits: 398 local and one across an area boundary. Its local graph is one weak
component. Raw directed traversal from #23400 reaches 157/161 rooms; all
161/161 have a path back to #23400. Rooms #23434 and #23558–#23560 are
outside root-forward reach. No internal exit has flag 8. These figures
describe authored WLD edges, not door, reset, script, or runtime admission.

Its sole active boundary is reciprocal with Surface: Moss #23400 north to
Surface #611257 and Surface #611257 south to Moss #23400 ([pair rows 295 and
551](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L295), [Moss room](../../../areas/wld/moss.wld#L1),
[Surface room](../../../areas/wld/surface.wld#L2335794)).

## Inactive WLD sweep

All 95 WLD files outside the active areas/AREA list parsed cleanly: 527,791
rooms and 2,080,773 exits. No inactive source room overlaps a Moss room.
Three non-overlapping source-only Surf exits target Moss:

| Source record | Source-only exit | Evidence |
| --- | --- | --- |
| surf.wld #229223 | south to #23400 | [Source room](../../../areas/wld/surf.wld#L613015) |
| surf.wld #229522 | east to #23400 | [Source room](../../../areas/wld/surf.wld#L619294) |
| surf.wld #229524 | west to #23400 | [Source room](../../../areas/wld/surf.wld#L619336) |

These records raise curated source-only totals to 3,352 overall and 172 for
the reviewed Surf subset; ud_map2 remains 15. They are source-only, not active
ingress. Keep surf RETAIN IN PLACE and preserve the existing Surf candidate
row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

## Exact typed and structured dependencies

The selected external scans found no QST item command, SHP product, structured
story item_vnums field, or ZON reset-role reference into Moss rooms, mobiles,
or objects. All 231 other story sidecars parsed without errors and contained
no numeric token match into Moss prototypes. Moss's own QST/story item fields,
SHP products, and ZON reset roles stay within its local object, mobile, and
room sets. The local shop file exists; no external product target was
identified.

No external duplicate mobile or object prototype was found. Across 872
external type-25 declarations, none targets Moss. A literal C/C++ scan found
no direct numeric prototype reference into Moss. These bounded scans do not
exhaust arbitrary computed, editor, administrative, or installed references.

## Title and bounded history

The normalized title “Mosswood” belongs only to active stem moss among 442
scanned .zon files ([title source](../../../areas/zon/moss.zon#L2)). Followed
moss.wld history reaches the 2009 areas/ move and same-stem 2011, 2013, 2016,
and 2026 edits. moss.zon history reaches the same 2009 move and same-stem
2011, 2013, 2015 quest-item flag, and 2016 typo fixes. The reviewed paths
establish no alternate-stem rename, distinct predecessor, or whole-zone
replacement. See the [duplicate and replacement review](DUPLICATE_AND_REPLACED_ZONES.md).

## Migration disposition

Keep active moss DO NOT MOVE. Preserve its reciprocal Surface boundary and
the three source-only Surf approaches. Keep surf RETAIN IN PLACE and update
its existing candidate row. No new candidate row is added. Editor,
administrative, installed-data, persisted-player, indirect-runtime, and
actual traversal closure remain outside this bounded scan; no gameplay,
migration, or source change is proposed.


## Checkpoint 292: direct builder and owner questions

The [Mosswood dossier](../zone-stories/MOSSWOOD.md) documents one exact-beet
request and nine broader story areas, while this supplement records the
repository-wide route, dependency, and title/history evidence. The questions
below turn its source gaps and decision points into builder review items.
Recommendations preserve native behavior and keep live availability, owner
decisions, persistence, and played outcomes unqualified.

1. **Question:** Should active stem moss remain the authoritative owner of the
   normalized title “Mosswood”?
   **Recommendation:** Keep the unique all-442 title match and current active
   area DO NOT MOVE. Resolve any command-target alias or semantic owner
   question explicitly before considering a rename or merge.

2. **Question:** Should the single reciprocal Surface boundary at Moss #23400
   and Surface #611257 remain Mosswood’s active WLD approach?
   **Recommendation:** Preserve the current north/south pair and keep its
   loader-selected ownership distinct from the three source-only Surf exits.

3. **Question:** Should the three inactive Surf exits into #23400 remain
   attached to existing Surf candidate row 97?
   **Recommendation:** Keep surf RETAIN IN PLACE and preserve the already
   counted source-only records. They do not add active ingress or a new
   candidate row.

4. **Question:** Should rooms #23434 and #23558–#23560, which ordinary directed
   traversal from #23400 does not reach, contain useful player-facing stock?
   **Recommendation:** Confirm intended staging and distribution behavior
   before promising those encounters. Qualify actual movement and population
   separately; do not treat a weak graph component as usable access.

5. **Question:** Should mobile prototypes #23417, #23447, and #23490 be
   expected as present Mosswood contacts even though they have no local M/F
   reset source?
   **Recommendation:** Keep definitions and narrative references separate
   from current or renewable populations. Confirm their actual controller or
   intended dormant status before mapping an encounter prerequisite.

6. **Question:** Are the nine object definitions #23418 and #23427–#23434
   intentionally absent from local G/O reset stock, including robe #23434?
   **Recommendation:** Preserve the source inventory and confirm intended
   availability before adding any stock. Descriptive gear, room prose, and a
   definition alone do not establish a recoverable item.

7. **Question:** Is beet #23436 intentionally placed at cattle plot #23444,
   whose room prose says the plot has no beets, as well as at #23448?
   **Recommendation:** Diagnose the authored provenance and reset placement
   while preserving the two-row cap. Keep the accepted supplied-beet request
   valid during any separate builder decision.

8. **Question:** Should Ijale’s native request continue to accept one exact
   beet #23436 for C1000 without a departure requirement?
   **Recommendation:** Preserve the supplied-or-recovered input semantics.
   Do not add greeting, harvest, eating, apple purchase, or source-first
   prerequisites to the existing contract.

9. **Question:** Should successful SEARCH reveal, actual pickup, custody, and
   accepted supply remain separate events for the SECRET beet?
   **Recommendation:** Record only the native successful reveal when its
   visibility and chance checks pass; require committed custody for recovery
   and keep a supplied copy eligible without claiming personal discovery.

10. **Question:** Should eating the beet remain separate from completing
    Ijale’s request?
    **Recommendation:** Keep successful food use and TAG_EATEN history distinct
    from the accepted trade. An attempted EAT or consumed item must not create
    an accepted supply receipt or erase an already accepted result.

11. **Question:** What explicit consent, tools, source materials, and lasting
    plot result should qualify broader farm work?
    **Recommendation:** Author a named work target and beneficiary, then
    require actual admitted contribution and a persistent useful effect.
    Field visits, generic CARVE, pest kills, or the existing beet exchange do
    not establish crop, fence, barn, or building repair.

12. **Question:** Should player-facing livestock care, riding, adoption, and
    return be added around the distinct animals and stable keeper?
    **Recommendation:** Specify owner consent, exact living animal, allowed
    interaction, and surviving destination. NPC feeding, tethering prose, or
    reset M/R/F rows do not prove a player mount, pet, or escort.

13. **Question:** Should Swideth’s general store and Jurushka’s clothier become
    priced services when neither has a raw SHP keeper record?
    **Recommendation:** Keep them as descriptive contacts unless a builder
    adds scoped stock, prices, and an actual service endpoint. Preserve the
    three existing Ralg, Ijale, and Bedgar shops and qualify their real stock,
    admission, and transfer separately.

14. **Question:** Should the wizard household, servant, golem, imp, and gnome
    contraption support an authored knowledge, commission, or crafting story?
    **Recommendation:** Name the commissioned action, valid inputs, caller,
    recipient, and useful saved output before treating these themes as
    progression. Presence, generic class casting, and cabbage-construction
    prose do not establish an installed task.

15. **Question:** Is the pickable gate between #23475 and #23484 intended to
    remain open after its ordinary D reset despite the “Locked Gate” wording?
    **Recommendation:** Confirm the intended access policy and preserve current
    challenge until a separately scoped decision defines keys, permission,
    retries, and both-side movement expectations.

16. **Question:** Is the leaf-covered north exit at #23539 meant to be an
    ordinary exit, a concealed passage, or only description?
    **Recommendation:** Resolve the wording against the rawstate-0 exit and
    key field before changing it. The current record installs neither a door
    nor a SECRET flag from that text alone.

17. **Question:** Should pond #23452’s finite DRINKCON remain ordinary water
    use, separate from pond or habitat restoration?
    **Recommendation:** Preserve the defined drink/fill and depletion behavior.
    Add a distinct consented stewardship action and durable beneficiary state
    if restoration is intended; do not infer healing or an infinite source.

18. **Question:** What accepted knowledge and useful outcome should connect
    the faerie ring, brook, forest life, and local magical creatures?
    **Recommendation:** Define consent and an actual ecological or community
    effect with a surviving beneficiary. Friendly stock, visitation, or
    atmospheric descriptions do not establish a reward, shelter, pet, or
    stewardship completion.

19. **Question:** What choices should qualify investigation of the swamp,
    threatening creatures, crevasses, caves, and hunter routes?
    **Recommendation:** Identify intended noncombat and combat responses,
    admitted passage, return conditions, and saved beneficiary outcomes.
    Creature defeat, shelter prose, and scenic route clues do not prove safe
    travel or village protection.

20. **Question:** What evidence should make any wider Mosswood story READY,
    accepted, and replay-safe?
    **Recommendation:** Bind each authored objective to typed source,
    contribution, movement or controller, recipient, durable result, and
    accountable renewal where relevant. Keep daily policy disabled and the
    native beet request independently available until separate active READY
    qualification establishes actual arrival, admission, save, recovery, and
    replay behavior.

Checkpoint 292 completes twenty builder and owner questions for Mosswood.
It adds no WLD route or candidate row, changes no source disposition, and
does not claim a live encounter, accepted wider story, or persisted outcome.