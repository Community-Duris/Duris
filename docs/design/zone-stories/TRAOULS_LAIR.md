# Traoul's Lair: source-comprehensive zone dossier

Traoul's Lair is the royal section at the end of Truktakyagyo's main avenue.
Its authored story is a fortified goblin settlement: guards and trainees,
servant housing, barracks, hidden tunnels, a royal foyer, and Traoul's chamber.
The [repository-wide audit supplement](../quest-story-followups/TRAOUL_AUDIT_REVIEW.md)
reconciles its cross-area doorway, reset-state locks, typed references, title,
history, and retention evidence.

The active [WLD](../../../areas/wld/traoul.wld) contains 44 rooms and 91
resolving exits. Its authored room graph reaches and returns from all 44 rooms
when every exit is treated as passable; that includes closed, locked, and
secret exits. The [ZON](../../../areas/zon/traoul.zon) restores 14 door states
as closed, locked, or secret. The route graph does not prove that the keys or
skills needed for those routes are currently available.

`areas/qst/AREA` lists `traoul`, but `areas/qst/traoul.qst` is absent. There is
no local QST contract, shop file, or `traoul.story.json` sidecar in the source
tree. The WLD, mobile, item, and reset descriptions do not by themselves record
a quest, rescue, allegiance change, or persistent result.

## Place and authored story

Room #7000 begins with a paved avenue described as finer than the main city,
with a large gate in front of the king's lair. The adjoining Truk room #1622
calls the gate the end of Truktakyagyo's avenue and describes Traoul's large
residence and its servants and slaves. The #7001 courtyard displays a violent,
flattering statue of the king. Its north gate is described as leading to an
inner area reserved for privileged goblins. ([Traoul entrance](../../../areas/wld/traoul.wld#L1),
[#7001 courtyard](../../../areas/wld/traoul.wld#L21), [Truk boundary](../../../areas/wld/truk.wld#L435))

The outer circuit separates a guard walkway, guard barracks and training
ground from a servant walkway, quarters, the master servant's office, and a
storage room. The warmaster trains goblin guards. The master servant assigns
work and prevents slaves from escaping; a separate slave goblin is chained in
Traoul's chamber. These are authored descriptions, not player-facing trainer,
assignment, release, or rescue contracts. ([guard route and training](../../../areas/wld/traoul.wld#L53),
[servant rooms](../../../areas/wld/traoul.wld#L163), [master servant](../../../areas/mob/traoul.mob#L46),
[slave prototype](../../../areas/mob/traoul.mob#L191))

The inner foyer and two royal floors are carved into the cave wall and covered
with carvings of Traoul and earlier kings. The inner chamber is described as
the settlement's richest room, decorated with raid spoils, a throne, and a pet
pen. King Traoul's mobile description calls him an assassin and magic user who
charms goblins or kills those he cannot charm. The pet cildabrin is described
as serving and protecting him under his magic. Neither mobile has a native
quest block or a selected room-specific special assignment. ([foyer](../../../areas/wld/traoul.wld#L495),
[royal chamber](../../../areas/wld/traoul.wld#L834), [Traoul mobile](../../../areas/mob/traoul.mob#L206),
[pet mobile](../../../areas/mob/traoul.mob#L176))

## Room atlas

| Rooms | Authored place |
| --- | --- |
| #7000 | Entrance road and gate to the courtyard |
| #7001 | Large courtyard and statue of the king |
| #7002–#7006 | Five sections of the guard's walkway around the plateau |
| #7007–#7011 | Servant's walkway and its side streets |
| #7012–#7013 | Servants' quarters |
| #7014–#7015 | Master servant's office and upstairs room |
| #7016–#7017 | Storage room and hidden tunnel entrance |
| #7018–#7022 | Guard barracks |
| #7023 | Training grounds |
| #7024 | Inner foyer |
| #7025 | Warmaster's room |
| #7026–#7038 | Connected chain of rough secret tunnels |
| #7039 | Side passage in the secret tunnel |
| #7040 | Small storage room and alternate west entrance to the foyer |
| #7041–#7043 | Lower floor, upper floor, and Traoul's inner chamber |

## Routes, doors, and keys

The single active area boundary is reciprocal: Truk #1622 north leads to
Traoul #7000, and #7000 south returns to #1622. The outer gate from #7000 north
to courtyard #7001 resets closed and locked. Its key is #1609, a bright red
key defined and reset by Truk: the key is carried by gate guard #1601 in room
#1622. This is a typed cross-area entrance dependency; it preserves Truk's
ownership of that key. ([pair register](../quest-story-followups/WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L667),
[reverse pair](../quest-story-followups/WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L676),
[gate reset](../../../areas/zon/traoul.zon#L8), [key item](../../../areas/obj/truk.obj#L85),
[Truk guard and key reset](../../../areas/zon/truk.zon#L82))

The courtyard's north gate leads to foyer #7024 and resets closed and locked
with key #7009. The key is carried by a hulking guard placed in that foyer, on
the far side of the same gate. A raw map path continues from the foyer through
the lower and upper floors to Traoul's chamber. The upper-floor door also
resets closed and locked, but its exit key field is zero; its unlocking path
and any pick or knock alternative remain a live-qualification question. ([gate
exits](../../../areas/wld/traoul.wld#L33), [gate resets](../../../areas/zon/traoul.zon#L12),
[foyer guard reset](../../../areas/zon/traoul.zon#L101), [upper-floor door](../../../areas/wld/traoul.wld#L814),
[door resets](../../../areas/zon/traoul.zon#L56))

Two concealed approaches converge on the long secret tunnel. The storage room
#7016 opens east through a closed secret door to #7017. The down trapdoor from
#7017 to #7038 is locked and names key #7001. The other approach runs west from
the training grounds #7023 through a secret, locked door to #7026 and also
names #7001. The tunnel reaches side passage #7039, whose closed secret
trapdoor leads up to #7040; #7040 has an open west exit to the inner foyer.
These authored branches would bypass the foyer gate, but both branches first
depend on a #7001 lock. ([storage and first tunnel](../../../areas/wld/traoul.wld#L342),
[training tunnel](../../../areas/wld/traoul.wld#L475), [side passage](../../../areas/wld/traoul.wld#L759),
[tunnel and foyer resets](../../../areas/zon/traoul.zon#L24))

Prototype #7001 is a moldy steel key referenced by four reciprocal WLD door
records, but no active local or external ZON reset, shop, QST item row, or
compiled placement assigns it. The shady goblin in #7017 has no key inventory
reset. This leaves its intended source unresolved; do not claim the secret
route is available from authored stock without qualifying a key or a
skill-based alternative. ([key prototype](../../../areas/obj/traoul.obj#L11),
[shady goblin reset](../../../areas/zon/traoul.zon#L91),
[hidden tunnel lock](../../../areas/wld/traoul.wld#L378),
[training entrance](../../../areas/wld/traoul.wld#L493),
[training reverse](../../../areas/wld/traoul.wld#L545),
[tunnel return](../../../areas/wld/traoul.wld#L757))

## Cast, placements, and items

The [mobile catalog](../../../areas/mob/traoul.mob) defines 15 prototypes;
the ZON has 38 mobile reset commands. Every prototype is placed at least once.
Their source roles and reset placements are:

| VNUM | Prototype and authored role | Reset placement |
| --- | --- | --- |
| #7000 | Hulking royal gate guard | Two guards at entrance #7000 ([definition](../../../areas/mob/traoul.mob#L1), [resets](../../../areas/zon/traoul.zon#L70)) |
| #7001 | Shady, anxious goblin | Hidden tunnel #7017; no item reset ([definition](../../../areas/mob/traoul.mob#L15), [reset](../../../areas/zon/traoul.zon#L91)) |
| #7002 | Warmaster who trains guards | Warmaster's room #7025 ([definition](../../../areas/mob/traoul.mob#L30), [reset](../../../areas/zon/traoul.zon#L104)) |
| #7003 | Master servant who organizes the king's orders | Room #7015 ([definition](../../../areas/mob/traoul.mob#L46), [reset](../../../areas/zon/traoul.zon#L90)) |
| #7004 | Hulking royal guard | Inner foyer #7024 ([definition](../../../areas/mob/traoul.mob#L61), [reset](../../../areas/zon/traoul.zon#L101)) |
| #7005 | Young trainee practicing wildly | Four trainees at training ground #7023 ([definition](../../../areas/mob/traoul.mob#L75), [resets](../../../areas/zon/traoul.zon#L96)) |
| #7006 | Large hunting spider | Secret tunnel rooms #7031, #7033, and twice at #7039 ([definition](../../../areas/mob/traoul.mob#L90), [resets](../../../areas/zon/traoul.zon#L108)) |
| #7007 | Guard trainee polishing a practice weapon | Guard barracks #7020, #7021, and twice at #7022 ([definition](../../../areas/mob/traoul.mob#L104), [resets](../../../areas/zon/traoul.zon#L92)) |
| #7008 | Busy servant following orders | Walkways and inner foyer ([definition](../../../areas/mob/traoul.mob#L119), [resets](../../../areas/zon/traoul.zon#L75)) |
| #7009 | Tired, sleeping servant | Servants' quarters #7013, three reset commands ([definition](../../../areas/mob/traoul.mob#L133), [resets](../../../areas/zon/traoul.zon#L87)) |
| #7010 | Patrolling goblin guard | Courtyard-side walkways ([definition](../../../areas/mob/traoul.mob#L148), [resets](../../../areas/zon/traoul.zon#L74)) |
| #7011 | Elite king's guard | Upper floor #7042 ([definition](../../../areas/mob/traoul.mob#L162), [reset](../../../areas/zon/traoul.zon#L112)) |
| #7012 | Traoul's cildabrin pet | Inner chamber #7043 ([definition](../../../areas/mob/traoul.mob#L176), [reset](../../../areas/zon/traoul.zon#L114)) |
| #7013 | Chained goblin slave attending the king | Two at inner chamber #7043 ([definition](../../../areas/mob/traoul.mob#L191), [resets](../../../areas/zon/traoul.zon#L115)) |
| #7014 | King Traoul, described as assassin and magic user | Inner chamber #7043 ([definition](../../../areas/mob/traoul.mob#L206), [reset](../../../areas/zon/traoul.zon#L117)) |

These are ordinary resets; they do not record personal allegiance,
emancipation, player training, or boss completion.

The [object catalog](../../../areas/obj/traoul.obj) defines 15 prototypes. Its
ZON uses two room-level object placements, two nested placements, three carried
items, and nine equipment placements:

| VNUM | Prototype and authored role | Reset placement or typed use |
| --- | --- | --- |
| #7000 | Single-bladed battle axe | Equipped on five guards ([definition](../../../areas/obj/traoul.obj#L1), [resets](../../../areas/zon/traoul.zon#L70)) |
| #7001 | Moldy steel key | Not reset; named by the tunnel/training locks ([definition](../../../areas/obj/traoul.obj#L11), [door](../../../areas/wld/traoul.wld#L378)) |
| #7002 | Polished stone ring | No local or external reset placement ([definition](../../../areas/obj/traoul.obj#L24)) |
| #7003 | Metal brace set into the wall | Room-level container reset in warmaster's room #7025; names key #7004 ([definition](../../../areas/obj/traoul.obj#L41), [reset](../../../areas/zon/traoul.zon#L65)) |
| #7004 | Small rusty key | Carried by the warmaster; matches brace #7003 ([definition](../../../areas/obj/traoul.obj#L54), [reset](../../../areas/zon/traoul.zon#L106)) |
| #7005 | Large desk | Room-level container reset in room #7025; names key #7009 ([definition](../../../areas/obj/traoul.obj#L62), [reset](../../../areas/zon/traoul.zon#L67)) |
| #7006 | Dark stone key | Carried by the warmaster; no current door/container key field names it ([definition](../../../areas/obj/traoul.obj#L70), [reset](../../../areas/zon/traoul.zon#L107)) |
| #7007 | Parchment with unexplained symbols | Nested in desk #7005 ([definition](../../../areas/obj/traoul.obj#L78), [reset](../../../areas/zon/traoul.zon#L68)) |
| #7008 | Polished demilune polearm | Nested in brace #7003 ([definition](../../../areas/obj/traoul.obj#L90), [reset](../../../areas/zon/traoul.zon#L66)) |
| #7009 | Shiny metal key | Carried by the foyer guard; key for the courtyard gate and desk ([definition](../../../areas/obj/traoul.obj#L107), [gate](../../../areas/wld/traoul.wld#L33), [reset](../../../areas/zon/traoul.zon#L103)) |
| #7010 | Jagged long sword | Equipped on the warmaster ([definition](../../../areas/obj/traoul.obj#L115), [reset](../../../areas/zon/traoul.zon#L105)) |
| #7011 | Razor-sharp sword | No local or external reset placement ([definition](../../../areas/obj/traoul.obj#L127)) |
| #7012 | Polished katar | Equipped on the elite guard ([definition](../../../areas/obj/traoul.obj#L139), [reset](../../../areas/zon/traoul.zon#L113)) |
| #7013 | Golden crown of Traoul | Equipped on the king ([definition](../../../areas/obj/traoul.obj#L151), [reset](../../../areas/zon/traoul.zon#L118)) |
| #7014 | Glowing blue-steel dagger | Equipped on the king ([definition](../../../areas/obj/traoul.obj#L163), [reset](../../../areas/zon/traoul.zon#L119)) |

Prototype definitions do not guarantee live stock. In particular, key #7001,
ring #7002, and sword #7011 have no reset placement, and key #7006 has no
matched local WLD/container lock.

Three prototypes have no placement in the local reset file or any typed
external reset: moldy key #7001, polished ring #7002, and razor sword #7011.
Key #7001 has the noted door references. Dark stone key #7006 is placed on the
warmaster, but no current WLD door or container key field uses it. Keep these
facts distinct from live stock; a prototype definition is not a placement.

## Registry, dependencies, and qualification

There are no native quest giver prototypes or QST contracts. The QST manifest
entry and missing file should be reconciled with the area owner; neither
supports inventing a request or reward. There is no local shop record or story
sidecar. The only exact external QST text match to object-range #7000 is
`R E 7000`, an experience reward in Surface Keeps, not an item reference. The
corresponding sidecar's `E:7000` token is also experience. No external active
reset or shop issues a local mobile/object prototype, no outside type-25 portal
targets a Traoul room, and no fixed compiled room/mobile/object binding uses
these VNUMs. ([QST manifest row](../../../areas/qst/AREA#L36),
[Surface Keeps reward](../../../areas/qst/surfacekeeps.qst#L373),
[Surface Keeps story mapping](../../../areas/story/surfacekeeps.story.json#L522))

The active normalized title “Traoul's Lair” is unique among 442 current ZON
titles. Truk's “Truktakyagyo” title is a connected neighbor, not another owner
of the Lair identity. Repository-wide current WLD/MOB/OBJ scans found no exact
room or prototype VNUM collision. Followed same-stem file history records the
2009 tree move and later 2012–2015 maintenance; no alternate-stem predecessor,
rename, or whole-zone replacement is established in those bounded paths. Keep
active Traoul **DO NOT MOVE** and preserve the reciprocal Truk entry and the
cross-area key ownership. No static review establishes live access, door
availability, safe arrival, or a played outcome.

## Explicit builder questions and recommended resolutions

| # | Source-backed question | Conservative recommendation |
| --- | --- | --- |
| 1 | Is the `traoul` QST/AREA entry intentional when no `traoul.qst` file exists? ([manifest](../../../areas/qst/AREA#L36)) | Reconcile the manifest with the area owner. Do not infer or create a native quest contract from the room and mobile prose alone. |
| 2 | Should defeating King Traoul produce a persistent story outcome? ([king mobile](../../../areas/mob/traoul.mob#L206), [chamber](../../../areas/wld/traoul.wld#L834)) | Keep combat, crown/dagger loot, and any future story receipt separate until an explicit contract and persistence path are designed. |
| 3 | Is bright red key #1609 the intended way through the first gate? ([gate](../../../areas/wld/traoul.wld#L1), [key holder reset](../../../areas/zon/truk.zon#L82)) | Preserve Truk's key and the existing area boundary; qualify actual acquisition and unlock before promising entry. |
| 4 | Is the #7009 courtyard key intentionally carried by a guard placed behind the gate it opens? ([gate](../../../areas/wld/traoul.wld#L33), [guard/key reset](../../../areas/zon/traoul.zon#L100)) | Keep the current identity and placement visible in the design record. Verify pick/knock and secret-route alternatives before moving or duplicating the key. |
| 5 | Are the secret tunnels intended to bypass the #7009 foyer gate? ([tunnel route](../../../areas/wld/traoul.wld#L342), [foyer exit](../../../areas/wld/traoul.wld#L776)) | Treat them as authored map alternatives, but do not claim access until the #7001 locks and actual search/open sequence are resolved. |
| 6 | Should the unplaced moldy key #7001 be reset on the shady goblin or elsewhere? ([key](../../../areas/obj/traoul.obj#L11), [goblin placement](../../../areas/zon/traoul.zon#L91)) | Confirm the intended source before adding a drop or reset. Until then, label both tunnel branches as conditionally accessible. |
| 7 | What does the warmaster's dark stone key #7006 open? ([key](../../../areas/obj/traoul.obj#L70), [warmaster reset](../../../areas/zon/traoul.zon#L104)) | Keep the current carried item while its purpose is checked; no current local door or container names it as a key. |
| 8 | Is key #7004 meant to unlock the wall brace containing demilune #7008? ([brace/key](../../../areas/obj/traoul.obj#L41), [child reset](../../../areas/zon/traoul.zon#L65)) | Preserve the typed container/key/child relationship, and report the weapon as nested stock rather than loose room loot. |
| 9 | Is shiny key #7009 intended to open both the foyer gate and warmaster's desk? ([gate](../../../areas/wld/traoul.wld#L33), [desk](../../../areas/obj/traoul.obj#L62)) | Preserve one key identity and both source links. Do not clone the key or mark the desk open as a reward. |
| 10 | Do the symbols on desk parchment #7007 have a deciphering action or outcome? ([parchment](../../../areas/obj/traoul.obj#L78), [nested placement](../../../areas/zon/traoul.zon#L67)) | Treat the present text as atmosphere until a source-backed puzzle action, output, and retry rule exist. |
| 11 | How is the locked upper-floor door with no WLD key field meant to open? ([door](../../../areas/wld/traoul.wld#L823), [reset](../../../areas/zon/traoul.zon#L56)) | Keep the reset state explicit and qualify ordinary keyless lock interactions; do not promise a found key that is not configured. |
| 12 | Should searching reveal the storage room's secret door and the tunnel trapdoors? ([secret door](../../../areas/wld/traoul.wld#L351), [searchable tunnel exits](../../../areas/wld/traoul.wld#L771)) | Preserve the secret/closed states. Count actual discovery and opening separately from the static presence of the exit. |
| 13 | Do the 14 door reset rows describe the intended reset-state cycle for every gate and secret passage? ([door resets](../../../areas/zon/traoul.zon#L8)) | Treat reset comments and current state as separate evidence; confirm the live reset timing and state before claiming a stable route. |
| 14 | Are unplaced ring #7002 and razor sword #7011 meant to be unavailable prototypes or missing loot? ([ring](../../../areas/obj/traoul.obj#L24), [sword](../../../areas/obj/traoul.obj#L127)) | Keep them as defined prototypes with no placement until their ownership and intended source are confirmed. |
| 15 | Does the chained goblin slave imply a rescue or emancipation objective? ([mobile](../../../areas/mob/traoul.mob#L191), [king room](../../../areas/wld/traoul.wld#L834)) | Describe the captive state as authored. Do not report a rescue or liberation outcome without a recorded action and persistent result. |
| 16 | Does the cildabrin description imply that Traoul can charm or summon it during play? ([pet](../../../areas/mob/traoul.mob#L176), [king](../../../areas/mob/traoul.mob#L206)) | Keep the prose separate from an active mobile special or summon callback; no area-specific code binding was found. |
| 17 | Is the warmaster a player trainer or service provider? ([warmaster](../../../areas/mob/traoul.mob#L30), [training grounds](../../../areas/wld/traoul.wld#L475)) | Treat the training as goblin-setting text; do not promise player training or a skill transaction without a selected handler. |
| 18 | Should the king's guard, servant, and spider reset groups be read as stable access or population guarantees? ([reset groups](../../../areas/zon/traoul.zon#L70)) | Preserve the authored placement identities and quantities; qualify reset timing, mobile movement, survival, and key custody in play. |
| 19 | Is a local shop, addressed conversation, or quest service intended despite the missing SHP/QST files? ([QST manifest](../../../areas/qst/AREA#L36)) | Keep the current absence accurately documented and do not invent purchases, dialogue, or rewards from descriptive text. |
| 20 | What evidence is needed before calling the route and lair story playable end to end? ([raw graph](../../../areas/wld/traoul.wld#L1), [reset states](../../../areas/zon/traoul.zon#L8)) | Qualify the Truk key, courtyard key, hidden doors, #7001 source, keyless lock options, king/pet/captive interaction, reset behavior, and any persistence in an actual run. |
