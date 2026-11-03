# Quietus Quay: comprehensive source story map

Reviewed October 2, 2026. Source area `quietus`, zone 17, journal revision 2.
The local source contains 95 rooms, 54 mobiles, 55 objects, 16 Q contracts,
14 addressable M responses, five shops and 216 reset commands. The journal
retains four separate mission achievements and seven supporting service rows.
All native bindings, categories, receipt identities and economic terms survive.

## Evidence boundary

Reviewed the complete active [Q/M file](../../../areas/qst/quietus.qst),
[rooms](../../../areas/wld/quietus.wld), [mobiles](../../../areas/mob/quietus.mob),
[objects](../../../areas/obj/quietus.obj), [resets](../../../areas/zon/quietus.zon)
and [shops](../../../areas/shp/quietus.shp). The
[reproducible audit index](../../reference/zone-story-audits/quietus.md) records
exact native contracts, dialogue families, resets and special assignments.

The four literal assignments in [specs.assign.c](../../../src/specs/specs.assign.c)
are bartender 1709's `world_quest`, shipwright room 1719's `ship_shop_proc`,
Sharkbait Inn room 1736's `inn`, and Dark Tavern room 1734's `crew_shop_proc`.
Reviewed their implementations in
[specs.world_quest.c](../../../src/specs/specs.world_quest.c),
[ship_shop.c](../../../src/ships/ship_shop.c) and
[specs.room.c](../../../src/specs/specs.room.c), including the reachable
ship transaction helpers and hull callback. Shared implementations are not
evidence that every other port or world-quest giver has been fully mapped.

Also reviewed [world_quest.c](../../../src/world/world_quest.c),
[shop.c](../../../src/economy/shop.c),
[utility.c](../../../src/core/utility.c),
[currency_transaction.c](../../../src/economy/currency_transaction.c),
[economic_gameplay_authority.c](../../../src/economy/economic_gameplay_authority.c),
[quest.c](../../../src/world/quest.c),
[item_movement_transaction.c](../../../src/item/item_movement_transaction.c),
[db.c](../../../src/world/db.c), ordinary locks/container access and liquids in
[actmove.c](../../../src/cmd/actmove.c) and
[actobj.c](../../../src/cmd/actobj.c), and wandering in
[mobact.c](../../../src/mob/mobact.c). Bounded foreign reads establish Rodev's
sword, Ceothian credentials and the mansion's seal/staff, without claiming a
comprehensive map of Sarmiz or Ceothia. The mansion has its own
[source dossier](KRIMENEHAS_MANSION.md).

## Suggested progression and actual admission

The narrated route is Darvanu introduction → officer briefings → independent
lieutenant contracts → captain's proof-of-skill briefing → secret bloodstone
mission. This is useful guidance, not an enforced membership or rank state
machine. No local Q demands earlier Q receipts, learned topics, a personal
kill, all three lieutenant contracts, or an allegiance flag.

All native offerings here are item-only. The four terminal deliveries retain
existing native accounting; their history records accepted offerings and
rewards, not the source of those materials. Supplied proof items work. Display
optional briefing receipts alongside live materials without letting optional
history displace the required next action. A future complete-Darvanu campaign
needs an explicitly authored all-stage policy and cannot use today's any-of
`contracts` array to imply completion of all missions.

The thin duergar 1728 begins on the barracks path at 1758. The drow lieutenant
1734 begins on path 1768; the orcish lieutenant 1735 is in Officers' Quarters
1770; the angry **duergar** lieutenant 1736 begins on path 1782. The captain
1749 loads in special room 1784 and may reach Spartan Home 1783. His load and
current availability are discussed below. Contacts are revealed after encounter;
the journal's officer/source hints are not a global live NPC locator.

## Credentials: five briefings, two accepted items

Each of these five services accepts either **badge of the Darvanu 1701** or
**blackened steel longsword 80808**. Those are distinct item types/prototypes;
the longsword alternative is executable even though the text calls it a badge.

| Recipient | Supporting outcome | Item return / extra reward |
| --- | --- | --- |
| Thin duergar 1728 | Membership introduction and direction to officers | Matching credential |
| Drow lieutenant 1734 | Aresliean mission note | Matching credential plus short note 1732 |
| Orcish lieutenant 1735 | Rodev sword briefing | Matching credential |
| Angry duergar lieutenant 1736 | Eckraldu briefing | Matching credential |
| Captain 1749 | Request for proof of skill | Matching credential |

These ten native alternatives project five service rows, not ten completed
missions. Darvanu guard resets equip the badge, including the thin duergar,
officers and captain. Ceothian guard resets equip the longsword, sometimes in
both wield slots; its source is [ceothia.zon](../../../areas/zon/ceothia.zon),
not a local hidden badge conversion. Gifts satisfy either route.

The badge is an ordinary **container**, capacity 2, flags 13 (closeable, closed,
locked), key 0. The captain's reset sequence includes a **gold key 1742** put
inside badge 1701. `P` chooses a live matching container through `get_obj_num`,
not a proven parent occurrence attached to that exact captain. Do not promise
that every badge contains the key or identify its owner from command adjacency.

Native `R I 1701` creates a replacement badge; it does not return the original
UID or copy its contents. Durable offering submission captures the full input
tree, sends it to destruction, and publication extracts the root. Reward
creation then supplies a new empty badge. Thus an accepted filled-badge
briefing can consume the key or other contents while prose says "hands it back."
The current journal tells players to empty badges before offering them.

**ZSQ-CONTAINER-OFFERING:** reproduce empty/filled credentials with exact child
UIDs under active accounting. Builders should decide whether to preserve the
original tree, transfer contents safely, or reject a nonempty credential before
submission. Do not merely copy contents into a new badge after retirement or
change stable native receipt terms without a migration/content policy.

## Mission: Gr'Zz'Lien and Aresliean

The drow's credential service supplies **short note 1732**. Her terminal
requires that note **and bloody head of Aresliean 1746 together**, and awards
**sleek mithril dagger 1747**. She does not disappear. A note alone is preparation;
a head alone is incomplete; no other pirate head qualifies.

The actual reset carries head 1746 on Aresliean 1751, with `G`, after his `M`
at room **1784**. It is preloaded inventory, not an item created by an encoded
personal killing. Four pirate/bartender families discuss foreign-port rumors,
the coup and Darvanu dominance. They are contextual leads. The source does not
place Aresliean in Frielehold or Tharnadia simply because an M response says
someone saw him there. No foreign reset of this mobile was found in active
area resets. He can still move; prototype owner, reset origin and current
location are separate facts.

The dagger supplies the captain's alternative proof-of-skill briefing. That
service does not consume the original drow achievement, grant a second mission
achievement, or require that the same player earned the dagger personally.
Revision 2 adds optional note preparation and names both exact delivery items.

## Mission: the orcish lieutenant and King Rodev

Offer **doubling sword of Chaos 9436** to lieutenant 1735. He awards **massive
mithril sword 1750**, **obsidian necklace 1748** and **obsidian bracer 1749**,
then disappears. His remaining gear is extracted by the shared Q departure
path. The mission is independent of the drow and angry lieutenant contracts.

Rodev mobile 9448 loads at **Throne Room 9962** in Sarmiz and has the sword in
wield slot 16 through `E 1 9436 1 16 50 ...`: ordinary equipment chance is 50%
and live sword cap is one. This is an intermittent source, not a missing
prototype or a guaranteed sword on every king episode. The throne room and
its ordinary exits are in [sarmiz.wld](../../../areas/wld/sarmiz.wld).
The mission does not require a castle key or encoded personal kill receipt.
The journal supplies the verified foreign lead and accepts supplied swords.

## Mission: the angry lieutenant and Eckraldu

Offer **Eckraldu's staff 16450** to angry duergar 1736 for **long silk cloak
1751**. He remains available. His request describes killing Eckraldu and
incidental humans, but the native contract only consumes the exact staff.
No extra kill, guard-duty timer or later assigned mission is encoded.

Eckraldu 16423 equips the staff at mansion room 16486 and can roam through
the mansion's links toward Misty Vale, Twin Towers and Woodseer. The source
lead is the mobile, not a guaranteed stationary object in his birthplace.
Lord Krimeneha also consumes this staff to supply the fragment for his spirit
route. Preserve exact competing consumption: handing this staff to the angry
lieutenant does not also prepare the mansion's fragment. A gift remains valid
proof without a mansion briefing, personal recovery or earlier story history.

## Captain's proof services and bloodstone finale

The captain's credential briefing requests **Lord Krimeneha's Seal 16429** or
other proof. The seal is nested in **ornate desk 16428 at mansion room 16459**;
it is distinct from haunted boy's gold ring 16452. The desk is closed/locked,
ordinary pickable, key 0, and has a data-driven opening trap. Its route and
hazards belong to the mansion rather than becoming Quietus achievements.

The seal offering **consumes the seal and supplies no item reward**. It still
delivers a meaningful secret briefing. The other route accepts dagger 1747
and supplies a replacement dagger 1747 while giving a similar briefing.
These are two service rows, with any-of optional preparation for the finale.
They are not two additional terminal missions or an enforced faction switch.

The final contract requires only **Quietus' bloodstone 1730** and awards
**pulsating amulet 1752**, **gold horn earring 1753** and **smooth black ring
1754**. The captain then disappears to report. No credential, seal, dagger or
lieutenant receipt is required for that final native offering.

Quietus 1729 loads in his estate at room **1761**, carries bloodstone 1730,
and is followed by bodyguard 1730 through the `F` reset. The vampire woman
1731 at 1763 does not carry this proof. A bodyguard, vampire prose, payment
contract and final delivery are different evidence. The contract does not
persist a coup, promotion, new pirate allegiance, threat of retaliation, or
personal killing. Its narration may be intended closure; builders must decide
before adding gameplay effects or a complete political campaign achievement.

Secure any desired captain badge/key before his departure; disappearing Q
givers lose remaining inventory/equipment. The journal warns about availability
and final departure without requiring the optional treasure route.

## Accepted dialogue, rumors and abandoned narrative leads

All 14 native M families have representative topic guidance in ten contacts:

| Mobile | Native topic families | Role |
| --- | --- | --- |
| Graceful pirate 1704 | `aresliean` | Pirate/coup rumors |
| One-eyed bartender 1709 | `aresliean` | Foreign-port rumors; separate world-quest service also recognizes `quest` |
| Thin duergar 1728 | `darvanu mercenary mercenaries` | Membership and rules |
| Wealthy pirate 1732 | `aresliean`; `darvanu` | Trade interests and port control |
| Drow lieutenant 1734 | `duty duties job`; `aresliean` | Credential request and pirate leads |
| Drunken rakshasa 1737 | `aresliean`; `drunk despair sad`; `god` | Rejection, despair and lost divine powers |
| Captain 1749 | `krimeneha` | Proof-of-skill lead |
| Aresliean 1751 | `name`; `aresliean`; `darvanu` | Identity and threatened counterdeal |

The orc and angry duergar have Q briefings but no addressable M family here.
Repeated aliases are one family, not multiple achievements. These responses
remain printed lore; no accepted-topic historical adapter is shipped. The
drunken rakshasa's failed divine task has no recovery Q or assigned local
restoration procedure. Pirate disapproval, a threatened human deal and the
captain's promised rank do not establish unimplemented durable objectives.

## Random load and current source availability

Captain 1749 and Aresliean 1751 both use **1784**, not unused Aresliean load
room 1794. Room 1784 has four exits to **Mob Trap Room 1781** and a down exit
to **Spartan Home 1783**; it has no up exit. Room 1781 has no outgoing route.
Ordinary wandering checks existing exits and `ROOM_NO_MOB`; neither the load
room nor trap forbids mobiles. These mobile flags allow ordinary roaming.

The enchanted bat 1748 loads at **1780**, with five exits into 1781 and a west
exit into **Bat Cave 1779**; it carries pulsating stone 1741. These special
room descriptions explicitly describe chance-based loads and a "wrong decision."
Some apparent unavailability is deliberate rarity. Do not assign an exact
spawn probability from exit counts alone: movement cadence, failed directions,
combat and other state affect timing. A live trapped episode still counts
toward a mobile's ordinary cap and can suppress a fresh source.

Unused room 1794 includes exits to `-1`, which `setup_dir` rejects; they are
not dynamic travel to a human port. No active local reset uses that room.
Record it as a placement/retirement review lead, not the active head source.

**ZSQ-RARE-SOURCE-AVAILABILITY:** reproduce captain, pirate and bat episodes,
including failed directions, waiting, trap occupancy, death and ordinary reset.
Builders should decide whether shared captain/Aresliean placement is intentional,
then retain rarity through explicit availability policy or repair placement/
return/reset lifecycle. Do not blanket-remove intentional random load behavior.

## Supporting exploration and services

| Route | Verified mechanics | Story treatment |
| --- | --- | --- |
| Minotaur/ebonwood chest | Minotaur 1725 at 1746 carries key 1709; chest 1708 at 1752 holds rug 1713 and belt 1714 | Optional treasure, no Q consumer |
| Estate desk/dresser | Desk 1724 at 1762 holds golden key 1725; dresser 1729 at 1763 holds gold chest 1726 with jade 1727 and coin pile 1728 | Optional nested treasure; no coup prerequisite |
| Captain's badge/key | `P` puts gold key 1742 into a matching badge; closed/locked/pickproof wooden chest 1743 at 1783 holds buckler 1744 | Exact source-parent identity needed; optional to all four missions |
| Rubble | Container 1717 at 1756 holds bracer 1718 | Ordinary recovery, no scripted cleanup achievement |
| Golden Lyre painting | Secret west exit 1774 → 1775, reset closed/unlocked | Hidden instrument shop, not a mission gate |
| Hidden bat shrine | Secret east wall 1776 → 1778 → 1779; shrine 1740 and rare bat's stone 1741 | Lore/treasure; no assigned local shrine completion |
| Flind house | 1789 south door is locked, key 0 and ordinary pickable; reverse door starts closed/unlocked; Flind 1750 at 1790 has globe 1745 | Optional ordinary access; no local finale |
| Green granite fountain | Drink-container 1706 has infinite unholy water, liquid 28, poison field 0; shared drinking heals evil alignment and harms good alignment | Property-driven effect, no fountain cure/investigation Q |

Chest 1708 has flags **25**, and nested gold chest 1726 has **27**. Both have
locked/pickproof bits but **lack the closed bit**. Shared get preflight checks
closed, so their keys are not established prerequisites for initial retrieval.
Their outer containers or access can still matter. This may be intentional open
treasure or content drift; builders should reproduce and decide before changing
flags. Wooden chest 1743 has **29**, including closed, so its gold-key target is
a different actual access state. A room's chest description is not a live lock
predicate. Preserve current executable flags in hints.

Five shopkeepers are one-eyed bartender 1709 (1734), Blierthro 1712 (1735),
trolless 1726 (1755), skeletal bartender 1738 (1774), and bard 1744 (1775).
They sell ale, ordinary supplies, two potions or six instruments. Shared shop
dispatch preserves the bartender's secondary world-quest procedure. Under active
accounting the ordinary shop trade/service guard deliberately refuses mutations;
list/lore and world-quest requests do not become a purchase accomplishment.

The Sharkbait Inn uses the shared rent/save flow: after admission checks it
changes the home/stupor state, requests terminal persistence, restores those
values on save failure, and extracts after success. This existing save ordering
is useful and must be preserved. Staying at the inn is not an officer mission.

Global active reset/contract scans found no foreign reset of local mobiles and
no foreign native Q consumer of local Quietus items. Ordinary Quietus objects
are reused by resets in Heavens, Winterhaven, Alatorin and Surface Keeps. Those
copies are not additional mission sources for bloodstone/head/note. Prototype
ownership and source occurrences must remain distinct in future provenance.

## Bartender world quests: separate native namespace

`ask one-eyed quest` uses the shared random world-quest system, with minimum
level 11, quota, active-task checks and return-to-original-giver rules. New-task
fees default to 20 copper per level, scaled by difficulty. Map fees are ten
copper per level; abandonment prices vary with level cubed and elapsed time,
and progressed kill tasks require the explicit resign route. Those service
commands are not Quietus's four static officer contracts or daily groups.

New quest fees already use a committed currency callback on supported admission
paths. However, `prepare_currency` currently refuses generic `wallet_spend`
under active accounting: new paid quest/map/abandon requests fail submission
before this callback. This is an explicit economic admission blocker, not proof
that an active player was charged and lost a refund. Existing tasks and their
reward path remain separate. Creation uses a
persisted giver VNUM, bounded target/history search, actual live target counts,
and a unique retained `quest_started` generation. Active item rewards use a
committed grant with player/start/target identity. Higher-level mercenary coin
rewards deliberately remain unavailable under active accounting. Preserve
those existing adapters and explicit refusals rather than treating all world
quests as unported or granting Quietus achievements for their outcomes.

**ZSQ-WORLD-QUEST-SERVICE:** port identified paid-service admission before
advertising new active requests. The callback's refunds from failed creation or
changed state call `ADD_MONEY`, which refuses active cash credits, after prose
says money is returned. Repair this downstream gap together with admission,
including retained payments that settle across activation; plan a recoverable
refund tied to the original committed debit.
Map/abandon payment context contains action/fee/giver but not the original quest
start or target; rechecks use current boolean state. Bind the exact attempt
before adding journal service evidence. Test stale/replaced quests, unavailable
targets, refund denial/recovery and restart. A configured zero new-task fee is
rejected by the wallet delta helper; the checked-in fee is positive. Decide
explicitly whether zero is supported, then test that policy without assuming
current production configuration or silently changing the ordinary price.

## Shipwright and crew hall: explicit economic qualification

The shipwright accepts hull, equipment, weapon, cargo/contraband, rename,
repair, reload and summon commands. Existing hull/slot/capacity/skill/fragment,
location, docked and maintenance conditions are real service prerequisites.
Whole-ship selling refuses immediately; its old mutation tail is unreachable.
Summoning starts a delayed arrival and can clear cargo; issuing the command
does not prove arrival. Customs can confiscate contraband during docking.

Quietus's crew hall at 1734 offers **Quietus Powder Monkeys**, **Experienced
Canoneer** and **Old Quartermaster**, according to the local hire-room tables
in [ship_variables.c](../../../src/ships/ship_variables.c). Hiring needs an
owned ship and the appropriate experience/fragment alternatives. Neither the
town faction nor the distant Magical Automatons unlock is a local hire gate.
Crew selection and money amounts are ship state, not item-delivery objectives.

**ZSQ-SHIP-SERVICE-SETTLEMENT:** `SUB_MONEY` returns failure when accounting is
active, but crew/chief hiring, repairs, reloads, summon, weapon/equipment/cargo
purchases and renaming ignore it and proceed. Cargo/slot sales clear assets
and use `ADD_MONEY`, which refuses an active credit. These are confirmed source
ordering gaps, not an assertion that every deployed ship transaction was used
or failed. Their story integration must wait for an economic repair.

Hull purchases already wait for committed **epic** payment and recheck selected
old hull/cargo/coin balance. The reachable completion still ignores the coin
debit or refund outcome before finishing the hull change. Extend this existing
adapter to coordinated coins/epics/ship publication with recoverable refunds;
do not cite the obsolete direct `buy_hull` tail after unconditional return as
the active failure. Crew/service continuations need stable ship identity,
expected revision, owner, selected product/slot, frozen cost, port, timer and
operation identity. Publish success and any future story event only after the
economic outcome and ship save can be recovered together.

## Qualification and next implementation work

Source-comprehensive mapping is complete. Fresh-world gameplay is unqualified
because [db.c](../../../src/world/db.c) refuses fresh O/P/G/E item generation
under active accounting without reset-generation authority. Recovered stock
may exist; do not report every deployed proof or badge absent.

Qualify these cases before promoting the zone to an active-world pilot:

1. Both credential alternatives at all five recipients, empty/filled badge trees,
   rejected submission and committed replacement recovery; exact nested key parent.
2. Note/head independently incomplete and together sufficient, supplied items,
   drow dagger receipt and replay; no inferred personal kill or learned topic.
3. Supplied Rodev sword and staff without briefings; intermittent sword source,
   roaming staff and competing mansion consumer; correct rewards and NPC departure.
4. Both captain proof services, seal consumption versus dagger replacement,
   supplied bloodstone without prior history, final rewards and remaining gear removal.
5. Captain/Aresliean/bat rare-load episodes, trap occupancy and reset caps;
   current availability distinct from historical encounter/source ownership.
6. Actual open/closed container flags, exact keys, hidden exits, traps, unholy-water
   alignment effects and confirmed movement/retrieval; no loot-only mission success.
7. World-quest namespace, charged failures/refunds, attempt changes, supported
   zero-fee policy, class reward refusal and restart; no duplicate local daily credit.
8. Ship service debit/credit failure, mixed epic/coin hull outcome, changed ownership,
   departure/summon arrival, durable ship save and replay; ordinary shop refusal
   and inn terminal save behavior remain explicit.

Builder decisions remain open for filled-badge behavior, open-but-locked chests,
shared rare-load placement and the coup/rank narrative. Record intent and qualify
the chosen repair; do not erase rare content, add forced kills, or make every
briefing a new achievement to conceal an unsupported capability.
