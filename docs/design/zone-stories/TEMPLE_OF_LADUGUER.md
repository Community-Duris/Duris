# Temple of Laduguer: comprehensive story and environment mapping

Checkpoint 381 maps the active Temple of Laduguer from its current area files.
The zone is an inhabited Duergar temple and craft settlement built into a
volcanic Underdark cavern. Its source story comes from rooms, mobile and object
prototypes, reset placement, and exit descriptions. It has no local QST
contracts, QST giver list, story sidecar, or shop source. No task, trade, or
personal progress system should be inferred from the narrative alone.

The static repository audit and conservative owner questions are in the
[Checkpoint 381 audit](../quest-story-followups/TEMPLE_LADUGUER_AUDIT_REVIEW.md).
This dossier is source mapping, not a played journey or proof of live access,
service operation, safe return, or item availability.

## Complete source closure

| Source | Reviewed scope and story implications |
| --- | --- |
| [Area membership](../../../areas/AREA#L46) | The un-commented temple_laduguer *81,82 entry makes this an active full-area contributor. |
| [Zone/reset file](../../../areas/zon/temple_laduguer.zon#L1) | Zone 81, title “The Temple of Laduguer,” reset mode 2. The 230 counted commands are 98 M, 72 D, 56 E, and 4 G. Reset groups locate the guards, learners, trainers, pilgrims, elemental creatures, forge keeper, innkeeper, assassin, mounts, and their equipment. |
| [Rooms](../../../areas/wld/temple_laduguer.wld#L1) | All 103 rooms, #8100–#8202, and all 215 authored directions are reviewed. Descriptions form the tunnel, gatehouse, temple, towers, forge, shrines, inn, and stables mapped below. |
| [Mobiles](../../../areas/mob/temple_laduguer.mob#L1) | All 24 prototypes, #8100–#8123, are reviewed by identity, prose, and reset placement. |
| [Objects](../../../areas/obj/temple_laduguer.obj#L1) | All 28 prototypes, #8100–#8127, are reviewed by name, descriptions, type, and reset/equipment role. |
| QST and authored story | There is no areas/qst/temple_laduguer.qst, no giver record for this zone in the QST source, and no mapped story sidecar. The local files define no native request, hand-in, journal, or quest outcome. |
| Shops and special procedures | There is no local SHP file or local room/mobile procedure assignment for an inn, shop, offering, or Temple-specific service. Generic game systems may still apply, but the source does not configure those services here. |
| Typed external references | No active external QST, ZON reset, or SHP product consumes a Temple prototype. Eight product entries in the unregistered Sylvandawn SHP are source-only references; their current rows and ownership are documented in the audit. |
| Title and followed history | The normalized active title is unique. The followed WLD and ZON history identifies the 2016 creation and zone-number change; no distinct active predecessor or replacement is established. |

## Story and settlement

The Temple presents Laduguer as the Duergar deity of artisans, smithing, and
magic weapon creation. Its residents protect a working forge and a network of
formal shrines, with pilgrims and clerics sharing space with instructors,
guards, mounts, and visiting travelers. Room prose alternates between calling
Laduguer a “patroness” and a god. This is a source-level naming and identity
question, not a basis for silently rewriting the deity’s lore.

The named residents make the settlement legible even without a local quest:

- **Training and instruction:** Murgol (#8103) teaches warriors; Weltha
  (#8104) is the mercenary instructor; Dorthis (#8105) is the rogue instructor;
  Ivar (#8106) is the anti-paladin instructor; Granhol (#8113) is the clerical
  instructor. Young guard and cleric prototypes occupy the training and
  barracks cluster.
- **Temple and craft community:** Two Duergar craftsman pilgrims (#8110,
  #8111) are described in relation to their faith and work. Their associated
  clothing, ash badges, and forged gear reinforce the artisan community. The
  shrine descriptions include offering pedestals, but no local object or
  transaction source defines a required offering.
- **Security and forge work:** Gatehouse, barracks, veteran, elite, and
  inexperienced guards protect the entrances and interior. Xaphan (#8117),
  “Keeper of the Volcanic Forge,” is placed at the inner forge with a swirling
  mass of dancing flames (#8122). Rogue and summoned fire elementals (#8115,
  #8118) also occupy the lava passages and forge.
- **Hospitality and travel:** Valkar (#8121) is called the innkeeper and is
  reset into room #8200, titled an inn for Temple visitors. The prose and NPC
  establish hospitality thematically, but the files contain no inn procedure,
  shop stock, or explicit lodging transaction. Giant steeders (#8122) occupy
  three stable rooms whose descriptions welcome resident and visiting mounts.
- **An isolated threat:** A raiding Halfling assassin (#8123) is reset in the
  stables with two short glassteel daggers (#8127). The mobile prose identifies
  an Underdark assassin and disguise, but no local QST names a victim or
  contracts a hunt. This dossier does not invent one.

### Complete mobile roster and placements

| Mobile | Source identity | Reset rooms |
| --- | --- | --- |
| #8100 | Duergar guard in training | #8120, #8121, #8122 |
| #8101 | Adolescent Duergar guard in training | #8123, #8124, #8125 |
| #8102 | Duergar gatehouse guard | #8103 |
| #8103 | Murgol, warrior instructor | #8126 |
| #8104 | Weltha, mercenary instructor | #8127 |
| #8105 | Dorthis, rogue instructor | #8128 |
| #8106 | Ivar, anti-paladin instructor | #8129 |
| #8107 | Duergar guard | #8131, #8133, #8135, #8137, #8139, #8141, #8143, #8145, #8147 |
| #8108 | Elite Duergar guard | #8173, #8174 |
| #8109 | Elite shrine guard | #8180, #8182 |
| #8110 | Duergar craftsman pilgrim | #8185, #8186, #8187 |
| #8111 | Duergar craftsman pilgrim | #8189, #8190, #8191, #8192 |
| #8112 | Young Duergar cleric in training | #8150, #8152, #8154, #8156, #8160, #8162, #8164, #8166 |
| #8113 | Granhol, clerical instructor | #8193 |
| #8114 | Veteran Duergar guard | #8167 |
| #8115 | Rogue fire elemental | #8169, #8170, #8194 |
| #8116 | Elite forge guard | #8196 |
| #8117 | Xaphan, Keeper of the Volcanic Forge | #8197 |
| #8118 | Summoned fire elemental | #8197 |
| #8119 | Inexperienced Duergar guard | #8110, #8112 |
| #8120 | Young Duergar cleric adept | #8171, #8172 |
| #8121 | Valkar, the innkeeper | #8200 |
| #8122 | Giant steeder | #8157, #8201, #8202 |
| #8123 | Raiding Halfling assassin | #8201 |

The resident equipment reinforces these roles. Objects #8100–#8101 are
cold-iron axes; #8102 is an ash badge of the Laduguer artisans; #8103–#8113
are volcanic-forged clothing and armor; #8114–#8120 are clerical clothing and
ornaments; #8121 and #8125 are separate small mithril key prototypes; #8122
is the fire mass; #8123–#8124 are woven-mithril jewelry; #8126 is a glistening
black potion; and #8127 is the assassin’s short glassteel dagger. Their reset
placement is finite source configuration, not a promise that an item will be
available to a particular player.

## Environment and route map

| Room span | Place and authored function |
| --- | --- |
| #8100–#8102 | Basalt volcanic tunnel approach and split. Room #8100 connects west with active Underdark #827455 and east into the Temple approach. |
| #8103–#8125 | Gatehouse, entrance guard posts, corridors, staircase landing, guardhouse barracks, and resident rooms. The prose emphasizes visitor screening, guard duties, rest, and Temple protection. |
| #8126–#8129 | Training rooms behind guarded doors in the barracks cluster. |
| #8130–#8147 | Circular foyer and curved halls around the volcanic temple core. |
| #8148–#8157 | Eastern tower routes and one steeder stable. |
| #8158–#8166 | Western tower routes and the approach toward the forge stair landing. |
| #8167–#8174 | Forge stair landing and stair/tower connectors. The down route from #8167 enters the volcanic tunnel at #8168. |
| #8175–#8178 | Metal bridge across a volcanic hot spring; steam and difficult breathing are described. |
| #8179–#8182 | Lava-flow stair, ash-and-smoke stair, and volcanic spring rooms. |
| #8183–#8187 | Laduguer shrine and connecting shrine rooms. |
| #8188–#8192 | Separate ash-shrine foyer and four side shrines. |
| #8193–#8197 | Tributary chapel and the approach to the guarded subterranean forge, including Xaphan’s room. |
| #8198–#8199 | Basalt tunnel and wooden-door junction serving the stables and routes toward the Temple. |
| #8200 | Visitor inn and reception room; Valkar is reset here. |
| #8201–#8202 | Two steeder stable rooms. |

Every ID in the contiguous range #8100–#8202 has a room record. Static directed
traversal from #8100 reaches all 103 rooms, and every room has a directed path
back to #8100 when WLD directions are treated as open and scripts are ignored.
Doors, keys, room hazards, guard behavior, travel spells, and runtime arrival
conditions can change practical access.

## Reset and item custody notes

The mode-2 reset describes a resident settlement, not a guaranteed service
schedule. Repeated M commands place multiple guards and creatures; E and G
commands equip or give locally defined armor, clothing, keys, flames, potion,
and weapons. The small mithril key #8125 is assigned to the last member of the
four-veteran-guard reset group in room #8167. The other key, #8121, is assigned
to the last of the four elite forge guards in room #8196. The assassin in
#8201 receives two copies of #8127 in equipment slots 16 and 17.

Two rooms have fall-chance fields in the inventory, and no current marker is
recorded. Static WLD connectivity ignores both these hazards and the reset
state of doors. See the [audit supplement](../quest-story-followups/TEMPLE_LADUGUER_AUDIT_REVIEW.md#active-connectivity-and-door-state) for the exact paired exits and source inconsistencies.

## Identity and migration boundary

The active source identity is temple_laduguer, listed in areas/AREA and using
zone number 81. Inactive sylvandawn.wld reuses all 103 Temple room numbers,
but every overlapping source room has different room text. Its non-overlapping
approaches, resets, and shop records stay owned by the unregistered Sylvandawn
source and its existing RETAIN IN PLACE candidate record. These numeric
matches do not establish a replacement, shared runtime map, or safe relocation.
