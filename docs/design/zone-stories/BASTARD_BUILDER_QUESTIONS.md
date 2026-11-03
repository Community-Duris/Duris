# Die Hall: builder questions

Canonical key: `component:bastard`. Physical coverage row 364. The
[story dossier](BASTARD.md) and
[technical audit](../quest-story-followups/BASTARD_AUDIT_REVIEW.md) preserve
the map text, reset/shop sources, three-way VNUM collision, and unregistered
`ud_map2` boundary. These recommendations do not establish live room, mob,
object, or shop availability.

## Explicit builder questions and recommended resolutions

1. **Is source stem `bastard` and raw title “Die hall” the intended identity?**
   Neither appears in `areas/AREA`; ZON header #302 is shared with
   `sunhall`. **Recommend** retain the raw stem, title, and header separately
   until the area owner identifies the map's canonical name.
2. **Is omission from the full-area list intentional?** All five area source
   files remain in the repository. **Recommend** confirm the intended load
   set before changing any manifest or source path.
3. **Which source owns rooms #30200–#30204?** Active `underworld3` defines all
   five as dwarven mining tunnels; `bastard` defines a root/cave/tavern, and
   unregistered `sunhall` defines pyramid rooms. **Recommend** retain all
   three source identities and request a VNUM ownership decision before any
   room edit, merge, or path move.
4. **Is the room-number collision with active Underworld3 intentional?** Its
   active WLD exit graph also uses the five numbers, so those live numeric
   references resolve to Underworld3 ownership. **Recommend** keep the
   candidate's source-only WLD exits distinct from active Underworld3 access.
5. **Are `bastard` and `sunhall` alternate drafts, separate guild halls, or
   otherwise related?** Their same-header zones share five room VNUMs, three
   mobile VNUMs, one object VNUM, and shop key #30201, with different
   descriptions and prototypes. **Recommend** do not infer replacement from
   the shared numbers or guild setting; ask their respective owners.
6. **Should the exposed root at #30200 lead up to `ud_map2` #1120721?** That
   map is unregistered and its source has no exit back to #30200.
   **Recommend** preserve the one-way source edge and resolve both map
   owners and the intended return path before describing it as usable travel.
7. **Is the entry to #30201 intended to connect this candidate to the
   active Underworld3 tunnel numbered #30201?** The text and room identity
   differ. **Recommend** preserve the numeric boundary evidence and confirm
   what the intended load/generation system does with the conflict.
8. **What is the intended relationship among the Clavados cave mark, the
   Carnage skull badge, and the Blood and Skull Tavern?** The source names
   them separately. **Recommend** retain each authored label without
   assigning a single guild owner until the builder confirms the lore.
9. **Are the frost-giant guard and Ilixctthu resets in their current rooms
   intended?** Five M commands reference the three local mobile prototypes.
   **Recommend** preserve the exact resets and review their authored limits
   in the zone editor.
10. **Should the cave continue to describe durithalite as blocking magical
    remote entry and as a safe structure?** These are prose statements, not
    qualified mechanic or safety findings. **Recommend** keep the text
    distinct from any code-backed travel or hazard claim.
11. **Are the Player Castles objects #11060, #11110, and #11013 intended in
    the root, tavern, and barracks?** They are three external O resets.
    **Recommend** preserve the exact source imports and their rooms until
    both area owners confirm placement.
12. **What supplies the reset objects #6084 and #6094?** No current `.obj`
    prototype for either number was found. **Recommend** resolve their
    intended source owner before editing the ZON command or replacing them.
13. **What should the #6089 shop product/reset resolve to?** No current
    object prototype exists in the scanned source, while the ID appears in
    both the ZON and SHP. **Recommend** keep both references linked and ask
    the item owner to resolve the missing prototype.
14. **Is #53 meant to be the Heavens rope coil in this shop/reset?** The same
    number also has source-only Dalvik, Hostel, and Mavulsk prototypes.
    **Recommend** confirm which object identity the guild intended before
    treating the numeric reference as unambiguous.
15. **Are the remaining Heavens and Verz1 stock/give imports intentional?**
    Items #347/#377/#364/#412/#398/#108/#109/#111 have Heavens definitions;
    #3050 has a Verz1 definition, and #108/#109 also occur in source-only
    Mavulsk. **Recommend** keep the object owners and import records
    separate until the item owner resolves shared identities.
16. **Should Ilixctthu #30201 sell the twelve items in the current SHP
    record from room #30202?** Only one item is local object #30200; #6089
    lacks a current prototype. **Recommend** retain the source inventory and
    do not infer which products load from the standalone record.
17. **How should the `bastard` and `sunhall` shop records with the same
    keeper VNUM #30201 be handled?** Their keeper descriptions and rooms
    differ, and both SHP files are separate sources. **Recommend** resolve
    aggregate inputs and shop ownership before any loader or stock change.
18. **Does zone reset mode 0 and the three repeated frost-giant reset lines
    reflect the intended reset schedule?** **Recommend** ask the area owner
    to confirm this in the editor without inferring current population from
    source counts.
19. **Is the absence of a local QST and story sidecar intentional?** The
    area story currently resides in room/mobile/object prose. **Recommend**
    preserve the current files while asking whether any external builder
    project or persisted quest/map identity exists.
20. **What evidence is required before a path or archive decision?** Current
    source cannot qualify editor ownership, generated aggregate state,
    persisted instances, or live loader behavior. **Recommend** keep
    candidate row 13 **RETAIN IN PLACE** until those owners provide that
    evidence and reconcile the active Underworld3, Sunhall, and `ud_map2`
    identities.
