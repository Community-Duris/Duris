# Fort Marigot (`marigot`): source story review

**Scope:** source review of 118 WLD rooms, 48 mobile prototypes, 31 local
object prototypes, seven `.shp` records, the reset file, and the authored story
sidecar. This maps static text, placements, and source dependencies; it does
not qualify live shops, access, combat, special procedures, or safe traversal.

## Story coverage state

`marigot` is an active contributor in `areas/AREA` as zone 53, titled Fort
Marigot, with last room 5417 ([AREA entry](../../../areas/AREA#L27)). Its 118
rooms, 48 mobiles, 31 objects, and two QST giver records are recorded in the
[coverage row](ZONE_COVERAGE_REGISTER.csv#L171). The QST file has two message
records but zero native Q/QA contracts. The sidecar marks coverage complete,
has one contact, and leaves both `stories` and `exclusions` empty
([sidecar](../../../areas/story/marigot.story.json#L5)).

The contact is Kerbien, mobile 5317, with keyword `druid`; the sidecar lists
only topic `hi` ([contact](../../../areas/story/marigot.story.json#L11)). The
QST record accepts `hi hello` and gives a short statement about worshipping
Nature ([Kerbien QST record](../../../areas/qst/marigot.qst#L1)). This is
conversation, not a mapped objective or reward chain. The sidecar's generic
promise to “uncover local requests” is broader than its mapped content.

The second QST record is Talish, mobile 5347, with `qc_action 48`; its message
describes him smiling and wiping a glass ([Talish QST record](../../../areas/qst/marigot.qst#L10)).
The source implementation treats `qc_action` as a timed room emote sent with
`TO_ROOM` and `has_quest_ask` skips those entries when checking for an ask
([routine](../../../src/world/quest.c#L88), [room message](../../../src/world/quest.c#L105),
[ask check](../../../src/world/quest.c#L2446)). It is ambient dialogue, not a
player story contract.

## Environmental story

A text grouping finds 91 distinct exact room title/description pairs across
the 118 rooms. Repeated stretches of the Pathway of the Princess and similar
street or lake text are environmental repetition; they do not establish
duplicate rooms or a replacement area.

| Setting | Source evidence | Story cues |
| --- | --- | --- |
| Civic center and gates | [Olde Inn](../../../areas/wld/marigot.wld#L2), [bakery](../../../areas/wld/marigot.wld#L24), [central pond](../../../areas/wld/marigot.wld#L39), [Princess pathway](../../../areas/wld/marigot.wld#L222), [granite gate](../../../areas/wld/marigot.wld#L366) | Fort Marigot is a settled centaur town with an inn, baker, magical pond, paved streets, and a stone-and-iron boundary. The pathway and paired gate approaches frame the fort as a lived-in, guarded community. |
| Justice, guard, and holy army | [jailhouse](../../../areas/wld/marigot.wld#L121), [justice hall](../../../areas/wld/marigot.wld#L150), [judgement room](../../../areas/wld/marigot.wld#L182), [holding cell](../../../areas/wld/marigot.wld#L207), [training ground](../../../areas/wld/marigot.wld#L675), [army training room](../../../areas/wld/marigot.wld#L696) | The fort has a court and jail as well as royal guards, elevated posts, and holy-army training. The source shows a civic and military order, but it does not define legal outcomes or player advancement. |
| Guilds, worship, and craft | [Bar of Mysteries](../../../areas/wld/marigot.wld#L406), [armory](../../../areas/wld/marigot.wld#L447), [ranger guild](../../../areas/wld/marigot.wld#L791), [shaman halls](../../../areas/wld/marigot.wld#L958), [druid sanctum](../../../areas/wld/marigot.wld#L1027), [barrelers](../../../areas/wld/marigot.wld#L1955), [instrument shop](../../../areas/wld/marigot.wld#L2004) | Bards and tavern visitors sit alongside ranger, druid, and shaman institutions. Training rooms, instructors, ritual spaces, and local instrument and barrel crafts give the settlement distinct guild and trade districts. |
| Lake, forest, and the dragon | [Mullet Lake](../../../areas/wld/marigot.wld#L1147), [underwater rooms](../../../areas/wld/marigot.wld#L1388), [marble doors](../../../areas/wld/marigot.wld#L1548), [Mullet’s lair](../../../areas/wld/marigot.wld#L1569), [druid grove](../../../areas/wld/marigot.wld#L1712), [garden](../../../areas/wld/marigot.wld#L936) | The lake descends through underwater rooms to a hidden cavern. Mobile 5314 is described as a lake dragon trapped after fighting the citizens; the text says they took pity rather than killing him ([Mullet](../../../areas/mob/marigot.mob#L214)). Nearby forest paths, wild flora, garden, and grove extend the druidic setting beyond the walls. |
| Memory and everyday life | [bank](../../../areas/wld/marigot.wld#L1971), [memorial room](../../../areas/wld/marigot.wld#L2070), [cemetery](../../../areas/wld/marigot.wld#L2091), [centaur church](../../../areas/wld/marigot.wld#L2207), [homes and cellar](../../../areas/wld/marigot.wld#L1884) | Bank, housing, church, cemetery, and memorial spaces put family, mourning, worship, and commerce in the same town. Resets also place grieving residents and ordinary centaur children, guards, and workers through these areas. |

## Residents and shops

The mobile source describes centaur bakers, guards, military trainers, bards,
rangers, shamans, druids, residents, and craftspeople. Kerbien is the elder
druid who trained younger centaurs ([mobile 5317](../../../areas/mob/marigot.mob#L262));
the reset places him in the inner stones of the grove
([reset](../../../areas/zon/marigot.zon#L288)). Seven shop records are present:

| Keeper / room | Stock source indicated by `.shp` | Source record |
| --- | --- | --- |
| Ghel 5301 / 5301 | Local bakery objects 5301–5302 | [shop](../../../areas/shp/marigot.shp#L1) |
| Laaeq 5307 / 5318 | Heavens objects 590, 667, 211, 246, 214, 686 | [shop](../../../areas/shp/marigot.shp#L36) |
| Hebira 5327 / 5383 | Heavens objects 385, 347, 386, 128, 129 | [shop](../../../areas/shp/marigot.shp#L78) |
| Ismira 5328 / 5386 | Local instruments 5319–5323 | [shop](../../../areas/shp/marigot.shp#L116) |
| Druid keeper 5331 / 5387 | Heavens objects 590, 276, 455, 678, 128 | [shop](../../../areas/shp/marigot.shp#L155) |
| Shaman keeper 5332 / 5388 | Heavens objects 105–109, 111, 128, 455, 678, 590, 363, 368, 369 | [shop](../../../areas/shp/marigot.shp#L193) |
| Ranger keeper 5333 / 5389 | Heavens objects 239, 217, 211, 285, 128 | [shop](../../../areas/shp/marigot.shp#L239) |

The imported shop objects resolve to definitions in active Heavens sources
([AREA entry](../../../areas/AREA#L2); examples: [105](../../../areas/obj/heavens.obj#L1558),
[128](../../../areas/obj/heavens.obj#L1756), [363](../../../areas/obj/heavens.obj#L3783),
[368](../../../areas/obj/heavens.obj#L3836),
[369](../../../areas/obj/heavens.obj#L3850)). IDs 105–109 also occur in
current unregistered `mavulsk` object source; `mavulsk` is not in `areas/AREA`
([register row](ZONE_COVERAGE_REGISTER.csv#L421)), so that duplicate source is
not a second active full-world definition. Marigot resets additionally use
Heavens object 361 and the wooden counter 3097 from active Verz1
([reset references](../../../areas/zon/marigot.zon#L122),
[Verz1 AREA entry](../../../areas/AREA#L15),
[counter prototype](../../../areas/obj/verz1.obj#L1199)). This records stock and
placements only; shop access, pricing, and live sales remain unqualified.

## Mullet’s door and key

Room 5365’s south marble-door exit names object 5317 as its key and leads to
room 5366, Mullet’s lair ([exit record](../../../areas/wld/marigot.wld#L1566)).
Object 5317 is explicitly a small blue key
([prototype](../../../areas/obj/marigot.obj#L192)), and the zone reset block
contains a `G` command for that object near the lake golem and Mullet resets
([reset](../../../areas/zon/marigot.zon#L272)). The reverse exit returns north
through the marble door
([reverse exit](../../../areas/wld/marigot.wld#L1582)). The reset file marks the paired door closed, locked,
and secret ([door resets](../../../areas/zon/marigot.zon#L72)). This is an
explicit static key-to-door link, although it does not qualify how a player
acquires or uses the key under live conditions.

The zone file also contains an object reset for crown 5316 near the Mullet
reset sequence ([reset](../../../areas/zon/marigot.zon#L274)); its prototype is
a crown of blue scales ([object](../../../areas/obj/marigot.obj#L180)), echoing
the dragon’s blue-scaled description. The sources make those names and
placements adjacent, but do not define a story reward or a progression step.

## Reset and cross-area dependencies

The reset source has 323 typed commands: 169 `M`, 5 `F`, 43 `G`, 67 `E`, 28
`D`, and 11 `O`. Its 28 door commands describe 14 paired boundaries:

| Rooms / boundary | Reset comment state |
| --- | --- |
| 5307–5308, judgement room and cell | Closed and locked |
| 5313–5314, inner pathway and gate | Closed |
| 5317–5382, tavern and bard backroom | Closed |
| 5319–5321 and 5320–5324, guardhouse ladders | Closed |
| 5328–5330, training room | Closed |
| 5333–5334, ranger guild entrance | Closed |
| 5339–5340, garden and connecting hall | Closed |
| 5365–5366, marble doors to Mullet | Closed, locked, secret |
| 5391–5392, cemetery and hall | Closed |
| 5396–5397, church entrance | Closed |
| 5400–5403 and 5403–5404, bedroom and cellar | Closed |
| 5415–5416, outer gate | Closed |

The `.zon` comments provide the paired room names and states
([first doors](../../../areas/zon/marigot.zon#L8),
[guild and guard doors](../../../areas/zon/marigot.zon#L24),
[lake and cemetery doors](../../../areas/zon/marigot.zon#L72),
[church, homes, and outer gate](../../../areas/zon/marigot.zon#L88)). These are
static declarations, not proof that the routes are normally impassable or
that players can safely traverse them.

| Direction | Source evidence | Review boundary |
| --- | --- | --- |
| Marigot shops and resets → Heavens objects | [shop records](../../../areas/shp/marigot.shp#L36), [reset objects](../../../areas/zon/marigot.zon#L122), [Heavens prototypes](../../../areas/obj/heavens.obj#L1558) | Heavens is active in `areas/AREA`. Preserve the listed stock and object references pending owner review. |
| Marigot bank reset → Verz1 object 3097 | [reset](../../../areas/zon/marigot.zon#L129), [counter](../../../areas/obj/verz1.obj#L1199) | Verz1 is active; the object is a wooden counter, not evidence of a live banking service. |
| Active `spshold` reset → Marigot object 5308 | [reset](../../../areas/zon/spshold.zon#L97), [active AREA entry](../../../areas/AREA#L114) | This active area has an inbound reference to a local Marigot object. |
| Current unregistered `nexus56` reset → Marigot object 5319 | [reset](../../../areas/zon/nexus56.zon#L1369), [register row](ZONE_COVERAGE_REGISTER.csv#L430) | A current source dependency; `nexus56` is not an active full-area contributor. |
| Current unregistered `solstice_gh` resets → Marigot objects 5319 and 5322 | [resets](../../../areas/zon/solstice_gh.zon#L30), [second reset](../../../areas/zon/solstice_gh.zon#L33), [register row](ZONE_COVERAGE_REGISTER.csv#L446) | Current source dependencies; `solstice_gh` is not an active full-area contributor. |

This is a source-reference crosswalk, not complete typed dependency closure.
Scripts, editor tooling, special procedures, generated identities, and
runtime behavior have not been qualified.

## Static connectivity and replacement boundary

The source exit census records 265 exits, all 265 loader-resolved. All 118
rooms are in one weak local component; the coverage row records two incoming
and two outgoing cross-area exits with two reverse records, two rooms with
fall-chance values, and no currents
([coverage row](ZONE_COVERAGE_REGISTER.csv#L171)). Both boundary pairs connect
to Surface: room 5315 exits west to 519317, and room 5417 exits east to 519319.
Surface has the reverse exits ([outbound pair](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L264),
[inbound pair](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L538),
[first Marigot exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L88),
[second Marigot exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L89),
[first reverse exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1357),
[second reverse exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1358)). This is
reciprocal static source topology, not a claim that ordinary players can enter,
leave, or return safely under live conditions.

The active-title scan records no repeated current area title for Marigot
([coverage row](ZONE_COVERAGE_REGISTER.csv#L171)). The repeated room prose and
the presence of other centaur or forest areas do not show that Marigot is
replaced. Keep this active area separate unless explicit source history or a
builder identifies a replacement relationship.

## Builder questions and recommendations

1. **Is the sidecar’s complete state intentionally conversation-only?** The
   sidecar has one greeting contact and no story or exclusion entries, while
   the intro promises local requests. Confirm that environmental scope is
   intended; either document that no local request chain is mapped or add the
   missing story content. Add `hello` as an alias in the sidecar if the QST
   `hi hello` record is intended to expose both topics.
2. **Should Talish’s QST record remain ambient-only?** The code classifies the
   `qc_action` entry as a room message and excludes it from ask detection. Keep
   it out of objective counts unless the builder identifies another intended
   player-facing contract.
3. **Are the Mullet door and key placement still intended as authored?** The
   WLD exit explicitly names 5317 as its key and the zone reset includes that
   object. Preserve the current key and door declarations while the builder
   confirms whether the secret lair is meant as an exploration/combat endpoint
   and whether the current reset placement is deliberate. Do not invent a
   reward or objective around crown 5316.
4. **Are all 14 paired doors intended access boundaries?** Confirm the jail,
   guild, church, housing, gate, and hidden-lair states before any route or
   reachability claim. Keep the current paired declarations pending that
   review.
5. **Should imported goods and the bank counter remain as authored?** Preserve
   Heavens stock and the Verz1 counter; also retain the inbound active
   `spshold` and current-source `nexus56` / `solstice_gh` reset references.
   The `mavulsk` duplicates for 105–109 are not active full-world definitions.
6. **Is there any replacement history beyond the current title scan?** No
   repeated active title or replacement evidence is recorded for Marigot.
   Keep it in the active list and do not move its files to an attic based on
   name similarity or repeated room text.

## Safe resolution summary

Keep Fort Marigot and its current sources in place. Record the town’s civic,
military, guild, worship, lake, and memorial setting; preserve the explicit
key-to-door reference, all 14 door pairs, seven shops, imported goods, bank
counter, and inbound reset dependencies. The sidecar and QST mismatch, ambient
Talish line, and safe route behavior need builder/runtime review before any
interpretive or gameplay change. No attic move or gameplay change is proposed.


## Checkpoint 273: direct builder and owner questions

These questions focus the remaining decisions around Marigot's conversation-only journal, civic and lake stories, keyed routes, services, and incoming source references. The recommendations preserve current source ownership and access declarations until builders choose a concrete outcome.

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 1 | Is the sidecar's complete state intentionally conversation-only, with no mapped local story or exclusion? | [sidecar coverage](../../../areas/story/marigot.story.json#L5), [empty story lists](../../../areas/story/marigot.story.json#L22), [coverage review](#story-coverage-state) | Keep the current classification if no request is intended; update the broad introduction or map any real objective and its saved result. |
| 2 | Should Kerbien's sidecar topics include hello as well as hi to match the QST record? | [sidecar topic](../../../areas/story/marigot.story.json#L17), [Kerbien QST](../../../areas/qst/marigot.qst#L1) | Add the alias only if the same greeting is intended; keep it one conversation rather than a new story achievement. |
| 3 | Should Talish's QST line remain an ambient room emote? | [Talish QST](../../../areas/qst/marigot.qst#L10), [ambient action behavior](../../../src/world/quest.c#L88) | Keep qc_action excluded from player objectives unless an owned player-facing interaction is designed. |
| 4 | Should the phrase about worshipping Nature remain Kerbien's only local conversation outcome? | [Kerbien QST](../../../areas/qst/marigot.qst#L1), [druid role](../../../areas/mob/marigot.mob#L262) | Keep a single informational greeting until a specific rite, beneficiary, and persistent result are selected. |
| 5 | Is Mullet meant to remain a trapped lake dragon and optional lair encounter, or should players have a defined release or rescue outcome? | [Mullet description](../../../areas/mob/marigot.mob#L214), [lake and lair setting](../../../areas/wld/marigot.wld#L1147) | Preserve the source encounter; define the actor, release state, and affected community before recording rescue or victory. |
| 6 | Is key #5317's placement and its locked, secret marble door the intended route to Mullet's lair? | [door and key review](#L80), [key reset](../../../areas/zon/marigot.zon#L272), [door reset](../../../areas/zon/marigot.zon#L72) | Preserve the typed key-to-door link; qualify acquisition and successful open separately from discovering or defeating Mullet. |
| 7 | Is Mullet's crown of blue scales intended as a story reward, or only as nearby reset stock? | [crown placement](../../../areas/zon/marigot.zon#L274), [crown prototype](../../../areas/obj/marigot.obj#L180) | Keep the current placement; do not infer an accepted reward or progression step from proximity and matching color prose. |
| 8 | Are the fourteen door pairs meant to keep their current reset-closed states across the jail, guilds, church, homes, and gates? | [paired door states](#reset-and-cross-area-dependencies), [reset declarations](../../../areas/zon/marigot.zon#L8) | Confirm the intended boundaries and any key or role policy before making route claims or changing reset values. |
| 9 | Which of the two reciprocal Surface crossings should be presented as the ordinary public entrance and return? | [Surface pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L264), [static connectivity](#static-connectivity-and-replacement-boundary) | Preserve both authored pairs; qualify actual entry, return, reset states, and safe traversal before publishing navigation. |
| 10 | Are the two rooms with fall chances intentional hazards, and what landing and recovery outcome should players expect? | [fall markers](ZONE_COVERAGE_REGISTER.csv#L171), [route census](#static-connectivity-and-replacement-boundary) | Keep the authored values; define eligible movement, impact, recovery, and any death risk before describing safe passage. |
| 11 | Are all seven shop records intended to operate with their current stock, prices, schedules, keepers, and race restrictions? | [shop inventory](#residents-and-shops), [first record](../../../areas/shp/marigot.shp#L1), [ranger record](../../../areas/shp/marigot.shp#L239) | Retain each shop record and qualify actual listing, payment, stock, delivery, and save before promising a live service. |
| 12 | Should the Heavens prototypes remain Heavens-owned goods sold and reset in Marigot? | [imported shop inventory](../../../areas/shp/marigot.shp#L36), [Heavens reset](../../../areas/zon/marigot.zon#L122), [ownership crosswalk](#reset-and-cross-area-dependencies) | Preserve Heavens as the prototype owner and keep the import relationship explicit; do not copy the goods into Marigot to imply local authorship. |
| 13 | Is the Verz1 wooden counter intended as a bank prop, or should builders define a compiled banking service for Marigot? | [counter reset](../../../areas/zon/marigot.zon#L129), [counter prototype](../../../areas/obj/verz1.obj#L1199) | Keep the foreign counter identity; describe a banking service only after its actual handler, access, fees, and saved transaction are verified. |
| 14 | Should active spshold continue resetting Marigot object #5308 as an outside consumer? | [active reset](../../../areas/zon/spshold.zon#L97), [dependency finding](#reset-and-cross-area-dependencies) | Preserve the active typed reference and its owner boundary; qualify stock and recipient behavior before changing it. |
| 15 | Should the current unregistered nexus56 and solstice_gh reset and shop records keep importing Marigot instruments #5319 and #5322? | [nexus56 reset](../../../areas/zon/nexus56.zon#L1369), [nexus56 shop](../../../areas/shp/nexus56.shp#L451), [solstice_gh resets](../../../areas/zon/solstice_gh.zon#L30), [solstice_gh shop](../../../areas/shp/solstice_gh.shp#L1), [source-only distinction](#reset-and-cross-area-dependencies) | Preserve the exact source references and Marigot object ownership; do not treat these unregistered files as active resets or operating shops. |
| 16 | What stock-renewal and daily availability should builders promise for Marigot's mode-2 reset population and imported shop goods? | [reset header](../../../areas/zon/marigot.zon#L1), [shop and reset boundary](#residents-and-shops) | Tie any daily claim to actual reset admission, caps, shop stock, and active READY accounting; mode 2 alone does not guarantee present stock. |
| 17 | Should the justice hall and holding cell lead to any player-facing legal or rescue outcome? | [justice and jail rooms](../../../areas/wld/marigot.wld#L121), [door states](../../../areas/zon/marigot.zon#L8) | Keep civic scenery outside completion credit until a consent and beneficiary policy, actor state, and durable result are defined. |
| 18 | Should the memorial, cemetery, and grieving residents form a specific family or remembrance story? | [memorial](../../../areas/wld/marigot.wld#L2070), [cemetery](../../../areas/wld/marigot.wld#L2091), [residents](../../../areas/mob/marigot.mob#L1) | Let builders name the people and lasting remembrance outcome; keep general room prose and visits informational. |
| 19 | Are the lake, underwater rooms, forest, and grove intended to support a local objective beyond environmental exploration? | [environmental story groups](#environmental-story), [Mullet and grove context](../../../areas/wld/marigot.wld#L1548) | Preserve exploration as optional context; add a contract only after a precise source, recipient, and accepted result are selected. |
| 20 | Should the sidecar's broad promise to uncover local requests be narrowed or expanded to match the two conversation records and zero Q/QA contracts? | [sidecar introduction](../../../areas/story/marigot.story.json#L6), [QST inventory](#story-coverage-state), [coverage row](ZONE_COVERAGE_REGISTER.csv#L171) | Align the sidecar wording with the actual mapped scope, or author explicit requests and outcomes before describing them as available. |

Checkpoint 273 validation: all twenty questions link to the native sidecar, QST, WLD, MOB, OBJ, ZON, SHP, shared conversation code, and source review. The title scan and bounded same-stem history now cover all 442 .zon files and the followed Marigot source paths. No area or gameplay source changed; no tests, build, server, database operation, migration, or operational script was run.

## Checkpoint 943: bounded typed and compiled-code closure

A repository-wide exact-VNUM pass expands the source crosswalk beyond Marigot's local reset and shop records. The active `spshold.zon` has a `G` reset for Marigot object #5308 at line 97. Current but unregistered Nexus56 has a `G` reset for Marigot instrument #5319 at `nexus56.zon` line 1369 and shop #83037 lists that product at `nexus56.shp` line 457. Current but unregistered Solstice GH has `G` resets for #5322 and #5319 at `solstice_gh.zon` lines 30 and 33; shop #29100 lists both objects at `solstice_gh.shp` lines 15 and 18. These two candidate packages are source-only consumers: their reset and stock records do not establish active resets or live shops. The product VNUMs resolve to Marigot's local instrument prototypes, preserving Marigot as the object owner.

The bounded source sweep found no external same-type WLD, MOB, or OBJ prototype definitions colliding with Marigot's current room, mobile, or object identities. No external QST numeric operand matched those ranges; a typed walk of story-sidecar VNUM fields found no external hit. No external OBJ type-25 or type-29 destination targets a Marigot room. The active WLD crossings and the already-counted source-only Surf and Underworld2 approaches remain the CP273 graph evidence; these typed references add no WLD edge or connectivity count.

Compiled C/C++ source does contain additional fixed Marigot destinations. `chaos_port` names #5302 as “marigot centaur” and resolves that entry with `real_room` ([destination table](../../../src/combat/chaos.c#L20), [resolver](../../../src/combat/chaos.c#L64)). `HOME_MARIGOT` is home index 15 and the `hometown[]` table maps that entry to #5302 ([home constant](../../../src/core/defines.h#L1108), [home table](../../../src/core/constant.c#L1735)). The Fort Marigot justice row names report room #5307, guard rooms #5319/#5306/#5371, guard mobile #5340, and jail room #5308 ([justice table](../../../src/combat/justice.c#L84)). The `guild_locations` row contains #5302, #5336, #5374, #5343, and #5385 ([guild table](../../../src/core/constant.c#L1956)). These are compiled table bindings and destinations; this source review does not establish live invocation, access policy, or safe travel.

Several numeric mentions are not active bindings. The legacy Verzanan mobile assignments that reuse Marigot mobile VNUMs are inside `#if 0` in `specs.assign.c` ([disabled block](../../../src/specs/specs.assign.c#L626)); related routines in `specs.verzanan.c` therefore remain dormant/unbound. The ticket table in `specs.mobile.c` names rooms #5313/#5399 and item #5341, but `ticket_taker` has only a declaration and definition, no assignment, and no current `areas/obj` definition for #5341 ([table and routine](../../../src/specs/specs.mobile.c#L783), [declaration](../../../src/world/specs.prototypes.h#L476)). Neither is counted as an active consumer.

This closes the bounded repository source pass. Private editor projects, generated exports, deployment manifests, persisted player references, loader behavior, shop operation, and live access remain unqualified. Keep Marigot **DO NOT MOVE** and retain the existing source identities until those external states and remaining owner decisions are resolved.

### Builder question 21

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 21 | Should the compiled Chaos Port, hometown, justice, and guild destinations continue to point at their current Fort Marigot rooms and mobile? | [Chaos Port destination](../../../src/combat/chaos.c#L20), [hometown mapping](../../../src/core/constant.c#L1735), [justice row](../../../src/combat/justice.c#L84), [guild row](../../../src/core/constant.c#L1956) | Preserve the current bindings while the code and area owners confirm intended entry, eligibility, and service behavior; do not infer live access or safe return from the constants alone. |
