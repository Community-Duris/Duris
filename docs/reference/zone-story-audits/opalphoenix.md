# Enclave of the Opal Phoenix: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence opalphoenix \
  --evidence-format markdown --output docs/reference/zone-story-audits/opalphoenix.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 70801 | `give=I:70823;receive=E:40000,I:70811,I:70812;disappear=1` | story: Alazia: deliver the student’s sand | [areas/qst/opalphoenix.qst:14](../../../areas/qst/opalphoenix.qst#L14) |
| 70802 | `give=I:70812;receive=E:20000,I:70823;disappear=1` | story: The student: recover the lost quill | [areas/qst/opalphoenix.qst:34](../../../areas/qst/opalphoenix.qst#L34) |
| 70815 | `give=I:70819;receive=I:70820;disappear=0` | story: The old woman: bring bear remains | [areas/qst/opalphoenix.qst:62](../../../areas/qst/opalphoenix.qst#L62) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 70801 | hi hello sand | [areas/qst/opalphoenix.qst:2](../../../areas/qst/opalphoenix.qst#L2) |
| 70802 | hi hello lost something | [areas/qst/opalphoenix.qst:29](../../../areas/qst/opalphoenix.qst#L29) |
| 70815 | elves | [areas/qst/opalphoenix.qst:52](../../../areas/qst/opalphoenix.qst#L52) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 70549 | `circlet_of_light` | [src/specs/specs.assign.c:1494](../../../src/specs/specs.assign.c#L1494) |
| obj | 70554 | `ljs_sword` | [src/specs/specs.assign.c:1495](../../../src/specs/specs.assign.c#L1495) |
| obj | 70556 | `wuss_sword` | [src/specs/specs.assign.c:1496](../../../src/specs/specs.assign.c#L1496) |
| obj | 70558 | `head_guard_sword` | [src/specs/specs.assign.c:1497](../../../src/specs/specs.assign.c#L1497) |
| obj | 70559 | `priest_rudder` | [src/specs/specs.assign.c:1498](../../../src/specs/specs.assign.c#L1498) |
| obj | 70565 | `alch_bag` | [src/specs/specs.assign.c:1499](../../../src/specs/specs.assign.c#L1499) |
| obj | 70568 | `alch_rod` | [src/specs/specs.assign.c:1500](../../../src/specs/specs.assign.c#L1500) |
| obj | 70571 | `ljs_armor` | [src/specs/specs.assign.c:1501](../../../src/specs/specs.assign.c#L1501) |
| obj | 70572 | `dragon_skull_helm` | [src/specs/specs.assign.c:1502](../../../src/specs/specs.assign.c#L1502) |
| mob | 70535 | `long_john_silver_shout` | [src/specs/specs.assign.c:1507](../../../src/specs/specs.assign.c#L1507) |
| mob | 70542 | `undead_parrot` | [src/specs/specs.assign.c:1508](../../../src/specs/specs.assign.c#L1508) |
| mob | 70546 | `undead_dragon_east` | [src/specs/specs.assign.c:1509](../../../src/specs/specs.assign.c#L1509) |
| mob | 70552 | `pirate_cabinboy_talk` | [src/specs/specs.assign.c:1512](../../../src/specs/specs.assign.c#L1512) |
| mob | 70554 | `pirate_female_talk` | [src/specs/specs.assign.c:1513](../../../src/specs/specs.assign.c#L1513) |
| mob | 70502 | `pirate_talk` | [src/specs/specs.assign.c:1514](../../../src/specs/specs.assign.c#L1514) |
| mob | 70503 | `pirate_talk` | [src/specs/specs.assign.c:1515](../../../src/specs/specs.assign.c#L1515) |
| mob | 70539 | `pirate_talk` | [src/specs/specs.assign.c:1516](../../../src/specs/specs.assign.c#L1516) |
| mob | 70540 | `pirate_talk` | [src/specs/specs.assign.c:1517](../../../src/specs/specs.assign.c#L1517) |
| mob | 70541 | `pirate_talk` | [src/specs/specs.assign.c:1518](../../../src/specs/specs.assign.c#L1518) |
| mob | 70549 | `pirate_talk` | [src/specs/specs.assign.c:1519](../../../src/specs/specs.assign.c#L1519) |
| mob | 70551 | `pirate_talk` | [src/specs/specs.assign.c:1520](../../../src/specs/specs.assign.c#L1520) |
| mob | 70561 | `pirate_talk` | [src/specs/specs.assign.c:1521](../../../src/specs/specs.assign.c#L1521) |
| room | 70501 | `ship_shop_proc` | [src/specs/specs.assign.c:2384](../../../src/specs/specs.assign.c#L2384) |

## Reset coverage

84 parsed reset commands: D: 12, E: 4, G: 13, M: 50, O: 3, P: 2.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
