# Dalvik’s Dockyard: source graph and identity audit

Reviewed October 11, 2026, Checkpoint 466, against source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. This is a static review of the unregistered `dalvik` component. It does not establish that these files are loaded, that authored routes are usable, or that a path move is safe.

## Source scope and narrative

The current component consists of [`dalvik.wld`](../../../areas/wld/dalvik.wld), [`dalvik.mob`](../../../areas/mob/dalvik.mob), [`dalvik.obj`](../../../areas/obj/dalvik.obj), [`dalvik.zon`](../../../areas/zon/dalvik.zon), and [`dalvik.qst`](../../../areas/qst/dalvik.qst). It is absent from the full [`areas/AREA`](../../../areas/AREA) and [`areas/qst/AREA`](../../../areas/qst/AREA) lists. The raw area annotation is blank; its ZON header is #1, title is “Dalvik’s Dockyard,” and last VNUM is 476. Rooms occupy #1–#476, four mobile prototypes occupy #1–#3 and #162, 63 objects occupy #1–#63, and two QST giver sections contain 420 QA contracts in total. There is no current same-stem SHP file or story sidecar.

The room descriptions cover jungle trails and the Khomani-Khan, Manshaka’s streets and Blood Arena, the Great Temple of Ubtao, the shipyard and docks, warehouses, the Broken Mast, a trade guild, and a slave compound. Mobile #2 is described as a trade guild leader and mobile #3 as a doomsayer high priest. The source-only QST sections #2 and #3 each contain 210 QA entries. Their text describes deals and coins in #2 and a blue-light transformation to water in #3; the raw records reference object VNUMs #400000–#400209. All 210 IDs are defined by active `tradeskills.obj`. This is authored content and numeric linkage, not proof that the QST source or its givers are loaded.

The ZON resets place mobile #2 in room #470 and mobile #162, the Manshaka slavemaster, in #162. They do not place mobile #3. The ZON has 54 door-reset records, two mobile-reset records, and eleven object-reset records. Nine object resets use local objects #1–#8 and #63; two import object #72 and #132581 from outside this candidate OBJ file. There is no SHP record despite several storefront room descriptions.

## Static WLD graph

| Measure | Result |
| --- | --- |
| Rooms / authored directions | 476 / 1,274 |
| Candidate-local direction targets | 1,273 |
| Outbound target in the active WLD set | One: #298 south → Surface #635660 |
| Positive unresolved / negative targets | 0 / 0 |
| Weak local components | 34; largest has 434 rooms |
| Plain directed reach from #1 / directed return to #1 | 411 / 427 rooms |
| Fall / current fields | 7 rooms / 6 rooms |
| Active WLD numeric inbound references | One Thetis exit to #104; owner unresolved |

Every room lies within the raw #1–#476 VNUM band and all 1,273 internal exit targets have local room definitions. From room #1, the raw directed graph reaches 411 rooms, including #298; 427 rooms have an authored directed path back to #1. These traversals ignore door state, secrecy, hazards, live conditions, and player safety. Fall fields occur in #250–#255 and #266; current fields occur in #250–#255.

Room #298, “Dalvik’s Bay,” has one south exit to active Surface room #635660. The source-only direction was already included in the Checkpoint 373 Surface recount, so this review does not add it to curated inactive-WLD totals. Current code maps Surface port #635260, named “Dalvik,” to arrival room #635660 ([`ship_variables.c`](../../../src/ships/ship_variables.c#L41)); Chaos portal aliases also name #635260 “dalvik kk” ([`chaos.c`](../../../src/combat/chaos.c#L28)). These numeric and name matches are relevant context, but they do not establish that the unregistered Dalvik map is loaded or that the source-only exit is an operational route.

The active Thetis WLD retains #38095 north → #104 as a positive unresolved target ([unresolved-exit row 209](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L209), [Thetis review](THETIS_AUDIT_REVIEW.md#unresolved-exit-to-104)). Dalvik defines #104 as the Great Temple of Ubtao; inactive Mavulsk #104 is a quiet library. Neither source is in areas/AREA. Do not assign Thetis’s target to Dalvik based on numeric coincidence; see the [Mavulsk audit](MAVULSK_AUDIT_REVIEW.md#thetis-target-104) for its wider low-VNUM collisions.

## Typed references and numeric ownership

The current full-area WLD set contains 50 VNUMs in Dalvik’s #1–#476 room range, and none of those same-number records is text-identical to the Dalvik room block. Dalvik mobile VNUMs #1–#3 overlap different active Limbo mobiles #1–#3; #162 has no active same-number mobile definition. All 63 Dalvik object VNUMs overlap active object VNUMs: #1–#9 are Limbo objects and #10–#63 are Heavens objects. None of those 63 complete object blocks matches its active same-number owner. In particular, Dalvik #53 is a dazzling sapphire bracelet, while active Heavens #53 is a rope ([Dalvik #53](../../../areas/obj/dalvik.obj#L681), [Heavens #53](../../../areas/obj/heavens.obj#L788)). These collisions make filename- or number-only reassignment unsafe.

The two external objects used by the candidate’s ZON are currently defined as active Heavens #72 and active Tharnadia #132581 ([Heavens #72](../../../areas/obj/heavens.obj#L1000), [Tharnadia #132581](../../../areas/obj/tharnadia.obj#L1373)). The candidate QST’s #400000–#400209 item references resolve numerically to 210 prototypes in active Tradeskills. Because Dalvik’s QST is absent from `areas/qst/AREA`, `areas/world.qst` is absent in this checkout, and [`open_quest_stream`](../../../src/world/quest.c#L2086) explicitly does not glob `areas/qst/*.qst`, these records establish source dependencies only. They do not establish active contracts.

The word Dalvik is also used by the separate Surface ship-port code and by mobile #11006 in `player_castles.mob`; those uses do not identify the five-file candidate as their owner. No current candidate SHP product or keeper exists to map.

## Title, tracked history, and retention

After color-token and whitespace normalization, “Dalvik’s Dockyard” is unique among the 442 current ZON title sources. The current tracked history visible for each of the five candidate paths is limited to the source-file addition and a line-ending cleanup (`3f835d646`, `428ebfddf`). No alternate-stem rename, predecessor, or explicit whole-zone replacement is visible in that bounded history. It does not establish earlier upstream lineage or prove that no predecessor existed.

Keep candidate row 29 **RETAIN IN PLACE**. Resolve why the component is absent from both manifests; the owner of the #1–#476 map; the collision families; Thetis’s #104 destination; the Surface port relationship; both external ZON object imports; the 420 QST contracts and their loader path; and editor, deployment, and persisted-data references before considering any separate path or activation proposal. See the [28 builder questions](DALVIK_BUILDER_QUESTIONS.md), [coverage row 380](ZONE_COVERAGE_REGISTER.csv#L380), [candidate row 29](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L29), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-466-dalvik-dockyard-source-graph), [identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-466-dalvik-title-map-identity-and-history), and [migration plan](ATTIC_MIGRATION_PLAN.md#checkpoint-466-dalvik-retained-pending-owner-review).
