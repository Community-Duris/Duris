# Tunnels of Payang (`rtun`): source story review

**Scope:** source review of the 87 WLD rooms, 29 mobile prototypes, four local
object prototypes, four `.shp` records, zone resets, and authored story
sidecar. It maps static source facts and dependencies; it does not qualify
live shops, special behavior, route safety, or ordinary versus staff
reachability.

## Story coverage state

`rtun` is active in the full `areas/AREA` source list as zone `903`, with 87
rooms, 29 mobiles, and four local objects. The coverage register has no local
`rtun.qst` in its file set and records zero native Q/QA contracts, even though
the QST area index lists the key
([coverage row](ZONE_COVERAGE_REGISTER.csv#L255),
[AREA entry](../../../areas/AREA#L319)). Its story sidecar says
[`coverage: complete`](../../../areas/story/rtun.story.json#L5), but has empty
[`contacts`, `stories`, and `exclusions`](../../../areas/story/rtun.story.json#L11).
The introduction says players can meet residents and “uncover local requests,”
which the current sidecar does not map. That source combination records no
local story contact or step; it does not prove that no special or live service
behavior exists.

## Environmental story

A source-text grouping finds 41 distinct room title/description pairs across
the 87 records. Repeated environmental prose is not treated as proof that the
rooms are duplicates.

| Setting | Source evidence | Story cues |
| --- | --- | --- |
| Ancient crypt and upper tombs | [Crypt entrance](../../../areas/wld/rtun.wld#L799), [upper tombs](../../../areas/wld/rtun.wld#L823), [chamber of caskets](../../../areas/wld/rtun.wld#L909) | A large tombstone opens onto a crypt built by an ancient civilization and now claimed by vast undead armies. The upper rooms hold caskets and noble graves; the chamber of caskets is also the target of a one-way Heavens exit record. |
| Halls and honored dead | [Halls of the Dead](../../../areas/wld/rtun.wld#L961), [warrior tomb](../../../areas/wld/rtun.wld#L1026), [magi tomb](../../../areas/wld/rtun.wld#L1078) | Separate burial spaces preserve the remains of warriors and magi. Crypt keepers and guards connect the grave rooms to the undead occupation rather than depicting them as empty ruins. |
| Dwarven tunnels, shrine, and laboratory | [Unholy shrine](../../../areas/wld/rtun.wld#L373), [small armoury](../../../areas/wld/rtun.wld#L395), [unholy laboratory](../../../areas/wld/rtun.wld#L1278) | The deeper routes include dwarven tombs and tunnels, enraged dwarven dead, an unholy shrine and armoury, and a laboratory. These scenes point to the crypt’s conversion into a fortified undead complex. |
| Digging and surface approaches | [Excavation rooms](../../../areas/wld/rtun.wld#L1471), [Bog of Death entrance](../../../areas/wld/rtun.wld#L1453), [Above the Crypt](../../../areas/wld/rtun.wld#L1527) | Undead crews excavate newer tunnels for corpses while suffering cave-ins. The Bog of Death opening and the wasteland tombstone give the complex two distinct surface-facing approaches in prose and exits. |
| Service alcoves and unresolved room text | [Gravedigger alcove](../../../areas/wld/rtun.wld#L1348), [room `90381`](../../../areas/wld/rtun.wld#L1544), [room `90382`](../../../areas/wld/rtun.wld#L1566), [blank alcoves](../../../areas/wld/rtun.wld#L1581), [banker alcove](../../../areas/wld/rtun.wld#L1603) | The area includes a gravedigger merchant and a lich banker. Rooms `90381–90384` have “King Load” titles, including literal “Doowap Doowap” text and empty descriptions; `90385` and `90387` are also blank-description alcoves. The reset file places a lurking shadow beast at `90385` and Nyroclex the Lich banker at `90387`, with counter object `3097` in the banker’s room ([shadow beast](../../../areas/zon/rtun.zon#L185), [banker](../../../areas/zon/rtun.zon#L186), [counter](../../../areas/zon/rtun.zon#L66)). |

## Residents and authored services

Static resets place the master piper `90306` at room `90329`, a vampire
battle-forger `90312` at `90366`, a gravedigger merchant `90308` at `90370`, a
wight weaponsmith `90325` at `90320`, and Nyroclex the Lich banker `90387` in
room `90387` ([resets](../../../areas/zon/rtun.zon#L96),
[piper and instruments](../../../areas/zon/rtun.zon#L106),
[forger and merchant](../../../areas/zon/rtun.zon#L145),
[banker](../../../areas/zon/rtun.zon#L186)). These names and placements map
the authored cast and locations; they do not establish live dialog, banking,
crafting, or shop behavior.

The `.shp` file has four records: piper `90306`, merchant `90308`, battle-forger
`90312`, and weaponsmith `90325` ([piper](../../../areas/shp/rtun.shp#L1),
[merchant](../../../areas/shp/rtun.shp#L39),
[battle-forger](../../../areas/shp/rtun.shp#L76),
[weaponsmith](../../../areas/shp/rtun.shp#L117)). The weaponsmith record has
no listed goods. The other stock lists include prototypes defined in other
active areas, described below. This is a mapping of source records and stock
IDs, not a qualification of customer access, restocking, sales, or crafting.

## Reset and cross-area dependencies

The zone file has 141 typed commands: 94 `M`, 20 `G`, 12 `D`, 10 `O`, and 5
`E`. The 20 `G` commands include four instruments (`36245–36248`) on the master
piper, eight reagent or material objects (`822–828`, `835`) on the
battle-forger, merchant stock/reset objects `53`, `500`, `377`, and `3050`,
three shovel `191` placements with miners, and one local object `90304`
([instrument resets](../../../areas/zon/rtun.zon#L106),
[battle-forger resets](../../../areas/zon/rtun.zon#L145),
[merchant resets](../../../areas/zon/rtun.zon#L154),
[miner shovels](../../../areas/zon/rtun.zon#L169)). Five `E` commands equip
local bone-lance prototype `90303` on skeletal lancers. Nine `O` commands
place local coffins and lanterns; the tenth places counter `3097` in room
`90387` ([room-object placements](../../../areas/zon/rtun.zon#L57)).

The referenced external prototypes are defined in active source areas:

| Reset or stock use | Prototype source | Source status |
| --- | --- | --- |
| Instruments `36245–36248` | [Llzazan objects](../../../areas/obj/llzazan.obj#L653) | `llzazan` is active in [`areas/AREA`](../../../areas/AREA#L174). |
| Merchant stocks and resets `500`, `377`, `191`, `822–828`, `835`, and `53` | [Heavens objects](../../../areas/obj/heavens.obj#L788) | `heavens` is active in [`areas/AREA`](../../../areas/AREA#L2). The listed IDs occur in its object source. |
| Merchant reset object `3050` and bank counter `3097` | [Verz1 objects](../../../areas/obj/verz1.obj#L850), [counter](../../../areas/obj/verz1.obj#L1199) | `verz1` is active in [`areas/AREA`](../../../areas/AREA#L15). |

Mobile `90305`, the local carrion crawler
([prototype](../../../areas/mob/rtun.mob#L58)), is also referenced by 17
static reset rows in active `mobs_underdark.zon` (for example,
[the first cluster](../../../areas/zon/mobs_underdark.zon#L146) and
[later cluster](../../../areas/zon/mobs_underdark.zon#L459)). Preserve this
prototype and its numeric identity while those inbound references remain.

Object `53` has additional definitions in the current `dalvik`, `hostel`, and
`mavulsk` component files
([Dalvik](../../../areas/obj/dalvik.obj#L681),
[Hostel](../../../areas/obj/hostel.obj#L695),
[Mavulsk](../../../areas/obj/mavulsk.obj#L873)). Their register rows mark those
components as outside the full `areas/AREA` list
([Dalvik](ZONE_COVERAGE_REGISTER.csv#L380),
[Hostel](ZONE_COVERAGE_REGISTER.csv#L403),
[Mavulsk](ZONE_COVERAGE_REGISTER.csv#L421)); do not treat those declarations
as loaded duplicates without separate loader evidence. The current active
source list does include Heavens’ `53`. Confirm owner and loader behavior
before changing the gravedigger stock or reset ID.

## Static connectivity and access boundaries

The source exit census records 172 raw exits, all 172 loader-resolved, with all
87 rooms in one weak local component. There are three incoming cross-area
records, two outgoing records, and two reverse-direction records; the row also
records two rooms with fall chance and no current-bearing rooms
([coverage row](ZONE_COVERAGE_REGISTER.csv#L255)). These are static source
facts, not proof of ordinary-player reachability or safety.

The outbound routes connect room `90376` to Surface room `574593` and room
`90380` to Surface room `576997`; both have reverse records in Surface
([pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L412),
[outbound exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1112),
[reverse exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1496),
[second reverse exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1504)). Heavens room `30`
has a north exit to `90346` with no reverse-direction record in the census
([pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L179),
[exact exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L10)). A separate route may
exist, but its safe return is not established here.

The `.zon` comments mark five paired closed doors, including the laboratory,
shrine/armoury, burial plot, crypt entrance, and Halls of the Dead, plus a
paired closed/secret ladder between `90325` and `90326`
([door-state declarations](../../../areas/zon/rtun.zon#L8),
[other paired doors](../../../areas/zon/rtun.zon#L16),
[additional paired doors](../../../areas/zon/rtun.zon#L32),
[crypt entrance doors](../../../areas/zon/rtun.zon#L40),
[Halls of the Dead doors](../../../areas/zon/rtun.zon#L48),
[secret ladder](../../../areas/zon/rtun.zon#L24)). Their effects on ordinary
routes require a dynamic review; the static graph does not account for those
states.

## Builder questions and recommendations

1. **Is the empty sidecar intentional?** Confirm whether `coverage: complete`
   with no contacts, stories, or exclusions is the intended record for this
   environmental crypt, especially given its generic “local requests”
   introduction. If it is environmental-only, document that scope explicitly;
   do not invent contacts, objectives, or rewards.
2. **Are the “King Load” and blank-description rooms intentional?** Confirm
   whether `90381–90385` are internal loading/test spaces or unfinished public
   rooms. Separately confirm the intended description and presentation of
   banker room `90387`, which currently resets Nyroclex and the external bank
   counter into a blank-description alcove.
3. **Is the Heavens entrance intended to be one-way?** Confirm the planned
   return route from Heavens room `30` and whether it depends on another exit,
   script, transport, or access condition. Do not add a reverse exit based on
   the static census alone.
4. **Are the listed doors and secret ladder states intended?** Review the
   five closed door pairs and the closed/secret ladder against intended public
   access before proposing a route change.
5. **Are imported item IDs and the inbound crawler resets intentional?**
   Preserve Llzazan instruments, Heavens stock/equipment, the Verz1 counter,
   and all 17 Underdark references to mobile `90305` until the respective
   owners confirm any replacement or migration.

## Safe resolution summary

Keep `rtun` and all current prototypes in place. Its active `areas/AREA`
membership, external shop/reset objects, inbound Underdark mobile references,
and cross-area exits are not attic candidates. Record the environmental story
and builder questions; leave the local quest/story arrays, stock, combat,
object placements, doors, and routes unchanged pending owner decisions. No
gameplay or migration change is proposed.

## Checkpoint 343 audit reconciliation

The [comprehensive RTUN audit](RTUN_AUDIT_REVIEW.md) now records reset-aware directed reach, the two inactive Surf approaches, complete bounded typed/code dependency closure, title/history, and 20 builder decisions. It supplements the earlier census here; it still does not qualify live route access, shop behavior, or safe return.
