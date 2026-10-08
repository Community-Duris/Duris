# Plane of Fire, Brass: comprehensive source map

Reviewed October 4, 2026. Zone 1390, brass; roadmap priority 59.
**Source review is comprehensive; live gameplay qualification remains pending.**
Active, ready accounting is mandatory for discovery, visible encounters,
journals and new achievement/daily credit. Frozen obligation recovery remains
separate.

The [schema-three journal](../../../areas/story/brass.story.json) covers all
seven native exchanges as four named stories, one equipment service and two
exclusions, with 25 contacts and eighteen optional carried-item checks.
Four-source, three-head and six-head collections have multiple preparation
steps but each retains one accepted outcome. Collectible coins, wallet money,
quantity-two rewards and competing proof ownership remain distinct.

## Reviewed evidence

- All twenty [native blocks](../../../areas/qst/brass.qst): twelve M, one MA,
  six Q and one QA. Twelve addressed families cover seven recipient kinds;
  Yodono's additional qc_action family is internal ambient metadata. Its
  missing interval does not satisfy shared qc_action-with-integer dispatch.
  Questions do not award learned-topic achievements.
- All 357 [rooms](../../../areas/wld/brass.wld), in registry range
  139000–139357: 265 exact prose groups, nine headers, nine non-exit metadata
  groups and 193 exit families. Full prose, headers, metadata and exits were
  reviewed. Unused 139248 has no reviewed active reference; archived
  brass-old-1/generated world files do not establish current progression.
  Header 139357 1 0 25 35 4 retains reset mode one.
- All 147 [mobiles](../../../areas/mob/brass.mob), 170
  [objects](../../../areas/obj/brass.obj), eighteen valid new-format
  [shops](../../../areas/shp/brass.shp), and 779
  [reset commands](../../../areas/zon/brass.zon), in 452 exact families:
  M408/G133/D112/E78/F25/O13/R9/P1. R loads mounts for the previous M/F actor,
  rather than a random actor replacement. Following, riding, slots, caps,
  probabilities and actual admission matter.
- Active referenced reset prototypes and exit targets resolve. No local
  ACT_TEACHER prototype or literal local mobile special establishes additional
  teaching or quest terminals. Ordinary protector behavior and descriptive
  guild titles are not guild-member admission receipts.
- Local assignments are holy mace 139004, hiding cloak 139138 and
  Flaming Dragon Inn 139078. Imported entrance golems 25400/25401 use
  guild_guard and unblock_on_death; the latter reads
  [Plane of Fire control source](../../../areas/qst/plane_fire_one.qst#L1).
  Imported magma actors, tome, stat pool, artifacts, rod, flamberge and epic
  monolith were traced to their separate shared behavior. No local switch/
  portal prototype or local trigger source adds an access quest.
  The world.trg file is absent.
- Bounded global reset/producer scans reviewed all eighteen native item
  input/output kinds; no foreign native Q/QA consumers were found for them.
  The dying djinn has no active M/F/R placement. Entrance 139000 joins
  Plane of Fire room 25455 reciprocally. These findings do not certify every
  dynamically named script, future integration or live supply.
- Shared offering selection, quantity-aware reward/recovery, currency rewards,
  accounting admission, equipment replacement, wandering, perception, barriers,
  effective doors, key breakage, heat, rental and item effects were traced.
  The [generated review index](../../reference/zone-story-audits/brass.md)
  supplies exact evidence navigation.

## Progression stories and exact acceptance

| Recipient and native block | Exact offering → outcome | Journal treatment |
| --- | --- | --- |
| Herl 139083, Q22 | Blood 139128 → C1000000, or 1000 platinum | Named rare-beast request; regular shop transactions remain separate |
| Dying djinn 139110, Q33 | One vial 139018 → no reward, response or disappearance | Excluded unfinished interaction; no active placement or verified rescue |
| Tax collector 139119, QA89 | Surtr coin 139028 + Imix coin 139031 + quill 139026 + quartz 139033 → Palace Gate Key 139070 | One four-source story; both coins are collectible objects |
| Armorer 139121, Q118 | Elder scales 139127 + C7500000 → same scales kind + C7500000 | Excluded narrated refusal; coin-offering guard retained |
| Armorer 139121, Q131 | Elder scales 139127 + ancient scales 139142 + C7500000 → bracer 139137 | Independent equipment service; mixed fee guarded |
| Yodono 139124, Q159 | Caltoon 139011 + arch-magi 139016 + Ornon 139017 → two separate vials 139018 | One story with quantity-two reward; no second-task credit |
| Spy 139132, Q182 | Artoon 139144 + arch-magi 139016 + Ornon 139017 + high priest 139139 + Wrinyen 139140 + Hakim 139141 → sash 139143; D1 | One six-head story followed by recipient departure |

The collector's coins are ITEM_TREASURE objects, unlike wallet C and money
piles. His item-only offering does not need a hypothetical currency-input
guard. Herl's C is a reward, not a fee: durable reward recovery supports
currency outputs. The bracer and smaller refusal require a 7500000-copper fee
and retain their present unsupported durable-offering guard.

The loader prepends completion blocks, so the armorer's later complete
commission precedes the smaller refusal in dispatch. His prose asks for
ancient scales first and describes legacy incremental delivery to an NPC.
Current durable ownership gathers exact player-owned loose items as a complete
bundle; coin goals remain unsupported. Do not invent a required refusal
receipt, accept partial paid escrow, or equate one offered scale with success.

## Exact sources, quantities and competing materials

| Item kind | Declared source | Consequence |
| --- | --- | --- |
| Blood 139128 | Ancient chimaera 139144 at zoo cage 139091, G100/cap one | Supplied blood fits; no personal kill or recovery requirement |
| Surtr coin 139028 | Priest 139038 at shrine 139178, G100/cap one | Followers and neighboring priests are separate owners |
| Imix coin 139031 | Priest 139045 at shrine 139246, G100/cap one | Different collectible from Surtr coin and wallet currency |
| Quill 139026 | Dock master 139036 at 139192, E slot 18 after dagger 139025 | Shared E moves the dagger to carrying before equipping the quill; no missing-quill defect |
| Quartz 139033 | O at Magma Lake 139241, cap one | Quartz armor/gems do not match |
| Palace key 139070 | Collector's accepted exchange only | Break value 100; receipt does not prove current key or passage |
| Caltoon head 139011 | Caltoon 139118 at 139041, G100/cap one | Caltoon differs from palace arch-magi |
| Arch-magi head 139016 | Palace arch-magi 139009 at 139299, G100/cap one | Both head requests consume this kind |
| Ornon head 139017 | Ornon 139000 at 139292, G100/cap one | Both head requests consume this kind |
| Artoon head 139144 | Artoon 139010 at 139286, G100/cap one | One of six distinct spy inputs |
| High-priest head 139139 | Priest 139012 at 139330, G100/cap one | City cleric El'Quilan 139004 is different |
| Wrinyen head 139140 | Wrinyen 139089 at 139336, G100/cap one | Armory-key possession is separate |
| Hakim head 139141 | Hakim 139092 at 139344, G100/cap one | Mounted/following creatures are separate owners |
| Elder scales 139127 | Elder hydra 139145 at zoo cage 139085, G80/cap one | Different from ancient scales |
| Ancient scales 139142 | Pyrohydra 139102 at rare room 139247, M60 and G100/cap one | Qualify admitted birth, movement and supply |
| Vials 139018 | Yodono only; two receive entries | Two reward roots, one receipt; spell 267 is elemental form |
| Bracer 139137 / sash 139143 | Complete armorer / spy exchanges | Service and story retain different achievement policy |

One physical arch-magi or Ornon head cannot fund both requests. A prior
Yodono receipt does not replenish heads or identify a new object's source.
Native acceptance allows supplied exact kinds; handoff must not fabricate
personal combat, original recovery, first acquisition or producer history.

The shared reward routine distinguishes duplicate receive ordinals in source
IDs. Two identical-kind vials remain two recoverable grants under one accepted
Yodono transaction. Potion use or the ambiguous narrated second task cannot
add another story outcome. The potion is elemental form, not healing.

## Actual access and world constraints

Entrance 139000 north is reset closed/blocked (D9). Golem 25400 blocks north
for ordinary players at its birth room. Death of 25401 clears EX_BLOCKED
through qc_unblock 139000 north. It neither defeats the first golem nor opens
the door. Future successful-defeat, shared-barrier, open and surviving-arrival
events need separate personal/group attribution and reset episodes.

The collector is at office 139020, not temple room 139119: mobile VNUMs are
not addresses. Palace gate 139140 north is locked by D2 and uses 139070;
its loader door-kind bits make it pickproof. Reverse 139249 south is closed
by D1 with no key. Issuance, key use/breakage, unlocking, opening and arrival
are separate. Actual resets/current shared state determine access; raw door
kind is not initial locked state.

Green key 139130 comes from the first royal guard 139091 at hall 139283.
It opens forward 139283 north and 139284 east; reverse doors are closed,
not locked. Connected 139290/289/288/287 rooms reach Artoon from behind.
Purple key 139129 is carried by Ornon and locks 139291 east/139292 west;
the connected 139294/293 route reaches Ornon from the other side. Avoid false
self-key cycles or universal front-key prerequisites. Watch-tower key 139014
comes from Enlightened guard 139006 at 139207: bridge exit 139131 north is
open despite its configured key; tower 139207 north/139132 south has D2 locks.

Two blue-fire keys share names but not identity: 139021 from the first
personal-guard 139021 reset at 139307 opens 139311 north toward 139357;
139022 from the second at 139315 opens 139357 north toward the throne.
Sultan 139011 carries treasury key 139023 for 139312 north. Wrinyen's 139034
opens armory 139334 south and distinct slave-quarter doors at 139338/139339,
with asymmetric reverse states. Chief 139013 at 139351 carries skeleton
key 139135 for armoire 139134 at armory 139335. Its imported band 67244 belongs
to the container, not the chief or spy.

Rare room 139247 has outward north/west exits to 139302/139252 and no reviewed
ordinary incoming exit. Its non-sentinel red dragon M70, brass dragon M25 and
pyrohydra M60 may wander into the palace through admitted movement. It is
not a mandatory player destination. Spy M10 is at Magma Lake 139238; steam
spouts are dialogue imagery. Normal unforced active-accounting M admission
skips these lower chances; item resets need admitted durable generation.
Static declarations do not guarantee daily stock, perception or renewal.

Nearly all rooms are SECT_FIREPLANE. Shared firesector schedules exposure;
the event can strip ordinary protect-from-fire/fire-ward spells and damage
unprotected non-exempt characters. A fire-ward label or random elemental-form
potion does not certify survival. Inn 139078 has real assigned shared rental;
successful rental and persistence remain separate.

## Services, information and contextual stories

Eighteen shops have valid new-format records. Produced stock differs from
reset-carried stock. Shanaanaa's shop configures nine external note kinds:
Githzerai Stronghold, Apocalypse Castle, Gibberling King, Mazzolin, Shadamehr
Keep, Celestial Plane, Ultarium, Arachdrathos Guilds and Jotunheim.
Transparent Tower note 139149 is G70 carried stock rather than a produced
kind. Purchasing/reading notes does not prove their foreign route, password
or quest claims against today's active source. Attribute these leads and
verify each destination dossier.

Ordinary shops, bakery, bank imagery, customs, guild titles, shrine offerings,
meditation/scrying pools, prisoners, Sultan battle and treasure do not create
extra accepted native terminals. Builders must choose actual rescue, escort,
liberation, combat, knowledge or campaign closure events. No reviewed local
procedure implements the dying-djinn rescue.

Holy mace 139004 uses the maintained THARKUN_ARTIS combat branch, defined by
the compiled config. Worn cloak 139138 uses shared SAY hide, cooldown and
environment checks. Imported pool 60, band 67244, rod 67245, flamberge 430 and
monolith 360 retain separate stat/artifact/combat/epic effects. None is a
native story receipt. The earlier shared epic-node post-extraction/traversal
concern remains a separate qualification proposal; no new live crash or
procedure repair is claimed.

## Capability additions and qualification work

| Requirement found in Brass | Implementation/qualification proposal |
| --- | --- |
| Two controls on one entrance | Publish actor-specific defeat and actual barrier transition, preserve reset episode, then evaluate open and surviving arrival; separate shared state/personal credit |
| Four collectibles including coins | Allocate exact roots as one bundle; distinguish I collectibles from wallet C and preserve ownership/provenance |
| Three/six heads with shared kinds | Reserve each UID once; preserve supplied acceptance independently of original recovery and personal kill |
| Duplicate identical rewards | Freeze quantity and per-copy identity; replay/interrupt/recover both grants without a second outcome or lost vial |
| Rare hidden/holding-room actors | Qualify admitted initial/forced birth, allowed wandering, perception and renewable supply before repeatable dailies |
| Commission and refusal | Atomic exact items plus normalized-fee debit and fixed reward recovery; keep guard and do not require refusal history |
| Keys and alternate routes | Resolve effective reset/shared state; successful matching-key/breakage, unlock/open and arrival events without universal front-key history |
| Empty unplaced djinn branch | Builder selects placement, accepted endpoint and safe potion/allocation semantics before a separate native repair and rescue/second-task fixtures |
| External adventure notes | Attribute clue/version and validate destination; learned-topic receipts require successful replies/reads, never attempted aliases |
| Fire/item effects | Qualify actual predicate, target identity, effect and survival; labels are insufficient |

Source assertions and actual C++ journal journeys cover exact I/C distinction,
seven bindings, quantity-two vials, competing heads, loose/equipped input,
optional key guidance, service/exclusion evidence without achievement
inflation, read-only rendering, replay and cold recovery. Injected historical
service/exclusion receipts do not qualify current fees or rescue behavior.

Live admitted placement/generation, GET/handoff, hidden perception, wandering,
gate/door/key use and breakage, heat/combat/effects, fee settlement, retirement,
renewal and played persistence remain unqualified. No DB/account/server
operation, migration, deployment or merge was performed.

## Actual shipped repair and balanced pending findings

**Separate native fix [d18758098](https://github.com/Community-Duris/Duris/commit/d18758098083b28463279c130a3e7ec24bb48b24):**
the east exit at Imix Avenue room 139017 previously said west. It now says
east, matching D1 to 139016 and reciprocal D3. Exactly one native description
word changes. The original color-normalized clue fails the focused regression;
repaired source and exact native bytes pass. Live LOOK/traversal is unqualified.

**News:** “Brass's eastern Imix Avenue exit now describes its actual direction.”

Pending proposals are separate from that shipped repair:

- The dying djinn lacks placement and meaningful response/reward/closure.
  Yodono promises two tasks, but only the three-head request has a verified
  reward. Confirm rescue or other intent before changing placement, effect,
  reward or objective code; cold-blue-flame wording does not imply healing.
- Yodono's qc_action lacks an interval. Confirm ambient frequency and add an
  isolated timed-dispatch fixture before a native fix.
- Several descriptions reflect old directions/shop names. Room 139075
  describes both guild and bank to the west although the bank exit is east;
  139066 misdescribes its intersection. Room 139073 lacks a south exit toward
  139074 despite the road narrative. Confirm text versus topology intent
  before a separate bounded repair.
- Chimaera's “grey bird” conflicts with its grey-bearded body; the brass
  dragon copies the red-dragon body; Groyana's gnome describes a female
  efreeti. These are wording proposals, not proof of wrong quest identity,
  changed combat or a repaired NPC.
- The inactive alternative holy-mace branch ends with incomplete enclosing
  scopes before its preprocessor boundary. The maintained THARKUN_ARTIS
  branch is defined and builds successfully. Confirm whether alternate
  configurations are supported, then use a specific compile fixture and
  isolated repair; no active combat failure or player-facing fix is claimed.
- Preserve outward-only rare-room design pending actual wandering/supply
  qualification: it may deliberately disperse rare actors. Do not add an
  entrance or force spawns solely because a birth room has no incoming exit.
  Unused 139248 and valid shop headers are not proved defects.

Catalog: **80 journals / 1630 achievement units / 1463 potential daily units /
2208 rows**. Two exclusions remove two generic outcomes; the bracer service
removes one achievement. The empty djinn also loses its former potential
daily slot. All seven native definitions, fingerprint, revision two and
registry remain. Original order: **59/220 complete, 161 pending; The Tower
of Darkness (lortower) next.**
