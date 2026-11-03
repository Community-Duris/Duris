# mtncastl builder questions

These source-backed prompts are for the area, map, editor, tooling, and
persisted-data owners. They identify decisions and recommended resolutions;
they do not imply runtime or gameplay verification.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| MTN-01 | What source or project identity does the stem mtncastl represent? | Keep the current stem and record aliases only when an owner identifies their use. |
| MTN-02 | Is “The Horrible Tracts of the Unnamed” the intended title for this component? | Preserve the exact ZON title until its owner confirms the canonical name. |
| MTN-03 | What is the relationship among the five current ZON files with this exact title? | Keep astral, eoo_op2, mtncastl, theurgist, and ud-inside separate absent source-history or owner evidence of replacement. |
| MTN-04 | Is mtncastl’s room #1 intentionally byte-identical to the #1 placeholders in astral and ud-inside? | Treat the shared block as copied content evidence, not shared identity. |
| MTN-05 | How should this room be identified when #1 is reserved as ROOM_LIMBO_VNUM and active Limbo is the current owner? | Keep the source-only record distinct; nine active Desert exits target #1 and resolve to Limbo, not mtncastl. |
| MTN-06 | Is ZON header #1 intentional for room #1, which falls outside the header’s #100–#199 VNUM/100 band? | Preserve the raw values and ask the area owner to resolve the range before renumbering. |
| MTN-07 | Is an unnamed room with a blank description intended content or an unfinished placeholder? | Preserve the source record until the map author confirms whether content is missing. |
| MTN-08 | Is the reset mode 2 with no reset commands intentional? | Keep the header unchanged until the builder confirms whether this is a deliberate empty zone or a partial export. |
| MTN-09 | Are the zero-byte MOB and OBJ files intentional package members? | Preserve both tracked files while the source owner confirms placeholder versus deliberate-empty status. |
| MTN-10 | Is the component intentionally absent from areas/AREA and areas/qst/AREA? | Confirm desired loader and quest-index membership before editing either manifest. |
| MTN-11 | Is there an external map-editor project or map export when no same-stem areas/map JSON exists? | Identify the editor, project location, and export owner before proposing any source move. |
| MTN-12 | Do any scripts, tools, deployments, or persisted character references assign room #1 specifically to mtncastl? | Require typed owner evidence; the 23 WLD directions and four ZON room-field commands found in source do not establish mtncastl as an active owner, and the typed shop, portal/switch, sidecar, and code scan binds no active room reference to it. |
| MTN-13 | Should this source remain inactive while its identity, range, and relationship to the other titled files are unresolved? | Retain all four paths in place and make a later activation or archive decision separate. |

See the [mtncastl source audit](MTNCASTL_AUDIT_REVIEW.md) for the source,
graph, VNUM, title, and history evidence behind these questions.
