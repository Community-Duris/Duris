# Unique static dependency review

Checkpoint 870 closes the bounded tracked-source dependency scan for active unique, against source snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697. The active manifest lists unique *672 at [areas/AREA line 254](../../../areas/AREA#L254), and the QST index lists the same stem at [areas/qst/AREA line 228](../../../areas/qst/AREA#L228). The source dossier remains [UNIQUE.md](../zone-stories/UNIQUE.md); its 24 direct-source builder questions are unchanged.

## Map identity and local graph

unique.wld defines rooms #67200 and #67300 and a reciprocal vertical pair: #67200 down to #67300, then #67300 up to #67200. The static world census records two authored and two resolved exits, one weak component, no external WLD boundary, and no inactive-WLD target into either room. The local [unique.zon](../../../areas/zon/unique.zon#L1) contains only its header and terminator, so none of the 84 local object prototypes is reset into either room.

Checkpoint 765's all-442 title and history review found “The Unique's Vault” has one current exact title owner and no followed same-stem predecessor or replacement. Its object imports do not make consumer maps alternate owners. Keep this separate from any map with similar artifact lore or a consumer reset; see [title and ownership review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-765-unique-vault-title-and-object-ownership) and [checkpoint 415 graph review](WORLD_CONNECTIVITY.md#checkpoint-415-unique-vault-boundaries-and-directed-access).

## Active external ZON imports

A token-exact scan of current ZON commands, classified by source stem in areas/AREA, found 42 active external reset commands across 38 active contributors. Each command points to an OBJ prototype owned by unique.obj; there are 42 distinct target prototypes. The reset opcodes show source intent only and do not establish that an item is currently spawned, available, or in stock.

| Active source | ZON line(s): opcode → Unique OBJ |
| --- | --- |
| airshipgrave | 300: G → #67208 |
| astral_main | 45: G → #67206 |
| bahamut | 310: E → #67200 |
| banditca | 222: G → #67262 |
| barovia2 | 399: G → #67215 |
| basin_wa | 142: G → #67240 |
| brass | 464: O → #67202; 467: P → #67244; 1031: G → #67245 |
| caves_skelenak | 164: G → #67270 |
| citadel | 601: G → #67239 |
| clfhaven | 1145: G → #67242 |
| darkfall | 35: G → #67259 |
| dlsc | 503: G → #67235 |
| hall | 464: G → #67201 |
| hunt | 164: O → #67278 |
| icecrag | 740: G → #67252 |
| icetower | 306: G → #67258 |
| jade | 586: G → #67212 |
| jotun | 526: G → #67241 |
| juiblex | 222: O → #67274 |
| khildarak | 2250: G → #67268 |
| labyrinth | 222: G → #67233 |
| mansion | 358: G → #67203 |
| mira | 357: G → #67221 |
| mist_chasm | 112: G → #67273 |
| negplane | 495: G → #67281; 581: G → #67243 |
| new_cavecity | 172: G → #67272 |
| pharrvly | 462: G → #67255 |
| plane_air_one | 57: G → #67207 |
| plane_fire_one | 65: G → #67210 |
| prisonb | 306: G → #67230 |
| ravenloft2 | 1178: G → #67260 |
| realm | 322: G → #67256; 381: G → #67250 |
| shadamehr_keep | 38: O → #67219 |
| shipy | 518: G → #67261 |
| suntmpl | 360: G → #67253 |
| swamp_two | 59: G → #67238 |
| tezcat | 193: G → #67280 |
| trnsptow | 156: O → #67276 |

juiblex is counted as active by its stem in both current area indexes. Its ZON starts with header #875 while the two index rows say *876; that separate header/index mismatch is retained in the Juiblex coverage row and is not resolved by this Unique review.

## Inactive candidate reset references

Four additional ZON commands occur in three non-active source groups. They are not active Unique stock routes. Preserve them on their existing candidate plans:

| Candidate source | ZON record | Candidate plan |
| --- | --- | --- |
| brass-old-1 | line 466 P → #67244; line 1040 G → #67245 | [candidate row 16](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17) |
| brass-old-2 | line 505 G → #67245 | [candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L18) |
| nexus56 | line 1585 E → #67276 | [candidate row 78](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79) |

These are source-only candidate records. Their occurrence does not authorize copying the prototypes, merging map identities, or moving any source group.

## Other typed and compiled references

- Current tracked QST and SHP sources contain no direct numeric references to Unique OBJ prototypes #67200–#67283. unique has no local QST, SHP, or story-sidecar file; the QST/AREA entry is index metadata.
- Current object sources contain no external type-25 destination to rooms #67200 or #67300. The only exact #67200 object-source hit is Unique's own OBJ declaration. The local map has no WLD target or reset path beyond its reciprocal room pair.
- The 84 contiguous prototypes #67200–#67283 have no same-type active OBJ definition in another current contributor; coverage records zero exact OBJ overlap.
- specs.assign.c binds 21 Unique prototypes to object callbacks: #67200, #67203, #67206, #67207, #67215, #67220, #67237, #67239, #67242, #67243, #67244, #67245, #67260, #67262, #67272, #67274, #67276, #67279, #67280, #67281, and #67282. The specs.assign.c #67258 icicle-saber line is commented out.
- Additional object-specific code recognizes #67207 for air-staff conjuration and #67243 for necroplasm behavior, including its native item-action and artifact hooks. tradeskill.c mentions #67283 only in commented-out material cases; it is not a live tradeskill consumer. Generic #67200–#67299 range checks are not per-prototype bindings.

## Disposition and limits

Keep active unique DO NOT MOVE and add no attic candidate. Preserve all 42 active resets as imports of Unique-owned prototypes, and keep the four inactive commands on Brass Old 1 row 16, Brass Old 2 row 17, and Nexus56 row 78. No object placement, shop, redemption, encounter, safety, or live availability claim follows from the static source. Installed/editor state, production, persisted player-held copies, and runtime behavior remain outside this checkpoint; no migration or gameplay clearance is granted.

