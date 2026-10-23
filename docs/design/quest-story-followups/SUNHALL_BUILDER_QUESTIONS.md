# sunhall builder questions and recommended resolutions

Checkpoint 533, source snapshot `c500ac51b8cd26540943257e7ca4e9807263ef74`.
These questions concern the unlisted Dark Sun Guild Hall, its Underworld3
and Bastard number collisions, the Duris3 outlet, Player Castles imports,
shop stock, and a same-key fallback shop record. They do not authorize
activation, source changes, movement, merging, or archiving.

## Explicit builder questions and recommended resolutions

1. **Who owns the five `sunhall` files and can decide their status?**
   Recommended resolution: identify the area, shop, deployment, and tooling
   owners before proposing any change.
2. **Is “Dark Sun Guild Hall” the canonical title, and what aliases are
   approved?** Recommended resolution: ask the source owner to record the
   approved title and aliases; uniqueness alone does not settle intent.
3. **Why is `sunhall` absent from both active area lists?** Recommended
   resolution: confirm intended full-area and QST loader status with the
   manifest/deployment owner before changing either list.
4. **Who owns the ten pyramid-room identities #30200–#30209, all of which
   currently resolve to Underworld3 mining tunnels?** Recommended
   resolution: document the competing map ownership and preserve both
   namespaces until the builders resolve the collision.
5. **How does Sunhall relate to unlisted `bastard`, which shares header
   #302 and rooms #30200–#30204?** Recommended resolution: confirm the two
   area histories and intended relation; shared numbers do not prove one
   map replaces the other.
6. **Should D4 up from #30200 lead to Duris3 #158571, “Mountains”?**
   Recommended resolution: confirm the endpoint with both map owners and
   state whether the Sunhall entrance is meant to open there.
7. **Is the #158571 outlet intentionally one-way?** Recommended
   resolution: identify a return route or document that no return is
   intended; its current Duris3 exits lead to other Duris3 rooms.
8. **Is the 19-exit graph complete with 18 local targets and one outlet?**
   Recommended resolution: have the mapper confirm the authored directions
   and preserve the source graph while review is open.
9. **Do the 22 active Underworld3 exits and four active ZON room placements
   that reuse #30200–#30209 belong only to Underworld3?** Recommended
   resolution: confirm numeric ownership with the Underworld3 owner; these
   references currently resolve to its identities.
10. **Are four Sunhall mobile prototypes and five reset rows intentional,
    including two repeated #30203 resets?** Recommended resolution: have
    the mobile owner confirm descriptions, placement, and reset limits.
11. **How should Sunhall's #30200–#30202 prototypes be distinguished from
    Bastard's #30200–#30202 records?** Recommended resolution: record the
    owner and current purpose of each same-number prototype before any
    merge or activation.
12. **Are local objects #30200–#30202 intentionally defined but not placed
    or stocked?** Recommended resolution: have the object owner confirm
    whether these definitions are dormant props or incomplete content.
13. **Are Player Castles #11058, #11108, and #11013 meant in rooms
    #30200, #30204, and #30206?** Recommended resolution: preserve all three
    O rows and confirm each placement with both area owners.
14. **What current object is intended for imported #6089, which has no OBJ
    prototype?** Recommended resolution: identify its owner and restore or
    correct the typed reference only with owner confirmation.
15. **Should #53 resolve to the active Heavens rope, or one of the distinct
    Dalvik, Hostel, or Mavulsk items?** Recommended resolution: ask the
    affected item owners which identity the reset and shop mean.
16. **Are Heavens items #108/#109 intended instead of the alternate Mavulsk
    coat and sign?** Recommended resolution: have the shop, Heavens, and
    Mavulsk owners confirm the intended product identities.
17. **Were Heavens bags #491–#494 intentionally added to both Sunhall's ZON
    and SHP in September 2026?** Recommended resolution: confirm the change
    with the bag and area owners and preserve its source history.
18. **Why do `sunhall.shp` and `bastard.shp` both use shop key and keeper
    #30201 while describing Ahash and Ilixctthu in different rooms?**
    Recommended resolution: ask the two area/shop owners how these records
    should coexist in the all-file fallback.
19. **Should full-mode shop loading see keeper #30201 when no active MOB
    matches, and should `in_room` #30205 resolve to Underworld3?**
    Recommended resolution: have shop, code, and deployment owners review
    the missing keeper and negative-index path before any activation.
20. **Do private editor, export, saved-state, or deployment records preserve
    Sunhall or its VNUM references?** Recommended resolution: recover that
    provenance and keep all five files IN PLACE until a separate owner
    decision is documented.

See the [sunhall static audit](SUNHALL_AUDIT_REVIEW.md).
