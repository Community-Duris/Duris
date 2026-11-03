# Alterian Wilderness: builder questions

Canonical key: `component:alterian_wilderness`. Physical coverage row 359.
The [source dossier](ALTERIAN_WILDERNESS.md) and
[static audit](../quest-story-followups/ALTERIAN_WILDERNESS_AUDIT_REVIEW.md)
capture the environmental story, overlapping Mining/Tchan VNUMs, source-only
routes, and typed reset/code dependencies. These recommendations do not
establish that the component is loaded.

## Explicit builder questions and recommended resolutions

1. **Should this component remain outside the full area list?** Its WLD, MOB,
   OBJ, and ZON files are absent from `areas/AREA`. **Recommend** preserve the
   source-only status until the loader owner identifies the intended file set.
2. **Is zone header #47 the intended global identity?** The title is “Alterian
   Region - Wilderness,” and no other current ZON header uses #47.
   **Recommend** retain the current header while the owner confirms reset-zone
   identity and future manifest placement.
3. **How should Wilderness rooms #4801–#4809 relate to active Tchan's
   same-numbered rooms?** All nine paired room records differ.
   **Recommend** preserve the separate map identities and resolve loader/VNUM
   ownership before co-loading or moving either source.
4. **Who owns Wilderness room numbers #4900–#4919?** All 20 numbers also have
   different active Mining room records. **Recommend** keep both maps intact
   and assign each room identity before any merge or shared-map claim.
5. **Are the five source-only exits from #4897–#4899 intended to enter active
   Mining?** Their targets are Mining-numbered rooms, and 17 Wilderness M
   reset fields also fall in Mining's numeric range. **Recommend** preserve
   the five edges and reset fields as source references until the loader owner
   assigns target-map identity.
6. **Should Wilderness #4801 and #4918 connect south to Shadow Forest
   #6368?** Both exits are in this unlisted WLD; their source numbers also
   overlap active Tchan and Mining rooms, respectively. **Recommend** retain
   both source records without describing them as active access.
7. **How should the four unique-origin Tchan approaches be treated?** Exits
   from #4810, #4815, #4821, and #4822 target active Tchan room numbers; 16
   more matches start at overlapping #4801–#4809. **Recommend** preserve the
   four distinct origins and keep the 16 overlapping-origin matches separate
   from the entrance count.
8. **Should the two Alterian source maps stay linked at #4750/#4980 and
   #4919/#4920?** Both pairs have reciprocal source exits, while #4919/#4920
   also have separate active Mining records. **Recommend** retain the
   source-only Mountain/Wilderness links and settle all overlapping room
   ownership before making a live-route claim.
9. **Is the #4915 east exit to Surf #229817 intended to remain reciprocal?**
   Source-only `surf.wld` #229817 points west to #4915.
   **Recommend** preserve both candidate records as a paired dependency and
   confirm their shared loader owner before exposing the route.
10. **Are the three WLD graph components intended?** The steam caves have nine
    rooms, the dark lair has two, and the forest/farm network has 119.
    **Recommend** keep this graph inventory and ask the builder to confirm
    whether item callbacks are meant to provide the missing cave-to-lair path.
11. **Should the iron grates require black iron key #4800?** The #4752/#4753
    exits encode that key, but the ZON sets both door states open.
    **Recommend** confirm the intended lock/reset state before changing either
    the key VNUM or the D commands.
12. **Is portal object #4801 intended to send a traveler to #4759?** Its
    `value[0]` is #4759, the source reset places it in #4758, and code assigns
    `magic_pool`. **Recommend** preserve the value and callback pairing while
    the loader owner confirms that the prototype and target room resolve.
13. **Is portal object #4802 intended to return from #4759 to #4758?** Its
    stored destination is #4758 and it is reset in #4759 with the same
    callback. **Recommend** retain the reciprocal source data and confirm the
    return behavior in the owning runtime before describing it as playable.
14. **Which #4800 mobile owns `newbie_guard_east`?** Wilderness defines a
    brown bear; active Tchan defines a guard and code binds #4800 to the guard
    callback. **Recommend** keep the current callback with active Tchan's
    identity and do not transfer it to the Wilderness bear by VNUM alone.
15. **Should Wilderness object #4800 remain a separate key prototype from
    Tchan's #4800 key?** Wilderness resets its black iron key, while Tchan has
    a different same-number object. **Recommend** preserve both prototypes
    and their source-owned resets until object identity is assigned.
16. **Are the reciprocal Mountain/Wilderness reset imports intentional?**
    Wilderness has 30 M commands using 13 Mountain mobile prototypes;
    Mountain resets Wilderness mobiles #4812/#4813 and object #4806.
    **Recommend** retain both directions of the dependency and resolve
    prototype ownership before editing either ZON.
17. **Should Wilderness mobile #4812 keep the compiled `poison` callback?**
    The source defines #4812 as a displacer beast and `specs.assign.c` binds
    #4812 to `poison`. **Recommend** keep the current source/callback pairing
    intact until the owner confirms the intended loaded prototype.
18. **Should active `bs.zon` continue to request object #4814?** It has two P
    reset commands for #4814, which Wilderness defines as an ear of corn;
    `bs` has no matching local object definition. **Recommend** record this
    typed external dependency and confirm which source should own the resolved
    object before changing either reset.
19. **Should the farmstead remain environmental story without local quest or
    shop contracts?** The source describes a road, yard, well, cornfields,
    shed, and family home, but has no same-stem QST/SHP/story sidecar.
    **Recommend** preserve the authored rural setting and add a task or service
    only if its owner supplies a concrete design.
20. **What external editor, persisted-instance, and runtime evidence is needed
    before a later move or merge decision?** Source history does not include
    those stores and the files are unlisted. **Recommend** keep the component
    RETAIN IN PLACE until the responsible owner resolves those identities and
    confirms any intended loaded routes.
