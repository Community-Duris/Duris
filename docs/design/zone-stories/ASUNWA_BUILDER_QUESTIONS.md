# Lair of the Asunwa Jaguar: builder questions

Canonical key: `component:asunwa`. Physical coverage row 363. The
[source dossier](ASUNWA.md) and
[technical audit](../quest-story-followups/ASUNWA_AUDIT_REVIEW.md) map its
story, reset, route, quest, shop, and manifest evidence. The recommendations
below do not establish live map or shop availability.

## Explicit builder questions and recommended resolutions

1. **Is the WLD/MOB/OBJ/ZON component intentionally commented out of
   `areas/AREA` while `asunwa.qst` remains in `areas/qst/AREA`?** The two
   manifests disagree on world versus quest membership. **Recommend** ask the
   map owner to state the intended load set before changing either manifest.
2. **Is the raw alias `asunwa` and title “Lair of the Asunwa Jaguar” the
   intended identity?** The full-area comment preserves the numeric band
   #353–#354. **Recommend** keep the alias and normalized title distinct from
   any editor or runtime name until the owner confirms them.
3. **Should the entrance at #35300 still connect north to Surface #604903?**
   The active Surface room has no return WLD edge to Asunwa. **Recommend**
   confirm intended access and return behavior before enabling the WLD area.
4. **Is Surf room #247317's south exit to Asunwa #35300 intentional?** `surf`
   is source-only, so this is not active ingress. **Recommend** preserve it as
   a separate source edge and resolve Surf/Asunwa ownership together.
5. **Are the 106/108 rooms forward-reachable from the entrance graph
   intentional?** Rare Mob Load Rooms #35406/#35407 form a directed one-way
   approach back to #35311 and receive a rare snow leopard reset.
   **Recommend** retain these staging rooms until the builder confirms
   encounter placement and intended access.
6. **Should the six fall settings and fourteen current settings remain as
   authored?** Fall chances lead from waterfall ledges into the pool and from
   the lower pool lip into the chasm. **Recommend** preserve their exact
   strengths/directions and require an authorized route-safety review before
   describing traversal as safe.
7. **Are all eight door resets intended to match the WLD's closed,
   locked, or secret exits?** The ZON resets all eight at 100% chance.
   **Recommend** keep the paired WLD flags and ZON resets together.
8. **Is jade statue #35307 meant to open the #35349 passage?** The object is
   reset in altar room #35334 and is the WLD key for that door.
   **Recommend** preserve this key relationship while the owner reviews the
   puzzle's placement and access.
9. **Is tiny jade key #35312 meant to open the #35399 secret passage?**
   Balgar carries that key in the ZON. **Recommend** keep the mobile/object/
   exit references together and confirm the route in the intended map.
10. **Should Balgar #35319's QST stay registered while his WLD is not in the
    full-area list?** The separate quest manifest includes Asunwa, but the
    aggregate `areas/world.qst` is absent from this checkout.
    **Recommend** confirm the build's aggregate inputs and the intended
    quester availability before altering either manifest.
11. **Does Balgar's fetch quest still require scroll #35315 and reward the
    ancient jade plaque #35313 plus 10,000 experience?** Those QST commands
    match his dialogue and the ZON placements. **Recommend** preserve this
    source-backed pairing unless the quest owner approves a text/contract
    revision.
12. **Is the plaque meant to remain on Balgar before the quest is completed?**
    The reset gives him #35313, and the QST also lists it as a reward.
    **Recommend** confirm intended inventory/duplication behavior without
    changing the current source contract during a path review.
13. **Should Anan's shop #35320 remain with the source-only world component?**
    It stocks local objects #35311, #35304, and #35301 in room #35374, while
    the shop loader has a source glob fallback. **Recommend** resolve aggregate
    shop inputs and mobile/room ownership before treating the shop as live.
14. **Are the `ddguild` and `player_guilds` shop references to mobile #35300,
    object #35301, and room #35303 intentional?** Each separate SHP record
    uses those numeric IDs. **Recommend** identify their owning shop and keep
    the external records distinct from Asunwa's local reset/shop data.
15. **Is `player_guilds.qst` record #35320 meant to share Anan's mobile VNUM?**
    That record has no text or completion contract. **Recommend** resolve the
    source identity and assignment intent before changing QST registrations.
16. **Should the 107 mobile resets, 23 equipment resets, five gives, four
    follows, and three object placements remain at their current rooms?**
    **Recommend** retain the reset structure until the area owner reviews its
    population and encounters.
17. **Are the same-stem files and map ownership complete?** Tracked histories
    show maintenance edits and no explicit whole-zone replacement; no map JSON
    is present. **Recommend** add any external rename, editor, or replacement
    evidence before a disposition decision.
18. **What external and runtime evidence is required before any path move?**
    The source tree cannot establish live QST/shop boot state, persisted
    instances, or safe travel. **Recommend** keep candidate row 12
    **RETAIN IN PLACE** until the owner resolves both manifests, shop/QST
    references, editor state, and live qualification separately.
