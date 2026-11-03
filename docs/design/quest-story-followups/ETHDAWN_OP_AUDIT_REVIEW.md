# ethdawn_op — Etherial Dawn Outpost source audit

Canonical key: component:ethdawn_op. Physical coverage row 391 / CSV line
392; candidate dependency row 41 / CSV line 41. Reviewed October 11, 2026,
Checkpoint 478. This is a static source, connectivity, identity, history, and
typed-reference review. It does not establish runtime portal behavior, a
canonical map owner, or permission to activate or move these files.

## Source scope, title, and story

The candidate comprises [WLD](../../../areas/wld/ethdawn_op.wld#L1),
[MOB](../../../areas/mob/ethdawn_op.mob#L1),
[OBJ](../../../areas/obj/ethdawn_op.obj#L1), and
[ZON](../../../areas/zon/ethdawn_op.zon#L1). It is absent from full
[areas/AREA](../../../areas/AREA#L141) and
[areas/qst/AREA](../../../areas/qst/AREA); there is no same-stem QST, SHP,
story sidecar, or editor-map file. Its raw AREA annotation is blank. The
ZON title is Etherial Dawn Outpost, the only exact match among tracked ZON
titles. Preserve the source spelling; do not silently normalize Etherial to
Ethereal. The ZON header is #1 with top VNUM #29312; each WLD room record
also stores zone value 1, although the room VNUMs lie outside header #1’s
numeric range (100–199).

The WLD prose places a burrow below a mighty oak in Mistywood, then describes
an artificial cavern, a green portal, and a druidic inscription naming Jheral.
The MOB file supplies a petrified treant, grey elf, and cloaked statue. This
is story-bearing source text, but no QST or story sidecar establishes a
canonical sequence or relationship to the other maps using these VNUMs.

## Candidate WLD graph and active-table ownership

The WLD defines rooms #29310–#29312 and four directions: #29310 north to
#29311 and up to #156745; #29311 south to #29310 and west to #29312; #29312
east to #29311. The three local directions form one weak component, and all
three rooms are directed-reachable from #29310. The up target #156745 is the
inactive Duris3 room Rolling Hills
([target](../../../areas/wld/Duris3.wld#L1191646)); it has no exit back to
#29310–#29312. All three room VNUMs are outside the numeric range for zone
header #1 (100–199), so all four authored directions originate in
range-mismatch rooms. This records the source fields without declaring them
incorrect. There are no fall-chance or current records.

Each candidate room number also occurs in active [Newbie WLD](../../../areas/wld/newbie.wld#L2129)
and in inactive loc_gh and eskull_hall WLD. All nine complete same-number
comparisons differ. Candidate #29310 is a burrow under Mistywood; the other
records call it a Grotto, Arvandor Outpost, and an enormous stone building.
Candidate #29311 is its cavern, while the three other records are a home, a
Vallenwood tree interior, and a dark passage. Candidate #29312 contains the
green portal and Jheral’s inscription; the other records are Life Crossroads,
the Inn of Last Hope, and the Room of Nothingness. Newbie’s
[AREA entry](../../../areas/AREA#L141) owns the active zone range *292–293,
so the three local candidate targets currently resolve to Newbie room records.

A full active-WLD scan found six raw Newbie directions to these numbers:

| Active Newbie source | Direction | Raw target | Active Newbie room |
| ---: | --- | ---: | --- |
| #29228 | south | #29310 | A Grotto |
| #29295 | north | #29311 | Inside A Small Home |
| #29313 | west | #29312 | Life Crossroads |
| #29314 | east | #29312 | Life Crossroads |
| #29315 | south | #29312 | Life Crossroads |
| #29316 | north | #29312 | Life Crossroads |

These are Newbie-to-Newbie directions. Candidate-identity active inbound
remains zero; do not treat the six raw number matches as routes into the
Mistywood outpost. Curated inactive-WLD totals remain 11,987 overall and 415
from Surf.

## Mobile and object identities, portals, and typed references

The candidate defines three mobiles: petrified treant #29310, grey elf #29311,
and cloaked statue #29312. No other current MOB file defines those mobile
VNUMs. Its ZON resets two treants, one grey elf, and one cloaked statue into
candidate room #29310. These mobile VNUMs must remain distinct from same-number
room or OBJ records; no same-type active mobile collision was found.

The two candidate objects are type-25 portals. #29310 is a green portal whose
stored destination is #29307; the ZON places it in #29312. #29311 is described
as a portal to the Goodie continent and stores #29312, but ethdawn_op.zon does
not reset it. Both require command 7 (CMD_ENTER) and store charge value -1.
Current source code treats value[0] as a room VNUM, value[1] as the required
command, requires a nonzero value[2], and decrements only positive charges
([handling](../../../src/magic/spell_travel.c#L991),
[CMD_ENTER](../../../src/cmd/interp.h#L42),
[charge handling](../../../src/magic/spell_travel.c#L1173)). The title and
stored numbers do not resolve who owns either destination: #29307 is outside
this WLD and is also an Ebon Skull room number; #29312 is both this
candidate’s cavern and active Newbie’s Life Crossroads.

Active Newbie defines different OBJ records at #29310 and #29311: a hefty
bag and arrows ([Newbie objects](../../../areas/obj/newbie.obj#L1521)).
Newbie also owns object #29312, a quiver
([Newbie quiver](../../../areas/obj/newbie.obj#L1546)). Its QST/story item
fields explicitly reward #29310, #29311, and #29312
([QST rewards](../../../areas/qst/newbie.qst#L539),
[story rewards](../../../areas/story/newbie.story.json#L718)). Do not treat
those active item references as uses of the candidate portals. The separate
source-only [Ravenmis reset](../../../areas/zon/ravenmis.zon#L13) imports
object #29311 into room #29307; Ravenmis is also unlisted, so preserve this
cross-source reference without assigning it a current active owner.

## Resets, Player Castles imports, and history

ethdawn_op.zon has zone header #1 and reset mode 2. It imports Player Castles
objects #11054 into #29310 and #11104 into #29311; it also resets candidate
portal #29310 into #29312. The two imports are preserved in the Player
Castles audit; they are not locally defined objects
([reset file](../../../areas/zon/ethdawn_op.zon#L9),
[prior import review](PLAYER_CASTLES_AUDIT_REVIEW.md#typed-dependencies-and-external-consumers)).
The object #29311 portal is absent from this ZON’s O resets but has the
separate Ravenmis reset noted above. Do not infer that the same-number
Newbie items satisfy either source-only portal reference.

The followed WLD history reaches the 2009 area-tree move and a 2011 one-word
spelling correction in the Mistywood description. MOB, OBJ, and ZON histories
reach the 2009 move only. No alternate-stem predecessor or explicit
whole-component replacement appears in these paths. This bounded history does
not identify the outpost owner, the meaning of zone header #1, or the intended
relationship to Newbie, loc_gh, eskull_hall, or Ravenmis.

Keep candidate row 41 **RETAIN IN PLACE**. Resolve the Rolling Hills
boundary, all three Newbie/loc_gh/eskull_hall room identities, both portal
destinations and charge settings, Newbie’s same-number item consumers, the
Ravenmis #29311 reset, the Player Castles imports, the source and editor
owners, deployment, and persisted-VNUM closure before any activation, move,
rename, archive, or merge proposal. See the [24 builder questions](ETHDAWN_OP_BUILDER_QUESTIONS.md),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-478-ethdawn_op-mistywood-outpost-map-and-portal-targets),
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-478-ethdawn_op-identity-and-bounded-history),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-478-ethdawn_op-retained-pending-portal-and-owner-review),
[candidate row 41](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L41), and
[coverage row 391](ZONE_COVERAGE_REGISTER.csv#L392). No area/source files,
tests, builds, runtime, server, database, or containers were changed or used.

## Checkpoint 890: role-aware portal and reset closure

This supplement distinguishes ethdawn_op's room, mobile, and object roles despite the shared #29310-#29312 numbers. The zone fields are classified under the loader's `renum_zone_table()` contract ([role mapping](../../../src/world/db.c#L1612)).

The WLD result remains one three-room component with four directions and zero candidate-identity inbound edges. Six active Newbie WLD targets to #29310-#29312 remain Newbie-to-Newbie. The cross-source ZON scan found sixteen external room-role operands: eskull_hall 6, loc_gh 7, and active Newbie 3. Those include door-state fields as well as reset locations and belong to the source maps that authored them. No external ZON mobile-prototype operand referenced ethdawn_op's #29310-#29312 mobiles. One external object-prototype operand is present: source-only `ravenmis.zon` resets object #29311 into #29307. EThdawn_op defines #29311 as its Goodie-continent portal; Ravenmis does not define that object in its own OBJ file, while active Newbie owns #29311 as arrows. Preserve the exact source reference without assigning it to either current active object identity ([Ravenmis reset](../../../areas/zon/ravenmis.zon#L13), [Newbie arrows](../../../areas/obj/newbie.obj#L1533)).

One external OBJ type-25 target enters the same numeric room band: source-only eskull_hall portal #29300 stores #29312. That number is Ebon Skull's Room of Nothingness, ethdawn_op's cavern, and active Newbie's Life Crossroads. The candidate's own unreset #29311 portal also stores #29312; these source facts do not select a destination owner or add a WLD edge. No external type-29 target was found ([Ebon Skull portal](../../../areas/obj/eskull_hall.obj#L7), [green portal](../../../areas/obj/ethdawn_op.obj#L7) and [Goodie portal](../../../areas/obj/ethdawn_op.obj#L15)).

Newbie QST and sidecar numeric references in this band are item roles: QST requires #29310, #29311, and #29312; its story sidecar has `I:` item completion keys for the same values. Those identify Newbie's hefty bag, arrows, and quiver, not the outpost's same-number mobiles, rooms, or portals ([QST](../../../areas/qst/newbie.qst#L539), [story sidecar](../../../areas/story/newbie.story.json#L718)). No matching direct C/C++ numeric consumer or parser-valid N-format SHP keeper, room, or product field was found. The Newbie ZON door-state and mobile-room fields at #29310-#29311 likewise remain Newbie local ([reset examples](../../../areas/zon/newbie.zon#L20)).

Keep candidate row 41 **RETAIN IN PLACE**. Preserve the header #1/range mismatch, one-way Duris3 #156745 edge, Player Castles imports #11054/#11104, candidate portal #29310 -> #29307, the unreset #29311 -> #29312 portal, Ravenmis's external #29311 reset, all distinct Newbie/loc_gh/eskull_hall room and object identities, and the owner/editor/deployment/persisted-ID questions. No WLD metric or approach total changes. See the [24 builder questions](ETHDAWN_OP_BUILDER_QUESTIONS.md), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-890-ethdawn-op-role-aware-boundaries), [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-890-ethdawn_op-retains-typed-portal-and-map-identities), [candidate row 41](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L41), and [coverage row 391](ZONE_COVERAGE_REGISTER.csv#L392). No area/gameplay source, tests, builds, server, database, migration, or runtime was changed or used.
