# Ashrumite Village: comprehensive source story map

**Source-comprehensive, revision two — October 3, 2026. Gameplay qualification
remains open.** The [sidecar](../../../areas/story/ashrumite.story.json)
explains all twelve native exchanges as support services, with sixteen
contacts, every addressed topic and 21 optional material/history checks.
There are no authored quest achievement or daily units in this zone.
Revision one counted eight crafting exchanges as requests and excluded
four information/missing-prototype contracts. Revision two makes all twelve
visible with accurate service roles and explicit blockers. Native receipts
and contract identities are preserved; projected achievement totals change.

Active, ready accounting remains mandatory for discovery, encounters,
journals and new credit. All twelve paid exchanges remain guarded under
active accounting. Guidance, possession and a recovered receipt do not
prove personal mining, first acquisition, learned dialogue or the advertised
crafting campaign. **No native zone or quest repair ships in this checkpoint.**

## Evidence and review boundary

Reviewed all 25 [native blocks](../../../areas/qst/ashrumite.qst): twelve Q
and thirteen addressed M responses, belonging to four speakers. There are
no ambient M blocks. Reviewed all 153 [rooms](../../../areas/wld/ashrumite.wld),
66001–66153, including 111 exact prose groups, 34 numeric headers, 36
non-exit metadata groups and 177 exit families; all 53
[mobiles](../../../areas/mob/ashrumite.mob), 65
[objects](../../../areas/obj/ashrumite.obj), twelve
[shops](../../../areas/shp/ashrumite.shp), and 275
[reset commands](../../../areas/zon/ashrumite.zon) across 181 source families.
Resets comprise 118 M, 101 G, 28 E, 24 O and four D; all chance fields are
100 and reserved arguments zero. Caps, preceding-command success, roaming,
pickup and actual recipient/source episodes still affect availability.
Zone 660 has reset mode two; its catalog range starts at 65300 because of
the surrounding zone registry, while its actual rooms start at 66001.

The [reproducible review index](../../reference/zone-story-audits/ashrumite.md)
records the native identities, current classifications, material declarations
and fifteen literal assignments. Complete local identifier/name searches
and bounded shared execution review cover quest dispatch, guild gates,
guards/justice, teaching, shops, inn rental, pet handling, dump behavior,
mining, money-changing, janitor/drunk behavior, wonder effects and the disabled
wagon setup. No local custom crafting, disc-recovery, mining-defense,
forest-search or escort endpoint was found outside these contracts.

The surface approach, room 66140, has south/west boundaries to Surface
Realm 550481/550080, with north/east foreign returns. Mine junction 66087
has east/south boundaries to Underdark 701670/702069, with west/north
returns. Room 66149 instead points east to 224166, absent from the active
world. That room exists in inactive historical world variants; this is a
stale active boundary, not evidence that every approach to Ashrumite fails.
Legacy key field 2147483647 on flag-zero ordinary exits is unset metadata,
not a requirement for a missing physical key.

Active inventory verification confirms missing objects 4372, 66066, 66067
and reset inventory item 6089. The latter belongs to the thief teacher's inventory
reset at 66106; it is unrelated to the crafting reward identities. A similarly
named platinum disc 4022 exists in active Caves of Skelenak prototypes,
but the current mage contract accepts only 4372. No active reset or native
reward producer for 4022 was found. Neither its existence nor bartender
prose establishes a usable replacement source. Do not substitute by name.

## Intended progression and the actual contracts

Dialogue describes a coherent builder intent: recover silver ore, refine it
into five pure bars, commission a plain silver necklace, cut five different
gems, set the necklace, learn a disc rumor, recover the rare disc, then pay
for rainbow enchantment. Ring and earring setting are side services. This
is useful design evidence, but the actual numeric contracts do not implement
that progression. Existing prototype identities make several intended
corrections plausible; they do not select the disc source, prices, balance,
source ownership or campaign achievement policy automatically.

| Native line / giver | Exact current offering, in addition to copper | Copper | Actual output | Interpretation and blocker |
| --- | --- | ---: | --- | --- |
| 132 / smith 66040 | Raw gem 66032 | 10 | Different raw gem 66033 | Dialogue promises silver ore → pure bar. Both current kinds display `a raw gem`; this is not silver refining. |
| 138 / smith 66040 | Five copies of raw gem 66033 | 2,500 | Magical necklace 66049 | Dialogue promises five pure bars → plain necklace. Current output is already a charged rainbow staff. |
| 57 / jeweler 66021 | Raw gem 66039 | 20 | Amethyst 66044 | Current materials/output agree with one gem-cutting service. |
| 63 / jeweler 66021 | Raw gem 66040 | 20 | Tigers-eye 66045 | Current materials/output agree with a separate gem-cutting service. |
| 69 / jeweler 66021 | Raw gem 66041 | 20 | Diamond 66046 | Current materials/output agree with a separate gem-cutting service. |
| 75 / jeweler 66021 | Already-cut sapphire 66042 | 20 | Plain necklace 66047 | Acceptance prose describes a cut gem; current output is a necklace. |
| 81 / jeweler 66021 | Already-cut emerald 66043 | 20 | Ordinary gem-set necklace 66048 | Acceptance prose describes a cut gem; current output is a necklace. |
| 87 / jeweler 66021 | Already-set earring 66065 and ordinary necklace 66048 | 100 | Missing 66067 | Dialogue requests an earring and diamond. Current input kinds differ and the reward prototype is absent. |
| 94 / jeweler 66021 | Already-set ring 66064 and ordinary necklace 66048 | 100 | Missing 66066 | Dialogue requests a ring and diamond. Current input kinds differ and the reward prototype is absent. |
| 101 / jeweler 66021 | Magical necklace 66049, amethyst 66044, tigers-eye 66045, diamond 66046, plain necklace 66047, ordinary necklace 66048 | 1,000 | Real gold ore 66050 | Six items means three different necklaces and three gems, not a necklace plus five gems. Acceptance describes a beautiful necklace, but gives gold. |
| 18 / mage 66017 | Real gold ore 66050 and missing disc 4372 | 25,000 | Pyrite 66051 | Both the offered necklace and promised magical reward disagree with the actual kinds; the required disc is absent. |
| 157 / bartender 66041 | No items | 1,000 | Information only | Ranger/Tethir/kobold rumor. No disc, personal recovery, scripted search or ranger completion is implemented here. |

All givers remain after acceptance. Eleven mixed item/coin exchanges retain
the native `Unsupported durable offering` daily exclusion; the rumor retains
`No repeatable item offering`. Services earn neither story achievements nor
dailies regardless of these underlying native definition flags. The
[quest executor](../../../src/world/quest.c) rejects coin-only offers under
active accounting and rejects mixed durable offers before unsafe settlement.
These guards are required dependencies, not new regressions introduced by
this journal. Replay fixtures project recovered historical receipts; they
do not execute these unavailable paid services.

The smith's actual room sign says five silver, cost dialogue says one silver
and cites an old twenty-five-silver sign, and the contract charges ten copper
(one silver). The dossier records all three instead of choosing a new price.
The three matching gem-cutting contracts must not be shifted along with the
misaligned contracts by a blanket numeric-offset repair.

## Material identity, alternatives and current preparation

Ten raw gems, 66032–66041, share the same short name and keywords. Exact
inventory identity matters: the smith's five-copy service needs five instances
of 66033; five 66032 gems or five mixed raw kinds do not fit. The journal has
one count-five current check, with an optional earlier refining receipt.
The receipt is a producer clue; it cannot replace consumed or absent gems.

Local ground declarations place 66032 in 66035, 66033/66041 in 66034,
66039 in 66033 and 66040 in 66032, each with cap 25. These are predeclared
objects, not evidence of the player's skill-mining action. Ground declarations
for real gold 66050 and pyrite 66051 occur in eastern ore shafts 66111/66112.
Their visible short names match; only real gold fits the current mage contract.
The silver ore 66030 and pure bar 66031 prototypes are distinct from all these
kinds. Their intended refining relationship is not currently a native recipe.

The jeweler at 66060 carries/declares cut gems 66042–66046 with cap one and
plain/set necklaces 66047/66048 with cap 999; shop declarations also list
these supplies. The smith at 66081 carries/stocks finished ring 66064 and
earring 66065. Stock declarations are not a purchase receipt or a guarantee
that an item is available now. Shop and source generation require their own
active-accounting qualification; the journal does not supply materials.

Magical necklace 66049 has the same visible name as ordinary necklace 66048,
but is type-four staff with three rainbow charges. The current five-raw-gem
service is its producer, without a local reset source. Plain necklace 66047
is a third kind. The six-material recipe needs all three separately and only
its own current receipt produces gold ore. The mage recipe produces pyrite;
an earlier necklace or rumor receipt cannot repair this mismatch.

City of Brass Groyana 139125 at marketplace 139212 has additional 90-percent,
cap-one declarations for 66045 and 66048. These are optional foreign supply
leads, with no imported foreign quest ownership or required travel. Wonder
effects also select gems 66034–66043 and place them on the origin-room ground;
they do not select 66032/66033. The
[typed wonder path](../../../src/item/wonder_actions.c) and
[legacy path](../../../src/specs/specs.highway.c) need separate source/grant
qualification before being advertised as a reliable personal-recovery route.
The `HG 0 66048 0 1` line in an Avernus mobile is a race/home/class/spec/size
header, not a gem-set-necklace grant. The
[mobile loader](../../../src/world/db.c) confirms that distinction.

## Town access, support and quest-like orphans

The fifteen [literal assignments](../../../src/specs/specs.assign.c) comprise
the bartender's world quest binding; drunk, three cityguards, five guild guards,
janitor and money-changer; and explicit inn, dump and pet-shop room procedures.
Other quest speakers are bound from native data; teacher and shop roles use
their shared setup. The six guild teachers plus the jeweler have teacher flags.
The illusionist teacher also has the special-teacher flag; ordinary teaching
and accepted class advancement are not local Q objectives.

All five named guildguard prototypes 66022–66026 are absent from local reset
placements. Four assigned guard prototypes are unplaced, while the cleric
guildmaster 66031 is assigned the gate procedure and reset at entrance 66088.
The [shared gate](../../../src/specs/specs.guards.c) uses the actor's birth
room and commands: west from 66065 for shaman, east from 66088 for cleric,
south from 66028 for rogue, north from 66084 for sorcerer/summoner/conjurer,
and south from 66078 for warrior, with protected combat and trusted/hunting
exceptions. This is uneven wiring requiring a builder decision; it does not
prove class-restricted movement actually occurs at every described gate.
Do not infer a training quest or required guild membership from room prose.

Twelve shop definitions cover armorer 66015, weaponsmith 66016, mage 66017,
barreler 66018, general merchant 66020, jeweler 66021, smith 66040, bartender
66041, and cleric/warrior/thief/mage storekeepers 66042–66045. Bakery keeper
66019 and shaman storekeeper 66032 have reset stock but no local shop record.
Determine intended merchant roles before promising those purchase routes.
Guild race policies and actual stock remain separate from quest eligibility.

Gateguard 66002 at 66040 has the one-cap gate key 66002; jailkeeper 66039 at
headquarters 66069 has the one-cap jail key 66001. Both are ordinary keyed
door mechanisms, not rescue contracts. The gate and jail D resets set their
door states; actual unlocking and passage need accepted events. Shared
[town justice](../../../src/combat/justice.c) identifies Ashrumite's reporting,
guard and jail rooms, but no local bounty or prisoner-release Q exists.
The headquarters wall-map extra description is placeholder builder text.

Inn 66039 has the inn flag; additional room 66054 is explicitly assigned
rental. The stable at 66116 uses adjacent storage 66117, whose sole exitless
purpose is hireling inventory; cave spider 66004 is declared there. Do not
label its missing exits a player-route failure. The
[pet service](../../../src/specs/specs.room.c) guards purchase, rent and claim
while accounting is active. Its legacy claim path consumes ticket/payment
and announces a return while the `petrestore` call is commented out. Existing
owner-qualified persistence/restoration and atomic ticket/payment recovery
plans must cover this caller before claiming a working pet-return milestone.

The dump's drop reward is guarded under active accounting. The
[money-changer](../../../src/economy/currency_exchange_proc.c) has been retired
and redirects `list`/`exchange` to the Royal Bank, despite local bank prose.
[Skill mining](../../../src/economy/mining.c) and mine generation are guarded;
predeclared ore pickup is different. Drunk 66037 periodically speaks/acts,
while janitor 66036 picks up eligible floor objects; neither records a player
quest. The shared archer table names watchtower 66005 but no local archer
assignment was found; three cityguards are assigned different behavior.
[Wagon setup](../../../src/classes/mount.c) returns before initialization;
historical Ashrumite caravan comments are not an active transport service.
Unplaced alchemist/guards, mine beasts, orc and giant danger, jail fiction and
burned-clearance descriptions remain narrative/role decisions, without an
implemented defense, escort, monster-clearance or rescue completion here.

## Capability additions and qualification plan

Use static extraction to find exact contracts, quantities, producer edges,
resets, stock, key links, assigned procedures and broken references. Use the
builder sidecar to explain which edges are services, optional preparations,
supplied alternatives and intended stories. Automatic name matching cannot
decide that a raw gem means silver ore, select a replacement disc, set prices,
infer magic access rules or assert a campaign from acceptance prose.

- Qualify one atomic mixed-payment transaction with exact material UIDs,
  fee authorization/debit, recipient/source episode, output identity and
  immutable result/recovery. A fee failure, missing reward or replay must
  consume neither extra coins nor extra materials and publish no credit.
- Validate an intended repair matrix before rebinding contracts. Verify
  active required and reward prototypes and feasible source producers;
  preserve the working three gem cuts. Native contract edits change the
  content fingerprint and need explicit migration/rebinding review, while
  old receipt recovery still resolves the frozen original terms.
- Add readable same-name variant discrimination for raw gems, ordinary versus
  magical necklaces and real gold versus pyrite. Keep live checks keyed by
  exact kind/count/UID. Do not turn labels, history or a five-copy count into
  proof of a personal source or mining action.
- Qualify accepted skill-mining, ground recovery, shop acquisition, random
  effect birth and player gift as distinct source routes, with shared-cap,
  incarnation, nested ownership and drop-laundering cases. A random wonder
  selection or producer receipt alone is not a completed material grant.
- Separate accepted free dialogue from paid information settlement, selected
  rumor/session and learned-topic objectives. Map the Tethir/disc lead only
  after reviewing its actual source and recipient endpoints.
- Choose an optional all-stage crafted-necklace achievement only after native
  repairs and payment/source qualification. Define supplied-item acceptance
  separately from personally refined/cut/set/enchanted lineage. Twelve
  services do not automatically become twelve quest achievements or dailies.
- Qualify actual class gating/teaching, keyed access, justice events and pet
  restoration only if selected as objectives. Reuse the planned accepted
  access, actor/effect, owner-qualified pet and travel adapters; do not add
  fictional milestones just because a room describes those actions.

## Pending native repairs and news handoff

**These are findings and proposed repairs, not shipped fixes.** Some choices
involve intended balance or roles and need builder selection. Record every
implemented native repair in a clearly named separate fix commit where
practical, and the PR's **Zone and quest repairs (news)** section, including
player trigger, before/after behavior, validation and a news sentence.

| Finding | Fair assessment and proposed work | Required focused proof before a news claim |
| --- | --- | --- |
| Silver and necklace contracts | Align ore 66030 → bar 66031 and five bars → plain necklace 66047 if that intent is selected; choose one consistent price/sign. Current gem contracts are explicit and still guarded. | Valid sources, exact five copies, fee/output settlement, failure/replay and correct sign. |
| Two gem cuts and setting recipes | Select raw inputs for sapphire/emerald; ring 66062 + diamond 66046 → 66064 and earring 66063 + diamond → 66065 are plausible prototype pairings. Do not invent missing 66066/66067 without a reason. | All five cuts, each plain/finished jewelry identity, exact diamond, valid outputs, accounting/recovery and retained unrelated cuts. |
| Necklace setting and enchantment | Plausible intended setting is plain necklace 66047 plus gems 66042–66046 → ordinary necklace 66048; enchantment would produce 66049. Select a real disc and feasible source deliberately. | Every distinct material, correct output/charges, supplied and personally crafted alternatives, missing-disc failure and atomic payment. |
| Stale east exit | 66149 → inactive 224166 has no active destination; four other boundary edges resolve. Select an active destination or remove only the stale edge. | Outbound and intended return travel, ordinary approach and preserved unaffected routes. |
| Inventory item 6089 | Thief teacher has a missing equipment reference. Choose a valid intended item or remove that one inventory reset, without changing teacher behavior casually. | Active prototype/reset validation and teacher inventory; no claimed quest reward repair. |
| Guild guards and merchant roles | Five guard prototypes unplaced; four procedure bindings dormant; cleric teacher does guard duty. Bakery and shaman-store stock lack shop records. Determine deliberate role/setup intent. | Correct actor placement, birth-room/class behavior, no duplicate guard ownership, and genuine shop purchase/stock behavior if selected. |
| Bank/map/sign prose | Money-changing prose is historical, wall map placeholder and smith prices inconsistent. Update the specific claims after behavior/price decisions. | Text matches the verified service and route; keep cosmetic news separate from gameplay repair. |
| Legacy pet claim | Current accounting guard prevents the legacy claim path. Restoration is commented out despite payment/ticket consumption and a success message. Repair ownership, persistence and atomic return before enabling. | Owner and foreign ticket, successful restoration, missing/corrupt pet, duplicate/restart recovery, preserved tickets/coins on failure. |

## Verification and remaining limits

Production regression verifies all twelve terminal bindings, service roles,
four speakers/thirteen addressed families, exact five-copy requirement,
same-name material distinctions, missing-reference warnings, source parents,
four working boundaries/stale fifth, guild-role and merchant evidence.
Native journal regression checks encounter visibility, exact current kinds
and quantities, eleven generic payment warnings plus the paid-rumor guard,
read purity, optional producer history, service-only receipt recovery and zero
zone/daily achievement credit.
Catalog checks preserve all 2,668 native definitions, revision-two content
fingerprint/registry and the other 57 maps.

These are source/projection and persistence checks. They do not qualify fresh
paid crafting, personal mining, live source stock, foreign disc recovery,
actual gate travel or pet restoration. Keep every existing accounting guard.
The roadmap is now 37 of 220 source-comprehensive areas, with 183 remaining;
58 authored journals project 1,713 achievement units, 1,498 potential daily
units and 2,227 total rows. Continue with The Hall of the Ancients (`hall`).
