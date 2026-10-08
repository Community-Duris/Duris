# Tribal Forest: comprehensive source map

Reviewed October 4, 2026. Zone 422, `tribal`; roadmap priority 57.
**Source review is comprehensive; live gameplay qualification remains pending.**
Active, ready accounting is required for discovery, visible encounters, journals
and new achievement/daily credit. Frozen obligation recovery remains separate.

The [schema-three journal](../../../areas/story/tribal.story.json) maps nine
meaningful outcomes, all ten native exchanges, seventeen contacts and 21 optional
checks: twenty current carried items and one earlier hunter receipt. The huge
bluebird's refusal of larger worm meat is explicitly excluded. That refusal
returns the same prototype kind without a meaningful reward; it should not
complete a story or achievement. All nine other exchanges retain native daily
shape. This classification does not qualify actual supply, difficulty or renewal.

## Reviewed evidence

- All 25 [native blocks](../../../areas/qst/tribal.qst): thirteen M, ten Q and
  two MA, representing fifteen addressed dialogue families on seven recipients.
  Both MA responses belong to addressed dialogue, even though their audience
  differs. All aliases are preserved, including `gretings` and `husban`; a
  keyword question alone is not an achievement.
- All 175 [rooms](../../../areas/wld/tribal.wld): 42200–42374 physically,
  42181–42374 in the registry. Full prose, headers, metadata and exits were
  reviewed: 163 exact prose groups, 46 headers, 61 non-exit metadata groups
  and 189 exit families. Header `42374 1 0 15 25 4` and reset mode one remain.
- All 67 [mobiles](../../../areas/mob/tribal.mob), 103
  [objects](../../../areas/obj/tribal.obj) and 377
  [resets](../../../areas/zon/tribal.zon), in 234 exact command families:
  M133/E87/O49/D38/G30/P25/F15. Inventory ownership changes after F; equipment
  slots, probabilities, caps, secret objects, containers and key breakage are
  relevant evidence. Active referenced reset prototypes and rooms resolve;
  that does not establish admitted generation or current presence.
- The [apprentice shop](../../../areas/shp/tribal.shp) has a valid tilde-ended
  `#42235~` header followed by the new-format `N` marker. It declares four
  produced items: siege horn 42231, parchment 42246, potion 42248 and ward
  scroll 42247. Keeper 42235 operates at 42259 with two 0–28 hour ranges.
  The shared [shop loader](../../../src/economy/shop.c#L2597) reads this form.
  Shop headers require a tilde-aware read rather than the generic `#number`
  counter used for mobile/object prototypes. The native record was checked
  directly; the production review index does not count shops.
- One literal local object special is active: object 42235 is assigned
  [`amethyst_orb`](../../../src/specs/specs.assign.c#L1738). Mobile 42235 is
  the apprentice, a separate namespace. The master hunter has ACT_TEACHER;
  the apprentice receives the shared shop role. No other literal local mobile
  or room special establishes a campaign, racial grove gate or escort endpoint.
  Numeric race/class/size tails are not custom procedure IDs.
- Imported fountain 72 in the glade uses the existing
  [`spell_pool`](../../../src/specs/specs.heavens.c#L963). Its DRINK effects
  are separate from native quest acceptance. No local trigger file is shipped;
  `areas/world.trg` is absent in this checkout. Reviewed global material
  producer/consumer scans found no foreign native consumers for the listed
  Tribal quest outputs. The physical boundary joins three surface hill rooms;
  the local Astral junction has six outward exits to `astral_main`.
- Relevant shared quest offering/reward recovery, reset admission, ordinary
  wandering/scavenging, switches, lock/key break, portals, movement/falls,
  container GET/PUT, traps and item identity were traced. The
  [generated review index](../../reference/zone-story-audits/tribal.md) is
  evidence navigation; this dossier supplies the deeper interpretation.

## Progression stories and exact acceptance

Each row is an independent accepted exchange. Current preparation remains
optional and read-only: it guides the player without imposing hidden history.
Native offering searches require loose carried items, not held, worn or nested
objects. Matching supplied materials fit without a personal kill or first-copy
source history. Ordinary dialogue does not implement a learned-topic milestone.

| Story | Exact player inputs | Native output and progression |
| --- | --- | --- |
| [Master hunter](../../../areas/qst/tribal.qst#L46) | Larger white-worm meat 42201 | Oat grain 42265 and 50000 XP. Grain is a producer link to the bluebird staff, not automatic completion of that next exchange. |
| [Bluebird grain](../../../areas/qst/tribal.qst#L11) | Oat grain 42265 | Blue-feather staff 42200 and 80000 XP. Optional hunter history does not prove the current grain's origin or replenish spent grain. |
| [Bluebird nest](../../../areas/qst/tribal.qst#L18) | Small white-worm meat 42202 | Small bird nest 42204 and 33000 XP. Larger meat is refused. The nest does not supply the Spider Queen's egg. |
| [Wife's escape](../../../areas/qst/tribal.qst#L77) | Bluish key 42222, fine shoes 42219, deerskin skirt 42220, wool blanket 42242 | Well-crafted skirt 42268, then D1 native retirement. It does not prove a personal chief defeat, shackles removal or escort home. |
| [Lost comb](../../../areas/qst/tribal.qst#L119) | Beautiful wooden comb 42241 | Gemmed brooch 42260 and 250000 XP. Her equipped crude comb 42259 is a different kind. |
| [Spotted deerskin](../../../areas/qst/tribal.qst#L180) | Green spotted deerskin 42244 | Bearhide amulet 42224. No earlier-receipt condition is imposed on the separate crystal recipe. |
| [Crystal experiment](../../../areas/qst/tribal.qst#L191) | Twisted bone 42221, magnifying crystal 42227, runed old bone 42212, devil horn 42263, ancient root 42267 | Magical magnifying crystal 42262. Five distinct kinds; narrated blood, sulphur and olive oil are supplied by the shaman, not additional player inputs. |
| [Xazapath's parts](../../../areas/qst/tribal.qst#L266) | Left leg 42293, left hand 42294, right hand 42295, right leg 42296, drow head 42297 | Hunting bow 42285 and leather quiver 42291 in one receipt, then D1 retirement. Forgiveness and returning home remain narration without a separate recorded endpoint. |
| [Queen's missing egg](../../../areas/qst/tribal.qst#L308) | Soft white egg 42302 | Queen Spider ring 42301 and 250000 XP. The egg is on an ancient tree above ground, not the descriptive egg pile in her underground room. |

The [larger-meat refusal](../../../areas/qst/tribal.qst#L25) is the tenth native
exchange. Preserve its native binding and settlement, but exclude it from the
nine story/achievement/daily outcomes. The durable offering path consumes the
input roots and issues rewards with `read_object` and creation grants in
[quest.c](../../../src/world/quest.c#L1395). Returning the same VNUM does not
establish reuse of the same physical UID; provenance must represent a replacement
honestly. Its existing native daily exclusion remains “Item exchange.”

## Sources, ownership and available material

The larger worm starts at loading room 42291 and carries 42201. The three
smaller worms start at 42272, 42274 and 42278, but only the final reset declares
42202. Both meats share similar aliases: use exact kinds, and never infer that
every worm owns a proof. The huge bluebird starts at 42289, with exits to forest,
oak and subsidiary staging rooms. It is distinct from ordinary bluebird
prototypes. The master hunter is fixed in his narrow hut at 42217.

The chieftain carries the bluish key at 42248; the wife starts at 42249 behind
the locked kitchen door. Shoes, skirt and blanket are O-declared in secret closet
42263. Several tribal NPCs also wear or carry the same shoe/skirt kinds, and
shoes are P-declared in the slain-tribesman container at 42220. Clothing worn by
the player must be removed before offering. The key is both ordinary access
equipment and a consumed recipe input; a successful unlock can break it at
fifty percent. A supplied replacement or shared open door can change the route.
Do not make personal unlocking or chief defeat an invented recipe prerequisite.

The huge brown spider at glade 42219 carries the beautiful comb. The green
spotted deer at 42206 carries the shaman's deerskin; ordinary deer are different
prototypes. The black bear at 42220 carries the twisted bone. These are declared
inventory objects rather than an implemented skinning or severing achievement.
The skinning knife is P-declared in human-carcass container 42298 at clearing
42367. It is ordinary equipment; its presence alone does not implement those
source events.

The lookout at 42269 holds the magnifying crystal in slot 19 with nominal
75-percent issuance. The runed old bone is P-declared in the unlocked bone-pile
container at 42282. The first devil following the shaman carries the horn; the
next following demon does not. Three ancient trees follow the forest mother at
42267: the first G-declares the root, the second the egg, and the third neither.
Both item declarations have nominal 75-percent issuance and cap one. Actor
order matters; do not attribute both items to the mother or every tree.

All five corpse parts are secret O-declared objects: left leg 42339, left hand
42352, right hand 42335, right leg 42349 and head 42346. The head is a quest-type
object; the four limbs have other-type objects. These are fixed source kinds,
not automatically cut from a dead drow. Xazapath begins at staging room 42317;
three equipment branches 42354–42356 lead toward apartment 42353. His NPC has
scavenging and wandering behavior, but actual arrival, equipment and survival
are not guaranteed by a static room title. Queen 42259 starts at 42346 with
three following spiders and can move. Meeting the actual recipient matters.

Ordinary wandering selects among ten directions plus a no-move draw, with
sentinel, master, combat, zone, room and movement checks in
[mobact.c](../../../src/mob/mobact.c#L8017). Do not translate staging exits into
an assured immediate encounter or a single calculated quest probability.
Reset comments about other bluebird/Hellwing holding rooms are not an active
placement proof: the shipped huge-bluebird M actually starts at 42289, rather
than unused first-load room 42368. Active, ready accounting preserves its item
reset guard; declarations alone do not promise renewable daily inventory.

## Access, controls, travel and hazards

| Route or interaction | Actual source behavior | Qualification needed before credit |
| --- | --- | --- |
| Village kitchen and closet | Bluish key opens 42248 west / 42249 east. Closet has secret entrances from the women's and warriors' huts; shoes/skirt/blanket have real declarations. | Accepted unlock and key destruction, secret discovery/opening, shared-door state and actual arrival; separate from supplied recipe inputs. |
| Vegetation, leaves, web floor and boulder | Switch 42271 uses PUSH at 42239 west; 42283 PUSH at 42285 north; 42284 PUSH at 42322 southeast; 42282 STOMP at 42315 down; 42287 PUSH at 42342 south. | Exact command and targeted switch, configured room/direction, mutation and later passage; no credit for unrelated PUSH/STOMP or an already open route. |
| Cellar and rear cave treasure | Red key 42233 is P-declared in shelf 42261 in the shaman hut; it breaks at 30 percent and fits the secret cellar door. Dracolizard's yellow key 42210 breaks at 100 percent and fits the rear treasure lock. | Source and current key lifecycle, approved scarcity, consumed-key recovery and route state. Neither is a crystal-offering input. |
| Fixed travel objects | Web 42281 uses ENTER to 42315; pentagram 42234 ENTER to 42297; black rift 42253 ENTER back to 42295; well 42250 ENTER to 42370; mirror 42264 STARE to 42281. All have unlimited native charges. | Targeted dispatcher and surviving actor arrival; room descriptions, issuing a command or seeing a glow are insufficient. |
| Amethyst orb | RUB selects a runtime room index; STARE shows room text after the 30-second viewing window, GLANCE shows its name, TOUCH burns or travels and subtracts 60–160 HP capped at current HP. | Target ownership, validated selection/guard policy, HP/effect settlement, actor location and replay/recovery; seeing a destination is not arriving. |
| Bone and ring traps | Runed old bone has `T 2 9 1 50`; ring of destruction has `T 2 10 1 50`. Damage codes 9/10 have no branch in the current handler's supported 0–8 switch. | Builder chooses intended supported payloads; qualify triggering charge mutation and subsequent recovery separately before any native repair. |
| Branches, trunks, forest and well | Narrow passages, no-ground sectors, F fall values, combat ivy and well depth have actual movement/hazard behavior. | Surviving actor/mount/follower arrival, flight/levitate and actor-specific eligibility; no generic “access solved” from a carried object. |

The [switch procedure](../../../src/specs/specs.object.c#L309) resolves the
actual named object, requires its exact command, validates room/direction and
clears EX_BLOCKED. It does not generally clear secret, closed or locked state.
Source-file D-state bits are distinct from runtime exit flags: the
[loader](../../../src/world/db.c#L1490) reads only the door kind, and
[D resets](../../../src/world/db.c#L4061) establish closed/locked/secret/blocked
state. Vegetation and boulder reset closed as well as secret/blocked; the web
floor and two leaf branches reset secret/blocked without being closed. Do not
describe every PUSH as completing all remaining passage conditions.

The [portal dispatcher](../../../src/magic/spell_travel.c#L932) matches the
target object and configured command. Native fixed routes and unlimited charges
are distinct from the orb's bespoke raw-index travel. The
[orb procedure](../../../src/specs/specs.highway.c#L123) has a takeable-object
condition on its source no-teleport guard; the local orb is fixed. RUB uses a
bounded random search, and TOUCH does not reapply RUB's destination filters.
GLANCE does not share STARE's timer. TOUCH can reduce HP to zero; model the
actual post-operation position rather than announcing a safe expedition.
Review bounded indices and target dispatch with actual-procedure fixtures before
any repair; no reproduced live stale-index crash is claimed here.

The [trap handler](../../../src/combat/trap.c#L479) consumes a positive charge
before selecting a payload; codes 9/10 have no supported branch. A GET/PUT
trigger can still reject that pickup. The bone-pile container is unlocked; a
yellow key for the rear chamber does not unlock the bone itself. A platinum
wand has supported cold code 3, and other traps must retain their own effects.

Grove prose describes ancient magic preventing human approach. Reviewed flags
include room/magic/travel restrictions, not a specific human-only exclusion;
no local assigned routine enforces that narrated condition. Record the prose
as lore until a builder chooses a real actor-specific gate and outcome. Wolf
dens, Hellwing's ivy passage, the force cage, ordinary birds and unbound staging
rooms also do not become invented quest terminals just because they have lore.

## Actual native repair and news treatment

The separate [`f8090481d` fix commit](https://github.com/Community-Duris/Duris/commit/f8090481df837bb4e1a675c901488f13090657d1)
changes exactly three exit-description words:

| Player trigger | Before | After and source proof |
| --- | --- | --- |
| Inspect west in forest room 42204 | “south” | “west”; D3 reaches 42203, whose D1 returns. |
| Inspect west on the dark forest path 42231 | “east” | “west”; D3 reaches 42232, whose D1 returns. |
| Inspect south at the village's northeast corner 42261 | “west” | “south”; D2 reaches 42260, whose D0 returns. |

[The focused regression](../../../tests/async/test_tribal_directions.py) fails
all three original clues and passes the repaired source. Exact-byte comparison
preserves every other native Tribal byte: destinations, door states, keys,
resets, mobs, objects, recipes and shop data. Live LOOK/traversal remains
unqualified. This is a shipped description repair, separate from journal
authoring and the pending proposals below.

**News sentence:** “Tribal Forest's two western forest exits and the southern
village exit now describe their actual directions.”

## Blockers, plan expansions and balanced repair proposals

| Finding | Evidence and impact | Next work and status |
| --- | --- | --- |
| Staged recipients and scarce source | Bird, large worm and drow use loading rooms; item caps and 75-percent rolls differ by actor. Wife and drow retire under mode one. | Qualify admitted M/F/O/P/G/E sources, actual wandering/presence and reset episodes before assignment. Preserve reset guards; do not promise a repair to guaranteed spawn behavior without builder intent. |
| First recovery versus supplied proof | Native delivery accepts supplied loose materials, and larger-meat refusal reissues the same kind. | Use committed UID/root/container/NPC ownership and transfer lineage. Keep original-source acquisition, same-kind replacement and turn-in receipts distinct; optional history does not allocate current material. |
| Reused access key and controls | Bluish key is required both for ordinary kitchen unlock and wife's offering; it may break. Switches clear blocking but can leave closed/secret state. | Add successful control/unlock/key-break/arrival events, shared-state and renewal policy. Builder decides scarcity and gate intent before changing key breakage or door behavior. |
| Unsupported trap payloads | Exact bone/ring codes 9/10 fall outside supported damage cases. This is a concrete source mismatch; intended payloads are unknown. | Choose supported effects with builder, then separate `fix:` commit and native GET/PUT/trap/recovery fixtures. Report charge/effect/pickup before-after and player news. No trap repair ships here. |
| Object targeting, random travel and HP | Orb's alias gate is not generic_find identity; selected target is a runtime index. Imported spell pool ignores DRINK target; fixed mirror STARE instead travels. | Qualify actual dispatcher collision, valid selection and actor/HP effects; persist a stable world identity if selection becomes durable. Preserve native policies until demonstrated defects and intent justify separate fixes. |
| Unimplemented lore endpoints | Wife has D1 but no escorted trip; Xazapath has D1 but no forgiveness/home endpoint; narrated human grove exclusion has no reviewed local implementation. | Builder chooses accepted actor/escort, scoped campaign and predicate terminals. Do not infer completed rescue, divine favor or racial access from prose, questions or static disappearance. |
| Direction and actor text | Leaves 42284 describe north while their configured switch opens southeast; tunnel-end 42342 says northwest although its ordinary exit is north; small redbird 42254 has a blue long description. | These are concrete text discrepancies requiring an exact field/route or actor fixture and separately identified news repair. Keep them pending here; do not rewrite maze topology or translate every similar description into a bug. |
| Audit methodology | The valid `#42235~` shop record needs a tilde-aware read. Runtime loader and direct record establish the real shop; generic prototype counters are insufficient. | Keep direct shop stock/role verification in the source fixture. No native shop repair is needed for this header, and no production-parser defect is claimed. |

## Verification and remaining qualification

Focused source fixtures preserve all ten bindings and exact rewards, every
dialogue alias, twenty optional carried checks, optional hunter history, F owner
order, unique corpse kinds, actual key/control/portal/trap fields, active roles
and the refusal exclusion. Actual C++ journal journeys verify discovery and
encounter reveal, read-only rendering, wrong-kind and worn-material rejection,
supplied independent acceptance, earlier history versus spent grain, one receipt
for the paired bow/quiver, independent outcomes, replay and cold recovery.

Model completion fixtures supply an accepted receipt; they do not simulate native
GET, combat, spawning, door state, trap effects, actor retirement or a full played
persistence journey. Those remain live qualification work. No DB, account,
server, migration, deployment or merge operation is performed.

The catalog becomes 78 maps, 1636 achievement units, 1464 potential daily units
and 2210 projected rows. Native definitions/fingerprint/content revision two,
registry and earlier 77 maps remain unchanged. The one achievement/row reduction
is the larger-meat refusal exclusion; no daily candidate is removed. Original
roadmap order remains: 57/220 complete, 163 pending; The Ancient Halls of
Ironstar (`lornecro`) is next.
