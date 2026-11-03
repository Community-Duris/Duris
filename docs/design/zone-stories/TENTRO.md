# The Mountain Village of Tentro: source story and environment map

Checkpoint 382 maps the active tentro2 area from its current WLD, MOB, OBJ,
ZON, area-list, quest-list, and related source files. The area combines the
Field of Endolin prairie, a Troglodyte settlement, the walled village of
Tentro, Nu's Oddity Park and its hidden zoo, and the separate Salol farming
community. This is a repository-source review; it does not establish that a
route, door, service, encounter, or item was exercised in a running game.

The source-backed graph, identity, dependency, and builder questions are in
the [Checkpoint 382 audit](../quest-story-followups/TENTRO2_AUDIT_REVIEW.md).

## Source closure

| Source | Reviewed scope |
| --- | --- |
| [Full area list](../../../areas/AREA#L259) | tentro2 is an active full-area contributor for zones 690-692. |
| [Zone reset file](../../../areas/zon/tentro2.zon#L1) | Zone 690, title "The Mountain Village of Tentro", reset mode 2, maximum room #69293. The file has 550 reset commands: 327 M, 94 D, 36 O, 56 E, 32 G, 4 P, and 1 R. |
| [Rooms](../../../areas/wld/tentro2.wld#L1) | All 294 rooms, #69000-#69293, and all 682 authored directions are mapped below. |
| [Mobiles](../../../areas/mob/tentro2.mob#L1) | All 115 local mobile prototypes, #69000-#69114, are grouped by the source story they support. |
| [Objects](../../../areas/obj/tentro2.obj#L1) | All 83 local object prototypes, #69000-#69082, are inventoried below. |
| Quest and story files | There is no local QST file or giver record, despite the stale tentro2 entry in [areas/qst/AREA](../../../areas/qst/AREA#L233). No story sidecar or local SHP file is present. The files define no local quest contract or shop stock. |
| Typed references and code | Six local M resets import ConnectorZones mountain wildlife; one local O reset imports the Verz1 bank-counter object #3097. No active external reset, QST prototype reference, story sidecar, SHP product, or maintained numeric code reference consumes a Tentro mobile or object prototype. |
| Title and tracked history | The normalized zone title is unique. Same-type VNUM scans find no active collision, and the followed history shows no named predecessor or whole-area replacement. The history does show a removed reciprocal Winterhaven sewer exit; see the audit. |

## Story and room map

The Field of Endolin opens as a dry, wildlife-rich prairie below the Black Dog
Mountains. A watering hole sustains the herds, while the descriptions contrast
the quiet grassland with danger in the mountains. Room #69040 attributes a
strange magical presence to a druid; that is authored room prose, not evidence
of a running room effect.

The Troglodytes are presented as a hidden, hostile society beneath the prairie.
The underwater approach opens into natural cave passages, homes, nurseries,
hatcheries, a latrine, stores, and guarded tunnels. The prose describes raids
for people and valuables, disease that is usually fatal to outsiders, ritual
killing of rival princesses, and worship of a strange god. No local quest or
room procedure in this source set establishes a disease, raid, or ritual
mechanic.

The mountain route reaches Tentro as a walled village with a square, market,
church, pub, bathhouse, bank, greenhouse, merchant stalls, homes, and guard
posts. Its inhabitants are described as diverse and often poor, while the
Battle School is called closed even as some room prose suggests activity. The
church serves both Tentro and Salol, but the source says Tentro is not the
religious center. Nu's Oddity Park is advertised as family entertainment; the
rooms below it instead describe a prison, stolen treasure, smuggling,
summoning, creature experiments, and a surgeon.

Salol is a small cooperative farming community. Ditches and roads connect
irrigated fields; families and workers cultivate corn, potatoes, and radishes
and share the harvest and its profits. The prose credits help from Tentro and
Woodseer. Its active WLD remains a separate weak component from the village
and cave graph, though it has its own active Surface approach.

| Room span | Place and authored story |
| --- | --- |
| #69000-#69040 | Field of Endolin prairie, wildlife, watering hole, foothills, and the druid's magical-energy description. |
| #69041-#69065 | Underwater arrival and natural Troglodyte cave settlement: homes, young and adult clan members, stores, nursery, hatchery, waste passages, and ambush routes. |
| #69066-#69079 | Secret worked tunnels, hidden passages, the princess's magical chamber, ritual lore, storage, and the locked chieftain's throne room and hoard. |
| #69080-#69094 | Narrow mountain trail, ledges, gorge crossings, and the guarded approach to Tentro. |
| #69095-#69111 | Village streets, alleys, market, square, guards, residents, and local trades. |
| #69112-#69115 | Shared Tentro and Salol cathedral, with mosaics and formal holy architecture. |
| #69116-#69119 | Further homes and village streets around the central district. |
| #69120-#69123 | Golden Dragon pub and its patrons. |
| #69124-#69136 | Closed Battle School, mayor's office, and meeting room. The mayor's desk is at #69135. |
| #69137-#69160 | Bank, greenhouse, merchant tent, and civic service rooms. The bank contains imported object #3097, whose shared code binding is described below. |
| #69161-#69179 | Bathhouse and underwater hot-spring supply, followed by secret catacomb-like spaces beneath Tentro. |
| #69180 | Wall lookout and guard post. |
| #69181-#69195 | Wall route and paved approach to Nu's Oddity Park. |
| #69196-#69218 | Hidden zoo security corridor, cells, captive rooms, stolen goods, summoning shrines, creature laboratory, and surgeon's room. |
| #69219-#69221 | Open plain outside Tentro's east gate. |
| #69222-#69246 | Salol's farm roads, irrigation ditches, and first fields and farmsteads. |
| #69247-#69266 | Cooperative crop fields, farm buildings, and workers' routes. |
| #69267-#69288 | Further crops, family homes, harvest spaces, and the prose about the Troglodyte virus. |
| #69289-#69293 | Mobile-holding and test-like rooms. Their raw WLD placement and one-way paths do not establish who may enter them or whether they are player-facing. |

These spans partition the full 294-room range without gaps. They describe the
text in the files; they do not establish a live traversal.

## Mobile roster

The 115 local mobile prototypes follow the area's five main story groups:

- **Prairie, #69000-#69011:** buffalo, prairie dog, vulture, sparrow, field
  mouse, female and young buffalo, rabbit, wolf, wild horse, half-elf
  explorer, and golden eagle.
- **Troglodyte clan, #69012-#69020:** stone golem, warrior, chieftain,
  princess, female, sentinel, mother, bodyguard, and child. Mobile #69042 is a
  shifty-eyed Troglodyte.
- **Tentro residents and passersby, #69021-#69041 and #69043-#69051:**
  Myrabolus visitor, bartender, drunk patron, gnome, church follower, bum,
  child, the five named Holy Order paladins, mayor, citizen, commoner, guard,
  rat, drunk dwarf, mayor's assistant, bank teller, street sweeper, Woodseer
  Halfling merchant, bathhouse workers, hunter, apprentice priest, greenhouse
  caretaker, naked man, traveling bard, and village guard.
- **Nu's park and hidden zoo, #69052-#69079:** curator Nu; chimera, manticore,
  lamia, satyr, barak, diatryma, gryphon, roc, simurgh, phouka, hobyah, trow,
  ghost, incubus, barquest, sea scrag, svirfnebli prisoner, bean si, yochlol,
  ettin, and white steed; plus the dungeon keeper, popcorn seller, clown,
  deranged surgeon, ancient elf, and elderly landscaper.
- **Salol wildlife and farm community, #69080-#69099:** bumblebee, robin,
  black mouse, stray cat, children, family dog, teenager, cow, farmers' wife,
  gnome merchant, elderly and young farmers, old and elderly women, work
  horse, ant, sparrow, potato bug, and drunk farmer.
- **Further Troglodytes, #69100-#69109:** warrior, outraged Troglodyte, hemp
  farmer, meek shaman, killer mosquito, half-turtle/half-sheep creature,
  male, berserker, female, and assassin.
- **Other placed figures, #69110-#69114:** church priest, barbarian merchant,
  bankrupt merchant, Winterhaven militia member, and the black essence of Nu.

The zone resets place these groups across the prairie, cave settlement, town,
park cells and lab, and farm. Counts and illustrative placements are recorded
with the graph and key network below; reset presence is finite source
configuration, not a guarantee of a particular encounter or drop.

## Object roster

The 83 local objects support the prairie, Troglodyte caves, town, zoo, and
farm:

- **#69000-#69020:** water, the pool teleporter, buffalo-hide backpack,
  golem mask, Troglodyte sword and axe, chieftain totem, Troglodyte belt,
  steel, throne, Sword of Halp, ranger cloak, steel javelin, bar, barquest
  tail, bean-si mask, surgeon's mask, elven quiver, shears, brass ring, and
  fountain.
- **#69021-#69038:** rat tail, order form, notched axe, blessed platemail,
  desk, flute, wristband, cap, mask, iron shield, iron dagger, baked potato,
  iron locket, green book, bookcase, fountain, triangular shield, and metal
  ring.
- **#69039-#69053:** blue cell key, simple key, devil's key, popcorn, blue and
  red balloons, spider dagger, Key of Shadows, moldy helm, sundial, clown
  paint, coins, scalpel, surgeon's silver earring, and surgeon's head.
- **#69054-#69073:** corn and radish plants, potato, dirt, farmer's hat, boots,
  bracer, golden ring, Troglodyte spiked ring and earring, gag, ettin whip,
  white bow, Tentro sash and spellbook, Troglodyte eyepatch and mask, hemp
  pants, and arrow.
- **#69074-#69082:** buffalo totem, wooden desk key, vest, harp, map,
  Troglodyte key and dagger, battered human corpse, and corn husks.

Several other-area systems may affect generic objects, but no local SHP file
defines Tentro shop inventory. Object #69052 is explicitly flagged
_noquest_; that flag does not create a local quest contract.

## Reset placements, locks, and travel

The zone reset file has 94 D commands matching the 94 WLD exits with the door
flag. The WLD-only lock network includes the following authored placements:

- The Troglodyte throne door between #69078 and #69079 is closed and locked
  with object #69079. A Troglodyte warrior in #69078 receives that key; the
  chieftain is reset in #69079.
- Zoo access through #69192 down and #69196 up uses simple key #69040. The
  key and an order form are placed inside desk #69025 in the mayor's office
  (#69135).
- The zoo cell doors use blue key #69039. The dungeon keeper #69060 is placed
  in #69197 with the key.
- The hidden trapdoor between #69209 and #69217 uses devil's key #69041,
  carried by incubus #69067 at #69212.
- Key of Shadows #69046 opens the hidden hall at #69205/#69208, the passage
  at #69213/#69216, and the door at #69217/#69218. One copy is reset on the
  ground at #69205; another is carried by yochlol #69072 at #69217.
- Wooden desk key #69075 is inside vest #69076 worn by Nu in the park office.
  Other P resets put Sword of Halp #69010 inside throne #69009 and put the
  order form and simple key inside desk #69025.

Three door pairs have asymmetric WLD flags: #69166/#69168 and #69177/#69178
mark only one direction secret; #69205/#69208 additionally differ in the
secret flag, although both directions are keyed and reset closed/locked.
These are source differences, not proof of a runtime exploit or intended
one-way discovery.

Object #69001 is a type-25 teleporter reset in room #69038, the prairie pool.
Its destination is room #69041 and its activation value is 7, CMD_ENTER. The
generic travel handler is in [spell_travel.c](../../../src/magic/spell_travel.c#L932),
and CMD_ENTER is 7 in [interp.h](../../../src/cmd/interp.h). This is an
authored item-travel route into the underwater cave and is separate from a
WLD exit. Static WLD counts below therefore describe the WLD layer only.

Six M resets import ConnectorZones wildlife: mountain goat #53612 in #69086
(twice), mountain lion #53619 in #69091, mountain eagle #53639 in #69092,
and rock lizard #53642 in #69083 and #69089. Those prototypes remain owned by
ConnectorZones. The bank places Verz1 object #3097 at #69149. Its sign says
deposit, withdraw, and balance, while its shared code binding is
storage_locker_obj_hook; neither its owner nor its general code binding is
Tentro-specific. The audit records the resulting service question without
assuming what a player sees at runtime.

## Active WLD connectivity

The active WLD has 294 rooms and 682/682 resolving directions: 678 internal
and four outward. All active targets resolve. The undirected weak graph has
two components of 227 and 67 rooms. Salol (#69222-#69288) is the 67-room
component; the other includes prairie, cave, mountain route, Tentro, and
zoo. Five active directions enter through four distinct rooms:

| Active source | Direction | Tentro target |
| --- | --- | --- |
| caves_skelenak #4152 | up | #69088 |
| Surface #543736 | north | #69222 |
| Surface #557287 | east | #69080 |
| Surface #557688 | north | #69080 |
| Winterhaven #55559 | up | #69249 |

Four outward directions are paired with active returns: #69080 south to
Surface #557688 and west to #557287; #69088 down to caves_skelenak #4152; and
#69222 south to Surface #543736. Tentro #69249 has no outward direction to
Winterhaven #55559, so that source edge has no ordinary WLD return.

Raw directed WLD traversal from the four active inbound target rooms reaches
250/294 rooms; reverse traversal from outward source rooms covers 291/294.
These calculations treat directions as open and omit item travel, scripts,
locks, and hazards. The authored pool teleporter above supplies a separate
entry to #69041; its effect is not counted as a WLD direction. No negative
target, unresolved active target, fall-chance field, or current marker was
found in this local WLD census.

The separate [Surface WLD](../../../areas/wld/surf.wld) labels room #557288
"Tentro". That room exits to neighboring #557287 and #557688 but has no
direct WLD exit into tentro2; the neighboring rooms provide the paired routes
shown above.

## Quest, identity, and source limits

No local QST file or giver contract exists. The quest area list nevertheless
names tentro2, and no same-stem QST appears in repository history. An external
Winterhaven quest passage mentions Tentro among places where a psychomia plant
has been heard of, but it names no Tentro VNUM and creates no local giver,
hand-in, or reward. A 2009 change described as completing a missing Tentro
quest changed only the zone reset, adding surgeon's mask #69016 to the
deranged surgeon #69075; it did not add a QST file.

The exact normalized title "The Mountain Village of Tentro" occurs once among
the 442 current ZON titles and is active. Active same-type WLD, MOB, and OBJ
checks find no exact VNUM overlap with Tentro; the bounded inactive-WLD scan
also finds no duplicate Tentro room VNUM. The followed paths do not establish
a renamed predecessor or whole-area replacement.
