# Alterian Mountains: builder questions

Canonical key: `component:alterian_mountains`. Physical coverage row 358. The
[source dossier](ALTERIAN_MOUNTAINS.md) and [static audit](../quest-story-followups/ALTERIAN_MOUNTAINS_AUDIT_REVIEW.md)
capture source-backed decisions about the mountain map, Mining's overlapping
room range, and shared Wilderness resets. These recommendations do not prove
the unregistered files are loaded.

## Explicit builder questions and recommended resolutions

1. **Should this source remain unlisted?** `areas/AREA` lists active Mining,
   while the Mountain WLD/MOB/OBJ/ZON files are excluded. **Recommend** retain
   the current source-only status until the loader owner identifies the
   intended set of area files.
2. **Which zone owns header number 49?** Both `mining.zon` and this ZON use
   49, but have different titles and resets. **Recommend** keep their zone
   identities separate until reset ownership and uniqueness rules are
   confirmed.
3. **Who owns the 49 overlapping room VNUMs #4920–#4968?** All same-number
   Mountain and Mining WLD records differ. **Recommend** treat each map as a
   distinct source and assign ownership per room before co-loading or moving.
4. **Which #4919 is intended by the Mountain trail?** The target has a
   Mountain Wilderness “Mountain Trail” record and a separate active Mining
   service-tunnel record. **Recommend** decide its map identity and intended
   connection before describing a route as active.
5. **Should the source-only Wilderness connections stay linked?** Mountain
   #4920 leads west to #4919 and #4980 leads down to #4750. **Recommend** keep
   both source references attached to the Wilderness map pending a joint
   loader and VNUM decision.
6. **Are the three exits into Mining-numbered rooms intended?** Mountain
   #4969, #4981, and #4986 target #4968, #4924, and #4925. **Recommend** decide
   whether each points into Mountain's own same-number room or active Mining.
7. **What route and return behavior should the lake, mountain passes, and
   Mining boundary provide?** Static exits do not establish that a traveler
   can use or safely return from them. **Recommend** verify both maps' intended
   route and boundary ownership before claiming access.
8. **Should Keeper's Dale be reachable from the root trail?** A local directed
   sweep from #4920 does not reach #4995–#4999, although the rooms can reach
   back toward the root. **Recommend** confirm whether the concealed dale is
   intended to require an alternate Wilderness approach.
9. **Which climb rules apply to the risky scrambles and cracks?** Rooms #4954
   and #4983 describe precarious climbing. **Recommend** qualify movement,
   fall, and return behavior before promising a safe passage.
10. **Is the warm mountain lake meant to reveal a geothermal fissure?**
    #4969–#4977 describe clear, steaming water. **Recommend** preserve the
    clue while the builder decides whether it has a playable source or remains
    environmental prose.
11. **What inhabits the bone-filled lair?** #4956 and #4957 describe an animal
    cave, humanoid bones, and a dark chamber. **Recommend** confirm the intended
    creature and whether the current MOB records support that story.
12. **What is the hot cave's relationship to the Vertical Cavern?** #4980
    descends to source-only Wilderness #4750. **Recommend** resolve the shared
    map and steam-source story across both files before exposing the route.
13. **Were the large doors at Keeper's Dale meant to open?** #4999 east stores
    target `-1`, while its prose describes metal doors. **Recommend** confirm
    whether this is a deliberate dead end or an unfinished endpoint before
    adding a destination.
14. **Should the Mountain resets import Wilderness mobiles #4812 and #4813?**
    The 13 M commands reference those prototypes, absent from this MOB file
    but present in `alterian_wilderness.mob`. **Recommend** keep both source
    definitions together until reset ownership is resolved.
15. **Should the E reset import Wilderness object #4806?** The Mountain ZON
    references an object missing from its OBJ file. **Recommend** preserve the
    cross-file object dependency and identify its source owner before edits.
16. **Should Wilderness continue resetting Mountain mobiles?** Its ZON has
    30 M commands using 13 Mountain mobile VNUMs. **Recommend** preserve this
    reverse dependency while both zones remain source-only.
17. **Should mobile #4830 retain its compiled `wanderer` callback?**
    `specs.assign.c` binds the callback to the Mountain adventurer. **Recommend**
    keep the prototype and callback identity paired unless the owner selects a
    replacement.
18. **What is the intended role of the reset destinations in #4920–#4968?**
    Sixteen Mountain M-reset fields overlap active Mining room VNUMs.
    **Recommend** keep them out of active reset claims until source ownership
    is settled.
19. **How should object #4900 be protected from the Mining collision?**
    Mountain defines `nothing`; active Mining defines a reset wooden crate at
    the same object VNUM. **Recommend** preserve both records and the active
    Mining reset until a separate object-identity decision is made.
20. **Should the range remain an environmental adventure without QST, shop,
    or story-sidecar contracts?** The same-stem source set defines rooms,
    mobiles, one object, and resets only. **Recommend** keep future tasks or
    services owner-approved and tied to actual source behavior.
