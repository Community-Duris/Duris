# Tundra: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence tundra \
  --evidence-format markdown --output docs/reference/zone-story-audits/tundra.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 13703 | `give=I:13708,I:13709,I:13710,I:13711;receive=I:13713;disappear=0` | story: Bone Fort Inn: return four different books | [areas/qst/tundra.qst:40](../../../areas/qst/tundra.qst#L40) |
| 13710 | `give=C:500000,I:13714;receive=I:13715;disappear=0` | service: The blacksmith: order red scale armor | [areas/qst/tundra.qst:66](../../../areas/qst/tundra.qst#L66) |
| 13716 | `give=I:13713;receive=I:13712;disappear=0` | story: Eleadora: return the snowy adventurer boots | [areas/qst/tundra.qst:109](../../../areas/qst/tundra.qst#L109) |
| 13716 | `give=I:13722;receive=I:13720;disappear=1` | story: Eleadora: bring Malinar’s bloody head | [areas/qst/tundra.qst:120](../../../areas/qst/tundra.qst#L120) |
| 13722 | `give=I:43137;receive=C:85000,E:55000;disappear=0` | story: The shaman: deliver a snapjaw turtle shell | [areas/qst/tundra.qst:144](../../../areas/qst/tundra.qst#L144) |
| 13722 | `give=I:43138;receive=C:85000,E:65000;disappear=0` | story: The shaman: deliver a fire gland | [areas/qst/tundra.qst:139](../../../areas/qst/tundra.qst#L139) |
| 13723 | `give=I:318,I:319,I:334;receive=I:13705;disappear=0` | story: The village: feed the starving barbarian | [areas/qst/tundra.qst:157](../../../areas/qst/tundra.qst#L157) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 13703 | hi | [areas/qst/tundra.qst:2](../../../areas/qst/tundra.qst#L2) |
| 13703 | books | [areas/qst/tundra.qst:9](../../../areas/qst/tundra.qst#L9) |
| 13703 | guests | [areas/qst/tundra.qst:21](../../../areas/qst/tundra.qst#L21) |
| 13703 | boots | [areas/qst/tundra.qst:33](../../../areas/qst/tundra.qst#L33) |
| 13710 | hi | [areas/qst/tundra.qst:53](../../../areas/qst/tundra.qst#L53) |
| 13710 | scales dragon | [areas/qst/tundra.qst:58](../../../areas/qst/tundra.qst#L58) |
| 13716 | hi | [areas/qst/tundra.qst:74](../../../areas/qst/tundra.qst#L74) |
| 13716 | boots boot | [areas/qst/tundra.qst:81](../../../areas/qst/tundra.qst#L81) |
| 13716 | malinar | [areas/qst/tundra.qst:91](../../../areas/qst/tundra.qst#L91) |
| 13716 | sword | [areas/qst/tundra.qst:102](../../../areas/qst/tundra.qst#L102) |
| 13722 | hi | [areas/qst/tundra.qst:133](../../../areas/qst/tundra.qst#L133) |
| 13723 | hi hello hey | [areas/qst/tundra.qst:151](../../../areas/qst/tundra.qst#L151) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| room | 13714 | `inn` | [src/specs/specs.assign.c:2459](../../../src/specs/specs.assign.c#L2459) |

## Reset coverage

246 parsed reset commands: D: 36, E: 6, G: 16, M: 180, O: 8.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
