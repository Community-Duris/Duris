# Neverwinter Woods repository-wide source audit

Checkpoint 291 supplements the source-comprehensive [Neverwinter Woods
dossier](../zone-stories/NEVERWINTER_WOODS.md) with the repository-wide title,
history, connectivity, outside typed-reference, and disposition review. The
review is static; it does not qualify live reset stock, travel, contracts,
maze access, player state, or successful gameplay.

## Checkpoint 291: all-442 title and bounded history

After color-code, trailing-tilde, case, and whitespace normalization,
“Neverwinter Woods” has one owner among all 442 current `.zon` files:
[`moria.zon`](../../../areas/zon/moria.zon#L2). The source stem is `moria` and
the zone header is #990. Followed WLD and MOB histories remain on the same
stem from the 2009 area-tree move through later edits, including the January
2026 WLD maze change and its reversion. OBJ and ZON histories remain on that
stem through 2015 and 2016 respectively; QST history stays on the stem through
2013, and SHP through 2014. The bounded paths establish no alternate-stem
rename, distinct predecessor, retirement, or whole-zone replacement. Related
forest wording and other source-map names remain separate content questions,
not replacement evidence.

Keep active `moria` **DO NOT MOVE**. Preserve both reciprocal Surface
boundaries, the source-typed arrivals and consumers below, and the local maze
implementation until an owner reviews any separate source change.

## Active graph and source-only WLD sweep

The active WLD has 335 rooms and 1,041 authored exits; all 1,041 resolve in
the active room catalog. The local weak graph is one component of 335 rooms.
Its two reciprocal cross-area pairs are:

| Moria endpoint | Surface reverse | Register evidence |
| --- | --- | --- |
| #99028 east → #528071 | #528071 west → #99028 | [Moria row 1183](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1183), [Surface reverse row 1380](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1380) |
| #99335 west → #528069 | #528069 east → #99335 | [Moria row 1184](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1184), [Surface reverse row 1379](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1379) |

All 95 current WLD files outside the active manifest were scanned. None has a
Moria room-number overlap or a source-only exit into the Moria room range. No
WLD route or curated source-only approach count changes. The compiled
`do_shift_prime` destination to #99295 and the typed references below are
separate from the authored WLD graph; they do not add static exit records.

## Bounded direct typed-reference closure

The local source contains seven native Q/M blocks: five Malchor exchanges and
two dialogue-response blocks. All five exchanges accept one each of rune
objects #99002–#99006 and declare different rewards. The repository-wide
outside-reference scan found these direct owners:

| Source and type | Target | Classification |
| --- | --- | --- |
| Active [`divhome.qst`](../../../areas/qst/divhome.qst#L286) and [`divhome.story.json`](../../../areas/story/divhome.story.json#L1301) | Moria object #99061, the golden horseshoes | One active outside QST consumer and its typed sidecar reference. Wicks #40716 pays 40,000 copper. This is separate from Malchor's rune story. |
| [`do_shift_prime`](../../../src/classes/innates.c#L2011) | Moria room #99295 | One compiled Githzerai destination among eight. The command selects it from the Astral plane and moves the character when the exact destination resolves; this is a dynamic code dependency, not a WLD exit or a claim of played access. |
| Unregistered [`moriatst.shp`](../../../areas/shp/moriatst.shp#L1), existing attic candidate row 73 | Mobile #99035–#99037, object stock #99063–#99070, and Moria rooms #99306/#99312/#99318 | Three records mirror the active shops in [`moria.shp`](../../../areas/shp/moria.shp#L1). Candidate raw buy values (0.90/1.00/1.00) clamp to 0.80; candidate #99037 sell value 1.00 remains below active Moria's 1.10. Normal `AREA` compilation omits the candidate; conditional runtime fallback can include it when `areas/world.shp` is unavailable, duplicating keeper/room records. See the [moriatst audit](MORIATST_AUDIT_REVIEW.md); no runtime boot was tested. |

All 231 story sidecars outside Moria parsed without error; only the Divine
Home #99061 contract matched a typed local item. Exact active outside ZON
reset scans found no Moria mobile or object consumer. No outside QST item
consumer was found beyond Divine Home. The `moriatst.shp` records are the
outside SHP reference identified by the source scan; they are omitted from
normal `AREA` aggregation, but the runtime fallback can read them when
`areas/world.shp` is unavailable. No outside MOB or OBJ prototype duplicates
a Moria mobile or object VNUM. WLD key fields do not refer to a Moria object.

The external object scan parsed 21,607 records. No outside ITEM_TELEPORT
destination (`value[0]`, or the random-zone field when `value[0]` is -1) or
ITEM_SWITCH destination (`value[1]`) targets a Moria room. A raw-number sweep
found #99328 and #99330 in `player_castles.obj`, but those tokens occupy the
extra-flags field; the serialized item-value arrays do not target those Moria
rooms. See the [object loader fields](../../../src/world/db.c#L4662) and
[teleport destination handling](../../../src/magic/spell_travel.c#L991).

The exact C/C++ scan found the 28 local mobile-procedure bindings in
[`specs.assign.c`](../../../src/specs/specs.assign.c#L1159), the Moria maze
implementation in [`specs.nw.c`](../../../src/specs/specs.nw.c#L1039), and the
outside #99295 `do_shift_prime` destination above. The maze routines are local
compiled consumers of Moria rooms, not outside area owners. No other exact
compiled room, mobile, or object reference to the Moria ranges was found.

## Checkpoint 291: direct builder and owner questions

These twenty questions preserve the source distinctions documented in the
dossier. Their recommendations do not claim live availability, travel,
completion, maze safety, or committed rewards.

1. **Question:** Should “Neverwinter Woods” remain the authoritative active
   title for source stem `moria`?
   **Recommendation:** Keep the unique normalized title owner and current
   source stem. Do not rename or merge the area based on broad forest wording.
2. **Question:** Does tracked WLD/MOB/OBJ/ZON/QST/SHP history establish a
   different-stem predecessor or replacement?
   **Recommendation:** No such predecessor, rename, retirement, or whole-zone
   replacement appears in the followed same-stem paths. Keep the bounded
   history result separate from any future content-identity judgment.
3. **Question:** Should Moria keep both reciprocal Surface boundaries at
   #99028/#528071 and #99335/#528069?
   **Recommendation:** Preserve both pairs as separate existing approaches;
   obtain an owner decision before changing either endpoint.
4. **Question:** Should the two Surface pairs remain the only authored WLD
   cross-area boundaries for Moria?
   **Recommendation:** Keep the full active and 95-file inactive-WLD census:
   there is no source-only WLD approach or room overlap, so add no candidate
   route from historical files.
5. **Question:** Should Githzerai `do_shift_prime` retain Moria room #99295 as
   one of its possible destinations?
   **Recommendation:** Preserve the compiled source reference and ask its
   owner before moving that destination. Keep it classified as conditional
   code travel, separate from WLD exits and Malchor's story.
6. **Question:** Should active Divine Home QST #40716 continue to accept Moria
   golden horseshoes #99061 for 40,000 copper?
   **Recommendation:** Preserve the outside typed consumer and its separate
   Divine Home sidecar. Do not make it a Malchor prerequisite or a Moria
   recovery receipt.
7. **Question:** Should unregistered `moriatst.shp` remain alongside active
   `moria.shp` with the same keepers and stock VNUMs?
   **Recommendation:** Keep it source-only on existing attic candidate row 73;
   ask its editor/tool owner whether it is a retained alternate or stale copy.
   Normal `AREA` aggregation omits it, but the runtime fallback globs it when
   `areas/world.shp` is unavailable. Resolve that entry path and the duplicate
   keeper/room bindings before any archive or move decision.
8. **Question:** Are the five native rune exchanges intended to share the
   same five-item offering while listing five different rewards?
   **Recommendation:** Preserve the current terms in the audit. If a selectable
   reward is intended, author an explicit selector; otherwise document the
   effective fixed reward and the shadowed variants.
9. **Question:** Should the journal continue to count the five native Q
   variants as one Malchor recovery family?
   **Recommendation:** Preserve one story and one achievement across historical
   variants unless the builder explicitly authors separate objectives. The
   five native giver bindings remain available for receipt compatibility.
10. **Question:** Does delivery prove that the player personally found or
    defeated a rune master?
    **Recommendation:** No. The native exchange accepts supplied matching
    runes; keep delivery, first acquisition, and personal victory as distinct
    claims.
11. **Question:** Should one exact copy of each rune kind remain required?
    **Recommendation:** Keep all five distinct item VNUMs. Several copies of
    one kind cannot substitute for a missing kind, and item flags do not prove
    the source of a carried rune.
12. **Question:** Does cap-one G stock guarantee a fresh rune set for every
    reset or eligible player?
    **Recommendation:** No. Keep source placement and cap records documented,
    but qualify current reset admission, shared limits, and committed stock
    separately before promising availability.
13. **Question:** Is the bridge wand optional while the ordinary reset opens
    the bridge east exit?
    **Recommendation:** Confirm the intended relationship between the open D
    reset and the blocked raw exit. Preserve the sidecar's optional-wand wording
    while the reviewed reset remains open.
14. **Question:** Should the wand inscription continue to say “wave” when the
    ITEM_SWITCH handler is triggered by `use`?
    **Recommendation:** Ask the builder to align the inscription or the handler
    in a separately reviewed change. Do not claim that waving opens the bridge.
15. **Question:** Does the guardian golem's offering language implement a
    payment, permission, or access exchange?
    **Recommendation:** No such acceptance is defined by its periodic procedure.
    Keep the golem's dialogue separate from the optional wand and bridge route.
16. **Question:** Should the maze's authored entrance room #99200 and exit room
    #99237 remain one-way-connected to the changing maze boundary?
    **Recommendation:** Decide and document the intended exterior pair before
    changing the `nw_mirroid` topology; the source implementation does not
    install a new exterior destination when it opens an existing edge.
17. **Question:** Should the maze boundary-search loops inspect each candidate
    room's exit instead of repeatedly inspecting the current mirroid's exit?
    **Recommendation:** Review the loops against each actual candidate edge and
    preserve intentional one-way paths. Do not infer successful maze access
    from the code comments.
18. **Question:** Should `e_room` be converted from a virtual room number before
    it is passed to `nw_reset_maze` as an index into `world`?
    **Recommendation:** Validate the VNUM-to-index conversion and bounds before
    any topology mutation; source inspection identifies a representation
    mismatch, not a played failure.
19. **Question:** Are the maze reset loops and block/unblock transitions meant
    to preserve reciprocal pointers, bounds, paired display, and confirmed
    movement?
    **Recommendation:** Freeze the intended 6×6 topology first. Check reverse
    exits before mutation, display both rooms when intended, and close the old
    edge only after movement confirms arrival.
20. **Question:** Should Agatha's debt, news, rescue, farm, logging, or healing
    flavor complete any part of Malchor's rune story?
    **Recommendation:** Keep those as separate owner-authored stories if they
    are implemented. The current sources define no Agatha payment, news,
    horseshoe acceptance, worker rescue, or local production achievements; the
    actual horseshoe exchange belongs to Divine Home.

Checkpoint 291 closes Moria's bounded title/history and direct typed-reference
reviews and completes twenty builder questions. It adds no WLD route, candidate
row, area-source change, or migration clearance.

## Checkpoint 510 supplement: moriatst shop-loader path

The source-only [moriatst review](MORIATST_AUDIT_REVIEW.md) sharpens the
Checkpoint 291 shop comparison. Its three records point to the same Moria
keepers, stock objects, and rooms as active `moria.shp`. The candidate's raw
buy multipliers differ, and its #99037 sell field is 1.00 versus active
Moria's 1.10. `shop.c` clamps each candidate buy value above 0.80 to 0.80 and
keeps the candidate sell field at 1.00 after the 1.00 minimum clamp.
The normal `make_shp` compiler reads `areas/AREA`, which lists `moria` and
omits `moriatst`. However, runtime `open_shop_stream` falls back to globbing
all `areas/shp/*.shp` files if `areas/world.shp` cannot be opened; that path
includes `moriatst.shp`. The duplicate keeper/room records then make
`configured_shopkeeper_for_room` return no unique match. This is a conditional
code path, not evidence about a live deployment or observed gameplay. Retain
the candidate in place pending loader-path and owner review; no migration is
cleared.

## Checkpoint 921 supplement: moriatst typed shop roles and effective factors

The candidate's keeper fields resolve to active Moria mobiles #99035–#99037,
its room fields resolve to Moria rooms #99306/#99312/#99318, and its stock
lists resolve to Moria objects #99063–#99070. Moria's ZON resets place each
keeper in the corresponding room, and [`specs.assign.c`](../../../src/specs/specs.assign.c#L1184)
binds those mobiles to `nw_brock`, `nw_merthol`, and `nw_vitnor`. The
`moriatst` records remain separate shop source records; this typed overlap
does not establish a predecessor or replacement relationship.

The CP921 factor check corrects the earlier summary: candidate #99037 has a
raw sell multiplier of 1.00 while active Moria has 1.10. The loader preserves
the 1.00 candidate sell value after clamping its buy factor to 0.80. The
wildcard fallback and duplicate keeper/room selector ambiguity remain
conditional source-code findings, with no live boot or trade result inferred.
