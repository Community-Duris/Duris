# Brass Old 3 Audit Review

Reviewed October 11, 2026, Checkpoint 456. This is a static source, followed-history, WLD graph, and
typed-dependency review of the unregistered `brass-old-3` component. It does not establish live
loading, player access, quest operation, data ownership, or permission to move files.

## Source scope and identity

The current source family is [WLD](../../../areas/wld/brass-old-3.wld),
[MOB](../../../areas/mob/brass-old-3.mob), [OBJ](../../../areas/obj/brass-old-3.obj),
[ZON](../../../areas/zon/brass-old-3.zon), and [QST](../../../areas/qst/brass-old-3.qst). The ZON
header is #252, titled “Plane of Fire, Brass,” with last VNUM #25370 and reset mode 1. The
checked-in records comprise 170 rooms #25201–#25370, 42 mobiles #25100–#25231, 28 objects
#25101–#25191, and two QST giver keys #25210 and #25224 with two native completion blocks. There is
no same-stem SHP, map sidecar, story sidecar, or prior source dossier.

No `brass-old-3` row appears in the current [full area manifest](../../../areas/AREA) or [QST
manifest](../../../areas/qst/AREA). The latter still lists `plane_fire_two *252-253` at [line
119](../../../areas/qst/AREA#L119); the current same-stem QST path is `brass-old-3.qst`. Thus the
preserved QST content is not registered under its current path, and the old index row does not prove
that this file is loaded.

## Followed history and neighboring Brass sources

At the parent of 2025 commit `0e925fa9`, the full area manifest listed `plane_fire_one *250-251`,
`plane_fire_two *252-253`, and `plane_fire_three *254-255`. The commit retains the former
`plane_fire_two` files under `brass-old-3`; Git records all five WLD/MOB/OBJ/ZON/QST paths as 100%
renames. The current active `plane_fire_one *254-255` is the former third range, while Old 2 is the
former first range. The three equal raw ZON titles do not make them one source.

| Source | Parent `plane_fire_two` blob | Current `brass-old-3` blob |
| --- | --- | --- |
| WLD | `52cb04df833790d61610814d0319c3c377c72aba` | `52cb04df833790d61610814d0319c3c377c72aba` |
| MOB | `a1f76a4631170825ed84b6ef1af4980a507b22e5` | `a1f76a4631170825ed84b6ef1af4980a507b22e5` |
| OBJ | `b8a7ce2287e4603b177c85edbdb6753da096139b` | `b8a7ce2287e4603b177c85edbdb6753da096139b` |
| ZON | `a474784eb38144132608366216d437796c905cf0` | `a474784eb38144132608366216d437796c905cf0` |
| QST | `18a0ce52a793a8b7bae2b41a3940b061f5a8ffe1` | `18a0ce52a793a8b7bae2b41a3940b061f5a8ffe1` |

The old `plane_fire_two` QST path existed at the parent and was renamed with the other four files.
The still-listed `plane_fire_two *252-253` QST manifest row is therefore stale against the current
file path. It is different from Old 2: no parent-era `plane_fire_one.qst` existed there, while the
`plane_fire_two.qst` predecessor did.

The Yodono dialogue in Old 3 QST giver #25224 is also present in active `brass.qst` giver #139124
and archived Brass predecessor giver #139224. These are shared quest text, not evidence that Old 3
is a full-area replacement for active Brass. Active `brass` owns the current story dossier; see the
[active Brass dossier](../zone-stories/PLANE_OF_FIRE_BRASS.md), the [Old 1
review](BRASS_OLD_1_AUDIT_REVIEW.md#relationship-to-active-brass), and [Old 2
review](BRASS_OLD_2_AUDIT_REVIEW.md#followed-history-and-the-qst-index).

## Narrative and quest-story review

The rooms lay out a substantial Efreeti city: the brass gates and Royal Way lead through Imix
Avenue, warrior and magi guilds, the bazaar and merchants’ street, royal library, watchtowers, bank,
inns, zoo, noble houses, Kossuth’s temple, the low quarter, and the bridge and gates of the Sultan’s
palace. A long magma-lake edge forms the city’s northern landmark. The prose gives institutional
color through a tax office, census bureau, guild halls, library, shops, court and religious spaces;
it also repeatedly describes immense buildings and a population of 42,638,943.

The social story is more pointed than a neutral city tour. The low quarter is described as the home
of common Efreeti, fallen nobles, and non-Efreeti residents; separate mobile and room text presents
captured Djinn slaves, revenge, a tortured corpse, and a harem. Kossuth and Imix both appear as
powerful Efreeti gods. The temple source names Latisia (#25104) as supreme cleric, while room #25326
identifies El’Quilan as pontiff and supreme cleric. The source does not explain whether one
succeeded the other or whether they are different offices. The palace approach describes the Sultan
but the reviewed Old 3 rooms stop at the gates.

The two quest blocks need owner review before any story rewrite:

- Giver #25210 is the “dying djinn” mobile. Its Q completion text is empty and its only item command
  gives object #25119, the cold blue-flame vial. It has no authored receive goal.
- Giver #25224 is Yodono. His message says to bring him the heads of Caltoon, Amir, and the
  Arch-Magi, but it first says there are “two important tasks.” In the current [quest
  parser](../../../src/world/quest.c#L2194), `R I` records a receive requirement and `G I` records a
  give/reward item. The source instead requires `R I #55271` plus two `R I #25119` vials, then gives
  `G I #25112`, `#25117`, and `#25118`—the three flaming-head objects. This is opposite the apparent
  dialogue objective. Winterhaven object #55271 is “the broken rays of evening sunset” and its
  keyword includes `_noquest_` ([object source](../../../areas/obj/wh.obj#L3374)).
- The identity count is unclear: Old 3 mobile #25218 is Caltoon, “the arch-magi of all efreeti,”
  while the QST defines separate head objects for Caltoon and “the Arch-Magi.” The room and mobile
  text do not establish whether those are one target or two.

The same Yodono dialogue and head-item reward pattern appear in both current `brass.qst` and
`brass-old-1.qst`; current Brass omits the Old 3/Old 1 `R I #55271` requirement and uses its own
VNUMs. This recurring text suggests shared legacy authoring, but it does not settle which
requirements or quest flow the owner intends. No local SHP exists even though the room and mobile
prose advertises several vendors. The object catalog also contains Kossuth regalia, blue-flame
weapons, the three head tokens, and other fire-themed equipment.

## WLD graph and source-only routes

The Old 3 WLD has 170 rooms and 362 authored directions: 360 local and two cross-zone. Its local
graph is one weak component; authored directed traversal from entry #25201 reaches all 170 rooms.
All rooms fall within the declared #25200–#25399 range. There are no negative targets, fall-chance
fields, or current fields.

| Old 3 source exit | Target | Classification |
| --- | --- | --- |
| #25201 D2 south | Current active Fire #25455 | Source-only Old 3 exit to an active WLD room. Fire #25455 has no return to #25201; its north exit reaches active Brass #139000. This is the unique-source approach already counted in the Plane of Fire review, not active ingress into Old 3. |
| #25341 D0 north | Old 2 #25001, “Entry Way” | Source-only inactive-to-inactive link from the palace gates into Old 2. Old 2 has no reciprocal WLD edge to #25341. It is not an active entrance into either old component. |

No active WLD source targets an Old 3 room. Candidate-specific active inbound remains zero. The Old
3 → Old 2 link newly closes the static map relationship between their source files; it does not
change active inbound counts or the curated inactive-WLD totals. See [World Connectivity, Checkpoint
456](WORLD_CONNECTIVITY.md#checkpoint-456-brass-old-3-connectivity-and-the-old-2-link), the [Old 2
graph review](BRASS_OLD_2_AUDIT_REVIEW.md#wld-graph-and-current-inbound-references), and the [Plane
of Fire review](PLANE_OF_FIRE_AUDIT_REVIEW.md#inactive-wld-approaches-and-candidate-records).

## Typed references, reset closure, and code consumers

Old 3 ZON has 312 M/O/G/E reset commands: 278 M, 31 E, two G, and one O. Of those, 279 are local
commands (246 M plus all E/G). The nonlocal or unmatched references are:

| Source command | Count | Typed destination |
| --- | ---: | --- |
| M #25000 | 30 | Old 2 mobile #25000 (“efreeti guard gate”); lines 173–196 and 365–374. |
| M #25400 | 2 | Current active `plane_fire_one` mobile #25400; both at room #25201. |
| O #2097 | 1 | No current object prototype was found; line 169 targets room #25296, and the comment says the type is not in this OBJ file. |

The reverse reset dependency is also present: Old 2 has 66 ZON commands for Old 3 prototypes (57 M
and nine E/G). Taken together, Old 2 and Old 3 have 96 cross-candidate reset commands in both
directions, in addition to Old 3’s one-way WLD edge into Old 2. These are source-only reset records
because neither old component is in the current full area manifest.

Three active ZON files contain typed imports from Old 3’s object catalog: Bahamut O #25106 at [line
166](../../../areas/zon/bahamut.zon#L166), Celestia G #25103 at [line
374](../../../areas/zon/celestia.zon#L374), and Tiamat O #25106 at [line
92](../../../areas/zon/tiamat.zon#L92). #25106 is the Old 3 level-52 glowing potion; #25103 is the
staff of blue flames. Each prototype is defined only by this unregistered Old 3 OBJ in the current
source scan. `renum_zone_table` converts the missing typed VNUM to `-1` and disables O/G reset
commands ([loader source](../../../src/world/db.c#L1612)); Tiamat’s exact #25106 behavior is
separately documented in the [Tiamat review](TIAMAT_AUDIT_REVIEW.md). These active source records
are not current potion or staff stock.

Old 3’s QST also contains a source-only Winterhaven item requirement at [line
25](../../../areas/qst/brass-old-3.qst#L25): `R I 55271`. It is absent from active Brass’s current
corresponding Yodono block, and Old 1 preserves a similar legacy reference. No SHP file or external
shop product references were found for Old 3 objects.

The source code contains additional exact VNUM references:

| Code reference | Source identity | Static finding |
| --- | --- | --- |
| [`specs.assign.c:1055–1056`](../../../src/specs/specs.assign.c#L1055) | Mobiles #25101 and #25104 | Both are assigned `guild_guard` in the Plane of Fire block. |
| [`specs.assign.c:1964`](../../../src/specs/specs.assign.c#L1964) | Object #25103, staff of blue flames | Bound to `staff_of_blue_flames`. |
| [`specs.guards.c:232`](../../../src/specs/specs.guards.c#L232) and [`:441`](../../../src/specs/specs.guards.c#L441) | Room #25201 | Hardcoded guard direction checks include the Old 3 entry room. |
| [`randomeq.c:126`](../../../src/item/randomeq.c#L126) | Mobile #25110, Padashaw | It remains in `highdrop_mobs`, but the only lookup loop for that array is commented out at lines 650–660; it is not an active consumer. |

These are source-level references, not evidence that an Old 3 prototype is loaded or that a runtime
fault occurs. Preserve them for code-owner review; do not use numeric matches alone to infer a safe
source move.

## Disposition

Keep `brass-old-3` **RETAIN IN PLACE**. The source is a byte-preserved former `plane_fire_two`; it
has a source-only route to active Fire, a one-way source-only WLD link and 30 mobile resets into Old
2, 66 reverse Old 2 reset imports, three active ZON references to its unregistered objects, a legacy
Yodono QST with contradictory receive/reward data, an orphaned QST manifest row, and hardcoded C/C++
references. Resolve builder intent, QST index ownership, route behavior, typed reset closure, code
ownership, editor/tooling, and persisted identity before considering any path decision. No area,
code, QST manifest, or gameplay source was changed by this review.


