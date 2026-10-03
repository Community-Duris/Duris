# The Jade Empire: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence jade \
  --evidence-format markdown --output docs/reference/zone-story-audits/jade.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 76603 | `give=I:318;receive=C:10000;disappear=0` | request: One Fish for the Fisherman | [areas/qst/jade.qst:9](../../../areas/qst/jade.qst#L9) |
| 76603 | `give=I:319;receive=C:10000;disappear=0` | request: One Fish for the Fisherman | [areas/qst/jade.qst:14](../../../areas/qst/jade.qst#L14) |
| 76606 | `give=I:76619,I:76619,I:76619,I:76619;receive=C:50000,E:80000;disappear=0` | request: Four Strips for the Hunter | [areas/qst/jade.qst:32](../../../areas/qst/jade.qst#L32) |
| 76606 | `give=I:76620;receive=C:0;disappear=1` | Excluded: Giving the huge boar hide upsets and retires the hunter; this rejection has a zero-copper reward and is not a successful request. | [areas/qst/jade.qst:21](../../../areas/qst/jade.qst#L21) |
| 76623 | `give=I:76626;receive=I:76635;disappear=0` | service: Fold a Rice Paper Hat | [areas/qst/jade.qst:44](../../../areas/qst/jade.qst#L44) |
| 76629 | `give=I:76624;receive=I:76652;disappear=0` | service: Cook Turtle Soup | [areas/qst/jade.qst:61](../../../areas/qst/jade.qst#L61) |
| 76629 | `give=I:76626;receive=I:76651;disappear=0` | service: Make Rice Sushi | [areas/qst/jade.qst:57](../../../areas/qst/jade.qst#L57) |
| 76630 | `give=I:76608,I:76608,I:76608,I:76608,I:76608;receive=I:76607;disappear=0` | story: Five Portions for a Harvest Bag | [areas/qst/jade.qst:71](../../../areas/qst/jade.qst#L71) |
| 76654 | `give=C:10000,I:76635;receive=I:76636;disappear=0` | service: Enchant the Rice Paper Hat | [areas/qst/jade.qst:82](../../../areas/qst/jade.qst#L82) |
| 76654 | `give=C:10000,I:76724;receive=I:76725;disappear=0` | service: Enchant the Simple Necktie | [areas/qst/jade.qst:92](../../../areas/qst/jade.qst#L92) |
| 76654 | `give=I:76706;receive=;disappear=0` | Excluded: The white silk belt exchange consumes the offering without declared reward or explanation; exclude this unfinished outcome until builder review. | [areas/qst/jade.qst:89](../../../areas/qst/jade.qst#L89) |
| 76656 | `give=I:76678,I:76690,I:76690,I:76692,I:76693;receive=I:76703;disappear=0` | request: The Gardener's Seed Bouquet | [areas/qst/jade.qst:99](../../../areas/qst/jade.qst#L99) |
| 76657 | `give=I:76607;receive=I:76626;disappear=0` | service: Grind the Harvest Bag | [areas/qst/jade.qst:109](../../../areas/qst/jade.qst#L109) |
| 76658 | `give=I:76623;receive=I:76624;disappear=1` | story: Free the Entangled Turtle | [areas/qst/jade.qst:119](../../../areas/qst/jade.qst#L119) |
| 76659 | `give=I:76626;receive=I:76628;disappear=0` | service: A Drink from Ground Rice | [areas/qst/jade.qst:129](../../../areas/qst/jade.qst#L129) |
| 76660 | `give=I:76615;receive=I:76707;disappear=1` | request: Warn the Soldier of Fortune | [areas/qst/jade.qst:140](../../../areas/qst/jade.qst#L140) |
| 76665 | `give=I:76704;receive=C:200000;disappear=1` | story: The Outlaw's Bounty | [areas/qst/jade.qst:156](../../../areas/qst/jade.qst#L156) |
| 76668 | `give=C:10000;receive=I:76617;disappear=0` | service: Buy a Capture Net | [areas/qst/jade.qst:166](../../../areas/qst/jade.qst#L166) |
| 76668 | `give=I:76624;receive=I:222,I:222;disappear=0` | service: Exchange a Turtle for Silver Ore | [areas/qst/jade.qst:180](../../../areas/qst/jade.qst#L180) |
| 76668 | `give=I:76644;receive=C:100000;disappear=0` | request: A Razorback for the Tamer | [areas/qst/jade.qst:170](../../../areas/qst/jade.qst#L170) |
| 76669 | `give=I:76665,I:76666;receive=I:76667;disappear=0` | story: Earn the Royal Invitation | [areas/qst/jade.qst:199](../../../areas/qst/jade.qst#L199) |
| 76669 | `give=I:76665;receive=I:76665;disappear=0` | service: The Daimyo's Heart Briefing | [areas/qst/jade.qst:189](../../../areas/qst/jade.qst#L189) |
| 76674 | `give=I:76617;receive=I:76644;disappear=1` | service: Prepare a Captured Razorback | [areas/qst/jade.qst:210](../../../areas/qst/jade.qst#L210) |
| 76679 | `give=I:76708,I:76711,I:76712,I:76713;receive=I:76649,I:76723;disappear=0` | story: Four Directional Holy Relics | [areas/qst/jade.qst:221](../../../areas/qst/jade.qst#L221) |
| 76684 | `give=I:76654;receive=I:76663;disappear=1` | story: The Woman's Family Heirloom | [areas/qst/jade.qst:231](../../../areas/qst/jade.qst#L231) |
| 76688 | `give=I:67116,I:76730;receive=I:32019,I:32019,I:55324;disappear=0` | story: Macavor's Two Proofs | [areas/qst/jade.qst:268](../../../areas/qst/jade.qst#L268) |
| 76691 | `give=C:10000,I:76659,I:76660;receive=I:76699;disappear=0` | service: Forge Miasma | [areas/qst/jade.qst:282](../../../areas/qst/jade.qst#L282) |
| 76691 | `give=C:10000,I:76659,I:76679;receive=I:76720;disappear=0` | service: Forge the Electium Chestplate | [areas/qst/jade.qst:292](../../../areas/qst/jade.qst#L292) |
| 76691 | `give=I:233;receive=I:76700;disappear=0` | service: Refine Inferior Mithril | [areas/qst/jade.qst:288](../../../areas/qst/jade.qst#L288) |
| 76691 | `give=I:76657,I:76658;receive=I:76659;disappear=0` | service: Combine Electrum and Adamantium | [areas/qst/jade.qst:277](../../../areas/qst/jade.qst#L277) |
| 76693 | `give=I:76687;receive=I:76648,I:76726;disappear=0` | request: The Chamberlain's Mankiller Proof | [areas/qst/jade.qst:300](../../../areas/qst/jade.qst#L300) |
| 76696 | `give=I:76617;receive=I:76704;disappear=1` | service: Prepare a Captured Outlaw | [areas/qst/jade.qst:307](../../../areas/qst/jade.qst#L307) |
| 76697 | `give=I:76667;receive=I:76689;disappear=0` | service: Exchange the Royal Invitation | [areas/qst/jade.qst:317](../../../areas/qst/jade.qst#L317) |
| 76699 | `give=I:76676;receive=C:10000,E:10000;disappear=1` | request: A Map for the Lost Legionaire | [areas/qst/jade.qst:326](../../../areas/qst/jade.qst#L326) |
| 76711 | `give=I:77205;receive=I:55372,I:76722;disappear=0` | story: The Princess's Royal Token | [areas/qst/jade.qst:337](../../../areas/qst/jade.qst#L337) |
| 76715 | `give=I:76688;receive=I:76729;disappear=0` | request: The Bloodstone Delegate's Proof | [areas/qst/jade.qst:347](../../../areas/qst/jade.qst#L347) |
| 76718 | `give=I:77200;receive=E:30000;disappear=1` | request: Report the Stowaway's Purse | [areas/qst/jade.qst:353](../../../areas/qst/jade.qst#L353) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 76688 | hi hello mande | [areas/qst/jade.qst:244](../../../areas/qst/jade.qst#L244) |
| 76688 | mande | [areas/qst/jade.qst:254](../../../areas/qst/jade.qst#L254) |
| 76688 | troggahn | [areas/qst/jade.qst:259](../../../areas/qst/jade.qst#L259) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| room | 76859 | `crew_shop_proc` | [src/specs/specs.assign.c:2418](../../../src/specs/specs.assign.c#L2418) |
| room | 76659 | `ship_shop_proc` | [src/specs/specs.assign.c:2454](../../../src/specs/specs.assign.c#L2454) |

## Reset coverage

583 parsed reset commands: D: 76, E: 71, F: 4, G: 30, M: 298, O: 70, P: 34.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
