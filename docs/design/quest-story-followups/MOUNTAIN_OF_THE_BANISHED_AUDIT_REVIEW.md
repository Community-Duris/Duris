# The Mountain of the Banished: connectivity, history, and dependency reconciliation

Checkpoint 109 reconciles the existing source-comprehensive Mountain dossier with the full-world graph, inactive-WLD approaches, followed file history, typed references, and candidate-specific retention plan. The existing dossier remains the canonical account of the three returns, environmental prose, spoken access, ENTER travel, services, and bounded NPC circulation.

## Active graph and route types

The active manifest lists `mount` at [areas/AREA line 52](../../../areas/AREA#L52); the [ZON header](../../../areas/zon/mount.zon#L1) names zone 91. Its WLD has 67 rooms and 135/135 resolving exits: 134 local and one outbound. The raw local weak components are 46, 15, 5, and 1 room. Starting at #9100 and treating authored exits plus all six fixed ENTER objects as available reaches 64/67 rooms; administrative #9157, #9165, and #9166 remain outside. This static model does not establish current stock, door state, NPC behavior, or safe player traversal. The existing [source-comprehensive dossier](../zone-stories/THE_MOUNTAIN_OF_THE_BANISHED.md) covers all 72 QST lines, three accepted returns, the sidecar stories, and action-level questions.

The ordinary boundary is reciprocal: Mountain #9100 down to Surface #517517 and Surface #517517 up to #9100 ([exit rows 184 and 1345](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L184), [pair rows 296 and 552](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L296)). Two other active incoming edges are one-way load-room paths: Juiblex #87673 down to #9106 and Scorch Valley #71326 down to #9122. Their room prose identifies NPC loads, and neither has a reciprocal exit ([Juiblex exit row 1089](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1089), [Scorch Valley exit row 852](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L852), [pair rows 214 and 418](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L214)). Preserve the distinction between these and the Surface entrance.

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits with no parse issues or source-room VNUM overlap. It found two non-overlapping source-only approaches to entrance #9100: inactive `oldmap8.wld` room #184183 east and inactive `surf.wld` room #256676 west ([oldmap8 source](../../../areas/wld/oldmap8.wld#L87556), [Surf source](../../../areas/wld/surf.wld#L1189272)). Neither is active ingress. The reviewed Surf subset rises from 134 to 135 and the curated source-only total from 1,521 to 1,523.

## Typed dependencies

The local reset file imports four externally defined objects: fountain #72 and potion #371 as room stock, plus epic stone #359 and memory #55444 as mobile-carried stock ([mount.zon reset entries](../../../areas/zon/mount.zon#L45)). Bounded all-area scans found no external exact QST item command, SHP product, ZON reset-role reference, typed story-sidecar VNUM, external MOB/OBJ duplicate, or target among 866 external type-25 declarations. One broad sidecar token match is Alatorin ID `exchange-83342-7f7e9148c733`; its #9148 substring is not a VNUM field ([source line](../../../areas/story/alatorin.story.json#L3193)).

A direct compiled-source scan found two actual uses of local object #9114 in standard and enhanceable shaman kits ([first kit](../../../src/account/chaos_eq_data.h#L140), [second kit](../../../src/account/chaos_eq_data.h#L390)). Other raw number matches are fractional math-table entries, shop-refusal IDs, and the Alatorin story-ID substring. No local literal callback or epic-teacher binding was selected. These findings supplement the existing dossier's exact reset and outside NPC source/recipient review.

## Title, followed history, and disposition

After color-code removal, “The Mountain of the Banished” has one exact owner among 351 active non-empty ZON titles. Similar mountain titles, including Mountain Tracts, are separate active areas. Followed `mount.wld` history includes the 2009 area-path relocation and same-stem edits; `mount.zon` has same-stem format, quest-item-flag, and 2026 equipment/ambassador updates. The `mount.story.json` journal/spoken-route sidecar was added on 2026-10-06. The followed paths show no explicit predecessor, alternate-stem rename, or replacement claim. The inactive approach names do not prove supersession.

Mountain remains **DO NOT MOVE** as an active manifest contributor with a reciprocal Surface route, two one-way load-room dependencies, four imported objects, and compiled kit consumers. Keep `oldmap8`, `surf`, and `surface2011` **RETAIN** until provenance and map ownership are resolved. Ask map owners whether the two inactive approaches remain intended geography and who maintains them; no source move or native repair is part of this checkpoint.

## Source references

- [Three native QST returns](../../../areas/qst/mount.qst#L1), [reset imports](../../../areas/zon/mount.zon#L41), and [existing dossier](../zone-stories/THE_MOUNTAIN_OF_THE_BANISHED.md).
- [Mountain entrance](../../../areas/wld/mount.wld#L1), [Juiblex load room](../../../areas/wld/juiblex.wld#L3624), [Scorch Valley load room](../../../areas/wld/scorchvalley.wld#L2798), and [Surface entrance](../../../areas/wld/surface.wld#L367762).
- [Coverage row](ZONE_COVERAGE_REGISTER.csv), [connectivity report](WORLD_CONNECTIVITY.md), [history register](DUPLICATE_AND_REPLACED_ZONES.md), [candidate ledger](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv), and [attic plan](ATTIC_MIGRATION_PLAN.md).

No native gameplay code, test, build, database, migration, or operational input was changed or run for this documentation checkpoint.


## Checkpoint 293: all-442 title and direct builder questions

A fresh normalized scan of all 442 current .zon files gives “The Mountain of
the Banished” one exact owner: active stem mount. The [source dossier](../zone-stories/THE_MOUNTAIN_OF_THE_BANISHED.md)
and [Checkpoint 109 supplement](MOUNTAIN_OF_THE_BANISHED_AUDIT_REVIEW.md)
remain the source for its three native returns, spoken seals, fixed ENTER
objects, imported services, NPC circulation, administrative rooms, and
bounded same-stem history. These twenty questions preserve those findings
while naming the remaining owner choices and gameplay qualification gates.

1. **Question:** Should active stem mount remain the authoritative owner of
   “The Mountain of the Banished”?
   **Recommendation:** Keep the unique normalized title owner among all 442
   .zon files and preserve DO NOT MOVE. Resolve broad command-target aliases
   and semantic map identity as separate owner questions.

2. **Question:** Should the reciprocal Surface entrance and the one-way
   Juiblex and Scorch Valley arrivals retain their different purposes?
   **Recommendation:** Preserve #9100/#517517 as the ordinary player approach
   and #87673→#9106 and #71326→#9122 as authored outside load-room paths.
   Keep inactive oldmap8 and Surf approaches on their existing candidate
   records; ask map owners about provenance and maintenance without adding a
   route.

3. **Question:** Are administrative rooms #9157, #9165, and #9166 intended to
   remain outside ordinary entrance reach, including the exitless holding
   room #9166?
   **Recommendation:** Confirm intended staff and NPC parking before any route
   or sentinel change. Do not create a player escape or exploration objective
   from the NPC wander path alone.

4. **Question:** Should Q11, Q31, and Q56 remain three independent native
   exchanges, with the four-essence tribute as one accepted receipt?
   **Recommendation:** Preserve the exact offerings and rewards: warangel
   #9106 for mask #9109, mirror case #9107 for snowglobe #9108, and four
   distinct essences for symbol #9132 and visor #9135. Keep all three without
   departure requirements and do not split the tribute into four collections
   or two reward achievements.

5. **Question:** Does Q11 require a live warangel capture or escort, or only
   the exact battered warangel proof #9106?
   **Recommendation:** Keep the existing floor item and accepted item
   exchange distinct from the interrogator mobile, a corpse, combat, capture,
   or escort. Do not add an unsupported live-prisoner prerequisite.

6. **Question:** Should only wager wildmage #9123 at #9152 accept mirror case
   #9107?
   **Recommendation:** Preserve the giver prototype and room identity. Keep
   the broom and beetles as the mage’s stakes, not player rewards, and do not
   let similarly named wildmages or dialogue create another exchange.

7. **Question:** Should the priest continue to accept any supplied set of the
   four exact essence types, independent of who recovered them?
   **Recommendation:** Preserve one copy each of #9117–#9120 as the native
   offering. Keep source lineage and personal boss victories separate from
   valid supplied proof and the accepted four-item receipt.

8. **Question:** Do priest instances #9136 at #9100 and #9163 share one
   exchange identity and mobile reset cap?
   **Recommendation:** Preserve the shared canonical Q and cap-two source.
   Qualify the actual instance and accepted participants without awarding a
   second completion for meeting or paying both copies; NPC F followers do
   not establish a player group outcome.

9. **Question:** Should speaking “Pandora” at the peak continue to clear lock
   and secret state while leaving the door closed?
   **Recommendation:** Preserve the current speech handler and distinguish
   speech eligibility, matching the last exit keyword, shared unlock, ordinary
   opening, and arrival. Pattern #9127 is an ITEM_NOTE clue, not a physical
   key or a required consumed item.

10. **Question:** Should speaking “Lokpan” at the slab remain an optional
    clue action rather than an item turn-in?
    **Recommendation:** Keep stone #9128’s ITEM_TRASH role and the slab’s
    paired door state. Attribute a successful unlock to the actor and both
    reciprocal exits; do not create a READ receipt or key-consumption
    requirement from its inscription.

11. **Question:** Should the six fixed-destination ENTER objects remain the
    authored route endpoints for their three room pairs?
    **Recommendation:** Preserve the non-takeable objects, command, charges,
    and destinations. Record selected object and actual arrival if travel is
    ever credited; do not infer portal-door behavior or a new escape route
    from names.

12. **Question:** Should drinking at imported fountain #72 qualify any
    optional shared spell-service story?
    **Recommendation:** Name the actual spell effect, target, recipient, and
    fountain instance before recording a useful outcome. The callback ignores
    its command argument and rotates shared selection; no mandatory buff
    objective or new daily is defined.

13. **Question:** Should a future epic-stone objective require committed
    touch submission rather than carrying the stone or hearing its echo?
    **Recommendation:** Reuse the submitted transaction and frozen participant
    authority for any optional task. Keep memory #55444, the four local
    essences, and the imported stone #359 as separate typed items and
    outcomes.

14. **Question:** Should epic_stone_absorb remain outside any player objective
    until its post-extraction object access is repaired?
    **Recommendation:** Keep the documented pointer-lifetime issue as a
    separately scoped safety follow-up. Preserve current item balance and
    require owned durable retirement and replay evidence before integrating
    cleanup or promising an absorption result.

15. **Question:** Should the generic random R-reset refusal path receive a
    separate safety repair while the selected griffon source remains valid?
    **Recommendation:** Review null handling on failed rolls and limits in a
    named follow-up. Preserve Pakar’s cap-one, 100-percent mounted griffon
    source and do not conflate a mount relationship with item issuance or
    random-room placement.

16. **Question:** Should outside NPC circulation from Juiblex and Scorch
    Valley remain separate from local player access and local essence proof?
    **Recommendation:** Preserve the two one-way load edges and bounded
    outside stock references. Keep outside NPC drops and their independent
    QST returns from substituting for any of the four local essence types.

17. **Question:** Should references with the same number but different
    prototype kinds remain distinct in any audit or journal output?
    **Recommendation:** Keep object #71248 as a necklace and mobile #71248 as
    Tallin’s bodyguard; the selected four-ring reward is an item. Preserve
    item, mobile, and room namespaces and do not infer a companion reward
    from numeric overlap.

18. **Question:** Should the mask #9109 extra description and clerical wording
    be corrected separately from its reward behavior?
    **Recommendation:** Confirm intended text, then use a separately scoped
    wording change. Keep prototype identity, stats, source flags, and all
    three native exchange terms intact.

19. **Question:** How should mode-one reset, capped sources, imported stock,
    and shared services be described to players between reset generations?
    **Recommendation:** Keep source stock, object custody, imports, and
    accepted history separate. Qualify actual generation, cap/chance,
    parent source, recovery, and no-loss restart/replay before promising
    renewed availability; daily rollover does not create stock or unlock
    spoken doors.

20. **Question:** What evidence should qualify any additional Mountain story
    as READY, accepted, and replay-safe?
    **Recommendation:** Bind each authored action to its actual source,
    controller, recipient, durable effect, and recovery boundary. Keep active
    READY accounting and save/replay qualification distinct from native
    receipts; no played arrival, exchange, speech, ENTER travel, fountain
    effect, mount, cleanup, or outside-circulation result is claimed here.

Checkpoint 293 confirms the normalized title across all 442 current .zon
files and adds twenty builder questions. It adds no route, candidate row,
native source change, or migration clearance.


## Checkpoint 511: separate unregistered Mountain QST

The uppercase `areas/qst/Mountain.qst` is a separate, unregistered source
from active lowercase `areas/qst/mount.qst`. Normal QST aggregation follows
the root `areas/AREA` entry `mount`; it does not select the `Mountain` stem.
The two files use different giver VNUMs and tell different stories: the
active file covers Lokpan's prison, while the unregistered file describes a
dwarven smithing/order quest and references Dungeon object #93001. Similar
mountain wording and nearby same-stem history do not prove replacement,
aliasing, or shared authorship. See the
[Mountain QST audit](MOUNTAIN_QST_AUDIT_REVIEW.md) for the incomplete
quester/item identities, load-path evidence, and owner questions.

CP922 also found that the unregistered file's #210xx mobile and item values
match rooms in active mountaintracks; those room identities establish no
relationship to lowercase mount.
