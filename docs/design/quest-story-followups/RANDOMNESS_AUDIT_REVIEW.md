# Randomness typed dependency audit

## Checkpoint 821: current typed dependency closure

This source review closes the typed reset, stock, compiled encounter-selector, and static WLD-reference census for active Randomness. It does not establish live spawn frequency, shop availability, editor or deployment identity, persisted player references, indirect scripted references, or safe runtime access.

### Prototype ownership and local inventory

The current un-commented [`areas/AREA` entry](../../../areas/AREA#L365) owns active `random`; the QST roster also lists it at [`areas/qst/AREA` line 323](../../../areas/qst/AREA#L323), although `areas/qst/random.qst` is absent. Randomness defines 10,000 rooms (#100000–#109999) and 199 mobiles (#100001–#100168 and #100300–#100330). Its [`random.obj`](../../../areas/obj/random.obj) is empty, and there is no local story sidecar. The active WLD starts with blank “Unnamed” rooms ([room #100000](../../../areas/wld/random.wld#L1)).

Every one of Randomness’s 10,000 room numbers also appears in unregistered [`Duris3.wld`](../../../areas/wld/Duris3.wld#L1). All paired Randomness titles are “Unnamed”; the Duris3 rooms carry “The Ocean.” This is an exact room-number overlap between separate source map identities, not evidence of a route, predecessor, or replacement. The current Duris3 plan remains separately retained on [candidate data row 35 / CSV line 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36).

### Local resets and shop stock

[`random.zon` lines 10–17](../../../areas/zon/random.zon#L10) contain eight reset commands and ten typed operands: merchants #100013 and #100015 at rooms #105301 and #105302, plus six imported object stock resets. The active owner of each imported object is:

- Heavens #384, oak water barrel; #364, iron rations; #347, leather waterskin; and #53, coil of hemp rope ([prototype sources](../../../areas/obj/heavens.obj#L4020), [#364](../../../areas/obj/heavens.obj#L3796), [#347](../../../areas/obj/heavens.obj#L3600), [#53](../../../areas/obj/heavens.obj#L788)).
- Lizard #6008, severed Ss'lith lizardman head ([prototype](../../../areas/obj/lizard.obj#L145)).
- Asylum #9225, cup of coffee ([prototype](../../../areas/obj/asylum.obj#L327)).

[`random.shp`](../../../areas/shp/random.shp#L1) has exactly two shop headers, #100013 and #100015, which stock the same four and two objects respectively. This reconciles the local shop count in [coverage row 246 / CSV line 247](ZONE_COVERAGE_REGISTER.csv#L247) to 2. These source records do not verify restock, shop admission, or stock availability.

### External typed consumers

Across active and source-only ZON files, 84 reset records reference Randomness mobiles. Active [`surface.zon`](../../../areas/zon/surface.zon#L1666) has four rows for Randomness mobile #100323, the xorn ([prototype](../../../areas/mob/random.mob#L2547)). Unregistered, source-only [`surf.zon`](../../../areas/zon/surf.zon#L129) has 80 rows across 28 mobile VNUMs: #100041, #100083, #100092, #100094–#100098, #100101, #100113, #100115–#100116, #100121, #100124, #100132, #100135, #100140, #100307–#100308, #100310, #100316, #100318, #100320–#100321, and #100323, #100327–#100329. Keep those 80 candidate resets on Surf’s [existing RETAIN plan, data row 96 / CSV line 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); they are not active reset consumers and do not change Surf’s active WLD-ingress count.

No external QST item or giver/mobile role, structured story `item_vnums`/`mob_vnum`/`giver_vnum` field, or shopkeeper binding references a Randomness prototype. No external reset references a Randomness room or object; there are no local object prototypes. Existing coverage already records Randomness’s active use of Asylum #9225 and Lizard #6008; see [Asylum](ASYLUM_AUDIT_REVIEW.md), [Lizard](LIZARDMAN_SWAMPS_AUDIT_REVIEW.md), and [Heavens coverage row 120](ZONE_COVERAGE_REGISTER.csv#L121).

### Compiled selectors and broad numeric rules

[`src/mob/encounters.c` lines 68–81](../../../src/mob/encounters.c#L68) selects VNUMs #100300–#100330 for Underdark sectors and #100001–#100168 elsewhere, then calls `read_mobile`. Those two ranges cover Randomness’s 199 local mobile prototypes exactly. This is a separate selector from the theme-based `create_random_mob` path, which uses templates #1255/#1256 ([`random.mob.c` lines 585–612](../../../src/world/random.mob.c#L585)). The static room-range helper is only #100000–#105000 inclusive ([`random.zone.h` lines 27–34](../../../src/world/random.zone.h#L27)), shorter than Randomness’s full WLD range.

The generic [`IS_RANDOM_MOB` predicate](../../../src/core/utils.h#L709) covers NPC VNUMs strictly between #100000 and #200000; [`handler.c`](../../../src/world/handler.c#L5873) also has a generic cleanup check for #100000–#109999. These are numeric-range behaviors, not bindings to an individual Randomness mobile. The all-object field scan found no type-25 fixed/random destination or type-29 switch target-room field naming a Randomness room.

### Static WLD closure and candidate identity

The active Randomness WLD has 39,600 authored and resolved exits, one weak component containing all 10,000 rooms, and zero active cross-area ingress, egress, or reciprocal WLD directions. No WLD door-key VNUM resolves to a local Randomness object; the area has no local OBJ prototype. A current all-project scan covered the 446 `areas/wld/*.wld` zone-source files plus the two additional `.wld` files elsewhere in the repository: 448 files and 2,987,758 parsed exit records total. It found no active external exit targeting #100000–#109999. The two nonstandard door records are in `northern_wilderness.wld` at source rooms #12272 and #12310; their indicated targets (#12270 and #6803) are outside Randomness. The only non-overlapping source-only exits targeting Randomness’s numeric interval are 602 directions in `Duris3.wld`, all part of its separate unregistered map source; these do not establish an active Randomness route. The full typed scan adds no WLD edge; see [WORLD_CONNECTIVITY.md](WORLD_CONNECTIVITY.md#checkpoint-821-randomness-typed-references-do-not-add-active-wld-edges).

Keep active `random` **DO NOT MOVE**. Do not add a Randomness candidate. Preserve the separate **RETAIN IN PLACE** plans for Surf’s source-only resets, Duris3’s overlapping room IDs, and map22’s distinct #900000–#909999 random-lab grid ([map22 candidate row 64 / CSV line 65](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L65)). Keep the active Heavens #53 object separate from same-number source-only copies in Dalvik, Hostel, and Mavulsk; their respective candidate rows remain **RETAIN IN PLACE** ([Dalvik row 28 / line 29](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L29), [Hostel row 51 / line 52](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L52), [Mavulsk row 69 / line 70](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L70)). None of these numeric overlaps authorizes activation, merging, renumbering, or a path move. Runtime availability, persisted identity, and indirect references remain open.
