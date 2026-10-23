# Checkpoint 192 — Avernus source audit

This supplement reconciles the active `avernus` contributor at zone #320. The
[story dossier](../zone-stories/AVERNUS.md) reviews its authored environments,
mobile and object text, and builder questions. The review is source-backed and
static; it does not claim a live game journey or reset observation.

## Source scope and registration

The un-commented `areas/AREA` entry is `avernus` (`*320-323 (Mabort)`); it is
also listed in `areas/qst/AREA`. Its zone header is #320, title “The Plane of
Avernus,” top #32399, and reset mode 0
([AREA](../../../areas/AREA#L156), [QST AREA](../../../areas/qst/AREA#L145),
[zone header](../../../areas/zon/avernus.zon#L1)). The source set contains 392
rooms (#32000–#32399), 25 mobile prototypes (#32001–#32025), and 33 objects
(#32000–#32032). `areas/qst/avernus.qst` is absent; the active registry records
zero native Q/QA contracts and no story sidecar. The sole shop record is
keeper #32008 at room #32279 and stocks objects #32019–#32021
([shop source](../../../areas/shp/avernus.shp#L1)). `areas/RANDOM_AREA` is
empty and does not list Avernus.

## WLD census and static directed reach

The active loader scan parsed 351 `.wld` files and 253,262 active room VNUMs.
Avernus has 1,437 authored exits: 1,436 local and one cross-area. All 1,437
destinations resolve to active rooms. Its weak local components contain 375,
16, and 1 room:

- Main component: #32001–#32399, except source VNUMs absent from the map. From
  #32031, directed local traversal reaches 369 of its 375 rooms; all 375 have
  a directed local path back to #32031.
- Six source-notice rooms (#32001, #32002, #32003, #32004, #32006, and #32007)
  cannot be reached by following local exits from #32031. Their titles and
  notices label them “random mob load” rooms and tell visitors to contact a
  god ([examples](../../../areas/wld/avernus.wld#L6)); their presence is not
  evidence of an ordinary player route.
- The 16-room component is #32008–#32023; its rooms are titled “Unnamed” and
  have sparse descriptions. Room #32000 is isolated and also “Unnamed.”

The only active cross-area WLD exit is #32031 north to Tiamat #19600. Tiamat
#19600 has the reciprocal south exit. The active WLD registers record one
inbound, one outbound, and one reverse edge. The all-95-file inactive-WLD scan
found no source-room overlap with Avernus and no inactive exit targeting an
Avernus room. Avernus has no WLD exits with nonzero door/key fields, no ZON `D`
door resets, and no fall-chance records. Seven River-of-Blood rooms have a
current record of speed 50, direction 0: #32110, #32130, #32150, #32170,
#32190, #32210, and #32230 ([first current](../../../areas/wld/avernus.wld#L2383)).
Those currents are a conditional movement effect and are not simulated by the
static path count.

## Reset commands and source behavior

The zone file has 275 executable reset commands: 202 `M`, 23 `F`, 44 `E`, five
`G`, and one `O`; it has no `D` door reset. The `F` command loads a follower
mobile prototype and attaches it to the last `M`-loaded mobile. Its first VNUM
is still an exact mobile-prototype reference
([F reset implementation](../../../src/world/db.c#L7747)).

External mobile prototypes imported by the Avernus ZON are owned by active
areas:

| Owner | Referenced mobile VNUMs | Reset actions |
| --- | --- | --- |
| `river_styx` | #90701–#90707, #90709–#90712 | 72 `M`/`F` rows combined |
| `hell_one_avernus` | #51002, #51003 | 10 `M`/`F` rows combined |

All other `M`/`F` and object reset prototypes resolve to Avernus-owned records.
The full active-prototype scan found no duplicate external definition of an
Avernus mobile or object VNUM, and no external object prototype imported by
Avernus resets. Every nonlocal reset reference resolves to an active prototype.

The boot loop schedules periodic `event_reset_zone` events only for zones with
nonzero `reset_mode`; during an ordinary boot it still calls `reset_zone(j, 2)`
for every zone. Thus Avernus's reset commands are a boot-time source population,
while source code does not establish periodic repop for this mode-0 zone. The
event handler contains a separate DB-backed `no_reset_zone_reset` branch if a
mode-0 zone event is fired; it checks `zones.reset_perc` and epic completion.
The source snapshot does not establish that a periodic event is scheduled for
Avernus or what the live DB would do
([boot loop](../../../src/world/new_events.c#L2068),
[reset event](../../../src/world/events.c#L428),
[DB reset path](../../../src/world/db.c#L6846)).

## Typed travel and world connectivity

Three different source mechanisms connect Avernus to other areas:

| Mechanism | Exact source | Source destination | Source evidence |
| --- | --- | --- | --- |
| Authored WLD pair | Avernus #32031 north; Tiamat #19600 south | Reciprocal WLD travel | [Avernus room](../../../areas/wld/avernus.wld#L421), Tiamat boundary in [WORLD CONNECTIVITY](WORLD_CONNECTIVITY.md#tiamat-avernus-and-astral-tiamat-boundaries) |
| Bronze Citadel teleport pair | Avernus object #32013, reset at #32057; Bctdl object #32436, reset at #32420 | #32013 → Bctdl #32420; #32436 → Avernus #32057 | [Avernus item](../../../areas/obj/avernus.obj#L157), [Avernus reset](../../../areas/zon/avernus.zon#L9), [Bctdl item](../../../areas/obj/bctdl.obj#L179), [Bctdl reset](../../../areas/zon/bctdl.zon#L137) |
| Ravenloft teleport items | Objects #59078, #59080, #59084, #59086, #59088, #59090, all reset at #59169 | #32063, #32029, #32057, #32290, #32279, and #32090 respectively | [object prototypes](../../../areas/obj/ravenloft2.obj#L1191), [reset group](../../../areas/zon/ravenloft2.zon#L913) |

These objects are type `ITEM_TELEPORT` (25). Their data sets command 7
(`CMD_ENTER`), a direct destination VNUM, and charge value -1. The command
dispatcher calls `check_item_teleport`; that handler requires the matching
command and compatible arena flags, resolves the room, and teleports the user.
This confirms a compiled route mechanism and source-declared reset placements.
It does not prove that each reset has run, that the source room is reachable,
or that arrival and return are safe
([type constant](../../../src/core/defines.h#L140),
[command dispatch](../../../src/cmd/interp.c#L2410),
[teleport handler](../../../src/magic/spell_travel.c#L944)).

The compiled `gate` spell is another conditional route. Its plane table uses
#32385 as the Hell anchor; for a Hell destination it samples a room VNUM from
#32101–#32399, then rejects invalid, no-magic/no-gate, or hometown endpoints
before creating a temporary portal ([plane table and gate](../../../src/magic/spells.c#L294)).
That path can choose an Avernus room but is not a guaranteed entry, landing, or
return route.

## Typed cross-area references

| Source | Typed reference into or out of Avernus | Context |
| --- | --- | --- |
| `bctdl` | QST action on object #32029; local teleport item #32436 targets #32057 | Zariel exchange in `bctdl.qst`; paired with Avernus #32013 to #32420 |
| `heavens` | Four `G I #32011` actions | Joukarth's reagent story; #32011 is the basalt scimitar |
| `jade` | Two `R I #32019` actions | A QST exchange; #32019 is a vial of purple liquid |
| `trakkia` | One `R I #32019` action | Saint Gauce reward branch; same vial |
| `wh` | `G I #32031` action | Adryv's delivery request; the story sidecar marks it as a carried delivery item |
| `tiamat` | Three `G` reset rows load object #32019 | Cross-area reset stock in `tiamat.zon` lines 156–158 |
| Avernus callbacks | Staff #32008 reads mobile #92076 and, in its exhausted-use branch, #200 | #92076 is owned by `undermountain`; #200 is owned by `heavens` |

The exact QST and story references are `bctdl.qst:35` with
`bctdl.story.json:313,335`; `heavens.qst:52–55`; `jade.qst:271–272` with
`jade.story.json:1423,1453`; `trakkia.qst:30` with
`trakkia.story.json:125,147`; and `wh.qst:3606` with
`wh.story.json:1067`. There are no external SHP product rows for Avernus-owned
objects. The Armaros shop is local Avernus stock.

The two callback bindings are `sinister_tactics_staff` on object #32001 and
`staff_shadow_summoning` on object #32008. The latter reads mobile #92076 from
`undermountain` and #200 from `heavens` as an aggressive fallback. This is an explicit compiled
prototype dependency; it must not be confused with a ZON reset or a local mob
definition ([assignments](../../../src/specs/specs.assign.c#L1441),
[procedure](../../../src/specs/specs.avernus.c#L56)).

## Title, history, and migration disposition

The active normalized `.zon` title “The Plane of Avernus” is unique among the
351 active contributors. The same normalized title appears on the inactive,
unregistered `plane_avernus` component, whose zone #135 contains only rooms
#135 and #136 and has no mobile or object records. The all-inactive-WLD scan
found no VNUM overlap or approach into active Avernus. The repeated title is a
name collision, not evidence that `plane_avernus` replaced `avernus`.

Followed `avernus` WLD/MOB/OBJ/ZON paths remain on the same stem across the
2009 move into the `areas/` tree, later 2010 and 2013 edits, and the 2015
quest-item flag pass. The object history includes same-stem changes; the shop
history includes the 2014 “Cleaned up all the shops so that they load
correctly” pass. Followed `plane_avernus` history reaches its 2009 path move
and 2011 tracking addition. The bounded paths establish no alternate-stem
rename, distinct predecessor, or whole-zone replacement. See
[DUPLICATE_AND_REPLACED_ZONES.md](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-192-avernus-title-and-path-history).

Keep active `avernus` **DO NOT MOVE** as a current full-world contributor. Do
not add an attic candidate for it. Keep existing `plane_avernus` candidate row
[83](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L83) **RETAIN IN PLACE** while
Checkpoint 520’s [plane_avernus audit](PLANE_AVERNUS_AUDIT_REVIEW.md)
completes the current source-tree typed-reference scan for the candidate and
still recommends RETAIN IN PLACE because of the exact title collision and
two additional inactive WLD definitions of #135–#136.
Editor/import/deployed and persisted-identity closure remains unverified;
the title and lack of an active WLD approach do not grant move clearance.

## Builder questions

No source-backed builder resolution was found for the questions in the
[story dossier](../zone-stories/AVERNUS.md#builder-questions-and-source-resolutions).
Confirm the Peaceable Lands' rescue claim, the blank/diagnostic rooms, the
Great Cavern Azharul/Dis wording, the intended external object exchanges and
portal arrivals, and the two staff callbacks with the relevant content owners.
Until then, this audit records the source behavior and preserves each area and
prototype owner separately.
