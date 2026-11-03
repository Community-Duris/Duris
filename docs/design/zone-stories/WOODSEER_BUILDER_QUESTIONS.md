# Woodseer: builder questions

Canonical key: area:woodseer. Physical coverage row 349. The
[source-comprehensive dossier](WOODSEER.md) and
[connectivity and dependency audit](../quest-story-followups/WOODSEER_AUDIT_REVIEW.md)
preserve the authored seven requests, local services, active boundaries, and
foreign references. These recommendations record open design decisions; they
are not builder answers or live-play qualification.

## Explicit builder questions and recommended resolutions

1. **Should the seven native requests remain independent?** The checked-in QST
   has seven exact-item returns and no native prerequisites, while some dialogue
   suggests other tasks. **Recommend** keep them independent unless a builder
   explicitly defines shared prerequisites, terminal proof, and reset policy.
2. **Does returning the broken mandolin prove it was repaired or played again?**
   The bard accepts object #16526, receives it, and walks away humming, but no
   repair or performance event is persisted. **Recommend** treat the native
   return as item acceptance only; add a separate committed repair/play outcome
   only if that milestone is intended. [Bard Q](../../../areas/qst/woodseer.qst#L20)
3. **Should the badger-burrow mandolin remain behind the park's hidden boulder?**
   Its maintained object reset is in the burrow, beyond a hidden, closed down
   passage. **Recommend** preserve the declared route and qualify discovery and
   opening separately before describing it as an ordinary or safe approach.
   [Source dossier](WOODSEER.md#complete-source-closure)
4. **Is the farmer's wax lead meant to require first returning honey?** Her
   dialogue mentions finding wax “when you got me the honey,” but the wax request
   has no native prerequisite. **Recommend** make the optional order clear in
   dialogue and keep both receipts independently completable. [Farmer Q](../../../areas/qst/woodseer.qst#L45)
5. **Should the woodworker's reward text mention coins?** The response mentions
   coins, while the maintained reward list grants wooden ring #16576 only.
   **Recommend** confirm whether to correct the prose or separately change the
   reward; do not add a coin payment by implication. [Woodworker Q](../../../areas/qst/woodseer.qst#L81)
6. **Should a request prove personal harvesting or only delivery of the exact
   item?** Native acceptance checks supplied item identity, not its acquisition
   source. **Recommend** keep source-neutral delivery semantics unless a
   personal-source milestone is deliberately designed with transfer, party,
   and durable source evidence.
7. **Does the hive have a required queen-first or cleared-hive progression?**
   Honey, wax, and egg sources appear at separate reset locations, and their
   native returns do not require one another. **Recommend** preserve that
   independence; document a sequence only if builders add enforceable gates and
   terminal outcomes. [Hive source map](WOODSEER.md#seven-accepted-requests-and-exact-declared-sources)
8. **Is the loose bee egg meant to represent a birth, hatching, or simply a
   found item?** The request accepts egg #16547 and its maintained source is an
   object reset above the queen's chamber; the larvae are separate mobiles.
   **Recommend** describe only item recovery unless an actual hatch or birth
   event is added. [Mage Q](../../../areas/qst/woodseer.qst#L100)
9. **Should the wizard's requested feather be distinguished from the outer
   jungle's glowing feather?** The local request names tiny hummingbirds and
   accepts #16544; the outer jungle bird carries a different object, #16925.
   **Recommend** retain the exact local item distinction in clues and rewards.
   [Wizard Q](../../../areas/qst/woodseer.qst#L122)
10. **Should the artist's pigment clues continue to point to river dragonflies?**
    Reset records place the requested pigment on giant dragonflies at river
    locations, not only around the lake. **Recommend** keep the source clue
    aligned with those placements and avoid implying the pigment is guaranteed
    at every dragonfly encounter. [Artist Q](../../../areas/qst/woodseer.qst#L68)
11. **Which turtles are intended to supply the maitre d's meat?** The item reset
    appears on snapping turtles at two maintained placements; the other turtle
    prototypes and placements do not all carry it. **Recommend** preserve those
    exact distinctions in any clue or source-credit design. [Maitre Q](../../../areas/qst/woodseer.qst#L140)
12. **Can bought turtle soup satisfy or prove the raw-meat request?** The waiter
    sells soup #16610, which is also part of the QST reward, while the accepted
    offering is raw meat #16543. **Recommend** keep purchase, consumption, and
    acceptance as distinct events. [Shop file](../../../areas/shp/woodseer.shp#L623)
13. **Are Luc's missing stock entries obsolete or meant to reference restored
    objects?** His shop and zone resets name objects #6070, #6109, and #6110,
    which have no matching maintained object prototypes. **Recommend** confirm
    intended merchandise before editing either shop or reset data; do not infer
    replacements from neighboring IDs. [Luc's shop](../../../areas/shp/woodseer.shp#L42)
14. **Should the fish merchant's salmon remain owned by the separate Surface
    Mini story?** The active foreign request requires salmon #16635 together
    with firebreather #3003 and has its own giver and reward. **Recommend** keep
    that receipt with the existing Surface story; Woodseer's stock is a lead,
    not another copy of its completion. [Typed-reference audit](../quest-story-followups/WOODSEER_AUDIT_REVIEW.md#typed-external-references)
15. **Is Woodseer's up exit from #16500 meant to reach a registered area?** Its
    target #154581 exists in unregistered Duris3.wld, not an areas/AREA
    contributor, and normal generation removes unresolved exits.
    **Recommend** confirm deployed world generation and intended destination
    before a separately scoped route repair. [Room source](../../../areas/wld/woodseer.wld#L1)
16. **Are the river currents and authored falls intended hazards on ordinary
    routes?** The dossier records 44 current rooms and 23 fall-chance rooms,
    including a 100% fall field. **Recommend** preserve their source values and
    document recovery/access limits accurately; source-graph reach does not
    establish safe traversal. [Connectivity audit](../quest-story-followups/WOODSEER_AUDIT_REVIEW.md#active-wld-graph)
17. **Are DeLoran's boots meant to start a return request?** Their object text
    recommends dropping or returning them to DeLoran, but there is no matching
    maintained local QST contract or literal special binding. **Recommend** keep
    it as authored guidance until a builder supplies a giver, accepted proof,
    and outcome. [Boots](../../../areas/obj/woodseer.obj#L1641)
18. **Is the artifact_invisible assignment meant for corpse object #16904?**
    The prototype has no wearable slot while the callback requires worn state.
    **Recommend** confirm whether the binding or intended prototype is stale
    before changing either; do not claim the effect ordinarily activates.
    [Binding audit](../quest-story-followups/WOODSEER_AUDIT_REVIEW.md#direct-compiled-references)
19. **Where should the Woodseer guild guard act, and what should count as its
    insignia?** The local guard case matches room #16501, but maintained local
    births are elsewhere and expected object #9316 is a ruby weapon.
    **Recommend** confirm placement, expected item, and access intent together
    before treating this as an active gate or making a repair.
    [Guard case](../../../src/specs/specs.guards.c#L107)
20. **Should the bartender's world quest, inn, and pet-shop transactions stay
    separate from Woodseer's seven requests?** They have distinct shared
    service and persistence paths, and none is a native local request.
    **Recommend** keep them as independent services unless a builder defines an
    explicit durable trigger, owner, and completion outcome.
    [Service review](WOODSEER.md#custom-services-dormant-bindings-and-foreign-boundaries)

This is a source-backed recommendation review only. It changes no area source,
quest behavior, travel behavior, or player-facing text.
