# Krethik Keep: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence krethik \
  --evidence-format markdown --output docs/reference/zone-story-audits/krethik.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 20003 | `give=I:20017;receive=I:20018;disappear=0` | story: Bring food to Freth | [areas/qst/krethik.qst:8](../../../areas/qst/krethik.qst#L8) |
| 20024 | `give=I:20016;receive=I:20066;disappear=1` | story: Return the troll's stolen totem | [areas/qst/krethik.qst:22](../../../areas/qst/krethik.qst#L22) |
| 20045 | `give=I:20034;receive=C:250000,E:100000;disappear=1` | story: Bring the conspiracy note to the advisor | [areas/qst/krethik.qst:34](../../../areas/qst/krethik.qst#L34) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 20003 | hi hello | [areas/qst/krethik.qst:2](../../../areas/qst/krethik.qst#L2) |
| 20024 | hi help quest | [areas/qst/krethik.qst:16](../../../areas/qst/krethik.qst#L16) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 20000 | `mindbreaker` | [src/specs/specs.assign.c:1290](../../../src/specs/specs.assign.c#L1290) |

## Reset coverage

591 parsed reset commands: D: 56, E: 191, F: 12, G: 24, M: 278, O: 20, P: 10.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
