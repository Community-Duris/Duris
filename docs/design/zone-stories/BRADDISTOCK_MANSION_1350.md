# Braddistock Mansion (brad): comprehensive source map

Reviewed October 4, 2026. Zone1350, `brad`; roadmap priority80.
**Source review is comprehensive; played gameplay qualification remains pending.**
Active, ready accounting is required for all new discovery, encounter, journal,
achievement and daily credit. Frozen reward recovery is separate.

This mansion is distinct from [older Braddistock](BRADDISTOCK_MANSION.md),
zone13/`braddistock`. Its [schema3/revision2 journal](../../../areas/story/brad.story.json)
retains five existing story IDs and native bindings, with16 contacts/all32
addressed aliases and ten optional checks: nine current material checks and one
earlier Isabia receipt. Five cards explain exchanges physically in the
[Tower of Darkness](TOWER_OF_DARKNESS.md), whose giver VNUMs currently assign
credit to Braddistock. Local mansion exploration is guidance without an invented
quest terminal. One actual native entry-message repair ships in a separate fix.

## Reviewed evidence

- All14 owned [native blocks](../../../areas/qst/lortower.qst): M9/Q5,
  nine addressed families/32 aliases. There is no `brad.qst`. Full acceptance,
  outcome, inputs, rewards and D flags were read. These five contracts are
  declared and placed in the Tower, while the canonical owner is zone1350.
- All48 [rooms](../../../areas/wld/brad.wld),135001–135048:47 full
  title/prose families,five headers,99 exact exits/83 complete exit-text
  families and26 extra descriptions/25 families. All prose, directions,
  keywords and extras were read. There are no room F/T records. Header
  135048 2 4 10 20 1 and registry134142–135048 retain reset mode2.
- All14 [mobiles](../../../areas/mob/brad.mob),135001–135014, including
  all14 complete prose families; all73 [objects](../../../areas/obj/brad.obj),
 135001–135073, including complete E/A metadata. Types:15 treasure,
 15 container,13 trash,11 armor,seven weapon,three potion,two key,two wand,
  one each type37,drink container,food,light and boat. No local type25 portal,
  type29 switch or object T record establishes another quest event.
- All175 [resets](../../../areas/zon/brad.zon): D10/O74/P26/M48/E17,
 116 exact/125 parent-aware families. Duplicate placements, caps, probability,
 parent containers and equipped children remain distinct. The static corpse
 has nine P children, not a recorded personal kill. No local shop file or
 explicit inn room flag was found.
- The one literal local assignment is mob135014 →
 [braddistock](../../../src/specs/specs.assign.c#L343). Its full
 [implementation](../../../src/specs/specs.braddistock.c) was read, along
 with shared command/awake-NPC dispatch and descriptor-based message delivery.
 The same file defines `jet_black_maul`, but its actual assignment is older
 object1372, not local maul135071. Do not attach a SAY titan objective here.
- Bounded global scans found five relevant foreign recipes, seven material
 producer reset groups and five recipient groups, all in `lortower`. Full
 twelve input/reward prototypes, twelve recipient/source actors, twelve
 placement rooms and all twelve reset groups were reviewed; shared ration364
 and torch398 prototypes were also read. This bounded closure reuses the
 comprehensive Tower dossier without reclassifying another zone as newly done.
- All713 active portal prototypes were inspected for fixed arrivals: none
 points into this mansion. There are two boundary edges/one complete foreign
 neighbor: mansion road135001 south ↔ Tharnadia sea wall132650 north.
 No relevant foreign reset supplies local objects/actors; local iron key135050,
 maul135071 and sword135073 have no identified fresh reset or Q producer.
 These are availability findings, not permission to manufacture new sources.
- Shared exact loose-item offering, quantities, receipt ownership/recovery,
 GET/OPEN/READ, hidden search, door/key/container state and reset parent selection
 were traced. [Generated navigation](../../reference/zone-story-audits/brad.md)
 retains exact source links; raw declarations do not prove admission or live stock.

## Mansion exploration story and actual prerequisites

| Phase | Source and route | Meaning and limit |
| --- | --- | --- |
| Approach and level gate | Road135001, spirit135014; NORTH to entrance135002 | Ordinary actors above level14 are blocked. Trusted staff, null combat callbacks, unrelated directions and self-commands bypass. No hand-in unlocks admission. Repaired speech now reaches the blocked player. |
| Occupied-house clues | Entrance tracks extra; library135004 has closed tome135003 with note135004 | OPEN/GET reveal the note pointing beyond skeletons. Tome flags5 mean closed/closeable, without locked; key-1 does not forbid opening. LOOK/READ is guidance, not a stored learned objective. |
| Upstairs investigation | Key135016 on floor135026; south door to135027; Ned135003 with dagger135017 | Actual scratched key matches the bedroom lock. World kind2 is valid pickable door encoding, not a malformed missing door flag. South resets closed/locked; reverse north resets closed/unlocked. No Ned rescue or reward controller exists. |
| Household recovery | Broken desk135005; metal box135011; closed chests135018; fireplace135014 | Pots, ring, crystal, diamonds and supplies are ordinary nested loot. Locked key0 desk/box can use normal PICK/KNOCK subject to admission and success. Furniture need not be carried. No collection terminal is bound. |
| Secret cellar | Winecellar135035 south →135036 is secret/closed;135036 east → skeleton room135037 | Search/open actual doors. Bar wording is not an extra custom bar-state procedure. Corpse135024 is a static container with armor/sword/shield/dagger/backpack/waterskin/ration/torch/emerald children; no personal corpse/death proof. |
| Beyond the skeletons | Secret/closed east135037 → lab135038 | Alchemy book135035, gold skull36/apple37/rose38/weights39, skeleton40 (NOLOCATE, not invisible/secret) with pebble42, table41 with scroll43. Five weights form one object. No alchemy transformation, instruction-completion or lab finale is implemented. |
| Private quarters | Lab route to135039; table44/scroll45; locked strongbox46, key50 | Strongbox holds three yellow/two green potions. Missing identified fresh key source does not prove a hard blocker: normal PICK/KNOCK may open this non-pickproof box. Builder decides intended key supply before any repair. |
| Smuggler stores | Sanbalet135009 in135045, seafaring/human smugglers and gnolls in lower caves | Sanbalet's boots/rod and silk/brandy stores are equipment/treasure, not a named reward exchange. Defeating him or gathering cargo records no authored campaign outcome. |
| Sea cave | Room135048, glowing boat135052 | Boat is type22, not a portal. Only the west return exit exists. Tide, voyage and weightless descriptions have no corresponding implemented route/effect; builder must define the endpoint before credit or topology changes. |

All14 local creatures are accounted for: spiders, centipedes, stirges, cellar
rats, giant weasels and ants provide ordinary encounters; Ned, two smuggler kinds,
skeletons, Sanbalet, gnolls and wandering goblins inhabit the exploration route;
the spirit alone has the local bound procedure. No local native Q/M interaction
or automatic quest completion was found. Ordinary weapon, armor, potion, wand,
food, light and container behavior remains support, not story completion.

## Five preserved Tower exchanges

| Existing story / native block | Exact offering → outcome | Source, optional guidance and limit |
| --- | --- | --- |
| Azlion134146, Q201; request-134146-a4f6aa87c7b1 | Staff134105 → broadsword134106; D1 | Staff E18 on excited conjurer134148 at134114; Azlion at134112. Recover loose, then deliver. Reward is named Redemption of Light. Circle/rift text does not produce a new portal. |
| Jenifer134150, Q218; request-134150-cc22370abf33 | Head134006 → E100000; D0 | Ogre134004 carries G head at foyer134010; Jenifer at farmhouse134127. Fixed head proof does not require this player's kill/decapitation/first source history. |
| Darrin134162, Q266; request-134162-0aa9fcadf7a5 | Different pieces134131/132/133/134/135 → Star Key134125; D1 | Kinslor134040/134018; Mordren134062/134029; Cardinal134081/134039; Earlion that Is134091/134046; Urian134133/134122. Five different loose roots together. Darrin at134138. Receipt and retirement do not defeat Sargon. |
| Danthas134167, Q292; request-134167-d86a832b706b | Ring134144 → leggings134145; D0 | Isabia Q304 is the identified producer. Optional earlier receipt explains the route; supplied ring is valid. Danthas starts134140 and can wander through outward exits; actual visible availability is unqualified. |
| Isabia134169, Q304; request-134169-05c42346bab0 | Bone key134048 → ring134144; D0 | Cardinal carries G key alongside piece3. Isabia at134042. Key is type12 OTHER, ring type13 TRASH; exact identity is valid despite names/types. Farewell does not remove Isabia or record an escort. |

The Cardinal's bone key also opens a Tower chest; spending it on a hand-in and
using it on a door/container are different facts. Staff is type4 STAFF in held
slot18. Star pieces and Star Key are type12 OTHER; `has_key` accepts exact VNUM
without requiring ITEM_KEY. Current loose supplies do not certify original source,
personal combat, accepted ASK, doors, portal travel or earlier completion.
An old receipt does not recreate spent items. Ten optional checks display
preparation; only the five native terminal bindings award the five story outcomes.

Nine addressed families retain32 aliases: Azlion's urian/sargon/power families,
Darrin's urian/tower/sargon/key families, Danthas's noble/isabia/elf and Isabia's
key/prison. Jenifer and local mansion contacts have no mapped ASK response.
Aliases are alternative topics, not one achievement apiece.

Darrin's Star Key fits the hill down door/keystone and the prison route. Rift of
Exile134137 reaches134128; onward prison/Sargon/return travel is separate from
issuing a key. Azlion's narrated rift, Darrin's sacrifice and Isabia's farewell
are current lore, without corresponding campaign, newly spawned portal or escort
terminal. See the complete Tower dossier for native access/password/portal routes.

## Ownership, discovery and renewal gap

Both [catalog ownership](../../../scripts/zone_story_quest_catalog.py#L155) and
[runtime ownership](../../../src/world/zone_story_quest_production.c#L222)
use giver ranges. Tower's top is134141; Braddistock begins134142. Consequently
all five contracts retain zone1350 ownership while physical placement is1340.
Mapping validation currently rejects adding those native bindings to the Tower
journal. A Tower encounter does not discover the mansion or expose its journal.

Static daily candidacy also uses owner reset mode2, while the Tower's physical
reset mode is0. Azlion and Darrin retire on success but remain potential daily
candidates under the current catalog. Neither this journal nor the source scan
proves renewable physical recipients. Fixing this requires explicit physical
affiliation, discovery/referral policy and actual actor/source reset episodes,
with frozen canonical receipt identity and versioned compatibility tests.
No ownership migration, discovery fabrication or daily eligibility bypass ships.

## Actual native repair and news

Separate **[fix5b4c4f34a](https://github.com/Community-Duris/Duris/commit/5b4c4f34a151f624d21558fb86cb815f05cf76b1)**
changes exactly two lines in [the entry procedure](../../../src/specs/specs.braddistock.c#L97):
the spirit's existing refusal speech is sent to the blocked player rather than
the NPC, and receives CRLF. With an ordinary NPC lacking a descriptor, the old
`send_to_char(..., ch)` speech disappeared at the shared descriptor boundary.
The player already received the separate passage-block action; now they also
receive the spoken reason on its own line. Gate policy, threshold, observer
messages, directions, staff bypass, native recipes and all area bytes are intact.

The [focused executable regression](../../../tests/async/test_braddistock_entry.py)
extracts the maintained production function and compiles it as C++20 with isolated
character/message stubs reflecting the actual descriptor boundary. Original
source fails; repaired source passes level15 block/exact speech/player recipient,
three unchanged action audiences/actors, level14 admission, trusted level59,
unrelated SOUTH, null callback, periodic registration and self-command cases.
Maintained server build, exact native-byte scope, changed/staged formatting and
whitespace checks pass. This does not claim a played network entry/staff journey.

**Suggested news:** “Braddistock Mansion's spirit now delivers its entry refusal
directly to the blocked player.”

## Remaining capability work and fair repair proposals

| Finding | Evidence and boundary | Plan and qualification |
| --- | --- | --- |
| Physical story versus canonical owner | Five Tower exchanges credited to Braddistock; encounter discovery and reset mode differ | Explicit physical affiliation/referral rendering; retain immutable receipt owner. Test discovered Tower/undiscovered mansion, multiple locations, cold upgrade, erasure, actor retirement and actual physical renewal before daily admission. |
| Learned clues and successful access unrecorded | Note/books/extras, hidden exits, containers and key use currently affect guidance/readiness only | Accepted LOOK/READ/SEARCH/UNLOCK/PICK/KNOCK/OPEN/GET events with actual target/root/parent/source revision and committed transition. Cover failure, blindness, empty target, stale/shared state, reset, retry, cold recovery and supplied gifts. |
| Source versus handoff | Static corpse/containers, held staff, fixed head/five pieces/ring have different sources | First admitted source/custody facts per UID; source admission versus loot versus gift. Qualify parent lineage, multiple copies, equipped removal, partial bundle rejection and atomic reward/root consumption. |
| Narration exceeds native endpoints | Rift, rescue, sacrifice, Sargon victory, alchemy and smuggling voyage lack terminal controllers | Builder defines personal/group attribution, stages, actor/world transitions, alternative routes, finale and renewal. Add adapters only after agreed semantics and accounting/recovery proof; no lore-string achievement. |
| Missing local supply or unbound equipment | Key135050, maul135071 and sword135073 lack identified fresh producer; maul procedure belongs to older1372 | Builder confirms intended supply/use/retirement. Strongbox PICK/KNOCK alternatives mean missing key is not proven inaccessible. Do not copy another zone's procedure solely because names match. Any selected repair gets its own fix commit and news. |
| Static clue discrepancies | Library topic refers to philosophy though tome is magical properties; boat claims weightless but weighs70; high-tide/sea voyage only prose | Confirm intended prose/mechanics first. Choose minimal text repair or designed route/effect with actual output/action regression and distinct news. These remain proposals. |

Validation: complete local/bounded foreign source/schema fixture; full production
catalog/inventory/audit regression; all98 Python/C++ journal file-loader/projection
journeys; focused native entry regression; maintained build; formatting/whitespace,
source links, five preserved IDs/bindings,96 unchanged other journals plus the explicitly corrected Fields journal,2668 native definitions,
fingerprint/content revision2/registry, original220 queue and earlier PR/news
preservation. Synthetic receipts qualify projections, not played source/access,
combat/travel/offer/settlement/actor renewal/database persistence or daily renewal.

Catalog totals remain98 journals/1591 achievement units/1446 potential dailies/
2200 story rows. Queue:80/220 source-comprehensive,140 pending; The Desert City
of Venan'Trut (`desert`) next. Full goal remains active. No accounting activation,
DB/server operation, migration, deployment or merge occurred.
