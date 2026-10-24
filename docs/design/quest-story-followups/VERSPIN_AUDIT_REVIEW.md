# Verspin tracked-source audit

Checkpoint 876, reviewed October 12, 2026. This review closes the bounded tracked-source typed-reference pass for active `verspin` and reconciles the inactive-WLD approach scan. It records authored declarations and compiled numeric bindings; it does not establish successful loading, reset admission, quest completion, shop operation, item availability, player access, or safe return. No area or gameplay source was changed, and no runtime operation was performed.

## Static WLD graph and inactive-map sweep

The CP425 active graph census remains 200 rooms with 435/435 resolving authored exits in one weak component. There are two active inbound records, one outbound record, and one reciprocal pair: the Surface highway pair at Verspin #28100, plus Winterhaven's one-way arrival at #28109. Thirty-five rooms have fall fields and none has a current field. See the [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-425-verspin-local-graph-entrances-and-hazards), the [local entrance](../../../areas/wld/verspin.wld#L1), and [coverage line 338](ZONE_COVERAGE_REGISTER.csv#L338).

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits with no parse errors. It found no source-room VNUM overlap with Verspin and one source-only approach: inactive `surf.wld` room #229065, titled “Lush Plains of Shrubs and Small Bushes,” exits north to Verspin #28100 at [source line 609705](../../../areas/wld/surf.wld#L609705). Keep it on the existing Surf candidate plan, data row 96 / CSV line 97. It is not an active WLD edge and is already represented in the current curated source-only totals of 11,993 overall / 415 from Surf; no aggregate count changes.

## Outbound active reset imports

Verspin's local ZON has 21 resolved external object-reset operands across seven active source areas, covering 13 distinct objects. Each prototype has one active same-type owner. No local mobile or object reset operand remained unresolved in the scanned reset-role fields.

| Active owner | Reset rows | Object prototypes | Verspin ZON evidence |
| --- | ---: | --- | --- |
| Foggy Woods | 1 | #90026 | [line 228](../../../areas/zon/verspin.zon#L228) |
| Verz1 | 1 | #3097 | [line 235](../../../areas/zon/verspin.zon#L235) |
| Heavens | 7 | #363 (2), #368 (2), #369 (3) | [251](../../../areas/zon/verspin.zon#L251), [252](../../../areas/zon/verspin.zon#L252), [253](../../../areas/zon/verspin.zon#L253), [345](../../../areas/zon/verspin.zon#L345), [346](../../../areas/zon/verspin.zon#L346), [347](../../../areas/zon/verspin.zon#L347), [500](../../../areas/zon/verspin.zon#L500) |
| Herders | 4 | #94336 | [line 262](../../../areas/zon/verspin.zon#L262), [302](../../../areas/zon/verspin.zon#L302), [328](../../../areas/zon/verspin.zon#L328), [367](../../../areas/zon/verspin.zon#L367) |
| Val | 1 | #38421 | [line 372](../../../areas/zon/verspin.zon#L372) |
| Graves | 5 | #19500, #19501, #19502, #19503, #19504 | [452](../../../areas/zon/verspin.zon#L452), [454](../../../areas/zon/verspin.zon#L454), [456](../../../areas/zon/verspin.zon#L456), [458](../../../areas/zon/verspin.zon#L458), [461](../../../areas/zon/verspin.zon#L461) |
| Nizari | 2 | #40132 | [537](../../../areas/zon/verspin.zon#L537), [538](../../../areas/zon/verspin.zon#L538) |

These reset declarations preserve their active prototype owners. They do not establish reset admission or available stock.

## Inbound active reset consumers

The only active external ZON reset-role reference into a Verspin prototype is one G reset of Verspin OBJ #28146 from [Mntcastl line 383](../../../areas/zon/mntcastl.zon#L383). No active external ZON mobile-reset operand targets a Verspin mobile.

## QST, story-sidecar, and SHP dependencies

The local QST records three `R I` operands for Heavens objects #223, #224, and #225 at [lines 123–125](../../../areas/qst/verspin.qst#L123). It also records `G I #74298` at [line 348](../../../areas/qst/verspin.qst#L348); the active owner is Bloodstone OBJ #74298, matching the [Bloodstone audit](BS_AUDIT_REVIEW.md#inbound-active-consumers). The Verspin story sidecar names #74298 in `$.stories[5].steps[0].item_vnums` ([line 422](../../../areas/story/verspin.story.json#L422)). These are outbound item dependencies and do not change ownership of the prototypes.

The tracked QST scan found no external item command or giver header targeting a local Verspin object or mobile. Across tracked story sidecars, no typed `*_vnum` field outside `verspin.story.json` points to a Verspin room, mobile, or object. The tracked SHP scan likewise found no external product targeting a Verspin object.

`verspin.shp` has five shop headers. Two shops list Heavens products #363, #368, and #369: shop #28110 at [lines 5–7](../../../areas/shp/verspin.shp#L5) and shop #28139 at [lines 155–157](../../../areas/shp/verspin.shp#L155). The remaining listed products resolve to local Verspin objects. The coverage register's shop count remains zero; this discrepancy is already captured by builder question 13 and remains for owner/census clarification.

## Prototype identity, portals, and compiled references

No other tracked MOB file defines any of Verspin's 79 mobile VNUMs, and no other tracked OBJ file defines any of its 57 object VNUMs. Across 872 parsed tracked type-25 declarations (713 in active object sources), none targets a Verspin room; Verspin defines no local type-25 object. The tracked ITEM_SWITCH scan found no value[1] destination into a Verspin room. The relevant declarations are in [defines.h for ITEM_TELEPORT](../../../src/core/defines.h#L140) and [ITEM_SWITCH](../../../src/core/defines.h#L144), with destination-field behavior in [specs.object.c](../../../src/specs/specs.object.c#L302).

Exact C/C++ numeric matches identify these separate bindings:

| Local prototype | Compiled reference | Source role |
| --- | --- | --- |
| OBJ #28153 | [chaos_eq_data.h line 341](../../../src/account/chaos_eq_data.h#L341) | Static Chaos equipment array entry. |
| MOB #28124 | [staff_newbie_aid.c line 99](../../../src/cmd/staff_newbie_aid.c#L99) | `read_mobile` call in the staff pet command's dog branch. |
| Room #28197 | [ship_variables.c line 182](../../../src/ships/ship_variables.c#L182), [specs.assign.c line 2398](../../../src/specs/specs.assign.c#L2398), and [ship_shop.c line 3238](../../../src/ships/ship_shop.c#L3238) | Crew-shop room table and procedure bindings. |
| Room #28281 | [specs.assign.c line 2320](../../../src/specs/specs.assign.c#L2320) | Stat-shop procedure assignment. |

These are code-bound references, separate from WLD exits and the static source counts.

## Disposition

Keep active `verspin` **DO NOT MOVE**. The prior [Checkpoint 771 title and map-identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-771-verspin-destination-references) remains unchanged: it found one exact active title owner and no distinct predecessor or replacement in the followed same-stem history. Add no Verspin attic candidate. Preserve the Surf route on its existing candidate plan and keep each imported item under its active prototype owner. The existing [20 builder questions](../zone-stories/VERSPIN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions) remain the owner-facing list. Live reset and shop behavior, item availability, command-target aliasing, editor/tool state, deployment/runtime state, persisted identity, and owner intent remain unqualified.

See the [Verspin source dossier](../zone-stories/VERSPIN.md), [coverage line 338](ZONE_COVERAGE_REGISTER.csv#L338), and the existing [Surf candidate row 96](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
