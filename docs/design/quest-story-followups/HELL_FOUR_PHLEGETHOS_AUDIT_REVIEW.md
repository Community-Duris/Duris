# Hell, Phlegethos audit review

## Source inventory

The canonical contributor is area:hell_four_phlegethos. It appears in the full area list at [areas/AREA line 224](../../../areas/AREA#L224) and the QST roster at [areas/qst/AREA line 205](../../../areas/qst/AREA#L205). Its zone title is Hell, Phlegethos, zone number 522, reset mode 1, range ending at 52599 ([zone header](../../../areas/zon/hell_four_phlegethos.zon#L1)).

Checked-in files are a 400-room WLD, one MOB prototype, one OBJ prototype, and a ZON file with no reset commands. There is no same-stem QST, SHP, or story sidecar. The WLD covers #52200-#52599. The only mobile and object prototypes are both #52200 and are named nothing monster and nothing; no external same-role declarations duplicate these VNUMs.

The register records source snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697. A source diff from that snapshot to current HEAD c500ac51b8cd26540943257e7ca4e9807263ef74 is empty for areas/ and src/, so the checked-in area and code evidence remain at the recorded source state.

## Story and quest census

Every room has the same short title and the same long environmental paragraph: a tortured volcanic plane with fire, ash, smoking-dung pits, persistent heat, tremors, eruptions, and fissures. The prose describes why intruders run, but none of the 400 rooms identifies a named landmark. The text uses fouth and gouting fires ([WLD source](../../../areas/wld/hell_four_phlegethos.wld#L1)).

The local MOB and OBJ entries are generic placeholders. The ZON terminates at S without any reset command, so it does not place them. The roster entry in areas/qst/AREA does not substitute for a local QST file. No SHP file, story sidecar, or Q/QA contract supplies a separate objective or ending. There is no source basis for treating this as a quest zone.

No fall-chance or current field is authored in the room records. The environmental hazards appear in room prose; a distinct heat, eruption, fissure, or movement effect was not found in the zone source or an area-specific code binding.

## Static connectivity and reachability

The 400 room records each contain D0-D3 exits, for 1,600 directed exits. All targets resolve within #52200-#52599. Every exit has empty description and keyword strings and exit-info flags of zero. No authored exit leaves the numeric zone range.

The undirected interpretation is one weak component of 400 rooms. Direction matters: the directed graph has five strongly connected components, one of 396 rooms and four single-room components. Starting at #52200 reaches and can return through the 396-room component; #52311, #52312, #52313, and #52599 are not directionally reachable from that seed. Eighty-six directed exits have no matching reverse-direction exit. This is a source-graph result, not a claim about a live player start, because no active ingress is established.

A scan of all 446 WLD files found no room-VNUM overlap and no inbound exit target in the other 445 files: 350 other active WLD files and 95 inactive WLD files. No Phlegethos exit points to an outside room. Static WLD sources therefore establish neither an entry nor an exit route.

The Bronze Citadel object catalog includes object #32456 named shimmering portal phlegethos, with value line 99999 0 100 ([object prototype](../../../areas/obj/bctdl.obj#L374)). The BCTDL ZON line that also contains number 32456 is an M reset placing mobile #32437 in room #32456; it does not load object #32456 ([reset line](../../../areas/zon/bctdl.zon#L244)). No object reset or code read of the portal prototype was found. Its destination value does not identify a valid Phlegethos arrival in the checked-in evidence.

A code search also found digits 52333 inside Verzanan's Piergeiron movement script ([path string](../../../src/specs/specs.verzanan.c#L48)). The code processes the path one character at a time as movement directions through exitnumb_to_cmd, rather than parsing 52333 as a room VNUM ([path dispatch](../../../src/specs/specs.verzanan.c#L304)). It is not a direct Phlegethos target. Dynamic spells, staff travel, and live destination state were not exercised.

## Inbound and outbound dependency closure

Typed source scans found no external active or inactive WLD exit into or out of Phlegethos, no external M/O/G/E/P or D/R ZON role reference to its room, mobile, or object VNUMs, no external QST numeric target, no SHP product targeting object #52200, and no story-sidecar VNUM reference. The 872 external type-25 object declarations contain no target in #52200-#52599. No other WLD, MOB, or OBJ file declares a VNUM in the matching local namespace.

The Bronze Citadel portal prototype is retained as an unresolved named reference only; its same-number M reset is a room destination in a different VNUM namespace. No active importer was established. The local ZON has no reset-generated outbounds, and no explicit source-code VNUM binding into the room range was found.

## Title and replacement history

The normalized title Hell, Phlegethos is unique among 442 current ZON titles. The other named Nine Hells areas, Avernus, Dis, and Minauros, remain distinct sibling contributors; their shared setting does not show that any one replaces this area.

Followed history shows the files entering the tracked areas tree in the 2009 source-tree move, broad edits in 2012 and 2013, and a 2015 mobile/reset field update. It exposes no alternate-stem predecessor, title rename, deletion, or whole-zone replacement. This is limited to the repository's tracked history; it does not prove undocumented builder history.

## Builder questions and recommended resolutions

The [source dossier's builder table](../zone-stories/HELL_FOUR_PHLEGETHOS.md#builder-questions-and-recommended-resolutions) records five owner questions: whether repeated room copy is intentional; whether one-way exits and four seed-unreachable rooms are deliberate; whether the Bronze Citadel portal is meant to work; whether placeholder prototypes and the empty reset block are reserved; and whether quest-free environmental copy is intentional. It recommends source-backed answers and a documented entry/return route before treating the area as safely reachable.

## Attic disposition

Keep this active full-area contributor **DO NOT MOVE**. It is not a source-only candidate, and no candidate-specific attic plan or candidate row is warranted. The ambiguous portal template does not establish an active dependency or retirement clearance. Any future disposition change requires owner-confirmed content intent and a route, return, and safety review; no zone move is proposed here.

## Checkpoint 817: static reachability reconciled

CP817 reconciles the source-backed connectivity result with coverage row 121. The 400-room map contains 1,600 internal directed exits in one weak component and five strongly connected components. From analysis seed #52200, the 396-room component is mutually reachable; rooms #52311, #52312, #52313, and #52599 lie outside that seed's directed reach. Eighty-six exit records lack a reverse-direction record. This describes authored topology, not a verified player start or universal runtime lockout.

The complete 446-file WLD scan still finds no same-number room overlap, external target into the range, or outbound edge. Bronze Citadel object #32456 remains a template with destination value 99999; the same number in the BCTDL ZON is a room operand for a mobile reset, not an object load. No checked-in reset or direct code loader makes the portal active. Dynamic spells, staff travel, live destination state, and player return/safety were not exercised. Keep the active contributor **DO NOT MOVE**; no source-only candidate or candidate-specific attic plan is established. See the [reachability report](WORLD_CONNECTIVITY.md#checkpoint-817-phlegethos-reachability-remains-analysis-seed-bounded), [source dossier](../zone-stories/HELL_FOUR_PHLEGETHOS.md#connectivity-and-retention-boundary), [attic disposition](ATTIC_MIGRATION_PLAN.md#checkpoint-817-phlegethos-remains-an-active-retained-area), and [coverage row 121](ZONE_COVERAGE_REGISTER.csv#L122).
