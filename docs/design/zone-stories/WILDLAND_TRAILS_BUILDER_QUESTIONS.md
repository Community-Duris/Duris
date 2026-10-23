# Wildland Trails builder questions

Canonical key: `area:wildland_trails`. Physical coverage row 346. The
[environmental review](WILDLAND_TRAILS_STORY_REVIEW.md) and
[bounded audit](../quest-story-followups/WILDLAND_TRAILS_AUDIT_REVIEW.md)
document the terrain, tribal camp, map identities, resets, and special
procedures. These questions collect the remaining content and owner decisions.

## Explicit builder questions and recommended resolutions

1. **What is the intended current entrance and safe exit for Wildland Trails?**
   No active WLD edge connects it to another loaded area; two exits point at
   inactive map candidates. **Recommend** confirm a live route before
   describing the area as reachable or adding a new connection.
2. **Which source owns target rooms #215745 and #217245?** Both are in inactive
   Surf and Surface2011, but Surf describes Quistery and brush while Surface2011
   describes a storm sea. **Recommend** decide by room identity and history,
   not VNUM alone; preserve candidate rows 97 and 98 until resolved.
3. **Is “WD bypass road” an intended alias for “The Wildland Trails”?** The
   phrase occurs in the special-assignment comment, while the unique active
   ZON title is different. **Recommend** record any author-approved alias
   separately from title or replacement history.
4. **Should the northern scrub trail feel nearly abandoned and unusually
   silent?** Prose describes a worn but overgrown route with few animal sounds.
   **Recommend** retain that eerie transition unless the builder confirms it is
   leftover text.
5. **Which tribal spelling is authoritative: Welgurd or Welgund?** Both appear
   in mobile/object text. **Recommend** confirm the canonical spelling and
   correct only the inconsistent authored references.
6. **Is the river settlement meant to imply a dependable fishing supply?**
   Fishermen carry spears and several reset groups place fish stringers.
   **Recommend** keep fixed reset placements distinct from a live fishing
   system or guaranteed stock.
7. **Is the lack of local QST and story-sidecar content intentional?** The camp,
   outpost, cavern, totem, and pit imply several leads but no local receipts.
   **Recommend** preserve them as environmental story unless the builder
   chooses to author a separate quest with explicit ownership and proof.
8. **How is the closed/secret route to the Decaying Pit intended to be found?**
   The pit and corpse are present behind a reset-controlled exit. **Recommend**
   supply a deliberate clue or confirm the route is intentionally obscure
   before describing discovery as guaranteed.
9. **Is carved wooden key #14600 intended to be the only key for the Quistery
   storage-room doors?** It is reset in the pit and both locked door directions
   use that VNUM. **Recommend** retain this exact connection and confirm key
   availability and any replacement policy.
10. **Should crate #14602's several padlocks be an actual obstacle?** It is
    reset in the storage room with a bracelet and crystal rod inside, while the
    container description emphasizes locks. **Recommend** align the prose,
    container state, and intended key path so the reset contents are not
    mistaken for guaranteed access.
11. **Are the camp's closed tent doors intended to be player-openable?** Both
    sides of the fisherman, family, chieftain, and spiritist tent exits have
    closed reset states. **Recommend** preserve authored privacy boundaries and
    clarify any expected invitation or access clue.
12. **Is the Welgurd chieftain's runed axe meant to have a story endpoint?**
    Its prose calls it an heirloom whose power belongs to the rightful ruler;
    the local zone has no native contract for it. **Recommend** keep the
    inheritance claim as lore unless a reviewed exchange or effect is intended.
13. **Is the Spiritist's bone totem and medicine pouch meant to offer a service?**
    The item text describes protection as a tribal belief, while the local
    procedure is combat-oriented. **Recommend** keep belief, item possession,
    and any future healing/protection service distinct.
14. **Should the Spiritist be able to hide a player's weapon after disarming
    it?** One random procedure branch adds `ITEM_SECRET` and drops the weapon in
    the room. **Recommend** confirm recovery behavior and item-safety intent
    before presenting the effect as reliable or harmless.
15. **Are the thorn, strangler, and red-musk effects intended as the main
    hazards of the scrub?** Their specials can poison, paralyze, or blind after
    chance/save checks. **Recommend** qualify these as source behavior and
    review encounter fairness separately from the descriptive prose.
16. **Should the thick vine #14624 share one of the other vine procedures?** It
    has thorn/root prose but no corresponding special assignment in the checked
    table. **Recommend** preserve the current distinction unless an owner
    confirms it is accidental.
17. **Should vine death remove the paralysis applied by #14602?** The code
    contains a cleanup branch after it returns on nonzero commands, including
    `CMD_DEATH`. **Recommend** review the branch order and intended duration
    before making any live cleanup claim.
18. **How much of the ruined outpost's caravan history is intended as fact?**
    The room prose only suggests a former route, perhaps to Daggerford.
    **Recommend** retain “perhaps” until a builder supplies firmer historical
    evidence.
19. **What is the intended story of the adventurer in the Decaying Pit?** The
    corpse's gear, wounds, compost, and root network imply a recent death but
    no named culprit or recovery task. **Recommend** leave the cause open unless
    another authored source establishes it.
20. **Does the threat progression reflect the intended ecology of the
    corridor?** Resets place wildcats, grouse, horses, and antelope across the
    plains, gnolls in the ravine, worgs/verbeeg in caves, and hostile plants in
    scrub. **Recommend** preserve these habitat groupings and confirm encounter
    density before changing placements or access.
