# Abandoned Elven Homestead: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence elvish \
  --evidence-format markdown --output docs/reference/zone-story-audits/elvish.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 35800 | `give=I:35813;receive=E:50000,I:35820;disappear=1` | story: Release the Cursed Drider | [areas/qst/elvish.qst:35](../../../areas/qst/elvish.qst#L35) |
| 35800 | `give=I:35817,I:35818;receive=I:35814,I:35816;disappear=0` | service: Prepare the Two Statues | [areas/qst/elvish.qst:55](../../../areas/qst/elvish.qst#L55) |
| 35801 | `give=I:35812,I:35812,I:35812,I:35812;receive=E:30000,I:35824;disappear=0` | request: The Spider Eggs and the Hidden Key | [areas/qst/elvish.qst:76](../../../areas/qst/elvish.qst#L76) |
| 35801 | `give=I:35814,I:35816;receive=I:35813;disappear=0` | service: Combine the Statues | [areas/qst/elvish.qst:96](../../../areas/qst/elvish.qst#L96) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 35800 | life quest | [areas/qst/elvish.qst:2](../../../areas/qst/elvish.qst#L2) |
| 35800 | fighter great | [areas/qst/elvish.qst:12](../../../areas/qst/elvish.qst#L12) |
| 35800 | help past | [areas/qst/elvish.qst:16](../../../areas/qst/elvish.qst#L16) |
| 35800 | yes | [areas/qst/elvish.qst:25](../../../areas/qst/elvish.qst#L25) |
| 35800 | no | [areas/qst/elvish.qst:29](../../../areas/qst/elvish.qst#L29) |
| 35801 | spiders spider key | [areas/qst/elvish.qst:71](../../../areas/qst/elvish.qst#L71) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

138 parsed reset commands: D: 20, E: 6, F: 12, G: 8, M: 65, O: 11, P: 16.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
