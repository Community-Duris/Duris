# Mountain Tracts of the Untamed: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence mountaintracks \
  --evidence-format markdown --output docs/reference/zone-story-audits/mountaintracks.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 20928 | `give=I:20923;receive=I:20930;disappear=0` | story: Tall monk: the small carved piece | [areas/qst/mountaintracks.qst:13](../../../areas/qst/mountaintracks.qst#L13) |
| 20981 | `give=I:20954;receive=I:20955;disappear=0` | story: Half-orc mage: reclaim the marble piece | [areas/qst/mountaintracks.qst:34](../../../areas/qst/mountaintracks.qst#L34) |
| 20983 | `give=I:20947,I:20948;receive=I:20949;disappear=0` | story: Futni: mix the explorer’s potion | [areas/qst/mountaintracks.qst:57](../../../areas/qst/mountaintracks.qst#L57) |
| 20984 | `give=I:20949;receive=I:20950;disappear=0` | story: Bumble: a potion for the explorer | [areas/qst/mountaintracks.qst:85](../../../areas/qst/mountaintracks.qst#L85) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 20928 | statue astansus underdark hi | [areas/qst/mountaintracks.qst:2](../../../areas/qst/mountaintracks.qst#L2) |
| 20981 | hi half orc half-orc | [areas/qst/mountaintracks.qst:19](../../../areas/qst/mountaintracks.qst#L19) |
| 20983 | hi quest elementalist potion drow futni | [areas/qst/mountaintracks.qst:40](../../../areas/qst/mountaintracks.qst#L40) |
| 20984 | hi quest bumble explorer potion | [areas/qst/mountaintracks.qst:70](../../../areas/qst/mountaintracks.qst#L70) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

259 parsed reset commands: D: 40, E: 16, G: 12, M: 173, O: 18.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
