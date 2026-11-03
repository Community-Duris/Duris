# Faerie Realm: comprehensive source story map

Priority 28, source area `realm`, canonical zone 140. Source review is complete;
active-world qualification remains pending. Discovery, NPC encounters, journals,
achievements and new daily eligibility require active, ready economic accounting.
This mapping does not activate accounting or change native world data.

The [revision 1 journal](../../../areas/story/realm.story.json) classifies all
seven native exchanges as three named stories, two supporting services and one
excluded walnut return. The identical forge recipes share a service with two
alternative recipients. Ten contacts cover every one of the ten addressed M
families and six useful source/lore contacts; thirteen optional checks explain
current materials, access and earlier Finn receipts. Three story outcomes are
potential daily candidates under reset mode two. Recipient retirement and the
ability to repeat a contract are distinct from current availability.

## Evidence and review boundary

Reviewed all seventeen [Q/M blocks](../../../areas/qst/realm.qst), all 211
[rooms](../../../areas/wld/realm.wld), 74 [mobile prototypes](../../../areas/mob/realm.mob),
123 [objects](../../../areas/obj/realm.obj), the single [Fulbar shop](../../../areas/shp/realm.shp),
and all 454 [reset commands](../../../areas/zon/realm.zon) in 213 grouped families.
Resets contain 246 M, 66 G, 45 E, 41 O, thirty P and 26 D commands. Canonical
bounds are 13939–14210; source-room membership is 14000–14210. All reserved reset
fields are zero. The missing `141120` object declaration is recorded below.

Read the whole [realm procedure file](../../../src/specs/specs.realm.c), its
four actual local [assignments](../../../src/specs/specs.assign.c#L981), and
the relevant native giving/reward recovery, room/item bootstrap, command
dispatch, shared speech doors, keys, movement/falling/no-magic, mobile wandering,
combat/periodic/death callbacks, shop stock and device/repair paths. The range
extractor also lists `bridge_troll` at mobile 14202, but its comment belongs to
Wilderness Near Verzanan and that mobile is absent from the loaded prototype
inventory. It is not a fifth local realm interaction. No `_proclib_` object
record supplies a local handler; optional `areas/world.trg` is absent here.
Deployment-local scripts require their own review.

Bounded foreign review read the complete five planar component carriers and
their reset/source rooms, plus Moradin's Fix source/gift and meeting room.
Reviewed all 23 loaded foreign boundary/dispersal destinations, including the
mainland forest approach. Missing targets 164929 and 25087 are explicit findings.
These are supply/access reviews, without claims of comprehensive foreign-zone
coverage or played acquisition and travel.

## Named progression and exact accepted terms

The following order explains the narrative. Native deliveries do not require
the player to ask every topic, defeat the carrier personally, read the pools,
or finish earlier deliveries. Supplied proofs remain valid.

| Family | Exact native terms | Meaning and journal treatment |
| --- | --- | --- |
| [Finn's lost signet](../../../areas/qst/realm.qst#L41) | Signet 14036 → map 14041 and glowing ring 14018; Finn 14015 remains | A named story. The tree spirit wears the real signet. This answers the lost-ring request, while his missing key remains unresolved. |
| [Finn's castle key and departure](../../../areas/qst/realm.qst#L60) | Mammoth key 14037 → sword 14023 and 500,000 copper; Finn retires | A separate named story with optional earlier ring history and amulet/key preparation. Accepted retirement and farewell are implemented; a player escort or actual castle arrival is not. |
| [Celriya's family blade](../../../areas/qst/realm.qst#L94) | Ancient elvish sword 14001 → goggles 14011 and wand 14076; Celriya 14028 retires | A named story. She interprets Tvelor's blade as reassurance about her people. The blade is local ground stock, so her response does not prove foreign recovery, Oberon's revival, or time travel. |
| [Finn's glowing-ring trade](../../../areas/qst/realm.qst#L29) | Glowing ring 14018 → map 14041 and 500 copper; Finn remains | Supporting service. The body calls the offered object a scroll; the actual input is armor. Returning the signet is one producer route, not a mandatory prior contract. |
| [The five-plane forge service](../../../areas/qst/realm.qst#L125) | Five distinct parts 14121–14125 plus 5,000,000 copper → Fix scroll 14126 | Jamfluul 14073 and [Dopplepopper 14074](../../../areas/qst/realm.qst#L153) are alternative recipients for the same service. No reconstructed forge state or separate repair outcome follows the exchange. Both paid offerings remain unsupported by active accounting settlement. |
| [The walnut rejection](../../../areas/qst/realm.qst#L53) | Walnut 14028 → walnut 14028 | Reviewed exclusion. Finn rejects it and gives the same kind back. This replacement response is not a rewarded recovery or story milestone. |

The key finale's 500,000 copper is 500 platinum; the forge fee's 5,000,000 copper
is 5,000 platinum, agreeing with the makers' explanations. Cash rewards use the
existing reward path; a cash offering plus five materials is a separate,
currently unsupported settlement. Empty forge success prose does not remove
the actual Fix output. The two recipes keep their exact giver/receipt identities
even though journal presentation groups their equivalent service.

## Material sources, containers and competing uses

| Proof or aid | Actual declaration | Consequence |
| --- | --- | --- |
| Finn's signet 14036 and tree amulet 14038 | E on tree spirit 14026, initially equipment load room 14199; cap one each | The spirit can wander through the load-room exits toward its chamber. Its chamber prose alone does not establish initial placement. Both items are worn stock; ownership/recovery must be qualified separately. |
| Ancient elvish sword 14001 | O at cottage heart 14112; cap one | Not Finn's final sword or a weapon from Oberon's corpse. It supplies Celriya's independent accepted request. |
| Mammoth castle key 14037 | G on Oberon's spirit 14071 at tomb 14158; cap one | No reviewed local door uses this key. It is an offered proof for Finn; the amulet opens the route to its carrier. |
| Glowing faerie ring 14018 | Finn's signet exchange only | It is [type-9 armor](../../../areas/obj/realm.obj#L242), with effects, without a teleport type or assigned handler. The follow-up map/copper trade does not make it a homeward scroll. |
| Maps 14041 | O at Way 14001 and P in rotted corpse 14039 at mud pit 14110; shared cap two; both Finn exchanges also produce maps | Finding or receiving a map is orientation, without a new receipt for a discovered exit. The corpse is a pre-existing container; its lore does not establish the player's party. |
| White-metal key 14034 | E in Anna's HOLD slot at cottage front room 14064; cap one | Fits the cellar door, trapdoor and dusty chest. Acquiring it need not mean killing Anna; gifts and supported alternate access remain possible. |
| Amulet 14038 | Key-type object, worn at the tree spirit's neck | Fits the keyed southern tomb slab and is listed on the chamber's downward exit. Shared `has_key` accepts top-level carried or HOLD, not a necklace worn at the neck. The optional carried check matches preparation without adding an equipped prerequisite. |
| Goblin tablet 14068 | P in mountainous junk heap 14062 at cave 14195 | The text identifies Tvelor, Celriya and the ancient blade. It is an explanatory clue; possession alone is not accepted reading or comprehension. |
| Walnut 14028 | G on grand Fianna knight 14069 in gazebo 14089; cap one | A real source with an unrewarded Finn response. The knight is a contact, not an extra achievement. |
| Guhan's bronze key 14013 and Mab's note 14014 | Key G on Guhan 14005 at 14051; note P in his locked desk 14012 there | The note says the lost Song was not found. No local Song collection or Mab-restoration quest endpoint is implemented. |
| Dark-steel key 14081 | P in Finn's equipped traveling cloak 14080 | No reviewed local door/container uses it. The rowan key 14107 also has no declared ordinary reset producer or local lock consumer. These are orphan aids, not required journal gates. |

### Five distinct planar parts and alternate Fix sources

Every source below is a single G declaration with cap one and chance 100. All
five exact objects are needed together for either maker. Current possession
does not prove who defeated the carrier, where the part first appeared, or
which player's journey supplied it.

| Part | Carrier / initial room | Source |
| --- | --- | --- |
| Heat 14121 | Imix 25440 / 25450 | [Fire reset](../../../areas/zon/plane_fire_one.zon#L68) |
| Wind 14122 | Yan-C-Bin 24440 / 24450 | [Air reset](../../../areas/zon/plane_air_one.zon#L61) |
| Hammer 14123 | Ogremoch 23806 / 23850 | [Earth reset](../../../areas/zon/plane_earth_one.zon#L64) |
| Astral forge 14124 | Demogorgon 19704 / 19730 | [Astral reset](../../../areas/zon/astral_main.zon#L43) |
| Water basin 14125 | Olhydra 23240 / 23250 | [Water reset](../../../areas/zon/plane_water_one.zon#L62) |

Fix 14126 also has G stock on Moradin 83521 at Alatorin 84055 and a golden-shard
gift there. That receipt remains Alatorin's and its retirement/entitlement are
separate from the realm's five-part service. The epic store offers the same
prototype for 105 epics through its purchase operation. These alternatives
must not fabricate five-plane recovery or a local forge completion.

## Actual access and unfinished routes

- The only reviewed ordinary foreign inbound edge is mainland forest 531694
  east to side path 14002; its west edge returns. The entry clearing loops
  through 14003–14007 before the closed/secret southern path at 14006 leads
  toward the Way. The library orb is [type 25, touch 320, target 14002](../../../areas/obj/realm.obj#L643),
  with unlimited uses. O stock at library 14067 and heart 14112 shares cap two.
  It returns to the realm's approach; westward travel leaves the realm. Record
  accepted teleport and actual boundary arrival separately.
- Anna's front-room description and Finn's addressed/periodic advice direct
  players to the [library](../../../areas/wld/realm.wld#L1492). All its entries
  are static extra descriptions reached through ordinary looking. No Anna
  M/Q binding or special grants a learned riddle. The [golden gate](../../../areas/wld/realm.wld#L2008)
  has key `-2`, keyword `riddleanswer`, no destination, and no locked reset;
  the library literally contains an undetermined-riddle placeholder. Builders
  must choose destination, answer, gate state and route before claiming passage.
- The cellar couch entrance and white-key trapdoor lead through dangerous,
  partly no-magic roots and the floorless shaft. `tree_spirit` intercepts down
  while present, prints the throw-back prose and returns handled; it does not
  itself relocate the player. The passage from mud pit 14110 into chamber
  14111 has no ordinary reverse west edge. The heart has an upward edge;
  source prose promising magical prevention is not a separate scripted veto.
  Qualify live presence, movement, falling and orb return rather than copying
  every warning as an enforced quest prerequisite.
- The [temple garden](../../../areas/wld/realm.wld#L3352) has real `-2` magic-word
  doors with keyword `peace`. [Shared speech handling](../../../src/cmd/actcomm.c#L186)
  clears locked/secret state on a matching spoken word, then the player opens
  the still-closed door. The [tomb's southern slab](../../../areas/wld/realm.wld#L3671)
  is keyed to 14038; its reverse door instead uses `-2` and final keyword
  `blah`. Review the intended return word and reciprocal policy without
  silently replacing either side or inferring a rescue.
- The two [reflecting pools](../../../areas/wld/realm.wld#L3551) have detailed
  room E visions from Oberon's and Celriya's viewpoints. The drink-container
  prototypes have no time-travel procedure. These histories explain the blade
  request and the ruined city, without recorded examination objectives.
- Both makers reset at [dispersal room 14209](../../../areas/wld/realm.wld#L4688).
  Rooms 14208–14210 have outward edges to distant bosses, treasure rooms and
  cities, with no ordinary inbound path in the loaded graph. Neither maker is
  sentinel or stay-zone. The missing 25087 exit is one obsolete lead among
  otherwise loaded dispersal choices. Describe roaming recipients, not a
  reachable Faerie Realm smithy waypoint. The tree spirit likewise starts in
  an equipment load network, not its chamber; its stock/encounter must be traced.

## Procedures, source discrepancies and balanced repair plans

Finn's periodic procedure supplies advice/combat taunts, cricket supplies
chirping, and woodland faerie supplies pranks and an attempted theft. None
records a named quest completion or learned topic. The actual journal delivery
and NPC encounter paths remain separate from these effects.

| Finding | Current evidence | Plan and qualification |
| --- | --- | --- |
| ZSQ-REALM-CONTENT | Finn's scroll response accepts armor 14018. Fulbar's wall advertises a bulky travel scroll, while the actual shop/native stock 14112 is an identify scroll. Anna's riddle is unfinished. | Builders select corrected explanations or deliberate replacement content with receipt compatibility. Preserve actual map/ring rewards and stock until intent is settled; do not repurpose identify or invent a working teleport scroll. Choose a complete riddle route or retire its promise. |
| ZSQ-REALM-COMBAT | The tree procedure returns false for `CMD_SET_PERIODIC`, handles summoning only on 0, and rejects modern `CMD_MOB_COMBAT` (-102). Bootstrap therefore schedules no periodic callback; ordinary combat does not reach the helper block. Shared static counters would span appearances if enabled. | Treat the helper block as an unserved combat behavior. Builder review should choose supported combat/tick dispatch, per-appearance state, cadence and cap before re-enabling. Preserve the intended bounded helper difficulty; qualify concurrent/replaced spirits, death reset, frozen draws and accepted creature publication. Do not make a helper kill a new quest requirement. |
| ZSQ-REALM-SOURCE | Tree spirit and both makers start in load/dispersal rooms and can wander; current inventory guidance cannot prove their live locations or personal source recovery. Some recipients retire after delivery. | Qualify active reset issuance, actual parent/slot/generation, wandering and physical encounters, third-party supplies, accepted deliveries and post-retirement/restart availability. Exact source history needs committed custody/actor attribution; current optional checks remain possession/history only. |
| ZSQ-REALM-ACCESS | Golden-gate destination is absent; cottage east exit 164929 and dispersal exit 25087 are unloaded. Tomb sides have different key/word policy. | Builders select restoration or retirement, not guessed VNUMs. Validate both sides and accepted opening/arrival separately, including wrong word/key, already-open state, reset, denied travel and return. Existing approach/orb routes mean these findings do not establish that all quests are unreachable. |
| ZSQ-REALM-RESET | O line 174 requests absent object 141120 at 14203; object 14120 is a real bracelet. | Likely extra digit is a repair candidate, not authorization to substitute. Select intended equipment, preserve its rarity/placement and qualify reset issuance without duplicates. Validate the missing bridge-troll literal separately; it is not local quest content. |
| ZSQ-REALM-FORGE / REPAIR | Two five-item cash recipes produce the same Fix scroll. Spell 595 sets the selected item's condition to 100. Existing device actions resolve target UID/location, consume ink and publish effects, but no quest repair receipt follows. | Keep paid refusal until atomic material/fee/output settlement. Qualify exact recipe allocation, alternative recipients and interruption/recovery. Extend existing device/custody work for accepted scroll retirement plus stable target condition mutation and durable replay; a generic completed action alone is insufficient proof of a particular repaired item. Preserve repair strength and distinguish obtaining from using Fix. |
| ZSQ-REALM-LORE | Mab's restoration is historical, the lost Song has no local terminal, Oberon's revival is hoped for, and the golden gate is unfinished. | Keep lore visible. Builders may retain it or author explicit outcomes/episodes/branch ownership before adding semantic objectives. Finn's final farewell and Celriya's departure have real retirement receipts, without proving every earlier stage or a player escort. |

## Verification and next qualification

Focused regressions protect all seven exact bindings, the three/three achievement
and potential-daily denominator, equivalent forge recipients/five independent
parts, service/exclusion classification, all addressed topics, thirteen optional
checks, initial reset parents and current object/route identities. The actual
native file loader must accept the whole journal directory and show optional
preparation without blocking supplied final proofs. Regenerate the
[source index](../../reference/zone-story-audits/realm.md), inventory and production
catalog together; native definitions, revision/fingerprint and prior maps stay
unchanged as parsed data.

Active-world journeys remain pending: fresh sources and suppliers; signet versus
glowing ring; held/carried/neck amulet access; both garden/tomb directions;
floorless/no-magic paths and orb/mainland return; retiring Finn/Celriya appearances;
both makers after dispersal; supported mixed fee; five-part provenance; Fix
obtaining/targeted use/abort/restart. Tests of static mappings and source behavior
do not certify these played outcomes. No accounting activation, migration or
DB/server operation is required to ship this source map.

The production-catalog regression and native C++20 story/file-loader regression
passed for all 51 maps, including the supplied-key route, independent receipt
recovery and all five exact forge-material checks. All 28 source indices are
reproducible. Native definitions/fingerprint, registry and fifty earlier parsed
maps are unchanged. The maintained SQL server build completed with current
objects; formatting, whitespace and all 605 local/source-line links in this
checkpoint passed review. These are source and fixture results, without live
combat or economic settlement qualification.
