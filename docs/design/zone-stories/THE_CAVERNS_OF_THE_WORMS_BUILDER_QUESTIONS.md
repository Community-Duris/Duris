# The Caverns of the Worms: builder questions

Canonical key: area:worms. Physical coverage row 350. The
[source-comprehensive dossier](THE_CAVERNS_OF_THE_WORMS.md) and
[repository-wide audit](../quest-story-followups/WORMS_AUDIT_REVIEW.md)
document sixteen exact-item commissions, their source and dispatch rules,
exploration routes, conditional foreign references, and active area boundaries.
These are recommended design decisions, not builder answers or live-play
qualification.

## Explicit builder questions and recommended resolutions

1. **Should Wilms's sixteen equipment commissions remain independent services?**
   The sidecar records sixteen accepted contracts and no shared native
   prerequisite. **Recommend** preserve the independent identities unless a
   builder defines explicit dependencies, terminal proof, and renewal policy.
   [Service map](THE_CAVERNS_OF_THE_WORMS.md#exact-equipment-services)
2. **How should a player intentionally select one commission when bundles
   overlap?** ASK is descriptive; native GIVE chooses the first complete bundle
   matching the offered item. **Recommend** preserve that settlement order unless
   a separately designed selection step is bound to the exact accepted recipe.
   [Dispatch review](THE_CAVERNS_OF_THE_WORMS.md#recipe-dispatch-and-source-provenance)
3. **Should the exact item used to trigger GIVE always be among the consumed
   items?** With surplus same-kind roots, the native search can consume a
   different matching item. **Recommend** qualify intended bulk-trigger
   semantics before changing selection or claiming the offered pointer was
   consumed. [Dispatch review](THE_CAVERNS_OF_THE_WORMS.md#recipe-dispatch-and-source-provenance)
4. **Should each commission continue to require its full exact bundle at once?**
   Native recipes accept all ingredients together, with repeated quantities
   represented as distinct roots. **Recommend** keep the present all-at-once
   contract unless a builder deliberately designs incremental, persisted
   collection.
5. **Should Wilms accept exact supplies from another source or player?** Native
   matching checks item identities and quantities, not personal harvesting
   history. **Recommend** preserve source-neutral acceptance; do not imply the
   actor killed or skinned a worm when only delivery is established.
6. **If a future milestone requires personal gathering, what event should count?**
   The item that triggered the hand-in is not necessarily a consumed item.
   **Recommend** base such credit on the frozen consumed item roots, actor,
   source, custody, and reset generation, with party and transfer rules decided
   separately.
7. **Should completion of all sixteen recipes unlock a set-level campaign or
   achievement?** The current service records are independent and grant no such
   terminal. **Recommend** retain the sixteen accepted receipts without
   deriving a full-set completion from inventory or reward possession.
8. **What stock and renewal promise should builders make for the ingredients
   and Wilms?** The source declares finite reset caps and reset mode 2, but not
   accounting admission or personal availability. **Recommend** describe the
   source and caps without promising a guaranteed or repeatable supply.
9. **Should the reward equipment itself prove an accepted commission?** Rewards
   can be transferred, retained from before, or absent after use.
   **Recommend** keep durable acceptance tied to the exact accepted input/output
   transaction, not later ownership of an equipment prototype.
10. **Should the belt commission have a distinct written acceptance response?**
    Its native accepted outcome has an empty completion reply.
    **Recommend** retain the accepted receipt; decide separately whether to add
    player-facing confirmation so silence is not mistaken for refusal.
    [Belt Q](../../../areas/qst/worms.qst#L170)
11. **Does “curer by trade” promise another healing or tradeskill callback?**
    Wilms's dialogue identifies him as a curer and gives the sixteen recipes,
    but the dossier found no native tradeskill-curer or local literal-special
    assignment. **Recommend** treat the authored equipment services as the
    maintained behavior unless a separate effect and handler are specified.
    [Wilms Q](../../../areas/qst/worms.qst#L1)
12. **Should the entry caption say south instead of west for the Underdark?**
    Room #6900 says west, while its reciprocal active exit is south.
    **Recommend** confirm the one-word text correction and keep the authored
    exits and room ownership unchanged until it is handled as a separately
    scoped content fix. [Entry room](../../../areas/wld/worms.wld#L1)
13. **Is the fixed garbage push switch meant to be only shared door state?**
    Object #6931 opens the south exit from #6937; that does not record an
    individual solve. **Recommend** preserve the native passage behavior and add
    personal completion only with an actor-specific durable event.
    [Access review](THE_CAVERNS_OF_THE_WORMS.md#exploration-and-access-stories)
14. **Are the five worm burrow families and duergar alcove exploration areas,
    rather than ordered clear-the-cavern objectives?** The local sources give
    no boss or completion contract. **Recommend** keep source locations as
    material and exploration guidance; do not infer kills, a cleared cave, or
    safe movement from reaching a room.
15. **What ownership is intended for the unregistered KzKrKeep QST commands
    that name Worms objects #6900 and #6912?** That file is absent from the
    active area list, and the QST loader does not glob it.
    **Recommend** keep those records source-only unless the owning contributor
    and registration are explicitly confirmed.
    [Typed-reference audit](../quest-story-followups/WORMS_AUDIT_REVIEW.md#external-typed-and-code-references)
16. **Is the unregistered KzKrKeep shop file intended to offer Worms equipment?**
    It contains nine matching product VNUMs; the shop loader's fallback can
    glob shop files when areas/world.shp is absent. **Recommend** resolve the
    shop owner, keeper, and deployed input list before claiming either active
    availability or guaranteed unavailability. [Shop audit](../quest-story-followups/WORMS_AUDIT_REVIEW.md#external-typed-and-code-references)
17. **Should Worms remain without a local SHP file?** Its maintained equipment
    exchange is Wilms's QST service, while the similarly numbered KzKrKeep
    stock belongs to an unregistered source. **Recommend** keep the local shop
    inventory at zero unless the builder supplies an explicitly owned local
    shop design.
18. **Should Ixarkon's random teleport arrival at #6900 count as Worms progress?**
    The veil object and handler belong to Ixarkon and can select #6900 among
    twenty-five destinations. **Recommend** keep its travel outcome and policy
    under the owning zone; it is neither a Wilms prerequisite nor proof of a
    Worms commission. [Travel reference](../quest-story-followups/WORMS_AUDIT_REVIEW.md#external-typed-and-code-references)
19. **What role, if any, should the inactive ud_map2 approach and its path from
    the unresolved Shafts target have?** Both are source-only; the active Shafts
    exit remains unresolved. **Recommend** preserve the map distinction and do
    not treat that path as active access or use it to rewrite the Shafts target.
    [Inactive approach review](../quest-story-followups/WORMS_AUDIT_REVIEW.md#unregistered-wld-approach-and-unresolved-shafts-target)
20. **Should the active Lair of the Purple Worm have any authored relationship
    to this differently titled area?** The bounded title/history review records
    no predecessor or replacement. **Recommend** keep active worms and pworm
    as separate owners unless a builder supplies an explicit lore or route link.
    [Title and history review](../quest-story-followups/WORMS_AUDIT_REVIEW.md#title-and-tracked-history)

This review records source-grounded recommendations only. It changes no area
source, quest settlement, switch behavior, travel behavior, or player-facing
text.
