# The Temple to Skrentherlog: builder questions

Canonical key: area:yuan_ti. Physical coverage row 351. The
[source-comprehensive dossier](THE_TEMPLE_TO_SKRENTHERLOG.md) and
[repository-wide audit](../quest-story-followups/TEMPLE_TO_SKRENTHERLOG_AUDIT_REVIEW.md)
document the ranger's one accepted blood exchange, the captive's separate
hammer lead, keyed routes, custom item effects, and foreign memory return.
These questions record recommendations, not builder answers or live-play
qualification.

## Explicit builder questions and recommended resolutions

1. **Does the ranger's accepted blood exchange mean the cure and departure
   succeeded as separate world events?** Native completion records blood #80570
   for scimitar #80572 and departure; the cure remains narrated text.
   **Recommend** keep the receipt scoped to the accepted exchange until a
   separate cure state is persisted. [Ranger Q](../../../areas/qst/yuan_ti.qst#L51)
2. **Must the player personally kill Skrentherlog to complete that exchange?**
   Native GIVE checks the exact loose item, not how it was acquired.
   **Recommend** preserve source-neutral delivery; any personal-kill or
   first-recovery milestone needs its own actor, item, and source evidence.
3. **Can a vial, corpse, other blood, or the scimitar substitute for the exact
   blood?** The maintained contract names only object #80570.
   **Recommend** keep those items distinct and avoid describing them as valid
   offerings. [Source journal](../../../areas/story/yuan_ti.story.json#L9)
4. **Should the ranger remain in the temple after the accepted exchange?** The
   native contract removes her after giving the scimitar.
   **Recommend** treat that as the authored departure only; do not infer a
   separately persisted rescue, destination, or lasting cure.
5. **Does the captive's death dialogue ask the player to kill her or grant
   liberation credit?** The woman has no accepted contract.
   **Recommend** preserve her distress as setting and the stone/priest clue as
   a lead; do not turn ASK topics or a lethal outcome into a quest.
   [Captive dialogue](../../../areas/qst/yuan_ti.qst#L66)
6. **What should a successful hammer strike prove?** Drowcrusher consumes
   hammer #80556 and ground stone #80563 and moves current room occupants, but
   the handler does not explicitly kill Skrentherlog, free captives, or replace
   the temple. **Recommend** keep those outcomes separate unless a committed
   event records each one. [Hammer handler](../../../src/specs/specs.zalrix.c#L73)
7. **Which occupants should move when the hammer action succeeds?** Its active
   loop includes NPCs, bystanders, opponents, and other occupants, while
   movement failures are not checked. **Recommend** decide participants,
   consent, combat, and failure handling before changing that scope; preserve
   the current source behavior as unqualified.
8. **Should the chest containing Drowcrusher stay locked but pickable without
   a configured key?** Chest #80549 is closed/locked with key VNUM zero.
   **Recommend** retain the ordinary pick route and its requirements; do not
   fabricate a key or infer the chest is unreachable from its prose.
   [Chest review](THE_TEMPLE_TO_SKRENTHERLOG.md#keys-pick-access-and-ordinary-routes)
9. **Is key-amulet #80561 meant to function as a key despite its name and wear
   slot?** Its object type is ITEM_KEY and its reset links the approach door.
   **Recommend** preserve its typed key behavior and distinguish that from
   wearing or merely possessing it. [Object source](../../../areas/obj/yuan_ti.obj#L1030)
10. **Are all declared keys intended as mandatory steps for the blood exchange?**
    The shaman, warrior, small priest, and Skrentherlog supply keys for separate
    chests or doors, while the native blood Q has no key prerequisite.
    **Recommend** keep route discovery, key custody, opening, and the accepted
    receipt distinct; existing shared access can change the route.
11. **Should the bone key be required for the hidden treasure-room entrance
    when another side is already open?** The #80544-to-#80545 door resets
    secret/closed/locked, but its reverse exit is open.
    **Recommend** describe the declared key and direction without promising
    that every player must acquire or use it.
12. **Is the rubble route meant to represent a permanently collapsed phase?**
    The prose describes closing tunnels, while maintained exits still connect
    rubble back to the intact entrance. **Recommend** keep the existing map
    paths and ask the builder to select any phase transition separately.
13. **Should the up exit from rubble room #80546 start closed or hidden?** Its
    prototype state is 4, but no D reset overrides it and setup_dir masks
    prototype bits to 3. **Recommend** preserve the currently authored loader
    result until a builder confirms an intended reset/door-state change.
    [Access audit](THE_TEMPLE_TO_SKRENTHERLOG.md#keys-pick-access-and-ordinary-routes)
14. **Is Winterhaven's one-way arrival directly into room #80545 intended as an
    ordinary approach?** Its cross-area exit has no reciprocal Temple edge and
    lies outside the local key chain. **Recommend** preserve it as a separate
    inbound boundary, without adding a guessed gnomish gate or safe-return
    claim. [Boundary audit](../quest-story-followups/TEMPLE_TO_SKRENTHERLOG_AUDIT_REVIEW.md#active-wld-graph)
15. **Should memory #55177 on Skrentherlog remain part of Winterhaven's
    ambassador request?** Winterhaven Q2881 accepts that memory with its own
    items and reward. **Recommend** keep that completion and ownership with
    Winterhaven; the Temple's local blood request does not include it.
    [Winterhaven Q](../../../areas/qst/wh.qst#L2881)
16. **What renewal should the reset-mode-zero epic stone and rare ranger imply?**
    Touch can request a zone reset, but mode zero does not mean never-renewing
    or guarantee fresh blood, key, or ranger stock. **Recommend** keep touch,
    reset identity, ranger availability, and the story-only daily policy
    distinct until an isolated renewal review qualifies them.
17. **Are the two Astral-Tiamat coin-pile resets intended to share Temple
    object #80571?** They reference the same active object prototype but create
    no local quest. **Recommend** preserve both source owners and avoid treating
    the duplicate source use as an extra Temple request. [Foreign resets](../../../areas/zon/astral_tiamat.zon#L76)
18. **Should Squelcher or Dragonslayer effects count as quest protection or
    successful escape?** The equipment has its own silence, attack, breath, and
    combat-removal behavior. **Recommend** keep equipment possession and the
    blood receipt separate from effect success, safety, or a story outcome.
    [Equipment review](THE_TEMPLE_TO_SKRENTHERLOG.md#equipment-touch-availability-and-ownership)
19. **What should the imported epic-stone touch do to this reset-mode-zero
    zone's later stock?** The source-code path can request renewal, and the
    audit records a stable-zone-identity concern without a reproduced live
    wrong-zone reset. **Recommend** qualify an isolated divergent-ID case before
    selecting a separate code repair; do not infer a live reset defect here.
    [Touch audit](THE_TEMPLE_TO_SKRENTHERLOG.md#equipment-touch-availability-and-ownership)
20. **Should the temple remain a separate area from the neighboring
    Yuan-Ti Village?** The active sources have different titles, manifests, and
    a reciprocal boundary. **Recommend** preserve separate ownership and the
    authored village route unless a builder supplies an explicit replacement
    or merged-area decision.
    [Title and history audit](../quest-story-followups/TEMPLE_TO_SKRENTHERLOG_AUDIT_REVIEW.md#title-history-and-disposition)

This source-backed review selects no content repair or runtime behavior. Access,
reset renewal, item admission, combat, and the hammer journey remain outside
static qualification.
