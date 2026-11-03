# Mountain.qst builder and owner questions

These questions separate the unregistered uppercase `Mountain.qst` from
active lowercase `mount` and the active `mountaintracks` map. CP922 found
same-number WLD rooms for the candidate's mobile and item references; those
room identities do not resolve either QST role. Recommendations preserve the
current records and do not claim that the candidate loads or that any
exchange was played.

| ID | Builder or owner question | Recommended resolution |
| --- | --- | --- |
| MTQ-01 | Is `Mountain.qst` an independent missing area story, a retired version of active `mount`, a quest file intended for `mountaintracks`, or a test/alternate file? | Keep it separate until the owner identifies its original map and editor; filename similarity and cross-type number overlap are not lineage evidence. |
| MTQ-02 | Why does the full-area manifest list lowercase `mount` while this file is uppercase and absent from both area lists? | Preserve the manifest and file as found; confirm intended source membership before changing either path or list. |
| MTQ-03 | Are the five #21012/#21014/#21025/#21028/#21029 quester headers meant to bind to removed or external mobiles, despite those numbers currently naming `mountaintracks` rooms? | Resolve the exact mobile prototypes and their source owner; a same-number room or a different mobile placed there does not satisfy a quester header. |
| MTQ-04 | Where are item objects #21000, #21002, #21008, #21009, #21025, #21026, and #21027 defined, given that the same numbers currently name `mountaintracks` rooms? | Treat all seven references as unresolved OBJ VNUMs; room definitions do not satisfy item goals. Recover only owner-confirmed prototypes and do not invent replacements. |
| MTQ-05 | Does `R E 10000` intentionally impose a 10,000 experience removal on #21012's completion? | Confirm the authored cost or requirement and preserve the raw EXP-typed clause until the owner resolves it. |
| MTQ-06 | Is the great-white-dragon metal request intended to connect to the Givnab/Sorlag crafting chain? | Preserve the story sequence and ask for the exact two input chips, enchant output, smithing inputs, and final weapon VNUMs. |
| MTQ-07 | Which item should Gulik remove when the text says the player presents stolen armor but the clause removes missing #21027? | Obtain the exact accepted-item VNUM and keep the existing clause unchanged pending confirmation. |
| MTQ-08 | Should Gulik's completion give Dungeon object #93001 when the dialogue describes an armoury key and the same number also names a Dungeon mobile, QST header, and room? | Resolve the typed object identity and key-versus-armor wording with both source owners; do not rename or repurpose active Dungeon object #93001 from this orphaned QST. |
| MTQ-09 | Is `R I 21026` followed by three outputs #21009/#21008/#21025 the intended Sorlag crafting contract? | Confirm the required objects, generated components, and finished weapon before calling the written chain complete. |
| MTQ-10 | Does uppercase `Mountain.qst` have a documented relationship to lowercase active `mount.qst` and its Lokpan/prison story? | Treat the two QST files as distinct; record a relationship only with editor or owner evidence. |
| MTQ-11 | Should this candidate remain outside `areas/AREA` and `areas/qst/AREA`? | Keep the current source/build exclusion until its missing prototypes, intended story owner, and load contract are reconciled. |
| MTQ-12 | Do any deployed aggregates, editors, or persisted quest receipts still refer to Mountain giver IDs or item IDs? | Complete those references before any archive or path proposal; source-only status alone is not migration clearance. |

See the [Mountain.qst source audit](MOUNTAIN_QST_AUDIT_REVIEW.md) for the
QST clauses, current prototype resolutions, typed room-number overlaps, and
source/build evidence.
