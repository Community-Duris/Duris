# Orog Encampment (`orogs`): source story review

**Scope:** source review of the 124 WLD rooms, 37 mobile prototypes, eight
local object prototypes, three shop records, zone resets, and authored story
sidecar. It does not qualify live shop services, special behavior, route
safety, or ordinary versus staff reachability.

## Story coverage state

`orogs` is an active full-area contributor, zone `458`, with 124 WLD rooms,
37 mobile prototypes, eight local object prototypes, and three shop records.
It has no native QST contracts. Its sidecar reports
[`coverage: complete`](../../../areas/story/orogs.story.json#L5), but has empty
[`contacts`, `stories`, and `exclusions`](../../../areas/story/orogs.story.json#L11)
arrays. Its introduction
invites players to meet Orog residents and “uncover local requests,” while its
orientation points to general `help quest` guidance. That source combination
records no local story contact or request; it does not prove that no other
special, shop, or live service behavior exists.

## Environmental story

The WLD contains 124 room records and 28 distinct title/description
combinations. Repeated descriptions are used for distinct path and village
rooms; the source review groups their setting beats below rather than treating
identical prose as duplicate room records.

| Setting | Source evidence | Story cues |
| --- | --- | --- |
| Urusi cave approach | [Cave entrance](../../../areas/wld/orogs.wld#L1), [deep cave](../../../areas/wld/orogs.wld#L26) | A pair of cave entrances opens into a carved tunnel network. Battle scriptures, crossed spear-and-shield displays, and a persistent drumbeat establish a long-settled, martial culture before the village appears. |
| Gorge and bridge | [First approach](../../../areas/wld/orogs.wld#L503), [second approach](../../../areas/wld/orogs.wld#L836), [bridge span](../../../areas/wld/orogs.wld#L857) | A deep gorge separates the cave approach from the Orog side. A swaying wood-and-rope bridge crosses it; the text emphasizes wind, darkness, and the bridge's improvised anchoring. |
| Fortified village | [Orog entrance](../../../areas/wld/orogs.wld#L967), [village wall](../../../areas/wld/orogs.wld#L1007), [boardwalk](../../../areas/wld/orogs.wld#L1948), [below the palisades](../../../areas/wld/orogs.wld#L2145) | The village is excavated into a cavern and enclosed by a high bulwark and palisade. Its open center and busy residents contrast with military organization, lookout paths, drums, blood-stained timbers, and war trophies. |
| Command and worship | [Command compound](../../../areas/wld/orogs.wld#L2347), [elite garrison](../../../areas/wld/orogs.wld#L2426), [bloodletting altar](../../../areas/wld/orogs.wld#L2458), [Gruumsh altar](../../../areas/wld/orogs.wld#L2472), [foyer](../../../areas/wld/orogs.wld#L2487), [chieftain's office](../../../areas/wld/orogs.wld#L2507) | The inner compound has regular patrols and a stocked garrison. The bloodletting room ties pain and hardening to warrior training; the adjacent temple centers Gruumsh worship, prayer, and destruction. The decorated foyer and command office add signs of hierarchy, planning, raids, and clan identity. |
| Work and trade | [Foundry](../../../areas/wld/orogs.wld#L2700), [witchdoctor's hut](../../../areas/wld/orogs.wld#L2716), [weaponmaster's hut](../../../areas/wld/orogs.wld#L2731), [supply depot](../../../areas/wld/orogs.wld#L2746), [mounts](../../../areas/wld/orogs.wld#L2760), [bank](../../../areas/wld/orogs.wld#L2787), [inn](../../../areas/wld/orogs.wld#L2802) | The encampment has metalworking, medicine and ritual supplies, weapons, provisions for a “non-violent sector,” animal care, a bank, and an inn. Prose associates the bank with outside merchants and a dead-animal tithe; the inn displays the tribe's hunting and warfare trophies. |

Several rooms are explicitly population staging rooms rather than public
destinations: `45909` is marked for loading Urusi cave creatures, and
`45910–45915` for loading village populations
([Urusi room](../../../areas/wld/orogs.wld#L2526),
[village room](../../../areas/wld/orogs.wld#L2572)). Their titles should not be read
as missing story scenes.

## Residents and authored services

Static reset rows place the Orog chieftain `45822` in his office at `45908`,
the ancient bladesman `45820` at the bloodletting practice room `45905`, and
the ancient priest `45821` at the Gruumsh temple `45906`
([resets](../../../areas/zon/orogs.zon#L122),
[bladesman](../../../areas/mob/orogs.mob#L319),
[priest](../../../areas/mob/orogs.mob#L335),
[chieftain](../../../areas/mob/orogs.mob#L351)).
Commanders and legionnaires, shadowpriests and doomguards, ordinary soldiers,
adolescents, workers, clergy, and cave creatures populate the connected
staging rooms. This gives the setting a visible command hierarchy, training,
ritual, and village economy, but the reviewed sidecar and QST sources do not
turn those scenes into named local story steps.

The `.shp` file has shop records for mobile VNUMs `45829`, `45831`, and
`45832` ([weapon stock](../../../areas/shp/orogs.shp#L1),
[beastmaster](../../../areas/shp/orogs.shp#L47),
[traveling merchant](../../../areas/shp/orogs.shp#L80)). The prose places the
weaponmaster, supply depot, and mount paddock in these same service areas.
This is source mapping, not qualification of shop access, inventory renewal,
or the mount service. Matching zone resets place `45829` in the weaponmaster's
hut, `45832` in the supply depot, and `45831` in the mount paddock
([weaponmaster](../../../areas/zon/orogs.zon#L229),
[merchant](../../../areas/zon/orogs.zon#L240),
[beastmaster](../../../areas/zon/orogs.zon#L251)).

Two Orog mobile prototypes and their duplicate variants carry the literal
`_spec1_` token ([spearmaster](../../../areas/mob/orogs.mob#L18),
[priest](../../../areas/mob/orogs.mob#L35),
[later spearmaster](../../../areas/mob/orogs.mob#L559),
[later priest](../../../areas/mob/orogs.mob#L576)). This review does not
identify or qualify the class-specific handler behind that token.

## Reset and cross-area dependencies

The zone file contains 203 commands: 132 `M`, 12 `F`, 12 `O`, 13 `E`, 20
`G`, and 14 `D`. The mobile, follower, and equipment references resolve to
local prototypes. The 20 `G` commands reference objects from active
`heavens.obj`: ten weapon VNUMs (`254`, `260`, `258`, `263`, `255`, `288`,
`286`, `280`, `267`, `289`) placed on the nasty-looking Orog, and ten supply
VNUMs (`385`, `386`, `393`, `363`, `368`, `369`, `377`, `376`, `367`, `191`)
placed on the traveling merchant ([weapon resets](../../../areas/zon/orogs.zon#L230),
[merchant resets](../../../areas/zon/orogs.zon#L240),
[Heavens prototypes](../../../areas/obj/heavens.obj)). Both `heavens` and
`orogs` are in the active source list
([AREA entries](../../../areas/AREA#L2), [Orog entry](../../../areas/AREA#L215)).

Eleven room-object placement commands use local prototypes. The remaining
`O 0 3097` at bank room `45922` references a wooden counter from active
`verz1.obj` ([reset](../../../areas/zon/orogs.zon#L76),
[counter](../../../areas/obj/verz1.obj#L1199),
[AREA entry](../../../areas/AREA#L15)). These are explicit cross-area source
dependencies. The available sources do not justify replacing the IDs or
changing stock, equipment, or bank setup.

The ZON comments mark twelve door-state commands as closed and two commands
to the Immortals-only mount storeroom as closed/locked
([door state block](../../../areas/zon/orogs.zon#L8),
[storeroom doors](../../../areas/zon/orogs.zon#L56)). The room graph alone
therefore does not establish which ordinary-player paths through the village
are open or safe.

The static WLD census records 307 exits, all loader-resolved. Four exits lead
from Orog entrance rooms `45800` and `45807` to `underdark`; all four have
reverse-direction records in the other area. The exact pairs are in the
[cross-area ledger](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L335) and the four
[Orog exit rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L624). The source register
places all 124 local rooms in one weak component and records no unresolved or
unregistered-WLD targets
([coverage row](ZONE_COVERAGE_REGISTER.csv#L224)). This is a static source
graph, not a proof of player access, door state, or safe return.

## Title, related area identities, and followed history

After removing MUD color controls, trimming the title terminator and whitespace,
and folding case, “Orog Encampment” appears once among all 442 current `.zon`
titles. The only other current title containing “Encampment” is “The Cimmerian
Nomad Encampment” (`nomads`, zone 62); related Ogre titles are “Ogre Raiders”
(`raiders`, zone 89) and “Valley of the Snow Ogres” (`snogres`, zone 877).
These are separate current contributors in the world roster
([Nomads](../../../areas/AREA#L31), [Raiders](../../../areas/AREA#L50),
[Orogs](../../../areas/AREA#L215), [Snow Ogres](../../../areas/AREA#L307)) and
the QST roster ([Nomads](../../../areas/qst/AREA#L29),
[Raiders](../../../areas/qst/AREA#L44), [Orogs](../../../areas/qst/AREA#L197),
[Snow Ogres](../../../areas/qst/AREA#L273));
their `.zon` titles are distinct ([Orogs](../../../areas/zon/orogs.zon#L2),
[Nomads](../../../areas/zon/nomads.zon#L2),
[Raiders](../../../areas/zon/raiders.zon#L2),
[Snow Ogres](../../../areas/zon/snogres.zon#L2)).

The Snow Ogres are not a second Orog map or a tracked replacement: their
independent source-story audit documents a 100-room valley at #87700–#87799,
its own QST, resets and procedure set, and separate Surface/Faerie routes
([audit](SNOGRES_AUDIT_REVIEW.md#active-graph-and-source-identity)). The Orog
rooms occupy #45800–#45923; the four recorded Orog cross-area exit records are
the reciprocal Underdark boundary already listed above. A bounded search of
the Snow Ogres WLD, MOB, OBJ, ZON, QST, story sidecar and story dossier found no
Orog-name reference. A wider scan finds Orog residents and story references in
other identities: Underdark has the mapped Orog entrance rooms and its own
dispersal room ([entrances](../../../areas/wld/underdark.wld#L215426),
[dispersal](../../../areas/wld/underdark.wld#L230082)); other independent
references appear in Faang, Dagger Peak, Ceothia, Alatorin, and Surface
([Faang prose](../../../areas/wld/faang.wld#L7772),
[Dagger Peak quest](../../../areas/qst/caertannad.qst#L461),
[Ceothia mobile](../../../areas/mob/ceofutur.mob#L859),
[Alatorin quest](../../../areas/qst/alatorin.qst#L7041)). They do not title a
second Orog Encampment zone.

The active Surface reset places Ruukoz mobile #500182 in room #660000, the
“Dispersement room for Evil Spec Teachers” ([reset](../../../areas/zon/surface.zon#L1726),
[room](../../../areas/wld/surface.wld#L3359377)). His description says he was
sent out to teach from a small Orog encampment. Retained source-only
`surface2011` has a parallel Ruukoz mobile #200161 and a copy of the Gr'Tak
quest text, but its WLD room #200161 is an empty-description “Azure Waters of a
Stormy Sea” grid tile and its ZON has no reset for mobile #200161
([mobile](../../../areas/mob/surface2011.mob#L2406),
[room](../../../areas/wld/surface2011.wld#L3382),
[quest](../../../areas/qst/surface2011.qst#L144),
[retained candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L98)).
These Surface references preserve separate NPC/story copies; they do not
identify an alternate Orog home map or predecessor. Keep the existing
`surface2011` candidate plan and retention unchanged.

Followed history for the current Orog WLD, MOB, OBJ, ZON and SHP paths begins at
the 2009-08-12 area-tree move. The WLD has no later tracked edit; MOB, OBJ, ZON
and SHP edits remain under the `orogs` stem through their latest entries in
2026, 2013, 2015 and 2014 respectively. The `orogs` story sidecar was added on
2026-10-02. There is no local QST source file. No alternate-stem predecessor,
tracked rename, or whole-zone replacement appears in the followed histories or
current source rosters. Keep active `orogs` **DO NOT MOVE**; command-target
aliases and owner disambiguation remain open separately.

## Builder questions and recommendations

1. **OROGS-Q01 — Is the empty story sidecar intentional?** Confirm whether
   [`coverage: complete`](../../../areas/story/orogs.story.json#L5) with no
   contacts, stories, or exclusions is the intended record for an
   environmental-only village, given its generic
   [“local requests” introduction](../../../areas/story/orogs.story.json#L6).
   Do not invent contacts, request steps, or quest rewards.
2. **OROGS-Q02 — Should the chieftain, trainers, Gruumsh worship, or village
   services have authored story outcomes?** Identify any intended request or
   service arc before adding it to the mapping. The present area has no native
   QST file; the sidecar's [contacts, stories, and exclusions](../../../areas/story/orogs.story.json#L11)
   are empty. Do not infer an accepted outcome from a resident, service, or
   worship scene. See the [chieftain reset](../../../areas/zon/orogs.zon#L122),
   [bladesman](../../../areas/mob/orogs.mob#L319), and
   [priest](../../../areas/mob/orogs.mob#L335).
3. **OROGS-Q03 — Are the external reset objects intentional?** Preserve the
   Heavens weapon and traveling-merchant items and the Verz1 bank counter until
   the owners confirm their roles; do not substitute same-purpose items or
   rebalance stock. See the [bank counter reset](../../../areas/zon/orogs.zon#L76),
   [weapon resets](../../../areas/zon/orogs.zon#L230), and
   [traveling-merchant resets](../../../areas/zon/orogs.zon#L240).
4. **OROGS-Q04 — Are the closed and locked door states intentional for the
   village's ordinary route?** Review the two Underdark connections, the
   bulwark gates, service doors, and staff-only storeroom before any access
   proposal. Do not open or relocate routes from the static graph alone. See
   the [door state block](../../../areas/zon/orogs.zon#L8),
   [staff storeroom doors](../../../areas/zon/orogs.zon#L56), and
   [four reciprocal Underdark exits](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L335).
5. **OROGS-Q05: What class specialization should the `_spec1_` spearmaster and priest select?**
   Conversion code reads `_spec1_` as specialization slot 1 when the mobile is
   single-class, has no assigned spec, and that class has slot-1 data
   ([conversion](../../../src/mob/mobconv.c#L112),
   [availability check](../../../src/world/db.c#L2327)). Keep the markers and
   confirm each prototype's class/spec mapping and resulting behavior before
   describing a live ability.
6. **OROGS-Q06 — Are any player-facing command aliases intended to target the
   Surface copies of Orog characters or their quest text?** Active Surface
   places Ruukoz #500182 in the teacher-distribution room; retained
   `surface2011` has a separate Ruukoz #200161 prototype and a Gr'Tak quest
   copy, while room #200161 has no reset for that mobile. Confirm the command
   or quest owner and intended target before adding or changing an alias.
   Preserve the active `orogs`, active `surface`, and retained `surface2011`
   identities separately; do not infer a rename, redirect, or replacement
   from the copied names. See [the source identity review](#title-related-area-identities-and-followed-history),
   [active Surface reset](../../../areas/zon/surface.zon#L1726),
   [retained Ruukoz mobile](../../../areas/mob/surface2011.mob#L2407), and
   [retained Gr'Tak quest](../../../areas/qst/surface2011.qst#L144).

## Safe resolution summary

Keep `orogs` and all referenced active-area objects in place. Record the
environmental story and open builder questions while leaving the local QST
absent and the sidecar story arrays unchanged pending an owner decision.
Preserve current shop stock, external reset IDs, and door states. No gameplay,
access, combat, shop, item, placement, command-alias, or migration change is
proposed.

## Checkpoint 944: typed source and fixed-code closure

The bounded repository scan found no external exact VNUM references to Orogs in other `.zon`, `.shp`, or `.qst` files, and no typed VNUM hit in external story sidecars. A raw number in `wh.story.json` belongs to a request UUID, not a room, mobile, or object field. No external WLD, MOB, or OBJ prototype has a same-type definition colliding with Orogs, and no external OBJ type-25/type-29 destination targets an Orog room. The four registered reciprocal WLD pairs with Underdark remain the authored route boundary. The existing outbound typed dependencies remain the 20 `G` rows for active Heavens objects and bank counter #3097 from active Verz1.

Compiled source also binds `rentacleric` to mobile #45818 and `money_changer` to #45833 ([assignments](../../../src/specs/specs.assign.c#L221)). `HOME_OROGS` is home index 24 and the `hometown[]` table maps it to room #45906 ([home constant](../../../src/core/defines.h#L1117), [start room](../../../src/core/constant.c#L1744)); the Orog `guild_locations` row includes #45905 and #45906 ([guild destinations](../../../src/core/constant.c#L2001)). These are compiled source bindings, not proof that a player can invoke the service or use every route.

The `_spec1_` markers on the spearmaster and priest are interpreted by mobile conversion code as specialization slot 1 when conversion conditions and class data permit it ([mobile conversion](../../../src/mob/mobconv.c#L112), [class-data check](../../../src/world/db.c#L2327)). This is a class-specialization selector, not an individual special-procedure assignment. The specific class data and live ability outcomes remain for owner/runtime qualification.

This closes the bounded typed-source and fixed-code search. Editor projects, generated exports, deployment inputs, persisted player state, live shop/service outcomes, door and reset effects, and dynamic access/safety remain unqualified. Keep Orogs **DO NOT MOVE** and preserve its active Heavens, Verz1, Underdark, and fixed-code identities while those questions remain open.

### Builder question 7

**Should the compiled `rentacleric` and `money_changer` bindings to Orog mobiles #45818 and #45833 remain?** Preserve both bindings while the area and code owners confirm intended services and access; do not promise live service from the assignments alone. See the [assignments](../../../src/specs/specs.assign.c#L221).
