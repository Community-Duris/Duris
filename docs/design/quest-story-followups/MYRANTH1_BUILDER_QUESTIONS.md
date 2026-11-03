# myranth1 builder questions

These source-backed prompts are for the quest, area, shop, mobile, object,
story, tooling, and persisted-data owners. They record unresolved decisions
and recommended resolutions; they do not imply runtime or gameplay testing.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| MYR-01 | What area or project identity does the stem myranth1 represent when it has only QST and SHP files? | Preserve the current stem and record a zone alias only after an owner identifies it. |
| MYR-02 | Is candidate QST Moorney the same apprentice imprisoned in active Zyla #70919’s backstory? | Treat the matching names and story as narrative-link evidence; ask the Knight and quest owners to confirm before assigning a live campaign link. |
| MYR-03 | Where should the two QST giver prototypes #70132 and #70133 exist? | Do not invent a map or mobiles; locate the owner’s source or have the builder resolve the missing prototypes. |
| MYR-04 | Does the #70133 contract’s removal of #70108 and reward of #70107 match the dialogue that it gives Moorney’s key, and how does #70132’s removal of #70157 fit the sequence? | Preserve the QST commands and ask the owner to identify all three item definitions and their intended roles. |
| MYR-05 | Why do item-typed QST commands use #70107 and #70108, which are active Moregeeth warg mobile VNUMs? | Keep the types distinct and resolve the collision before any quest loading or renumbering. |
| MYR-06 | Is removal of 10,000 coins part of the intended Moorney completion cost? | Preserve the source command until the quest owner confirms its current design and currency intent. |
| MYR-07 | Are all five myranth1 shops intended to bind to active Moregeeth mobiles #70101–#70105 in their current rooms? | Confirm each keeper’s intended shop role and location; preserve the SHOP records until then. |
| MYR-08 | Do the 19 product VNUMs in these five shops have an external or retired object source? | Locate owner-maintained item definitions before classifying the stock as usable or obsolete. |
| MYR-09 | Is the file meant to add separate shops to existing Moregeeth mobiles while active goblinht.shp contains shops #70029–#70038? | Keep the source identities separate and ask the Moregeeth shop owner to confirm intended shop assignment. |
| MYR-10 | Should myranth1 remain absent from areas/AREA and areas/qst/AREA? | Confirm normal aggregation and quest-index intent before changing either manifest. |
| MYR-11 | Is the conditional all-SHP fallback path an intended way for these five records to enter a shop stream? | Treat the fallback as source-code exposure, not runtime confirmation; have the loader owner resolve the intended source set. |
| MYR-12 | Is the QST intended to remain unaggregated when the runtime quest loader reads world.qst without a per-file glob? | Keep the file in place and ask the quest owner to identify the intended deployment and aggregate path. |
| MYR-13 | Is there a WLD/ZON/map project or other room owner for this QST when no same-stem map files exist? | Identify the external editor and room owner before proposing activation or a file move. |
| MYR-14 | Do the 2009/2013 same-stem QST and SHP histories represent one legacy package? | Record only the tracked path history; request owner evidence for package provenance. |
| MYR-15 | Should the QST and SHP files remain source-only until item, giver, shop, and loader closure is complete? | Retain both files and make any later activation or archive decision separate. |
| MYR-16 | Why does #70132’s D completion text say “the conjurer” leaves when its giver dialogue and Q text name Moorney? | Preserve the raw message and ask the quest/story owner to resolve the actor, location, and intended completion beat. |

See the [myranth1 source audit](MYRANTH1_AUDIT_REVIEW.md) for the
quest, shop, typed-reference, loader, and history evidence behind these
questions.