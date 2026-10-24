# Checkpoint 365: Shairak and Smokeveil Forest audit

Physical coverage line 278, `area:smokev`, is reconciled against the existing [source-comprehensive story map](../zone-stories/SHAIRAK_AND_SMOKEVEIL_FOREST.md), which covers its 100 rooms, 74 mobiles, 59 objects, 435 resets, and 43 native interaction blocks. This supplement closes the register's connectivity, typed-dependency, title/history, and builder-question reviews. It also corrects the coverage inventory's shop count from zero to one based on the tracked SHP source.

## Active source graph and inventory correction

The active [smokev WLD](../../../areas/wld/smokev.wld) has 100 rooms (#20200–#20299) and 226 authored directions: 224 local and two to Surface. Both Surface directions are reciprocal: Smokeveil #20201 east to Surface #614187 and Surface #614187 west to #20201; Smokeveil #20297 south to #614586 and Surface #614586 north to #20297. Every direction resolves in the active source set. The local weak graph has one 100-room component, and authored directed traversal from #20200 reaches all 100 rooms. There are no negative or positive unresolved targets, fall markers, or current markers.

All 226 exit keys are zero. Of 15 flagged directions, twelve state-1 directions have exact 100%-chance local D resets in [smokev.zon](../../../areas/zon/smokev.zon). Three state-4 directions have no matching D reset: #20200 north to #20220, #20220 south to #20200, and #20281 down to #20282. Preserve these distinctions; do not infer their reset or gameplay behavior.

The [local shop source](../../../areas/shp/smokev.shp#L1) contains one shop record, #20234, with six listed products: #20217, #20221, #20224, #20225, #20226, and #20245. Its owner is Otsurd, mobile #20234, reset at room #20277. The coverage row previously reported no shop; its shop range and count are corrected to 20234–20234 and one.

## Inactive WLD approach

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 directions without errors. No inactive source room reuses a Smokeveil room VNUM. The only direction into Smokeveil is inactive [Surf room #263026, D3 west to #20201](../../../areas/wld/surf.wld#L1322554). No active WLD owns the source room, so preserve it on existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) as source-only, not active ingress. Surf's reviewed source-only subset rises from 282 to 283, and the curated total rises from 4,848 to 4,849.

## Typed dependencies

Smokeveil is in `qst/AREA`, has its local QST and story sidecar, and has no local type-25 portal prototype. The scan of all 713 active type-25 prototypes found no fixed destination in its 100 rooms.

One active external QST item command references a local Smokeveil object: [Alatorin QST line 6580](../../../areas/qst/alatorin.qst#L6580), `G I 20253`, the Tentabeast heart. The matching [Alatorin story-sidecar entry](../../../areas/story/alatorin.story.json#L3644) declares that same item VNUM for Ravi's separate contract. Keep that foreign progression terminal distinct from Smokeveil's two local consumers of the heart.

Active foreign ZON sources contain 19 exact G reset references to Smokeveil object prototypes: eleven in [Raxthan](../../../areas/zon/raxthan.zon#L86) (#20213, #20217, #20203, #20224, #20225, #20246, #20221, #20222, #20227, #20240, #20243), one in [Tezcat](../../../areas/zon/tezcat.zon#L245) (#20255), and seven in [Valdrak](../../../areas/zon/valdrak.zon#L106) (#20204, #20221, #20222, #20224, #20227, #20240, #20243). The Tezcat reset follows Diabolus #98961, the documented carrier for Grishnak's head. No active foreign reset role references a Smokeveil room or mobile VNUM.

The local [Smokeveil shop](../../../areas/shp/smokev.shp#L1) lists six object prototypes. Two other active shops list additional local objects: Raxthan shop #42963 has ten products (#20217, #20203, #20224, #20225, #20246, #20221, #20222, #20227, #20240, #20243); Valdrak shop #53156 has seven (#20204, #20221, #20222, #20224, #20227, #20240, #20243). These are exact source listings; they do not prove a live stock or transaction.

A type-aware source scan found these maintained code references:

- Chaos starter-kit profiles load local object #20201 (blacksmith hammer), #20205 (Tentabeast ring), and #20254 (sleeves) through the account-entry path in [nanny.c](../../../src/account/nanny.c#L706), using entries in [chaos_eq_data.h](../../../src/account/chaos_eq_data.h#L115).
- The smith table registers mobile #20240, Dorno, in [tradeskill.c](../../../src/economy/tradeskill.c#L206). The epic teacher table registers mobile #20242, Keebo, for Improved Track in [epic_skills.c](../../../src/classes/epic_skills.c#L194). These are mobile references even though separate Smokeveil OBJ prototypes use the same numeric VNUMs.
- The maintained but unassigned `seas_coral_golem` handler in [specs.mobile.c](../../../src/specs/specs.mobile.c#L529) refers to rooms #20200/#20201 and object #20202. The local #20202 object is spiked leather armor, while the handler comment calls it an abalone earring. The function is declared and defined but has no active special assignment; do not count it as a live Smokeveil travel gate.

The bounded scans found no other external QST item command, story-sidecar item reference, active fixed portal destination, or direct C/C++ VNUM reference outside these typed cases. Preserve prototype type when reading repeated numbers; the active room, mobile, and object ranges overlap numerically inside this area.

## Title, same-type identity, and tracked history

After color-code removal, terminator trimming, whitespace normalization, and case folding, “Shairak and Smokeveil Forest” occurs once among all 442 current ZON titles and once among the 351 active contributors. It is distinct from the Para-Elemental Plane of Smoke. Same-type scans across current active and source-only files found no other WLD, MOB, or OBJ header using a Smokeveil VNUM.

Followed WLD, MOB, OBJ, ZON, QST, and SHP paths remain on the `smokev` stem and reach the 2009-08-12 areas-tree move. WLD history reaches 2013; MOB and SHP reach 2014; OBJ and ZON include the 2015 quest-item-flag update; QST history reaches 2013. The `smokev.story.json` sidecar was added on 2026-10-02. No alternate-stem rename, distinct predecessor, or whole-zone replacement appears in the bounded paths or current manifest. Keep active `smokev` DO NOT MOVE.

## Builder questions and conservative recommendations

The existing source map records the detailed story and capability boundaries. These twenty questions make the remaining builder decisions explicit while preserving current source identities and receipts.

1. Should the two reciprocal Surface boundary pairs remain the ordinary forest entrances and exits? Preserve both exact pairs until an owner defines another route.
2. Should Surf's #263026 west direction remain source-only? Keep it on candidate row 97; do not add active ingress from an inactive map.
3. Are the state-4 #20200/#20220 directions intended as closed barriers without resets? Keep their authored state until the intended open/close behavior is specified.
4. Is #20281 down to #20282 intentionally state 4 without a D reset? Retain the record and require an explicit builder state decision before changing it.
5. Should the twelve state-1 doors continue resetting closed? Preserve the twelve current door directions and exact resets unless the route design changes.
6. Should the derived catalog span beginning at #20148 reserve rooms before actual source room #20200? Keep the actual WLD range at #20200–#20299 and do not create missing rooms from the catalog span alone.
7. Should the heart-of-Sanrabb and heart-of-Dolgar request stay a single simultaneous offering despite sequential prose? Keep the native two-item contract; revise prose if staged delivery is intended.
8. How should Ivar remain available for both his heart request and separate talon revenge? Define giver availability or renewable episodes before relying on both receipts in one visit.
9. Should Ivar's disappearance after either reward remain the terminal behavior? Preserve the native removal until an explicit repeat or branch policy is approved.
10. Should Oystein keep accepting the exact Grishnak head regardless of how it was obtained? Preserve supplied-proof semantics; do not infer a personal kill.
11. Should either Dorno dragon commission retire Dorno and block the other commission? Keep the two independent exact contracts and decide recipient availability before making both reliably completable.
12. Is Xertrath's preloaded head meant to prove the bunny hunt without personal-kill history? Preserve the reset and separate any future kill requirement as a new, explicit predicate.
13. What event completes the full Tarlator-then-Raltron family? Define the two receipts and branch/attempt policy; do not infer a transformation, letter, or reunion from their dialogue.
14. How should the limited Tentabeast-heart stock be allocated across Tarlator, Ilvorntas, and Ravi? Preserve distinct exact consumers and document supply before promising all routes at once.
15. Should Forvos's repeatable whiskey service stay separate from curing alcoholism or reconciling the family? Keep its current service receipt and no achievement/daily credit.
16. Is the exact dead-animal object #20211 still the intended Azcatlipoca offering? Preserve its type-specific predicate; do not generalize it to any corpse or add a freshness rule without design.
17. Should Otsurd's shop receive renewable stock support? Preserve shop #20234 and its six products; qualify reset admission and replenishment before promising availability.
18. Should Dorno's legacy smith purchases become usable? Resolve the table/menu and player-ore custody defects before enabling purchases or recording a service stage.
19. Should Keebo's Improved Track teaching be a standalone service? Keep the teacher table distinct from the native bunny-quest reward and require a successful teaching receipt if later journaled.
20. Should the stale `seas_coral_golem` handler be repaired and assigned, or remain unused? Preserve its current unassigned status until its item identity, directions, and owner are decided.

This documentation-only review did not run builds, tests, a server or gameplay session, database work, or operational scripts. Live supply, source admission, recipient availability, accounting settlement, service transactions, and the unassigned coral-golem path remain outside static qualification.
