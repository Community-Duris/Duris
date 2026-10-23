# ethdawn_op builder questions

Canonical key: component:ethdawn_op. Checkpoint 478; coverage row 391 /
CSV line 392; candidate dependency row 41. These are source-backed decisions,
not implementation instructions. Keep this component RETAIN IN PLACE until
the map, item, portal, reset, and external-data owners resolve its identity.

## Explicit builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| EDO-01 | Who owns the unlisted WLD/MOB/OBJ/ZON files and their external editor data? | Record the current source, map, deployment, and tooling owners before changing paths or loader membership. |
| EDO-02 | Is Etherial Dawn Outpost the canonical title, and is the spelling Etherial intentional? | Preserve the ZON spelling until the owner documents its canonical title and any aliases. |
| EDO-03 | Is the absence of a same-stem QST and story sidecar intentional for the Mistywood, Jheral, and portal prose? | Identify any external narrative or quest source before treating the prose as incomplete or disposable. |
| EDO-04 | Are zone header #1, reset mode 2, and rooms #29310–#29312 coherent even though all room VNUMs fall outside header #1’s numeric range? | Preserve the exact fields and ask the builder to resolve the range mismatch before changing the header or room IDs. |
| EDO-05 | How does this outpost relate to active Newbie and inactive loc_gh and eskull_hall rooms at the same VNUMs? | Preserve all three complete map identities and require an owner-approved crosswalk before any merge or activation. |
| EDO-06 | Should Newbie continue to own #29310 “A Grotto,” #29311 “Inside A Small Home,” and #29312 “Life Crossroads”? | Keep the current active room meanings until the area and persisted-room owners approve a documented change. |
| EDO-07 | Are the north/south #29310↔#29311 exits and west/east #29311↔#29312 exits the intended complete local route? | Preserve the three reciprocal directions and ask the builder to confirm that no local passage is missing. |
| EDO-08 | Is #29310 up to inactive Duris3 #156745 “Rolling Hills” intentional, and should it remain one-way? | Confirm the boundary, arrival, and return path with both map owners before changing either WLD. |
| EDO-09 | Are petrified treant #29310, grey elf #29311, and cloaked statue #29312 intended to reset together in #29310? | Preserve the four M resets and confirm the two-treant count and placements with the builder. |
| EDO-10 | Should candidate mobiles #29310–#29312 remain separate from same-number Newbie OBJ records? | Preserve VNUM types and resolve each typed reset and consumer independently. |
| EDO-11 | Should portal #29310 target #29307, which this map does not define and which is also an Ebon Skull room number? | Ask the portal and map owners to identify the intended #29307 room source; do not select Newbie or Ebon Skull by number alone. |
| EDO-12 | Why does the green portal store #29307, and where is it meant to be entered from? | Document its source placement, destination map, and return route; preserve the current target until then. |
| EDO-13 | Is the Goodie-continent portal #29311 meant to target #29312, a room in this same outpost? | Confirm its intended Goodie destination and distinguish the candidate cavern from active Newbie’s Life Crossroads. |
| EDO-14 | Should both portals require CMD_ENTER (7) and keep value[2] = -1? | Confirm the command and negative-charge convention against current item handling before changing either OBJ. |
| EDO-15 | Is Ravenmis’s object #29311 reset into room #29307 an intended import of this candidate’s Goodie portal? | Ask both unlisted map owners to confirm the object identity and route; retain the source-only reset meanwhile. |
| EDO-16 | Should active Newbie continue to own object #29310 as a hefty bag and #29311 as arrows? | Preserve the active OBJ records and Newbie QST/story consumers until the item and area owners approve any separate VNUMs. |
| EDO-17 | Are Newbie’s #29310/#29311/#29312 item rewards meant to remain distinct from both candidate portals and the Newbie quiver? | Confirm item ownership and all reward/recovery data before considering any shared VNUM assignment. |
| EDO-18 | Should Player Castles objects #11054 and #11104 remain imported into candidate rooms #29310/#29311? | Preserve both exact O references until Player Castles and outpost owners confirm their item purpose and placement. |
| EDO-19 | Should candidate portal #29310 reset into room #29312 while #29311 has no local O reset? | Document each portal’s intended reset owner, location, and external import before moving or duplicating resets. |
| EDO-20 | Are Jheral and the druidic inscription intended to connect this outpost to another Mistywood story? | Request a story-owner decision and preserve the existing inscription until the relationship is documented. |
| EDO-21 | Should the source remain absent from both full AREA and QST AREA lists? | Confirm the intended loading and quest contract separately from the title and source-file existence. |
| EDO-22 | Does tracked history identify this source as a predecessor, rename, or replacement for another Mistywood or guild map? | Record only explicit lineage evidence; the 2011 spelling edit and shared VNUMs do not establish replacement. |
| EDO-23 | Which scripts, generators, editor packages, deployment jobs, and persisted player locations or items refer to these paths and VNUMs? | Close external consumer and saved-state dependencies before a path, loader, or identity decision. |
| EDO-24 | What evidence would permit a future activation, move, rename, merge, or archive proposal? | Require map/story ownership, exact portal targets, all typed imports and consumers, editor/deployment closure, and persisted-ID review; retain in place until then. |

See the [static audit](ETHDAWN_OP_AUDIT_REVIEW.md), its
[connectivity findings](WORLD_CONNECTIVITY.md#checkpoint-478-ethdawn_op-mistywood-outpost-map-and-portal-targets),
and [identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-478-ethdawn_op-identity-and-bounded-history).