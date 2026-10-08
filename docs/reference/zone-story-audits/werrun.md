# Village of Werrun: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence werrun \
  --evidence-format markdown --output docs/reference/zone-story-audits/werrun.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 38301 | `give=I:38311;receive=C:12345,I:38308;disappear=0` | story: Malfun: another book by Revan | [areas/qst/werrun.qst:80](../../../areas/qst/werrun.qst#L80) |
| 38301 | `give=I:38312;receive=I:38313;disappear=0` | story: Malfun: Lady Sklera's keepsake | [areas/qst/werrun.qst:91](../../../areas/qst/werrun.qst#L91) |
| 38305 | `give=C:350000;receive=I:38304;disappear=0` | story: The mage: help with his debt | [areas/qst/werrun.qst:144](../../../areas/qst/werrun.qst#L144) |
| 38308 | `give=I:38326;receive=E:35000,I:38327;disappear=1` | story: The traveller: Vewon's writing | [areas/qst/werrun.qst:160](../../../areas/qst/werrun.qst#L160) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 38301 | hello hi | [areas/qst/werrun.qst:2](../../../areas/qst/werrun.qst#L2) |
| 38301 | yes | [areas/qst/werrun.qst:8](../../../areas/qst/werrun.qst#L8) |
| 38301 | wife young | [areas/qst/werrun.qst:17](../../../areas/qst/werrun.qst#L17) |
| 38301 | trouble | [areas/qst/werrun.qst:25](../../../areas/qst/werrun.qst#L25) |
| 38301 | task | [areas/qst/werrun.qst:34](../../../areas/qst/werrun.qst#L34) |
| 38301 | book | [areas/qst/werrun.qst:41](../../../areas/qst/werrun.qst#L41) |
| 38301 | revan | [areas/qst/werrun.qst:47](../../../areas/qst/werrun.qst#L47) |
| 38301 | vewon etsh | [areas/qst/werrun.qst:55](../../../areas/qst/werrun.qst#L55) |
| 38301 | delwyn | [areas/qst/werrun.qst:63](../../../areas/qst/werrun.qst#L63) |
| 38301 | scrolls | [areas/qst/werrun.qst:71](../../../areas/qst/werrun.qst#L71) |
| 38302 | hello | [areas/qst/werrun.qst:105](../../../areas/qst/werrun.qst#L105) |
| 38302 | stable werrun | [areas/qst/werrun.qst:111](../../../areas/qst/werrun.qst#L111) |
| 38305 | werrun | [areas/qst/werrun.qst:122](../../../areas/qst/werrun.qst#L122) |
| 38305 | help | [areas/qst/werrun.qst:129](../../../areas/qst/werrun.qst#L129) |
| 38305 | broke money | [areas/qst/werrun.qst:137](../../../areas/qst/werrun.qst#L137) |
| 38308 | vewon | [areas/qst/werrun.qst:154](../../../areas/qst/werrun.qst#L154) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 38309 | `world_quest` | [src/specs/specs.assign.c:792](../../../src/specs/specs.assign.c#L792) |

## Reset coverage

132 parsed reset commands: D: 20, E: 16, F: 2, G: 11, M: 57, O: 20, P: 5, R: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
