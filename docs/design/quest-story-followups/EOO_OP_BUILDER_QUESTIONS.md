# eoo_op builder questions

Canonical key: `component:eoo_op`. Checkpoint 475; coverage row 388 /
CSV line 389; candidate dependency row 38. These are source-backed decisions,
not implementation instructions. Keep the component RETAIN IN PLACE until
the responsible map, story, reset, object, and tooling owners answer them.

## Explicit builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| EOO-OP-01 | Who owns the unlisted component, and is `Eye of Odin Outpost` its canonical title or only an editor label? | Record the current owner, source of truth, alias set, and intended loader status before proposing a path or manifest change. |
| EOO-OP-02 | Is this a separate Muspelheim map, an outpost attached to EOO Valhalla, or one part of a larger EOO/EOO_OP/EOO_OP2 design? | Keep the component identities separate until their map owners document the intended relationships. |
| EOO-OP-03 | Which story version is authoritative for Surt/Surtr, Twilight, the flame of life, and the nine-world/Ragnarok premise? | Preserve the current WLD/MOB prose; request a canon decision before rewriting names or linking quests. |
| EOO-OP-04 | Is the absence of a QST and story sidecar intentional for this story-bearing map? | Identify any external quest, editor, or narrative source before treating the prose as unimplemented or disposable. |
| EOO-OP-05 | Are ZON header #1, top VNUM #29212, and WLD room IDs #29210–#29212 the intended range and source-zone identity? | Confirm the authoring convention and numeric owner; do not alter zone or room fields based on the shared numbers alone. |
| EOO-OP-06 | Are the reciprocal #29210 north/#29211 south exits intended as the only route into the throne room? | Preserve both records until the outpost map owner confirms direction, door, key, and reset behavior. |
| EOO-OP-07 | Are the reciprocal #29211 east/#29212 west exits intended to lead through the Wall of Flames room? | Confirm intended passage and return path; no gameplay traversal was tested. |
| EOO-OP-08 | Is #29210 up to Duris3 #123225 “The Ocean” intentional, and should the connection be one-way? | Ask the Duris3 and outpost owners to identify the boundary and any return or safety route; keep it source-only meanwhile. |
| EOO-OP-09 | Which source owns room #29210: Muspelheim’s fiery realm, active Newbie’s Walkway turn, or inactive dark_gh’s Undead Warding circle? | Preserve each distinct complete room block and obtain an explicit VNUM-ownership decision before any merge or activation. |
| EOO-OP-10 | Which source owns the #29211 and #29212 room identities that also appear in active Newbie and inactive dark_gh? | Keep the candidate, Newbie, and dark_gh titles, exits, and descriptions distinct in the inventory; do not count Newbie routes as candidate ingress. |
| EOO-OP-11 | Are the seven active Newbie directions to #29210–#29212 exclusively Newbie routes, or is any separate source-only portal/route intended? | Retain the Newbie route classification and request a separate explicit route record for any intended Outpost approach. |
| EOO-OP-12 | Are Surt, the flame mass, Pyros, and the fire serpent (#29210–#29213) meant to remain separate from active Newbie’s same-number fish mobiles? | Preserve both MOB identities and confirm every reset/consumer uses the intended source owner. |
| EOO-OP-13 | Are eoo_op objects #29210–#29213 meant to coexist with the different active Newbie objects at the same VNUMs? | Confirm each prototype owner and typed reset/import consumer; do not select a prototype solely by numeric match. |
| EOO-OP-14 | Should the Wall of Flames object #29210 enter EOO #29206, which is inactive Valhalla, or active Newbie’s Shaman’s Guild at the same number? | Resolve the destination room identity with EOO and Newbie owners before changing `value[0]`. |
| EOO-OP-15 | Should portal #29211’s Undead Continent description target #29222 in active Newbie, or the inactive eoo_op2 Hel Inner Sanctum? | Confirm the intended map and exact room record; do not infer from the shared VNUM or label alone. |
| EOO-OP-16 | Why does the Khomani-Kan portal #29212 store its own VNUM, which is also this component’s Wall of Flames room and active Newbie’s Rogues Alley? | Confirm the intended continent-room VNUM and resolve the self/shared-number target before use. |
| EOO-OP-17 | Should Goodie portal #29213 target #29225 in active Newbie, or the inactive eoo_op2 Hel Inner Sanctum? | Ask both map owners to resolve the competing room identity and preserve the portal prose until answered. |
| EOO-OP-18 | Are `value[1] = 7` (CMD_ENTER) and `value[2] = -1` intentional for all four portals? | Confirm the enter command and negative-charge convention against current item handling before changing any prototype. |
| EOO-OP-19 | Are EOO ZON resets of portal objects #29211–#29213 into #29202/#29206 intended cross-component imports? | Preserve the three typed references and identify the intended OBJ source for each reset. |
| EOO-OP-20 | Why does eoo_op.zon reset only its own Wall of Flames #29210 while the other three portal objects are reset by eoo.zon? | Document each portal’s intended placement, owner, and reset responsibility; do not duplicate or transfer resets speculatively. |
| EOO-OP-21 | Is the #11051 Player Castles object import in eoo_op.zon required, and what external source owns its behavior? | Preserve the existing imported object reference and include it in the Player Castles dependency closure. |
| EOO-OP-22 | Are all four mobiles intentionally reset into #29210, and is object #29210 intended to load in #29212 under the current limits? | Confirm room placement, reset limits, and map-owner intent from the builder; leave source reset records untouched pending that review. |
| EOO-OP-23 | Which editor, generator, package, code, deployment, and persisted-player data refer to these filenames or VNUMs? | Close those external consumers and saved-room dependencies before any path move, activation, or archive proposal. |
| EOO-OP-24 | What evidence would permit a later migration or manifest decision? | Require the map/story owner, exact portal targets, typed reset imports, VNUM collision closure, and tool/data owners to agree; retain in place until then. |

See the [static audit](EOO_OP_AUDIT_REVIEW.md) and its
[connectivity findings](WORLD_CONNECTIVITY.md#checkpoint-475-eoo-op-muspelheim-map-and-portal-targets).
