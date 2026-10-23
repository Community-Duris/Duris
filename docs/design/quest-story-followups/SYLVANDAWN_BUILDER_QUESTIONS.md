# Sylvandawn builder questions and recommendations

These questions retain the current source evidence without inventing a quest
or deciding ownership. Review alongside
[SYLVANDAWN_AUDIT_REVIEW.md](SYLVANDAWN_AUDIT_REVIEW.md).

1. **Map owner:** Who owns Sylvandawn's five source files and the editor/export
   path that produced them? **Recommendation:** keep all five files RETAIN IN
   PLACE until the map owner confirms the source of truth.
2. **Manifest status:** Why is `sylvandawn` absent from current `areas/AREA`
   while its files and code references remain? **Recommendation:** record the
   owner-approved manifest history before proposing activation or archival.
3. **Temple room-number collision:** How should Sylvandawn #8100–#8202 remain
   distinct from active Temple of Laduguer's same-number rooms? **Recommendation:**
   preserve the 103 different records and titles as separate identities.
4. **Seventeen approaches:** Are the 17 routes from Sylvandawn-only source
   rooms into Temple rooms intentional legacy approaches? **Recommendation:**
   retain them as source-only evidence; do not call them active ingress.
5. **Two hundred six shared-number transitions:** Which map owns each source
   and target in the 206 transitions starting from IDs shared with Temple?
   **Recommendation:** require per-room owner evidence before classifying any
   as travel.
6. **Duris3 destinations:** Are #8087 west to #163980 and #8325 east to
   #164082 still intended links to the unregistered Duris3 map?
   **Recommendation:** preserve both tuples and qualify the destination owner
   and return path separately.
7. **Local reachability:** What is intended for the three local graph
   components and the seven rooms not reached by a directed walk from #8001?
   **Recommendation:** ask the mapper whether those records are deliberate;
   do not infer a broken or safe route from topology alone.
8. **Doors and reset mode:** Are the 58 door commands and reset mode 2 still
   the intended operating state? **Recommendation:** keep the authored door
   and reset data pending a separate gameplay qualification.
9. **Story arc:** Is the elven city meant to remain environmental world
   fiction without a native quest? **Recommendation:** preserve current
   descriptions and do not create an objective or reward by inference.
10. **Prose references:** Do the Mazzolin star and Icecrag list mentions
    describe this same Sylvandawn identity? **Recommendation:** retain them as
    narrative references until their owners confirm any intended connection.
11. **Mobile #1253:** Are the two source-only reset rows at ZON lines 452 and
    626 meant to place active Heavens archer #1253?
    **Recommendation:** preserve the typed cross-area reference and confirm
    its population owner; maintained code also binds #1253 to `archer`.
12. **Mobile #666:** Is the reset at ZON line 627 intentionally using active
    Heavens mobile #666? **Recommendation:** verify the external mobile and
    its intended population role without copying or renumbering it.
13. **Unresolved object reset VNUMs:** What should the 38 G rows across 31
    currently undefined OBJ VNUMs represent? **Recommendation:** reconcile
    each typed object reference with an owner before dropping, substituting,
    or creating prototypes.
14. **Imported objects:** Are Lizard sign #6002, Orrak mask #9017, Kobold
    bread #1403, Realm sword #14001, Dragonnia emerald #6834, Plane of Fire
    potions #25400/#25401, and the other cited objects intentional external
    dependencies? **Recommendation:** preserve exact source-only typed
    references and their existing audit links.
15. **Shop records:** Should the 26 shop records remain associated with
    Sylvandawn's local keepers? **Recommendation:** keep the SHP source and
    resolve the shop owner before changing keeper or product VNUMs.
16. **Fallback shop stream:** Outside mini mode, is glob-loading every
    `areas/shp/*.shp` file when `areas/world.shp` is absent intentional,
    including unlisted Sylvandawn shops? **Recommendation:** confirm the
    loader/build contract before relying on manifest exclusion to exclude
    this SHP source.
17. **Undefined shop products:** What are the 29 product rows without a
    current OBJ source definition, including values that numerically overlap
    candidate MOB IDs? **Recommendation:** resolve them by typed OBJ ownership,
    never by cross-type number coincidence.
18. **Temple shop products:** Are the eight Sylvandawn shop products that
    reference active Temple objects still intended? **Recommendation:** keep
    them source-only and do not describe them as live stock.
19. **Legacy code bindings:** Should the candidate mobile handlers, five room
    specials, hometown tables, and #8200/#8113 guard cases still target
    Sylvandawn identities? **Recommendation:** have the code and map owners
    reconcile these numeric bindings together before any source movement.
20. **Disposition authority:** Who can confirm the current editor, loader,
    runtime, and persisted-identity use of Sylvandawn? **Recommendation:**
    retain the source in place and require a separate owner decision before
    activation, merge, move, or archive.
