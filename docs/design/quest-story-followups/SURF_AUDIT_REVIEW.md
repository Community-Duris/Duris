# surf source-component audit

Canonical key: component:surf. Coverage data row 447 / CSV line 448; candidate dependency data row 96 / CSV line 97. Reviewed October 11, 2026, Checkpoint 534, against checkout c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static source, manifest, graph, identity, typed-reference, code-binding, and followed-history review. It does not establish that Surf is loaded, that its resets run, or that its authored directions are currently traversable.

## Inventory, title, and manifest status

The four tracked files are [surf.wld](../../../areas/wld/surf.wld#L1), [surf.mob](../../../areas/mob/surf.mob#L1), the tracked but empty [surf.obj](../../../areas/obj/surf.obj), and [surf.zon](../../../areas/zon/surf.zon#L1). The WLD defines 90,000 unique rooms #200001–#290000. The MOB defines 322 prototypes #200000–#200321. The OBJ defines no prototypes. The ZON header is #200000, its title is “World of Duris,” its last room is #290000, its reset mode is 0, and it contains a 300 by 300 grid declaration. It has 492 reset commands: 453 M, 6 G, 32 F, and 1 E. There is no same-stem QST, story sidecar, or SHP file.

After ANSI color removal, whitespace normalization, and case folding, “World of Duris” occurs only in surf.zon among all 442 current ZON sources. The similarly named active Surface contributor has a distinct key and title. Surf is absent from [areas/AREA](../../../areas/AREA), [areas/qst/AREA](../../../areas/qst/AREA), and the empty current [RANDOM_AREA](../../../areas/RANDOM_AREA). The current QST manifest has no Surf entry. A type-aware scan found source-only headers and item operands in unlisted `surface2011.qst` and `underdark2011.qst` whose numbers overlap Surf’s mobile range; their source ownership and type distinction are recorded in Checkpoint 936. These absences describe repository manifests; they do not establish the intended deployment state.

## Competing map and prototype identities

Surf’s 90,000 room numbers overlap 89,999 records in unlisted surface2011.wld (#200000–#289999) and 15,481 records in unlisted underdark2011.wld. A streaming full-record comparison found no identical room blocks in either source. The sample names also differ: Surf #200001 is “The Great Sea of Silvanos,” while surface2011 #200001 is “The Azure Waters of a Stormy Sea.” Surf and surface2011 use shifted ranges, and underdark2011 is sparse within the overlapping range.

The 322 Surf mobile VNUMs overlap 213 surface2011 prototypes and 68 underdark2011 prototypes; none of the compared full mobile records is identical. Neither competing source is listed in areas/AREA, and no active MOB source defines a Surf mobile VNUM. Number overlap therefore leaves three distinct source families; it does not identify an active owner, a replacement, or an archive target.

Followed history shows WLD editing in 2011, a UTF-8 recode in 2025, and an ANSI cleanup in August 2026. MOB history includes a 2016 Kuo-Toa race adjustment and the 2026 ANSI cleanup. The tracked empty OBJ reaches back to the 2005–2006 history; ZON history reaches 2013. This maintenance history does not record an explicit predecessor, successor, retirement, or replacement decision. Keep Surf, surface2011, and underdark2011 as separate source identities pending owner and toolchain provenance.

## Authored graph and current numeric resolution

The 90,000-room Surf graph contains 359,659 authored exits: 359,228 target another defined Surf room and 431 target a number outside Surf’s room set. The direction counts are D0 89,926; D1 89,901; D2 89,906; D3 89,902; D4 2; and D5 22. There are no negative targets, no in-range undefined target numbers, no source rooms outside #200001–#290000, and no exits sourced from an out-of-range room. All local edges form one weak component of 90,000 rooms. A simple directed walk from #200001 reaches 89,894 rooms when access conditions are ignored; this is a source-graph observation, not gameplay reachability.

Of the 431 external target records, 415 target room VNUMs defined by the current active WLD set. Sixteen do not: three target VNUMs are defined only in inactive asunwa.wld (#35300) or jungle.wld (#67005 and #67087), while thirteen records target twelve VNUMs with no current WLD definition (#33807 twice, #33808, #33933, #66154, #76501, #76526, #91538, #94728, #95351, #95352, #95355, and #335270). The three inactive-source records include Surf #247317 south to #35300, #252412 south to #67005, and #253012 north to #67087. This recount corrects the older coverage prose that said 416 raw outbound records matched current loaded-room VNUMs; it adds no route to the separate global curated total.

The active unresolved-exit register has 145 WLD directions targeting Surf’s 90,000-number range, originating in 17 active contributors: ashrumite, charing, connectorzones, crakkaro, crypt, desolateinv, heavens, lava_tubes, minizones, mistywood, mobs_surf, olympus, patrols, rftjngle, sea, shipy, and wildland_trails. Examples include Heavens #36 up to #201670, Minizones #5700 north to #209375 and #5795 west to #252703, Lava Tubes #12138 east to #282989, Crypt #14568 up to #265588, and Patrols #30500 north to #207259. These 145 targets are identity-ambiguous: 134 also match surface2011, and the other 11 match surf, surface2011, and underdark2011. No active inbound direction can be assigned specifically to Surf from VNUM alone. Seventy-five of the 145 have an authored reverse direction in surf.wld; with the source unlisted and room identities unresolved, those records are not evidence of current reciprocal travel. See the [unresolved-exit register](WORLD_UNRESOLVED_EXIT_REGISTER.csv), [Surface Realm review](SURFACE_AUDIT_REVIEW.md), and [map identity checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-534-surf-surface2011-and-underdark2011-map-identities).

## Reset, object, and code dependencies

The 453 M commands include 42 rows using 18 Surf-local mobile prototypes, 365 rows whose mobile VNUMs resolve to active MOB prototypes, and 46 rows whose target numbers are neither Surf-local nor current active mobiles. Since surf.zon is not in areas/AREA, these rows are source-only placements. The 32 F rows refer to the candidate reset map and do not establish an active reset.

Checkpoint 547 identifies one such source-only placement exactly: [surf.zon line 162](../../../areas/zon/surf.zon#L162) resets mobile #100116 into room #349161. That room exists only in the separate unregistered [ud.wld map](../../../areas/wld/ud.wld#L155058) and is titled “Zone Entrance.” Preserve the reset as typed source evidence, not an active spawn or proof that Surf and ud share a route. See the [ud audit](UD_AUDIT_REVIEW.md#other-typed-references).

Six G rows reference active Killing Fields objects #33309–#33312, #33314, and #33315. One E row equips active object #5720 on a source mobile. The source OBJ file is empty, so these are external typed references, not local objects. Earlier reviews remain relevant: CP73 found 209 Surf M commands referencing 16 active Great Realm mobile prototypes; CP144 found source-only Surf reset references to Killing Fields mobile #33308 and the six objects above at room #221734; CP503 found 45 Surf M commands using nine Map5 mobile VNUMs across 40 Surf rooms. See the [Map5 audit](MAP5_AUDIT_REVIEW.md) and the Great Realm and Killing Fields links in the coverage row.

Code also contains direct assignments for Surf mobile VNUMs #200011, #200012, and #200321 at [specs.assign.c lines 436, 439, and 470](../../../src/specs/specs.assign.c#L436). The helper [real_mobile0()](../../../src/world/db.c#L8553) returns zero when the requested prototype is absent. These are direct source/code bindings; they do not demonstrate that the inactive prototypes enter the active table or that the assignments have a live effect.

## Disposition

Retain surf.wld, surf.mob, surf.obj, and surf.zon in place. Confirm map ownership, the intended relationship among the three overlapping source families, the 145 inbound target identities, the unresolved outbound destinations, reset imports, code bindings, and the authoritative editor/export/build inputs before proposing any separate activation, rewrite, move, merge, or archive. The 21 builder questions in [SURF_BUILDER_QUESTIONS.md](SURF_BUILDER_QUESTIONS.md) record the decisions needed. No migration clearance is implied.

## Checkpoint 821: source-only Randomness mobile resets

A typed scan of `surf.zon` finds 80 source-only M reset rows at lines 129–217 for 28 Randomness mobile VNUMs (#100041, #100083, #100092, #100094–#100098, #100101, #100113, #100115–#100116, #100121, #100124, #100132, #100135, #100140, #100307–#100308, #100310, #100316, #100318, #100320–#100321, #100323, #100327–#100329). `surf` is not an active area contributor, so these are not active mobile placements. Preserve them under Surf candidate data row 96 / CSV line 97 **RETAIN IN PLACE**; this does not change the 145 active WLD-ingress count or establish an active route. See the [Randomness typed audit](RANDOMNESS_AUDIT_REVIEW.md#checkpoint-821-current-typed-dependency-closure), [`surf.zon`](../../../areas/zon/surf.zon#L129), and [candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

## Checkpoint 936: old Surface QST values do not identify Surf mobiles

A type-aware scan found 37 QST headers and 56 `G I`/`R I` item operands
in `surface2011.qst` within Surf's #200000–#200321 mobile range; 32 distinct
item VNUMs occur. `underdark2011.qst` adds one header and four item
operands in the same range. Neither QST file is listed in
`areas/qst/AREA`, and neither parent source family is in `areas/AREA`. The
headers are source-only QST mobile keys owned by those files; the item
operands resolve only to source OBJ definitions in unlisted
`surface2011.obj` and/or `underdark2011.obj`. Surf's OBJ file is empty, and
the current full-area manifest has no exact active MOB or OBJ prototypes
for these IDs. The same numbers in `surf.mob` do not turn the QST records
into Surf dependencies or merge Surf with the other map families.

No typed `mob_vnum`, `giver_vnum`, or item VNUM in the story sidecars
matched Surf's 322 mobile IDs; story `E:200000` values are experience
amounts, not VNUM references. The mini-source inventory adds no matching
MOB/OBJ prototype, and the only direct source-code assignments in this
range remain #200011, #200012, and #200321 as previously recorded. No WLD
graph count, 145 ambiguous active inbound target count, or 431 outbound
record count changed. Retain Surf, `surface2011`, and `underdark2011` as
separate source families pending owner and loader decisions. See
[surface2011 QST](../../../areas/qst/surface2011.qst#L1),
[sample item operands](../../../areas/qst/surface2011.qst#L32),
[underdark2011 QST](../../../areas/qst/underdark2011.qst#L1),
[underdark item operands](../../../areas/qst/underdark2011.qst#L34),
[Surf mobile #200000](../../../areas/mob/surf.mob#L1),
[surface2011 object #200000](../../../areas/obj/surface2011.obj#L1), and
[underdark2011 object #200000](../../../areas/obj/underdark2011.obj#L1),
and the [surface2011 QST audit](SURFACE2011_AUDIT_REVIEW.md#qst-narrative-and-typed-item-dependencies).
