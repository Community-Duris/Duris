# BGTK shop-source questions

Canonical key: component:bgtk. Physical coverage row 366; attic candidate
row 16. The [technical audit](BGTK_AUDIT_REVIEW.md) records three SHP
records, their active Barrow rooms and products, and the full-mode shop
loader path. This source has no room, mobile, object, zone, or quest text,
so these questions focus on source ownership and typed dependencies.

## Explicit builder questions and recommended resolutions

1. **What area or owner does source stem bgtk identify?** The repository
   contains only bgtk.shp and has no title or area annotation.
   **Recommend** identify the owning map and shop builder before treating
   this as a complete, standalone area.
2. **Are all three SHP records intended to be consumed by full-world boot?**
   The aggregate areas/world.shp is absent, and the default non-mini loader
   concatenates every areas/shp/*.shp file. **Recommend** decide whether
   bgtk should remain in that fallback set or be excluded through an
   owner-approved loader/aggregate change.
3. **Is keeper #20607 intended to be the active Barrow old orc?** The
   prototype exists, but elftomb.zon resets it at #20710 while the shop
   record names room #20630. **Recommend** confirm both mobile identity and
   placement before attaching the shop special to it.
4. **Who should keep shop record #20615?** The VNUM is an active Barrow
   object, the Blood-eye amulet, and no mobile prototype #20615 was found.
   **Recommend** identify the intended mobile VNUM or retire the record only
   after its owner resolves the source.
5. **Who should keep shop record #20647?** No mobile prototype #20647 was
   found, while #20647 is an active Barrow room number.
   **Recommend** resolve the typed VNUM collision and intended keeper.
6. **Are products #20602 and #20603 intended to be sold by BGTK?** They are
   active Barrow objects (bloody meat and a pile of skulls).
   **Recommend** preserve the exact product references until the Barrow and
   BGTK owners confirm them.
7. **Are products #20611, #20612, and #20613 intended to be sold by BGTK?**
   They are active Barrow boots, gloves, and helm.
   **Recommend** retain those references and verify their intended shop and
   keeper relationship.
8. **What are the intended products #20631–#20636?** None has a current
   object prototype. **Recommend** recover the owning source or approve
   replacements before using or editing this list.
9. **Are rooms #20630, #20641, and #20657 intended BGTK shop locations or
   stale Barrow numbers?** All three are active Barrow rooms.
   **Recommend** keep the room fields until the map owner verifies each one.
10. **Should the shop loader accept a record whose keeper lookup returns
    -1?** Source inspection shows the current assignment path indexes the
    mobile table without validating that result. **Recommend** have the
    runtime owner resolve the boot-safety issue before starting a full-mode
    server with the current fallback.
11. **Does BGTK have historical or editor-side source outside the current
    checkout?** Only a same-stem SHP history was found.
    **Recommend** check owner/editor records before archiving or relocating
    this file.
12. **Should Barrow remain the documented owner of the five resolving
    products?** Existing Barrow prototypes and QST use the same numbers.
    **Recommend** preserve the cross-reference in both audits until the
    ownership decision is recorded.
