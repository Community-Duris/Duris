# fooquest - FooQuest source audit

Canonical key: component:fooquest. Physical coverage row 393 / CSV line
394; candidate dependency row 43 / CSV line 43. Reviewed October 11, 2026,
Checkpoint 480, against source snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697.
This is a static source, connectivity, identity, history, and typed-reference
review. It does not establish QST loading, the behavior of compiled specials,
or permission to activate or move these files.

## Source scope, title, and story

The canonical component has [WLD](../../../areas/wld/fooquest.wld#L1),
[MOB](../../../areas/mob/fooquest.mob#L1),
[OBJ](../../../areas/obj/fooquest.obj#L1),
[ZON](../../../areas/zon/fooquest.zon#L1), and
[QST](../../../areas/qst/fooquest.qst#L1) files. An additional same-stem
[areas/wld/fooquest.mob](../../../areas/wld/fooquest.mob#L1) contains another
mobile set outside the conventional MOB directory. The component is absent
from full [areas/AREA](../../../areas/AREA) and
[areas/qst/AREA](../../../areas/qst/AREA); its raw AREA annotation is blank.
There is no same-stem story sidecar, editor-map file, or SHP file. The exact
ZON title FooQuest occurs once among tracked ZON titles.

The QST contains a specific multi-area quest, not just generic test prose.
Warthehr Ngolahn asks players to recover eight sword fragments and the hilt,
have Telrar reforge the weapon, kill Ghalasax, daughter of Aghalhassax, and
return her skin. Later contracts distribute gemstones for class-specific
rewards. The same QST includes an Abal/Lyle scepter thread. It has 13 giver
records and 31 Q/QA contract directives: 24 QA and seven Q. The story names
prisons, forests, sewers, valleys, islands, and cities outside this WLD, but
the candidate map has no external WLD exits.

The ZON header is #636, top VNUM #65050, reset mode 2. All 17 candidate room
VNUMs (#65000-#65015 and #65050) lie outside the conventional #63600-#63699
range; all 30 authored WLD exits originate in those out-of-range rooms. This
records the source fields without declaring them incorrect. The WLD, MOB,
and OBJ files reuse numeric spans across different record types; preserve
those as typed identities rather than treating a room, mobile, and object
with the same VNUM as one record.

## Candidate WLD graph and active-table ownership

The WLD contains sixteen ship rooms plus the separate Newbie Quest Mob Room
#65050. Its 30 authored directions are:

| Room | Authored directions |
| ---: | --- |
| #65000 | south to #65001 |
| #65001 | north to #65000; east to #65003; west to #65002 |
| #65002 | east to #65001; west to #65012 |
| #65003 | east to #65004; west to #65001 |
| #65004 | east to #65005; west to #65003 |
| #65005 | north to #65006; east to #65007; west to #65004 |
| #65006 | south to #65005 |
| #65007 | east to #65008; west to #65005 |
| #65008 | east to #65009; west to #65007 |
| #65009 | north to #65010; east to #65011; west to #65008 |
| #65010 | south to #65009 |
| #65011 | west to #65009 |
| #65012 | east to #65002; west to #65013 |
| #65013 | north to #65014; east to #65012; west to #65015 |
| #65014 | south to #65013 |
| #65015 | east to #65013 |
| #65050 | no authored exits |

All 30 directions stay within the candidate's WLD VNUM set. The sixteen ship
rooms form one weak component, and all sixteen are directed-reachable from
#65000. Room #65050 is a separate one-room component with no WLD exit. There
are no fall-chance or current records.

No other current WLD source defines these 17 same-type room VNUMs, and the
active-WLD scan found zero active exits targeting them. Because the candidate
is not in full areas/AREA, none of its 30 targets resolve to an active WLD
record; they are source-local targets whose definitions live in this
candidate. There are no external WLD target references. Curated
inactive-WLD totals remain 11,987 overall / 415 from Surf.

## Mobile files, resets, objects, and QST dependencies

The conventional [MOB file](../../../areas/mob/fooquest.mob) defines 37
prototypes #65000-#65036, from ship pirates through Warthehr's contacts,
hunters, Ghalasax, the rabid chipmunk, and a feverish healer. A separate
[mobile file in the WLD directory](../../../areas/wld/fooquest.mob) defines
12 pirates #63600-#63611. Each of those 12 records is textually identical
to the corresponding first twelve #65000-#65011 MOB record after substituting
the VNUM offset of 1,400. The ZON uses the #65000-series identities; no
#63600-series M or F command appears in it. This exact normalized duplication
needs an owner explanation, but does not by itself prove which file is a
predecessor or replacement.

The OBJ file defines 46 prototypes across #65000-#65045 and #65050. They
include keys and chests, the eight sword fragments, quest letters, the hilt,
scepter, gemstones, class rewards, and the dragonslayer sword. OBJ #65004 is
absent: the 2016 unnamed-object cleanup deleted its former eight-line
prototype. The 2016 commit also changed the long descriptions of #65036 and
#65038. The same numeric IDs used for rooms and mobiles remain separate OBJ
identities.

The ZON has 37 M, 72 F, 330 G, and two E commands. All 13 QST giver VNUMs
are among the M resets into the disconnected #65050 room. The candidate MOB
prototype #65036, a feverish healer, has no M reset in this ZON. There are
320 consecutive G commands for object #65041, a foam-flecked chipmunk tooth,
following the M reset for the rabid chipmunk #65035; the other ten G commands
reset individual quest objects. Preserve these source counts and placements
for the zone owner to explain.

The Abal QST record consumes candidate object #65035 and external object
#26201 before its later scepter exchange
([QST reference](../../../areas/qst/fooquest.qst#L371)). Object #26201 is the
shimmering key in active [Moonshae OBJ](../../../areas/obj/moonshae.obj#L9);
Moonshae is listed in [areas/AREA](../../../areas/AREA#L132). Preserve this
typed cross-area dependency. The other QST G/R item VNUMs name the candidate's
own quest objects, including fragments, letters, the dragon skin, gemstones,
rewards, and sword.

## Compiled special bindings and ownership

Compiled [assignment code](../../../src/specs/specs.assign.c#L478) binds
mobile #65012 to fooquest_mob and #65013 to fooquest_boss. It also binds
13 candidate QST giver mobile VNUMs #65015, #65016, #65018, #65019,
#65022, #65024, #65026, #65028-#65030, and #65032-#65034 to newbie_quest.
Object #65050 is bound to dragonslayer
([assignment](../../../src/specs/specs.assign.c#L1545)). The same-stem
[procedure source](../../../src/specs/specs.fooquest.c#L16) references mobile
#65014 and loads #65013; the trusted transform branch on #65012 also loads
#65013 and gives object #21 or #22. In current listed Heavens OBJ those two
object IDs are Mayhem and Symmetry swords
([#21](../../../areas/obj/heavens.obj#L180),
[#22](../../../areas/obj/heavens.obj#L192)); other unlisted OBJ files also
contain those numbers. The #65050 dragonslayer procedure source includes its
own level and dragon-target checks. These are exact source-level code
dependencies, not confirmation that the unlisted FooQuest prototypes are
loaded or that any procedure currently runs.

## History and disposition

WLD, the extra WLD-directory MOB, ZON, and QST histories reach the 2009
area-tree move and a 2013 mode-only commit. The conventional MOB history
reaches the 2009 move. OBJ history reaches the 2009 move and 2016 object
cleanup and description edits. The compiled procedure source has later 2026
API/refactor commits. No alternate-stem predecessor or explicit
whole-component replacement appears in these bounded paths.

Keep candidate row 43 **RETAIN IN PLACE**. Resolve the header #636 versus
#65000-range fields, the duplicate #63600 and #65000 pirate sets, the isolated
quest room, all 13 QST givers and 31 contracts, the Moonshae #26201 key
dependency, the deleted #65004 object, the 320 tooth resets, compiled special
owners and their #21/#22 item identities, loader/QST manifests, story/editor
ownership, deployment, and persisted VNUM closure before any activation,
move, rename, archive, or merge proposal. See the
[24 builder questions](FOOQUEST_BUILDER_QUESTIONS.md),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-480-fooquest-ship-map-and-quest-room),
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-480-fooquest-title-history-and-prototype-sets),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-480-fooquest-retained-pending-quest-and-code-review),
[candidate row 43](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L43), and
[coverage row 393](ZONE_COVERAGE_REGISTER.csv#L394). No area/source files,
tests, builds, runtime, server, database, or containers were changed or used.

## Checkpoint 523: shared QST template with Raxquest and Black Pearl

A direct 462-line comparison adds a QST-specific relationship to the FooQuest
review: fooquest.qst, raxquest.qst, and blackpearl.qst become line-for-line
identical after normalizing their local 650xx, 705xx, and 142xxx VNUM tokens.
362 lines match before normalization. Black Pearl has a full-area WLD entry,
but its per-stem QST is not named in qst/AREA; this checkout also lacks the
areas/world.qst aggregate that src/world/quest.c names and the loader does not
glob per-stem files. Treat this as exact QST text reuse and an owner lineage
question, not proof that the Black Pearl area replaces either source-only
component or that any per-stem QST loads. FooQuest map, code, and source
identity remain separately unresolved. See the [Raxquest audit](RAXQUEST_AUDIT_REVIEW.md#story-and-exact-qst-mirror) and
[20 Raxquest builder questions](RAXQUEST_BUILDER_QUESTIONS.md).


## Checkpoint 892: role-aware external closure and room-target scan

This continuation classifies FooQuest's room IDs #65000–#65015 and #65050, mobile IDs #65000–#65036, and object IDs #65000–#65045 and #65050 separately. The existing WLD result is unchanged: sixteen connected ship rooms, isolated #65050, 30 local directions, and zero active inbound edges. None of the outside same-type WLD/MOB/OBJ files defines a matching candidate identity. The prior #63600–#63611 pirate duplicate set remains a separate mobile identity and lineage question.

A role-aware scan of the 441 other ZON files found no exact room, mobile-prototype, or object-prototype operand in FooQuest's typed ranges. The scan classifies M/R/F mobile and room operands, O object and room operands, G/E item operands, P item/container operands, D door-room operands, and C/Y room operands according to the loader's [zone renumbering](../../../src/world/db.c#L1612). No active or source-only reset therefore establishes a FooQuest room arrival or consumes one of its mobile/object prototypes.

The 264 other QST files contain no quester header in the candidate mobile range and no `G I`/`R I` item operand in the candidate object range. All 232 story sidecars parse without errors; none has a typed `mob_vnum`, `giver_vnum`, or `item_vnums` match, or an `I:` completion-key item token for these identities. No sidecar `room_vnum` or `room_vnums` field occurs in this corpus. This leaves FooQuest's own #26201 Moonshae key dependency unchanged ([Abal QST](../../../areas/qst/fooquest.qst#L371)); no external quest or story consumer was found.

Across all 442 OBJ files, the static record scan covers 21,683 object records and recognizes 872 type-25 `ITEM_TELEPORT` and 380 type-29 `ITEM_SWITCH` prototypes. No outside type-25 value[0] room target or type-29 value[1] blocked-exit room matches FooQuest's room set. FooQuest OBJ itself defines neither type, so this source has no object-encoded room destination in either role ([type definitions](../../../src/core/defines.h#L140), [switch room field](../../../src/specs/specs.object.c#L302)). No WLD edge or already-counted approach is added.

FooQuest has no SHP file. In the 180 shop shards, 712 `N`-format records yielded no keeper-mobile, shop-room, or product-object field in the candidate's corresponding typed range. The static shop input remains conditional: `areas/world.shp` is absent, and the non-mini [fallback](../../../src/economy/shop.c#L3267) concatenates shard files, including eight legacy `O`-format records in [Luiren](../../../areas/shp/luiren.shp#L1) that the current parser rejects at its [format gate](../../../src/economy/shop.c#L3360). This scan identifies source fields; it does not establish a successfully loaded shop catalog.

The exact-number C/C++ scan adds no compiled room consumer for #65000–#65015 or #65050. Relevant matches remain the compiled #65012 `fooquest_mob` and #65013 `fooquest_boss` assignments, the 13 `newbie_quest` giver bindings, and object #65050's `dragonslayer` assignment ([assignments](../../../src/specs/specs.assign.c#L479), [giver bindings](../../../src/specs/specs.assign.c#L1229), [dragonslayer binding](../../../src/specs/specs.assign.c#L1545)). `specs.fooquest.c` checks and reads mobile #65014 and reads #65013 ([procedure](../../../src/specs/specs.fooquest.c#L38)); its existing #21/#22 transform outputs remain external object dependencies as documented above. The unrelated exact number #65000 in `player_quarantine_recovery.c` is a recovery-receipt type constant, not a FooQuest VNUM. These assignments and procedure references still do not establish that the absent world/QST aggregates load FooQuest or that the callbacks run. The Checkpoint 523 exact QST-text reuse with Raxquest and Black Pearl is unchanged and does not identify a replacement area.

Keep candidate row 43 **RETAIN IN PLACE**. Checkpoint 892 adds no external reset, quest, sidecar, shop, portal, switch, or compiled room consumer and changes no graph count. Preserve the local 13-giver/31-contract quest, external Moonshae key #26201, duplicate #63600 mobile file, isolated room #65050, 320 tooth resets, compiled specials, and the unresolved #21/#22 identities. Owner, manifest, editor/tooling, deployment, story, and persisted-ID questions remain open; the 24 builder questions remain unchanged. See the [builder questions](FOOQUEST_BUILDER_QUESTIONS.md), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-892-fooquest-role-aware-boundaries), [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-892-fooquest-retains-quest-and-map-identities), [candidate row 43](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L43), and [coverage row 393](ZONE_COVERAGE_REGISTER.csv#L394). No gameplay/area source, tests, builds, runtime, server, database, migration, or container was changed or used.
