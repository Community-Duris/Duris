# eskull_hall — Ebon Skull Guildhall source audit

Canonical key: component:eskull_hall. Physical coverage row 390 / CSV line
391; candidate dependency row 40 / CSV line 40. Reviewed October 11, 2026,
Checkpoint 477. This is a static source, connectivity, identity, history, and
typed-reference review. It does not establish runtime shop behavior, the
intended owner of shared VNUMs, or permission to activate or move the source.

## Source scope, title, and story

The component has [WLD](../../../areas/wld/eskull_hall.wld#L1),
[MOB](../../../areas/mob/eskull_hall.mob#L1),
[OBJ](../../../areas/obj/eskull_hall.obj#L1),
[ZON](../../../areas/zon/eskull_hall.zon#L1), and
[SHP](../../../areas/shp/eskull_hall.shp#L1) files. It is absent from full
[areas/AREA](../../../areas/AREA#L141) and
[areas/qst/AREA](../../../areas/qst/AREA); there is no same-stem QST, story
sidecar, or editor-map file. Its raw AREA annotation is blank. The ZON title
is Ebon Skull Guildhall, the only exact match among tracked ZON titles.

The WLD and MOB prose describe a dark mausoleum, tortured souls, Azrel's
long-term guard duty, Lacha of Clan Sabertooth, a lich and a scroll said to
summon undead from hell, the Hall of Lost Souls, the Archane Elders, and a
gateway to oblivion. Khayne says the guild hired him to tend the Shop of the
Dead. The theme and names are story-bearing, but there is no candidate QST or
story sidecar that defines an authoritative sequence, owner, or questline.
Preserve the text and spelling as source evidence until the content owner
resolves its canon and title.

## Candidate WLD graph and active-table ownership

The candidate has rooms #29300–#29312 and 24 authored directions. Twenty-two
directions stay within those numbers. The other two are D4/up exits:
#29300 to #156221 and #29310 to #162341, both titled The Ocean in inactive [Duris3 #156221](../../../areas/wld/Duris3.wld#L1180642)
and [Duris3 #162341](../../../areas/wld/Duris3.wld#L1309162). Neither target has a
return exit to this candidate. The candidate-only graph has two weak
components: #29300–#29309 (10 rooms), all directed-reachable from #29300,
and #29310–#29312 (3 rooms), all directed-reachable from #29310. There are no
WLD fall-chance or current records.

All thirteen room VNUMs also occur in active [Newbie WLD](../../../areas/wld/newbie.wld#L1975)
with different complete room blocks. The first ten also occur in inactive
cbr_hall, loc_gh, and ravenmis; #29310–#29312 also occur in inactive
ethdawn_op and loc_gh. Across these 49 complete same-number room
comparisons, none is identical. For example, candidate #29302 is the Shop of
the Dead while active Newbie #29302 is Tight Squeeze; candidate #29305 is
the Falling Star Inn while active Newbie #29305 is A Secret Magic Shop
([candidate #29302](../../../areas/wld/eskull_hall.wld#L36),
[active #29302](../../../areas/wld/newbie.wld#L2021),
[candidate #29305](../../../areas/wld/eskull_hall.wld#L92),
[active #29305](../../../areas/wld/newbie.wld#L2050)). Newbie's [AREA
entry](../../../areas/AREA#L141) currently owns zone range *292–293. Thus the
22 local candidate target numbers resolve in the current active table to
Newbie room records; that does not load this candidate's map.

A full active-WLD scan found thirteen raw Newbie directions to #29300–#29312:

| Active Newbie source | Direction | Raw target |
| ---: | --- | ---: |
| #29224 | east | #29304 |
| #29228 | south | #29310 |
| #29292 | north | #29300 |
| #29295 | north | #29311 |
| #29301 | down | #29302 |
| #29302 | up | #29301 |
| #29306 | north | #29307 |
| #29307 | south | #29306 |
| #29309 | down | #29306 |
| #29313 | west | #29312 |
| #29314 | east | #29312 |
| #29315 | south | #29312 |
| #29316 | north | #29312 |

These are Newbie-to-Newbie directions because Newbie owns the target room
records. Candidate-identity active inbound remains zero; do not count these
raw numeric matches as approaches to Ebon Skull Guildhall. Curated inactive-WLD
totals remain 11,987 overall and 415 from Surf.

## Same-number prototypes, portals, and code consumers

Candidate mobiles #29300–#29303 have different complete same-number records
from active Newbie: Azrel, Lacha, the lich, and Khayne are not Newbie's
training mercenary, assassin student, alchemist, and bleeding human. The
candidate's #29303 is Khayne; active [Newbie #29303](../../../areas/mob/newbie.mob#L1544)
is the bleeding man, reset into #29289
([Newbie reset](../../../areas/zon/newbie.zon#L391)). Inactive guild-hall
stems also reuse these mobile numbers.

Two compiled consumers make the VNUM type especially important. The Ailvio
constants reserve mobile #29303 and #29304
([definitions](../../../src/world/vnum.mob.h#L13)); the handler checks the
#29303 incapacitated mobile ([handler](../../../src/specs/specs.ailvio.c#L168)).
Newbie supplies those mobile identities as its bleeding man and the
half-elf woman. Candidate #29304 is a room, the ancient library; it does not
define mobile #29304. Separately, compiled code attaches the inn callback to
real_room0(29305) ([assignment](../../../src/specs/specs.assign.c#L2447)).
The active Newbie room at #29305 is A Secret Magic Shop; this candidate's
#29305 is Falling Star Inn. The current VNUM consumer therefore points at
the active Newbie room identity, not this unlisted inn.

The candidate OBJ file defines two type-25 portals
([prototypes](../../../areas/obj/eskull_hall.obj#L1)). #29300 is reset in
#29309 and stores target #29312; #29301 is reset in #29312 and stores target
#29309. Both store command 7 (CMD_ENTER) and charge value -1. This is a
source-level reciprocal-looking pair; the prose only calls them gateways to
oblivion. Current active Newbie objects #29300 and #29301 are instead the
Pain dagger and a sign, and Newbie defines object records #29300–#29312.
Newbie QST/story item fields use #29300, #29302, and #29310–#29312
([QST examples](../../../areas/qst/newbie.qst#L539),
[story rewards](../../../areas/story/newbie.story.json#L718)). These fields
belong to Newbie's active item identities, not the candidate portals. The
portal implementation treats value[0] as a room VNUM, value[1] as the
required command, and value[2] as a nonzero charge setting
([handling](../../../src/magic/spell_travel.c#L991),
[CMD_ENTER](../../../src/cmd/interp.h#L42)); it decrements only positive
charges ([charge handling](../../../src/magic/spell_travel.c#L1173)).
Confirm both target identities and the negative-charge convention with the
object owner.

## Resets, SHP fallback, and history

eskull_hall.zon imports Player Castles objects #11056 into #29300,
#11106 into #29303, #11013 into #29305, and #11106 into #29312
([reset imports](../../../areas/zon/eskull_hall.zon#L9)). It resets Azrel,
Lacha, two liches, and Khayne in #29300/#29302, then lists fifteen G-reset
objects immediately after Khayne's M reset. That list exactly matches the
fifteen products declared by the one SHP record: #377, #3050, #398, #6089,
#111, #109, #108, #412, #53, #347, #364, and #491–#494
([shop](../../../areas/shp/eskull_hall.shp#L1),
[zone inventory](../../../areas/zon/eskull_hall.zon#L20)).
Fourteen of these product VNUMs have current OBJ definitions; #6089 has no
prototype in areas/obj/*.obj. Several IDs also have inactive duplicate
definitions, while their current active definitions are in Heavens or Verz1.
The existing Player Castles import review covers the four external reset
references; retain them until both area owners confirm their intended items.

areas/world.shp is absent, and SHOP_FILE names that path
([definition](../../../src/world/db.h#L22)). In non-mini mode,
[open_shop_stream](../../../src/economy/shop.c#L3267) first tries that file
and, if absent, concatenates areas/shp/*.shp before appending end.shp;
this fallback does not filter by areas/AREA. The source therefore reaches a
conditional global shop-loader path even though its WLD/MOB/OBJ/ZON stem is
unlisted. The candidate SHP names keeper VNUM #29303 and shop-room VNUM
#29302 ([keeper and room fields](../../../areas/shp/eskull_hall.shp#L49)).
The loader passes keeper #29303 to real_mobile
([keeper lookup](../../../src/economy/shop.c#L3489)). In today's active
tables those numbers identify Newbie's bleeding man and Tight Squeeze, not
candidate Khayne and the Shop of the Dead. This is a static path and identity
conflict, not a runtime shop test or claim that the candidate map is loaded.

The followed histories are bounded: WLD has a 2009 tree move and a 2013
same-stem zero-line diff; MOB reaches the 2009 move; OBJ has the 2009 move and
a 2016 capitalization/punctuation edit; ZON and SHP have the 2009 move, a
2013 zero-line diff, and a 2026 change adding gathering bags #491–#494 to
both the G resets and shop product list
([2026 resets](../../../areas/zon/eskull_hall.zon#L32),
[shop products](../../../areas/shp/eskull_hall.shp#L14)). No alternate-stem
predecessor or explicit whole-component replacement appears in these histories.
The recent stock edit shows maintenance of this source, but by itself does not
identify its map owner or establish that the area files are loaded.

Keep candidate row 40 **RETAIN IN PLACE**. Resolve the two Duris3 exits, all
shared room/mobile/object identities, the inn and Ailvio code bindings, the
type-25 portal pair, the Player Castles imports, the conditional SHP fallback
and its missing #6089 product, the source/editor owner, deployment, and
persisted-VNUM closure before any activation, move, rename, archive, or merge
proposal. See the [26 builder questions](ESKULL_HALL_BUILDER_QUESTIONS.md),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-477-eskull_hall-ebon-skull-guildhall),
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-477-eskull_hall-identity-and-bounded-history),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-477-eskull_hall-retained-pending-map-code-and-shop-review),
[candidate row 40](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L40), and
[coverage row 390](ZONE_COVERAGE_REGISTER.csv#L391). No area/source files,
tests, builds, runtime, server, database, or containers were changed or used.

## Checkpoint 889: role-aware source and shop-field closure

This pass classifies Ebon Skull Guildhall's room IDs #29300-#29312, mobile IDs #29300-#29303, and portal objects #29300-#29301 by the fields the loader resolves ([`renum_zone_table`](../../../src/world/db.c#L1612)). Same-number hits from another source remain that source's identity.

The WLD finding remains thirteen active Newbie-to-Newbie target matches and zero candidate-identity inbound exits. No approach or curated total changes. Role-aware scans found 65 external ZON room operands in the shared room range: cbr_hall 7, ethdawn_op 7, loc_gh 17, active Newbie 24, and ravenmis 10. The scan includes door-state (`D`) room fields as well as reset locations. It also found 27 mobile-prototype operands in cbr_hall (5), loc_gh (13), Newbie (5), and ravenmis (4), plus six object-prototype operands for #29300/#29301 in cbr_hall (1), loc_gh (2), Newbie (1), and ravenmis (2). Each belongs to the zone that authored the reset; the shared numbers do not load or consume Ebon Skull's different rooms, mobiles, or portals ([cbr_hall resets](../../../areas/zon/cbr_hall.zon#L9), [ethdawn_op resets](../../../areas/zon/ethdawn_op.zon#L9), [loc_gh resets](../../../areas/zon/loc_gh.zon#L9), [Newbie resets](../../../areas/zon/newbie.zon#L108), [ravenmis resets](../../../areas/zon/ravenmis.zon#L9)).

Six external OBJ type-25 destinations also fall in the shared room band: ethdawn_op #29310 -> #29307 and #29311 -> #29312; loc_gh #29300 -> #29309 and #29302 -> #29307; active Newbie #29299 -> #29305 and #29313 -> #29308. The inactive components own their portal source records, and Newbie's two portals target active Newbie rooms. No external type-29 destination was found. These source-level targets do not point to a uniquely identified Ebon Skull room ([ethdawn_op portals](../../../areas/obj/ethdawn_op.obj#L7), [loc_gh portals](../../../areas/obj/loc_gh.obj#L7), [Newbie portals](../../../areas/obj/newbie.obj#L1367)).

The all-shard `N`-format SHP scan found four other records with roles in this numeric band: cbr_hall keeper/room #29303 and stock #29300; loc_gh keeper #29301 and room #29308; Newbie shop #29283 in room #29305; and ravenmis keeper #29300, room #29309, and stock #29301. The cbr_hall and ravenmis stock prototypes are their own Ogre-hide backpack and fried Ogre-toe records; under today's active table, those numbers select Newbie's Pain dagger and sign instead. The loc_gh/Newbie/cbr_hall/ravenmis shopkeeper and room IDs likewise belong to their own source maps. Ebon Skull's own #29303 shop uses keeper #29303, room #29302, and its documented fifteen-product list. `assign_the_shopkeepers()` explicitly supports multiple room-scoped shops sharing one mobile prototype, so the matching cbr_hall/eskull_hall keeper number is a source-owner question, not by itself a parser collision ([shopkeeper assignment](../../../src/economy/shop.c#L3602), [cbr_hall shop](../../../areas/shp/cbr_hall.shp#L1), [loc_gh shop](../../../areas/shp/loc_gh.shp#L1), [Newbie shop](../../../areas/shp/newbie.shp#L256), [ravenmis shop](../../../areas/shp/ravenmis.shp#L1), [candidate shop](../../../areas/shp/eskull_hall.shp#L1)).

QST `I` fields and story-sidecar `I:` / item values are Newbie object roles, not exits into candidate rooms. The #29300 item collision resolves to active Newbie's Pain dagger, while #29302 and #29310-#29312 are other Newbie items. The sidecar's #29304 `mob_vnum` is a Newbie mobile role; Ebon Skull has no mobile #29304. Compiled references have the same typed split: Ailvio mobile constants #29303/#29304 select active Newbie's bleeding man and half-elf woman, while Ebon Skull #29304 is a room only; `real_room0(29305)` binds the inn callback to active Newbie's room #29305, not Ebon Skull's Falling Star Inn. No direct compiled consumer of candidate portal object #29300/#29301 was found ([Newbie QST](../../../areas/qst/newbie.qst#L539), [Newbie story](../../../areas/story/newbie.story.json#L718), [Ailvio mobile constants](../../../src/world/vnum.mob.h#L13), [Ailvio handler](../../../src/specs/specs.ailvio.c#L168), [inn binding](../../../src/specs/specs.assign.c#L2447)).

The global shop caveat from Checkpoint 886 also applies here. This checkout has no `areas/world.shp`; the non-mini fallback globs all `areas/shp/*.shp`, including the candidate and unlisted shop records. Its aggregate includes eight legacy `O`-format records in `luiren.shp`, and the current loader calls `fatal_boot_error` when it sees a non-`N` record. Therefore source inclusion is not proof of a successfully loaded catalog or live keeper/room mapping ([fallback and format gate](../../../src/economy/shop.c#L3267), [legacy records](../../../areas/shp/luiren.shp#L1), [Checkpoint 886 loader review](EOO_AUDIT_REVIEW.md#checkpoint-886-role-aware-reference-closure-and-shop-stream-qualification)).

Keep candidate row 40 **RETAIN IN PLACE**. Preserve the two Duris3 exits, all distinct cbr_hall/ethdawn_op/loc_gh/Newbie/ravenmis identities, candidate portal pair, four Player Castles imports, shop stock and #6089 gap, Ailvio/inn bindings, and the source-specific SHP records. The owner, external-tool, catalog, deployment, story, and persisted-VNUM questions remain open; no WLD approach count changes. See the [24 builder questions](ESKULL_HALL_BUILDER_QUESTIONS.md), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-889-eskull_hall-typed-role-connectivity-and-shop-dependencies), [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-889-eskull_hall-retains-separate-map-and-shop-identities), [candidate row 40](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L40), and [coverage row 390](ZONE_COVERAGE_REGISTER.csv#L391). No area/gameplay source, tests, builds, runtime, server, database, or container was changed or used.
