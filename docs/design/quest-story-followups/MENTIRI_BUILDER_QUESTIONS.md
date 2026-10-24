# Mentiri builder questions

These are source-backed prompts for a builder, map owner, or data owner. They
record unresolved decisions and suggested resolutions; they do not imply that
runtime or gameplay outcomes have been tested.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| MT-01 | Is “Mentiri, the Prison of Dis” the canonical title, and are there owner-confirmed aliases for Mentiri? | Keep the ZON title as the source label; record aliases only when an owner confirms their use. |
| MT-02 | Is the four-file WLD/MOB/OBJ/ZON component intentionally absent from both areas/AREA and areas/qst/AREA? | Confirm its intended load and quest-index status before changing either list. |
| MT-03 | Is areas/map/Mentiri.json the canonical map-editor project data, a generated export, or a separately maintained layout? | Identify its source of truth and preserve its 72 positions with the matching owner. |
| MT-04 | Are the 39 literal “Unnamed” room titles at #12 and #35–#72 intentional? | Ask the map owner to confirm which titles are final and retain any intentional placeholders until then. |
| MT-05 | Are the empty WLD room-description bodies intentional, given the named prison, judgement, armory, and slag landmarks? | Confirm whether concise environmental prose is missing before presenting the map as finished. |
| MT-06 | Is Mentiri #1, the descent above Dis, intended to be the player entrance, and what source-owned route should reach it? | Keep the local graph separate from active Limbo/Heavens low-VNUM lookups until the route owner identifies an endpoint. |
| MT-07 | Should the #19–#34 molten slag rooms be understood as a hazard area, a workshop, or both? | Clarify the distinction between the “pit” and repeated “pot” titles before adding gameplay assumptions. |
| MT-08 | What is the intended relationship among gate guardian Vesper #19, captured archon Solarious #20, and the Sundered Flame commander Murmur #18? | Preserve the named source roles and obtain the story owner’s account before linking their motivations. |
| MT-09 | Is Solarious’s chained captivity meant to be discoverable environmental lore only, or part of a future interaction or rescue story? | Do not invent a quest or outcome; ask the story owner what the text is meant to establish. |
| MT-10 | Is Murmur #18 intentionally not reset even though his title names him as commander? | Confirm whether the absent reset is deliberate, draft content, or an omitted placement. |
| MT-11 | Is reset mode 2 and the seven-command population plan still intended? | Keep the source-only reset file and confirm the desired reset policy before any loader-list change. |
| MT-12 | Are the seven M rows all intentionally assigned to room #4, with third-field values 1, 1, 2, 2, 3, 3, and 3 for #20, #19, #22 twice, and #21 three times? | Preserve the exact M rows until the zone owner confirms their count fields, repeated rows, and room placement. |
| MT-13 | What are the intended identities and uses of the 20 generic OBJ prototypes, none of which is placed by the local ZON? | Confirm whether they are unfinished placeholders or required item data before changing descriptions or resets. |
| MT-14 | Is the absence of a same-stem QST file intentional, and should the environmental story remain without native contracts? | Keep the current story and quest inventories separate unless the owner identifies missing authored contracts. |
| MT-15 | Does the armory room represent a narrative location only, given that there is no same-stem SHP file? | Confirm any intended shop owner separately: parser-valid N-format records have no keeper #1–#30 match, three #5 stock values resolve to active Limbo, and Heavens shop room #23 is a distinct active room. |
| MT-16 | How should the 72 room VNUMs be handled when every one overlaps another WLD source and 43 overlap active Limbo/Heavens rooms? | Preserve Mentiri as a distinct map identity and resolve the complete room set before activation or renumbering. |
| MT-17 | How should the 30 mobile and 20 object VNUMs be handled when all overlap other sources and active ZON, QST, story, shop, teleport, and code roles use some of the same numbers? | Keep each prototype distinct; resolve Heavens M #2, cross-source object resets #5/#17, active QST/story/shop references, and ITEM_TELEPORT targets #2/#9/#50 with their owners before attributing any role to Mentiri. |
| MT-18 | Do overlaps with 27 rooms, 24 mobiles, and 20 objects in areas_mini reflect any shared identity, or are they separate minimal-mode data? | Keep the miniature fixture distinct unless its owner documents a deliberate shared prototype. |
| MT-19 | Are any of the 70 active WLD directions that numerically target #1–#72 intended to enter Mentiri? | Treat their current Limbo/Heavens destinations as separate active identities unless source owners identify a different intended recipient. |
| MT-20 | Do followed same-stem Git histories reflect the whole Mentiri lineage, or are there alternate editor, export, or deployment copies? | Reconcile external copies with the map and source owners; the tracked history alone does not identify a replacement or canonical editor copy. |
| MT-21 | Are the many generic mob descriptions intentional for the named devils, abishai, garrison, and prison figures? | Confirm which records are finished and avoid using placeholder text as narrative evidence. |
| MT-22 | Should Mentiri remain an attic/source-only component, or is its omission from the full world list temporary? | Keep all files in place until the owner resolves identity, dependencies, map tooling, and intended load status. |

See the [Mentiri source audit](MENTIRI_AUDIT_REVIEW.md) for the inventory,
graph, collision evidence, and source links behind these questions.
