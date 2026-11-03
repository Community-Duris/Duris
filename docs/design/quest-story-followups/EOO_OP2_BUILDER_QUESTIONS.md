# eoo_op2 builder questions

Canonical key: `component:eoo_op2`. Checkpoint 476; coverage row 389 /
CSV line 390; candidate dependency row 39. These questions preserve observed
source facts while the map, story, reset, object, and tooling owners decide
what they mean. Keep eoo_op2 RETAIN IN PLACE until those owners agree.

## Explicit builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| EOO-OP2-01 | Who owns this unlisted component, and is `The Horrible Tracts of the Unnamed` an intentional title? | Record the owner and canonical alias; the same raw ZON title also appears in astral, mtncastl, theurgist, and ud-inside. |
| EOO-OP2-02 | Is eoo_op2 part of the EOO/Valhalla/Muspelheim story, a standalone Niflheim map, or another project? | Keep these source identities separate until their owners document the relationship. |
| EOO-OP2-03 | Which canon source governs Hel, Loki, Angrboda, Fenris, Jormungand, Valhalla, and the Asgard threat described here? | Preserve the existing prose and request a story-owner decision before editing names or continuity. |
| EOO-OP2-04 | Why are the same Niflheim, gate, and Inner Sanctum passages copied into two three-room chains? | Confirm whether the copies support distinct portal destinations; retain both VNUM sets until the map owner answers. |
| EOO-OP2-05 | Is the #29220–#29222 chain intended as the Undead Continent entrance that eoo_op #29211 targets? | Preserve the EOO portal/reset and eoo_op2 room records as a typed source link, then confirm the exact map and room owners. |
| EOO-OP2-06 | Is the #29223–#29225 chain intended as the Goodie Continent entrance that eoo_op #29213 targets? | Confirm its relationship to the second chain and retain the separate #29223–#29225 identities. |
| EOO-OP2-07 | Is #29223 up to Duris3 #151855 “Grassy Plains” intentional, and why does that exit exist only on the second copy? | Ask both map owners to confirm the boundary and any intended return; do not treat the inactive target as a current route. |
| EOO-OP2-08 | Are the two WLD components meant to stay disconnected except for the separately reset type-25 portals? | Keep the graph metrics scoped to WLD exits and document any planned portal traversal separately. |
| EOO-OP2-09 | Who owns #29220–#29225: eoo_op2’s Hel passages or active Newbie’s distinct six rooms? | Preserve all complete room records and resolve the VNUM owner before any manifest or map change. |
| EOO-OP2-10 | Who owns #29220–#29222 where inactive dark_gh also defines different rooms? | Keep dark_gh’s Moss-Covered Cavern, Limestone Tunnel, and Moonwell distinct from both Newbie and eoo_op2. |
| EOO-OP2-11 | Are the 16 active Newbie directions to #29220–#29225 solely Newbie routes? | Preserve their Newbie-to-Newbie classification; do not count them as eoo_op2 inbound edges. |
| EOO-OP2-12 | Are Hel, the tormented soul, giant serpent, and nightmare creature (#29220–#29223) intended to remain distinct from active Newbie’s same-number MOBs? | Retain both prototype owners and check all resets and consumers against the intended source. |
| EOO-OP2-13 | Are portal objects #29220/#29221 meant to coexist with Newbie’s salt box and basilisk-tear vial at the same IDs? | Confirm the OBJ owner and keep both prototype records; do not let numeric equality collapse the portal and ingredient identities. |
| EOO-OP2-14 | Should Ailvio’s compiled `read_object(29220/29221)` calls continue to create the Newbie ingredients? | Keep the current Newbie behavior and require a complete code-consumer review before any VNUM change. |
| EOO-OP2-15 | Should active newbie.qst and newbie.story.json continue to treat #29220–#29225 as Newbie items? | Preserve the active quest/story references under their Newbie object identities and do not redirect them to eoo_op2 pools. |
| EOO-OP2-16 | Is the EOO #29202 → eoo_op2 #29222 → EOO #29202 portal pair intended? | Confirm both exact room identities and object owners before changing either stored target or reset. |
| EOO-OP2-17 | Is the EOO #29206 → eoo_op2 #29225 → EOO #29206 portal pair intended? | Confirm both exact room identities and object owners; do not combine its targets with the first pair. |
| EOO-OP2-18 | Should eoo_op2 targets #29202/#29206 point to EOO alcove/hallway rooms or the active Newbie rooms at those IDs? | Obtain an explicit target-owner decision; current active resolution selects Newbie, while EOO remains unlisted. |
| EOO-OP2-19 | Are `value[1] = 7` (CMD_ENTER) and `value[2] = -1` intentional for both pool portals? | Confirm the command and negative-charge convention against current item handling before changing either prototype. |
| EOO-OP2-20 | Do dark_gh portal objects #29203 → #29222 and #29204 → #29221 belong only to their own map? | Preserve their source-only dark_gh targets and reset ownership as separate identities. |
| EOO-OP2-21 | Is the imported Player Castles object #11051 in room #29223 required? | Preserve the external object reference and include it in Player Castles closure. |
| EOO-OP2-22 | Are all four mobiles intended to reset in both #29220 and #29223, while each pool portal appears only in its matching Inner Sanctum? | Confirm the two placement sets and limits from the reset owner; leave the ZON as recorded pending that answer. |
| EOO-OP2-23 | Which editor, generator, deployment, code, quest, and persisted-player data refer to these VNUMs and source paths? | Close those consumers and saved-room/object identities before any move, activation, or archive proposal. |
| EOO-OP2-24 | What evidence permits a later manifest or migration decision? | Require agreement on the duplicated map passages, reciprocal portal targets, VNUM collisions, imported reset object, and external tooling/data owners; retain in place until then. |

See the [static audit](EOO_OP2_AUDIT_REVIEW.md) and its
[connectivity findings](WORLD_CONNECTIVITY.md#checkpoint-476-eoo-op2-niflheim-map-and-reciprocal-portal-pairs).
